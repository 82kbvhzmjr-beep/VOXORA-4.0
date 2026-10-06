#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

// Bloques de procesado de Voxora. Todo es codigo propio, sin dependencias externas salvo JUCE.
namespace vx
{
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept  { return 20.0f * std::log10 (std::max (g, 1.0e-6f)); }

//==============================================================================
// Biquad estereo que solo recalcula coeficientes cuando cambia un parametro
class StereoBiquad
{
public:
    enum class Type { highPass, lowPass, bandPass, lowShelf, highShelf, peak };

    void prepare (double sr)
    {
        sampleRate = sr;
        lastFreq = lastQ = lastGain = -12345.0f;
        for (auto& f : filt) f.reset();
    }

    void set (Type t, float freq, float q, float gainDb = 0.0f)
    {
        if (freq == lastFreq && q == lastQ && gainDb == lastGain)
            return;

        lastFreq = freq; lastQ = q; lastGain = gainDb;

        using C = juce::dsp::IIR::Coefficients<float>;
        const float f = juce::jlimit (10.0f, (float) (sampleRate * 0.45), freq);
        const float g = dbToGain (gainDb);
        C::Ptr c;

        switch (t)
        {
            case Type::highPass:  c = C::makeHighPass  (sampleRate, f, q); break;
            case Type::lowPass:   c = C::makeLowPass   (sampleRate, f, q); break;
            case Type::bandPass:  c = C::makeBandPass  (sampleRate, f, q); break;
            case Type::lowShelf:  c = C::makeLowShelf  (sampleRate, f, q, g); break;
            case Type::highShelf: c = C::makeHighShelf (sampleRate, f, q, g); break;
            case Type::peak:      c = C::makePeakFilter (sampleRate, f, q, g); break;
        }

        passThrough = (t == Type::lowShelf || t == Type::highShelf || t == Type::peak) && std::abs (gainDb) < 0.01f;

        for (auto& fl : filt)
            fl.coefficients = c;
    }

    float process (int ch, float x) noexcept
    {
        return passThrough ? x : filt[(size_t) ch].processSample (x);
    }

private:
    juce::dsp::IIR::Filter<float> filt[2];
    double sampleRate = 44100.0;
    float lastFreq = -1.0f, lastQ = -1.0f, lastGain = -1.0f;
    bool passThrough = false;
};

//==============================================================================
// Filtro de corte de graves y agudos para los retornos (reverb / delay)
struct BandFilter
{
    void prepare (double sr) { hp.prepare (sr); lp.prepare (sr); }

    void set (float lowCutHz, float highCutHz)
    {
        hp.set (StereoBiquad::Type::highPass, lowCutHz, 0.707f);
        lp.set (StereoBiquad::Type::lowPass, highCutHz, 0.707f);
        hpOn = lowCutHz > 21.0f;
        lpOn = highCutHz < 19500.0f;
    }

    float process (int ch, float x) noexcept
    {
        if (hpOn) x = hp.process (ch, x);
        if (lpOn) x = lp.process (ch, x);
        return x;
    }

    StereoBiquad hp, lp;
    bool hpOn = false, lpOn = false;
};

//==============================================================================
class Eq
{
public:
    void prepare (double sr) { for (auto& b : bq) b.prepare (sr); }

    void process (juce::AudioBuffer<float>& buf, float lowCutHz, float lowDb, float midDb, float highDb)
    {
        bq[0].set (StereoBiquad::Type::highPass,  lowCutHz, 0.707f);
        bq[1].set (StereoBiquad::Type::lowShelf,  120.0f,   0.7f, lowDb);
        bq[2].set (StereoBiquad::Type::peak,      2800.0f,  0.9f, midDb);
        bq[3].set (StereoBiquad::Type::highShelf, 9000.0f,  0.7f, highDb);
        const bool hpOn = lowCutHz > 21.0f;

        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            auto* p = buf.getWritePointer (ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                float x = p[i];
                if (hpOn) x = bq[0].process (ch, x);
                x = bq[1].process (ch, x);
                x = bq[2].process (ch, x);
                x = bq[3].process (ch, x);
                p[i] = x;
            }
        }
    }

private:
    StereoBiquad bq[4];
};

