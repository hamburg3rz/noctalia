#include "compositors/generic/generic_toplevel_occupancy_backend.h"

#include "wayland/wayland_connection.h"
#include "wayland/wayland_toplevels.h"

namespace compositors::generic {

  GenericToplevelOccupancyBackend::GenericToplevelOccupancyBackend(const WaylandConnection& wayland) noexcept
      : m_wayland(wayland) {}

  void GenericToplevelOccupancyBackend::setChangeCallback(ChangeCallback callback) {
    // Occupancy here is derived synchronously from state WaylandConnection
    // already tracks (wlr-foreign-toplevel-management), so there's no
    // independent event source to notify on. Re-evaluation already happens
    // through the existing toplevel-change callback wired in
    // application_services.cpp, which calls Dock::scheduleSmartAutoHideReevaluation()
    // on every map/unmap/output move. We still store the callback so this
    // stays a well-behaved WorkspaceMetadataBackend implementation.
    m_changeCallback = std::move(callback);
  }

  void GenericToplevelOccupancyBackend::apply(std::vector<Workspace>& workspaces, const std::string& outputName) const {
    if (workspaces.empty()) {
      return;
    }
    if (!m_wayland.hasForeignToplevelManager()) {
      // No wlr-foreign-toplevel-management global bound (either not
      // supported by the compositor, or - if this is being called very
      // early during startup, before WaylandConnection::connect() has run -
      // not known yet). Leave workspaces untouched either way; this is
      // checked lazily on every call rather than once at construction time
      // specifically so it self-corrects once the connection is live.
      return;
    }

    wl_output* target = nullptr;
    if (!outputName.empty()) {
      for (const auto& candidate : m_wayland.outputs()) {
        if (candidate.connectorName == outputName) {
          target = candidate.output;
          break;
        }
      }
      if (target == nullptr) {
        // Unknown output name - nothing we can say, leave workspaces as-is.
        return;
      }
    }

    bool occupied = false;
    m_wayland.visitWlrToplevels([&](const WlrToplevelSnapshot& toplevel) {
      if (occupied || toplevel.minimized) {
        return;
      }
      if (target == nullptr || toplevel.output == target) {
        occupied = true;
      }
    });

    if (!occupied) {
      return;
    }

    // We can only tell whether the output as a whole has a mapped window,
    // not which workspace it belongs to, so mark whichever workspace is
    // currently active. This is exact for single-active-workspace-per-output
    // tiling compositors as long as all of a given output's windows live on
    // the currently active workspace; see the header comment for the case
    // where that doesn't hold.
    for (auto& workspace : workspaces) {
      if (workspace.active) {
        workspace.occupied = true;
      }
    }
  }

  void GenericToplevelOccupancyBackend::cleanup() {
    // No owned resources - WaylandConnection outlives this backend.
  }

} // namespace compositors::generic
