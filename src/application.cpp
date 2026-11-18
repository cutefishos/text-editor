// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "application.h"
#include "document.h"
#include "editorsettings.h"
#include "syntax.h"
#include "textcodec.h"

#include "base/dbus.h"
#include "base/event_loop.h"
#include "base/i18n.h"
#include "base/strings.h"
#include "base/url.h"
#include "ui/application.h"
#include "ui/clipboard.h"
#include "ui/window.h"

#include <algorithm>
#include <filesystem>

using cutefish::i18n::tr;
namespace fs = std::filesystem;
namespace strings = cutefish::base::strings;
namespace dbus = cutefish::base::dbus;

static constexpr const char *serviceName = "org.cutefish.TextEditor";
static constexpr const char *servicePath = "/TextEditor";

// Shown as it is, so a maximized window stays maximized.
static void present(EditorWindow *window)
{
    if (!window)
        return;
    window->window().show();
    window->window().activate();
}

EditorApplication::EditorApplication(EditorSettings &settings)
    : m_settings(settings)
    , m_recentFiles(settings.path())
{
    Document::setRecentFilesHook([this](const std::string &path) { m_recentFiles.add(path); });
}

EditorApplication::~EditorApplication()
{
    Document::setRecentFilesHook(nullptr);
    m_service.reset();
    m_order.clear();
    m_windows.clear();
}

static const std::vector<cutefish::ui::FontFamily> &families()
{
    static const std::vector<cutefish::ui::FontFamily> list = cutefish::ui::fontFamilies();
    return list;
}

std::vector<std::string> EditorApplication::monospaceFamilies() const
{
    std::vector<std::string> result;
    for (const cutefish::ui::FontFamily &family : families()) {
        if (family.fixedPitch && !strings::startsWith(family.name, "."))
            result.push_back(family.name);
    }
    return result;
}

std::string EditorApplication::encodingLabel(const std::string &encoding)
{
    for (const EncodingInfo &info : TextCodec::encodings()) {
        if (encoding == info.name)
            return tr(info.label);
    }
    return encoding;
}

std::vector<std::pair<std::string, std::string>> EditorApplication::languages() const
{
    std::vector<std::pair<std::string, std::string>> result{{std::string(), Syntax::name({})}};
    for (const Language *language : Syntax::languages()) {
        const std::string id = Syntax::id(language);
        result.emplace_back(id, Syntax::name(id));
    }
    return result;
}

int EditorApplication::windowCount() const
{
    return int(m_windows.size());
}

std::vector<EditorWindow *> EditorApplication::windows() const
{
    return m_order;
}

EditorWindow *EditorApplication::newWindow(EditorWindow::Options options)
{
    auto window = std::make_unique<EditorWindow>(*this, std::move(options));
    EditorWindow *raw = window.get();
    m_windows.push_back(std::move(window));
    m_order.push_back(raw);
    m_lastActive = raw;
    raw->window().show();
    windowCountChanged();
    return raw;
}

void EditorApplication::releaseWindow(EditorWindow *window)
{
    std::erase(m_order, window);
    if (m_lastActive == window)
        m_lastActive = m_order.empty() ? nullptr : m_order.back();
    const auto it = std::find_if(m_windows.begin(), m_windows.end(),
                                 [window](const std::unique_ptr<EditorWindow> &w) { return w.get() == window; });
    if (it == m_windows.end())
        return;
    // Destroyed after the event it closed in, which may be one of its own.
    std::shared_ptr<EditorWindow> doomed(std::move(*it));
    m_windows.erase(it);
    cutefish::base::EventLoop::current()->post([doomed] {});
    windowCountChanged();
}

void EditorApplication::windowActivated(EditorWindow *window)
{
    m_lastActive = window;
    // The window brought forward is the one that comes back in front.
    std::erase(m_order, window);
    m_order.push_back(window);
}

EditorWindow *EditorApplication::targetWindow(EditorWindow *preferred)
{
    if (preferred)
        return preferred;
    if (m_lastActive && !m_lastActive->window().isHidden())
        return m_lastActive;
    for (auto it = m_order.rbegin(); it != m_order.rend(); ++it) {
        if (!(*it)->window().isHidden())
            return *it;
    }
    return newWindow();
}

