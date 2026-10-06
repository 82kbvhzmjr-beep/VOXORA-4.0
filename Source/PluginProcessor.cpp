#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace vx;

namespace
{
struct PresetDef
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

// Indices de opciones:
// compMode: 0 Smooth, 1 Punchy, 2 Aggressive | satType: 0 Warm, 1 Agudo, 2 Saturacion
// reverbType: 0 Plate, 1 Room, 2 Large       | delayDiv: 0 1/16, 1 1/8, 2 1/4
const std::vector<PresetDef>& presetList()
{
    static const std::vector<PresetDef> list
    {
        // GENERAL
        { "Mid Control",    { { "compAmount", 43 }, { "compMode", 0 }, { "deEssAmount", 35 }, { "satAmount", 27 }, { "satType", 2 },
                              { "magic", 27 }, { "airMid", 25 }, { "airHigh", 30 }, { "reverbMix", 30 }, { "reverbType", 1 }, { "delayMix", 20 }, { "delayDiv", 2 } } },
        { "Pop Brillante",  { { "compAmount", 55 }, { "compMode", 0 }, { "deEssAmount", 55 }, { "eqHigh", 3 }, { "lowCut", 90 },
                              { "satAmount", 20 }, { "satType", 1 }, { "magic", 50 }, { "airMid", 35 }, { "airHigh", 45 }, { "reverbMix", 30 }, { "reverbType", 0 },
                              { "delayMix", 15 }, { "delayDiv", 1 } } },
        { "Rap Calido",     { { "compAmount", 65 }, { "compMode", 1 }, { "deEssAmount", 40 }, { "lowCut", 90 }, { "eqLow", 2 },
                              { "satAmount", 35 }, { "satType", 0 }, { "reverbMix", 12 }, { "reverbType", 1 }, { "delayMix", 25 }, { "delayDiv", 0 } } },
        { "Intimo",         { { "compAmount", 30 }, { "compMode", 0 }, { "deEssAmount", 40 }, { "eqHigh", 1.5f }, { "lowCut", 80 },
                              { "satAmount", 10 }, { "satType", 0 }, { "airMid", 20 }, { "airHigh", 25 }, { "reverbMix", 15 }, { "reverbType", 1 } } },
        { "Directo Amplio", { { "compAmount", 50 }, { "compMode", 1 }, { "deEssAmount", 45 }, { "lowCut", 100 }, { "eqMid", 1.5f }, { "airMid", 25 }, { "airHigh", 30 },
                              { "reverbMix", 45 }, { "reverbType", 2 }, { "delayMix", 30 }, { "delayDiv", 2 }, { "stereoWidth", 35 } } },
        { "Autotune Trap",  { { "compAmount", 60 }, { "compMode", 2 }, { "deEssAmount", 50 }, { "lowCut", 90 }, { "satAmount", 25 },
                              { "satType", 2 }, { "reverbMix", 25 }, { "reverbType", 0 }, { "delayMix", 25 }, { "delayDiv", 1 },
                              { "tuneOn", 1 }, { "tuneSpeed", 0 }, { "tuneAmount", 100 } } },

        // CANTANTES
        { "Quevedo", { { "tuneOn", 1 }, { "tuneSpeed", 18 }, { "tuneAmount", 100 }, { "tuneTol", 8 },
                       { "lowCut", 80 }, { "eqLow", 1.2f }, { "eqMid", -2.3f }, { "eqHigh", 1.0f },
                       { "compAmount", 58 }, { "compMode", 0 }, { "deEssAmount", 42 }, { "deEssFreq", 7000 },
                       { "satAmount", 18 }, { "satType", 0 }, { "magic", 15 }, { "reverbMix", 18 }, { "reverbType", 0 }, { "reverbLow", 180 }, { "reverbHigh", 8500 },
                       { "delayMix", 8 }, { "delayDiv", 1 }, { "micModel", 5 }, { "micAmount", 45 }, { "micProximity", 55 } } },
        { "Feid / Ferxxo", { { "tuneOn", 1 }, { "tuneSpeed", 3 }, { "tuneAmount", 100 }, { "tuneTol", 0 },
                       { "lowCut", 90 }, { "eqHigh", 2.5f }, { "compAmount", 25 }, { "compMode", 0 }, { "deEssAmount", 30 }, { "deEssFreq", 7200 },
                       { "airMid", 30 }, { "airHigh", 55 }, { "stereoWidth", 55 }, { "modAmount", 25 }, { "reverbMix", 30 }, { "reverbType", 2 }, { "reverbHigh", 8000 },
                       { "delayMix", 25 }, { "delayDiv", 1 }, { "delayFb", 30 }, { "delayHigh", 6500 }, { "micModel", 4 }, { "micAmount", 50 }, { "micProximity", 45 } } },
        { "Lucho RK", { { "tuneOn", 1 }, { "tuneSpeed", 40 }, { "tuneAmount", 75 }, { "tuneTol", 3 },
                       { "lowCut", 85 }, { "eqMid", 1.5f }, { "eqHigh", 0.5f }, { "compAmount", 72 }, { "compMode", 2 },
                       { "deEssAmount", 65 }, { "deEssFreq", 7600 }, { "satAmount", 10 }, { "satType", 1 }, { "delayMix", 28 }, { "delayDiv", 1 }, { "delayFb", 8 },
                       { "micModel", 1 }, { "micAmount", 40 }, { "micProximity", 42 } } },
        { "Cruzzi", { { "tuneOn", 1 }, { "tuneSpeed", 22 }, { "tuneAmount", 90 }, { "tuneTol", 5 },
                       { "lowCut", 75 }, { "eqLow", 1.8f }, { "eqMid", 2.0f }, { "compAmount", 38 }, { "compMode", 0 }, { "deEssAmount", 32 },
                       { "satAmount", 9 }, { "satType", 0 }, { "magic", 20 }, { "harmonyMix", 22 }, { "modAmount", 32 }, { "stereoWidth", 35 },
                       { "reverbMix", 42 }, { "reverbType", 2 }, { "reverbHigh", 9000 }, { "delayMix", 34 }, { "delayDiv", 2 }, { "delayFb", 30 },
                       { "micModel", 3 }, { "micAmount", 55 }, { "micProximity", 50 } } },
        { "Roa", { { "tuneOn", 1 }, { "tuneSpeed", 10 }, { "tuneAmount", 100 }, { "tuneTol", 2 },
                       { "lowCut", 80 }, { "eqLow", -1.0f }, { "eqMid", 0.5f }, { "compAmount", 62 }, { "compMode", 1 }, { "deEssAmount", 45 },
                       { "satAmount", 38 }, { "satType", 2 }, { "boomAmount", 70 }, { "megaphone", 0 }, { "magic", 10 }, { "reverbMix", 12 }, { "delayMix", 15 }, { "delayDiv", 0 },
                       { "micModel", 2 }, { "micAmount", 52 }, { "micProximity", 60 } } },
        { "Omar Courtz", { { "tuneOn", 1 }, { "tuneSpeed", 9 }, { "tuneAmount", 100 }, { "tuneTol", 0 },
                       { "lowCut", 80 }, { "eqMid", 1.0f }, { "eqHigh", 2.0f }, { "compAmount", 48 }, { "compMode", 0 }, { "deEssAmount", 36 },
                       { "airHigh", 42 }, { "magic", 12 }, { "reverbMix", 20 }, { "reverbType", 0 }, { "delayMix", 24 }, { "delayDiv", 2 }, { "delayFb", 28 }, { "delayLow", 180 }, { "delayHigh", 7500 },
                       { "micModel", 1 }, { "micAmount", 55 }, { "micProximity", 48 } } },
        { "Xiyo & Fernandezz", { { "tuneOn", 1 }, { "tuneSpeed", 0 }, { "tuneAmount", 100 }, { "tuneTol", 0 },
                       { "lowCut", 90 }, { "eqHigh", 1.5f }, { "compAmount", 55 }, { "compMode", 2 }, { "deEssAmount", 25 },
                       { "satAmount", 30 }, { "satType", 2 }, { "bitCrush", 75 }, { "modAmount", 70 }, { "stereoWidth", 65 }, { "glitch", 55 },
                       { "reverbMix", 18 }, { "delayMix", 22 }, { "delayDiv", 0 }, { "delayFb", 45 }, { "micModel", 4 }, { "micAmount", 35 }, { "micProximity", 45 } } }
    };
    return list;
}

void setParam (juce::AudioProcessorValueTreeState& s, const juce::String& id, float realValue)
{
    if (auto* p = s.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
}

float getParam (juce::AudioProcessorValueTreeState& s, const juce::String& id)
{
    return s.getRawParameterValue (id)->load();
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout VoxoraProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&] (const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
                                                                 juce::NormalisableRange<float> (lo, hi, 0.01f, skew), def));
    };
    auto addC = [&] (const char* id, const char* name, juce::StringArray items, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
    };

    addF ("inGain",  "Input Gain",  -24.0f, 24.0f, 0.0f);
    addF ("outGain", "Output Gain", -24.0f, 24.0f, 0.0f);

    addF ("lowCut", "Low Cut",  20.0f, 400.0f, 80.0f, 0.5f);
    addF ("eqLow",  "EQ Low",  -12.0f, 12.0f, 0.0f);
    addF ("eqMid",  "EQ Mid",  -12.0f, 12.0f, 0.0f);
    addF ("eqHigh", "EQ High", -12.0f, 12.0f, 0.0f);

    addF ("compAmount", "Comp Amount", 0.0f, 100.0f, 40.0f);
    addC ("compMode", "Comp Mode", { "Smooth", "Punchy", "Aggressive" }, 0);
    addF ("deEssAmount", "De-ess Amount", 0.0f, 100.0f, 30.0f);
    addF ("deEssFreq", "De-ess Focus", 3000.0f, 10000.0f, 6500.0f, 0.6f);

    addF ("satAmount", "Saturation", 0.0f, 100.0f, 0.0f);
    addC ("satType", "Saturation Type", { "Warm", "Agudo", "Saturacion" }, 0);
    addF ("magic", "Magic", 0.0f, 100.0f, 0.0f);
    addF ("boomAmount", "Dynamic Anti-Boom", 0.0f, 100.0f, 0.0f);
    addF ("harmonyMix", "Harmony Octaves", 0.0f, 100.0f, 0.0f);
    addF ("stereoWidth", "Stereo Width", 0.0f, 100.0f, 0.0f);
    addF ("modAmount", "Micro Mod", 0.0f, 100.0f, 0.0f);
    addF ("bitCrush", "Bit Crush", 0.0f, 100.0f, 0.0f);
    addF ("megaphone", "Megaphone", 0.0f, 100.0f, 0.0f);
    addF ("glitch", "Glitch", 0.0f, 100.0f, 0.0f);
    addC ("micModel", "Mic Model", { "Mac Natural", "Studio Condenser", "Vintage Dynamic", "Tube Premium", "Bright Modern", "Warm Large Diaphragm" }, 0);
    addF ("micAmount", "Mic Amount", 0.0f, 100.0f, 0.0f);
    addF ("micProximity", "Mic Proximity", 0.0f, 100.0f, 50.0f);

    addF ("airMid",  "Air Mid",  0.0f, 100.0f, 0.0f);
    addF ("airHigh", "Air High", 0.0f, 100.0f, 0.0f);

    addF ("reverbMix", "Reverb Mix", 0.0f, 100.0f, 0.0f);
    addC ("reverbType", "Reverb Type", { "Plate", "Room", "Large" }, 1);
    addF ("reverbLow",  "Reverb Low Cut",  20.0f, 1000.0f, 200.0f, 0.5f);
    addF ("reverbHigh", "Reverb High Cut", 2000.0f, 20000.0f, 9000.0f, 0.5f);

    addF ("delayMix", "Delay Mix", 0.0f, 100.0f, 0.0f);
    addC ("delayDiv", "Delay Time", { "1/16", "1/8", "1/4" }, 2);
    addF ("delayFb", "Delay Feedback", 0.0f, 90.0f, 35.0f);
    addF ("delayLow",  "Delay Low Cut",  20.0f, 1000.0f, 200.0f, 0.5f);
    addF ("delayHigh", "Delay High Cut", 2000.0f, 20000.0f, 7000.0f, 0.5f);

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "tuneOn", 1 }, "Tune On", false));
    addC ("tuneKey", "Tune Key", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0);
    addC ("tuneScale", "Tune Scale", { "Chromatic", "Major", "Minor", "Pentatonic Minor" }, 0);
    addF ("tuneSpeed", "Tune Speed", 0.0f, 200.0f, 15.0f);
    addF ("tuneAmount", "Tune Amount", 0.0f, 100.0f, 100.0f);
    addF ("tuneTol", "Tune Tolerance", 0.0f, 50.0f, 0.0f);

    return layout;
}

