// =================================================================================
// Filename:    engine/application_hosts/Win32DiagnosticCapture.h
// Author:      Ebdsaleh
// Description: Captures the visible SalixWeb32 window plus a text diagnostic report.
// =================================================================================
#pragma once

#include <windows.h>
#include <stdio.h>
#include <string>

#include "framework/View.h"
#include "engine/platform/win32/Win32ApplicationPaths.h"

class Win32DiagnosticCapture {
    public:
        static bool capture(
            HWND window_handle,
            View* application_view,
            const char* configured_diagnostics_directory,
            std::string& screenshot_path,
            std::string& report_path,
            std::string& error_text
        ) {
            screenshot_path.clear();
            report_path.clear();
            error_text.clear();

            if (window_handle == NULL || !IsWindow(window_handle)) {
                error_text = "The SalixWeb32 window is not available.";
                return false;
            }

            std::string diagnostics_directory;
            if (!get_diagnostics_directory(
                    configured_diagnostics_directory,
                    diagnostics_directory,
                    error_text
                )) {
                return false;
            }

            SYSTEMTIME local_time;
            GetLocalTime(&local_time);

            char timestamp[64];
            sprintf(
                timestamp,
                "SalixWeb32-%04u%02u%02u-%02u%02u%02u-%03u",
                (unsigned int)local_time.wYear,
                (unsigned int)local_time.wMonth,
                (unsigned int)local_time.wDay,
                (unsigned int)local_time.wHour,
                (unsigned int)local_time.wMinute,
                (unsigned int)local_time.wSecond,
                (unsigned int)local_time.wMilliseconds
            );

            std::string base_name = diagnostics_directory;
            if (
                !base_name.empty() &&
                base_name[base_name.size() - 1] != '\\'
            ) {
                base_name += "\\";
            }
            base_name += timestamp;

            screenshot_path = base_name;
            screenshot_path += ".bmp";
            report_path = base_name;
            report_path += ".txt";

            if (!capture_visible_window(window_handle, screenshot_path.c_str())) {
                error_text = "Could not capture the visible SalixWeb32 window.";
                screenshot_path.clear();
                report_path.clear();
                return false;
            }

            std::string report;
            if (application_view != 0) {
                application_view->build_diagnostic_report(report);
            }

            if (report.empty()) {
                report = "SalixWeb32 Diagnostic Report\r\nNo view diagnostics were available.\r\n";
            }

            report += "\r\nCapture\r\n-------\r\nScreenshot: ";
            report += screenshot_path;
            report += "\r\nReport: ";
            report += report_path;
            report += "\r\n";

            if (!write_text_file(report_path.c_str(), report)) {
                DeleteFileA(screenshot_path.c_str());
                error_text = "The screenshot was captured, but the diagnostic text file could not be written.";
                screenshot_path.clear();
                report_path.clear();
                return false;
            }

            return true;
        }

        static bool export_browser_report(
            View* application_view,
            const char* configured_diagnostics_directory,
            std::string& report_path,
            std::string& error_text
        ) {
            report_path.clear();
            error_text.clear();

            if (application_view == 0) {
                error_text = "The SalixWeb32 application view is not available.";
                return false;
            }

            std::string diagnostics_directory;
            if (!get_diagnostics_directory(
                    configured_diagnostics_directory,
                    diagnostics_directory,
                    error_text
                )) {
                return false;
            }

            SYSTEMTIME local_time;
            GetLocalTime(&local_time);

            char timestamp[80];
            sprintf(
                timestamp,
                "SalixWeb32-Browser-%04u%02u%02u-%02u%02u%02u-%03u.txt",
                (unsigned int)local_time.wYear,
                (unsigned int)local_time.wMonth,
                (unsigned int)local_time.wDay,
                (unsigned int)local_time.wHour,
                (unsigned int)local_time.wMinute,
                (unsigned int)local_time.wSecond,
                (unsigned int)local_time.wMilliseconds
            );

            report_path = diagnostics_directory;
            if (
                !report_path.empty() &&
                report_path[report_path.size() - 1] != '\\'
            ) {
                report_path += "\\";
            }
            report_path += timestamp;

            std::string report;
            if (
                !application_view->build_browser_diagnostic_report(report) ||
                report.empty()
            ) {
                error_text =
                    "No Browser diagnostic data is available to export.";
                report_path.clear();
                return false;
            }

            report += "\r\nExport\r\n------\r\nReport: ";
            report += report_path;
            report += "\r\n";

            if (!write_text_file(report_path.c_str(), report)) {
                error_text =
                    "The Browser diagnostic report could not be written.";
                report_path.clear();
                return false;
            }

            return true;
        }

