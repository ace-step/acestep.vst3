#pragma once

#include <array>

#include <JuceHeader.h>

#include "PluginEnums.h"
#include "PluginState.h"

namespace acestep::vst3
{
struct PluginBackendResultSlot final
{
    juce::String label;
    juce::String remoteFileUrl;
};

struct PluginHealthCheckResult final
{
    BackendStatus status = BackendStatus::offline;
    juce::String errorMessage;
};

struct PluginGenerationStartResult final
{
    bool succeeded = false;
    juce::String taskId;
    juce::String errorMessage;
};

struct PluginGenerationPollResult final
{
    JobStatus status = JobStatus::failed;
    juce::String progressText;
    juce::String errorMessage;
    std::array<PluginBackendResultSlot, static_cast<size_t>(kResultSlotCount)> resultSlots;
};

struct PluginPreviewDownloadResult final
{
    int slotIndex = -1;
    bool succeeded = false;
    juce::String localFilePath;
    juce::String displayName;
    juce::String errorMessage;
};

struct PluginLoRAStatusResult final
{
    bool succeeded = false;
    bool loaded = false;
    bool useLora = false;
    double scale = 1.0;
    juce::String activeAdapter;
    juce::StringArray adapters;
    juce::String errorMessage;
};

struct PluginLoRAOperationResult final
{
    bool succeeded = false;
    juce::String message;
    PluginLoRAStatusResult status;
    juce::String errorMessage;
};

class PluginBackendClient final
{
public:
    [[nodiscard]] PluginHealthCheckResult checkHealth(const juce::String& baseUrl) const;
    [[nodiscard]] PluginGenerationStartResult startGeneration(const PluginState& state) const;
    [[nodiscard]] PluginGenerationPollResult pollGeneration(const juce::String& baseUrl,
                                                            const juce::String& taskId) const;
    [[nodiscard]] PluginPreviewDownloadResult downloadPreviewFile(const juce::String& baseUrl,
                                                                  const juce::String& remoteFileUrl,
                                                                  int slotIndex) const;
    [[nodiscard]] PluginLoRAStatusResult getLoRAStatus(const juce::String& baseUrl) const;
    [[nodiscard]] PluginLoRAOperationResult loadLoRA(const juce::String& baseUrl,
                                                     const juce::String& loraPath) const;
    [[nodiscard]] PluginLoRAOperationResult unloadLoRA(const juce::String& baseUrl) const;
    [[nodiscard]] PluginLoRAOperationResult toggleLoRA(const juce::String& baseUrl,
                                                       bool useLora) const;
    [[nodiscard]] PluginLoRAOperationResult setLoRAScale(const juce::String& baseUrl,
                                                         double scale,
                                                         const juce::String& adapterName) const;
};
}  // namespace acestep::vst3