//==============================================================================
VoxoraProcessor::VoxoraProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOXORA", createLayout())
{
}

bool VoxoraProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void VoxoraProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    mic.prepare (sampleRate);
    boom.prepare (sampleRate);
    eq.prepare (sampleRate);
    comp.prepare (sampleRate);
    deEss.prepare (sampleRate);
    sat.prepare (sampleRate);
    air.prepare (sampleRate);
    reverb.prepare (spec);
    delay.prepare (sampleRate);
    detector.prepare (sampleRate);
    shifter.prepare (sampleRate);
    harmonyUp.prepare (sampleRate);
    harmonyDown.prepare (sampleRate);
    mod.prepare (sampleRate);
    crusher.prepare (sampleRate);
    megaphone.prepare (sampleRate);
    glitch.prepare (sampleRate);
    assistHp.prepare (sampleRate);

    dryCopy.setSize (2, samplesPerBlock);
    prevIn = vx::dbToGain (getParam (apvts, "inGain"));
    prevOut = vx::dbToGain (getParam (apvts, "outGain"));
    correction = desired = 0.0f;
    voiced = false;

    setLatencySamples (shifter.getLatency());
}

//==============================================================================
void VoxoraProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buf.getNumSamples();
    const int nch = juce::jmin (2, buf.getNumChannels());
    for (int c = getTotalNumInputChannels(); c < buf.getNumChannels(); ++c)
        buf.clear (c, 0, n);
    if (n == 0 || nch == 0) return;

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    // ---- Entrada ----
    const float inG = vx::dbToGain (P ("inGain"));
    buf.applyGainRamp (0, n, prevIn, inG);
    prevIn = inG;
    inPeak.store (std::max (inPeak.load(), buf.getMagnitude (0, n)));

    if (assistState.load() == assistListening)
        analyseForAssist (buf);

    // ---- Modelo de micro ----
    mic.process (buf, (int) P ("micModel"), P ("micAmount"), P ("micProximity"));

    // ---- Correccion de tono (siempre en la ruta para mantener la latencia constante) ----
    {
        const bool tuneOn = P ("tuneOn") > 0.5f;
        const int key = (int) P ("tuneKey");
        const int scale = (int) P ("tuneScale");
        const float speed = P ("tuneSpeed");
        const float amount = P ("tuneAmount") * 0.01f;
        const float tolCents = P ("tuneTol");
        const float confidenceGate = 0.42f;

        static const bool masks[4][12] = {
            { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },   // Chromatic
            { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 },   // Major
            { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 },   // Minor
            { 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0 }    // Pentatonic Minor
        };
        const bool* mask = masks[juce::jlimit (0, 3, scale)];

        const float coeff = speed < 0.5f ? 1.0f : 1.0f - std::exp (-1.0f / (speed * 0.001f * (float) sr));
        const int holdSamples = (int) (0.08 * sr);

        float* ch0 = buf.getWritePointer (0);
        float* ch1 = nch > 1 ? buf.getWritePointer (1) : nullptr;

        for (int i = 0; i < n; ++i)
        {
            float frame[2] = { ch0[i], ch1 != nullptr ? ch1[i] : 0.0f };

            if (tuneOn)
            {
                const float mono = ch1 != nullptr ? 0.5f * (frame[0] + frame[1]) : frame[0];
                float f0 = 0.0f;
                if (detector.push (mono, f0))
                {
                    if (f0 > 0.0f && detector.getConfidence() >= confidenceGate)
                    {
                        voiced = true; unvoicedSamples = 0; lastF0 = f0;

                        const float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
                        const int centre = (int) std::lround (midi);
                        float best = (float) centre, bestDist = 1.0e9f;
                        for (int k = -6; k <= 6; ++k)
                        {
                            const int note = centre + k;
                            const int pc = (((note - key) % 12) + 12) % 12;
                            if (! mask[pc]) continue;
                            const float dist = std::abs ((float) note - midi);
                            if (dist < bestDist) { bestDist = dist; best = (float) note; }
                        }

                        float diff = best - midi;
                        if (std::abs (diff) * 100.0f < tolCents) diff = 0.0f;
                        desired = diff * amount;
                        detectedMidi.store (midi);
                    }
                    else
                    {
                        voiced = false;
                        detectedMidi.store (-1.0f);
                    }
                }

                if (! voiced)
                {
                    if (++unvoicedSamples > holdSamples) desired = 0.0f;
                }

                correction += coeff * (desired - correction);
            }
            else
            {
                correction = desired = 0.0f;
            }

            const float ratio = std::exp2 (correction / 12.0f);
            const float mixT = (tuneOn && std::abs (correction) > 0.01f) ? 1.0f : 0.0f;
            shifter.processFrame (frame, nch, ratio, mixT);

            ch0[i] = frame[0];
            if (ch1 != nullptr) ch1[i] = frame[1];
        }

        if (! tuneOn) detectedMidi.store (-1.0f);
    }

    // ---- Armonías, ancho y modulación ----
    {
        const float harmony = P ("harmonyMix");
        const float stereo = P ("stereoWidth");
        const float modulation = P ("modAmount");
        float* ch0 = buf.getWritePointer (0);
        float* ch1 = nch > 1 ? buf.getWritePointer (1) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            float src[2] = { ch0[i], ch1 != nullptr ? ch1[i] : 0.0f };
            float up[2] = { src[0], src[1] }, down[2] = { src[0], src[1] };
            const float mix = juce::jlimit (0.0f, 1.0f, harmony / 100.0f);
            harmonyUp.processFrame (up, nch, 2.0f, mix * 0.18f);
            harmonyDown.processFrame (down, nch, 0.5f, mix * 0.10f);
            ch0[i] = src[0] + mix * 0.18f * (up[0] - src[0]) + mix * 0.10f * (down[0] - src[0]);
            if (ch1 != nullptr)
                ch1[i] = src[1] + mix * 0.18f * (up[1] - src[1]) + mix * 0.10f * (down[1] - src[1]);
        }
        widener.process (buf, stereo);
        mod.process (buf, modulation);
    }

    // ---- Tono / dinamica ----
    const float magic = P ("magic");
    boom.process (buf, P ("boomAmount"));
    eq.process (buf, P ("lowCut"), P ("eqLow"), P ("eqMid"), P ("eqHigh") + magic * 0.045f);

    comp.process (buf, P ("compAmount"), (int) P ("compMode"));
    grMeter.store (std::max (grMeter.load(), comp.grOut.load()));

    deEss.process (buf, P ("deEssAmount"), P ("deEssFreq"));
    sat.process (buf, juce::jlimit (0.0f, 100.0f, P ("satAmount") + magic * 0.2f), (int) P ("satType"));
    crusher.process (buf, P ("bitCrush"));
    megaphone.process (buf, P ("megaphone"));
    air.process (buf, P ("airMid"), P ("airHigh"));

    // ---- Espacio (reverb y delay en paralelo) ----
    if (dryCopy.getNumSamples() < n) dryCopy.setSize (2, n, false, false, true);
    for (int ch = 0; ch < nch; ++ch)
        dryCopy.copyFrom (ch, 0, buf, ch, 0, n);

    juce::AudioBuffer<float> dryView (dryCopy.getArrayOfWritePointers(), nch, n);

    reverb.process (dryView, buf, P ("reverbMix"), (int) P ("reverbType"), P ("reverbLow"), P ("reverbHigh"));
    delay.process (dryView, buf, P ("delayMix"), (int) P ("delayDiv"), P ("delayFb"), P ("delayLow"), P ("delayHigh"), bpm);
    glitch.process (buf, P ("glitch"), bpm);

    // ---- Salida ----
    const float outG = vx::dbToGain (P ("outGain"));
    buf.applyGainRamp (0, n, prevOut, outG);
    prevOut = outG;
    outPeak.store (std::max (outPeak.load(), buf.getMagnitude (0, n)));
}

