/* SPDX-License-Identifier: GPL-3.0-only */
/* Included after ui_input.c; the sound pages and calibrated panel stay upstream. */
static uint8_t dino_visual_active, dino_save_kind, dino_save_slot, dino_drum_lane;
static uint8_t dino_save_was, dino_save_done, dino_replace_bank;
static uint32_t dino_save_since;
static const char *const DINO_MODES[] = {"SINGLE", "LAYER", "SPLIT", "DRUMS"};

static void dino_wheel_owner(int owned)
{
    if ((dino.wheel_owned != 0) != (owned != 0)) {
        dino_enc_take(dino.mod_enc);
        dino_enc_take(dino.bend_enc);
        dino.wheel_owned = (uint8_t)(owned != 0);
    }
}
static void dino_home(void)
{
    dino.screen = DINO_SCREEN_HOME;
    ui.home = (uint8_t)!dino_visual_active;
    ui.page = (uint8_t)page_first(FAM_TRK);
    ui.layer = LY_PLAY;
    ui.confirm = ui.hold_kind = 0;
    vis_on = dino_visual_active;
    ui.force = 1;
    dino_wheel_owner(1);
}
static void dino_screen(uint32_t screen)
{
    dino.screen = (uint8_t)screen;
    ui.home = 0;
    ui.page = (uint8_t)page_first(FAM_TRK);
    vis_on = 0;
    ui.force = 1;
    dino_wheel_owner(screen == DINO_SCREEN_PRESETS);
}
static void dino_init_live(void)
{
    for (uint32_t i = 0; i < NTRK; i++) dino_track_direct(&trk[i]);
    trk[1].p[P_MUTE] = 1;
    song.playing = song.rec = song.seq_mode = 0;
    dino.split = (uint8_t)clamp(dino.split, 1, 127);
    dino_mode_set(dino.mode);
    dino_panel_bind();
    dino_controllers_reset();
    panic_req = (1u << NTRK) - 1u;
    dino_home();
}
static void dino_ui_sound_name(uint32_t part, char *out)
{
    track_t *t = &trk[part];
    if (part == TRK_DRUM) str_cpy(out, DRUM_KIT_NAMES[drum_kit()], 13);
    else if (user_of(t) < UP_SLOTS) up_name(user_of(t), out);
    else {
        const engine_t *e = ENGINES[t->eng_req % NENGINES];
        str_cpy(out, e->npresets ? e->presets[t->preset % e->npresets].name : e->name, 13);
    }
}
static uint32_t dino_preset_pos(uint32_t *total)
{
    if (is_drum(TSEL)) { *total = DRUM_KITS; return drum_kit(); }
    return preset_pos(total);
}
static void dino_preset_move(int32_t delta)
{
    uint32_t total, at = dino_preset_pos(&total);
    if (!total) return;
    uint32_t next = (uint32_t)(((int32_t)at + delta % (int32_t)total + (int32_t)total) % (int32_t)total);
    if (next == at) return;
    if (is_drum(TSEL)) TDRUM->p[P_E0] = (int16_t)next;
    else preset_go(next);
    ui.force = 1;
}
/* Both the list and its tests read the same names and type/engine labels. */
static void dino_preset_info(uint32_t index, char *name, char *detail)
{
    if (is_drum(TSEL)) {
        str_cpy(name, DRUM_KIT_NAMES[index % DRUM_KITS], 13);
        str_cpy(detail, "DRUMS / KIT", 24);
        return;
    }
    uint32_t slot, engine = preset_at(index, &slot), user = engine == NENGINES;
    if (user) { up_name(slot, name); engine = dino_patch_engine(slot); }
    else str_cpy(name, ENGINES[engine]->presets[slot].name, 13);
    str_cpy(detail, engine < NENGINES ? ENGINES[engine]->name : "SOUND", 24);
    uint32_t n = str_len(detail);
    str_cpy(detail + n, user ? " / USER" : " / FACTORY", 24u - n);
}
static void dino_open_save(void)
{
    dino_save_cancel();
    dino_save_kind = (uint8_t)(dino.mode != DINO_SINGLE);
    dino_save_slot = 0;
    if (!dino_save_kind) {
        uint32_t slot = user_of(TSEL);
        if (slot < UP_SLOTS) dino_save_slot = (uint8_t)slot;
        else for (slot = 0; slot < UP_SLOTS; slot++) if (!up_used(slot)) {
            dino_save_slot = (uint8_t)slot;
            break;
        }
    }
    dino_save_was = 1;
    dino_save_since = 0;
    dino_save_done = 0;
    dino_replace_bank = 0;
    dino_screen(DINO_SCREEN_SAVE);
}
static void dino_save_controls(uint32_t pressed)
{
    int32_t s;
    uint32_t down = (fm1_in.buttons >> panel.btn[B_SAVE]) & 1u;
    uint32_t full = dino_save_status() == DINO_SAVE_BANK_FULL;
    if ((s = panel_enc(EN_SELECT)) != 0) {
        dino_save_cancel();
        dino_save_kind ^= 1u;
        if (dino.mode == DINO_DRUMS) dino_save_kind = DINO_SAVE_PERFORMANCE;
        dino_save_slot = 0;
        dino_save_since = 0;
        ui.force = 1;
    }
    s = panel_enc(EN_PRESET);
    s += panel_enc(EN_K1);
    if (s && !down && full) {
        dino_replace_bank = (uint8_t)clamp(dino_replace_bank + s, 0, FM6_BANK_N - 1);
        ui.force = 1;
    } else if (s && !down) {
        dino_save_cancel();
        dino_save_slot = (uint8_t)clamp((int32_t)dino_save_slot + s, 0,
                                      dino_save_kind ? 3 : (int32_t)UP_SLOTS - 1);
        dino_save_since = 0;
        ui.force = 1;
    }
    if ((pressed >> panel.btn[B_OCTUP]) & 1u) {
        uint32_t rc = dino_save_kind ? dino_performance_load(dino_save_slot) :
                      (uint32_t)up_load(dino_save_slot);
        ui_message(rc == 3u ? "RELEASE NOTES" : rc ? "EMPTY / INVALID" : "LOADED");
        if (!rc) dino_home();
    }
    if (down && !dino_save_was) {
        dino_save_since = fm1_ms | 1u;
        dino_save_done = 0;
    }
    if (down && dino_save_since && !dino_save_done && fm1_ms - dino_save_since >= 1000u) {
        if (full) dino_save_replace_bank(dino_replace_bank);
        else dino_save_request(dino_save_kind, dino_save_slot);
        dino_save_done = 1;
        ui.force = 1;
    }
    if (!down) dino_save_since = 0;
    dino_save_was = (uint8_t)down;
    panel_enc(EN_K2); panel_enc(EN_K3); panel_enc(EN_K4); panel_enc(EN_ALGO);
}
static void dino_ui_input(void)
{
    uint32_t pressed = fm1_input_edges(0), home = btn_hold(&ui.home_t0, B_HOME, fm1_ticks(), 1);
    int32_t s;
    enc_hold = 0;
    fm1_input_note_edges();
    pc_poll();
    fm6_poll();
    ml_poll();
    dino_focus_set(dino.focus);
    if (home == BT_HOLD) {
        if (ui.menu) { menu_close(); dino_home(); }
        else { ui.menu = 1; ui.menu_sel = 0; ui.force = 1; dino_wheel_owner(0); }
    }
    if (ui.menu) {
        menu_input(pressed);
        if (!ui.menu) dino_home();
        return;
    }
    if (home == BT_TAP) {
        dino_save_cancel();
        if (dino.screen == DINO_SCREEN_HOME) dino_visual_active ^= 1u;
        dino_home();
    }
    if ((pressed >> panel.btn[B_PLAY]) & 1u) {
        panic_req = (1u << NTRK) - 1u;
        dino_controllers_reset();
        ui_message("PANIC");
    }
    if (dino.screen == DINO_SCREEN_SAVE) { dino_save_controls(pressed); return; }
    if ((pressed >> panel.btn[B_SAVE]) & 1u) { dino_open_save(); return; }
    if ((pressed >> panel.btn[B_REC]) & 1u) {
        dino_focus_set(!dino.focus);
        sync_reload = 1;
        ui.force = 1;
    }
    if ((pressed >> panel.btn[B_SCL]) & 1u) {
        if (dino.screen == DINO_SCREEN_PRESETS) dino_home();
        else dino_screen(DINO_SCREEN_PRESETS);
    }
    if ((pressed >> panel.btn[B_SEQ]) & 1u) dino_screen(DINO_SCREEN_PERFORMANCE);
    if ((pressed >> panel.btn[B_GLO]) & 1u) dino_screen(DINO_SCREEN_GLOBAL);
    if ((pressed >> panel.btn[B_EDIT]) & 1u && dino.mode == DINO_DRUMS)
        dino_screen(DINO_SCREEN_DRUM_EDIT);
    else {
        uint32_t fam = (pressed >> panel.btn[B_ENV]) & 1u ? FAM_ENV :
                       (pressed >> panel.btn[B_LFO]) & 1u ? FAM_LFO :
                       (pressed >> panel.btn[B_FX]) & 1u ? FAM_FX :
                       (pressed >> panel.btn[B_EDIT]) & 1u ? FAM_EDIT : FAM_COUNT;
        if (fam != FAM_COUNT) {
            dino.screen = DINO_SCREEN_EDIT;
            dino_wheel_owner(0);
            vis_on = 0;
            open_family(fam);
        }
    }
    if ((fm1_in.buttons & (dyn_bit[0] | dyn_bit[1])) == (dyn_bit[0] | dyn_bit[1])) song.octave = 0;
    else {
        if ((pressed >> panel.btn[B_OCTDN]) & 1u) song.octave = (int8_t)clamp(song.octave - 1, -3, 3);
        if ((pressed >> panel.btn[B_OCTUP]) & 1u) song.octave = (int8_t)clamp(song.octave + 1, -3, 3);
    }
    if ((s = panel_enc(EN_ALGO)) != 0 && !is_drum(TSEL)) {
        int32_t engine = ((int32_t)TSEL->eng_req + s) % NENGINES;
        select_engine((uint32_t)(engine < 0 ? engine + NENGINES : engine));
    }
    s = panel_enc(EN_PRESET);
    /* Coalesce both browsing knobs into one audition per UI frame. */
    if (dino.screen == DINO_SCREEN_PRESETS) s += panel_enc(EN_SELECT);
    if (s) dino_preset_move(s);
    if ((s = panel_enc(EN_SELECT)) != 0) {
        if (dino.screen == DINO_SCREEN_EDIT) page_walk(s);
        else if (dino.screen == DINO_SCREEN_HOME && dino_visual_active) vis_select(s);
        else song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + s, 40, 240);
        ui.force = 1;
    }
    for (uint32_t k = 0; k < 4u; k++) {
        if (!(s = panel_enc(EN_K1 + k))) continue;
        if (dino.screen == DINO_SCREEN_EDIT) edit_param(k, s);
        else if (dino.screen == DINO_SCREEN_PERFORMANCE) {
            if (!k) dino_mode_set((uint32_t)clamp((int32_t)dino.mode + s, DINO_SINGLE, DINO_DRUMS));
            else if (k == 1u) dino.split = (uint8_t)clamp(dino.split + s, 1, 127);
            else {
                track_t *t = &trk[k == 2u ? DINO_A : DINO_B];
                t->p[P_LEVEL] = (int16_t)clamp(t->p[P_LEVEL] + s, 0, 127);
            }
        } else if (dino.screen == DINO_SCREEN_GLOBAL) {
            uint32_t id = k == 0 ? G_BPM : k == 1 ? G_TUNE : k == 2 ? G_CH1 : G_BEND;
            song.g[id] = (int16_t)clamp(song.g[id] + s, GP[id].min, GP[id].max);
        } else if (dino.screen == DINO_SCREEN_DRUM_EDIT) {
            if (!k) dino_drum_lane = (uint8_t)clamp(dino_drum_lane + s, 0, 15);
            else if (k == 1u) drum_tune[dino_drum_lane] = (int8_t)clamp(drum_tune[dino_drum_lane] + s, DRUM_TUNE_MIN, DRUM_TUNE_MAX);
            else if (k == 2u) drum_dec[dino_drum_lane] = (int8_t)clamp(drum_dec[dino_drum_lane] + s, DRUM_DEC_MIN, DRUM_DEC_MAX);
            else TDRUM->p[P_E0] = (int16_t)clamp(TDRUM->p[P_E0] + s, 0, DRUM_KITS - 1);
        } else if (k >= 2u) {
            if (is_drum(TSEL)) {
                uint32_t lane = dino.last_drum % DRUM_LANES;
                int8_t *v = k == 2u ? &drum_tune[lane] : &drum_dec[lane];
                *v = (int8_t)clamp(*v + s, k == 2u ? DRUM_TUNE_MIN : DRUM_DEC_MIN,
                                  k == 2u ? DRUM_TUNE_MAX : DRUM_DEC_MAX);
            } else {
                uint32_t id = ENGINES[TSEL->eng_req % NENGINES]->macro[k - 2u];
                const param_desc_t *desc = track_desc(TSEL, id);
                TSEL->p[id] = (int16_t)clamp(TSEL->p[id] + s, desc->min, desc->max);
                ml_knob(TSEL, id);
            }
        }
        ui.force = 1;
    }
    dino_wheel_owner(dino.screen == DINO_SCREEN_HOME || dino.screen == DINO_SCREEN_PRESETS);
}
static void dino_row(uint32_t y, const char *label, uint16_t color)
{
    cv_begin(240, 20, C_BLACK);
    cv_text(4, 1, &FONT_S, label, color);
    cv_blit(0, y);
}
static void dino_value(uint32_t column, const char *label, int32_t value)
{
    char b[12];
    fmt_int(b, value);
    cv_text(4 + (int32_t)column * 60, 2, &FONT_S, label, TE_COL[column]);
    cv_text(4 + (int32_t)column * 60, 27, &FONT_S, b, C_WHITE);
}
static int dino_ui_draw(void)
{
    static uint32_t last;
    char a[24], b[24], name[13];
    uint32_t hash = dino.screen * 31u + dino.mode * 131u + dino.focus * 571u + dino.mod * 977u +
                    (uint32_t)(dino.bend + 8192) + (uint32_t)(song.octave + 3) * 8189u + dino.last_drum * 3931u;
    if (ui.menu) { draw_menu(); ui_timers(); ui.force = 0; return 1; }
    if (dino.screen == DINO_SCREEN_EDIT) {
        const page_t *pg = cur_page();
        uint16_t color = TE_COL[song.sel & 3u];
        if (ui.force) draw_frame();
        draw_head(); draw_columns();
        cv_begin(240, H_GRAPH, C_BLACK); cv_oy = G_OY;
        if (pg->graph == GR_ADSR) graph_adsr(TSEL, color);
        else if (pg->graph == GR_LFO) graph_lfo(TSEL, color);
        else if (pg->graph == GR_FX) graph_fx(TSEL, color);
        else if (pg->graph == GR_SLCR) graph_slicer(TSEL, color);
        else { cv_oy = 0; graph_big(); }
        cv_oy = 0; cv_blit(0, Y_GRAPH);
        draw_foot(); ui_timers(); ui.force = 0;
        return 1;
    }
    if (dino.screen == DINO_SCREEN_PRESETS) {
        if (!ui.force) { ui_timers(); return 1; }
        lcd_fill(0, 0, 240, 240, C_BLACK);
        dino_row(0, "PRESETS  SEL: CLOSE", C_HI);
        uint32_t total, at = dino_preset_pos(&total);
        for (uint32_t row = 0; row < 5u && total; row++) {
            uint32_t index = (at + total + row - 2u) % total;
            char detail[24]; dino_preset_info(index, name, detail);
            cv_begin(240, 40, row == 2u ? C_LINE : C_BLACK);
            cv_text(8, 2, &FONT_S, name, row == 2u ? C_WHITE : C_DIM);
            cv_text(8, 21, &FONT_S, detail, row == 2u ? C_HI : C_DIM);
            cv_blit(0, 20u + row * 40u);
        }
        dino_row(220, "SELECT / PRESETS: AUDITION", C_GRAY);
        ui_timers(); ui.force = 0;
        return 1;
    }
    if (dino.screen == DINO_SCREEN_HOME && dino_visual_active) {
        ui.home = 0;
        ui.page = (uint8_t)page_first(FAM_TRK);
        vis_on = 1;
        vis_draw();
        ui_timers(); ui.force = 0;
        return 1;
    }
    for (uint32_t part = 0; part < NTRK; part++) {
        hash = hash * 33u + trk[part].preset + 31u * trk[part].eng_req;
        for (uint32_t p = 0; p < P_COUNT; p++) hash = hash * 33u + (uint16_t)trk[part].p[p];
    }
    hash += dino_save_kind * 131u + dino_save_slot * 571u + dino_save_status() * 977u +
            dino_drum_lane * 8191u + (uint32_t)ui.msg_t * 3u;
    for (uint32_t lane = 0; lane < DRUM_LANES; lane++)
        hash = hash * 33u + (uint8_t)drum_tune[lane] + ((uint32_t)(uint8_t)drum_dec[lane] << 8);
    if (dino.screen == DINO_SCREEN_SAVE && dino_save_since) hash += (fm1_ms - dino_save_since) / 100u;
    if (!ui.force && hash == last) { ui_timers(); return 1; }
    last = hash;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    dino_ui_sound_name(DINO_A, name); str_cpy(a, "A ", sizeof a); str_cpy(a + 2, name, sizeof a - 2);
    dino_ui_sound_name(DINO_B, name); str_cpy(b, "B ", sizeof b); str_cpy(b + 2, name, sizeof b - 2);
    if (dino.screen == DINO_SCREEN_SAVE) {
        uint32_t status = dino_save_status();
        int used = dino_save_kind ? str_eq(dino_performance_label(dino_save_slot), "EMPTY") == 0 : up_used(dino_save_slot);
        dino_row(0, dino_save_kind ? "SAVE PERFORMANCE" : "SAVE PATCH", C_HI);
        if (dino_save_kind) str_cpy(name, dino_performance_label(dino_save_slot), sizeof name);
        else if (used) up_name(dino_save_slot, name);
        else str_cpy(name, "EMPTY", sizeof name);
        str_cpy(a, dino_save_kind ? "P" : "U", sizeof a);
        fmt_int(a + 1, dino_save_slot + 1u);
        str_cpy(a + str_len(a), used ? " REPLACE" : " EMPTY", sizeof a - str_len(a));
        dino_row(40, a, used ? TE_RED : C_WHITE);
        dino_row(64, name, C_WHITE);
        dino_row(112, "SELECT: PATCH / PERF", C_GRAY);
        dino_row(136, "PRESETS: SLOT  OCT+: LOAD", C_GRAY);
        dino_row(160, "HOLD SAVE 1S TO STORE", C_WHITE);
        if (status == DINO_SAVE_BANK_FULL) {
            str_cpy(a, "REPLACE FM6 B", sizeof a); fmt_int(a + str_len(a), dino_replace_bank + 1u);
            dino_row(112, a, TE_RED);
            dino_row(136, "MAY CHANGE OTHER PATCHES", TE_RED);
            dino_row(160, "K1: BANK  HOLD SAVE: YES", C_WHITE);
        }
        dino_row(190, status == DINO_SAVE_PENDING ? "WAIT FOR SILENCE" : status == DINO_SAVE_SAVED ? "SAVED" :
                 status == DINO_SAVE_BANK_FULL ? "HOME: CANCEL REPLACEMENT" : status == DINO_SAVE_ERROR ? "SAVE ERROR" :
                 dino_save_since ? "KEEP HOLDING..." : "HOME: CANCEL", status == DINO_SAVE_ERROR ? TE_RED : C_HI);
    } else {
        dino_row(0, dino.screen == DINO_SCREEN_PERFORMANCE ? "PERFORMANCE" : dino.screen == DINO_SCREEN_GLOBAL ? "GLOBAL" :
                 dino.screen == DINO_SCREEN_DRUM_EDIT ? "DRUM SOUND" : DINO_MODES[dino.mode], C_HI);
        dino_row(30, a, dino.focus == 0 ? C_WHITE : C_DIM);
        dino_row(52, b, dino.focus == 1 ? C_WHITE : C_DIM);
        cv_begin(240, 70, C_BLACK);
        if (dino.screen == DINO_SCREEN_PERFORMANCE) {
            cv_text(4, 2, &FONT_S, "MODE", TE_COL[0]); cv_text(4, 27, &FONT_S, DINO_MODES[dino.mode], C_WHITE);
            dino_value(1, "SPLIT", dino.split); dino_value(2, "A LVL", trk[DINO_A].p[P_LEVEL]);
            dino_value(3, "B LVL", trk[DINO_B].p[P_LEVEL]);
        } else if (dino.screen == DINO_SCREEN_GLOBAL) {
            dino_value(0, "BPM", song.g[G_BPM]); dino_value(1, "TUNE", song.g[G_TUNE]);
            dino_value(2, "RX CH", song.g[G_CH1]); dino_value(3, "BEND", song.g[G_BEND]);
        } else if (dino.screen == DINO_SCREEN_DRUM_EDIT) {
            dino_value(0, "SOUND", dino_drum_lane + 1u); dino_value(1, "TUNE", drum_tune[dino_drum_lane]);
            dino_value(2, "DECAY", drum_dec[dino_drum_lane]); dino_value(3, "KIT", drum_kit() + 1u);
        } else {
            dino_value(0, "MOD", dino.mod); dino_value(1, "BEND", dino.bend * 100 / 8192);
            if (is_drum(TSEL)) {
                uint32_t lane = dino.last_drum % DRUM_LANES;
                dino_value(2, "TUNE", drum_tune[lane]); dino_value(3, "DECAY", drum_dec[lane]);
            } else for (uint32_t k = 0; k < 2u; k++) {
                uint32_t id = ENGINES[TSEL->eng_req % NENGINES]->macro[k];
                dino_value(k + 2u, track_desc(TSEL, id)->label, TSEL->p[id]);
            }
        }
        cv_blit(0, 100);
        if (dino.mode == DINO_DRUMS) {
            dino_ui_sound_name(TRK_DRUM, name); dino_row(174, name, C_WHITE);
            dino_row(194, LANE_NAME[dino.last_drum % DRUM_LANES], C_GRAY);
        }
        else dino_row(184, "ARP + K2: PITCH BEND", C_GRAY);
        dino_row(220, ui.msg_t ? ui.msg : "SEL: PRESETS  SAVE: STORE", C_GRAY);
    }
    ui_timers(); ui.force = 0;
    return 1;
}
