"use strict";

const RESPONSE_TIMEOUT_MS = 180000;
const RESPONSE_POLL_MS = 250;
const RESPONSE_STABLE_MS = 2000;

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

function assistantSnapshots() {
  const roleNodes = Array.from(
    document.querySelectorAll("[data-message-author-role='assistant']")
  ).filter(visible);

  if (roleNodes.length) {
    return roleNodes
      .map((node) => {
        const markdown = node.querySelector(".markdown");
        return cleanNodeText(markdown || node);
      })
      .filter(Boolean);
  }

  const turns = Array.from(
    document.querySelectorAll("article[data-testid^='conversation-turn-']")
  );

  return turns
    .map((turn) => {
      const markdown = turn.querySelector(".markdown");
      return markdown ? cleanNodeText(markdown) : "";
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

function sleep(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

async function submitMessage(text) {
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

  setComposerText(composer, text);
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

      return {
        text: responseText,
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

    return {
      text: responseText,
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
    if (typeof message.text !== "string" || !message.text) {
      return Promise.resolve({
        ok: false,
        error: "Relay message text is empty."
      });
    }

    return submitMessage(message.text)
      .then((result) => ({
        ok: true,
        text: result.text,
        timing: result.timing
      }))
      .catch((exception) => ({
        ok: false,
        error: String(exception)
      }));
  }

  return undefined;
});