//==============================================================================
// Asistente vocal
void VoxoraProcessor::startAssist()
{
    assistProgress.store (0.0f);
    assistResetRequest.store (true);
    assistState.store (assistListening);
}

void VoxoraProcessor::analyseForAssist (const juce::AudioBuffer<float>& buf)
{
    if (assistResetRequest.exchange (false))
    {
        aSumSq = aSumHigh = 0.0; aPeak = 0.0f; aCount = 0;
    }

    assistHp.set (StereoBiquad::Type::highPass, 5000.0f, 0.707f);
    const int nch = juce::jmin (2, buf.getNumChannels());
    const long long target = (long long) (6.0 * sr);

    for (int i = 0; i < buf.getNumSamples(); ++i)
    {
        float m = 0.0f;
        for (int ch = 0; ch < nch; ++ch) m += buf.getSample (ch, i);
        m /= (float) nch;

        const float hp = assistHp.process (0, m);
        if (std::abs (m) > 0.003f)       // solo cuenta lo que suena (puerta ~ -50 dB)
        {
            aSumSq += (double) m * m;
            aSumHigh += (double) hp * hp;
            aPeak = std::max (aPeak, std::abs (m));
            ++aCount;
        }
    }

    assistProgress.store ((float) juce::jmin (1.0, (double) aCount / (double) target));

    if (aCount >= target)
    {
        const float rmsDb = (float) (10.0 * std::log10 (aSumSq / (double) aCount + 1.0e-12));
        const float peakDb = vx::gainToDb (aPeak);
        const float crest = peakDb - rmsDb;
        const float sib = (float) (aSumHigh / (aSumSq + 1.0e-12));

        assistResult[0].store (juce::jlimit (-12.0f, 12.0f, -18.0f - rmsDb));      // ajuste de ganancia
        assistResult[1].store (juce::jlimit (25.0f, 75.0f, 20.0f + (crest - 12.0f) * 5.0f));   // compresion
        assistResult[2].store (juce::jlimit (15.0f, 70.0f, sib * 350.0f));          // de-esser
        assistState.store (assistComputed);
    }
}

