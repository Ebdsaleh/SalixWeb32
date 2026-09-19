"use strict";

const RESPONSE_TIMEOUT_MS = 180000;
const RESPONSE_POLL_MS = 250;
const RESPONSE_STABLE_MS = 2000;
const ATTACHMENT_UPLOAD_TIMEOUT_MS = 30000;
const MAX_ATTACHMENT_COUNT = 8;
const MAX_ATTACHMENT_BYTES = 2 * 1024 * 1024;

function visible(element) {
  if (!element) {
    return false;
  }

  const rect = element.getBoundingClientRect();
  return rect.width > 0 && rect.height > 0;
}

function findComposer() {
  const selectors = [
    "textarea[name='prompt-textarea']",
    "textarea[data-testid='prompt-textarea']",
    "#prompt-textarea",
    "[contenteditable='true'][data-testid='prompt-textarea']",
    "div[contenteditable='true']"
  ];

  for (const selector of selectors) {
    const elements = Array.from(document.querySelectorAll(selector));

    for (const element of elements) {
      if (visible(element) && !element.disabled) {
        return element;
      }
    }
  }

  return null;
}

function cleanNodeText(node) {
  if (!node) {
    return "";
  }

  const clone = node.cloneNode(true);

  clone.querySelectorAll(
    "button, svg, [aria-hidden='true'], [data-testid='copy-turn-action-button']"
  ).forEach((child) => child.remove());

  return (clone.innerText || clone.textContent || "").trim();
}

function assistantNodes() {
  const roleNodes = Array.from(
    document.querySelectorAll("[data-message-author-role='assistant']")
  ).filter(visible);

  if (roleNodes.length) {
    return roleNodes;
  }

  return Array.from(
    document.querySelectorAll("article[data-testid^='conversation-turn-']")
  ).filter((turn) => !!turn.querySelector(".markdown"));
}

function assistantSnapshots() {
  return assistantNodes()
    .map((node) => {
      const markdown = node.querySelector(".markdown");
      return cleanNodeText(markdown || node);
    })
    .filter(Boolean);
}

function generationActive() {
  const selectors = [
    "button[data-testid='stop-button']",
    "button[aria-label='Stop generating']",
    "button[aria-label='Stop streaming']"
  ];

  return selectors.some((selector) =>
    Array.from(document.querySelectorAll(selector)).some(
      (node) => visible(node) && !node.disabled
    )
  );
}

function dispatchInput(element, data) {
  let event;

  try {
    event = new InputEvent("input", {
      bubbles: true,
      inputType: "insertText",
      data: data
    });
  } catch (_exception) {
    event = new Event("input", { bubbles: true });
  }

  element.dispatchEvent(event);
}

function setTextareaText(element, text) {
  const descriptor = Object.getOwnPropertyDescriptor(
    HTMLTextAreaElement.prototype,
    "value"
  );

  if (descriptor && descriptor.set) {
    descriptor.set.call(element, text);
  } else {
    element.value = text;
  }

  dispatchInput(element, text);
  element.dispatchEvent(new Event("change", { bubbles: true }));
}

function setContentEditableText(element, text) {
  element.focus();

  const selection = window.getSelection();
  if (selection) {
    const range = document.createRange();
    range.selectNodeContents(element);
    selection.removeAllRanges();
    selection.addRange(range);
  }

  let inserted = false;

  try {
    inserted = document.execCommand("insertText", false, text);
  } catch (_exception) {
    inserted = false;
  }

  if (!inserted) {
    element.textContent = text;
    dispatchInput(element, text);
  }
}

function setComposerText(composer, text) {
  composer.focus();

  if (composer instanceof HTMLTextAreaElement) {
    setTextareaText(composer, text);
  } else {
    setContentEditableText(composer, text);
  }
}

function findSendButton() {
  const selectors = [
    "button[data-testid='send-button']",
    "button[aria-label='Send prompt']",
    "button[aria-label='Send message']"
  ];

  for (const selector of selectors) {
    const buttons = Array.from(document.querySelectorAll(selector));

    for (const button of buttons) {
      if (visible(button) && !button.disabled) {
        return button;
      }
    }
  }

  return null;
}

function findFileInput() {
  const inputs = Array.from(
    document.querySelectorAll("input[type='file']")
  );

  return inputs.find((input) => !input.disabled) || null;
}

