#pragma once

class WaylandConnection;

namespace compositors::jay {
  [[nodiscard]] bool setOutputPower(WaylandConnection& wayland, bool on);

} // namespace compositors::jay
