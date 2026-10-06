// Offline test: sings a small detuned melody into the DSP and checks the Ouai follows it.
#include "../Source/OuaitotuneDSP.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <random>

static bool readWav (const char* path, std::vector<float>& L, std::vector<float>& R, int& sr)
{
    FILE* f = fopen (path, "rb"); if (! f) return false;
    std::vector<uint8_t> b; uint8_t tmp[65536]; size_t n;
    while ((n = fread (tmp, 1, sizeof tmp, f)) > 0) b.insert (b.end(), tmp, tmp + n);
    fclose (f);
    size_t p = 12; int ch = 2, bits = 16;
    while (p + 8 <= b.size())
    {
        uint32_t sz; memcpy (&sz, &b[p + 4], 4);
        if (! memcmp (&b[p], "fmt ", 4)) { ch = b[p + 10]; memcpy (&sr, &b[p + 12], 4); bits = b[p + 22]; }
        if (! memcmp (&b[p], "data", 4))
        {
            size_t frames = sz / (size_t) (ch * bits / 8);
            for (size_t i = 0; i < frames; ++i)
            {
                int16_t a, c; memcpy (&a, &b[p + 8 + i * ch * 2], 2); memcpy (&c, &b[p + 8 + i * ch * 2 + (ch > 1 ? 2 : 0)], 2);
                L.push_back (a / 32768.0f); R.push_back (c / 32768.0f);
            }
            return true;
        }
        p += 8 + sz;
    }
    return false;
}

static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = fopen (path, "wb"); uint32_t n = (uint32_t) L.size(), data = n * 4, riff = 36 + data, fmt = 16, rate = (uint32_t) sr, br = rate * 4;
    uint16_t pcm = 1, ch = 2, ba = 4, bits = 16;
    fwrite ("RIFF", 1, 4, f); fwrite (&riff, 4, 1, f); fwrite ("WAVEfmt ", 1, 8, f); fwrite (&fmt, 4, 1, f);
    fwrite (&pcm, 2, 1, f); fwrite (&ch, 2, 1, f); fwrite (&rate, 4, 1, f); fwrite (&br, 4, 1, f); fwrite (&ba, 2, 1, f); fwrite (&bits, 2, 1, f);
    fwrite ("data", 1, 4, f); fwrite (&data, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::max (-32767.0f, std::min (32767.0f, L[i] * 32767.0f));
        int16_t b = (int16_t) std::max (-32767.0f, std::min (32767.0f, R[i] * 32767.0f));
        fwrite (&a, 2, 1, f); fwrite (&b, 2, 1, f);
    }
    fclose (f);
}

int main (int argc, char** argv)
{
    std::vector<float> sL, sR; int fsr = 0;
    if (! readWav (argc > 1 ? argv[1] : "../Assets/Ouai_Long_loop.wav", sL, sR, fsr)) { puts ("no wav"); return 1; }
    OuaitotuneDSP dsp; dsp.setSample (sL, sR, fsr, 2000.0, 12996.0);
    const int SR = 44100; dsp.prepare (SR);
    // melody: (start s, dur s, midi), sung ~30 cents off with vibrato
    struct N { double t, d; int m; } mel[] = { {0.2,0.45,64},{0.7,0.45,66},{1.2,0.45,68},{1.7,0.9,71},{2.7,0.45,69},{3.2,0.9,68},{4.4,1.2,61} };
    const int total = (int) (6.0 * SR);
    std::vector<float> L (total, 0.0f), R (total, 0.0f);
    std::mt19937 rng (3); std::uniform_real_distribution<float> u (-35.0f, 35.0f);
    double ph = 0.0;
    for (auto& nt : mel)
    {
        float cents = u (rng);
        int a = (int) (nt.t * SR), b = (int) ((nt.t + nt.d) * SR);
        for (int i = a; i < b; ++i)
        {
            double tt = (i - a) / (double) SR;
            double f = 440.0 * std::pow (2.0, (nt.m - 69 + (cents + 20 * std::sin (2 * M_PI * 5.3 * tt)) / 100.0) / 12.0);
            ph += 2 * M_PI * f / SR;
            double env = std::min (1.0, std::min (tt / 0.03, (nt.d - tt) / 0.07));
            float s = 0.0f; for (int h = 1; h < 12; ++h) s += (float) (std::sin (h * ph) / h * std::exp (-0.25 * h));
            L[i] = R[i] = 0.3f * s * (float) env;
        }
    }
    std::vector<float> inCopy = L;
    // process in blocks of 256 and log the target note in the middle of each sung note
    for (int i = 0; i < total; i += 256)
    {
        int n = std::min (256, total - i);
        dsp.process (&L[(size_t) i], &R[(size_t) i], n);
        for (auto& nt : mel)
            if (std::abs ((i / (double) SR) - (nt.t + nt.d * 0.6)) < 128.0 / SR)
                printf ("sung %d -> Ouai at %.2f, level %.2f\n", nt.m, dsp.noteOut.load(), dsp.level.load());
    }
    writeWav ("ouaitotune_test_in.wav", inCopy, inCopy, SR);
    writeWav ("ouaitotune_test_out.wav", L, R, SR);
    puts ("ok");
    return 0;
}
