// =================================================================================
// Filename:    framework/Utf8Text.h
// Author:      Ebdsaleh
// Description: Small UTF-8 byte-boundary helpers for backend-neutral text storage.
// =================================================================================
#pragma once

namespace Utf8Text {
    inline bool is_continuation_byte(unsigned char value) {
        return (value & 0xC0) == 0x80;
    }

    inline int sequence_length(
        const char* text,
        int text_length,
        int position
    ) {
        if (
            text == 0 ||
            position < 0 ||
            position >= text_length
        ) {
            return 0;
        }

        const unsigned char first =
            (unsigned char)text[position];

        if (first < 0x80) {
            return 1;
        }

        if (
            first >= 0xC2 &&
            first <= 0xDF &&
            position + 1 < text_length &&
            is_continuation_byte(
                (unsigned char)text[position + 1]
            )
        ) {
            return 2;
        }

        if (
            first >= 0xE0 &&
            first <= 0xEF &&
            position + 2 < text_length
        ) {
            const unsigned char second =
                (unsigned char)text[position + 1];
            const unsigned char third =
                (unsigned char)text[position + 2];

            bool second_valid = is_continuation_byte(second);

            if (first == 0xE0) {
                second_valid = second >= 0xA0 && second <= 0xBF;
            } else if (first == 0xED) {
                second_valid = second >= 0x80 && second <= 0x9F;
            }

            if (
                second_valid &&
                is_continuation_byte(third)
            ) {
                return 3;
            }
        }

        if (
            first >= 0xF0 &&
            first <= 0xF4 &&
            position + 3 < text_length
        ) {
            const unsigned char second =
                (unsigned char)text[position + 1];
            const unsigned char third =
                (unsigned char)text[position + 2];
            const unsigned char fourth =
                (unsigned char)text[position + 3];

            bool second_valid = is_continuation_byte(second);

            if (first == 0xF0) {
                second_valid = second >= 0x90 && second <= 0xBF;
            } else if (first == 0xF4) {
                second_valid = second >= 0x80 && second <= 0x8F;
            }

            if (
                second_valid &&
                is_continuation_byte(third) &&
                is_continuation_byte(fourth)
            ) {
                return 4;
            }
        }

        // Invalid input remains addressable as one byte instead of making
        // navigation or layout stall. Valid relay/clipboard text is UTF-8.
        return 1;
    }

    inline bool is_valid(
        const char* text,
        int text_length
    ) {
        if (text == 0) {
            return text_length <= 0;
        }

        int position = 0;

        while (position < text_length) {
            unsigned char first =
                (unsigned char)text[position];

            int length = sequence_length(
                text,
                text_length,
                position
            );

            if (length <= 0) {
                return false;
            }

            if (first >= 0x80 && length == 1) {
                return false;
            }

            position += length;
        }

        return position == text_length;
    }

    inline int next_index(
        const char* text,
        int text_length,
        int position
    ) {
        if (position < 0) {
            position = 0;
        }

        if (position >= text_length) {
            return text_length;
        }

        int length = sequence_length(
            text,
            text_length,
            position
        );

        if (length < 1) {
            length = 1;
        }

        int next = position + length;
        return next > text_length ? text_length : next;
    }

    inline int previous_index(
        const char* text,
        int text_length,
        int position
    ) {
        if (position <= 0 || text == 0 || text_length <= 0) {
            return 0;
        }

        if (position > text_length) {
            position = text_length;
        }

        int candidate = position - 1;
        int minimum = position - 4;
        if (minimum < 0) {
            minimum = 0;
        }

        while (
            candidate > minimum &&
            is_continuation_byte(
                (unsigned char)text[candidate]
            )
        ) {
            --candidate;
        }

        int length = sequence_length(
            text,
            text_length,
            candidate
        );

        if (candidate + length == position) {
            return candidate;
        }

        // Invalid/truncated input: preserve progress one byte at a time.
        return position - 1;
    }

    inline int clamp_to_boundary(
        const char* text,
        int text_length,
        int position
    ) {
        if (position <= 0 || text == 0) {
            return 0;
        }

        if (position >= text_length) {
            return text_length;
        }

        int first_candidate = position - 3;
        if (first_candidate < 0) {
            first_candidate = 0;
        }

        for (
            int candidate = first_candidate;
            candidate < position;
            ++candidate
        ) {
            int length = sequence_length(
                text,
                text_length,
                candidate
            );

            if (
                length > 1 &&
                candidate + length > position
            ) {
                return candidate;
            }
        }

        // The position is already a boundary, including the legacy
        // ACP fallback case where a high byte is not valid UTF-8.
        return position;
    }

    inline bool is_ascii_whitespace_at(
        const char* text,
        int text_length,
        int position
    ) {
        if (
            text == 0 ||
            position < 0 ||
            position >= text_length
        ) {
            return false;
        }

        const unsigned char value =
            (unsigned char)text[position];

        return value == ' ' ||
            value == '\t' ||
            value == '\r' ||
            value == '\n' ||
            value == '\v' ||
            value == '\f';
    }

    inline int encode_code_point(
        unsigned long code_point,
        char output[5]
    ) {
        if (output == 0) {
            return 0;
        }

        output[0] = '\0';

        if (code_point <= 0x7F) {
            output[0] = (char)code_point;
            output[1] = '\0';
            return 1;
        }

        if (code_point <= 0x7FF) {
            output[0] = (char)(0xC0 | (code_point >> 6));
            output[1] = (char)(0x80 | (code_point & 0x3F));
            output[2] = '\0';
            return 2;
        }

        if (
            code_point >= 0xD800 &&
            code_point <= 0xDFFF
        ) {
            return 0;
        }

        if (code_point <= 0xFFFF) {
            output[0] = (char)(0xE0 | (code_point >> 12));
            output[1] = (char)(0x80 | ((code_point >> 6) & 0x3F));
            output[2] = (char)(0x80 | (code_point & 0x3F));
            output[3] = '\0';
            return 3;
        }

        if (code_point <= 0x10FFFF) {
            output[0] = (char)(0xF0 | (code_point >> 18));
            output[1] = (char)(0x80 | ((code_point >> 12) & 0x3F));
            output[2] = (char)(0x80 | ((code_point >> 6) & 0x3F));
            output[3] = (char)(0x80 | (code_point & 0x3F));
            output[4] = '\0';
            return 4;
        }

        return 0;
    }

    inline int prefix_length_for_byte_limit(
        const char* text,
        int text_length,
        int byte_limit
    ) {
        if (
            text == 0 ||
            text_length <= 0 ||
            byte_limit <= 0
        ) {
            return 0;
        }

        if (byte_limit >= text_length) {
            return text_length;
        }

        int position = 0;

        while (position < text_length) {
            int next = next_index(
                text,
                text_length,
                position
            );

            if (next > byte_limit) {
                break;
            }

            position = next;
        }

        return position;
    }
}
