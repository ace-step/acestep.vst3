#pragma once

#include <JuceHeader.h>

namespace acestep::vst3
{
class PreviewDeckComponent final : public juce::Component
{
public:
    PreviewDeckComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::TextButton& loadButton() noexcept;
    juce::TextButton& playButton() noexcept;
    juce::TextButton& stopButton() noexcept;
    juce::TextButton& clearButton() noexcept;
    juce::TextButton& revealButton() noexcept;
    void setPreviewSummary(const juce::String& summary);

private:
    juce::Label summaryLabel_;
    juce::TextButton loadButton_ {"Load Preview"};
    juce::TextButton playButton_ {"Play"};
    juce::TextButton stopButton_ {"Stop"};
    juce::TextButton clearButton_ {"Clear"};
    juce::TextButton revealButton_ {"Reveal File"};
};
}  // namespace acestep::vst3