//==============================================================================
// Compresor con rodilla suave. "amount" (0-100) baja el umbral; el modo cambia ratio y tiempos.
class Compressor
{
public:
    void prepare (double sr) { sampleRate = (float) sr; gr = 0.0f; grOut.store (0.0f); }

    void process (juce::AudioBuffer<float>& buf, float amount, int mode)
    {
        if (amount < 0.5f) { gr = 0.0f; grOut.store (0.0f); return; }

        static const float ratios[3]   = { 2.5f, 4.0f, 8.0f };
        static const float attackMs[3] = { 15.0f, 30.0f, 3.0f };
        static const float releaseMs[3] = { 150.0f, 80.0f, 60.0f };
        mode = juce::jlimit (0, 2, mode);

        const float ratio  = ratios[mode];
        const float thresh = -4.0f - amount * 0.26f;
        const float slope  = 1.0f - 1.0f / ratio;
        const float aC = 1.0f - std::exp (-1.0f / (attackMs[mode]  * 0.001f * sampleRate));
        const float rC = 1.0f - std::exp (-1.0f / (releaseMs[mode] * 0.001f * sampleRate));
        const float makeup = -thresh * slope * 0.35f;
        const float knee = 6.0f;

        const int nch = buf.getNumChannels();
        float* ptr[2] = { buf.getWritePointer (0), nch > 1 ? buf.getWritePointer (1) : nullptr };
        float maxGr = 0.0f;

        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            float lvl = std::abs (ptr[0][i]);
            if (ptr[1] != nullptr) lvl = std::max (lvl, std::abs (ptr[1][i]));

            const float over = gainToDb (lvl) - thresh;
            float target;
            if (2.0f * over < -knee)              target = 0.0f;
            else if (2.0f * std::abs (over) <= knee) target = slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
            else                                  target = over * slope;

            gr += (target > gr ? aC : rC) * (target - gr);
            const float g = dbToGain (makeup - gr);
            ptr[0][i] *= g;
            if (ptr[1] != nullptr) ptr[1][i] *= g;
            maxGr = std::max (maxGr, gr);
        }

        grOut.store (maxGr);
    }

    std::atomic<float> grOut { 0.0f };

private:
    float sampleRate = 44100.0f, gr = 0.0f;
};

//==============================================================================
// De-esser: banda de sibilancia detectada y atenuada en paralelo
class DeEsser
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        band.prepare (sr);
        env = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buf, float amount, float freqHz)
    {
        if (amount < 0.5f) return;

        band.set (StereoBiquad::Type::bandPass, freqHz, 1.4f);
        const float thresh = -22.0f - amount * 0.12f;
        const float maxRed = 3.0f + amount * 0.12f;
        const float aC = 1.0f - std::exp (-1.0f / (0.0005f * sampleRate));
        const float rC = 1.0f - std::exp (-1.0f / (0.030f * sampleRate));

        const int nch = juce::jmin (2, buf.getNumChannels());
        float* ptr[2] = { buf.getWritePointer (0), nch > 1 ? buf.getWritePointer (1) : nullptr };

        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            float bp[2] = { 0.0f, 0.0f };
            float lvl = 0.0f;
            for (int ch = 0; ch < nch; ++ch)
            {
                bp[ch] = band.process (ch, ptr[ch][i]);
                lvl = std::max (lvl, std::abs (bp[ch]));
            }

            env += (lvl > env ? aC : rC) * (lvl - env);
            const float over = gainToDb (env) - thresh;
            const float red = juce::jlimit (0.0f, maxRed, over * 0.8f);
            const float k = 1.0f - dbToGain (-red);

            for (int ch = 0; ch < nch; ++ch)
                ptr[ch][i] -= bp[ch] * k;
        }
    }

private:
    StereoBiquad band;
    float sampleRate = 44100.0f, env = 0.0f;
};

