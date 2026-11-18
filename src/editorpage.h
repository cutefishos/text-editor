// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef EDITORPAGE_H
#define EDITORPAGE_H

#include "document.h"

#include "base/signal.h"
#include "base/timer.h"
#include "ui/view.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cutefish::ui {
class ConfirmDialog;
class Menu;
}

class EditorApplication;
class EditorView;
class EditorWindow;
class FindBar;
class GoToLineBar;
class InfoBar;
class LoadingView;
class StatusBar;

// One tab: a document, the bars that belong to it and its view.
class EditorPage : public cutefish::ui::View
{
public:
    // How the tab starts: a file or text from standard input.
    struct Options
    {
        std::string initialPath;
        // From 0.
        int initialLine = 0;
        std::optional<std::string> initialText;
    };

    EditorPage(EditorApplication &application, EditorWindow *window, const Options &options);
    ~EditorPage() override;

    EditorWindow *editorWindow() const { return m_window; }
    // A tab moved to another window.
    void setEditorWindow(EditorWindow *window);
    EditorApplication &application() const { return m_application; }
    Document &document() const { return *m_document; }
    EditorView &editor() const { return *m_editor; }
    std::string title() const { return m_document->title(); }
    bool modified() const { return m_document->isModified(); }

    void focusEditor();
    // `line` from 1, or 0 for none.
    void open(const std::string &path, int line);
    void showFind(bool replace);
    void findNext(bool backwards);
    void showGoToLine();
    void setEncodingForReload(const std::string &encoding);
    void showEncodingMenu();
    void revert();
    void popupContextMenu(cutefish::ui::PointF position);
    // Follows the preferences, the theme and the window's zoom.
    void applySettings();
    // The bars and an open question take the language of the moment; the
    // text and its view stay.
    void retranslate();
    // Puts the cards in their corner and tells the view what they cover.
    void placeCards();
    // The card floating over the text, if one is open.
    cutefish::ui::View *card() const;
    FindBar *findBarIfMade() const { return m_findBar; }
    GoToLineBar *goToLineIfMade() const { return m_goToLine; }
    StatusBar &statusBar() const { return *m_statusBar; }
    InfoBar &infoBar() const { return *m_infoBar; }
    // Expires with the page, for callbacks that may outlive it.
    std::weak_ptr<bool> lifetime() const { return m_alive; }

    // The title or the modified state changed.
    cutefish::base::Signal<> tabChanged;

protected:
    void layout() override;
    void paint(cutefish::ui::Painter &painter) override;

private:
    class EditorArea;

    void applyTheme();
    void confirmReload(const std::string &encoding);
    void cardChanged();
    void settleScroll();
    void onLoaded();
    void updateLoading();

    EditorApplication &m_application;
    EditorWindow *m_window;
    std::unique_ptr<Document> m_document;
    std::vector<cutefish::base::Subscription> m_connections;
    InfoBar *m_infoBar;
    EditorArea *m_area;
    EditorView *m_editor;
    StatusBar *m_statusBar;
    LoadingView *m_loading = nullptr;
    // Made on first use.
    FindBar *m_findBar = nullptr;
    GoToLineBar *m_goToLine = nullptr;
    std::unique_ptr<cutefish::ui::Menu> m_contextMenu;
    std::unique_ptr<cutefish::ui::ConfirmDialog> m_reloadConfirm;
    std::string m_reloadEncoding;
    // A file that takes a moment shows its progress; a quick one shows nothing.
    cutefish::base::Timer m_loadingDelay;

    // The line (from 0) the cursor goes to once the file is loaded.
    std::optional<int> m_pendingLine;
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};

#endif
