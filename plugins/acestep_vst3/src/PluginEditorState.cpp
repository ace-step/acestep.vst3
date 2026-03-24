#include "PluginEditor.h"

#include "PluginConfig.h"
#include "PluginEnums.h"
#include "PluginProcessor.h"

namespace acestep::vst3
{
namespace
{
juce::String formatFileSummary(const juce::String& prefix, const juce::String& path)
{
    if (path.isEmpty())
    {
        return prefix + " // none";
    }

    const auto file = juce::File(path);
    const auto name = file.getFileName();
    return name.isEmpty() ? prefix + " // linked" : prefix + " // " + name;
}
}  // namespace

void ACEStepVST3AudioProcessorEditor::configureLabels()
{
    refreshStatusViews();
}

void ACEStepVST3AudioProcessorEditor::configureEditors()
{
    auto& backendEditor = synthPanel_.backendUrlEditor();
    auto& modeBox = synthPanel_.modeBox();
    auto& promptEditor = synthPanel_.promptEditor();
    auto& lyricsEditor = synthPanel_.lyricsEditor();
    auto& referenceAudioEditor = synthPanel_.referenceAudioEditor();
    auto& sourceAudioEditor = synthPanel_.sourceAudioEditor();
    auto& conditioningCodesEditor = synthPanel_.conditioningCodesEditor();
    auto& loraPathEditor = synthPanel_.loraPathEditor();
    auto& seedEditor = synthPanel_.seedEditor();
    auto& coverStrengthSlider = synthPanel_.coverStrengthSlider();
    auto& loraScaleSlider = synthPanel_.loraScaleSlider();
    auto& projectNameEditor = compositionLane_.projectNameEditor();
    auto& sectionPlanEditor = compositionLane_.sectionPlanEditor();
    auto& chordProgressionEditor = compositionLane_.chordProgressionEditor();
    auto& exportNotesEditor = compositionLane_.exportNotesEditor();

    backendEditor.setTextToShowWhenEmpty(kDefaultBackendBaseUrl, juce::Colours::grey);
    backendEditor.onTextChange = [this] { persistTextFields(); };

    modeBox.addItem(toString(WorkflowMode::text), 1);
    modeBox.addItem(toString(WorkflowMode::reference), 2);
    modeBox.addItem(toString(WorkflowMode::coverRemix), 3);
    modeBox.addItem(toString(WorkflowMode::customConditioning), 4);
    modeBox.onChange = [this] { persistTextFields(); };

    promptEditor.setTextToShowWhenEmpty("Describe the tape pass you want to print.",
                                        juce::Colours::grey);
    promptEditor.onTextChange = [this] { persistTextFields(); };

    lyricsEditor.setTextToShowWhenEmpty("Optional lyric sketch or arrangement notes.",
                                        juce::Colours::grey);
    lyricsEditor.onTextChange = [this] { persistTextFields(); };

    referenceAudioEditor.setTextToShowWhenEmpty("Reference WAV or render path",
                                                juce::Colours::grey);
    referenceAudioEditor.onTextChange = [this] { persistTextFields(); };

    sourceAudioEditor.setTextToShowWhenEmpty("Source audio path for cover/remix",
                                             juce::Colours::grey);
    sourceAudioEditor.onTextChange = [this] { persistTextFields(); };

    conditioningCodesEditor.setTextToShowWhenEmpty("Semantic audio codes for custom mode",
                                                   juce::Colours::grey);
    conditioningCodesEditor.onTextChange = [this] { persistTextFields(); };

    loraPathEditor.setTextToShowWhenEmpty("/absolute/path/to/adapter", juce::Colours::grey);
    loraPathEditor.onTextChange = [this] { persistTextFields(); };

    projectNameEditor.setTextToShowWhenEmpty("Name this tape session", juce::Colours::grey);
    projectNameEditor.onTextChange = [this] { persistTextFields(); };

    sectionPlanEditor.setTextToShowWhenEmpty("Intro / Verse / Chorus / Bridge",
                                             juce::Colours::grey);
    sectionPlanEditor.onTextChange = [this] { persistTextFields(); };

    chordProgressionEditor.setTextToShowWhenEmpty("Am - F - C - G", juce::Colours::grey);
    chordProgressionEditor.onTextChange = [this] { persistTextFields(); };

    exportNotesEditor.setTextToShowWhenEmpty("Mix notes, arrangement cues, export remarks",
                                             juce::Colours::grey);
    exportNotesEditor.onTextChange = [this] { persistTextFields(); };

    seedEditor.setInputRestrictions(10, "0123456789");
    seedEditor.onTextChange = [this] { persistTextFields(); };
    coverStrengthSlider.onValueChange = [this] { persistTextFields(); };
    loraScaleSlider.onValueChange = [this, &loraScaleSlider] {
        if (isSyncing_)
        {
            return;
        }
        persistTextFields();
        processor_.requestLoRAScale(loraScaleSlider.getValue());
        refreshStatusViews();
    };
}

void ACEStepVST3AudioProcessorEditor::configureSelectors()
{
    auto& durationBox = synthPanel_.durationBox();
    auto& modelBox = synthPanel_.modelBox();
    auto& qualityBox = synthPanel_.qualityBox();
    auto& loraAdapterBox = synthPanel_.loraAdapterBox();
    auto& resultSelector = resultDeck_.resultSelector();
    auto& comparePrimary = resultDeck_.comparePrimarySelector();
    auto& compareSecondary = resultDeck_.compareSecondarySelector();

    for (const auto duration : {10, 30, 60, 120})
    {
        durationBox.addItem(juce::String(duration) + " seconds", duration);
    }
    modelBox.addItem(toString(ModelPreset::turbo), 1);
    modelBox.addItem(toString(ModelPreset::standard), 2);
    modelBox.addItem(toString(ModelPreset::quality), 3);
    qualityBox.addItem(toString(QualityMode::fast), 1);
    qualityBox.addItem(toString(QualityMode::balanced), 2);
    qualityBox.addItem(toString(QualityMode::high), 3);
    loraAdapterBox.setTextWhenNothingSelected("Default");

    durationBox.onChange = [this] { persistTextFields(); };
    modelBox.onChange = [this] { persistTextFields(); };
    qualityBox.onChange = [this] { persistTextFields(); };
    loraAdapterBox.onChange = [this] { persistTextFields(); };

    synthPanel_.chooseReferenceButton().onClick = [this] { chooseReferenceFile(); };
    synthPanel_.clearReferenceButton().onClick = [this] { clearReferenceFile(); };
    synthPanel_.chooseSourceButton().onClick = [this] { chooseSourceFile(); };
    synthPanel_.clearSourceButton().onClick = [this] { clearSourceFile(); };
    synthPanel_.useLoRAToggle().onClick = [this] {
        if (isSyncing_)
        {
            return;
        }
        processor_.requestToggleLoRA(synthPanel_.useLoRAToggle().getToggleState());
        refreshStatusViews();
    };
    synthPanel_.loadLoRAButton().onClick = [this] {
        persistTextFields();
        processor_.requestLoadLoRA();
        refreshStatusViews();
    };
    synthPanel_.unloadLoRAButton().onClick = [this] {
        processor_.requestUnloadLoRA();
        refreshStatusViews();
    };

    resultSelector.onChange = [this] {
        if (isSyncing_)
        {
            return;
        }
        processor_.selectResultSlot(juce::jmax(0, resultDeck_.resultSelector().getSelectedItemIndex()));
        refreshStatusViews();
    };
    comparePrimary.onChange = [this] {
        if (isSyncing_)
        {
            return;
        }
        processor_.selectCompareSlots(juce::jmax(0, resultDeck_.comparePrimarySelector().getSelectedItemIndex()),
                                      juce::jmax(0, resultDeck_.compareSecondarySelector().getSelectedItemIndex()));
        refreshStatusViews();
    };
    compareSecondary.onChange = [this] {
        if (isSyncing_)
        {
            return;
        }
        processor_.selectCompareSlots(juce::jmax(0, resultDeck_.comparePrimarySelector().getSelectedItemIndex()),
                                      juce::jmax(0, resultDeck_.compareSecondarySelector().getSelectedItemIndex()));
        refreshStatusViews();
    };
    resultDeck_.cueCompareAButton().onClick = [this] { cueComparePrimary(); };
    resultDeck_.cueCompareBButton().onClick = [this] { cueCompareSecondary(); };
    resultDeck_.toggleCompareButton().onClick = [this] { toggleComparePreview(); };
    resultDeck_.dragToDawButton().onClick = [this] { dragSelectedResultToDaw(); };

    transport_.generateButton().onClick = [this] {
        persistTextFields();
        processor_.requestGeneration();
        refreshStatusViews();
    };
    transport_.auditionButton().onClick = [this] { playPreviewFile(); };
    transport_.stopButton().onClick = [this] { stopPreviewFile(); };
    transport_.revealButton().onClick = [this] { revealPreviewFile(); };
    previewDeck_.loadButton().onClick = [this] { choosePreviewFile(); };
    previewDeck_.playButton().onClick = [this] { playPreviewFile(); };
    previewDeck_.stopButton().onClick = [this] { stopPreviewFile(); };
    previewDeck_.clearButton().onClick = [this] { clearPreviewFile(); };
    previewDeck_.revealButton().onClick = [this] { revealPreviewFile(); };
    compositionLane_.exportButton().onClick = [this] { chooseSessionExportFile(); };
}

void ACEStepVST3AudioProcessorEditor::syncFromProcessor()
{
    const auto& state = processor_.getState();
    isSyncing_ = true;
    synthPanel_.backendUrlEditor().setText(state.backendBaseUrl, juce::dontSendNotification);
    synthPanel_.modeBox().setSelectedId(static_cast<int>(state.workflowMode) + 1,
                                        juce::dontSendNotification);
    synthPanel_.promptEditor().setText(state.prompt, juce::dontSendNotification);
    synthPanel_.lyricsEditor().setText(state.lyrics, juce::dontSendNotification);
    synthPanel_.referenceAudioEditor().setText(state.referenceAudioPath, juce::dontSendNotification);
    synthPanel_.sourceAudioEditor().setText(state.sourceAudioPath, juce::dontSendNotification);
    synthPanel_.conditioningCodesEditor().setText(state.customConditioningCodes,
                                                  juce::dontSendNotification);
    compositionLane_.projectNameEditor().setText(state.session.projectName,
                                                 juce::dontSendNotification);
    compositionLane_.sectionPlanEditor().setText(state.sectionPlan, juce::dontSendNotification);
    compositionLane_.chordProgressionEditor().setText(state.chordProgression,
                                                      juce::dontSendNotification);
    compositionLane_.exportNotesEditor().setText(state.exportNotes,
                                                 juce::dontSendNotification);
    synthPanel_.loraPathEditor().setText(state.loraPath, juce::dontSendNotification);
    synthPanel_.seedEditor().setText(juce::String(state.seed), juce::dontSendNotification);
    synthPanel_.durationBox().setSelectedId(state.durationSeconds, juce::dontSendNotification);
    synthPanel_.modelBox().setSelectedId(static_cast<int>(state.modelPreset) + 1,
                                         juce::dontSendNotification);
    synthPanel_.qualityBox().setSelectedId(static_cast<int>(state.qualityMode) + 1,
                                           juce::dontSendNotification);
    synthPanel_.coverStrengthSlider().setValue(state.audioCoverStrength,
                                               juce::dontSendNotification);
    synthPanel_.loraScaleSlider().setValue(state.loraScale, juce::dontSendNotification);
    synthPanel_.useLoRAToggle().setToggleState(state.useLora, juce::dontSendNotification);
    isSyncing_ = false;
}

void ACEStepVST3AudioProcessorEditor::persistTextFields()
{
    if (isSyncing_)
    {
        return;
    }

    auto& state = processor_.getMutableState();
    auto& backendEditor = synthPanel_.backendUrlEditor();
    auto& modeBox = synthPanel_.modeBox();
    auto& promptEditor = synthPanel_.promptEditor();
    auto& lyricsEditor = synthPanel_.lyricsEditor();
    auto& referenceAudioEditor = synthPanel_.referenceAudioEditor();
    auto& sourceAudioEditor = synthPanel_.sourceAudioEditor();
    auto& conditioningCodesEditor = synthPanel_.conditioningCodesEditor();
    auto& loraPathEditor = synthPanel_.loraPathEditor();
    auto& seedEditor = synthPanel_.seedEditor();
    auto& durationBox = synthPanel_.durationBox();
    auto& modelBox = synthPanel_.modelBox();
    auto& qualityBox = synthPanel_.qualityBox();
    auto& loraAdapterBox = synthPanel_.loraAdapterBox();
    auto& coverStrengthSlider = synthPanel_.coverStrengthSlider();
    auto& loraScaleSlider = synthPanel_.loraScaleSlider();
    auto& projectNameEditor = compositionLane_.projectNameEditor();
    auto& sectionPlanEditor = compositionLane_.sectionPlanEditor();
    auto& chordProgressionEditor = compositionLane_.chordProgressionEditor();
    auto& exportNotesEditor = compositionLane_.exportNotesEditor();

    state.backendBaseUrl = backendEditor.getText().trim();
    if (state.backendBaseUrl.isEmpty())
    {
        state.backendBaseUrl = kDefaultBackendBaseUrl;
        backendEditor.setText(state.backendBaseUrl, juce::dontSendNotification);
    }

    state.workflowMode = static_cast<WorkflowMode>(juce::jmax(0, modeBox.getSelectedItemIndex()));
    state.prompt = promptEditor.getText();
    state.lyrics = lyricsEditor.getText();
    state.referenceAudioPath = referenceAudioEditor.getText();
    state.sourceAudioPath = sourceAudioEditor.getText();
    state.customConditioningCodes = conditioningCodesEditor.getText();
    state.session.projectName = projectNameEditor.getText().trim();
    state.sectionPlan = sectionPlanEditor.getText();
    state.chordProgression = chordProgressionEditor.getText();
    state.exportNotes = exportNotesEditor.getText();
    state.loraPath = loraPathEditor.getText().trim();
    state.durationSeconds = durationBox.getSelectedId() == 0 ? kDefaultDurationSeconds
                                                             : durationBox.getSelectedId();
    state.seed = seedEditor.getText().getIntValue();
    state.audioCoverStrength = coverStrengthSlider.getValue();
    state.loraScale = loraScaleSlider.getValue();
    if (state.seed <= 0)
    {
        state.seed = kDefaultSeed;
        seedEditor.setText(juce::String(state.seed), juce::dontSendNotification);
    }

    state.modelPreset = static_cast<ModelPreset>(juce::jmax(0, modelBox.getSelectedItemIndex()));
    state.qualityMode = static_cast<QualityMode>(juce::jmax(0, qualityBox.getSelectedItemIndex()));
    if (loraAdapterBox.getSelectedItemIndex() >= 0)
    {
        state.activeLoraAdapter = loraAdapterBox.getText();
    }
    state.session.comparePrimarySlot =
        juce::jmax(0, resultDeck_.comparePrimarySelector().getSelectedItemIndex());
    state.session.compareSecondarySlot =
        juce::jmax(0, resultDeck_.compareSecondarySelector().getSelectedItemIndex());
    refreshStatusViews();
}

void ACEStepVST3AudioProcessorEditor::refreshResultSelector()
{
    const auto& state = processor_.getState();
    auto& resultSelector = resultDeck_.resultSelector();
    auto& comparePrimary = resultDeck_.comparePrimarySelector();
    auto& compareSecondary = resultDeck_.compareSecondarySelector();
    auto& loraAdapterBox = synthPanel_.loraAdapterBox();

    isSyncing_ = true;
    resultSelector.clear(juce::dontSendNotification);
    comparePrimary.clear(juce::dontSendNotification);
    compareSecondary.clear(juce::dontSendNotification);
    for (int index = 0; index < kResultSlotCount; ++index)
    {
        auto label = state.resultSlots[static_cast<size_t>(index)];
        if (label.isEmpty())
        {
            label = "Result " + juce::String(index + 1) + " - empty";
        }
        resultSelector.addItem(label, index + 1);
        comparePrimary.addItem(label, index + 1);
        compareSecondary.addItem(label, index + 1);
    }
    resultSelector.setSelectedId(state.selectedResultSlot + 1, juce::dontSendNotification);
    comparePrimary.setSelectedId(state.session.comparePrimarySlot >= 0 ? state.session.comparePrimarySlot + 1
                                                                       : 0,
                                 juce::dontSendNotification);
    compareSecondary.setSelectedId(state.session.compareSecondarySlot >= 0
                                       ? state.session.compareSecondarySlot + 1
                                       : 0,
                                   juce::dontSendNotification);

    loraAdapterBox.clear(juce::dontSendNotification);
    if (state.loraAdapters.isEmpty())
    {
        loraAdapterBox.setText("Default", juce::dontSendNotification);
    }
    else
    {
        auto itemId = 1;
        for (const auto& adapter : state.loraAdapters)
        {
            loraAdapterBox.addItem(adapter, itemId++);
        }
        if (state.activeLoraAdapter.isNotEmpty())
        {
            loraAdapterBox.setText(state.activeLoraAdapter, juce::dontSendNotification);
        }
        else
        {
            loraAdapterBox.setSelectedItemIndex(0, juce::dontSendNotification);
        }
    }
    isSyncing_ = false;
}

void ACEStepVST3AudioProcessorEditor::refreshStatusViews()
{
    const auto& state = processor_.getState();
    const auto sessionName = state.session.sessionName.isEmpty()
                                 ? (state.session.projectName.isEmpty() ? "UNTITLED SESSION"
                                                                        : state.session.projectName.toUpperCase())
                                 : state.session.sessionName.toUpperCase();
    statusStrip_.setSessionName("SESSION // " + sessionName);
    statusStrip_.setModeName(toString(state.workflowMode).toUpperCase() + " MODE");
    statusStrip_.setBackendStatus(state.backendStatus);

    auto transportMessage = state.progressText;
    if (transportMessage.isEmpty())
    {
        transportMessage = "Slot " + juce::String(state.selectedResultSlot + 1) + " armed.";
    }
    transport_.setTransportState(state.backendStatus,
                                 state.jobStatus,
                                 transportMessage,
                                 state.errorMessage,
                                 processor_.hasPreviewFile(),
                                 processor_.isPreviewPlaying());

    auto takeTitle = "Take " + juce::String(state.selectedResultSlot + 1);
    auto takeDetail = state.resultSlots[static_cast<size_t>(state.selectedResultSlot)];
    if (takeDetail.isEmpty())
    {
        takeDetail = "No printed result yet.";
    }
    const auto& localPath = state.resultLocalPaths[static_cast<size_t>(state.selectedResultSlot)];
    const auto& remoteUrl = state.resultFileUrls[static_cast<size_t>(state.selectedResultSlot)];
    if (!localPath.isEmpty())
    {
        takeDetail += "\nFILE // " + juce::File(localPath).getFileName();
        takeDetail += "\nREADY // audition or drag into the DAW";
    }
    else if (!remoteUrl.isEmpty())
    {
        takeDetail += "\nREMOTE // ready on backend";
        takeDetail += "\nURL // " + remoteUrl;
    }
    resultDeck_.setTakeSummary(takeTitle, takeDetail);

    const auto comparePrimarySlot = state.session.comparePrimarySlot >= 0
                                        ? state.session.comparePrimarySlot + 1
                                        : 0;
    const auto compareSecondarySlot = state.session.compareSecondarySlot >= 0
                                          ? state.session.compareSecondarySlot + 1
                                          : 0;
    auto compareSummary = juce::String("COMPARE // ");
    if (comparePrimarySlot == 0 || compareSecondarySlot == 0)
    {
        compareSummary += "Set A and B takes";
    }
    else
    {
        compareSummary += "A" + juce::String(comparePrimarySlot) + " vs B"
                          + juce::String(compareSecondarySlot);
        compareSummary += state.compareOnPrimary ? " // active A" : " // active B";
    }
    resultDeck_.setCompareSummary(compareSummary);
    resultDeck_.toggleCompareButton().setButtonText(state.compareOnPrimary ? "Toggle to B"
                                                                           : "Toggle to A");
    resultDeck_.dragToDawButton().setEnabled(!localPath.isEmpty());

    synthPanel_.loraStatusLabel().setText(state.loraStatusText.isEmpty() ? "No LoRA loaded."
                                                                         : state.loraStatusText,
                                          juce::dontSendNotification);
    synthPanel_.useLoRAToggle().setEnabled(state.loraLoaded);
    synthPanel_.unloadLoRAButton().setEnabled(state.loraLoaded);
    synthPanel_.loraScaleSlider().setEnabled(state.loraLoaded);

    auto previewText = state.previewDisplayName.isEmpty() ? "No preview file loaded."
                                                          : "Loaded // " + state.previewDisplayName;
    previewText += "\n";
    previewText += state.previewFilePath.isEmpty()
                       ? "Choose a local or generated file to preview."
                       : formatFileSummary("PATH", state.previewFilePath) + "\n" + state.previewFilePath;
    if (processor_.isPreviewPlaying())
    {
        previewText += "\nPlayback // active";
    }
    previewDeck_.setPreviewSummary(previewText);

    auto exportText = state.lastExportPath.isEmpty() ? "No session export written yet."
                                                     : "EXPORTED // " + state.lastExportPath;
    compositionLane_.setExportStatus(exportText);
}

void ACEStepVST3AudioProcessorEditor::cueComparePrimary()
{
    processor_.selectCompareSlots(juce::jmax(0, resultDeck_.comparePrimarySelector().getSelectedItemIndex()),
                                  juce::jmax(0, resultDeck_.compareSecondarySelector().getSelectedItemIndex()));
    processor_.cueCompareSlot(true);
    refreshStatusViews();
}

void ACEStepVST3AudioProcessorEditor::cueCompareSecondary()
{
    processor_.selectCompareSlots(juce::jmax(0, resultDeck_.comparePrimarySelector().getSelectedItemIndex()),
                                  juce::jmax(0, resultDeck_.compareSecondarySelector().getSelectedItemIndex()));
    processor_.cueCompareSlot(false);
    refreshStatusViews();
}

void ACEStepVST3AudioProcessorEditor::toggleComparePreview()
{
    processor_.selectCompareSlots(juce::jmax(0, resultDeck_.comparePrimarySelector().getSelectedItemIndex()),
                                  juce::jmax(0, resultDeck_.compareSecondarySelector().getSelectedItemIndex()));
    processor_.toggleCompareSlot();
    refreshStatusViews();
}
}  // namespace acestep::vst3
