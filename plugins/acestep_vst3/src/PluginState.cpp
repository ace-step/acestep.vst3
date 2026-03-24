#include "PluginState.h"

namespace acestep::vst3
{
namespace
{
void syncLegacyResultFields(PluginState& state)
{
    for (size_t index = 0; index < state.resultTakes.size(); ++index)
    {
        state.resultSlots[index] = state.resultTakes[index].slotLabel;
        state.resultFileUrls[index] = state.resultTakes[index].remoteFileUrl;
        state.resultLocalPaths[index] = state.resultTakes[index].localFilePath;
    }
}

void importLegacyResultFields(PluginState& state)
{
    for (size_t index = 0; index < state.resultTakes.size(); ++index)
    {
        auto& take = state.resultTakes[index];
        if (take.slotLabel.isEmpty())
        {
            take.slotLabel = state.resultSlots[index];
        }
        if (take.remoteFileUrl.isEmpty())
        {
            take.remoteFileUrl = state.resultFileUrls[index];
        }
        if (take.localFilePath.isEmpty())
        {
            take.localFilePath = state.resultLocalPaths[index];
        }
    }
}
}  // namespace

std::unique_ptr<juce::XmlElement> createStateXml(const PluginState& state)
{
    auto xml = std::make_unique<juce::XmlElement>(kStateRootTag);
    xml->setAttribute("schemaVersion", state.schemaVersion);
    xml->setAttribute("backendBaseUrl", state.backendBaseUrl);
    xml->setAttribute("prompt", state.prompt);
    xml->setAttribute("lyrics", state.lyrics);
    xml->setAttribute("referenceAudioPath", state.referenceAudioPath);
    xml->setAttribute("sourceAudioPath", state.sourceAudioPath);
    xml->setAttribute("customConditioningCodes", state.customConditioningCodes);
    xml->setAttribute("sectionPlan", state.sectionPlan);
    xml->setAttribute("chordProgression", state.chordProgression);
    xml->setAttribute("exportNotes", state.exportNotes);
    xml->setAttribute("lastExportPath", state.lastExportPath);
    xml->setAttribute("durationSeconds", state.durationSeconds);
    xml->setAttribute("seed", state.seed);
    xml->setAttribute("audioCoverStrength", state.audioCoverStrength);
    xml->setAttribute("modelPreset", toString(state.modelPreset));
    xml->setAttribute("qualityMode", toString(state.qualityMode));
    xml->setAttribute("backendStatus", toString(state.backendStatus));
    xml->setAttribute("jobStatus", toString(state.jobStatus));
    xml->setAttribute("transportState", toString(state.transportState));
    xml->setAttribute("workflowMode", toString(state.workflowMode));
    xml->setAttribute("loraPath", state.loraPath);
    xml->setAttribute("loraLoaded", state.loraLoaded);
    xml->setAttribute("useLora", state.useLora);
    xml->setAttribute("loraScale", state.loraScale);
    xml->setAttribute("loraStatusText", state.loraStatusText);
    xml->setAttribute("activeLoraAdapter", state.activeLoraAdapter);
    xml->setAttribute("loraAdapters", state.loraAdapters.joinIntoString("||"));
    xml->setAttribute("currentTaskId", state.currentTaskId);
    xml->setAttribute("progressText", state.progressText);
    xml->setAttribute("errorMessage", state.errorMessage);
    xml->setAttribute("sessionName", state.session.sessionName);
    xml->setAttribute("projectName", state.session.projectName);
    xml->setAttribute("lastCompletedSlot", state.session.lastCompletedSlot);
    xml->setAttribute("comparePrimarySlot", state.session.comparePrimarySlot);
    xml->setAttribute("compareSecondarySlot", state.session.compareSecondarySlot);
    xml->setAttribute("canCancelActiveTask", state.session.canCancelActiveTask);
    xml->setAttribute("selectedResultSlot", state.selectedResultSlot);
    xml->setAttribute("compareOnPrimary", state.compareOnPrimary);
    xml->setAttribute("previewFilePath", state.previewFilePath);
    xml->setAttribute("previewDisplayName", state.previewDisplayName);
    for (size_t index = 0; index < state.resultSlots.size(); ++index)
    {
        const auto& take = state.resultTakes[index];
        xml->setAttribute("resultTakeLabel" + juce::String(static_cast<int>(index)),
                          take.slotLabel);
        xml->setAttribute("resultTakeRemoteUrl" + juce::String(static_cast<int>(index)),
                          take.remoteFileUrl);
        xml->setAttribute("resultTakeLocalPath" + juce::String(static_cast<int>(index)),
                          take.localFilePath);
        xml->setAttribute("resultTakeStatusText" + juce::String(static_cast<int>(index)),
                          take.statusText);
        xml->setAttribute("resultTakeCompareGroup" + juce::String(static_cast<int>(index)),
                          take.compareGroup);
        xml->setAttribute("resultTakeSeed" + juce::String(static_cast<int>(index)), take.seed);
        xml->setAttribute("resultTakeDuration" + juce::String(static_cast<int>(index)),
                          take.durationSeconds);
        xml->setAttribute("resultTakeModelPreset" + juce::String(static_cast<int>(index)),
                          toString(take.modelPreset));
        xml->setAttribute("resultTakeQualityMode" + juce::String(static_cast<int>(index)),
                          toString(take.qualityMode));
        xml->setAttribute("resultTakeUpdatedAtMs" + juce::String(static_cast<int>(index)),
                          juce::String(take.updatedAtMs));
        xml->setAttribute("resultTakeReadyForCompare" + juce::String(static_cast<int>(index)),
                          take.readyForCompare);
        xml->setAttribute("resultSlot" + juce::String(static_cast<int>(index)),
                          take.slotLabel.isNotEmpty() ? take.slotLabel : state.resultSlots[index]);
        xml->setAttribute("resultFileUrl" + juce::String(static_cast<int>(index)),
                          take.remoteFileUrl.isNotEmpty() ? take.remoteFileUrl
                                                          : state.resultFileUrls[index]);
        xml->setAttribute("resultLocalPath" + juce::String(static_cast<int>(index)),
                          take.localFilePath.isNotEmpty() ? take.localFilePath
                                                          : state.resultLocalPaths[index]);
    }
    return xml;
}

std::optional<PluginState> parseStateXml(const juce::XmlElement& xml)
{
    if (!xml.hasTagName(kStateRootTag))
    {
        return std::nullopt;
    }

    PluginState state;
    state.schemaVersion = xml.getIntAttribute("schemaVersion", kCurrentStateVersion);
    state.backendBaseUrl = xml.getStringAttribute("backendBaseUrl", kDefaultBackendBaseUrl).trim();
    state.prompt = xml.getStringAttribute("prompt");
    state.lyrics = xml.getStringAttribute("lyrics");
    state.referenceAudioPath = xml.getStringAttribute("referenceAudioPath");
    state.sourceAudioPath = xml.getStringAttribute("sourceAudioPath");
    state.customConditioningCodes = xml.getStringAttribute("customConditioningCodes");
    state.sectionPlan = xml.getStringAttribute("sectionPlan");
    state.chordProgression = xml.getStringAttribute("chordProgression");
    state.exportNotes = xml.getStringAttribute("exportNotes");
    state.lastExportPath = xml.getStringAttribute("lastExportPath");
    state.durationSeconds = xml.getIntAttribute("durationSeconds", kDefaultDurationSeconds);
    state.seed = xml.getIntAttribute("seed", kDefaultSeed);
    state.audioCoverStrength = xml.getDoubleAttribute("audioCoverStrength", 0.6);
    state.modelPreset = modelPresetFromString(xml.getStringAttribute("modelPreset"));
    state.qualityMode = qualityModeFromString(xml.getStringAttribute("qualityMode"));
    state.backendStatus = backendStatusFromString(xml.getStringAttribute("backendStatus"));
    state.jobStatus = jobStatusFromString(xml.getStringAttribute("jobStatus"));
    state.transportState = transportStateFromString(xml.getStringAttribute("transportState"));
    state.workflowMode = workflowModeFromString(xml.getStringAttribute("workflowMode"));
    state.loraPath = xml.getStringAttribute("loraPath");
    state.loraLoaded = xml.getBoolAttribute("loraLoaded", false);
    state.useLora = xml.getBoolAttribute("useLora", false);
    state.loraScale = xml.getDoubleAttribute("loraScale", 1.0);
    state.loraStatusText = xml.getStringAttribute("loraStatusText");
    state.activeLoraAdapter = xml.getStringAttribute("activeLoraAdapter");
    state.loraAdapters.addTokens(xml.getStringAttribute("loraAdapters"), "||", {});
    state.currentTaskId = xml.getStringAttribute("currentTaskId");
    state.progressText = xml.getStringAttribute("progressText");
    state.errorMessage = xml.getStringAttribute("errorMessage");
    state.session.sessionName = xml.getStringAttribute("sessionName");
    state.session.projectName = xml.getStringAttribute("projectName");
    state.session.lastCompletedSlot = xml.getIntAttribute("lastCompletedSlot", -1);
    state.session.comparePrimarySlot = xml.getIntAttribute("comparePrimarySlot", -1);
    state.session.compareSecondarySlot = xml.getIntAttribute("compareSecondarySlot", -1);
    state.session.canCancelActiveTask = xml.getBoolAttribute("canCancelActiveTask", false);
    state.selectedResultSlot =
        juce::jlimit(0, kResultSlotCount - 1, xml.getIntAttribute("selectedResultSlot", 0));
    state.compareOnPrimary = xml.getBoolAttribute("compareOnPrimary", true);
    state.previewFilePath = xml.getStringAttribute("previewFilePath");
    state.previewDisplayName = xml.getStringAttribute("previewDisplayName");
    for (size_t index = 0; index < state.resultSlots.size(); ++index)
    {
        state.resultSlots[index] =
            xml.getStringAttribute("resultSlot" + juce::String(static_cast<int>(index)));
        state.resultFileUrls[index] =
            xml.getStringAttribute("resultFileUrl" + juce::String(static_cast<int>(index)));
        state.resultLocalPaths[index] =
            xml.getStringAttribute("resultLocalPath" + juce::String(static_cast<int>(index)));

        auto& take = state.resultTakes[index];
        take.slotLabel =
            xml.getStringAttribute("resultTakeLabel" + juce::String(static_cast<int>(index)));
        take.remoteFileUrl =
            xml.getStringAttribute("resultTakeRemoteUrl" + juce::String(static_cast<int>(index)));
        take.localFilePath =
            xml.getStringAttribute("resultTakeLocalPath" + juce::String(static_cast<int>(index)));
        take.statusText =
            xml.getStringAttribute("resultTakeStatusText" + juce::String(static_cast<int>(index)));
        take.compareGroup = xml.getStringAttribute("resultTakeCompareGroup"
                                                   + juce::String(static_cast<int>(index)));
        take.seed =
            xml.getIntAttribute("resultTakeSeed" + juce::String(static_cast<int>(index)),
                                kDefaultSeed);
        take.durationSeconds =
            xml.getIntAttribute("resultTakeDuration" + juce::String(static_cast<int>(index)),
                                kDefaultDurationSeconds);
        take.modelPreset = modelPresetFromString(
            xml.getStringAttribute("resultTakeModelPreset" + juce::String(static_cast<int>(index))));
        take.qualityMode = qualityModeFromString(
            xml.getStringAttribute("resultTakeQualityMode" + juce::String(static_cast<int>(index))));
        take.updatedAtMs = xml.getStringAttribute("resultTakeUpdatedAtMs"
                                                  + juce::String(static_cast<int>(index)))
                               .getLargeIntValue();
        take.readyForCompare = xml.getBoolAttribute("resultTakeReadyForCompare"
                                                        + juce::String(static_cast<int>(index)),
                                                    false);
    }

    if (state.backendBaseUrl.isEmpty())
    {
        state.backendBaseUrl = kDefaultBackendBaseUrl;
    }

    importLegacyResultFields(state);
    syncLegacyResultFields(state);
    if (state.transportState == TransportState::idle)
    {
        if (state.jobStatus == JobStatus::submitting)
        {
            state.transportState = TransportState::submitting;
        }
        else if (state.jobStatus == JobStatus::queuedOrRunning)
        {
            state.transportState = TransportState::rendering;
        }
        else if (state.jobStatus == JobStatus::succeeded)
        {
            state.transportState = TransportState::succeeded;
        }
        else if (state.jobStatus == JobStatus::failed)
        {
            state.transportState = TransportState::failed;
        }
    }

    return state;
}
}  // namespace acestep::vst3
