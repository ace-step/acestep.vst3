#include "PluginEnums.h"

namespace acestep::vst3
{
namespace
{
juce::String normalized(juce::String value)
{
    return value.trim().toLowerCase();
}
}  // namespace

juce::String toString(BackendStatus status)
{
    switch (status)
    {
        case BackendStatus::ready:
            return "Ready";
        case BackendStatus::offline:
            return "Offline";
        case BackendStatus::degraded:
            return "Degraded";
    }

    return "Ready";
}

juce::String toString(JobStatus status)
{
    switch (status)
    {
        case JobStatus::idle:
            return "Idle";
        case JobStatus::submitting:
            return "Submitting";
        case JobStatus::queuedOrRunning:
            return "Queued / Running";
        case JobStatus::succeeded:
            return "Succeeded";
        case JobStatus::failed:
            return "Failed";
    }

    return "Idle";
}

juce::String toString(TransportState state)
{
    switch (state)
    {
        case TransportState::idle:
            return "Idle";
        case TransportState::submitting:
            return "Submitting";
        case TransportState::queued:
            return "Queued";
        case TransportState::rendering:
            return "Rendering";
        case TransportState::succeeded:
            return "Succeeded";
        case TransportState::failed:
            return "Failed";
        case TransportState::compareReady:
            return "Compare Ready";
    }

    return "Idle";
}

juce::String toString(ModelPreset preset)
{
    switch (preset)
    {
        case ModelPreset::turbo:
            return "Turbo";
        case ModelPreset::standard:
            return "Standard";
        case ModelPreset::quality:
            return "Quality";
    }

    return "Turbo";
}

juce::String toString(QualityMode mode)
{
    switch (mode)
    {
        case QualityMode::fast:
            return "Fast";
        case QualityMode::balanced:
            return "Balanced";
        case QualityMode::high:
            return "High";
    }

    return "Balanced";
}

juce::String toString(WorkflowMode mode)
{
    switch (mode)
    {
        case WorkflowMode::text:
            return "Text";
        case WorkflowMode::reference:
            return "Reference";
        case WorkflowMode::coverRemix:
            return "Cover / Remix";
        case WorkflowMode::customConditioning:
            return "Custom Conditioning";
    }

    return "Text";
}

BackendStatus backendStatusFromString(const juce::String& value)
{
    const auto key = normalized(value);
    if (key == "offline")
    {
        return BackendStatus::offline;
    }
    if (key == "degraded")
    {
        return BackendStatus::degraded;
    }
    return BackendStatus::ready;
}

JobStatus jobStatusFromString(const juce::String& value)
{
    const auto key = normalized(value);
    if (key == "submitting")
    {
        return JobStatus::submitting;
    }
    if (key == "queued / running" || key == "queued/running" || key == "queued_or_running")
    {
        return JobStatus::queuedOrRunning;
    }
    if (key == "succeeded")
    {
        return JobStatus::succeeded;
    }
    if (key == "failed")
    {
        return JobStatus::failed;
    }
    return JobStatus::idle;
}

TransportState transportStateFromString(const juce::String& value)
{
    const auto key = normalized(value);
    if (key == "submitting")
    {
        return TransportState::submitting;
    }
    if (key == "queued")
    {
        return TransportState::queued;
    }
    if (key == "rendering")
    {
        return TransportState::rendering;
    }
    if (key == "succeeded")
    {
        return TransportState::succeeded;
    }
    if (key == "failed")
    {
        return TransportState::failed;
    }
    if (key == "compare ready" || key == "compare_ready" || key == "compare-ready")
    {
        return TransportState::compareReady;
    }
    return TransportState::idle;
}

ModelPreset modelPresetFromString(const juce::String& value)
{
    const auto key = normalized(value);
    if (key == "standard")
    {
        return ModelPreset::standard;
    }
    if (key == "quality")
    {
        return ModelPreset::quality;
    }
    return ModelPreset::turbo;
}

QualityMode qualityModeFromString(const juce::String& value)
{
    const auto key = normalized(value);
    if (key == "fast")
    {
        return QualityMode::fast;
    }
    if (key == "high")
    {
        return QualityMode::high;
    }
    return QualityMode::balanced;
}

WorkflowMode workflowModeFromString(const juce::String& value)
{
    const auto key = normalized(value);
    if (key == "reference")
    {
        return WorkflowMode::reference;
    }
    if (key == "cover / remix" || key == "cover/remix" || key == "cover_remix")
    {
        return WorkflowMode::coverRemix;
    }
    if (key == "custom conditioning" || key == "custom_conditioning"
        || key == "custom-conditioning")
    {
        return WorkflowMode::customConditioning;
    }
    return WorkflowMode::text;
}
}  // namespace acestep::vst3
