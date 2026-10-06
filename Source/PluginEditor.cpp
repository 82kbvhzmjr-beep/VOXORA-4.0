#include "PluginEditor.h"
#include <algorithm>

//==============================================================================
VoxoraLookAndFeel::VoxoraLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, ui::bg);
    setColour (juce::Label::textColourId, ui::dim);
    setColour (juce::Slider::textBoxTextColourId, ui::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::trackColourId, ui::accent);
    setColour (juce::Slider::thumbColourId, ui::text);
    setColour (juce::Slider::backgroundColourId, ui::panel2);
    setColour (juce::ComboBox::backgroundColourId, ui::panel2);
    setColour (juce::ComboBox::textColourId, ui::text);
    setColour (juce::ComboBox::outlineColourId, ui::panel2.brighter (0.2f));
    setColour (juce::ComboBox::arrowColourId, ui::accent);
    setColour (juce::PopupMenu::backgroundColourId, ui::panel);
    setColour (juce::PopupMenu::textColourId, ui::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent2);
    setColour (juce::TextButton::buttonColourId, ui::panel2);
    setColour (juce::TextButton::buttonOnColourId, ui::accent);
    setColour (juce::TextButton::textColourOffId, ui::dim);
    setColour (juce::TextButton::textColourOnId, ui::bg);
    setColour (juce::ToggleButton::textColourId, ui::text);
    setColour (juce::ToggleButton::tickColourId, ui::accent);
    setColour (juce::ToggleButton::tickDisabledColourId, ui::dim);
}

void VoxoraLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcR = r - 3.0f;

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (ui::panel2.brighter (0.2f));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (ui::accent);
    g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float kr = r - 11.0f;
    g.setGradientFill (juce::ColourGradient (ui::panel2.brighter (0.25f), c.x, c.y - kr, ui::panel, c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (ui::panel2.brighter (0.4f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

    juce::Path p;
    p.addRoundedRectangle (-1.5f, -kr + 3.0f, 3.0f, kr * 0.45f, 1.5f);
    g.setColour (ui::text);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& title,
            const juce::String& suffix, int decimals)
    : state (&state)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);

    if (auto* p = state.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

    label.setText (title, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.5f, juce::Font::bold));

    addAndMakeVisible (slider);
    addAndMakeVisible (label);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

void Knob::setTitle (const juce::String& title)
{
    label.setText (title, juce::dontSendNotification);
}

void Knob::setParameter (const juce::String& id)
{
    if (state == nullptr || state->getParameter (id) == nullptr) return;
    attachment.reset();
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (*state, id, slider);
    if (auto* p = state->getParameter (id))
    {
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        slider.setTextValueSuffix ({ });
    }
}

Choice::Choice (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& title)
{
    label.setText (title, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.5f, juce::Font::bold));

    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
        box.addItemList (p->choices, 1);

    addAndMakeVisible (label);
    addAndMakeVisible (box);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, id, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    box.setBounds (r.withSizeKeepingCentre (juce::jmax (40, r.getWidth() - 10), 28));
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ui::panel2);
    g.fillRoundedRectangle (b, 3.0f);
    b = b.reduced (2.0f);

    if (gr)
    {
        const float norm = juce::jlimit (0.0f, 1.0f, level / 20.0f);
        g.setColour (ui::accent2);
        g.fillRoundedRectangle (b.withWidth (b.getWidth() * norm), 2.0f);
    }
    else
    {
        const float db = vx::gainToDb (level);
        const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setGradientFill (juce::ColourGradient (ui::accent, 0.0f, b.getBottom(), ui::warn, 0.0f, b.getY(), false));
        g.fillRoundedRectangle (b.withTrimmedTop (b.getHeight() * (1.0f - norm)), 2.0f);
    }
}

//==============================================================================
namespace
{
void drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (ui::panel);
    g.fillRoundedRectangle (r.toFloat(), 10.0f);
    g.setColour (ui::panel2);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);
    g.setColour (ui::accent);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (title, r.getX() + 14, r.getY() + 6, r.getWidth() - 28, 18, juce::Justification::left);
}

