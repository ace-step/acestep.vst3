#pragma once

#include "PluginState.h"

namespace acestep::vst3
{
[[nodiscard]] bool beginMockGeneration(PluginState& state);
void advanceMockGeneration(PluginState& state, int phaseIndex);
}  // namespace acestep::vst3
