#pragma once

#include <array>
#include <memory>
#include <optional>

#include <JuceHeader.h>

#include "PluginConfig.h"
#include "PluginEnums.h"

namespace acestep::vst3
{
struct PluginResultTake final
{
    juce::String slotLabel;
    juce::String remoteFileUrl;
    juce::String localFilePath;
    juce::String statusText;
    juce::String compareGroup;
    int seed = kDefaultSeed;
    int durationSeconds = kDefaultDurationSeconds;
    ModelPreset modelPreset = ModelPreset::turbo;
    QualityMode qualityMode = QualityMode::balanced;
    juce::int64 updatedAtMs = 0;
    bool readyForCompare = false;
};

struct PluginSessionState final
{
    juce::String sessionName;
    juce::String projectName;
    int lastCompletedSlot = -1;
    int comparePrimarySlot = -1;
    int compareSecondarySlot = -1;
    bool canCancelActiveTask = false;
};

struct PluginState final
{
    int schemaVersion = kCurrentStateVersion;
    juce::String backendBaseUrl = kDefaultBackendBaseUrl;
    juce::String prompt;
    juce::String lyrics;
    juce::String referenceAudioPath;
    juce::String sourceAudioPath;
    juce::String customConditioningCodes;
    juce::String sectionPlan;
    juce::String chordProgression;
    juce::String exportNotes;
    juce::String lastExportPath;
    int durationSeconds = kDefaultDurationSeconds;
    int seed = kDefaultSeed;
    double audioCoverStrength = 0.6;
    ModelPreset modelPreset = ModelPreset::turbo;
    QualityMode qualityMode = QualityMode::balanced;
    BackendStatus backendStatus = BackendStatus::ready;
    JobStatus jobStatus = JobStatus::idle;
    TransportState transportState = TransportState::idle;
    WorkflowMode workflowMode = WorkflowMode::text;
    juce::String loraPath;
    bool loraLoaded = false;
    bool useLora = false;
    double loraScale = 1.0;
    juce::String loraStatusText;
    juce::String activeLoraAdapter;
    juce::StringArray loraAdapters;
    juce::String currentTaskId;
    juce::String progressText;
    juce::String errorMessage;
    PluginSessionState session;
    int selectedResultSlot = 0;
    std::array<PluginResultTake, static_cast<size_t>(kResultSlotCount)> resultTakes;
    bool compareOnPrimary = true;
    std::array<juce::String, static_cast<size_t>(kResultSlotCount)> resultSlots;
    std::array<juce::String, static_cast<size_t>(kResultSlotCount)> resultFileUrls;
    std::array<juce::String, static_cast<size_t>(kResultSlotCount)> resultLocalPaths;
    juce::String previewFilePath;
    juce::String previewDisplayName;
};

[[nodiscard]] std::unique_ptr<juce::XmlElement> createStateXml(const PluginState& state);
[[nodiscard]] std::optional<PluginState> parseStateXml(const juce::XmlElement& xml);
}  // namespace acestep::vst3
