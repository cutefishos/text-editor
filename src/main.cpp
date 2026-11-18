// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "application.h"
#include "editorsettings.h"

#include "base/dbus.h"
#include "base/i18n.h"
#include "ui/application.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <iterator>

#ifndef TEXT_EDITOR_VERSION
#define TEXT_EDITOR_VERSION "1.0"
#endif

namespace dbus = cutefish::base::dbus;

static void printHelp(const char *program)
{
    std::printf("Usage: %s [options] [+LINE] [FILE...] [-]\nCutefish Text Editor\n\nOptions:\n"
                "  -h, --help     Displays help on commandline options.\n"
                "  --help-all     Displays help.\n"
                "  -v, --version  Displays version information.\n"
                "  --new-window   Open a new window.\n\n"
                "Arguments:\n"
                "  files          Files to open; +LINE goes to that line of the next file, - reads\n"
                "                 standard input.\n",
                program);
}

// The live test runs the same with a scratch settings file, without the
// D-Bus service, so it never meets the user's session.
int main(int argc, char **argv)
{
#ifdef TEXT_EDITOR_LIVE_TEST
    // argv[1] is the scratch folder.
    const std::string scratch = argv[1];
    ++argv;
    --argc;
#endif
    std::vector<std::string> files;
    bool newWindow = false;
    bool readInput = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "-h" || argument == "--help" || argument == "-?" || argument == "--help-all") {
            printHelp(argv[0]);
            return 0;
        }
        if (argument == "-v" || argument == "--version") {
            std::printf("cutefish-text-editor %s\n", TEXT_EDITOR_VERSION);
            return 0;
        }
        if (argument == "--new-window") {
            newWindow = true;
            continue;
        }
        if (argument == "-") {
            readInput = true;
            continue;
        }
        if (argument.size() > 1 && argument[0] == '-' && argument[1] != '-' && !std::isdigit(static_cast<unsigned char>(argument[1]))) {
            std::fprintf(stderr, "Unknown option '%s'.\n", argument.substr(1).c_str());
            return 1;
        }
        if (argument.starts_with("--")) {
            std::fprintf(stderr, "Unknown option '%s'.\n", argument.substr(2).c_str());
            return 1;
        }
        files.push_back(argument);
    }

    std::optional<std::string> standardInput;
    if (readInput)
        standardInput = std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());

    cutefish::ui::Application app(argc, argv);
#ifdef TEXT_EDITOR_LIVE_TEST
    app.setApplicationId("cutefish-text-editor-live");
#else
    app.setApplicationId("cutefish-text-editor");
#endif
    cutefish::i18n::setDomain("cutefish-text-editor");

    std::error_code error;
    const std::string workingDirectory = std::filesystem::current_path(error).string();

#ifdef TEXT_EDITOR_LIVE_TEST
    EditorSettings settings(scratch + "/text-editor.conf");
    EditorApplication application(settings);
    if (false) {
#else
    EditorSettings settings(EditorSettings::defaultPath());
    EditorApplication application(settings);
    if (!application.registerService()) {
#endif
        // A running editor takes the files and this process ends.
        dbus::Proxy existing(dbus::Connection::session(), "org.cutefish.TextEditor", "/TextEditor",
                             "org.cutefish.TextEditor");
        existing.setTimeout(std::chrono::seconds(5));
        if (newWindow)
            existing.call("NewWindow");
        if (!files.empty() || !standardInput)
            existing.call("Open", files, workingDirectory);
        if (standardInput)
            existing.call("OpenText", *standardInput);
        return 0;
    }

    application.start(newWindow && files.empty() ? std::vector<std::string>() : files, workingDirectory, standardInput);
    return app.run();
}