function decodeBase64(value) {
  const binary = atob(value || "");
  const bytes = new Uint8Array(binary.length);

  for (let index = 0; index < binary.length; ++index) {
    bytes[index] = binary.charCodeAt(index) & 0xFF;
  }

  return bytes;
}

function encodeBase64(bytes) {
  let binary = "";
  const chunkSize = 0x8000;

  for (let offset = 0; offset < bytes.length; offset += chunkSize) {
    const chunk = bytes.subarray(
      offset,
      Math.min(offset + chunkSize, bytes.length)
    );

    let part = "";
    for (let index = 0; index < chunk.length; ++index) {
      part += String.fromCharCode(chunk[index]);
    }
    binary += part;
  }

  return btoa(binary);
}

function buildFiles(attachments) {
  if (!Array.isArray(attachments)) {
    return [];
  }

  if (attachments.length > MAX_ATTACHMENT_COUNT) {
    throw new Error("Too many Salix attachments.");
  }

  return attachments.map((attachment) => {
    if (
      !attachment ||
      typeof attachment.name !== "string" ||
      !attachment.name ||
      typeof attachment.data_base64 !== "string"
    ) {
      throw new Error("Salix attachment descriptor is invalid.");
    }

    const bytes = decodeBase64(attachment.data_base64);
    if (bytes.length > MAX_ATTACHMENT_BYTES) {
      throw new Error("Salix attachment exceeds 2 MB relay limit.");
    }

    return new File(
      [bytes],
      attachment.name,
      {
        type: (
          typeof attachment.mime_type === "string" &&
          attachment.mime_type
        ) ? attachment.mime_type : "application/octet-stream"
      }
    );
  });
}

function dispatchDrop(target, dataTransfer) {
  const eventNames = ["dragenter", "dragover", "drop"];

  for (const eventName of eventNames) {
    let event;

    try {
      event = new DragEvent(eventName, {
        bubbles: true,
        cancelable: true,
        dataTransfer: dataTransfer
      });
    } catch (_exception) {
      event = new Event(eventName, {
        bubbles: true,
        cancelable: true
      });

      try {
        Object.defineProperty(event, "dataTransfer", {
          value: dataTransfer
        });
      } catch (_propertyException) {
        return false;
      }
    }

    target.dispatchEvent(event);
  }

  return true;
}

async function injectAttachments(composer, attachments) {
  const files = buildFiles(attachments);
  if (!files.length) {
    return;
  }

  let dataTransfer;
  try {
    dataTransfer = new DataTransfer();
  } catch (_exception) {
    throw new Error("This LibreWolf build cannot construct file transfer data.");
  }

  for (const file of files) {
    dataTransfer.items.add(file);
  }

  let installed = false;
  const input = findFileInput();

  if (input) {
    try {
      input.files = dataTransfer.files;
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
      installed = true;
    } catch (_exception) {
      installed = false;
    }
  }

  if (!installed) {
    const dropTarget =
      composer.closest("form") ||
      composer.parentElement ||
      composer;

    installed = dispatchDrop(dropTarget, dataTransfer);
  }

  if (!installed) {
    throw new Error(
      "ChatGPT file-upload control is not available in the current page."
    );
  }

  const started = Date.now();

  while (Date.now() - started < ATTACHMENT_UPLOAD_TIMEOUT_MS) {
    const sendButton = findSendButton();
    const pageText = document.body
      ? (document.body.innerText || "")
      : "";
    const namesVisible = files.every((file) =>
      pageText.indexOf(file.name) >= 0
    );

    if (
      sendButton &&
      (namesVisible || Date.now() - started >= 2500)
    ) {
      return;
    }

    await sleep(RESPONSE_POLL_MS);
  }

  throw new Error("Timed out waiting for ChatGPT file upload to become ready.");
}

function looksLikeAttachmentName(value) {
  return /\.(txt|md|log|csv|json|xml|ini|cfg|conf|c|cc|cpp|cxx|h|hh|hpp|py|js|css|html|htm|lua|rs|toml|yaml|yml|bmp|gif|jpg|jpeg|png|tif|tiff|pdf|zip)$/i.test(
    value || ""
  );
}

function sanitizeAttachmentName(value) {
  const cleaned = String(value || "")
    .replace(/[\\/]+/g, "/")
    .split("/")
    .pop()
    .replace(/[\r\n]/g, "_")
    .trim();

  return cleaned || "attachment.bin";
}

