#include "PluginPreview.h"

namespace acestep::vst3
{
PluginPreview::PluginPreview()
{
    formatManager_.registerBasicFormats();
}

PluginPreview::~PluginPreview()
{
    clear();
}

void PluginPreview::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const juce::ScopedLock lock(lock_);
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
    transportSource_.prepareToPlay(samplesPerBlock, sampleRate_);
}

void PluginPreview::releaseResources()
{
    const juce::ScopedLock lock(lock_);
    transportSource_.releaseResources();
}

void PluginPreview::render(juce::AudioBuffer<float>& buffer)
{
    const juce::ScopedTryLock lock(lock_);
    if (!lock.isLocked() || readerSource_ == nullptr)
    {
        return;
    }

    juce::AudioSourceChannelInfo channelInfo(&buffer, 0, buffer.getNumSamples());
    transportSource_.getNextAudioBlock(channelInfo);
}

bool PluginPreview::loadFile(const juce::File& file, juce::String& errorMessage)
{
    errorMessage = {};
    if (!file.existsAsFile())
    {
        errorMessage = "Preview file not found.";
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));
    if (reader == nullptr)
    {
        errorMessage = "Preview file format is not supported.";
        return false;
    }

    const auto readerSampleRate = reader->sampleRate;
    auto nextReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);

    const juce::ScopedLock lock(lock_);
    transportSource_.stop();
    transportSource_.setSource(nullptr);
    readerSource_.reset();
    transportSource_.setSource(nextReaderSource.get(), 0, nullptr, readerSampleRate);
    readerSource_ = std::move(nextReaderSource);
    currentFile_ = file;
    return true;
}

void PluginPreview::clear()
{
    const juce::ScopedLock lock(lock_);
    transportSource_.stop();
    transportSource_.setSource(nullptr);
    readerSource_.reset();
    currentFile_ = juce::File();
}

void PluginPreview::play()
{
    const juce::ScopedLock lock(lock_);
    if (readerSource_ == nullptr)
    {
        return;
    }

    transportSource_.setPosition(0.0);
    transportSource_.start();
}

void PluginPreview::stop()
{
    const juce::ScopedLock lock(lock_);
    transportSource_.stop();
}

void PluginPreview::revealToUser() const
{
    const juce::ScopedLock lock(lock_);
    if (currentFile_.existsAsFile())
    {
        currentFile_.revealToUser();
    }
}

bool PluginPreview::hasLoadedFile() const
{
    const juce::ScopedLock lock(lock_);
    return readerSource_ != nullptr && currentFile_.existsAsFile();
}

bool PluginPreview::isPlaying() const
{
    const juce::ScopedLock lock(lock_);
    return transportSource_.isPlaying();
}

juce::String PluginPreview::getDisplayName() const
{
    const juce::ScopedLock lock(lock_);
    return currentFile_.existsAsFile() ? currentFile_.getFileName() : juce::String();
}

juce::String PluginPreview::getFilePath() const
{
    const juce::ScopedLock lock(lock_);
    return currentFile_.getFullPathName();
}
}  // namespace acestep::vst3