//==============================================================================
// Saturacion: Warm (asimetrica), Agudo (enfasis de agudos antes de saturar), Saturacion (mas drive)
class Saturator
{
public:
    void prepare (double sr)
    {
        a = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 3000.0f / (float) sr);
        lp[0] = lp[1] = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buf, float amount, int type)
    {
        if (amount < 0.5f) return;

        const float mix = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        float drive = 1.0f + amount * 0.045f;
        float bias = 0.0f;
        if (type == 0) bias = 0.2f;
        if (type == 2) drive *= 1.5f;

        const float biasOut = std::tanh (bias);
        const float comp = 1.0f / (1.0f + 0.5f * (drive - 1.0f));

        for (int ch = 0; ch < juce::jmin (2, buf.getNumChannels()); ++ch)
        {
            auto* p = buf.getWritePointer (ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                const float dry = p[i];
                float x = dry;
                if (type == 1)
                {
                    lp[ch] += a * (x - lp[ch]);
                    x += 0.6f * (x - lp[ch]);
                }
                const float wet = (std::tanh (drive * x + bias) - biasOut) * comp * 1.2f;
                p[i] = dry + mix * (wet - dry);
            }
        }
    }

private:
    float a = 0.3f, lp[2] = { 0.0f, 0.0f };
};

//==============================================================================
// Aire: dos bandas (Mid ~5 kHz y High ~12 kHz) que generan armonicos nuevos a partir de la
// señal y los suman junto con un ligero realce de brillo. Diseño propio.
class AirExciter
{
public:
    void prepare (double sr)
    {
        for (int b = 0; b < 2; ++b)
        {
            src[b].prepare (sr);
            out[b].prepare (sr);
            shelf[b].prepare (sr);
        }
    }

    void process (juce::AudioBuffer<float>& buf, float mid, float high)
    {
        if (mid < 0.5f && high < 0.5f) return;

        static const float srcHz[2]   = { 2500.0f, 6000.0f };    // de donde se extraen armonicos
        static const float outHz[2]   = { 5000.0f, 12000.0f };   // a partir de donde se anaden
        const float amt[2] = { mid * 0.01f, high * 0.01f };

        for (int b = 0; b < 2; ++b)
        {
            src[b].set (StereoBiquad::Type::highPass, srcHz[b], 0.707f);
            out[b].set (StereoBiquad::Type::highPass, outHz[b], 0.707f);
            shelf[b].set (StereoBiquad::Type::highShelf, outHz[b], 0.7f, amt[b] * 4.0f);
        }

        for (int ch = 0; ch < juce::jmin (2, buf.getNumChannels()); ++ch)
        {
            auto* p = buf.getWritePointer (ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                const float dry = p[i];
                float add = 0.0f;

                for (int b = 0; b < 2; ++b)
                {
                    if (amt[b] < 0.005f) continue;
                    const float h = src[b].process (ch, dry);
                    const float t = std::tanh (4.0f * h);
                    const float residual = t + 0.35f * t * t - 4.0f * h;     // solo la parte no lineal (armonicos)
                    add += amt[b] * 0.8f * out[b].process (ch, residual);
                }

                float y = dry;
                y = shelf[0].process (ch, y);
                y = shelf[1].process (ch, y);
                p[i] = y + add;
            }
        }
    }

private:
    StereoBiquad src[2], out[2], shelf[2];
};