void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items)
{
    const int w = area.getWidth() / (int) items.size();
    for (auto* c : items)
        c->setBounds (area.removeFromLeft (w));
}
} // namespace

//==============================================================================
AssistPage::AssistPage()
{
    for (auto* b : { &originalBtn, &assistedBtn })
    {
        b->setRadioGroupId (7);
        b->setClickingTogglesState (true);
        addChildComponent (b);
    }
    assistedBtn.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (startBtn);
}

void AssistPage::resized()
{
    auto c = getLocalBounds().getCentre();
    startBtn.setBounds (juce::Rectangle<int> (150, 44).withCentre ({ c.x, c.y - 10 }));
    originalBtn.setBounds (juce::Rectangle<int> (110, 34).withCentre ({ c.x - 60, getHeight() - 70 }));
    assistedBtn.setBounds (juce::Rectangle<int> (110, 34).withCentre ({ c.x + 60, getHeight() - 70 }));
}

void AssistPage::refresh (int newState, float newProgress)
{
    state = newState;
    progress = newProgress;

    startBtn.setButtonText (state == 1 ? "Escuchando..." : (state >= 3 ? "Repetir" : "Iniciar"));
    startBtn.setEnabled (state != 1);
    originalBtn.setVisible (state >= 3);
    assistedBtn.setVisible (state >= 3);
    if (state == 3) assistedBtn.setToggleState (true, juce::dontSendNotification);
    if (state == 4) originalBtn.setToggleState (true, juce::dontSendNotification);
    repaint();
}

void AssistPage::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (b.toFloat(), 12.0f);

    g.setColour (ui::accent);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("VOCAL ASSIST", 20, 14, 300, 20, juce::Justification::left);

    const auto c = b.getCentre().toFloat().translated (0.0f, -10.0f);
    const float r = 120.0f;
    const float a0 = juce::MathConstants<float>::pi * 1.0f * 0.0f;

    juce::Path ring;
    ring.addCentredArc (c.x, c.y, r, r, 0.0f, a0, juce::MathConstants<float>::twoPi, true);
    g.setColour (ui::panel2);
    g.strokePath (ring, juce::PathStrokeType (14.0f));

    const float p = (state >= 2) ? 1.0f : progress;
    if (p > 0.001f)
    {
        juce::Path v;
        v.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a0 + juce::MathConstants<float>::twoPi * p, true);
        g.setColour (ui::accent);
        g.strokePath (v, juce::PathStrokeType (14.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (ui::text);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText (juce::String ((int) (p * 100.0f)) + " %", (int) c.x - 80, (int) c.y - 70, 160, 34, juce::Justification::centred);

    g.setColour (ui::dim);
    g.setFont (juce::FontOptions (13.0f));
    juce::String msg = "Pulsa Iniciar y canta o habla unos 6 segundos";
    if (state == 1) msg = "Escuchando... sigue cantando";
    if (state >= 3) msg = "Listo. Compara el sonido original con el asistido";
    g.drawText (msg, getLocalBounds().withTop (getHeight() - 150).withHeight (22), juce::Justification::centred);
}

//==============================================================================
SimplePage::SimplePage (juce::AudioProcessorValueTreeState& s)
    : comp (s, "compAmount", "COMP", "%", 0),
      magic (s, "magic", "MAGIC", "%", 0),
      reverb (s, "reverbMix", "REVERB", "%", 0),
      delay (s, "delayMix", "DELAY", "%", 0)
{
    for (auto* k : { &comp, &magic, &reverb, &delay })
        addAndMakeVisible (k);
}

void SimplePage::setMacro (int slot, const juce::String& title, const juce::String& param, const juce::String& suffix, int decimals)
{
    Knob* k[] = { &comp, &magic, &reverb, &delay };
    if (slot < 0 || slot >= 4) return;
    k[slot]->setTitle (title);
    k[slot]->setParameter (param);
    k[slot]->slider.setTextValueSuffix (suffix);
    k[slot]->slider.setNumDecimalPlacesToDisplay (decimals);
}

void SimplePage::setMacroSet (std::initializer_list<std::tuple<juce::String, juce::String, juce::String, int>> items)
{
    int i = 0;
    for (const auto& [title, param, suffix, decimals] : items)
        setMacro (i++, title, param, suffix, decimals);
}

void SimplePage::resized()
{
    auto r = getLocalBounds().reduced (6, 10);
    const int w = r.getWidth() / 4;
    for (auto* k : { &comp, &magic, &reverb, &delay })
        k->setBounds (r.removeFromLeft (w).reduced (14, 40));
}

void SimplePage::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().reduced (6, 10);
    const int w = r.getWidth() / 4;
    const char* names[] = { "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4" };
    for (int i = 0; i < 4; ++i)
        drawSection (g, r.removeFromLeft (w).reduced (8, 0), names[i]);
}

