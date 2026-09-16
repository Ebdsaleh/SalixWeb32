// =================================================================================
// Filename:    framework/ApplicationCommand.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral application-shell command identifiers.
// =================================================================================
#pragma once

enum ApplicationCommand {
    application_command_none = 0,

    application_command_attach_file,
    application_command_exit,

    application_command_undo,
    application_command_cut,
    application_command_copy,
    application_command_paste,
    application_command_select_all,

    application_command_show_conversation,
    application_command_show_runtime,

    application_command_about
};
