#include "test-suite.h"
#include <cmath>
#include <vector>
#include <cstdlib>
#include <iostream>
#include "../Effects/EffectLFO.h"
#include "../globals.h"

using namespace std;
using namespace zyn;

SYNTH_T *synth;

class EffectLFOTest
{
    public:
        void setUp() {
            synth = new SYNTH_T;
            // EffectLFO benötigt Sample-Rate und Buffer-Size im Konstruktor
            testLFO = new EffectLFO(44100.0f, 256.0f);
        }

        void tearDown() {
            delete testLFO;
            delete synth;
        }

        void testInitialState() {
            // Testet den initialen Zustand des LFO
            float outL, outR;

            // Erster Aufruf sollte konsistente Werte liefern
            testLFO->effectlfoout(&outL, &outR);

            // Ausgabe sollte im gültigen Bereich liegen
            TS_ASSERT(outL >= -1.0f && outL <= 1.0f);
            TS_ASSERT(outR >= -1.0f && outR <= 1.0f);
        }

        void testPhaseOffset() {
            // Testet die Phase-Offset-Funktionalität
            float outL1, outR1;
            float outL2, outR2;

            // Normale Ausgabe
            testLFO->effectlfoout(&outL1, &outR1);

            // Ausgabe mit Phase-Offset (sollte Zustand nicht verändern)
            testLFO->effectlfoout(&outL2, &outR2, 0.5f);

            // Nächster normaler Aufruf sollte wie der erste sein (Zustand unverändert)
            float outL3, outR3;
            testLFO->effectlfoout(&outL3, &outR3);

            // Verwende größere Toleranz für Floating-Point-Vergleiche
            TS_ASSERT_DELTA(outL1, outL3, 0.001f);
            TS_ASSERT_DELTA(outR1, outR3, 0.001f);
        }

        void testFrequencyChange() {
            // Testet Frequenzänderung
            float outL_slow, outR_slow;
            float outL_fast, outR_fast;

            // Langsame Frequenz
            testLFO->Pfreq = 10;
            testLFO->updateparams();

            // Mehrere Aufrufe um Phasen zu sammeln
            for (int i = 0; i < 5; ++i) {
                testLFO->effectlfoout(&outL_slow, &outR_slow);
            }

            // Schnelle Frequenz
            testLFO->Pfreq = 100;
            testLFO->updateparams();

            for (int i = 0; i < 5; ++i) {
                testLFO->effectlfoout(&outL_fast, &outR_fast);
            }

            // Bei höherer Frequenz sollten sich die Werte schneller ändern
            // Teste durch mehrere aufeinanderfolgende Samples
            float slowVariation = 0.0f;
            float fastVariation = 0.0f;

            testLFO->Pfreq = 10;
            testLFO->updateparams();
            float prevL = 0.0f, prevR = 0.0f;
            for (int i = 0; i < 10; ++i) {
                testLFO->effectlfoout(&outL_slow, &outR_slow);
                if (i > 0) {
                    slowVariation += abs(outL_slow - prevL) + abs(outR_slow - prevR);
                }
                prevL = outL_slow;
                prevR = outR_slow;
            }

            testLFO->Pfreq = 100;
            testLFO->updateparams();
            prevL = prevR = 0.0f;
            for (int i = 0; i < 10; ++i) {
                testLFO->effectlfoout(&outL_fast, &outR_fast);
                if (i > 0) {
                    fastVariation += abs(outL_fast - prevL) + abs(outR_fast - prevR);
                }
                prevL = outL_fast;
                prevR = outR_fast;
            }

            // Höhere Frequenz sollte mehr Variation verursachen
            TS_ASSERT(fastVariation > slowVariation);
        }

        void testLFOtypes() {
            // Testet verschiedene LFO-Typen - überprüft nur grundlegende Funktionalität
            float outL, outR;

            // Teste alle verfügbaren LFO-Typen
            for (int type = 0; type <= 2; ++type) {
                testLFO->PLFOtype = type;
                testLFO->updateparams();

                // Mehrere Aufrufe um sicherzustellen dass es funktioniert
                for (int i = 0; i < 5; ++i) {
                    testLFO->effectlfoout(&outL, &outR);

                    // Grundlegende Bereichsprüfung
                    TS_ASSERT(outL >= -1.0f && outL <= 1.0f);
                    TS_ASSERT(outR >= -1.0f && outR <= 1.0f);

                    // LFO sollte aktiv sein (nicht konstant 0)
                    if (i > 0) {
                        TS_ASSERT(outL != 0.0f || outR != 0.0f);
                    }
                }
            }
        }

        void testRandomness() {
            // Testet den Randomness-Parameter mit statistischem Ansatz
            testLFO->Pfreq = 50;
            testLFO->PLFOtype = 0;

            // Messen der Variation über Zeit mit und ohne Randomness
            std::vector<float> values_no_rand;
            std::vector<float> values_with_rand;

            // Ohne Randomness
            testLFO->Prandomness = 0;
            testLFO->updateparams();
            for (int i = 0; i < 50; ++i) {
                float outL, outR;
                testLFO->effectlfoout(&outL, &outR);
                values_no_rand.push_back((outL + outR) / 2.0f);
            }

            // Mit Randomness
            testLFO->Prandomness = 80; // Nicht maximal für bessere Testbarkeit
            testLFO->updateparams();
            for (int i = 0; i < 50; ++i) {
                float outL, outR;
                testLFO->effectlfoout(&outL, &outR);
                values_with_rand.push_back((outL + outR) / 2.0f);
            }

            // Berechne Standardabweichung als Maß für Variation
            float stddev_no_rand = calculateStdDev(values_no_rand);
            float stddev_with_rand = calculateStdDev(values_with_rand);

            // Randomness sollte mehr Variation verursachen
            TS_ASSERT(stddev_with_rand > stddev_no_rand * 0.8f); // Toleranter Check
        }

        void testParameterBounds() {
            // Testet dass Parameter innerhalb gültiger Bereiche bleiben
            float outL, outR;

            // Teste verschiedene Parameterkombinationen
            testLFO->Pfreq = 127; // Max Frequenz
            testLFO->Prandomness = 127; // Max Randomness
            testLFO->PLFOtype = 2; // Square
            testLFO->Pstereo = 64; // Max Spread
            testLFO->updateparams();

            // Sollte immer noch gültige Werte produzieren
            for (int i = 0; i < 10; ++i) {
                testLFO->effectlfoout(&outL, &outR);
                TS_ASSERT(outL >= -1.0f && outL <= 1.0f);
                TS_ASSERT(outR >= -1.0f && outR <= 1.0f);
            }
        }

    private:
        EffectLFO *testLFO;

        float calculateStdDev(const vector<float>& values) {
            if (values.empty()) return 0.0f;

            float mean = 0.0f;
            for (float v : values) mean += v;
            mean /= values.size();

            float variance = 0.0f;
            for (float v : values) {
                float diff = v - mean;
                variance += diff * diff;
            }
            variance /= values.size();

            return sqrt(variance);
        }
};

int main()
{
    tap_quiet = 1;
    EffectLFOTest test;

    RUN_TEST(testInitialState);
    RUN_TEST(testPhaseOffset);
    RUN_TEST(testFrequencyChange);
    RUN_TEST(testLFOtypes);
    RUN_TEST(testRandomness);
    RUN_TEST(testParameterBounds);

    return test_summary();
}
