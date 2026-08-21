#include "compositors/jay/jay_workspace_backend.h"

#include "core/process/process.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <utility>

namespace {

  constexpr const char* kJayQuery = "jay";

  [[nodiscard]] bool sameWindowSnapshot(
      const std::vector<WorkspaceWindow>& lhs, const std::vector<WorkspaceWindow>& rhs
  ) {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
      const auto& a = lhs[i];
      const auto& b = rhs[i];
      if (a.windowId != b.windowId || a.workspaceKey != b.workspaceKey || a.appId != b.appId || a.title != b.title
          || a.x != b.x || a.y != b.y) {
        return false;
      }
    }
    return true;
  }

} // namespace

JayWorkspaceBackend::JayWorkspaceBackend() : m_available(process::commandExists(kJayQuery)) {
  if (m_available) {
    refresh();
  }
}

JayWorkspaceBackend::~JayWorkspaceBackend() { cleanup(); }

void JayWorkspaceBackend::setChangeCallback(ChangeCallback callback) { m_changeCallback = std::move(callback); }

int JayWorkspaceBackend::pollTimeoutMs() const noexcept {
  if (!m_available) {
    return -1;
  }

  return m_refreshRequested ? 0 : -1;
}

void JayWorkspaceBackend::dispatchPoll(short /*revents*/) {
  if (!m_available) {
    return;
  }

  if (m_refreshRequested) {
    refresh();
  }
}

void JayWorkspaceBackend::notifyToplevelChange() {
  if (!m_available) {
    return;
  }

  m_refreshRequested = true;
}

std::vector<WorkspaceWindow> JayWorkspaceBackend::workspaceWindows(const std::string& /*outputName*/) const {
  return m_windows;
}

void JayWorkspaceBackend::refresh() {
  m_refreshRequested = false;

  const auto result = process::runSync({"jay", "--json", "tree", "query", "match-windows", "-e", ""});
  if (!result) {
    return;
  }

  std::vector<WorkspaceWindow> next;
  std::size_t start = 0;
  while (start < result.out.size()) {
    const auto end = result.out.find('\n', start);
    const auto line = result.out.substr(start, end == std::string::npos ? std::string::npos : end - start);
    start = end == std::string::npos ? result.out.size() : end + 1;

    if (line.empty()) {
      continue;
    }

    try {
      const auto json = nlohmann::json::parse(line);
      if (!json.is_object()) {
        continue;
      }

      const auto type = json.value("type", "");
      if (type != "xdg-toplevel" && type != "x-window") {
        continue;
      }

      const auto workspace = json.value("workspace", "");
      const auto windowId = json.value("toplevel_id", "");
      if (workspace.empty() || windowId.empty()) {
        continue;
      }

      const auto position = json.value("position", nlohmann::json::object());
      next.push_back(
          WorkspaceWindow{
              .windowId = windowId,
              .workspaceKey = workspace,
              .appId = type == "x-window" ? json.value("x_class", "") : json.value("app_id", ""),
              .title = json.value("title", ""),
              .x = position.value("x1", 0),
              .y = position.value("y1", 0),
          }
      );
    } catch (const nlohmann::json::exception&) {
      continue;
    }
  }

  const bool windowsChanged = !sameWindowSnapshot(m_windows, next);
  m_windows = std::move(next);

  if (windowsChanged && m_changeCallback) {
    m_changeCallback();
  }
}

void JayWorkspaceBackend::cleanup() {
  m_windows.clear();
  m_changeCallback = {};
}