//==============================================================================
AdvancedPage::AdvancedPage (juce::AudioProcessorValueTreeState& s)
    : lowCut (s, "lowCut", "LOW CUT", " Hz", 0), eqLow (s, "eqLow", "LOW", " dB"), eqMid (s, "eqMid", "MID", " dB"), eqHigh (s, "eqHigh", "HIGH", " dB"),
      comp (s, "compAmount", "COMP", "", 1), deEss (s, "deEssAmount", "DE-ESS", "", 1), deEssFreq (s, "deEssFreq", "FOCUS", " Hz", 0),
      compMode (s, "compMode", "MODE"),
      micModel (s, "micModel", "MODEL"), micAmount (s, "micAmount", "AMOUNT", " %", 0), micProximity (s, "micProximity", "PROX", " %", 0),
      tuneKey (s, "tuneKey", "KEY"), tuneScale (s, "tuneScale", "SCALE"),
      tuneSpeed (s, "tuneSpeed", "SPEED", " ms", 0), tuneAmount (s, "tuneAmount", "AMOUNT", " %", 0), tuneTol (s, "tuneTol", "TOLERANCE", " ct", 0),
      satType (s, "satType", "TYPE"), satAmount (s, "satAmount", "INTENSIDAD", " %", 0), magic (s, "magic", "MAGIC", "", 0),
      airMid (s, "airMid", "MID AIR", " %", 0), airHigh (s, "airHigh", "HIGH AIR", " %", 0),
      reverbType (s, "reverbType", "TYPE"), reverbMix (s, "reverbMix", "MIX", " %", 0),
      reverbLow (s, "reverbLow", "LOW CUT", " Hz", 0), reverbHigh (s, "reverbHigh", "HIGH CUT", " Hz", 0),
      delayDiv (s, "delayDiv", "TIME"), delayMix (s, "delayMix", "MIX", " %", 0), delayFb (s, "delayFb", "FEEDBACK", " %", 0),
      delayLow (s, "delayLow", "LOW CUT", " Hz", 0), delayHigh (s, "delayHigh", "HIGH CUT", " Hz", 0)
{
    tuneOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, "tuneOn", tuneOn);

    noteTitle.setText ("NOTA DETECTADA", juce::dontSendNotification);
    noteTitle.setJustificationType (juce::Justification::centred);
    noteTitle.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    noteLabel.setText ("-", juce::dontSendNotification);
    noteLabel.setJustificationType (juce::Justification::centred);
    noteLabel.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    noteLabel.setColour (juce::Label::textColourId, ui::text);

    for (auto* c : std::initializer_list<juce::Component*> {
             &lowCut, &eqLow, &eqMid, &eqHigh, &comp, &deEss, &deEssFreq, &compMode, &grBar,
             &micModel, &micAmount, &micProximity,
             &tuneOn, &tuneKey, &tuneScale, &tuneSpeed, &tuneAmount, &tuneTol, &noteLabel, &noteTitle,
             &satType, &satAmount, &magic, &airMid, &airHigh, &reverbType, &reverbMix, &reverbLow, &reverbHigh,
             &delayDiv, &delayMix, &delayFb, &delayLow, &delayHigh })
        addAndMakeVisible (c);
}

