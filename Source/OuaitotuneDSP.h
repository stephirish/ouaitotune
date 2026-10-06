// Ouaitotune DSP core - framework independent (used by the JUCE plugin and by the offline test).
// Tracks the pitch of the incoming voice (YIN on a 2x decimated signal) and replays the long
// "Ouaiiii" sample (a steady C#4) transposed to that pitch, gated by the input level.
#pragma once
#include <vector>
#include <cmath>
#include <atomic>
#include <algorithm>

class OuaitotuneDSP
{
public:
    // ---- parameters (set from the host thread, read in process) ----
    std::atomic<float> seuilDb { -40.0f }, retuneMs { 25.0f }, ouaiDb { 0.0f }, dryDb { -70.0f };
    std::atomic<int> octave { 0 };
    std::atomic<bool> snap { true }, relance { true };
    std::atomic<float> level { 0.0f };      // 0..1, for the mouth animation
    std::atomic<float> noteOut { 0.0f };    // last target note (for display)

    // stereo sample at fileRate, sustain loop [loopA, loopB) in samples
    void setSample (std::vector<float> left, std::vector<float> right, double fileRate, double loopAms, double loopBms)
    {
        sL = std::move (left); sR = std::move (right); fileSR = fileRate;
        loopA = loopAms * 0.001 * fileSR;
        loopB = std::min ((double) sL.size() - 2.0, loopBms * 0.001 * fileSR);
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        ring.assign (kRing, 0.0f); ybuf.assign (kRing, 0.0f); ydiff.assign (kRing + 2, 0.0f); ringPos = 0; hop = 0; decimHalf = 0.0f; decimFlag = false;
        env = gate = 0.0f; prevOpen = false; pos = 0.0; ratio = 1.0; lastNote = -1000; hold = 0;
        gO = gD = 0.0f; curMidi = 61.0f; haveNote = false;
    }

    void process (float* L, float* R, int n)
    {
        if (sL.empty()) return;
        const float thr = std::pow (10.0f, seuilDb.load() / 20.0f);
        const float aUp = 1.0f - std::exp (-1.0f / (0.001f * (float) sr));
        const float aDn = 1.0f - std::exp (-1.0f / (0.05f * (float) sr));
        const float gUp = 1.0f - std::exp (-1.0f / (0.01f * (float) sr));
        const float gDn = 1.0f - std::exp (-1.0f / (0.1f * (float) sr));
        const float tOuai = std::pow (10.0f, ouaiDb.load() / 20.0f);
        const float tDry = dryDb.load() <= -69.9f ? 0.0f : std::pow (10.0f, dryDb.load() / 20.0f);
        const float gSm = 1.0f - std::exp (-1.0f / (0.03f * (float) sr));
        const float rt = retuneMs.load();
        const double kRet = rt <= 0.0f ? 1.0 : 1.0 - std::exp (-1.0 / (rt * 0.001 * sr / 3.0));
        const double step = fileSR / sr;
        const int holdLen = (int) (0.12 * sr);
        const bool doSnap = snap.load(), doRel = relance.load();
        const int oct = octave.load();
        float lvlAcc = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float inL = L[i], inR = R != nullptr ? R[i] : L[i];
            const float x = 0.5f * (inL + inR);

            // ---- level / gate (same constants as the Max device) ----
            const float ax = std::abs (x);
            env += (ax - env) * (ax > env ? aUp : aDn);
            const bool open = env > thr;
            gate += ((open ? 1.0f : 0.0f) - gate) * (open ? gUp : gDn);
            if (open && ! prevOpen) { pos = 0.0; hold = holdLen; }
            prevOpen = open;
            if (hold > 0) --hold;

            // ---- pitch tracking: 2x decimation into a ring, YIN every kHop decimated samples ----
            if (decimFlag)
            {
                ring[(size_t) ringPos] = 0.5f * (decimHalf + x);
                ringPos = (ringPos + 1) % kRing;
                if (++hop >= kHop)
                {
                    hop = 0;
                    if (open)
                    {
                        const float f0 = yin (sr * 0.5);
                        if (f0 > 0.0f)
                        {
                            const float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
                            curMidi = midi; haveNote = true;
                        }
                    }
                }
            }
            else decimHalf = x;
            decimFlag = ! decimFlag;

            if (open && haveNote)
            {
                const float m = (doSnap ? std::round (curMidi) : curMidi) + 12.0f * (float) oct;
                const int note = (int) std::lround (m);
                if (doRel && lastNote != -1000 && note != lastNote && hold <= 0) { pos = 0.0; hold = holdLen; }
                lastNote = note;
                targetRatio = std::pow (2.0, (m - 61.0) / 12.0) * (440.0 * std::pow (2.0, (61.0 - 69.0) / 12.0)) / 277.18;
                noteOut.store (m);
            }
            ratio += (targetRatio - ratio) * kRet;

            // ---- the Ouai voice ----
            const int i0 = (int) pos;
            const float fr = (float) (pos - i0);
            const float sl = sL[(size_t) i0] + (sL[(size_t) i0 + 1] - sL[(size_t) i0]) * fr;
            const float sr2 = sR[(size_t) i0] + (sR[(size_t) i0 + 1] - sR[(size_t) i0]) * fr;
            pos += ratio * step;
            if (pos >= loopB) pos = loopA + std::fmod (pos - loopB, loopB - loopA);

            gO += (tOuai - gO) * gSm;
            gD += (tDry - gD) * gSm;
            const float g = gate * gO;
            L[i] = sl * g + inL * gD;
            if (R != nullptr) R[i] = sr2 * g + inR * gD;
            lvlAcc = std::max (lvlAcc, gate * std::min (1.0f, env * 4.0f));
        }
        level.store (lvlAcc);
    }

