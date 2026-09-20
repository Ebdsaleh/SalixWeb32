"use strict";

const RESPONSE_TIMEOUT_MS = 180000;
const RESPONSE_POLL_MS = 250;
const RESPONSE_STABLE_MS = 2000;
const ATTACHMENT_UPLOAD_TIMEOUT_MS = 30000;
const ATTACHMENT_DISCOVERY_TIMEOUT_MS = 5000;
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

function buttonIsUsable(button) {
  return (
    button &&
    visible(button) &&
    !button.disabled &&
    button.getAttribute("aria-disabled") !== "true"
  );
}

function buttonLooksLikeSend(button) {
  if (!button) {
    return false;
  }

  const testId = (button.getAttribute("data-testid") || "").toLowerCase();
  const aria = (button.getAttribute("aria-label") || "").toLowerCase();
  const title = (button.getAttribute("title") || "").toLowerCase();
  const type = (button.getAttribute("type") || "").toLowerCase();

  if (
    testId.indexOf("stop") >= 0 ||
    aria.indexOf("stop") >= 0 ||
    title.indexOf("stop") >= 0
  ) {
    return false;
  }

  return (
    testId.indexOf("send") >= 0 ||
    aria.indexOf("send") >= 0 ||
    title.indexOf("send") >= 0 ||
    type === "submit"
  );
}

function findSendButton(composer) {
  const selectors = [
    "button[data-testid='send-button']",
    "button[data-testid*='send']",
    "button[aria-label='Send prompt']",
    "button[aria-label='Send message']",
    "button[aria-label^='Send']",
    "button[title^='Send']",
    "button[type='submit']"
  ];

  const scopes = [];
  if (composer) {
    const form = composer.closest("form");
    if (form) {
      scopes.push(form);
    }

    const parent = composer.parentElement;
    if (parent && !scopes.includes(parent)) {
      scopes.push(parent);
    }
  }

  scopes.push(document);

  for (const scope of scopes) {
    for (const selector of selectors) {
      const buttons = Array.from(scope.querySelectorAll(selector));

      for (const button of buttons) {
        if (
          buttonIsUsable(button) &&
          buttonLooksLikeSend(button)
        ) {
          return button;
        }
      }
    }
  }

  return null;
}

function composerText(composer) {
  if (!composer) {
    return "";
  }

  if (composer instanceof HTMLTextAreaElement) {
    return composer.value || "";
  }

  return composer.innerText || composer.textContent || "";
}

function userMessageCount() {
  const roleNodes = Array.from(
    document.querySelectorAll("[data-message-author-role='user']")
  ).filter(visible);

  if (roleNodes.length) {
    return roleNodes.length;
  }

  return Array.from(
    document.querySelectorAll("article[data-testid^='conversation-turn-']")
  ).filter((turn) => {
    const role = turn.querySelector("[data-message-author-role='user']");
    return !!role;
  }).length;
}

