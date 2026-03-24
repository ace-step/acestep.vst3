#include "PluginMockGeneration.h"

#include "PluginEnums.h"

namespace acestep::vst3
{
namespace
{
juce::String buildResultLabel(const PluginState& state, int slotIndex)
{
    const auto prompt = state.prompt.isEmpty() ? "Untitled prompt" : state.prompt.upToFirstOccurrenceOf("\n", false, false);
    return "Result " + juce::String(slotIndex + 1) + " - " + prompt.substring(0, 28) + " ("
           + juce::String(state.durationSeconds) + "s, " + toString(state.qualityMode) + ")";
}
}  // namespace

bool beginMockGeneration(PluginState& state)
{
    state.errorMessage = {};
    if (state.prompt.trim().isEmpty())
    {
        state.jobStatus = JobStatus::failed;
        state.errorMessage = "Prompt is required before generation.";
        return false;
    }

    if (state.backendStatus == BackendStatus::offline)
    {
        state.jobStatus = JobStatus::failed;
        state.errorMessage = "Backend is offline. Update the backend status before generating.";
        return false;
    }

    state.jobStatus = JobStatus::submitting;
    return true;
}

void advanceMockGeneration(PluginState& state, int phaseIndex)
{
    if (phaseIndex == 0)
    {
        state.jobStatus = JobStatus::queuedOrRunning;
        state.errorMessage = {};
        return;
    }

    state.jobStatus = JobStatus::succeeded;
    state.errorMessage = {};
    for (int index = 0; index < kResultSlotCount; ++index)
    {
        state.resultSlots[static_cast<size_t>(index)] = buildResultLabel(state, index);
    }
    state.selectedResultSlot = 0;
}
}  // namespace acestep::vst3
