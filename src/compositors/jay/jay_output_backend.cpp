#include "compositors/jay/jay_output_backend.h"

#include "core/process/process.h"
#include "wayland/wayland_connection.h"

#include <algorithm>
#include <string>
#include <vector>

namespace compositors::jay {

  bool setOutputPower(WaylandConnection& wayland, bool on) {
    // Jay removes a disabled output from the Wayland output list, so retain
    // connector names we've seen in order to be able to re-enable them later.
    static std::vector<std::string> s_knownConnectors;

    for (const auto& output : wayland.outputs()) {
      if (!output.connectorName.empty()
          && std::ranges::find(s_knownConnectors, output.connectorName) == s_knownConnectors.end()) {
        s_knownConnectors.push_back(output.connectorName);
      }
    }

    bool launchedAny = false;
    for (const auto& connector : s_knownConnectors) {
      if (process::runAsync(std::vector<std::string>{"jay", "randr", "output", connector, on ? "enable" : "disable"})) {
        launchedAny = true;
      }
    }
    return launchedAny;
  }

} // namespace compositors::jay
