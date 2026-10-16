/* SPDX-License-Identifier: GPL-3.0-only */
#include "dino_ui_host.h"
static const char *preview_dir;
static void preview(const char *name)
{
    if (!preview_dir) return;
    char path[512]; snprintf(path, sizeof path, "%s/%s.ppm", preview_dir, name);
    FILE *f = fopen(path, "wb"); assert(f); fprintf(f, "P6\n240 240\n255\n");
    for (uint32_t i = 0; i < 240u * 240u; i++) {
        uint16_t p = swap16(screen[i]);
        uint8_t rgb[3] = {(p >> 11) * 255 / 31, ((p >> 5) & 63) * 255 / 63, (p & 31) * 255 / 31};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}
int main(int argc, char **argv)
{
    preview_dir = argc > 1 ? argv[1] : 0;
    dino_test_init(); settings_init(); panel_init(); layers_init(); palette_set(0); dino_init_live();
    dino_test_frame(); preview("live");
    fm1_in.enc_steps[panel.enc[EN_K1]] = 20; dino_test_frame();
    assert(dino.mod == 20);
    dino_test_button(B_ENV); assert(!dino.wheel_owned);
    int attack = TSEL->p[P_ATK];
    fm1_in.enc_steps[panel.enc[EN_K1]] = 1; dino_test_frame();
    assert(TSEL->p[P_ATK] == attack + 1 && dino.mod == 20);
    dino_test_button(B_HOME); assert(dino.screen == DINO_SCREEN_HOME && dino.wheel_owned);
    dino_test_button(B_SEQ); assert(dino.screen == DINO_SCREEN_PERFORMANCE && !dino.wheel_owned);
    fm1_in.enc_steps[panel.enc[EN_K1]] = 1; dino_test_frame(); assert(dino.mode == DINO_LAYER);
    dino_test_button(B_REC); assert(dino.focus == 1 && song.sel == DINO_B);
    preview("performance");
    dino_test_button(B_HOME);
    dino_test_button(B_SCL); assert(dino.screen == DINO_SCREEN_PRESETS && dino.wheel_owned);
    uint32_t total, at = preset_pos(&total);
    fm1_in.enc_steps[panel.enc[EN_SELECT]] = 7;
    fm1_in.enc_steps[panel.enc[EN_PRESET]] = 5; dino_test_frame();
    assert(preset_pos(&total) == (at + 12u) % total && vis_style == 0);
    char listed[13], detail[24], actual[13];
    dino_preset_info(preset_pos(&total), listed, detail); dino_ui_sound_name(song.sel, actual);
    assert(!strcmp(listed, actual) && strstr(detail, "FACTORY") && strstr(detail, ENGINES[TSEL->eng_req]->name));
    preview("presets");
    dino_preset_info(total - 1u, listed, detail);
    assert(!strcmp(listed, "USER TEST") && strstr(detail, "USER") && strstr(detail, "FM6"));
    uint32_t kept_engine = TSEL->eng_req, kept_preset = TSEL->preset;
    dino_test_button(B_SCL);
    assert(dino.screen == DINO_SCREEN_HOME && TSEL->eng_req == kept_engine && TSEL->preset == kept_preset);
    dino_test_button(B_HOME); assert(vis_on);
    fm1_in.enc_steps[panel.enc[EN_SELECT]] = 12; dino_test_frame(); assert(vis_style == 12);
    fm1_in.buttons = 1u << panel.btn[B_ARP]; fm1_in.enc_steps[panel.enc[EN_K2]] = 4;
    dino_test_frame(); assert(dino.bend == 1024);
    fm1_in.buttons = 0; dino_test_frame(); assert(!dino.bend);
    dino_test_button(B_SAVE); assert(dino.screen == DINO_SCREEN_SAVE && !dino.wheel_owned);
    preview("save");
    assert(!test_saved);
    edges_btn = fm1_in.buttons = 1u << panel.btn[B_SAVE]; dino_test_frame();
    fm1_ms += 1100; dino_test_frame(); assert(test_saved == 1);
    fm1_in.buttons = 0; dino_test_frame();
    dino_save_kind = DINO_SAVE_PATCH; /* BANK_FULL can only result from saving an FM6 patch. */
    test_save_status = DINO_SAVE_BANK_FULL;
    fm1_in.enc_steps[panel.enc[EN_K1]] = 3; dino_test_frame(); assert(dino_replace_bank == 3);
    preview("bank-full");
    edges_btn = fm1_in.buttons = 1u << panel.btn[B_SAVE]; dino_test_frame();
    fm1_ms += 1100; dino_test_frame(); assert(test_saved == 2);
    fm1_in.buttons = 0; dino_test_frame();
    dino_test_button(B_HOME);
    for (uint32_t e = 0; e < NENGINES; e++) {
        set_engine_of(TSEL, e);
        for (uint32_t p = 0; p < ENGINES[e]->npresets; p++) {
            apply_preset_to(TSEL, p);
            assert(!TSEL->p[P_DIST] && !TSEL->p[P_CHOR] && !TSEL->p[P_DLY] && !TSEL->p[P_REV]);
            assert(!TSEL->p[P_ITYPE] && !TSEL->p[P_SLCR] && !TSEL->p[P_AMODE] && !TSEL->p[P_CHORD]);
        }
        dino_test_button(B_EDIT);
        dino_test_frame();
        dino_test_button(B_HOME);
    }
    /* A restored calibration must update audio-owned encoders and the ARP gate immediately. */
    panel_t old = panel;
    panel.enc[EN_K1] = old.enc[EN_K3]; panel.enc[EN_K3] = old.enc[EN_K1];
    panel.enc[EN_K2] = old.enc[EN_K4]; panel.enc[EN_K4] = old.enc[EN_K2];
    panel.dir[EN_K1] = panel.dir[EN_K2] = -1;
    panel.btn[B_ARP] = old.btn[B_REC]; panel.btn[B_REC] = old.btn[B_ARP];
    panel.btn[B_OCTUP] = old.btn[B_OCTDN]; panel.btn[B_OCTDN] = old.btn[B_OCTUP];
    dino_panel_bind();
    assert(dino.mod_enc == old.enc[EN_K3] && dino.bend_enc == old.enc[EN_K4]);
    assert(dino.mod_dir == -1 && dino.bend_dir == -1 && dino.arp_button == (1u << old.btn[B_REC]));
    assert(dyn_bit[0] == (1u << old.btn[B_OCTUP]) && dyn_bit[1] == (1u << old.btn[B_OCTDN]));
    dino.mod = 64; dino.bend = 0;
    fm1_in.enc_steps[dino.mod_enc] = -3; fm1_in.enc_steps[dino.bend_enc] = -2;
    fm1_in.buttons = dino.arp_button; dino_test_frame();
    assert(dino.mod == 67 && dino.bend == 512);
    fm1_in.buttons = 0; dino_test_frame(); assert(!dino.bend);
    panel = old; dino_panel_bind();
    dino_mode_set(DINO_DRUMS); dino_test_midi(0x99, 38, 100); dino_test_frame();
    assert(dino.last_drum == 2);
    int32_t tune = drum_tune[2], decay = drum_dec[2];
    fm1_in.enc_steps[panel.enc[EN_K3]] = 2; fm1_in.enc_steps[panel.enc[EN_K4]] = -3;
    dino_test_frame(); assert(drum_tune[2] == tune + 2 && drum_dec[2] == decay - 3);
    dino_visual_active = 0; dino_home(); dino_test_frame(); preview("drums");
    puts("DINO contextual knobs, calibration, visuals audition, SAVE confirmation and every dry factory patch: OK");
    return 0;
}
