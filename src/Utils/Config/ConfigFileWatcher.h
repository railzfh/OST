#pragma once

#include <string>

namespace ConfigFileWatcher {
    void Start(const std::string& configPath, const std::string& defaultLuaDir);
    void Stop();
    // Detach instead of joining during process termination.
    void Abandon();
}