private:
    static constexpr int kRing = 1024;   // decimated samples kept for analysis
    static constexpr int kWin = 512;     // YIN integration window (decimated)
    static constexpr int kHop = 128;

    // YIN (de Cheveigne & Kawahara 2002) on the last kRing decimated samples
    float yin (double dsr)
    {
        const int tauMin = std::max (2, (int) (dsr / 1000.0));
        const int tauMax = std::min (kRing - kWin - 1, (int) (dsr / 60.0));
        auto& buf = ybuf; auto& d = ydiff;
        std::fill (d.begin(), d.end(), 0.0f);
        for (int k = 0; k < kRing; ++k) buf[(size_t) k] = ring[(size_t) ((ringPos + k) % kRing)];
        const int start = kRing - kWin - tauMax - 1;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float s = 0.0f;
            for (int j = 0; j < kWin; ++j)
            {
                const float dd = buf[(size_t) (start + j)] - buf[(size_t) (start + j + tau)];
                s += dd * dd;
            }
            d[(size_t) tau] = s;
        }
        float run = 0.0f;   // cumulative mean normalised difference
        d[0] = 1.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            run += d[(size_t) tau];
            d[(size_t) tau] = run > 0.0f ? d[(size_t) tau] * (float) tau / run : 1.0f;
        }
        int best = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (d[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < tauMax && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau;
                best = tau; break;
            }
        }
        if (best < 0) return 0.0f;
        // parabolic interpolation
        const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
        const float den = a - 2.0f * b + c;
        const float t = best + (std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.0f);
        return (float) (dsr / t);
    }

    std::vector<float> sL, sR, ring, ybuf, ydiff;
    double fileSR = 48000.0, loopA = 0.0, loopB = 1.0, sr = 44100.0;
    int ringPos = 0, hop = 0, lastNote = -1000, hold = 0;
    float decimHalf = 0.0f; bool decimFlag = false;
    float env = 0.0f, gate = 0.0f, gO = 0.0f, gD = 0.0f, curMidi = 61.0f;
    bool prevOpen = false, haveNote = false;
    double pos = 0.0, ratio = 1.0, targetRatio = 1.0;
};