//==============================================================================
// Retorno de reverb (Plate / Room / Large) con cortes de graves y agudos
class ReverbSend
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        reverb.prepare ({ spec.sampleRate, spec.maximumBlockSize, 2 });
        reverb.reset();
        wet.setSize (2, (int) spec.maximumBlockSize);
        filt.prepare (spec.sampleRate);
    }

    void process (const juce::AudioBuffer<float>& dry, juce::AudioBuffer<float>& out,
                  float mix, int type, float lowCut, float highCut)
    {
        const int n = dry.getNumSamples();
        if (wet.getNumSamples() < n) wet.setSize (2, n, false, false, true);

        const int nIn = dry.getNumChannels();
        for (int ch = 0; ch < 2; ++ch)
            wet.copyFrom (ch, 0, dry, juce::jmin (ch, nIn - 1), 0, n);

        static const float room[3] = { 0.55f, 0.30f, 0.85f };
        static const float damp[3] = { 0.25f, 0.60f, 0.40f };
        type = juce::jlimit (0, 2, type);

        juce::dsp::Reverb::Parameters p;
        p.roomSize = room[type];
        p.damping = damp[type];
        p.wetLevel = 0.33f;
        p.dryLevel = 0.0f;
        p.width = 1.0f;
        p.freezeMode = 0.0f;
        reverb.setParameters (p);

        {
            juce::dsp::AudioBlock<float> block (wet.getArrayOfWritePointers(), 2, (size_t) n);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            reverb.process (ctx);
        }

        filt.set (lowCut, highCut);
        const float gain = juce::jlimit (0.0f, 1.0f, mix / 100.0f) * 0.7f;

        for (int ch = 0; ch < out.getNumChannels() && ch < 2; ++ch)
        {
            auto* w = wet.getWritePointer (ch);
            auto* o = out.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
                o[i] += gain * filt.process (ch, w[i]);
        }
    }

private:
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> wet;
    BandFilter filt;
};

//==============================================================================
// Delay sincronizado al tempo del host (1/16, 1/8, 1/4), ping-pong suave
class DelaySend
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        size = (int) (sr * 2.5) + 8;
        for (auto& l : line) l.assign ((size_t) size, 0.0f);
        wp = 0;
        currentDelay = 0.375f * sampleRate;
        filt.prepare (sr);
    }

    void process (const juce::AudioBuffer<float>& dry, juce::AudioBuffer<float>& out,
                  float mix, int div, float feedbackPct, float lowCut, float highCut, double bpm)
    {
        static const float beats[3] = { 0.25f, 0.5f, 1.0f };
        div = juce::jlimit (0, 2, div);
        bpm = juce::jlimit (30.0, 300.0, bpm);

        const float target = juce::jlimit (32.0f, (float) (size - 4), (float) (60.0 / bpm) * beats[div] * sampleRate);
        const float fb = juce::jlimit (0.0f, 0.9f, feedbackPct / 100.0f);
        const float gain = juce::jlimit (0.0f, 1.0f, mix / 100.0f) * 0.8f;
        filt.set (lowCut, highCut);

        const int nch = juce::jmin (2, dry.getNumChannels());
        const int n = dry.getNumSamples();

        for (int i = 0; i < n; ++i)
        {
            currentDelay += 0.0007f * (target - currentDelay);

            const float rp = (float) wp - currentDelay;
            const int i0 = (int) std::floor (rp);
            const float fr = rp - (float) i0;

            float d[2] = { 0.0f, 0.0f };
            for (int ch = 0; ch < nch; ++ch)
            {
                const auto& l = line[ch];
                const float a = l[(size_t) wrap (i0)];
                const float b = l[(size_t) wrap (i0 + 1)];
                d[ch] = filt.process (ch, a + fr * (b - a));
            }

            for (int ch = 0; ch < nch; ++ch)
            {
                const float cross = (nch > 1) ? d[1 - ch] : d[0];
                line[ch][(size_t) wp] = dry.getSample (ch, i) + fb * cross;
                out.getWritePointer (ch)[i] += gain * d[ch];
            }

            if (++wp >= size) wp = 0;
        }
    }

private:
    int wrap (int idx) const noexcept { idx %= size; return idx < 0 ? idx + size : idx; }

    std::vector<float> line[2];
    int size = 1, wp = 0;
    float sampleRate = 44100.0f, currentDelay = 0.0f;
    BandFilter filt;
};

//==============================================================================
// Detector de tono YIN (trabaja a media frecuencia de muestreo para ahorrar CPU)
class PitchDetector
{
public:
    float getConfidence() const noexcept { return confidence; }