    private:
        static bool get_diagnostics_directory(
            const char* configured_directory,
            std::string& directory,
            std::string& error_text
        ) {
            directory.clear();

            if (
                configured_directory == 0 ||
                configured_directory[0] == '\0'
            ) {
                error_text =
                    "No diagnostics directory is configured.";
                return false;
            }

            directory = configured_directory;

            if (directory.size() >= MAX_PATH) {
                error_text =
                    "The diagnostics directory path is too long for the NT5 target.";
                directory.clear();
                return false;
            }

            if (
                !Win32ApplicationPaths::is_absolute_path(
                    directory.c_str()
                )
            ) {
                error_text =
                    "The diagnostics directory must be an absolute path.";
                directory.clear();
                return false;
            }

            if (
                !Win32ApplicationPaths::ensure_directory_exists(
                    directory.c_str()
                )
            ) {
                error_text =
                    "Could not create or access the configured diagnostics directory.";
                directory.clear();
                return false;
            }

            return true;
        }

        static bool write_text_file(
            const char* path,
            const std::string& text
        ) {
            FILE* file = fopen(path, "wb");
            if (file == 0) {
                return false;
            }

            size_t written = fwrite(
                text.data(),
                1,
                text.size(),
                file
            );

            bool result = written == text.size();
            fclose(file);
            return result;
        }

        static bool capture_visible_window(
            HWND window_handle,
            const char* path
        ) {
            RECT window_rect;
            if (!GetWindowRect(window_handle, &window_rect)) {
                return false;
            }

            int width = window_rect.right - window_rect.left;
            int height = window_rect.bottom - window_rect.top;
            if (width <= 0 || height <= 0) {
                return false;
            }

            HDC screen_dc = GetDC(NULL);
            if (screen_dc == NULL) {
                return false;
            }

            HDC memory_dc = CreateCompatibleDC(screen_dc);
            if (memory_dc == NULL) {
                ReleaseDC(NULL, screen_dc);
                return false;
            }

            HBITMAP bitmap = CreateCompatibleBitmap(screen_dc, width, height);
            if (bitmap == NULL) {
                DeleteDC(memory_dc);
                ReleaseDC(NULL, screen_dc);
                return false;
            }

            HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
            BOOL copied = BitBlt(
                memory_dc,
                0,
                0,
                width,
                height,
                screen_dc,
                window_rect.left,
                window_rect.top,
                SRCCOPY
            );
            SelectObject(memory_dc, old_bitmap);
            ReleaseDC(NULL, screen_dc);

            if (!copied) {
                DeleteObject(bitmap);
                DeleteDC(memory_dc);
                return false;
            }

            BITMAPINFO bitmap_info;
            ZeroMemory(&bitmap_info, sizeof(bitmap_info));
            bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmap_info.bmiHeader.biWidth = width;
            bitmap_info.bmiHeader.biHeight = height;
            bitmap_info.bmiHeader.biPlanes = 1;
            bitmap_info.bmiHeader.biBitCount = 24;
            bitmap_info.bmiHeader.biCompression = BI_RGB;

            DWORD row_size = (DWORD)(((width * 24 + 31) / 32) * 4);
            DWORD image_size = row_size * (DWORD)height;
            bitmap_info.bmiHeader.biSizeImage = image_size;

            unsigned char* pixels = new unsigned char[image_size];
            if (pixels == 0) {
                DeleteObject(bitmap);
                DeleteDC(memory_dc);
                return false;
            }

            int scan_lines = GetDIBits(
                memory_dc,
                bitmap,
                0,
                (UINT)height,
                pixels,
                &bitmap_info,
                DIB_RGB_COLORS
            );

            if (scan_lines != height) {
                delete[] pixels;
                DeleteObject(bitmap);
                DeleteDC(memory_dc);
                return false;
            }

            BITMAPFILEHEADER file_header;
            ZeroMemory(&file_header, sizeof(file_header));
            file_header.bfType = 0x4D42;
            file_header.bfOffBits =
                sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            file_header.bfSize = file_header.bfOffBits + image_size;

            FILE* file = fopen(path, "wb");
            if (file == 0) {
                delete[] pixels;
                DeleteObject(bitmap);
                DeleteDC(memory_dc);
                return false;
            }

            bool result =
                fwrite(&file_header, sizeof(file_header), 1, file) == 1 &&
                fwrite(
                    &bitmap_info.bmiHeader,
                    sizeof(BITMAPINFOHEADER),
                    1,
                    file
                ) == 1 &&
                fwrite(pixels, 1, image_size, file) == image_size;

            fclose(file);
            delete[] pixels;
            DeleteObject(bitmap);
            DeleteDC(memory_dc);
            return result;
        }
};
