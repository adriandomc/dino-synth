/* SPDX-License-Identifier: GPL-3.0-only */
#include "dino_audio_host.h"
static uint16_t screen[240 * 240];
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    assert(x + w <= 240 && y + h <= 240);
    for (uint32_t j = 0; j < h; j++) for (uint32_t i = 0; i < w; i++) screen[(y + j) * 240 + x + i] = p[j * w + i];
}
#include "../firmware/src/gfx.c"
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    assert(x + w <= 240 && y + h <= 240);
    for (uint32_t j = 0; j < h; j++) for (uint32_t i = 0; i < w; i++) screen[(y + j) * 240 + x + i] = swap16(c);
}
static uint32_t fm1_ticks(void) { return fm1_ms * 24000u; }
#define FM1_TICKS_PER_US 24u
static int32_t fm1_enc_take(uint32_t e) { return dino_enc_take(e); }
static uint8_t fm1_led[16], fm1_led_dim[16], fm1_led_bg[16];
static volatile uint16_t fm1_led_bg_ns;
#define FM1_NCOL 16u
static const int8_t FM1_KEYMAP[5][16];
static void fm1_led_key(uint32_t id, int on) { (void)id; (void)on; }
static uint32_t edges_btn;
static uint32_t fm1_input_edges(int unused) { (void)unused; uint32_t e = edges_btn; edges_btn = 0; return e; }
static uint32_t fm1_input_note_edges(void) { return 0; }
static void fm1_wdt_feed(void) { fm1_ms++; }
static int32_t fm1_adc_read(int c) { (void)c; return -1; }
static struct { uint32_t magic, stage, page, home, ui_frames; } felucca_dbg;
#define FELUCCA_ICONS 1
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N], scope_bufr[SCOPE_N];
static uint32_t scope_w;
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
static int project_used(uint32_t i) { (void)i; return 0; }
static void project_save(uint32_t i) { (void)i; }
static void project_load(uint32_t i) { (void)i; }
static void project_erase(uint32_t i) { (void)i; }
static int up_used(uint32_t k) { return k == 0; }
static int up_load(uint32_t k) { (void)k; return 0; }
static int up_load_to(track_t *t, uint32_t k) { (void)t; (void)k; return 0; }
static uint32_t up_count(void) { return 1; }
static uint32_t up_nth(uint32_t n) { (void)n; return 0; }
static uint32_t up_rank(uint32_t s) { (void)s; return 0; }
static void up_name(uint32_t k, char *b) { (void)k; str_cpy(b, "USER TEST", 13); }
static void up_slot_label(char *b, uint32_t k) { fmt_int(b, k + 1); }
static void up_ui(uint32_t op, uint32_t k) { (void)op; (void)k; }
static void settings_save(void) {}
#include "../firmware/src/ui_song.c"
#include "../firmware/src/ui_studio.c"
#include "../firmware/src/icons.c"
#include "../firmware/src/ui_draw.c"
#include "../firmware/src/ui_vis.c"
#include "../firmware/src/ui_layers.c"
#include "../firmware/src/ui_menu.c"
#include "../firmware/src/ui_input.c"
static uint32_t test_saved, test_save_status;
static uint32_t dino_save_request(uint32_t kind, uint32_t slot) { (void)kind; (void)slot; test_saved++; return test_save_status = DINO_SAVE_PENDING; }
static void dino_save_cancel(void) { test_save_status = 0; }
static uint32_t dino_save_status(void) { return test_save_status; }
static const char *dino_performance_label(uint32_t slot) { return slot ? "EMPTY" : "LEGACY SONG"; }
static uint32_t dino_performance_load(uint32_t slot) { (void)slot; return 0; }
static uint32_t dino_save_replace_bank(uint32_t bank) { (void)bank; test_saved++; return test_save_status = DINO_SAVE_PENDING; }
static uint32_t dino_patch_engine(uint32_t slot) { (void)slot; return ENGI_FM6; }
#include "../firmware/src/ui_dino.c"
static int st_read(uint32_t off, void *dst, uint32_t n) { (void)off; (void)dst; (void)n; return -1; }
static int st_erase(uint32_t off) { (void)off; return -1; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { (void)off; (void)src; (void)n; return -1; }
#include "../firmware/src/storage.c"
static void dino_test_frame(void)
{
    ui_input();
    events_block(CTL);
    ui_draw();
    fm1_ms += 16;
}
static void dino_test_button(uint32_t label)
{
    edges_btn = fm1_in.buttons = 1u << panel.btn[label]; dino_test_frame();
    fm1_in.buttons = 0; dino_test_frame();
}
