// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef DIALOGS_H
#define DIALOGS_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cutefish::ui {
class DialogWindow;
class Window;
}

class EditorApplication;
class EditorPage;

// Save, Don't Save or Cancel for documents with unsaved changes. Several
// documents are listed with a check box each; Save saves the checked ones.
class SaveChangesDialog
{
public:
    explicit SaveChangesDialog(cutefish::ui::Window *parent);
    ~SaveChangesDialog();

    void ask(const std::vector<EditorPage *> &pages, std::function<void()> then);
    bool isOpen() const { return m_window != nullptr; }
    cutefish::ui::Window *window() const { return m_window.get(); }
    // The question in the language of the moment, its answers so far kept.
    void retranslate();

    // Saves one page; `done(saved)` follows, at once or after a Save As.
    std::function<void(EditorPage *page, std::function<void(bool saved)> done)> onSaveRequested;

private:
    class Content;

    void buildContent();
    void finish(bool proceed);
    void saveChecked();
    void release();
    // Expired once the page is closed, which another window may do meanwhile.
    std::weak_ptr<bool> lifetimeOf(const EditorPage *page) const;

    cutefish::ui::Window *m_parent;
    std::unique_ptr<cutefish::ui::Window> m_window;
    std::vector<EditorPage *> m_pages;
    std::vector<std::weak_ptr<bool>> m_lifetimes;
    std::vector<EditorPage *> m_checked;
    std::function<void()> m_action;
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};

// The global defaults; per-document settings live in the status bar.
class PreferencesDialog
{
public:
    PreferencesDialog(cutefish::ui::Window *parent, EditorApplication &application);
    ~PreferencesDialog();

    void show();
    cutefish::ui::Window *window() const;

private:
    class Content;

    cutefish::ui::Window *m_parent;
    EditorApplication &m_application;
    std::unique_ptr<cutefish::ui::DialogWindow> m_dialog;
};

// Every shortcut, by what it does, in four cards.
class ShortcutsDialog
{
public:
    explicit ShortcutsDialog(cutefish::ui::Window *parent);
    ~ShortcutsDialog();

    void show();
    cutefish::ui::Window *window() const;

private:
    class Content;

    cutefish::ui::Window *m_parent;
    std::unique_ptr<cutefish::ui::DialogWindow> m_dialog;
};

#endif
