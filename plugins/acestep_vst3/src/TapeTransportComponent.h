#pragma once

#include <JuceHeader.h>

#include "PluginEnums.h"

namespace acestep::vst3
{
class TapeTransportComponent final : public juce::Component
{
public:
    TapeTransportComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::TextButton& generateButton() noexcept;
    juce::TextButton& auditionButton() noexcept;
    juce::TextButton& stopButton() noexcept;
    juce::TextButton& revealButton() noexcept;
    void setTransportState(BackendStatus backendStatus,
                           JobStatus jobStatus,
                           const juce::String& message,
                           const juce::String& errorText,
                           bool previewLoaded,
                           bool previewPlaying);

private:
    juce::Label backendLabel_;
    juce::Label stateLabel_;
    juce::Label messageLabel_;
    juce::Label errorLabel_;
    juce::TextButton generateButton_ {"Render"};
    juce::TextButton auditionButton_ {"Audition"};
    juce::TextButton stopButton_ {"Stop"};
    juce::TextButton revealButton_ {"Reveal"};
    BackendStatus backendStatus_ = BackendStatus::offline;
    JobStatus jobStatus_ = JobStatus::idle;
    bool previewLoaded_ = false;
    bool previewPlaying_ = false;
};
}  // namespace acestep::vst3