async function submitComposer(composer, originalText) {
  const beforeUserCount = userMessageCount();
  const submitDeadline = Date.now() + 15000;
  let clickAttempted = false;

  while (Date.now() < submitDeadline) {
    const liveComposer = findComposer() || composer;
    const sendButton = findSendButton(liveComposer);

    if (sendButton) {
      sendButton.focus();
      sendButton.click();
      clickAttempted = true;

      const verificationDeadline = Date.now() + 1500;

      while (Date.now() < verificationDeadline) {
        if (userMessageCount() > beforeUserCount) {
          return;
        }

        const currentComposer = findComposer();
        if (
          currentComposer &&
          originalText &&
          composerText(currentComposer).trim() !== originalText.trim()
        ) {
          return;
        }

        await sleep(100);
      }
    } else {
      await sleep(100);
    }
  }

  throw new Error(
    clickAttempted
      ? "ChatGPT Send control did not accept the relay submission."
      : "ChatGPT Send control was not available after attachment upload."
  );
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
    const sendButton = findSendButton(composer);
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

function textMentionsAttachmentName(value) {
  return /\b[^\s<>:"|?*\/\\]+\.(txt|md|log|csv|json|xml|ini|cfg|conf|c|cc|cpp|cxx|h|hh|hpp|py|js|css|html|htm|lua|rs|toml|yaml|yml|bmp|gif|jpg|jpeg|png|tif|tiff|pdf|zip)\b/i.test(
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

function elementAttachmentHref(element) {
  const values = [
    element.getAttribute("href"),
    element.getAttribute("data-href"),
    element.getAttribute("data-url"),
    element.getAttribute("data-download-url")
  ];

  for (const value of values) {
    if (typeof value === "string" && value) {
      return value;
    }
  }

  return "";
}

function elementIsExplicitDownloadControl(element) {
  const aria = (element.getAttribute("aria-label") || "").toLowerCase();
  const title = (element.getAttribute("title") || "").toLowerCase();
  const href = elementAttachmentHref(element);

  return (
    element.hasAttribute("download") ||
    aria.indexOf("download") >= 0 ||
    title.indexOf("download") >= 0 ||
    href.indexOf("/interpreter/download") >= 0 ||
    href.indexOf("/backend-api/files/") >= 0
  );
}

function elementLooksLikeDownloadControl(element) {
  const label = (element.textContent || "").trim();
  const href = elementAttachmentHref(element);

  return (
    elementIsExplicitDownloadControl(element) ||
    href.startsWith("sandbox:") ||
    href.indexOf("/files/") >= 0 ||
    looksLikeAttachmentName(label) ||
    looksLikeAttachmentName(href)
  );
}

function findVisibleExplicitDownloadControl() {
  const elements = Array.from(
    document.querySelectorAll(
      "a, button, [role='button'], [data-href], [data-url], [data-download-url]"
    )
  );

  for (const element of elements) {
    if (
      visible(element) &&
      elementIsExplicitDownloadControl(element)
    ) {
      return element;
    }
  }

  return null;
}

function closeAttachmentPreview(downloadControl, debug) {
  const scopes = [];
  const selectors = [
    "[role='dialog']",
    "[data-testid*='preview']",
    "[data-testid*='modal']",
    "aside"
  ];

  for (const selector of selectors) {
    const scope = downloadControl.closest(selector);
    if (scope && !scopes.includes(scope)) {
      scopes.push(scope);
    }
  }

  for (const scope of scopes) {
    const buttons = Array.from(
      scope.querySelectorAll(
        "button[aria-label='Close'], button[title='Close']"
      )
    );

    for (const button of buttons) {
      if (visible(button)) {
        button.click();
        debug.preview_close_successes += 1;
        return true;
      }
    }
  }

  return false;
}

async function openAttachmentPreview(element, debug) {
  debug.preview_open_attempts += 1;
  element.click();

  const deadline = Date.now() + 3000;

  while (Date.now() < deadline) {
    const control = findVisibleExplicitDownloadControl();

    if (control) {
      debug.preview_download_controls += 1;
      return control;
    }

    await sleep(RESPONSE_POLL_MS);
  }

  debug.errors.push("preview opened but no Download control appeared");
  return null;
}

function assistantAttachmentRoot() {
  const nodes = assistantNodes();
  if (!nodes.length) {
    return null;
  }

  const node = nodes[nodes.length - 1];

  return (
    node.closest("article[data-testid^='conversation-turn-']") ||
    node.closest("article") ||
    node.parentElement ||
    node
  );
}

function attachmentCandidateScore(element) {
  if (elementIsExplicitDownloadControl(element)) {
    return 3;
  }

  const href = elementAttachmentHref(element);
  if (
    href.startsWith("sandbox:") ||
    href.indexOf("/files/") >= 0
  ) {
    return 2;
  }

  return 1;
}

function attachmentCandidateElements(root) {
  if (!root) {
    return [];
  }

  const elements = Array.from(
    root.querySelectorAll(
      "a, button, [role='button'], [data-href], [data-url], [data-download-url]"
    )
  ).filter(elementLooksLikeDownloadControl);

  elements.sort((left, right) => {
    return attachmentCandidateScore(right) -
      attachmentCandidateScore(left);
  });

  return elements;
}

function downloadControlUrls(element) {
  const values = [];

  function collect(candidate) {
    if (!candidate) {
      return;
    }

    const direct = [
      candidate.getAttribute("href"),
      candidate.href,
      candidate.getAttribute("data-href"),
      candidate.getAttribute("data-url"),
      candidate.getAttribute("data-download-url")
    ];

    for (const value of direct) {
      if (
        typeof value === "string" &&
        value &&
        !values.includes(value)
      ) {
        values.push(value);
      }
    }
  }

  collect(element);

  const anchor = element.closest("a[href]");
  if (anchor && anchor !== element) {
    collect(anchor);
  }

  const nestedAnchor = element.querySelector("a[href]");
  if (nestedAnchor) {
    collect(nestedAnchor);
  }

  const urls = [];

  for (const value of values) {
    if (!value || value.startsWith("sandbox:")) {
      continue;
    }

    try {
      const url = new URL(value, location.href);

      if (
        (url.protocol === "https:" || url.protocol === "http:") &&
        !urls.includes(url.href)
      ) {
        urls.push(url.href);
      }
    } catch (_exception) {
      // Keep only absolute/relative HTTP(S) URLs for managed downloads.
    }
  }

  return urls;
}

function attachmentCandidateUrls(element) {
  const values = [
    element.getAttribute("href"),
    element.href,
    element.getAttribute("data-href"),
    element.getAttribute("data-url"),
    element.getAttribute("data-download-url")
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

async function captureManagedDownload(
  element,
  debug
) {
  const urls = downloadControlUrls(element);

  debug.download_url_candidates += urls.length;

  if (!urls.length) {
    debug.errors.push(
      "Download control exposed no HTTP(S) URL; control was not clicked."
    );
    return null;
  }

  for (const url of urls) {
    debug.download_capture_attempts += 1;

    const started = await browser.runtime.sendMessage({
      type: "salix_start_managed_download",
      url: url
    });

    if (!started || started.ok !== true) {
      debug.errors.push(
        started && started.error
          ? String(started.error)
          : "managed download could not be started"
      );
      continue;
    }

    const captured = await browser.runtime.sendMessage({
      type: "salix_wait_download_capture",
      timeout_ms: 15000
    });

    if (
      !captured ||
      captured.ok !== true ||
      typeof captured.filename !== "string" ||
      !captured.filename
    ) {
      debug.errors.push(
        captured && captured.error
          ? String(captured.error)
          : "managed download returned no file"
      );
      continue;
    }

    debug.download_capture_successes += 1;
    debug.managed_download_successes += 1;

    const href = elementAttachmentHref(element);
    const label = (element.textContent || "").trim();
    const downloadName = element.getAttribute("download") || "";

    let hrefName = "";
    try {
      hrefName = decodeURIComponent(
        href.split("/").pop() || ""
      );
    } catch (_exception) {
      hrefName = href.split("/").pop() || "";
    }

    return {
      name: sanitizeAttachmentName(
        downloadName ||
        (looksLikeAttachmentName(label) ? label : "") ||
        hrefName ||
        captured.filename.split(/[\\/]/).pop()
      ),
      mime_type:
        (
          typeof captured.mime_type === "string" &&
          captured.mime_type
        ) ? captured.mime_type : "application/octet-stream",
      local_path: captured.filename,
      download_id: captured.download_id
    };
  }

  return null;
}

async function downloadAssistantAttachment(element, debug) {
  const href = elementAttachmentHref(element);
  const explicitDownload = elementIsExplicitDownloadControl(element);
  const urls = attachmentCandidateUrls(element);

  for (const url of urls) {
    debug.direct_fetch_attempts += 1;

    try {
      const response = await fetch(url, {
        credentials: "include",
        cache: "no-store"
      });

      if (!response.ok) {
        debug.errors.push(
          "direct fetch HTTP " + response.status
        );
        continue;
      }

      const lengthHeader = response.headers.get("content-length");
      if (
        lengthHeader &&
        Number(lengthHeader) > MAX_ATTACHMENT_BYTES
      ) {
        debug.errors.push("direct fetch file exceeds relay limit");
        continue;
      }

      const bytes = new Uint8Array(await response.arrayBuffer());
      if (bytes.length > MAX_ATTACHMENT_BYTES) {
        debug.errors.push("direct fetch file exceeds relay limit");
        continue;
      }

      const disposition = response.headers.get("content-disposition");
      const dispositionName = contentDispositionFileName(disposition);
      const downloadName = element.getAttribute("download") || "";
      const label = (element.textContent || "").trim();

      let urlName = "";
      try {
        const parsed = new URL(url);
        urlName = decodeURIComponent(
          parsed.pathname.split("/").pop() || ""
        );
      } catch (_exception) {
        urlName = "";
      }

      debug.direct_fetch_successes += 1;

      return {
        name: sanitizeAttachmentName(
          dispositionName ||
          downloadName ||
          (looksLikeAttachmentName(label) ? label : "") ||
          urlName
        ),
        mime_type:
          response.headers.get("content-type") ||
          "application/octet-stream",
        data_base64: encodeBase64(bytes)
      };
    } catch (exception) {
      debug.errors.push("direct fetch failed: " + String(exception));
    }
  }

  if (explicitDownload) {
    const captured = await captureManagedDownload(element, debug);
    if (captured) {
      return captured;
    }
  }

  if (!explicitDownload) {
    const previewDownload = await openAttachmentPreview(
      element,
      debug
    );

    if (previewDownload) {
      const captured = await captureManagedDownload(
        previewDownload,
        debug
      );

      closeAttachmentPreview(previewDownload, debug);

      if (captured) {
        return captured;
      }
    }
  }

  return null;
}

async function collectAssistantAttachments(responseText) {
  const debug = {
    scan_root: "none",
    scan_passes: 0,
    candidates_seen: 0,
    sandbox_candidates: 0,
    download_capture_attempts: 0,
    download_capture_successes: 0,
    download_url_candidates: 0,
    managed_download_successes: 0,
    direct_fetch_attempts: 0,
    direct_fetch_successes: 0,
    preview_open_attempts: 0,
    preview_download_controls: 0,
    preview_close_successes: 0,
    attachments_collected: 0,
    errors: []
  };

  const shouldWait = textMentionsAttachmentName(responseText);
  const deadline = Date.now() + (
    shouldWait ? ATTACHMENT_DISCOVERY_TIMEOUT_MS : 0
  );

  let elements = [];
  let root = null;

  do {
    root = assistantAttachmentRoot();
    debug.scan_passes += 1;

    if (root) {
      debug.scan_root = (
        root.getAttribute("data-testid") ||
        root.tagName.toLowerCase()
      );

      elements = attachmentCandidateElements(root);
      if (elements.length) {
        break;
      }
    }

    if (!shouldWait || Date.now() >= deadline) {
      break;
    }

    await sleep(RESPONSE_POLL_MS);
  } while (Date.now() < deadline);

  debug.candidates_seen = elements.length;
  debug.sandbox_candidates = elements.filter(
    (element) => elementAttachmentHref(element).startsWith("sandbox:")
  ).length;

  const attachments = [];
  const seen = new Set();

  for (const element of elements) {
    if (attachments.length >= MAX_ATTACHMENT_COUNT) {
      break;
    }

    const key =
      elementAttachmentHref(element) + "|" +
      (element.textContent || "") + "|" +
      (element.getAttribute("aria-label") || "") + "|" +
      (element.getAttribute("title") || "");

    if (seen.has(key)) {
      continue;
    }
    seen.add(key);

    const attachment = await downloadAssistantAttachment(
      element,
      debug
    );

    if (attachment) {
      attachments.push(attachment);
    }
  }

  debug.attachments_collected = attachments.length;
  debug.errors = debug.errors.slice(0, 8);

  return {
    attachments: attachments,
    debug: debug
  };
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

  await sleep(250);

  await submitComposer(composer, text);

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

      const attachmentResult = await collectAssistantAttachments(
        responseText
      );

      return {
        text: responseText,
        attachments: attachmentResult.attachments,
        attachment_debug: attachmentResult.debug,
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

    const attachmentResult = await collectAssistantAttachments(
      responseText
    );

    return {
      text: responseText,
      attachments: attachmentResult.attachments,
      attachment_debug: attachmentResult.debug,
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
        attachment_debug: result.attachment_debug || {},
        timing: result.timing
      }))
      .catch((exception) => ({
        ok: false,
        error: String(exception)
      }));
  }

  return undefined;
});