function contentDispositionFileName(value) {
  if (!value) {
    return "";
  }

  const utfMatch = /filename\*=UTF-8''([^;]+)/i.exec(value);
  if (utfMatch) {
    try {
      return decodeURIComponent(utfMatch[1]);
    } catch (_exception) {
      return utfMatch[1];
    }
  }

  const plainMatch = /filename="?([^";]+)"?/i.exec(value);
  return plainMatch ? plainMatch[1] : "";
}

function attachmentCandidateUrls(anchor) {
  const values = [
    anchor.getAttribute("href"),
    anchor.href,
    anchor.getAttribute("data-href"),
    anchor.getAttribute("data-url"),
    anchor.getAttribute("data-download-url")
  ];

  const urls = [];

  for (const value of values) {
    if (!value || urls.includes(value)) {
      continue;
    }

    if (value.startsWith("sandbox:")) {
      continue;
    }

    try {
      urls.push(new URL(value, location.href).href);
    } catch (_exception) {
      if (value.startsWith("blob:") || value.startsWith("data:")) {
        urls.push(value);
      }
    }
  }

  return urls;
}

function looksLikeAttachmentAnchor(anchor) {
  const href = anchor.getAttribute("href") || "";
  const label = (anchor.textContent || "").trim();

  return (
    anchor.hasAttribute("download") ||
    href.startsWith("sandbox:") ||
    href.indexOf("/interpreter/download") >= 0 ||
    href.indexOf("/backend-api/files/") >= 0 ||
    href.indexOf("/files/") >= 0 ||
    looksLikeAttachmentName(label) ||
    looksLikeAttachmentName(href)
  );
}

async function downloadAssistantAttachment(anchor) {
  const urls = attachmentCandidateUrls(anchor);

  for (const url of urls) {
    try {
      const response = await fetch(url, {
        credentials: "include",
        cache: "no-store"
      });

      if (!response.ok) {
        continue;
      }

      const lengthHeader = response.headers.get("content-length");
      if (
        lengthHeader &&
        Number(lengthHeader) > MAX_ATTACHMENT_BYTES
      ) {
        continue;
      }

      const bytes = new Uint8Array(await response.arrayBuffer());
      if (bytes.length > MAX_ATTACHMENT_BYTES) {
        continue;
      }

      const disposition = response.headers.get("content-disposition");
      const dispositionName = contentDispositionFileName(disposition);
      const downloadName = anchor.getAttribute("download") || "";
      const label = (anchor.textContent || "").trim();

      let urlName = "";
      try {
        const parsed = new URL(url);
        urlName = decodeURIComponent(
          parsed.pathname.split("/").pop() || ""
        );
      } catch (_exception) {
        urlName = "";
      }

      const name = sanitizeAttachmentName(
        dispositionName ||
        downloadName ||
        (looksLikeAttachmentName(label) ? label : "") ||
        urlName
      );

      return {
        name: name,
        mime_type:
          response.headers.get("content-type") ||
          "application/octet-stream",
        data_base64: encodeBase64(bytes)
      };
    } catch (_exception) {
      // Try the next candidate URL.
    }
  }

  return null;
}

async function collectAssistantAttachments() {
  const nodes = assistantNodes();
  if (!nodes.length) {
    return [];
  }

  const node = nodes[nodes.length - 1];
  const anchors = Array.from(node.querySelectorAll("a[href]"))
    .filter(looksLikeAttachmentAnchor);

  const attachments = [];
  const seen = new Set();

  for (const anchor of anchors) {
    if (attachments.length >= MAX_ATTACHMENT_COUNT) {
      break;
    }

    const key =
      anchor.getAttribute("href") + "|" +
      (anchor.textContent || "");

    if (seen.has(key)) {
      continue;
    }
    seen.add(key);

    const attachment = await downloadAssistantAttachment(anchor);
    if (attachment) {
      attachments.push(attachment);
    }
  }

  return attachments;
}

