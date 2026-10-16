/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#define DINO_VIS_N 16u
#define DINO_A 0u
#define DINO_B 2u
enum { DINO_SINGLE, DINO_LAYER, DINO_SPLIT, DINO_DRUMS };
enum { DINO_SCREEN_HOME, DINO_SCREEN_PERFORMANCE, DINO_SCREEN_PRESETS, DINO_SCREEN_SAVE, DINO_SCREEN_EDIT,
       DINO_SCREEN_GLOBAL, DINO_SCREEN_DRUM_EDIT };
typedef struct {
    uint8_t mode, focus, split, screen;
    volatile uint8_t mod, wheel_owned;
    volatile int16_t bend;
    uint8_t mod_enc, bend_enc;
    int8_t mod_dir, bend_dir;
    uint32_t arp_button;
    int32_t mod_s, vibrato;
    uint32_t mod_phase;
    volatile uint8_t last_drum;
} dino_state_t;
static dino_state_t dino = {.split = 60};
static void dino_mode_set(uint32_t mode);
static void dino_focus_set(uint32_t focus);
static void dino_controllers_reset(void);
static void dino_init_live(void);
static void dino_events_block(uint32_t n);
static void dino_ui_input(void);
static int dino_ui_draw(void);
static void dino_panel_bind(void);
static volatile uint32_t dino_signal_ms;
static int dino_audio_quiet(void);
static void dino_store_boot(void);
static void dino_track_direct(track_t *t)
{
    t->p[P_AMODE] = t->p[P_AHOLD] = t->p[P_CHORD] = t->p[P_QUANT] = 0;
    t->p[P_STRUM] = t->p[P_VLEAD] = 0;
}
enum { DINO_SAVE_PATCH, DINO_SAVE_PERFORMANCE };
enum { DINO_SAVE_IDLE, DINO_SAVE_PENDING, DINO_SAVE_SAVED, DINO_SAVE_ERROR, DINO_SAVE_BANK_FULL };
static uint32_t dino_save_request(uint32_t kind, uint32_t slot);
static void dino_save_poll(void);
static void dino_save_cancel(void);
static uint32_t dino_save_status(void);
static const char *dino_performance_label(uint32_t slot);
static uint32_t dino_performance_load(uint32_t slot);
static uint32_t dino_patch_named(uint32_t slot, const char *name);
static uint32_t dino_patch_engine(uint32_t slot);
static uint32_t dino_save_replace_bank(uint32_t bank);
