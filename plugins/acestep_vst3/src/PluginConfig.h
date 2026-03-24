#pragma once

namespace acestep::vst3
{
inline constexpr const char* kPluginName = "ACE-Step VST3 Shell";
inline constexpr const char* kVendorName = "ACE-Step";
inline constexpr const char* kPluginVersion = "0.1.0";
inline constexpr const char* kDefaultBackendBaseUrl = "http://localhost:8080";
inline constexpr const char* kStateRootTag = "acestep_vst3_shell_state";
inline constexpr int kCurrentStateVersion = 2;
inline constexpr int kDefaultDurationSeconds = 30;
inline constexpr int kDefaultSeed = 12345;
inline constexpr int kResultSlotCount = 3;
}  // namespace acestep::vst3
