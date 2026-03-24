#pragma once

#include <JuceHeader.h>

namespace acestep::vst3
{
class CompositionLaneComponent final : public juce::Component
{
public:
    CompositionLaneComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::TextEditor& projectNameEditor() noexcept;
    juce::TextEditor& sectionPlanEditor() noexcept;
    juce::TextEditor& chordProgressionEditor() noexcept;
    juce::TextEditor& exportNotesEditor() noexcept;
    juce::TextButton& exportButton() noexcept;
    void setExportStatus(const juce::String& status);

private:
    juce::Label projectNameLabel_;
    juce::Label sectionPlanLabel_;
    juce::Label chordProgressionLabel_;
    juce::Label exportNotesLabel_;
    juce::Label exportStatusLabel_;
    juce::TextEditor projectNameEditor_;
    juce::TextEditor sectionPlanEditor_;
    juce::TextEditor chordProgressionEditor_;
    juce::TextEditor exportNotesEditor_;
    juce::TextButton exportButton_ {"Export Session"};
};
}  // namespace acestep::vst3
