"use strict";

// Keep the validated 0.2.9 response detector unchanged; only extend its final wait ceiling.
const RESPONSE_TIMEOUT_MS = 4 * 60 * 60 * 1000;
const RESPONSE_POLL_MS = 250;
const RESPONSE_STABLE_MS = 2000;
const ATTACHMENT_SHORT_CANDIDATE_BYTES = 96;
const ATTACHMENT_SHORT_CANDIDATE_HOLD_MS = 12000;

const relayTrace = {
  state: "idle",
  assistant_snapshot_count: 0,
  candidate_bytes: 0,
  generation_active: false,
  rendered_anchor_found: false,
  rendered_delta_bytes: 0,
  thread_delta_bytes: 0,
  candidate_source: "",
  thread_progress_rejected: false,
  thread_attachment_guarded: false,
  thread_anchor_guarded: false,
  provider_status_stripped: false,
  short_candidate_hold: false,
  short_candidate_hold_ms: 0,
  command_received_utc: "",
  submitted_utc: "",
  first_candidate_utc: "",
  last_candidate_change_utc: "",
  completion_ready_utc: "",
  completed_utc: "",
  stable_ms: 0,
  error: ""
};

function updateRelayTrace(values) {
  Object.assign(relayTrace, values || {});
}

function utcNow() {
  return new Date().toISOString();
}

function relayTraceSnapshot() {
  return {
    state: String(relayTrace.state || ""),
    assistant_snapshot_count:
      Number(relayTrace.assistant_snapshot_count) || 0,
    candidate_bytes:
      Number(relayTrace.candidate_bytes) || 0,
    generation_active: !!relayTrace.generation_active,
    rendered_anchor_found:
      !!relayTrace.rendered_anchor_found,
    rendered_delta_bytes:
      Number(relayTrace.rendered_delta_bytes) || 0,
    thread_delta_bytes:
      Number(relayTrace.thread_delta_bytes) || 0,
    candidate_source:
      String(relayTrace.candidate_source || ""),
    thread_progress_rejected:
      !!relayTrace.thread_progress_rejected,
    thread_attachment_guarded:
      !!relayTrace.thread_attachment_guarded,
    thread_anchor_guarded:
      !!relayTrace.thread_anchor_guarded,
    provider_status_stripped:
      !!relayTrace.provider_status_stripped,
    short_candidate_hold:
      !!relayTrace.short_candidate_hold,
    short_candidate_hold_ms:
      Number(relayTrace.short_candidate_hold_ms) || 0,
    command_received_utc:
      String(relayTrace.command_received_utc || ""),
    submitted_utc:
      String(relayTrace.submitted_utc || ""),
    first_candidate_utc:
      String(relayTrace.first_candidate_utc || ""),
    last_candidate_change_utc:
      String(relayTrace.last_candidate_change_utc || ""),
    completion_ready_utc:
      String(relayTrace.completion_ready_utc || ""),
    completed_utc:
      String(relayTrace.completed_utc || ""),
    stable_ms: Number(relayTrace.stable_ms) || 0,
    error: String(relayTrace.error || "")
  };
}
const ATTACHMENT_UPLOAD_TIMEOUT_MS = 30000;
const ATTACHMENT_IMAGE_SETTLE_MS = 8000;
const ATTACHMENT_READY_STABLE_MS = 1500;
const SUBMIT_ATTACHMENT_VERIFY_MS = 5000;
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
  // Current ChatGPT can place data-message-author-role on a display:contents
  // wrapper. Such a wrapper has a zero-size client rect even though its
  // descendant response text is visibly rendered, so do not require the role
  // node itself to pass visible().
  const roleNodes = Array.from(
    document.querySelectorAll("[data-message-author-role='assistant']")
  ).filter((node) => !!cleanNodeText(node));

  if (roleNodes.length) {
    return roleNodes;
  }

  const turnNodes = Array.from(
    document.querySelectorAll("article[data-testid^='conversation-turn-']")
  ).filter((turn) => !!turn.querySelector(".markdown"));

  if (turnNodes.length) {
    return turnNodes;
  }

  // Provider markup can change independently of Salix. The rendered Markdown
  // response surface is the least-specific fallback and matches the original
  // text relay semantics without introducing turn-correlation heuristics.
  return Array.from(
    document.querySelectorAll("#thread .markdown, main .markdown, .markdown")
  ).filter((node, index, nodes) =>
    !!cleanNodeText(node) &&
    nodes.indexOf(node) === index
  );
}

