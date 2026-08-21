#pragma once

#include "compositors/workspace_backend.h"

#include <vector>

class JayWorkspaceBackend final : public compositors::WorkspaceMetadataBackend {
public:
  JayWorkspaceBackend();
  ~JayWorkspaceBackend() override;

  void setChangeCallback(ChangeCallback callback) override;
  [[nodiscard]] int pollTimeoutMs() const noexcept override;
  void dispatchPoll(short revents) override;
  void notifyToplevelChange() override;

  [[nodiscard]] std::vector<WorkspaceWindow> workspaceWindows(const std::string& outputName = {}) const override;

  void cleanup() override;

private:
  void refresh();

  ChangeCallback m_changeCallback;
  std::vector<WorkspaceWindow> m_windows;
  bool m_available = false;
  bool m_refreshRequested = false;
};