function sleep(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

async function submitMessage(text, attachments) {
  const commandStartedAt = performance.now();
  const composer = findComposer();

  if (!composer) {
    throw new Error(
      "ChatGPT composer is not visible in the current LibreWolf tab."
    );
  }

  const before = assistantSnapshots();
  const beforeCount = before.length;
  const beforeLast = before.length ? before[before.length - 1] : "";

  if (text) {
    setComposerText(composer, text);
  }

  if (Array.isArray(attachments) && attachments.length) {
    await injectAttachments(composer, attachments);
  }

  await sleep(100);

  const sendButton = findSendButton();

  if (sendButton) {
    sendButton.click();
  } else {
    composer.dispatchEvent(
      new KeyboardEvent("keydown", {
        key: "Enter",
        code: "Enter",
        keyCode: 13,
        which: 13,
        bubbles: true
      })
    );
    composer.dispatchEvent(
      new KeyboardEvent("keyup", {
        key: "Enter",
        code: "Enter",
        keyCode: 13,
        which: 13,
        bubbles: true
      })
    );
  }

  const submittedAt = performance.now();
  const deadline = Date.now() + RESPONSE_TIMEOUT_MS;
  let responseText = "";
  let lastChange = Date.now();
  let observedResponse = false;
  let firstResponseAt = 0;

  while (Date.now() < deadline) {
    const snapshots = assistantSnapshots();
    let candidate = "";

    if (snapshots.length > beforeCount) {
      candidate = snapshots[snapshots.length - 1];
    } else if (snapshots.length) {
      const last = snapshots[snapshots.length - 1];

      if (last !== beforeLast) {
        candidate = last;
      }
    }

    if (candidate) {
      if (!observedResponse) {
        firstResponseAt = performance.now();
      }

      observedResponse = true;

      if (candidate !== responseText) {
        responseText = candidate;
        lastChange = Date.now();
      }
    }

    if (
      observedResponse &&
      responseText &&
      !generationActive() &&
      Date.now() - lastChange >= RESPONSE_STABLE_MS
    ) {
      const completedAt = performance.now();
      const lastChangeAge = Date.now() - lastChange;
      const stabilizationMs = Math.max(0, Math.round(lastChangeAge));
      const firstResponseMs = firstResponseAt > 0
        ? Math.max(0, Math.round(firstResponseAt - submittedAt))
        : 0;
      const totalMs = Math.max(
        0,
        Math.round(completedAt - commandStartedAt)
      );
      const submitMs = Math.max(
        0,
        Math.round(submittedAt - commandStartedAt)
      );
      const generationMs = firstResponseAt > 0
        ? Math.max(
            0,
            totalMs - submitMs - firstResponseMs - stabilizationMs
          )
        : 0;

      const responseAttachments = await collectAssistantAttachments();

      return {
        text: responseText,
        attachments: responseAttachments,
        timing: {
          browser_submit_ms: submitMs,
          browser_first_response_ms: firstResponseMs,
          browser_generation_ms: generationMs,
          browser_stabilization_ms: stabilizationMs,
          browser_total_ms: totalMs
        }
      };
    }

    await sleep(RESPONSE_POLL_MS);
  }

  if (responseText) {
    const completedAt = performance.now();
    const firstResponseMs = firstResponseAt > 0
      ? Math.max(0, Math.round(firstResponseAt - submittedAt))
      : 0;

    const responseAttachments = await collectAssistantAttachments();

    return {
      text: responseText,
      attachments: responseAttachments,
      timing: {
        browser_submit_ms: Math.max(
          0,
          Math.round(submittedAt - commandStartedAt)
        ),
        browser_first_response_ms: firstResponseMs,
        browser_generation_ms: firstResponseAt > 0
          ? Math.max(0, Math.round(completedAt - firstResponseAt))
          : 0,
        browser_stabilization_ms: 0,
        browser_total_ms: Math.max(
          0,
          Math.round(completedAt - commandStartedAt)
        )
      }
    };
  }

  throw new Error(
    "Timed out waiting for a rendered assistant response."
  );
}

browser.runtime.onMessage.addListener((message) => {
  if (!message || typeof message.type !== "string") {
    return undefined;
  }

  if (message.type === "salix_status") {
    return Promise.resolve({
      composer_ready: !!findComposer(),
      url: location.href,
      title: document.title
    });
  }

  if (message.type === "salix_send_message") {
    const attachments = Array.isArray(message.attachments)
      ? message.attachments
      : [];

    if (
      typeof message.text !== "string" ||
      (!message.text && !attachments.length)
    ) {
      return Promise.resolve({
        ok: false,
        error: "Relay message text or attachment is required."
      });
    }

    return submitMessage(message.text, attachments)
      .then((result) => ({
        ok: true,
        text: result.text,
        attachments: result.attachments || [],
        timing: result.timing
      }))
      .catch((exception) => ({
        ok: false,
        error: String(exception)
      }));
  }

  return undefined;
});
