#pragma once

#include <JuceHeader.h>

#include "PluginEnums.h"

namespace acestep::vst3
{
class StatusStripComponent final : public juce::Component
{
public:
    StatusStripComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSessionName(const juce::String& sessionName);
    void setModeName(const juce::String& modeName);
    void setBackendStatus(BackendStatus status);

private:
    juce::Label brandLabel_;
    juce::Label sessionLabel_;
    juce::Label modeLabel_;
    juce::Label backendLabel_;
    BackendStatus backendStatus_ = BackendStatus::offline;
};
}  // namespace acestep::vst3
