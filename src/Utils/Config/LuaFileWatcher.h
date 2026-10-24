#pragma once

#include <string>
#include <vector>

namespace LuaFileWatcher {
    void Start(const std::vector<std::string>& directories);
    void Stop();
    // Detach instead of joining during process termination.
    void Abandon();
}