    void prepare (double sr)
    {
        decRate = (float) sr * 0.5f;
        win = std::max (96, (int) (decRate * 0.040f));
        tauMax = std::min (900, (int) (decRate / 50.0f) + 1);
        tauMin = std::max (2, (int) (decRate / 1000.0f));
        hopSize = std::max (32, (int) (decRate * 0.0058f));

        ring.assign (8192, 0.0f);
        seg.assign ((size_t) (win + tauMax + 4), 0.0f);
        cmnd.assign ((size_t) (tauMax + 4), 0.0f);
        w = 0; hop = 0; haveHalf = false; confidence = lastConfidence = 0.0f;
    }

    // Devuelve true cuando hay un resultado nuevo. f0 = 0 si no hay tono claro.
    bool push (float x, float& f0)
    {
        if (! haveHalf) { half = x; haveHalf = true; return false; }

        ring[(size_t) (w & 8191)] = 0.5f * (half + x);
        haveHalf = false;
        ++w;

        if (++hop < hopSize) return false;
        hop = 0;
        f0 = analyse();
        confidence = lastConfidence;
        return true;
    }

private:
    float analyse()
    {
        const int total = win + tauMax;
        if (w < total) { lastConfidence = 0.0f; return 0.0f; }

        const int start = w - total;
        double energy = 0.0;
        for (int i = 0; i < total; ++i)
        {
            seg[(size_t) i] = ring[(size_t) ((start + i) & 8191)];
            if (i >= total - win) energy += (double) seg[(size_t) i] * seg[(size_t) i];
        }

        if (std::sqrt (energy / win) < 0.003) { lastConfidence = 0.0f; return 0.0f; }   // silencio (< -50 dB aprox.)

        cmnd[0] = 1.0f;
        float running = 0.0f;
        for (int tau = 1; tau < tauMax; ++tau)
        {
            float d = 0.0f;
            for (int j = 0; j < win; ++j)
            {
                const float diff = seg[(size_t) j] - seg[(size_t) (j + tau)];
                d += diff * diff;
            }
            running += d;
            cmnd[(size_t) tau] = running > 0.0f ? d * (float) tau / running : 1.0f;
        }

        int tau = -1;
        for (int t = tauMin; t < tauMax - 1; ++t)
        {
            if (cmnd[(size_t) t] < 0.15f)
            {
                while (t + 1 < tauMax - 1 && cmnd[(size_t) (t + 1)] < cmnd[(size_t) t]) ++t;
                tau = t;
                break;
            }
        }

        if (tau < 0)
        {
            int best = tauMin;
            for (int t = tauMin; t < tauMax - 1; ++t)
                if (cmnd[(size_t) t] < cmnd[(size_t) best]) best = t;
            if (cmnd[(size_t) best] > 0.30f) { lastConfidence = 0.0f; return 0.0f; }
            tau = best;
        }

        float tauF = (float) tau;
        if (tau > 1 && tau < tauMax - 2)
        {
            const float s0 = cmnd[(size_t) (tau - 1)], s1 = cmnd[(size_t) tau], s2 = cmnd[(size_t) (tau + 1)];
            const float den = s0 - 2.0f * s1 + s2;
            if (std::abs (den) > 1.0e-9f) tauF += 0.5f * (s0 - s2) / den;
        }

        lastConfidence = juce::jlimit (0.0f, 1.0f, 1.0f - cmnd[(size_t) tau] / 0.30f);
        return decRate / tauF;
    }

    float confidence = 0.0f, lastConfidence = 0.0f;
    std::vector<float> ring, seg, cmnd;
    float decRate = 22050.0f, half = 0.0f;
    int win = 512, tauMax = 300, tauMin = 22, hopSize = 128, hop = 0, w = 0;
    bool haveHalf = false;
};

