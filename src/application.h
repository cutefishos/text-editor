// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef APPLICATION_H
#define APPLICATION_H

#include "editorwindow.h"
#include "recentfiles.h"

#include "base/signal.h"
#include "ui/glyph_run.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cutefish::base::dbus {
class Object;
}

class EditorSettings;

// The editor process: its windows, the recently used files and the D-Bus
// service later launches hand their files to.
class EditorApplication
{
public:
    explicit EditorApplication(EditorSettings &settings);
    ~EditorApplication();

    EditorApplication(const EditorApplication &) = delete;
    EditorApplication &operator=(const EditorApplication &) = delete;

    EditorSettings &settings() const { return m_settings; }
    RecentFiles &recentFiles() { return m_recentFiles; }

    std::vector<std::string> monospaceFamilies() const;
    static std::string encodingLabel(const std::string &encoding);
    // Plain Text first, then the languages by name: (id, name).
    std::vector<std::pair<std::string, std::string>> languages() const;

    int windowCount() const;
    bool isQuitting() const { return m_quitting; }

    EditorWindow *newWindow(EditorWindow::Options options = {});
    // Destroys a closed window once the event it closed in is over.
    void releaseWindow(EditorWindow *window);
    void windowActivated(EditorWindow *window);
    std::vector<EditorWindow *> windows() const;
    // Opens files in the window used last, or focuses their tabs where they are open.
    void openPaths(const std::vector<std::string> &paths, int line = 0, EditorWindow *window = nullptr);
    struct OpenDocument
    {
        EditorWindow *window;
        int index;
    };
    std::optional<OpenDocument> findOpenDocument(const std::string &path) const;

    // Ends the process, without asking: the unsaved documents are dealt with.
    void quit();

    void showInFileManager(const std::string &path) const;
    void copyText(const std::string &text) const;

    // Opens `arguments` (paths and +LINE) and `standardInput`, or an empty
    // document when there is neither.
    void start(const std::vector<std::string> &arguments, const std::string &workingDirectory,
               const std::optional<std::string> &standardInput);

    // The D-Bus methods: a later launch's files, resolved against its working directory.
    void open(const std::vector<std::string> &arguments, const std::string &workingDirectory);
    void openText(const std::string &text);
    void newWindowFromBus();
    // Registers org.cutefish.TextEditor; false when another process has it.
    bool registerService();

    cutefish::base::Signal<> windowCountChanged;

private:
    EditorWindow *targetWindow(EditorWindow *preferred = nullptr);

    EditorSettings &m_settings;
    RecentFiles m_recentFiles;
    std::vector<std::unique_ptr<EditorWindow>> m_windows;
    // Front to back: the window brought forward last is at the end.
    std::vector<EditorWindow *> m_order;
    EditorWindow *m_lastActive = nullptr;
    bool m_quitting = false;
    std::unique_ptr<cutefish::base::dbus::Object> m_service;
};

#endif