void VoxoraProcessor::commitAssist()
{
    if (assistState.load() != assistComputed) return;

    static const char* ids[] = { "inGain", "compAmount", "deEssAmount", "lowCut", "eqMid", "eqHigh" };

    assistBefore.clear(); assistAfter.clear();
    for (auto* id : ids) assistBefore.push_back ({ id, getParam (apvts, id) });

    const float newIn = juce::jlimit (-24.0f, 24.0f, getParam (apvts, "inGain") + assistResult[0].load());
    const float vals[] = { newIn, assistResult[1].load(), assistResult[2].load(), 80.0f, 1.5f, 1.5f };
    for (size_t i = 0; i < std::size (ids); ++i) assistAfter.push_back ({ ids[i], vals[i] });

    for (auto& [id, v] : assistAfter) setParam (apvts, id, v);
    assistState.store (assistApplied);
}

void VoxoraProcessor::setAssistApplied (bool applied)
{
    if (assistBefore.empty()) return;
    for (auto& [id, v] : (applied ? assistAfter : assistBefore)) setParam (apvts, id, v);
    assistState.store (applied ? assistApplied : assistRestored);
}

//==============================================================================
// Presets
int VoxoraProcessor::getNumPrograms() { return (int) presetList().size(); }

const juce::String VoxoraProcessor::getProgramName (int index)
{
    if (index < 0 || index >= getNumPrograms()) return {};
    return presetList()[(size_t) index].name;
}

const juce::String VoxoraProcessor::getProgramCategory (int index) const
{
    return (index >= 6 && index < (int) presetList().size()) ? juce::String ("CANTANTES") : juce::String ("GENERAL");
}

void VoxoraProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentPreset = index;
    applyPreset (index);
}

void VoxoraProcessor::applyPreset (int index)
{
    // Primero valores por defecto (menos ganancias y tune), luego los del preset
    for (auto* p : getParameters())
    {
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const auto id = r->paramID;
            const bool keepTune = id.startsWith ("tune") && id != "tuneOn" && id != "tuneSpeed" && id != "tuneAmount";
            if (id == "inGain" || id == "outGain" || keepTune)
                continue;
            r->setValueNotifyingHost (r->getDefaultValue());
        }
    }

    for (auto& [id, v] : presetList()[(size_t) index].values)
        setParam (apvts, id, v);
}

//==============================================================================
void VoxoraProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void VoxoraProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VoxoraProcessor::createEditor() { return new VoxoraEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxoraProcessor(); }