void AdvancedPage::resized()
{
    auto r = getLocalBounds();
    const int gap = 10, rowH = (r.getHeight() - gap * 3) / 4;

    auto row1 = r.removeFromTop (rowH); r.removeFromTop (gap);
    auto row2 = r.removeFromTop (rowH); r.removeFromTop (gap);
    auto row3 = r.removeFromTop (rowH); r.removeFromTop (gap);
    auto row4 = r;

    auto split = [&] (juce::Rectangle<int> row, float frac)
    {
        auto a = row.removeFromLeft ((int) ((float) row.getWidth() * frac));
        row.removeFromLeft (gap);
        return std::pair<juce::Rectangle<int>, juce::Rectangle<int>> { a, row };
    };

    std::tie (secEq, secDyn)        = split (row1, 0.5f);
    secTune = row2;
    std::tie (secReverb, secDelay)  = split (row3, 0.4f);
    std::tie (secTone, secAir)      = split (row4, 0.5f);

    auto body = [] (juce::Rectangle<int> s) { return s.withTrimmedTop (26).reduced (8, 2); };

    layoutRow (body (secEq), { &lowCut, &eqLow, &eqMid, &eqHigh });

    {
        auto b = body (secDyn);
        auto bar = b.removeFromBottom (8);
        grBar.setBounds (bar.reduced (10, 0));
        layoutRow (b, { &comp, &compMode, &deEss, &deEssFreq });
    }

    {
        auto b = body (secTune);
        auto first = b.removeFromLeft (110);
        tuneOn.setBounds (first.removeFromTop (28));
        tuneKey.setBounds (first.removeFromTop (first.getHeight() / 2));
        tuneScale.setBounds (first);
        auto last = b.removeFromRight (100);
        noteTitle.setBounds (last.removeFromTop (20).withTrimmedTop (4));
        noteLabel.setBounds (last.removeFromTop (50));
        layoutRow (b, { &tuneSpeed, &tuneAmount, &tuneTol });
    }

    layoutRow (body (secTone), { &satType, &satAmount, &magic, &micModel, &micAmount, &micProximity });
    layoutRow (body (secAir), { &airMid, &airHigh });
    layoutRow (body (secReverb), { &reverbType, &reverbMix, &reverbLow, &reverbHigh });
    layoutRow (body (secDelay), { &delayDiv, &delayMix, &delayFb, &delayLow, &delayHigh });
}

void AdvancedPage::paint (juce::Graphics& g)
{
    drawSection (g, secEq, "EQ");
    drawSection (g, secDyn, "DYNAMICS");
    drawSection (g, secTune, "TUNE");
    drawSection (g, secTone, "TONE / MIC");
    drawSection (g, secAir, "AIR");
    drawSection (g, secReverb, "REVERB");
    drawSection (g, secDelay, "DELAY");
}

//==============================================================================
VoxoraEditor::VoxoraEditor (VoxoraProcessor& p)
    : AudioProcessorEditor (&p), proc (p), simplePage (p.apvts), advancedPage (p.apvts)
{
    setLookAndFeel (&laf);
    setSize (980, 760);

    for (auto* b : { &tabAssist, &tabSimple, &tabAdvanced })
    {
        b->setRadioGroupId (1);
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    tabAssist.onClick   = [this] { showPage (0); };
    tabSimple.onClick   = [this] { showPage (1); };
    tabAdvanced.onClick = [this] { showPage (2); };

    addAndMakeVisible (tuneQuick);
    tuneQuickAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "tuneOn", tuneQuick);

    categoryBox.addItem ("TODOS", 1);
    categoryBox.addItem ("GENERAL", 2);
    categoryBox.addItem ("CANTANTES", 3);
    categoryBox.setSelectedId (1, juce::dontSendNotification);
    categoryBox.onChange = [this] { rebuildPresetMenu(); };
    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId() - 1); };
    prevBtn.onClick = [this] {
        if (filteredPresets.empty()) return;
        auto it = std::find (filteredPresets.begin(), filteredPresets.end(), proc.getCurrentProgram());
        const int pos = it == filteredPresets.end() ? 0 : (int) std::distance (filteredPresets.begin(), it);
        selectPreset (filteredPresets[(pos + (int) filteredPresets.size() - 1) % (int) filteredPresets.size()]);
    };
    nextBtn.onClick = [this] {
        if (filteredPresets.empty()) return;
        auto it = std::find (filteredPresets.begin(), filteredPresets.end(), proc.getCurrentProgram());
        const int pos = it == filteredPresets.end() ? -1 : (int) std::distance (filteredPresets.begin(), it);
        selectPreset (filteredPresets[(pos + 1) % (int) filteredPresets.size()]);
    };
    addAndMakeVisible (categoryBox);
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    rebuildPresetMenu();

    for (auto* l : { &inLabel, &outLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (l);
    }

    for (auto* s : { &inSlider, &outSlider })
    {
        s->setSliderStyle (juce::Slider::LinearVertical);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        s->setTextValueSuffix (" dB");
        s->setNumDecimalPlacesToDisplay (1);
        s->setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (s);
    }
    inAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "inGain", inSlider);
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "outGain", outSlider);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);

    assistPage.startBtn.onClick = [this] { proc.startAssist(); };
    assistPage.originalBtn.onClick = [this] { if (assistPage.originalBtn.getToggleState()) proc.setAssistApplied (false); };
    assistPage.assistedBtn.onClick = [this] { if (assistPage.assistedBtn.getToggleState()) proc.setAssistApplied (true); };

    addChildComponent (assistPage);
    addChildComponent (simplePage);
    addChildComponent (advancedPage);

    tabAdvanced.setToggleState (true, juce::dontSendNotification);
    showPage (2);
    startTimerHz (30);
}