//==============================================================================
// Cambiador de tono en el dominio del tiempo (dos cabezales con ventana seno^2).
// Siempre introduce la misma latencia (ventana / 2), este activo o no, para no desalinear la pista.
class PitchShifter
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        W = ((int) (sr * 0.030)) & ~1;
        size = 1;
        while (size < W * 2 + 16) size <<= 1;
        mask = size - 1;
        for (auto& l : line) l.assign ((size_t) size, 0.0f);
        wp = 0; phase = 0.25f; mix = 0.0f;
        mixCoeff = 1.0f - std::exp (-1.0f / (0.005f * sampleRate));
    }

    int getLatency() const noexcept { return W / 2; }

    void processFrame (float* x, int nch, float ratio, float mixTarget)
    {
        mix += mixCoeff * (mixTarget - mix);

        phase += (1.0f - ratio) / (float) W;
        phase -= std::floor (phase);
        float p2 = phase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;

        const float s1 = std::sin (juce::MathConstants<float>::pi * phase);
        const float s2 = std::sin (juce::MathConstants<float>::pi * p2);
        const float w1 = s1 * s1, w2 = s2 * s2;
        const float d1 = phase * (float) W, d2 = p2 * (float) W;

        for (int ch = 0; ch < nch; ++ch)
        {
            auto& l = line[ch];
            l[(size_t) wp] = x[ch];

            const float shifted = w1 * read (l, d1) + w2 * read (l, d2);
            const float dry = l[(size_t) ((wp - W / 2) & mask)];
            x[ch] = dry + mix * (shifted - dry);
        }

        wp = (wp + 1) & mask;
    }

private:
    float read (const std::vector<float>& l, float delay) const noexcept
    {
        const float rp = (float) wp - delay;
        const int i0 = (int) std::floor (rp);
        const float fr = rp - (float) i0;
        const float a = l[(size_t) (i0 & mask)];
        const float b = l[(size_t) ((i0 + 1) & mask)];
        return a + fr * (b - a);
    }

    std::vector<float> line[2];
    int W = 1024, size = 2048, mask = 2047, wp = 0;
    float phase = 0.25f, mix = 0.0f, mixCoeff = 0.01f, sampleRate = 44100.0f;
};


//==============================================================================
// Emulación de micrófono: coloreo tonal, proximidad y saturación muy sutil.
// No pretende clonar la respuesta exacta de un micrófono concreto; son perfiles vocales.
class MicEmulator
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        for (auto& b : bands) b.prepare (sr);
        hp.prepare (sr);
        for (auto& b : driveLP) b.prepare (sr);
        lowState[0] = lowState[1] = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buf, int model, float amount, float proximity)
    {
        if (amount < 0.5f) return;
        model = juce::jlimit (0, 5, model);
        const float a = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        const float prox = (juce::jlimit (0.0f, 100.0f, proximity) - 50.0f) * 0.055f;

        //                 HP, low@140, mid@3k, high@10k, drive
        static const float hpHz[6]   = { 60, 70, 90, 65, 75, 60 };
        static const float lowDb[6]  = { 0.0f, 1.8f, 2.2f, 2.2f, -1.0f, 2.8f };
        static const float midHz[6]  = { 2800, 4200, 2400, 3000, 5000, 1800 };
        static const float midDb[6]  = { 0.0f, 2.0f, -1.0f, 2.0f, 3.0f, 1.0f };
        static const float highHz[6] = { 10500, 12000, 8500, 11000, 12500, 9000 };
        static const float highDb[6] = { 1.0f, 3.0f, -2.0f, 2.0f, 4.0f, 2.0f };
        static const float drive[6]  = { 0.0f, 0.02f, 0.04f, 0.06f, 0.015f, 0.035f };

        hp.set (StereoBiquad::Type::highPass, hpHz[model], 0.707f);
        bands[0].set (StereoBiquad::Type::lowShelf, 140.0f, 0.70f, lowDb[model] + prox);
        bands[1].set (StereoBiquad::Type::peak, midHz[model], 0.85f, midDb[model]);
        bands[2].set (StereoBiquad::Type::highShelf, highHz[model], 0.70f, highDb[model]);

        for (int ch = 0; ch < juce::jmin (2, buf.getNumChannels()); ++ch)
        {
            auto* p = buf.getWritePointer (ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                float x = p[i];
                x = hp.process (ch, x);
                x = bands[0].process (ch, x);
                x = bands[1].process (ch, x);
                x = bands[2].process (ch, x);
                const float d = drive[model] * a;
                if (d > 0.0001f)
                    x = (1.0f - 0.25f * a) * x + 0.25f * a * std::tanh (x * (1.0f + 6.0f * d));
                p[i] = x;
            }
        }
    }

