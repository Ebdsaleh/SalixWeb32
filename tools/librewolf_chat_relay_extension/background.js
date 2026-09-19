"use strict";

const WORKER_BASE = "http://127.0.0.1:8766";
const EXTENSION_PROTOCOL = "SALIX-CHAT-EXTENSION/1";
const EXTENSION_VERSION = "0.1.0";

let commandBusy = false;

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

    if (
      !command ||
      command.protocol !== EXTENSION_PROTOCOL ||
      command.command !== "send_message" ||
      !Number.isInteger(command.request_id) ||
      typeof command.text !== "string" ||
      !command.text
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
        text: command.text
      });
    } catch (exception) {
      await postFailure(
        command.request_id,
        "Could not communicate with the ChatGPT tab: " + String(exception)
      );
      return;
    }

    if (!result || result.ok !== true || typeof result.text !== "string") {
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

      await postJson("/v1/result", {
        protocol: EXTENSION_PROTOCOL,
        request_id: command.request_id,
        text: result.text,
        timing: timing
      });
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