function assistantSnapshots() {
  return assistantNodes()
    .map((node) => {
      const markdown =
        node.matches && node.matches(".markdown")
          ? node
          : node.querySelector(".markdown");
      return cleanNodeText(markdown || node);
    })
    .filter(Boolean);
}

function normalizeRelayText(text) {
  return String(text || "")
    .replace(/\r\n/g, "\n")
    .replace(/[ \t]+/g, " ")
    .replace(/\n[ \t]+/g, "\n")
    .trim();
}

function renderedConversationText() {
  const root =
    document.querySelector("#thread") ||
    document.querySelector("main") ||
    document.body;

  if (!root) {
    return "";
  }

  const clone = root.cloneNode(true);

  clone.querySelectorAll(
    [
      "form",
      "textarea",
      "input",
      "button",
      "svg",
      "nav",
      "header",
      "[aria-hidden='true']",
      "[aria-live]",
      "[role='status']",
      "[role='alert']",
      ".sr-only",
      "[data-testid='composer-footer-actions']",
      "[data-composer-surface]"
    ].join(", ")
  ).forEach((node) => node.remove());

  return normalizeRelayText(
    clone.textContent || ""
  );
}

function insertedTextDelta(beforeText, afterText) {
  const before = normalizeRelayText(beforeText);
  const after = normalizeRelayText(afterText);

  if (!after || after === before) {
    return "";
  }

  let prefix = 0;
  const prefixLimit = Math.min(before.length, after.length);

  while (
    prefix < prefixLimit &&
    before.charAt(prefix) === after.charAt(prefix)
  ) {
    prefix += 1;
  }

  let beforeEnd = before.length - 1;
  let afterEnd = after.length - 1;

  while (
    beforeEnd >= prefix &&
    afterEnd >= prefix &&
    before.charAt(beforeEnd) === after.charAt(afterEnd)
  ) {
    beforeEnd -= 1;
    afterEnd -= 1;
  }

  if (afterEnd < prefix) {
    return "";
  }

  return normalizeRelayText(
    after.slice(prefix, afterEnd + 1)
  );
}

function renderedConversationResponseDelta(
  baselineText,
  currentText,
  submittedText
) {
  let delta = insertedTextDelta(
    baselineText,
    currentText
  );
  const submitted = normalizeRelayText(submittedText);

  if (!delta || !submitted) {
    return delta;
  }

  if (delta === submitted) {
    return "";
  }

  if (delta.startsWith(submitted)) {
    return normalizeRelayText(
      delta.slice(submitted.length)
    );
  }

  const submittedIndex = delta.indexOf(submitted);

  // The new user turn should be at, or very near, the beginning of the
  // inserted text. Allow a small amount of provider chrome before it, but do
  // not delete an incidental copy of the user's text from the assistant reply.
  if (submittedIndex >= 0 && submittedIndex <= 128) {
    return normalizeRelayText(
      delta.slice(0, submittedIndex) +
      delta.slice(submittedIndex + submitted.length)
    );
  }

  return delta;
}

function stripProviderProgressText(text, attachmentContext) {
  let current = normalizeRelayText(text);
  let stripped = false;

  const exactStatuses = [
    "chatgpt is responding",
    "chatgpt is thinking",
    "you said:chatgpt is responding",
    "you said:chatgpt is thinking"
  ];

  if (attachmentContext) {
    exactStatuses.push("confirming receipt");
    exactStatuses.push("requesting console details");
  }

  while (current) {
    const lower = current.toLowerCase();

    if (exactStatuses.includes(lower)) {
      return {
        text: "",
        stripped: true
      };
    }

    let matchedPrefix = "";

    for (const status of exactStatuses) {
      const prefix = status + "\n";

      if (lower.startsWith(prefix)) {
        matchedPrefix = current.slice(0, status.length);
        break;
      }
    }

    if (!matchedPrefix) {
      break;
    }

    current = normalizeRelayText(
      current.slice(matchedPrefix.length)
    );
    stripped = true;
  }

  return {
    text: current,
    stripped: stripped
  };
}

function looksLikeProviderProgressText(text) {
  const cleaned = stripProviderProgressText(text, false);

  return !!text && !cleaned.text && cleaned.stripped;
}

function shortAttachmentCandidateNeedsHold(text) {
  const normalized = normalizeRelayText(text);

  if (!normalized || normalized.indexOf("\n") >= 0) {
    return false;
  }

  const byteLength = new TextEncoder().encode(
    normalized
  ).length;

  if (byteLength > ATTACHMENT_SHORT_CANDIDATE_BYTES) {
    return false;
  }

  // Provider activity/status labels observed during attachment requests are
  // short, single-line phrases with no sentence terminator. A legitimate
  // short final answer that is rendered atomically is allowed after the
  // bounded hold window instead of being discarded.
  return !/[.!?][\]"')]*$/.test(normalized);
}