private:
    double sampleRate = 44100.0;
    StereoBiquad hp, bands[3], driveLP[2];
    float lowState[2] = { 0.0f, 0.0f };
};

//==============================================================================
// Control dinámico del exceso de proximidad en medios-bajos.
class DynamicBoom
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        band.prepare (sr);
        cut.prepare (sr);
        env = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buf, float amount)
    {
        if (amount < 0.5f) return;
        band.set (StereoBiquad::Type::bandPass, 250.0f, 0.75f);
        const float amt = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        const float attack = 1.0f - std::exp (-1.0f / (0.004f * sampleRate));
        const float release = 1.0f - std::exp (-1.0f / (0.090f * sampleRate));
        const float threshold = 0.025f;

        const int nch = juce::jmin (2, buf.getNumChannels());
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            float lowBand[2] = { 0.0f, 0.0f };
            float detector = 0.0f;
            for (int ch = 0; ch < nch; ++ch)
            {
                lowBand[ch] = band.process (ch, buf.getSample (ch, i));
                detector = std::max (detector, std::abs (lowBand[ch]));
            }
            env += (detector > env ? attack : release) * (detector - env);
            const float over = juce::jmax (0.0f, env - threshold) / 0.15f;
            const float reduction = juce::jlimit (0.0f, 0.65f, over * amt);
            for (int ch = 0; ch < nch; ++ch)
                buf.setSample (ch, i, buf.getSample (ch, i) - lowBand[ch] * reduction);
        }
    }

private:
    float sampleRate = 44100.0f, env = 0.0f;
    StereoBiquad band, cut;
};

//==============================================================================
// Ensanchamiento estéreo Mid/Side conservador.
class StereoWidener
{
public:
    void process (juce::AudioBuffer<float>& buf, float amount)
    {
        const float a = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        if (a <= 0.0001f || buf.getNumChannels() < 2) return;
        const float sideGain = 1.0f + 1.35f * a;
        auto* l = buf.getWritePointer (0);
        auto* r = buf.getWritePointer (1);
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            const float mid = 0.5f * (l[i] + r[i]);
            const float side = 0.5f * (l[i] - r[i]) * sideGain;
            l[i] = mid + side;
            r[i] = mid - side;
        }
    }
};

//==============================================================================
// Micro-modulación tipo chorus/micro-shift.
class MicroMod
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        size = 4096;
        for (auto& l : line) l.assign ((size_t) size, 0.0f);
        wp = 0;
        phase = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buf, float amount)
    {
        const float a = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        if (a <= 0.0001f) return;
        const float depth = 7.0f + 10.0f * a;
        const float rate = 0.18f + 0.55f * a;
        const int nch = juce::jmin (2, buf.getNumChannels());

        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            for (int ch = 0; ch < nch; ++ch)
            {
                auto& l = line[ch];
                const float x = buf.getSample (ch, i);
                l[(size_t) wp] = x;
                const float lfo = std::sin (phase + (ch == 1 ? 1.5707963f : 0.0f));
                const float d = 18.0f + depth * lfo;
                const float rp = (float) wp - d;
                const int i0 = (int) std::floor (rp);
                const float fr = rp - (float) i0;
                const float y = l[(size_t) wrap (i0)] + fr * (l[(size_t) wrap (i0 + 1)] - l[(size_t) wrap (i0)]);
                buf.setSample (ch, i, x + a * 0.22f * (y - x));
            }
            phase += juce::MathConstants<float>::twoPi * rate / sampleRate;
            if (phase >= juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
            if (++wp >= size) wp = 0;
        }
    }

private:
    int wrap (int i) const noexcept { i %= size; return i < 0 ? i + size : i; }
    float sampleRate = 44100.0f, phase = 0.0f;
    int size = 4096, wp = 0;
    std::vector<float> line[2];
};

//==============================================================================
// Bitcrusher suave a extremo, pensado para el preset experimental.
class BitCrusher
{
public:
    void prepare (double sr) { (void) sr; hold[0] = hold[1] = 0.0f; count = 0; }

