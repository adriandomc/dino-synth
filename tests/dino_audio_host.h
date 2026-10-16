/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define FELUCCA_DINO 1
#define FELUCCA_ARRANGER 0
#define DINO_HOST 1
#define __attribute__(x)
#define memset felucca_memset
#define memcpy felucca_memcpy
#define memcmp felucca_memcmp
#include "felucca_tables.h"
#include "../firmware/src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
static struct { volatile uint32_t notes, buttons; int16_t enc_steps[7]; } fm1_in;
static volatile uint32_t fm1_remote_btn;
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
#include "../firmware/src/core.h"
#include "../firmware/src/dino.h"
#include "../firmware/src/cpu1.h"
#include "cpu1_host.c"
#include "../firmware/src/engines.c"
#include "../firmware/src/drums.c"
#include "../firmware/src/params.c"
#ifdef DINO_DSP_REFERENCE
/* Compile the unchanged upstream voice render for a bit-exact MOD=0 comparison. */
#undef FELUCCA_DINO
#define FELUCCA_DINO 0
#endif
#include "../firmware/src/voice.c"
#ifdef DINO_DSP_REFERENCE
#undef FELUCCA_DINO
#define FELUCCA_DINO 1
#endif
#include "../firmware/src/slicer.c"
#include "../firmware/src/fx.c"
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "../firmware/src/usb.c"
#include "../firmware/src/midi_uart.c"
#include "../firmware/src/arranger.c"
#include "../firmware/src/seq.c"
static void dino_test_init(void)
{
    memset(trk, 0, sizeof trk); memset(&song, 0, sizeof song); memset(&dino, 0, sizeof dino);
    memset(&fm1_in, 0, sizeof fm1_in); memset(dino_refs, 0, sizeof dino_refs);
    memset(dino_key_mask, 0, sizeof dino_key_mask); memset(midi_sel_on, 0, sizeof midi_sel_on);
    memset(dino_trs_on, 0, sizeof dino_trs_on); dino_pedals = 0; dino_bend_owner = 255u;
    memset(sus_on, 0, sizeof sus_on); memset(sus_wait, 0, sizeof sus_wait);
    dino_keys = 0; dino_arp_was = 0; mi_r = mi_w = 0; panic_req = 0;
    dino.split = 60; dino.mod_enc = 0; dino.bend_enc = 1; dino.mod_dir = dino.bend_dir = 1;
    dino.arp_button = 1u; dino.wheel_owned = 1;
    for (uint32_t i = 0; i < G_COUNT; i++) song.g[i] = GP[i].def;
    for (uint32_t p = 0; p < NTRK; p++) {
        for (uint32_t i = 0; i < P_E0; i++) trk[p].p[i] = TP[i].def;
        for (uint32_t i = 0; i < 8u; i++) trk[p].p[P_E0 + i] = ENGINES[0]->edit[i].def;
        trk[p].p[P_SUS] = 127; trk[p].p[P_REL] = 0;
        steps_clear(&trk[p]);
    }
    song.master_q12 = 2048;
    fm6_init();
}
static void dino_test_midi(uint32_t status, uint32_t a, uint32_t b)
{
    midi_in_event((status >> 4) | status << 8 | a << 16 | b << 24);
    events_block(CTL);
}
static void dino_test_trs(uint32_t status, uint32_t a, uint32_t b)
{
    um_byte(status); um_byte(a); um_byte(b);
    events_block(CTL);
}