function findRenderedSubmittedMessage(text) {
  const wanted = normalizeRelayText(text);

  if (!wanted) {
    return null;
  }

  const root =
    document.querySelector("#thread") ||
    document.querySelector("main") ||
    document.body;

  if (!root) {
    return null;
  }

  const candidates = Array.from(
    root.querySelectorAll(
      "[data-message-author-role='user'], " +
      "[data-testid^='conversation-turn-'], " +
      "article, p, div"
    )
  );

  let best = null;

  for (const candidate of candidates) {
    const candidateText = normalizeRelayText(
      cleanNodeText(candidate)
    );

    if (candidateText !== wanted) {
      continue;
    }

    if (
      !best ||
      candidate.childElementCount < best.childElementCount
    ) {
      best = candidate;
    }
  }

  if (!best) {
    return null;
  }

  // Climb through wrappers that contain only the submitted message. This
  // leaves the range start immediately after the user turn without depending
  // on ChatGPT's role/turn class names.
  let anchor = best;

  while (
    anchor.parentElement &&
    anchor.parentElement !== root &&
    normalizeRelayText(cleanNodeText(anchor.parentElement)) === wanted
  ) {
    anchor = anchor.parentElement;
  }

  return {
    root: root,
    anchor: anchor
  };
}

function renderedTextAfterSubmittedMessage(anchorInfo) {
  if (
    !anchorInfo ||
    !anchorInfo.root ||
    !anchorInfo.anchor ||
    !anchorInfo.root.contains(anchorInfo.anchor)
  ) {
    return "";
  }

  const range = document.createRange();

  try {
    range.setStartAfter(anchorInfo.anchor);
    range.setEnd(
      anchorInfo.root,
      anchorInfo.root.childNodes.length
    );
  } catch (_exception) {
    return "";
  }

  const fragment = range.cloneContents();

  fragment.querySelectorAll(
    [
      "form",
      "textarea",
      "input",
      "button",
      "svg",
      "nav",
      "header",
      "[aria-hidden='true']",
      "[aria-live]",
      "[role='status']",
      "[role='alert']",
      ".sr-only",
      "[data-testid='composer-footer-actions']",
      "[data-composer-surface]"
    ].join(", ")
  ).forEach((node) => node.remove());

  return normalizeRelayText(
    fragment.textContent || ""
  );
}