    void process (juce::AudioBuffer<float>& buf, float amount)
    {
        const float a = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        if (a <= 0.0001f) return;
        const int step = juce::jmax (1, 1 + (int) std::round (a * 15.0f));
        const float bits = 16.0f - 12.0f * a;
        const float levels = std::pow (2.0f, bits);
        const float mix = 0.15f + 0.85f * a;
        const int nch = juce::jmin (2, buf.getNumChannels());
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            if (count-- <= 0)
            {
                count = step - 1;
                for (int ch = 0; ch < nch; ++ch)
                {
                    const float x = juce::jlimit (-1.0f, 1.0f, buf.getSample (ch, i));
                    hold[ch] = std::round (x * levels) / levels;
                }
            }
            for (int ch = 0; ch < nch; ++ch)
            {
                const float x = buf.getSample (ch, i);
                buf.setSample (ch, i, x + mix * (hold[ch] - x));
            }
        }
    }

private:
    float hold[2] = { 0.0f, 0.0f };
    int count = 0;
};

//==============================================================================
// Megáfono/telefono interno para transiciones.
class Megaphone
{
public:
    void prepare (double sr)
    {
        hp.prepare (sr);
        lp.prepare (sr);
    }

    void process (juce::AudioBuffer<float>& buf, float amount)
    {
        const float a = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        if (a <= 0.0001f) return;
        hp.set (StereoBiquad::Type::highPass, 320.0f + 300.0f * a, 0.707f);
        lp.set (StereoBiquad::Type::lowPass, 3800.0f - 900.0f * a, 0.707f);
        const float drive = 1.0f + 4.0f * a;
        const int nch = juce::jmin (2, buf.getNumChannels());
        for (int ch = 0; ch < nch; ++ch)
        {
            auto* p = buf.getWritePointer (ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                float x = hp.process (ch, p[i]);
                x = lp.process (ch, x);
                x = std::tanh (x * drive) / std::tanh (drive);
                p[i] = p[i] + a * (x - p[i]);
            }
        }
    }

private:
    StereoBiquad hp, lp;
};

//==============================================================================
// Stutter sincronizado al tempo: repite un micro-loop de forma periódica.
class StutterFx
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        size = 32768;
        for (auto& l : ring) l.assign ((size_t) size, 0.0f);
        wp = 0;
        clock = 0.0;
        remain = 0;
        loopPos = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buf, float amount, double bpm)
    {
        const float a = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
        if (a <= 0.0001f) return;
        bpm = juce::jlimit (40.0, 260.0, bpm);
        const float beat = (float) (60.0 / bpm) * sampleRate;
        const int loopLen = juce::jlimit (96, 8192, (int) (beat * (0.125f + 0.125f * a)));
        const float period = beat * (1.5f - 0.75f * a);
        const int nch = juce::jmin (2, buf.getNumChannels());

        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            if (clock >= period)
            {
                clock -= period;
                remain = loopLen;
                loopPos = (float) wrap (wp - loopLen);
            }

            for (int ch = 0; ch < nch; ++ch)
                ring[ch][(size_t) wp] = buf.getSample (ch, i);

            if (remain > 0)
            {
                for (int ch = 0; ch < nch; ++ch)
                {
                    auto& l = ring[ch];
                    const float y = l[(size_t) wrap ((int) loopPos)];
                    const float x = buf.getSample (ch, i);
                    buf.setSample (ch, i, x + a * (y - x));
                }
                loopPos += 1.0f;
                if (loopPos >= wp + 1.0f) loopPos = (float) wrap ((int) loopPos);
                --remain;
            }

            if (++wp >= size) wp = 0;
            clock += 1.0f;
        }
    }

private:
    int wrap (int i) const noexcept { i %= size; return i < 0 ? i + size : i; }
    float sampleRate = 44100.0f, clock = 0.0f, loopPos = 0.0f;
    int size = 32768, wp = 0, remain = 0;
    std::vector<float> ring[2];
};

} // namespace vx
