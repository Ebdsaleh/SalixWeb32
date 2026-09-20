"use strict";

const WORKER_BASE = "http://127.0.0.1:8766";
const EXTENSION_PROTOCOL = "SALIX-CHAT-EXTENSION/1";
const EXTENSION_VERSION = "0.2.3";

let commandBusy = false;
let activeDownloadCapture = null;

function sleep(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

browser.downloads.onCreated.addListener((download) => {
  if (
    activeDownloadCapture &&
    activeDownloadCapture.download_id === null
  ) {
    activeDownloadCapture.download_id = download.id;
  }
});

async function waitForDownloadCapture(timeoutMilliseconds) {
  const capture = activeDownloadCapture;
  if (!capture) {
    return {
      ok: false,
      error: "No assistant download capture is active."
    };
  }

  const deadline = Date.now() + timeoutMilliseconds;

  while (Date.now() < deadline) {
    if (capture.download_id !== null) {
      const matches = await browser.downloads.search({
        id: capture.download_id
      });

      const item = matches.length ? matches[0] : null;
      if (item) {
        if (item.state === "complete") {
          activeDownloadCapture = null;
          return {
            ok: true,
            download_id: item.id,
            filename: item.filename || "",
            mime_type: item.mime || "application/octet-stream",
            file_size:
              typeof item.fileSize === "number"
                ? item.fileSize
                : -1
          };
        }

        if (item.state === "interrupted") {
          activeDownloadCapture = null;
          return {
            ok: false,
            error: item.error || "Assistant file download was interrupted."
          };
        }
      }
    }

    await sleep(100);
  }

  activeDownloadCapture = null;
  return {
    ok: false,
    error: "Timed out waiting for assistant file download."
  };
}

browser.runtime.onMessage.addListener((message) => {
  if (!message || typeof message.type !== "string") {
    return undefined;
  }

  if (message.type === "salix_prepare_download_capture") {
    activeDownloadCapture = {
      started_at: Date.now(),
      download_id: null
    };

    return Promise.resolve({ ok: true });
  }

  if (message.type === "salix_wait_download_capture") {
    const requestedTimeout = Number(message.timeout_ms);
    const timeoutMilliseconds = (
      Number.isFinite(requestedTimeout) &&
      requestedTimeout >= 1000 &&
      requestedTimeout <= 60000
    ) ? Math.round(requestedTimeout) : 30000;

    return waitForDownloadCapture(timeoutMilliseconds);
  }

  if (message.type === "salix_cancel_download_capture") {
    activeDownloadCapture = null;
    return Promise.resolve({ ok: true });
  }

  return undefined;
});

async function findChatTab() {
  const tabs = await browser.tabs.query({
    url: [
      "https://chatgpt.com/*",
      "https://www.chatgpt.com/*"
    ]
  });

  if (!tabs.length) {
    return null;
  }

  const active = tabs.find((tab) => tab.active);
  return active || tabs[0];
}

async function postJson(path, body) {
  const response = await fetch(WORKER_BASE + path, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(body),
    cache: "no-store"
  });

  if (!response.ok) {
    throw new Error(path + " returned HTTP " + response.status);
  }

  return response;
}

async function sendHeartbeat() {
  let composerReady = false;
  let currentUrl = "";
  let title = "";
  let error = "";

  try {
    const tab = await findChatTab();

    if (tab) {
      currentUrl = tab.url || "";
      title = tab.title || "";

      const status = await browser.tabs.sendMessage(tab.id, {
        type: "salix_status"
      });

      composerReady = !!(status && status.composer_ready);
      if (status && status.error) {
        error = String(status.error);
      }
    } else {
      error = "no_chatgpt_tab";
    }
  } catch (exception) {
    error = String(exception);
  }

  try {
    await postJson("/v1/heartbeat", {
      protocol: EXTENSION_PROTOCOL,
      extension_version: EXTENSION_VERSION,
      composer_ready: composerReady,
      current_url: currentUrl,
      title: title,
      error: error
    });
  } catch (_exception) {
    // The localhost worker may not be running yet. Keep polling quietly.
  }
}

async function postFailure(requestId, detail) {
  try {
    await postJson("/v1/failure", {
      protocol: EXTENSION_PROTOCOL,
      request_id: requestId,
      error: detail || "unknown extension failure"
    });
  } catch (_exception) {
    // If the worker disappeared there is nowhere useful to report the failure.
  }
}

async function processCommand() {
  if (commandBusy) {
    return;
  }

  commandBusy = true;

  try {
    const response = await fetch(WORKER_BASE + "/v1/command", {
      method: "GET",
      cache: "no-store"
    });

    if (response.status === 204) {
      return;
    }

    if (!response.ok) {
      return;
    }

    const command = await response.json();

    const commandAttachments = (
      command &&
      Array.isArray(command.attachments)
    ) ? command.attachments : [];

    if (
      !command ||
      command.protocol !== EXTENSION_PROTOCOL ||
      command.command !== "send_message" ||
      !Number.isInteger(command.request_id) ||
      typeof command.text !== "string" ||
      (!command.text && !commandAttachments.length)
    ) {
      return;
    }

    const commandStartedAt = performance.now();
    const tab = await findChatTab();

    if (!tab) {
      await postFailure(
        command.request_id,
        "No ChatGPT tab is open in LibreWolf."
      );
      return;
    }

    let result;

    try {
      result = await browser.tabs.sendMessage(tab.id, {
        type: "salix_send_message",
        request_id: command.request_id,
        text: command.text,
        attachments: commandAttachments
      });
    } catch (exception) {
      await postFailure(
        command.request_id,
        "Could not communicate with the ChatGPT tab: " + String(exception)
      );
      return;
    }

    if (
      !result ||
      result.ok !== true ||
      typeof result.text !== "string" ||
      !Array.isArray(result.attachments || [])
    ) {
      await postFailure(
        command.request_id,
        result && result.error
          ? String(result.error)
          : "ChatGPT content script did not return a response."
      );
      return;
    }

    try {
      const backgroundTotalMs = Math.max(
        0,
        Math.round(performance.now() - commandStartedAt)
      );
      const timing = (
        result.timing &&
        typeof result.timing === "object"
      ) ? result.timing : {};

      timing.background_total_ms = backgroundTotalMs;

      const resultAttachments = result.attachments || [];

      await postJson("/v1/result", {
        protocol: EXTENSION_PROTOCOL,
        request_id: command.request_id,
        text: result.text,
        attachments: resultAttachments,
        attachment_debug:
          (
            result.attachment_debug &&
            typeof result.attachment_debug === "object"
          ) ? result.attachment_debug : {},
        timing: timing
      });

      for (const attachment of resultAttachments) {
        const downloadId = (
          attachment &&
          Number.isInteger(attachment.download_id)
        ) ? attachment.download_id : null;

        if (downloadId !== null) {
          try {
            await browser.downloads.removeFile(downloadId);
          } catch (_exception) {
            // Cleanup is best-effort; the relay result has already been accepted.
          }

          try {
            await browser.downloads.erase({ id: downloadId });
          } catch (_exception) {
            // Keep cleanup failures non-fatal.
          }
        }
      }
    } catch (exception) {
      await postFailure(
        command.request_id,
        "Could not return assistant text to localhost worker: " +
          String(exception)
      );
    }
  } catch (_exception) {
    // The localhost worker may not be running yet.
  } finally {
    commandBusy = false;
  }
}

setInterval(() => {
  void sendHeartbeat();
}, 1000);

setInterval(() => {
  void processCommand();
}, 500);

void sendHeartbeat();
void processCommand();