VoxoraEditor::~VoxoraEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxoraEditor::showPage (int index)
{
    assistPage.setVisible (index == 0);
    simplePage.setVisible (index == 1);
    advancedPage.setVisible (index == 2);
}

void VoxoraEditor::rebuildPresetMenu()
{
    filteredPresets.clear();
    presetBox.clear (juce::dontSendNotification);
    const int cat = categoryBox.getSelectedId();
    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        const auto c = proc.getProgramCategory (i);
        const bool matches = cat == 1 || (cat == 2 && c == "GENERAL") || (cat == 3 && c == "CANTANTES");
        if (! matches) continue;
        filteredPresets.push_back (i);
        presetBox.addItem (proc.getProgramName (i), i + 1);
    }
    const int current = proc.getCurrentProgram();
    const bool currentVisible = std::find (filteredPresets.begin(), filteredPresets.end(), current) != filteredPresets.end();
    if (! currentVisible && ! filteredPresets.empty())
        proc.setCurrentProgram (filteredPresets.front());
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    updateMacros();
}

void VoxoraEditor::selectPreset (int index)
{
    if (index < 0 || index >= proc.getNumPrograms()) return;
    proc.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
    updateMacros();
}

void VoxoraEditor::updateMacros()
{
    const int p = proc.getCurrentProgram();
    if (p < 6)
    {
        simplePage.setMacro (0, "COMP", "compAmount", "%", 0);
        simplePage.setMacro (1, "MAGIC", "magic", "%", 0);
        simplePage.setMacro (2, "REVERB", "reverbMix", "%", 0);
        simplePage.setMacro (3, "DELAY", "delayMix", "%", 0);
        return;
    }
    switch (p - 6)
    {
        case 0: simplePage.setMacroSet ({ { "TUNE SPEED", "tuneSpeed", " ms", 0 }, { "BODY", "eqLow", " dB", 1 }, { "SATURATION", "satAmount", " %", 0 }, { "SPACE", "reverbMix", " %", 0 } }); break;
        case 1: simplePage.setMacroSet ({ { "ROBOT SPEED", "tuneSpeed", " ms", 0 }, { "AIR", "airHigh", " %", 0 }, { "STEREO", "stereoWidth", " %", 0 }, { "DEPTH", "reverbMix", " %", 0 } }); break;
        case 2: simplePage.setMacroSet ({ { "TUNE", "tuneAmount", " %", 0 }, { "S / C / Z", "deEssAmount", " %", 0 }, { "PUNCH", "compAmount", " %", 0 }, { "SLAPBACK", "delayMix", " %", 0 } }); break;
        case 3: simplePage.setMacroSet ({ { "OCTAVES", "harmonyMix", " %", 0 }, { "WARMTH", "eqLow", " dB", 1 }, { "CHORUS", "modAmount", " %", 0 }, { "DREAM SPACE", "reverbMix", " %", 0 } }); break;
        case 4: simplePage.setMacroSet ({ { "QUICK TUNE", "tuneSpeed", " ms", 0 }, { "DIRT", "satAmount", " %", 0 }, { "BASS CONTROL", "boomAmount", " %", 0 }, { "MEGAPHONE", "megaphone", " %", 0 } }); break;
        case 5: simplePage.setMacroSet ({ { "PRO TUNING", "tuneAmount", " %", 0 }, { "COMMERCIAL COMP", "compAmount", " %", 0 }, { "HIT AIR", "airHigh", " %", 0 }, { "RHYTHM DELAY", "delayMix", " %", 0 } }); break;
        case 6: simplePage.setMacroSet ({ { "ROBOT HARD", "tuneSpeed", " ms", 0 }, { "BIT CRUSH", "bitCrush", " %", 0 }, { "MOD", "modAmount", " %", 0 }, { "GLITCH", "glitch", " %", 0 } }); break;
    }
}

void VoxoraEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.setColour (ui::text);
    g.drawText ("VOX", 22, 14, 90, 44, juce::Justification::centredLeft);
    g.setColour (ui::accent);
    g.drawText ("ORA", 98, 14, 90, 44, juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("VOCAL", 176, 28, 70, 20, juce::Justification::centredLeft);
}

void VoxoraEditor::resized()
{
    auto r = getLocalBounds().reduced (18);
    auto header = r.removeFromTop (48);

    tabAdvanced.setBounds (header.removeFromRight (100).reduced (2));
    tabSimple.setBounds (header.removeFromRight (100).reduced (2));
    tabAssist.setBounds (header.removeFromRight (100).reduced (2));
    header.removeFromRight (20);
    tuneQuick.setBounds (header.removeFromRight (90).reduced (2, 10));

    r.removeFromTop (8);
    auto presetRow = r.removeFromTop (34);
    auto categoryArea = presetRow.removeFromLeft (145);
    categoryBox.setBounds (categoryArea.reduced (2, 0));
    auto presetArea = presetRow.withSizeKeepingCentre (470, 34);
    prevBtn.setBounds (presetArea.removeFromLeft (40));
    nextBtn.setBounds (presetArea.removeFromRight (40));
    presetBox.setBounds (presetArea.reduced (6, 0));

    r.removeFromTop (12);
    auto left = r.removeFromLeft (78);
    auto right = r.removeFromRight (78);

    for (auto* side : { &left, &right })
    {
        const bool isIn = (side == &left);
        (isIn ? inLabel : outLabel).setBounds (side->removeFromTop (20));
        auto slider = side->removeFromBottom (170);
        (isIn ? inSlider : outSlider).setBounds (slider);
        (isIn ? inMeter : outMeter).setBounds (side->reduced (30, 8));
    }

    auto centre = r.reduced (12, 0);
    assistPage.setBounds (centre);
    simplePage.setBounds (centre);
    advancedPage.setBounds (centre);
}

void VoxoraEditor::timerCallback()
{
    // Medidores con caida suave
    inShown  = juce::jmax (proc.inPeak.exchange (0.0f),  inShown * 0.86f);
    outShown = juce::jmax (proc.outPeak.exchange (0.0f), outShown * 0.86f);
    inMeter.setLevel (inShown);
    outMeter.setLevel (outShown);
    advancedPage.setGr (proc.grMeter.exchange (0.0f));

    // Nota detectada
    const float midi = proc.detectedMidi.load();
    if (midi < 0.0f)
    {
        advancedPage.setNote ("-");
    }
    else
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int note = (int) std::lround (midi);
        const int cents = (int) std::lround ((midi - (float) note) * 100.0f);
        advancedPage.setNote (juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 1)
                              + (cents >= 0 ? " +" : " ") + juce::String (cents));
    }

    // Asistente
    if (proc.getAssistState() == VoxoraProcessor::assistComputed)
        proc.commitAssist();
    assistPage.refresh (proc.getAssistState(), proc.getAssistProgress());
}
