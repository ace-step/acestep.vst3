#pragma once

#include <memory>

#include <JuceHeader.h>

namespace acestep::vst3
{
class PluginPreview final
{
public:
    PluginPreview();
    ~PluginPreview();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void render(juce::AudioBuffer<float>& buffer);

    [[nodiscard]] bool loadFile(const juce::File& file, juce::String& errorMessage);
    void clear();
    void play();
    void stop();
    void revealToUser() const;

    [[nodiscard]] bool hasLoadedFile() const;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] juce::String getDisplayName() const;
    [[nodiscard]] juce::String getFilePath() const;

private:
    mutable juce::CriticalSection lock_;
    juce::AudioFormatManager formatManager_;
    juce::AudioTransportSource transportSource_;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource_;
    juce::File currentFile_;
    double sampleRate_ = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginPreview)
};
}  // namespace acestep::vst3