std::optional<EditorApplication::OpenDocument> EditorApplication::findOpenDocument(const std::string &path) const
{
    std::error_code error;
    const std::string absolute = fs::absolute(path, error).lexically_normal().string();
    const fs::path canonicalPath = fs::canonical(path, error);
    const std::string canonical = error ? std::string() : canonicalPath.string();
    for (EditorWindow *window : m_order) {
        const int index = window->indexOfPath(absolute, canonical);
        if (index >= 0)
            return OpenDocument{window, index};
    }
    return std::nullopt;
}

void EditorApplication::openPaths(const std::vector<std::string> &paths, int line, EditorWindow *preferred)
{
    EditorWindow *window = nullptr;
    for (const std::string &path : paths) {
        const std::optional<OpenDocument> open = findOpenDocument(path);
        EditorWindow *target = open ? open->window : targetWindow(preferred);
        if (!target)
            continue;
        std::error_code error;
        target->openFile(fs::absolute(path, error).lexically_normal().string(), line);
        window = target;
    }
    if (window)
        present(window);
}

void EditorApplication::openText(const std::string &text)
{
    EditorWindow *window = targetWindow();
    if (!window)
        return;
    window->openText(text);
    present(window);
}

void EditorApplication::open(const std::vector<std::string> &arguments, const std::string &workingDirectory)
{
    std::error_code error;
    const fs::path directory = workingDirectory.empty() ? fs::current_path(error) : fs::path(workingDirectory);
    int line = 0;
    std::vector<std::string> pending;
    bool any = false;
    for (const std::string &argument : arguments) {
        // "+42" applies to the files after it, as in other editors.
        if (argument.size() > 1 && argument[0] == '+') {
            if (const std::optional<int> value = strings::toNumber<int>(std::string_view(argument).substr(1))) {
                if (!pending.empty())
                    openPaths(pending, line);
                pending.clear();
                line = *value;
                continue;
            }
        }
        const std::optional<std::string> local = cutefish::base::url::isLocalFile(argument)
                ? cutefish::base::url::toLocalFile(argument)
                : std::nullopt;
        pending.push_back(local ? *local : (directory / argument).lexically_normal().string());
        any = true;
    }
    if (!pending.empty())
        openPaths(pending, line);
    if (!any)
        present(targetWindow());
}

void EditorApplication::newWindowFromBus()
{
    EditorWindow *window = newWindow();
    window->window().activate();
}

bool EditorApplication::registerService()
{
    dbus::Connection *bus = dbus::Connection::session();
    if (!bus)
        return true;
    if (!bus->requestName(serviceName))
        return false;
    m_service = std::make_unique<dbus::Object>(bus, servicePath, serviceName);
    m_service->addMethod("Open", [this](std::vector<std::string> arguments, std::string workingDirectory) {
        open(arguments, workingDirectory);
    });
    m_service->addMethod("OpenText", [this](std::string text) { openText(text); });
    m_service->addMethod("NewWindow", [this]() { newWindowFromBus(); });
    m_service->publish();
    return true;
}

void EditorApplication::start(const std::vector<std::string> &arguments, const std::string &workingDirectory,
                        const std::optional<std::string> &standardInput)
{
    if (!arguments.empty())
        open(arguments, workingDirectory);
    if (standardInput)
        openText(*standardInput);
    if (m_windows.empty())
        newWindow();
}

void EditorApplication::quit()
{
    m_quitting = true;
    // Later: this is called from handlers of the windows quitting closes.
    cutefish::base::EventLoop::current()->post([] { cutefish::ui::Application::instance()->quit(); });
}

void EditorApplication::showInFileManager(const std::string &path) const
{
    dbus::Connection *bus = dbus::Connection::session();
    if (!bus)
        return;
    dbus::Proxy proxy(bus, "org.freedesktop.FileManager1", "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1");
    proxy.send("ShowItems", std::vector<std::string>{cutefish::base::url::fromLocalFile(path)}, std::string());
}

void EditorApplication::copyText(const std::string &text) const
{
    cutefish::ui::Clipboard::setText(text);
}
