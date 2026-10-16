/* SPDX-License-Identifier: GPL-3.0-only */
/* Real DINO save logic, USER record loading, and A/B flash on a simulated NOR. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define FELUCCA_DINO 1
#define FELUCCA_FLASH 1
#define __attribute__(x)
#define memset felucca_memset
#define memcpy felucca_memcpy
#define memcmp felucca_memcmp
#include "../firmware/src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
#include "../firmware/src/core.h"
#include "../firmware/src/dino.h"
static int32_t clamp(int32_t n, int32_t lo, int32_t hi) { return n < lo ? lo : n > hi ? hi : n; }
static uint8_t nor[0x100000];
static int fail_program;
static uint32_t erases;
static int st_read(uint32_t off, void *p, uint32_t n) { memcpy(p, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 255, 4096); erases++; return 0; }
static int st_prog(uint32_t off, const void *p, uint32_t n)
{
    uint32_t i; const uint8_t *s = p;
    if (fail_program) return -1;
    for (i = 0; i < n; i++) nor[off + i] &= s[i];
    return 0;
}
#define ST_XIP_PTR(off) (nor + (off))
#include "../firmware/src/storage.c"
static uint8_t flash_ok = 1, sec_dirty, song_dirty, vis_style;
static uint8_t fx_lowcut, settings_later;
static struct { uint32_t lowcut; } settings;
static uint8_t midi_set[10], lights_sync, lights_mout, lights_min, fav_dirty;
static uint8_t settings_copy[14];
static void settings_save(void)
{
    uint8_t b[14];
    memcpy(b, midi_set, 10); b[10] = lights_sync; b[11] = lights_mout; b[12] = lights_min; b[13] = (uint8_t)settings.lowcut;
    if (!st_save(OBJ_SETTINGS, b, sizeof b)) { memcpy(settings_copy, b, sizeof b); fav_dirty = 0; }
    else settings_later = 1;
}
static uint32_t ui_input_ms, up_gen, sync_reload;
static uint8_t panic_req, arp_keep_req, transport_req;
static struct { uint8_t force, menu; } ui;
static void ui_message(const char *s) { (void)s; }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
static uint32_t test_irq = 1;
static uint32_t fm1_icfg(void) { return test_irq; }
static void fm1_irq_off(void) { test_irq = 0; }
static void fm1_irq_on(void) { test_irq = 1; }
static void fm1_irq_restore(uint32_t irq) { test_irq = irq; }
static param_desc_t TP[P_COUNT], GP[G_COUNT], DRUM_KIT_DESC;
static engine_t engine;
static const engine_t *ENGINES[NENGINES];
static const preset_t preset[] = {{.name = "TEST PATCH"}};
static int param_kept(uint32_t i) { return i == P_LEVEL || i == P_PAN || i == P_MUTE; }
static void sound_change(track_t *t) { (void)t; }
static uint32_t trk_index(const track_t *t) { return (uint32_t)(t - trk); }
#define FM6_PACKED 128u
#define FM6_BANK_N 27u
#define FM6_NFACTORY 8u
#define FP_SIZE 154u
static uint8_t fm6_patch[NPART][FP_SIZE + 1u], fm6_slot[NPART], fm6_pend;
static int (*fm6_bank_read)(uint32_t, uint8_t *);
static void fm6_pack(const uint8_t *v, uint8_t *pk) { memcpy(pk, v, 128); }
static void fm6_unpack(const uint8_t *pk, uint8_t *v) { memset(v, 0, FP_SIZE + 1u); memcpy(v, pk, 128); }
static void fm6_set_patch(uint32_t part, const uint8_t *v) { memcpy(fm6_patch[part], v, FP_SIZE + 1u); }
static void fm6_track_loaded(const track_t *t) { (void)t; }
static void fm6_slot_get(uint32_t slot, uint8_t *pk) { (void)slot; memset(pk, 0, 128); }
static union { uint32_t align; uint8_t raw[3840]; } proj_tmp, autosave_buf;
static void *up_stage(void) { return &proj_tmp; }
#include "../firmware/src/upreset.c"
#define FM6_BANK_XIP(copy) (nor + st_sector(OBJ_FM6BANK, copy) + ST_PAYLOAD_OFF)
#include "../firmware/src/fm6_bank.c"
typedef struct { uint32_t magic; uint8_t payload[3812]; } project_t;
static project_t proj_slot[4];
static int proj_ok(const project_t *p) { return p->magic == 0x46554E36u; }
static int8_t drum_tune[16], drum_dec[16];
static void steps_clear(track_t *t) { memset(t->step, 0, sizeof t->step); }
static int audio_quiet(void)
{
    for (uint32_t p = 0; p < NPART; p++)
        for (uint32_t v = 0; v < NVOICE; v++) if (trk[p].v[v].active) return 0;
    return 1;
}
static uint32_t delay_samples(void) { return 22050; }
static void dino_controllers_reset(void) { dino.mod = 0; dino.bend = 0; }
static void dino_mode_set(uint32_t m) { dino.mode = (uint8_t)m; }
static void dino_focus_set(uint32_t f) { dino.focus = (uint8_t)f; song.sel = f ? DINO_B : DINO_A; }
#define AUTOSAVE_IDLE 2500u
#define AUTOSAVE_GAP 20000u
#include "../firmware/src/dino_store.c"
static uint8_t wire[128];
static uint32_t wire_n;
static void ed_b(uint32_t b) { assert(wire_n < sizeof wire); wire[wire_n++] = (uint8_t)(b & 127u); }
static void ed_str(const char *s, uint32_t max) { while (*s && max--) ed_b((uint8_t)*s++); ed_b(0); }
#include "../firmware/src/editor_dino.c"
static void dino_wire(const uint8_t *a, uint32_t n) { wire_n = 0; ed_dino(a, n); }

static void reset(void)
{
    uint32_t i;
    memset(nor, 255, sizeof nor); memset(trk, 0, sizeof trk); memset(proj_slot, 0, sizeof proj_slot);
    memset(&dino, 0, sizeof dino); dino.split = 60; fm1_ms = 10000; dino_signal_ms = 0;
    for (i = 0; i < P_COUNT; i++) { TP[i].min = -8192; TP[i].max = 8191; }
    for (i = 0; i < G_COUNT; i++) { GP[i].min = -8192; GP[i].max = 8191; }
    DRUM_KIT_DESC = TP[0];
    for (i = 0; i < 8; i++) engine.edit[i] = TP[0];
    engine.name = "TEST"; engine.presets = preset; engine.npresets = 1;
    for (i = 0; i < NENGINES; i++) ENGINES[i] = &engine;
    dino_save_cancel(); up_boot(); fm6_bank_boot(); erases = 0; fail_program = 0;
}
int main(void)
{
    dino_performance_t copy;
    uint8_t pk[128]; uint32_t before;
    reset();
    trk[0].p[P_LEVEL] = 89; trk[0].p[P_PAN] = -31; trk[0].p[P_TFLT] = -17;
    trk[0].p[P_REV] = 37; trk[0].p[P_IMIX] = 52; trk[0].p[P_AMODE] = 3;
    trk[0].v[0].active = 1;
    assert(!dino_save_request(DINO_SAVE_PATCH, 0));
    dino_save_poll(); assert(dino_save_status() == DINO_SAVE_PENDING && !erases);
    trk[0].v[0].active = 0; dino_signal_ms = fm1_ms;
    dino_save_poll(); assert(!erases);
    fm1_ms += 999; dino_save_poll(); assert(!erases);
    fm1_ms++; dino_save_poll(); assert(dino_save_status() == DINO_SAVE_SAVED);
    memset(trk[0].p, 0, sizeof trk[0].p); assert(!up_load(0));
    assert(trk[0].p[P_LEVEL] == 89 && trk[0].p[P_PAN] == -31 && trk[0].p[P_TFLT] == -17);
    assert(trk[0].p[P_REV] == 37 && trk[0].p[P_IMIX] == 52 && !trk[0].p[P_AMODE]);
    puts("USER full audible state, retired arp, voice/tail deferral: ok");

    assert(!dino_save_request(DINO_SAVE_PATCH, 1)); before = erases; dino_save_cancel();
    dino_save_poll(); assert(erases == before && !up_used(1));
    assert(!dino_save_request(DINO_SAVE_PATCH, 1)); fail_program = 1; dino_save_poll();
    assert(dino_save_status() == DINO_SAVE_ERROR && !up_used(1)); fail_program = 0;
    assert(up_used(0)); puts("Cancel and failed commit preserve previous USER bank: ok");

    trk[0].eng_req = ENGI_FM6; memset(fm6_patch[0], 11, 128); memset(fm6_patch[2], 23, 128);
    fm6_slot[0] = (uint8_t)trk[0].p[P_E7];
    fm6_slot[2] = FM6_NFACTORY + 3;
    assert(!dino_save_request(DINO_SAVE_PATCH, 2)); dino_save_poll();
    assert(dino_save_status() == DINO_SAVE_SAVED && up_rec(2)->p[P_E7] == FM6_NFACTORY);
    assert(fm6_slot[2] == FM6_NFACTORY + 3 && fm6_patch[2][0] == 23);
    before = erases; assert(!dino_save_request(DINO_SAVE_PATCH, 3)); dino_save_poll();
    assert(erases == before + 1 && fm6_bank_used == 1u);
    memset(pk, 51, sizeof pk);
    for (uint32_t i = 1; i < FM6_BANK_N; i++) assert(!fm6_bank_put(i, pk));
    memset(fm6_patch[0], 99, 128); before = erases;
    assert(!dino_save_request(DINO_SAVE_PATCH, 4)); dino_save_poll();
    assert(dino_save_status() == DINO_SAVE_BANK_FULL && !up_used(4) && before == erases);
    fm1_ms += 30000; memset(fm6_patch[0], 5, 128); dino_save_poll();
    assert(dino_save_status() == DINO_SAVE_BANK_FULL && dino_snapshot->sound[0].fm6[0] == 99);
    assert(!dino_save_replace_bank(5)); dino_save_poll();
    assert(dino_save_status() == DINO_SAVE_SAVED && up_rec(4)->p[P_E7] == FM6_NFACTORY + 5);
    assert(!fm6_bank_get(5, pk) && pk[0] == 99 && fm6_patch[2][0] == 23);
    memset(fm6_patch[0], 99, 128);
    puts("FM6 exact reuse, allocation, RAM voice protection, full-bank refusal and explicit replacement: ok");

    dino.mode = DINO_SPLIT; dino.focus = 1; dino.split = 72; song.octave = -2; vis_style = 14;
    trk[2].eng_req = ENGI_FM6; trk[2].p[P_E7] = fm6_slot[2]; trk[2].p[P_LEVEL] = 71; song.g[G_RSIZE] = 43;
    drum_tune[5] = -5; drum_dec[5] = 2; dino.mod = 87; dino.bend = 3000;
    proj_slot[1].magic = 0x46554E36u; assert(!strcmp(dino_performance_label(1), "LEGACY SONG"));
    assert(dino_performance_load(1) == 2);
    assert(!dino_save_request(DINO_SAVE_PERFORMANCE, 1)); dino_save_poll();
    assert(dino_save_status() == DINO_SAVE_SAVED);
    copy = *(dino_performance_t *)&proj_slot[1];
    assert(dino_store_valid(&copy, sizeof copy, DINO_PERF_MAGIC));
    copy.sound[0].p[P_LEVEL]++; assert(!dino_store_valid(&copy, sizeof copy, DINO_PERF_MAGIC));
    copy = *(dino_performance_t *)&proj_slot[1]; copy.version++; dino_store_seal(&copy);
    assert(!dino_store_valid(&copy, sizeof copy, DINO_PERF_MAGIC));
    dino_store_boot(); dino.mode = DINO_SINGLE; memset(fm6_patch, 0, sizeof fm6_patch);
    test_irq = 0; panic_req = 0; assert(!dino_performance_load(1)); assert(!test_irq); test_irq = 1;
    assert(panic_req == (1u << NTRK) - 1u);
    assert(dino.mode == DINO_SPLIT && dino.focus == 1 && dino.split == 72 && song.octave == -2 && vis_style == 14);
    assert(trk[2].p[P_LEVEL] == 71 && song.g[G_RSIZE] == 43 && drum_tune[5] == -5 && drum_dec[5] == 2);
    assert(fm6_patch[0][0] == 99 && fm6_patch[2][0] == 23 && !dino.mod && !dino.bend);
    puts("PERFORMANCE round trip/reboot, embedded A/B FM6, legacy protection, corruption/version rejection: ok");

    copy = *(dino_performance_t *)&proj_slot[1]; copy.magic = DINO_AUTO_MAGIC; dino_store_seal(&copy);
    assert(!st_save(OBJ_AUTOSAVE, &copy, sizeof copy));
    dino.mode = DINO_SINGLE; dino_store_boot(); assert(dino.mode == DINO_SPLIT);
    memset(&copy, 0, sizeof copy); copy.magic = 0x46554E36u;
    assert(!st_save(OBJ_AUTOSAVE, &copy, sizeof copy));
    dino.mode = DINO_SINGLE; dino_store_boot(); assert(dino.mode == DINO_SINGLE && vis_style == 12u);
    puts("DINO autosave resumes; legacy autosave never enables old state: ok");

    dino_save_cancel(); settings_later = 1; trk[0].v[0].active = 1;
    song.g[G_CH1] = 7; song.g[G_BEND] = 12; song.g[G_ROUTE] = 3; song.g[G_MIDI] = 1;
    before = erases; fm1_ms += 10000; dino_save_poll();
    assert(erases == before && settings_later && midi_set[0] == 7 && midi_set[5] == 12);
    trk[0].v[0].active = 0; dino_signal_ms = fm1_ms; dino_save_poll(); assert(settings_later);
    fm1_ms += 1001; dino_save_poll();
    assert(!settings_later && settings_copy[0] == 7 && settings_copy[5] == 12 && settings_copy[12] == 3);
    memset(midi_set, 0, sizeof midi_set); memset(settings_copy, 0, sizeof settings_copy);
    assert(st_load(OBJ_SETTINGS, settings_copy, sizeof settings_copy) == sizeof settings_copy);
    memcpy(midi_set, settings_copy, 10); assert(midi_set[0] == 7 && midi_set[5] == 12);
    puts("MIDI RX channel/BEND and deferred menu settings persist after silence and reboot: ok");
    {
        uint8_t set[] = {1, DINO_LAYER, 65, 1, 13, 5}, bad[] = {1, 9, 60, 0, 0};
        uint8_t save[] = {3, DINO_SAVE_PERFORMANCE, 2}, cancel[] = {4};
        uint8_t rename[] = {5, 1, 'N', 'E', 'W', 0};
        dino_wire(set, sizeof set);
        assert(wire[0] == 1 && !wire[1] && wire[2] == DINO_LAYER && wire[3] == 65 && wire[4] == 1);
        assert(wire[5] == 5 && wire[6] == 13 && song.octave == 2);
        dino_wire(bad, sizeof bad); assert(wire[1] == 1 && dino.mode == DINO_LAYER);
        dino_wire(save, sizeof save); assert(!wire[1] && wire[7] == DINO_SAVE_PENDING);
        dino_wire(cancel, sizeof cancel); assert(!wire[7]);
        dino_wire(rename, sizeof rename); assert(!wire[1] && wire[7] == DINO_SAVE_PENDING);
        fm1_ms += 2000; dino_save_poll(); assert(!strcmp(dino_performance_label(1), "NEW"));
        assert(ed_dino_retired(0, P_AMODE) && ed_dino_retired(0, P_CHORD) && ed_dino_retired(1, G_NEWPRJ));
        assert(!ed_dino_retired(0, P_LEVEL) && !ed_dino_retired(0, P_TRANS) && !ed_dino_retired(1, G_BEND));
        puts("DINO wire state/set/SAVE/cancel/rename and retired parameter boundaries: ok");
    }
    return 0;
}