function renderedResponseDelta(currentText, baselineText) {
  const current = normalizeRelayText(currentText);
  const baseline = normalizeRelayText(baselineText);

  if (!current || current === baseline) {
    return "";
  }

  if (baseline && current.endsWith(baseline)) {
    return normalizeRelayText(
      current.slice(0, current.length - baseline.length)
    );
  }

  if (baseline && current.startsWith(baseline)) {
    return normalizeRelayText(
      current.slice(baseline.length)
    );
  }

  return current;
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

function submitStateSummary(composer) {
  const liveComposer = findComposer() || composer;
  const sendButton = findSendButton(liveComposer);
  const form = liveComposer
    ? liveComposer.closest("form")
    : null;
  const fileInput = findFileInput();

  const testId = sendButton
    ? (sendButton.getAttribute("data-testid") || "")
    : "";
  const aria = sendButton
    ? (sendButton.getAttribute("aria-label") || "")
    : "";
  const type = sendButton
    ? (sendButton.getAttribute("type") || "")
    : "";

  return [
    "composer=" + (liveComposer ? "yes" : "no"),
    "text_bytes=" + (
      liveComposer
        ? new TextEncoder().encode(
            composerText(liveComposer)
          ).length
        : 0
    ),
    "send=" + (sendButton ? "yes" : "no"),
    "send_testid=" + JSON.stringify(testId),
    "send_aria=" + JSON.stringify(aria),
    "send_type=" + JSON.stringify(type),
    "form=" + (form ? "yes" : "no"),
    "file_input_count=" + (
      fileInput && fileInput.files
        ? fileInput.files.length
        : -1
    ),
    "generation_active=" + (
      generationActive() ? "yes" : "no"
    ),
    "user_messages=" + userMessageCount()
  ].join(" ");
}

async function waitForSubmitAcceptance(
  beforeUserCount,
  originalText,
  verifyMilliseconds
) {
  const deadline = Date.now() + verifyMilliseconds;

  while (Date.now() < deadline) {
    if (userMessageCount() > beforeUserCount) {
      return true;
    }

    if (generationActive()) {
      return true;
    }

    const currentComposer = findComposer();
    if (
      currentComposer &&
      originalText &&
      composerText(currentComposer).trim() !== originalText.trim()
    ) {
      return true;
    }

    await sleep(100);
  }

  return false;
}

async function submitComposer(
  composer,
  originalText,
  attachmentCount
) {
  const beforeUserCount = userMessageCount();
  const submitDeadline = Date.now() + 20000;
  const verifyMilliseconds = attachmentCount > 0
    ? SUBMIT_ATTACHMENT_VERIFY_MS
    : 1500;
  let clickAttempted = false;
  let requestSubmitAttempted = false;

  while (Date.now() < submitDeadline) {
    const liveComposer = findComposer() || composer;
    const sendButton = findSendButton(liveComposer);

    if (!sendButton) {
      await sleep(100);
      continue;
    }

    sendButton.focus();
    sendButton.click();
    clickAttempted = true;

    if (
      await waitForSubmitAcceptance(
        beforeUserCount,
        originalText,
        verifyMilliseconds
      )
    ) {
      return;
    }

    const currentComposer = findComposer() || liveComposer;
    const form = currentComposer
      ? currentComposer.closest("form")
      : null;
    const currentSend = findSendButton(currentComposer);

    if (
      !requestSubmitAttempted &&
      form &&
      currentSend &&
      typeof form.requestSubmit === "function"
    ) {
      requestSubmitAttempted = true;

      try {
        form.requestSubmit(currentSend);
      } catch (_exception) {
        // Fall through to the normal retry loop.
      }

      if (
        await waitForSubmitAcceptance(
          beforeUserCount,
          originalText,
          verifyMilliseconds
        )
      ) {
        return;
      }
    }

    await sleep(250);
  }

  throw new Error(
    (
      clickAttempted
        ? "ChatGPT Send control did not accept the relay submission."
        : "ChatGPT Send control was not available after attachment upload."
    ) +
    " submit_state={" +
    submitStateSummary(composer) +
    "}"
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
  const hasImage = files.some((file) =>
    /^image\//i.test(file.type || "")
  );
  let sendReadySince = 0;

  while (Date.now() - started < ATTACHMENT_UPLOAD_TIMEOUT_MS) {
    const liveComposer = findComposer() || composer;
    const sendButton = findSendButton(liveComposer);
    const pageText = document.body
      ? (document.body.innerText || "")
      : "";
    const namesVisible = files.every((file) =>
      pageText.indexOf(file.name) >= 0
    );

    if (sendButton) {
      if (!sendReadySince) {
        sendReadySince = Date.now();
      }

      const elapsed = Date.now() - started;
      const readyStable = Date.now() - sendReadySince;

      if (
        !hasImage &&
        namesVisible &&
        readyStable >= 250
      ) {
        return;
      }

      if (
        hasImage &&
        elapsed >= ATTACHMENT_IMAGE_SETTLE_MS &&
        readyStable >= ATTACHMENT_READY_STABLE_MS
      ) {
        return;
      }

      if (
        !hasImage &&
        !namesVisible &&
        elapsed >= ATTACHMENT_IMAGE_SETTLE_MS &&
        readyStable >= ATTACHMENT_READY_STABLE_MS
      ) {
        return;
      }
    } else {
      sendReadySince = 0;
    }

    await sleep(RESPONSE_POLL_MS);
  }

  throw new Error(
    "Timed out waiting for ChatGPT file upload to become ready."
  );
}

function looksLikeAttachmentName(value) {
  return /\.(txt|md|log|csv|json|xml|ini|cfg|conf|c|cc|cpp|cxx|h|hh|hpp|py|js|css|html|htm|lua|rs|toml|yaml|yml|bmp|gif|jpg|jpeg|png|tif|tiff|pdf|zip)$/i.test(
    value || ""
  );
}

function textMentionsAttachmentName(value) {
  return attachmentNamesInText(value).length > 0;
}

function attachmentNamesInText(value) {
  const names = [];
  // Current ChatGPT attachment cards can flatten adjacent UI chrome into the
  // rendered text with no separator, for example:
  //   SalixWeb32-remote-test-image.pngImageOpen file
  //
  // Preserve the historical filename start semantics and only relax the
  // right-hand terminator: a known attachment-card UI label may immediately
  // follow the extension without whitespace.
  const pattern =
    /\b([^\s<>:"|?*\/\\]+\.(txt|md|log|csv|json|xml|ini|cfg|conf|c|cc|cpp|cxx|h|hh|hpp|py|js|css|html|htm|lua|rs|toml|yaml|yml|bmp|gif|jpg|jpeg|png|tif|tiff|pdf|zip))(?=\b|Image(?:Open(?:\s+file)?)?|Open(?:\s+file)?|Download|Preview)/gi;
  const text = String(value || "");
  let match;

  while ((match = pattern.exec(text)) !== null) {
    const name = sanitizeAttachmentName(match[1]);

    if (name && !names.includes(name)) {
      names.push(name);
    }
  }

  return names;
}

function attachmentNameFromElement(element) {
  if (!element) {
    return "";
  }

  const directValues = [
    element.getAttribute("download"),
    element.getAttribute("aria-label"),
    element.getAttribute("title"),
    element.textContent
  ];

  for (const value of directValues) {
    const names = attachmentNamesInText(value);
    if (names.length) {
      return names[0];
    }
  }

  let current = element.parentElement;
  let depth = 0;

  while (current && depth < 6) {
    const names = attachmentNamesInText(
      current.textContent || ""
    );

    if (names.length) {
      return names[0];
    }

    current = current.parentElement;
    depth += 1;
  }

  return "";
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

function nodeFollowsRenderedAnchor(node, anchorInfo) {
  if (
    !node ||
    !anchorInfo ||
    !anchorInfo.anchor ||
    !anchorInfo.root ||
    !anchorInfo.root.contains(node) ||
    !anchorInfo.root.contains(anchorInfo.anchor)
  ) {
    return true;
  }

  if (node === anchorInfo.anchor || node.contains(anchorInfo.anchor)) {
    return false;
  }

  const relation = anchorInfo.anchor.compareDocumentPosition(node);
  return !!(relation & Node.DOCUMENT_POSITION_FOLLOWING);
}

function renderedAssistantResponseNode(responseText, anchorInfo) {
  const wanted = normalizeRelayText(responseText);

  if (!wanted) {
    return null;
  }

  const root =
    (anchorInfo && anchorInfo.root) ||
    document.querySelector("#thread") ||
    document.querySelector("main") ||
    document.body;

  if (!root) {
    return null;
  }

  const candidates = Array.from(
    root.querySelectorAll(
      "[data-testid^='conversation-turn-'], article, section, div"
    )
  );

  let exact = null;
  let relaxed = null;
  let relaxedOverhead = Number.MAX_SAFE_INTEGER;

  for (const candidate of candidates) {
    if (!nodeFollowsRenderedAnchor(candidate, anchorInfo)) {
      continue;
    }

    const candidateText = normalizeRelayText(
      cleanNodeText(candidate)
    );

    if (!candidateText) {
      continue;
    }

    if (candidateText === wanted) {
      if (
        !exact ||
        candidate.childElementCount < exact.childElementCount
      ) {
        exact = candidate;
      }
      continue;
    }

    const index = candidateText.indexOf(wanted);
    if (index < 0) {
      continue;
    }

    // Allow a small amount of attachment/provider chrome around the rendered
    // assistant text, but never select a broad conversation/history wrapper.
    const overhead = candidateText.length - wanted.length;
    if (
      overhead >= 0 &&
      overhead <= 512 &&
      (
        !relaxed ||
        overhead < relaxedOverhead ||
        (
          overhead === relaxedOverhead &&
          candidate.childElementCount < relaxed.childElementCount
        )
      )
    ) {
      relaxed = candidate;
      relaxedOverhead = overhead;
    }
  }

  return exact || relaxed;
}

function assistantAttachmentRoot(responseText, anchorInfo) {
  const nodes = assistantNodes();

  if (nodes.length) {
    const node = nodes[nodes.length - 1];

    return {
      root:
        node.closest("article[data-testid^='conversation-turn-']") ||
        node.closest("article") ||
        node.parentElement ||
        node,
      source: "assistant_nodes"
    };
  }

  // Current ChatGPT can render the assistant response without any of the
  // historical assistant-role/.markdown selectors. Reuse the exact rendered
  // response correlation already proven by the text relay, then climb only
  // within that response's ancestors to find its file card/download control.
  const responseNode = renderedAssistantResponseNode(
    responseText,
    anchorInfo
  );

  if (!responseNode) {
    return {
      root: null,
      source: "none"
    };
  }

  let current = responseNode;
  let depth = 0;

  while (
    current &&
    depth < 8 &&
    (!anchorInfo || current !== anchorInfo.root)
  ) {
    if (
      anchorInfo &&
      anchorInfo.anchor &&
      current.contains(anchorInfo.anchor)
    ) {
      break;
    }

    if (attachmentCandidateElements(current).length) {
      return {
        root: current,
        source: "rendered_response"
      };
    }

    current = current.parentElement;
    depth += 1;
  }

  return {
    root:
      responseNode.closest("article[data-testid^='conversation-turn-']") ||
      responseNode.closest("article") ||
      responseNode.parentElement ||
      responseNode,
    source: "rendered_response"
  };
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

async function finishManagedCapture(
  element,
  captured,
  debug,
  preferredName
) {
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
    return null;
  }

  debug.download_capture_successes += 1;
  debug.managed_download_successes += 1;

  if (
    typeof captured.intercepted_requests === "number" &&
    captured.intercepted_requests > 0
  ) {
    debug.intercept_capture_successes += 1;
    debug.intercepted_requests += captured.intercepted_requests;
  }

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
      preferredName ||
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

async function captureManagedDownload(
  element,
  debug,
  preferredName
) {
  const urls = downloadControlUrls(element);

  debug.download_url_candidates += urls.length;

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

    const attachment = await finishManagedCapture(
      element,
      captured,
      debug,
      preferredName
    );

    if (attachment) {
      return attachment;
    }
  }

  if (!urls.length) {
    debug.intercept_capture_attempts += 1;

    const armed = await browser.runtime.sendMessage({
      type: "salix_arm_download_request_capture"
    });

    if (!armed || armed.ok !== true) {
      debug.errors.push(
        armed && armed.error
          ? String(armed.error)
          : "download request interception could not be armed"
      );
      return null;
    }

    element.click();

    const captured = await browser.runtime.sendMessage({
      type: "salix_wait_download_capture",
      timeout_ms: 15000
    });

    return finishManagedCapture(
      element,
      captured,
      debug,
      preferredName
    );
  }

  return null;
}

async function downloadAssistantAttachment(
  element,
  debug,
  preferredName
) {
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
          preferredName ||
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
    const captured = await captureManagedDownload(
      element,
      debug,
      preferredName
    );
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
        debug,
        preferredName
      );

      closeAttachmentPreview(previewDownload, debug);

      if (captured) {
        return captured;
      }
    }
  }

  return null;
}

async function collectAssistantAttachments(responseText, anchorInfo) {
  const debug = {
    scan_root: "none",
    scan_root_source: "none",
    scan_passes: 0,
    candidates_seen: 0,
    sandbox_candidates: 0,
    download_capture_attempts: 0,
    download_capture_successes: 0,
    download_url_candidates: 0,
    intercept_capture_attempts: 0,
    intercept_capture_successes: 0,
    intercepted_requests: 0,
    managed_download_successes: 0,
    direct_fetch_attempts: 0,
    direct_fetch_successes: 0,
    preview_open_attempts: 0,
    preview_download_controls: 0,
    preview_close_successes: 0,
    semantic_names_seen: 0,
    duplicate_candidates_skipped: 0,
    duplicate_attachments_skipped: 0,
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
    const rootInfo = assistantAttachmentRoot(
      responseText,
      anchorInfo
    );
    root = rootInfo.root;
    debug.scan_root_source = rootInfo.source;
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
  const seenCandidates = new Set();
  const seenAttachmentNames = new Set();
  const responseNames = attachmentNamesInText(responseText);
  let responseNameIndex = 0;

  debug.semantic_names_seen = responseNames.length;

  for (const element of elements) {
    if (attachments.length >= MAX_ATTACHMENT_COUNT) {
      break;
    }

    let preferredName = attachmentNameFromElement(element);

    if (!preferredName && responseNameIndex < responseNames.length) {
      preferredName = responseNames[responseNameIndex];
    }

    const structuralKey =
      elementAttachmentHref(element) + "|" +
      (element.textContent || "") + "|" +
      (element.getAttribute("aria-label") || "") + "|" +
      (element.getAttribute("title") || "");

    const semanticKey = preferredName
      ? "name:" + preferredName.toLowerCase()
      : "control:" + structuralKey;

    if (seenCandidates.has(semanticKey)) {
      debug.duplicate_candidates_skipped += 1;
      continue;
    }
    seenCandidates.add(semanticKey);

    const attachment = await downloadAssistantAttachment(
      element,
      debug,
      preferredName
    );

    if (attachment) {
      const normalizedName = String(
        attachment.name || ""
      ).toLowerCase();

      if (
        normalizedName &&
        seenAttachmentNames.has(normalizedName)
      ) {
        debug.duplicate_attachments_skipped += 1;
        continue;
      }

      if (normalizedName) {
        seenAttachmentNames.add(normalizedName);
      }

      attachments.push(attachment);

      if (
        preferredName &&
        responseNameIndex < responseNames.length &&
        preferredName === responseNames[responseNameIndex]
      ) {
        responseNameIndex += 1;
      }
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
  updateRelayTrace({
    state: "command_received",
    assistant_snapshot_count: 0,
    candidate_bytes: 0,
    generation_active: generationActive(),
    rendered_anchor_found: false,
    rendered_delta_bytes: 0,
    thread_delta_bytes: 0,
    candidate_source: "",
    thread_progress_rejected: false,
    thread_attachment_guarded: false,
    thread_anchor_guarded: false,
    provider_status_stripped: false,
    short_candidate_hold: false,
    short_candidate_hold_ms: 0,
    command_received_utc: utcNow(),
    submitted_utc: "",
    first_candidate_utc: "",
    last_candidate_change_utc: "",
    completion_ready_utc: "",
    completed_utc: "",
    stable_ms: 0,
    error: ""
  });

  const commandStartedAt = performance.now();
  const hasOutboundAttachments =
    Array.isArray(attachments) && attachments.length > 0;
  const composer = findComposer();

  if (!composer) {
    updateRelayTrace({
      state: "composer_missing",
      error: "composer not visible"
    });
    throw new Error(
      "ChatGPT composer is not visible in the current LibreWolf tab."
    );
  }

  updateRelayTrace({
    state: "composer_found"
  });

  // Capture the rendered thread before submission. This fallback baseline is
  // intentionally taken before the new user turn exists, so a very fast
  // assistant reply cannot be absorbed into a later post-submit baseline.
  const renderedConversationBaseline = renderedConversationText();
  const before = assistantSnapshots();
  const beforeCount = before.length;
  const beforeLast = before.length ? before[before.length - 1] : "";

  if (text) {
    setComposerText(composer, text);
  }

  if (hasOutboundAttachments) {
    await injectAttachments(composer, attachments);
  }

  await sleep(250);

  await submitComposer(
    composer,
    text,
    hasOutboundAttachments ? attachments.length : 0
  );

  updateRelayTrace({
    state: "submitted",
    submitted_utc: utcNow(),
    generation_active: generationActive()
  });

  const submittedAt = performance.now();
  const deadline = Date.now() + RESPONSE_TIMEOUT_MS;
  let responseText = "";
  let lastChange = Date.now();
  let observedResponse = false;
  let firstResponseAt = 0;
  let renderedAnchor = null;
  let renderedBaseline = "";
  let firstCandidateWallClock = 0;

  while (Date.now() < deadline) {
    if (!renderedAnchor && text) {
      renderedAnchor = findRenderedSubmittedMessage(text);

      if (renderedAnchor) {
        renderedBaseline = renderedTextAfterSubmittedMessage(
          renderedAnchor
        );
      }
    }
    const snapshots = assistantSnapshots();
    let candidate = "";
    let candidateSource = "";
    let renderedDelta = "";
    let threadDelta = "";
    let threadProgressRejected = false;
    let threadAttachmentGuarded = false;
    let threadAnchorGuarded = false;
    let providerStatusStripped = false;

    updateRelayTrace({
      state: "waiting_response",
      assistant_snapshot_count: snapshots.length,
      generation_active: generationActive(),
      rendered_anchor_found: !!renderedAnchor,
      stable_ms: Math.max(0, Date.now() - lastChange)
    });

    if (snapshots.length > beforeCount) {
      candidate = snapshots[snapshots.length - 1];
      candidateSource = candidate ? "assistant_snapshot" : "";
    } else if (snapshots.length) {
      const last = snapshots[snapshots.length - 1];

      if (last !== beforeLast) {
        candidate = last;
        candidateSource = candidate ? "assistant_snapshot" : "";
      }
    }

    if (!candidate && renderedAnchor) {
      renderedDelta = renderedResponseDelta(
        renderedTextAfterSubmittedMessage(renderedAnchor),
        renderedBaseline
      );

      const cleanedRendered = stripProviderProgressText(
        renderedDelta,
        hasOutboundAttachments
      );

      if (cleanedRendered.stripped) {
        providerStatusStripped = true;
      }

      renderedDelta = cleanedRendered.text;
      candidate = renderedDelta;
      candidateSource = candidate ? "rendered_anchor" : "";
    }

    // Selector-independent last resort: only use whole-thread diffing when
    // the exact submitted-message anchor cannot be found. Once the anchor
    // exists, its per-message response region is strictly safer than a global
    // thread diff, which can be invalidated by unrelated DOM reordering.
    if (
      !candidate &&
      text &&
      !hasOutboundAttachments &&
      !renderedAnchor
    ) {
      threadDelta = renderedConversationResponseDelta(
        renderedConversationBaseline,
        renderedConversationText(),
        text
      );

      const cleanedThread = stripProviderProgressText(
        threadDelta,
        false
      );

      if (cleanedThread.stripped) {
        providerStatusStripped = true;
      }

      if (
        threadDelta &&
        !cleanedThread.text &&
        cleanedThread.stripped
      ) {
        threadProgressRejected = true;
      }

      threadDelta = cleanedThread.text;
      candidate = threadDelta;
      candidateSource = candidate ? "thread_delta" : "";
    } else if (
      !candidate &&
      text &&
      !hasOutboundAttachments &&
      renderedAnchor
    ) {
      // The submitted user turn is known exactly. Do not let a whole-thread
      // diff override that stronger anchor, even if the anchored response is
      // still empty while ChatGPT is preparing the reply.
      threadAnchorGuarded = true;
    } else if (!candidate && text && hasOutboundAttachments) {
      // Uploading an attachment mutates large parts of ChatGPT's rendered
      // thread/composer DOM. A whole-thread diff can therefore contain stale
      // conversation text, attachment labels, and provider UI rather than the
      // new assistant response. Keep the validated per-message anchor and
      // assistant-snapshot paths, but never use whole-thread diffing for an
      // attachment-bearing request.
      threadAttachmentGuarded = true;
    }

    updateRelayTrace({
      rendered_anchor_found: !!renderedAnchor,
      rendered_delta_bytes: new TextEncoder().encode(
        renderedDelta || ""
      ).length,
      thread_delta_bytes: new TextEncoder().encode(
        threadDelta || ""
      ).length,
      candidate_bytes: new TextEncoder().encode(
        candidate || ""
      ).length,
      candidate_source: candidateSource,
      thread_progress_rejected: threadProgressRejected,
      thread_attachment_guarded: threadAttachmentGuarded,
      thread_anchor_guarded: threadAnchorGuarded,
      provider_status_stripped: providerStatusStripped,
      generation_active: generationActive(),
      stable_ms: Math.max(0, Date.now() - lastChange)
    });

    if (candidate) {
      if (!observedResponse) {
        firstResponseAt = performance.now();
        firstCandidateWallClock = Date.now();
        updateRelayTrace({
          first_candidate_utc: utcNow()
        });
      }

      observedResponse = true;

      if (candidate !== responseText) {
        responseText = candidate;
        lastChange = Date.now();
        updateRelayTrace({
          last_candidate_change_utc: utcNow()
        });
      }
    }

    const shortCandidateHoldAge =
      firstCandidateWallClock > 0
        ? Math.max(0, Date.now() - firstCandidateWallClock)
        : 0;

    const shortCandidateHold =
      hasOutboundAttachments &&
      shortAttachmentCandidateNeedsHold(responseText) &&
      shortCandidateHoldAge < ATTACHMENT_SHORT_CANDIDATE_HOLD_MS;

    updateRelayTrace({
      short_candidate_hold: shortCandidateHold,
      short_candidate_hold_ms: shortCandidateHoldAge
    });

    if (
      observedResponse &&
      responseText &&
      !shortCandidateHold &&
      !generationActive() &&
      Date.now() - lastChange >= RESPONSE_STABLE_MS
    ) {
      updateRelayTrace({
        state: "completion_ready",
        completion_ready_utc: utcNow(),
        candidate_bytes: new TextEncoder().encode(
          responseText
        ).length,
        generation_active: false,
        stable_ms: Math.max(0, Date.now() - lastChange)
      });

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
        responseText,
        renderedAnchor
      );

      updateRelayTrace({
        state: "completed",
        completed_utc: utcNow(),
        generation_active: false
      });

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
      generation_active: generationActive(),
      relay_trace: relayTraceSnapshot(),
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
      .catch((exception) => {
        updateRelayTrace({
          state: "error",
          error: String(exception)
        });
        return {
          ok: false,
          error: String(exception)
        };
      });
  }

  return undefined;
});
