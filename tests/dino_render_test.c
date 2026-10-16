/* SPDX-License-Identifier: GPL-3.0-only */
#include <math.h>
#include "dino_ui_host.h"

static uint64_t audio_hash = 1469598103934665603ull;
static uint32_t sounded;
static void render(uint32_t blocks)
{
    int32_t out[CTL * 2];
    for (uint32_t b = 0; b < blocks; b++) {
        fm6_poll(); mix_block(out, CTL); fm1_ms++;
        uint32_t busy = 0;
        for (uint32_t p = 0; p < NPART; p++) for (uint32_t v = 0; v < NVOICE; v++)
            busy += trk[p].v[v].active && trk[p].v[v].stage != 4u;
        assert(busy <= NVOICE);
        for (uint32_t i = 0; i < CTL * 2; i++) {
            assert(out[i] >= -32768 && out[i] <= 32767);
            sounded += out[i] != 0;
            audio_hash = (audio_hash ^ (uint32_t)out[i]) * 1099511628211ull;
        }
    }
}

static void all_sounds(uint32_t modulate)
{
    uint32_t patches = 0;
    for (uint32_t e = 0; e < NENGINES; e++) for (uint32_t pi = 0; pi < ENGINES[e]->npresets; pi++) {
        dino_test_init(); dino.mode = DINO_LAYER;
        set_engine_of(&trk[DINO_A], e); apply_preset_to(&trk[DINO_A], pi);
        uint32_t eb = (e + 1u) % NENGINES;
        set_engine_of(&trk[DINO_B], eb); apply_preset_to(&trk[DINO_B], pi % ENGINES[eb]->npresets);
        render(10);
        dino_test_midi(0x90, 48, 100); dino_test_trs(0x90, 64, 90);
        if (modulate) dino_test_midi(0xB0, 1, 127);
        render(80);
        if (modulate) dino_test_midi(0xE0, 0, 96);
        render(20);
        /* Change the routing while both sources own notes: their releases keep
         * their original destinations, and the render has no sequencer at all. */
        dino.mode = DINO_SPLIT; dino.split = 70; dino_focus_set(1);
        dino_test_midi(0x80, 48, 0); dino_test_trs(0x80, 64, 0);
        dino_test_midi(0xE0, 0, 64); dino_test_midi(0xB0, 1, 0);
        render(30);
        for (uint32_t p = 0; p < NPART; p++) {
            for (uint32_t n = 0; n < 128; n++) assert(!dino_refs[p][n]);
            for (uint32_t v = 0; v < NVOICE; v++) assert(!trk[p].v[v].gate);
        }
        assert(!song.playing && !song.rec && !song.seq_mode);
        patches++;
    }
    assert(sounded);
    printf("%u dry factory patches in A/B: %016llx\n", patches, (unsigned long long)audio_hash);
}

#ifndef DINO_DSP_REFERENCE
static void vibrato_physics(void)
{
    dino_test_init(); track_t *t = &trk[DINO_A];
    t->p[P_E0] = 3; t->p[P_E1] = t->p[P_E2] = t->p[P_E3] = 0;
    t->p[P_E4] = 127; t->p[P_E5] = t->p[P_E6] = t->p[P_E7] = 0;
    t->p[P_ED_PIT] = t->p[P_LD_PIT] = t->p[P_ED_FLT] = 0;
    trk_note_on(t, 69, 100);
    voice_t *v = 0;
    for (uint32_t i = 0; i < NVOICE; i++) if (t->v[i].active) v = &t->v[i];
    assert(v);
    dino.mod = 127; dino.mod_s = 127 * 256; dino.mod_phase = 0;
    double low = 1e9, high = -1e9, first = 0, last = 0, previous = -1;
    uint32_t crossings = 0; int32_t out[CTL];
    for (uint32_t b = 0; b < 2u * FS / CTL; b++) {
        dino_wheel_block(CTL);
        uint32_t phase = v->ph[0];
        track_render(t, out, CTL);
        uint32_t actual_inc = (v->ph[0] - phase) / CTL;
        double cents = 1200.0 * log2((double)actual_inc / pitch_inc(69u * 16u));
        if (cents < low) low = cents;
        if (cents > high) high = cents;
        if (previous <= 0 && cents > 0) {
            double at = (double)b * CTL / FS;
            if (!crossings) first = at;
            last = at; crossings++;
        }
        previous = cents;
    }
    double hz = (crossings - 1u) / (last - first);
    assert(hz > 4.99 && hz < 5.01 && low > -50.6 && low < -49.4 && high > 49.4 && high < 50.6);
    /* MOD returns to exactly dry; bend uses the existing quarter-step glide. */
    dino.mod = 0;
    for (uint32_t b = 0; b < 128; b++) dino_wheel_block(CTL);
    assert(!dino.mod_s && !dino.vibrato);
    t->bend = t->bend_s = 4096; track_render(t, out, CTL);
    t->bend = 0; int32_t before = t->bend_s; track_render(t, out, CTL);
    assert(t->bend_s > before * 7 / 10 && t->bend_s < before);
    for (uint32_t b = 0; b < 64; b++) track_render(t, out, CTL);
    assert(!t->bend_s);
    printf("Actual oscillator vibrato: %.4f Hz, %.3f..%.3f cents; MOD and bend return to zero\n", hz, low, high);
}
#endif

int main(int argc, char **argv)
{
    uint32_t zero = argc > 1 && !strcmp(argv[1], "zero");
    all_sounds(!zero);
#ifndef DINO_DSP_REFERENCE
    if (!zero) vibrato_physics();
#endif
    return 0;
}
