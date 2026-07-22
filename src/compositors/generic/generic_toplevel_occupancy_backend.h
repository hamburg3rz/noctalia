#pragma once

#include "compositors/workspace_backend.h"

#include <string>
#include <vector>

class WaylandConnection;

namespace compositors::generic {

  // Fallback WorkspaceMetadataBackend for compositors with no dedicated
  // integration (e.g. Jay). It has no way to learn the real per-window
  // workspace assignment - the base `ext-workspace-v1` protocol simply
  // doesn't expose that link - but it can still answer "does this output
  // currently have any mapped window" using the already-tracked
  // wlr-foreign-toplevel-management state in WaylandConnection.
  //
  // Caveat: for compositors with multiple workspaces per output where only
  // one is shown at a time (i3/sway-style tiling, which includes Jay),
  // output_enter/output_leave tell you which OUTPUT a toplevel belongs to,
  // not which of that output's workspaces is currently the visible one. So
  // this can't distinguish "empty active workspace, occupied inactive
  // workspace on the same output" from genuine occupancy of the active
  // workspace - it will report the output occupied either way. This is a
  // real limitation, not just an implementation detail: fixing it precisely
  // would require compositor-specific data this protocol doesn't provide.
  //
  // This intentionally only implements apply(); it never returns anything
  // from workspaceWindows(), so callers correctly fall through to the
  // occupied-flag-based path in their empty-assignment fallback instead of
  // assuming an (unavailable) precise per-window mapping.
  class GenericToplevelOccupancyBackend final : public compositors::WorkspaceMetadataBackend {
  public:
    explicit GenericToplevelOccupancyBackend(const WaylandConnection& wayland) noexcept;

    void setChangeCallback(ChangeCallback callback) override;
    void apply(std::vector<Workspace>& workspaces, const std::string& outputName = {}) const override;
    void cleanup() override;

  private:
    const WaylandConnection& m_wayland;
    ChangeCallback m_changeCallback;
  };

} // namespace compositors::generic
