/* SPDX-License-Identifier: GPL-3.0-only */
#if FELUCCA_DINO
#include "dino_store.h"
/* Reuse the old autosave and project RAM; flash addresses and A/B commit remain upstream's. */
#define dino_snapshot ((dino_performance_t *)&autosave_buf)
static uint8_t dino_pending_kind, dino_pending_slot, dino_save_result;
static int8_t dino_bank_replace = -1;
static uint8_t dino_restore_busy;
static uint32_t dino_restore_ms;
static uint32_t dino_auto_hash, dino_auto_checked, dino_auto_ms;
static const uint8_t DINO_TRACK[3] = {DINO_A, DINO_B, TRK_DRUM};
static int dino_global_saved(uint32_t id)
{
    return id == G_BPM || id == G_TUNE || (id >= G_DTIME && id <= G_CDEPTH) || id == G_DRLVL ||
           id == G_DRREV || id == G_DRDLY || id == G_DUST || id == G_DUCK || id == G_FILT ||
           (id >= G_TXSRC && id <= G_TXDUCK);
}
static int dino_audio_quiet(void)
{
    uint32_t gap = delay_samples() * 1000u / FS + 100u;
    if (gap < 1000u) gap = 1000u;
    return audio_quiet() && fm1_ms - dino_signal_ms >= gap;
}
static void dino_sound_name(const track_t *t, char *name)
{
    if (t->user && up_used(t->user - 1u)) up_name(t->user - 1u, name);
    else str_cpy(name, ENGINES[t->eng_req % NENGINES]->presets[
                     t->preset % ENGINES[t->eng_req % NENGINES]->npresets].name, 13);
}
static void dino_capture(dino_performance_t *p, uint32_t magic)
{
    uint32_t i, k;
    memset(p, 0, sizeof *p);
    p->magic = magic; p->size = sizeof *p; p->version = DINO_STORE_VERSION;
    p->np = P_COUNT; p->ng = G_COUNT;
    dino_sound_name(&trk[dino.focus ? DINO_B : DINO_A], p->name);
    p->mode = dino.mode; p->focus = dino.focus; p->split = dino.split;
    p->octave = song.octave; p->style = vis_style;
    for (i = 0; i < G_COUNT; i++) p->g[i] = dino_global_saved(i) ? song.g[i] : GP[i].def;
    for (k = 0; k < 3u; k++) {
        track_t *t = &trk[DINO_TRACK[k]];
        dino_sound_t *s = &p->sound[k];
        s->engine = k == 2u ? 0u : t->eng_req; s->preset = t->preset; s->user = t->user;
        memcpy(s->p, t->p, sizeof s->p);
        s->p[P_AMODE] = s->p[P_AHOLD] = s->p[P_CHORD] = s->p[P_QUANT] = s->p[P_STRUM] = s->p[P_VLEAD] = 0;
        if (k < 2u && s->engine == ENGI_FM6) {
            uint32_t part = DINO_TRACK[k];
            if (((fm6_pend >> part) & 1u) || fm6_slot[part] != t->p[P_E7]) fm6_slot_get((uint32_t)t->p[P_E7], s->fm6);
            else fm6_pack(fm6_patch[part], s->fm6);
        }
    }
    memcpy(p->tune, drum_tune, sizeof p->tune); memcpy(p->decay, drum_dec, sizeof p->decay);
    dino_store_seal(p);
}
static void dino_apply(const dino_performance_t *p)
{
    uint32_t i, k, irq = fm1_icfg();
    fm1_irq_off();
    dino_controllers_reset();
    song.playing = song.rec = song.solo = 0; transport_req = 0;
    for (i = 0; i < G_COUNT; i++)
        if (dino_global_saved(i)) song.g[i] = (int16_t)clamp(p->g[i], GP[i].min, GP[i].max);
    for (k = 0; k < 3u; k++) {
        track_t *t = &trk[DINO_TRACK[k]];
        const dino_sound_t *s = &p->sound[k];
        t->eng_req = s->engine; t->user = s->user; t->preset = s->preset;
        for (i = 0; i < P_COUNT; i++) {
            const param_desc_t *d = k == 2u && i == P_E0 ? &DRUM_KIT_DESC : up_desc(s->engine, i);
            t->p[i] = (int16_t)clamp(s->p[i], d->min, d->max);
        }
        dino_track_direct(t); steps_clear(t);
        if (k < 2u && s->engine == ENGI_FM6) {
            uint8_t v[FP_SIZE + 1u];
            fm6_unpack(s->fm6, v); fm6_set_patch(DINO_TRACK[k], v);
            fm6_slot[DINO_TRACK[k]] = (uint8_t)t->p[P_E7];
            fm6_pend &= (uint8_t)~(1u << DINO_TRACK[k]);
        }
    }
    song.octave = p->octave;
    memcpy(drum_tune, p->tune, sizeof p->tune); memcpy(drum_dec, p->decay, sizeof p->decay);
    vis_style = p->style;
    dino_mode_set(p->mode); dino_focus_set(p->focus); dino.split = p->split;
    panic_req = (1u << NTRK) - 1u;                    /* Also clear owners of decayed notes whose keys are still held. */
    fm1_irq_restore(irq);
    sync_reload = 1; ui.force = 1;
}
static uint32_t dino_save_status(void) { return dino_save_result; }
static uint32_t dino_patch_engine(uint32_t slot) { return up_used(slot) ? up_rec(slot)->engine : NENGINES; }
static void dino_save_cancel(void) { dino_save_result = DINO_SAVE_IDLE; }
static uint32_t dino_save_request(uint32_t kind, uint32_t slot)
{
    if (dino_save_result == DINO_SAVE_PENDING || dino_save_result == DINO_SAVE_BANK_FULL || kind > DINO_SAVE_PERFORMANCE ||
        slot >= (kind == DINO_SAVE_PATCH ? UP_SLOTS : 4u) || (kind == DINO_SAVE_PATCH && dino.mode == DINO_DRUMS))
        return 1;
    dino_capture(dino_snapshot, DINO_PERF_MAGIC);
    dino_pending_kind = (uint8_t)kind; dino_pending_slot = (uint8_t)slot;
    dino_bank_replace = -1;
    dino_save_result = DINO_SAVE_PENDING; ui.force = 1;
    return 0;
}
static uint32_t dino_save_replace_bank(uint32_t bank)
{
    if (dino_save_result != DINO_SAVE_BANK_FULL || bank >= FM6_BANK_N) return 1;
    dino_bank_replace = (int8_t)bank;
    dino_save_result = DINO_SAVE_PENDING;
    return 0;
}
static uint32_t dino_patch_named(uint32_t slot, const char *name)
{
    uint32_t rc = dino_save_request(DINO_SAVE_PATCH, slot);
    if (!rc && name && name[0]) {
        str_cpy(dino_snapshot->name, name, sizeof dino_snapshot->name);
        dino_store_seal(dino_snapshot);
    }
    return rc;
}
/* Find the exact voice first; only an empty slot may be allocated by SAVE. */
static int dino_fm6_keep(const uint8_t *pk)
{
    uint32_t k; int free_slot = -1; uint8_t old[128];
    for (k = 0; k < FM6_BANK_N; k++) {
        if (!fm6_bank_get(k, old)) { if (!memcmp(pk, old, 128)) return (int)k; }
        else if (free_slot < 0) free_slot = (int)k;
    }
    if (free_slot < 0) return -2;
    return fm6_bank_put((uint32_t)free_slot, pk) ? -1 : free_slot;
}
static uint32_t dino_patch_commit(void)
{
    const dino_sound_t *s = &dino_snapshot->sound[dino_snapshot->focus];
    up_rec_t r; int bank;
    memset(&r, 0, sizeof r);
    r.used = UP_USED; r.ver = UP_VER; r.engine = s->engine; r.np = P_COUNT;
    memcpy(r.name, dino_snapshot->name, sizeof r.name); memcpy(r.p, s->p, sizeof s->p);
    if (s->engine == ENGI_FM6) {
        bank = dino_bank_replace < 0 ? dino_fm6_keep(s->fm6) :
               fm6_bank_put((uint32_t)dino_bank_replace, s->fm6) ? -1 : dino_bank_replace;
        if (bank < 0) return bank == -2 ? DINO_SAVE_BANK_FULL : DINO_SAVE_ERROR;
        r.p[P_E7] = (int16_t)(FM6_NFACTORY + bank);
    }
    return up_put(dino_pending_slot, &r) ? DINO_SAVE_ERROR : DINO_SAVE_SAVED;
}
static const char *dino_performance_label(uint32_t slot)
{
    const dino_performance_t *p;
    if (slot >= 4u) return "EMPTY";
    p = (const dino_performance_t *)&proj_slot[slot];
    if (dino_store_valid(p, sizeof *p, DINO_PERF_MAGIC)) return p->name;
    return proj_ok(&proj_slot[slot]) ? "LEGACY SONG" : "EMPTY";
}
static uint32_t dino_performance_load(uint32_t slot)
{
    const dino_performance_t *p;
    if (slot >= 4u || dino_save_result == DINO_SAVE_PENDING || dino_save_result == DINO_SAVE_BANK_FULL) return 1;
    p = (const dino_performance_t *)&proj_slot[slot];
    if (!dino_store_valid(p, sizeof *p, DINO_PERF_MAGIC)) return 2;
    if (!dino_audio_quiet()) return 3;
    dino_apply(p); return 0;
}
static uint32_t dino_performance_rename(uint32_t slot, const uint8_t *name, uint32_t n)
{
    if (slot >= 4u || !up_name_ok(name, n) || dino_save_result == DINO_SAVE_PENDING || dino_save_result == DINO_SAVE_BANK_FULL) return 1;
    if (!dino_store_valid((const dino_performance_t *)&proj_slot[slot], sizeof *dino_snapshot, DINO_PERF_MAGIC)) return 2;
    memcpy(dino_snapshot, &proj_slot[slot], sizeof *dino_snapshot);
    memset(dino_snapshot->name, 0, sizeof dino_snapshot->name); memcpy(dino_snapshot->name, name, n);
    dino_store_seal(dino_snapshot); dino_pending_kind = DINO_SAVE_PERFORMANCE; dino_pending_slot = (uint8_t)slot;
    dino_save_result = DINO_SAVE_PENDING; return 0;
}
static void dino_store_boot(void)
{
    uint32_t i; int n;
    for (i = 0; i < 4u; i++) {
        memset(&proj_slot[i], 0, sizeof proj_slot[i]);
        if (flash_ok) st_load(OBJ_PROJECT0 + i, &proj_slot[i], sizeof proj_slot[i]);
    }
    sec_dirty = song_dirty = 0;
    vis_style = 12u;                                  /* COSMIC DOG until a DINO performance restores another style. */
    if (!flash_ok) { settings.lowcut = 0; fx_lowcut = 0; return; }
    n = st_load(OBJ_AUTOSAVE, dino_snapshot, sizeof *dino_snapshot);
    if (dino_store_valid(dino_snapshot, (uint32_t)n, DINO_AUTO_MAGIC)) {
        dino_auto_hash = dino_snapshot->sum; dino_apply(dino_snapshot);
    } else {
        settings.lowcut = 0; fx_lowcut = 0; settings_later = 1; /* First DINO boot starts with an unfiltered line output. */
    }
}
static uint32_t dino_restore(uint32_t slot, const void *raw, uint32_t n)
{
    const dino_performance_t *p = raw;
    if (!dino_audio_quiet() || dino_save_result == DINO_SAVE_PENDING || dino_save_result == DINO_SAVE_BANK_FULL) return 3;
    if (slot > 4u) return 1;
    if (!n && slot < 4u) {
        if (!flash_ok || st_save(OBJ_PROJECT0 + slot, raw, 0)) return 4;
        memset(&proj_slot[slot], 0, sizeof proj_slot[slot]); return 0;
    }
    if (!dino_store_valid(p, n, slot == 4u ? DINO_AUTO_MAGIC : DINO_PERF_MAGIC)) {
        if (slot >= 4u || n != sizeof(project_t) || !proj_ok((const project_t *)raw)) return 2;
    }
    if (slot == 4u) { dino_apply(p); return 0; }
    if (!flash_ok || st_save(OBJ_PROJECT0 + slot, raw, n)) return 4;
    memset(&proj_slot[slot], 0, sizeof proj_slot[slot]); memcpy(&proj_slot[slot], raw, n); return 0;
}
static void dino_settings_poll(void)
{
    static uint32_t tried;
    static const uint8_t ids[10] = {G_CH1, G_CH2, G_CH3, G_DRCH, G_CHOTH, G_BEND, G_FXCH, G_DRMAP, G_RMCH, G_CLKOUT};
    uint32_t i;
    for (i = 0; i < 10u; i++)
        if (midi_set[i] != (uint32_t)song.g[ids[i]]) { midi_set[i] = (uint8_t)song.g[ids[i]]; settings_later = 1; }
    if (lights_sync != (uint32_t)song.g[G_SYNC]) { lights_sync = (uint8_t)song.g[G_SYNC]; settings_later = 1; }
    if (lights_mout != (uint32_t)(song.g[G_MIDI] != 0)) { lights_mout = (uint8_t)(song.g[G_MIDI] != 0); settings_later = 1; }
    if (lights_min != (uint32_t)song.g[G_ROUTE]) { lights_min = (uint8_t)song.g[G_ROUTE]; settings_later = 1; }
    if (fav_dirty) settings_later = 1;
    if (!settings_later || dino_restore_busy || dino_save_result == DINO_SAVE_PENDING || dino_save_result == DINO_SAVE_BANK_FULL ||
        fm1_ms - ui_input_ms < 1500u || fm1_ms - tried < 5000u || !dino_audio_quiet()) return;
    tried = fm1_ms; settings_later = 0; settings_save();
}
static void dino_save_poll(void)
{
    uint32_t now = fm1_ms;
    dino_settings_poll();
    if (dino_restore_busy && now - dino_restore_ms < 15000u) return;
    dino_restore_busy = 0;
    if (dino_save_result == DINO_SAVE_BANK_FULL) return;
    if (dino_save_result == DINO_SAVE_PENDING) {
        if (!dino_audio_quiet()) return;
        if (!flash_ok) dino_save_result = DINO_SAVE_ERROR;
        else if (dino_pending_kind == DINO_SAVE_PATCH) dino_save_result = (uint8_t)dino_patch_commit();
        else if (st_save(OBJ_PROJECT0 + dino_pending_slot, dino_snapshot, sizeof *dino_snapshot)) dino_save_result = DINO_SAVE_ERROR;
        else {
            memset(&proj_slot[dino_pending_slot], 0, sizeof proj_slot[0]);
            memcpy(&proj_slot[dino_pending_slot], dino_snapshot, sizeof *dino_snapshot); dino_save_result = DINO_SAVE_SAVED;
        }
        ui.force = 1; return;
    }
    if (!flash_ok || ui.menu || now - ui_input_ms < AUTOSAVE_IDLE || now - dino_auto_ms < AUTOSAVE_GAP ||
        now - dino_auto_checked < 1000u || !dino_audio_quiet()) return;
    dino_auto_checked = now;
    dino_capture(dino_snapshot, DINO_AUTO_MAGIC);
    if (dino_snapshot->sum != dino_auto_hash && !st_save(OBJ_AUTOSAVE, dino_snapshot, sizeof *dino_snapshot))
        dino_auto_hash = dino_snapshot->sum;
    dino_auto_ms = now;
}
#endif
