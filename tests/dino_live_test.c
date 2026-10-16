/* SPDX-License-Identifier: GPL-3.0-only */
#include "dino_audio_host.h"
int main(void)
{
    dino_test_init();
    trk[DINO_A].p[P_TRANS] = -24; trk[DINO_B].p[P_TRANS] = 12;
    dino.mode = DINO_LAYER;
    fm1_in.notes = 1u << 7; events_block(CTL); /* C4 -> C2 and C5 */
    assert(dino_refs[DINO_A][36] == 1 && dino_refs[DINO_B][72] == 1);
    dino.mode = DINO_SPLIT; dino.split = 70;
    trk[DINO_A].p[P_TRANS] = 0; dino_focus_set(1);
    fm1_in.notes = 0; events_block(CTL);
    assert(!dino_refs[DINO_A][36] && !dino_refs[DINO_B][72]);
    dino_test_init(); dino.mode = DINO_SPLIT;
    dino_test_midi(0x90, 59, 90); dino_test_midi(0x90, 60, 90);
    assert(dino_refs[DINO_A][59] && dino_refs[DINO_B][60]);
    dino.split = 40;
    dino_test_midi(0x80, 59, 0); dino_test_midi(0x90, 60, 0);
    assert(!dino_refs[DINO_A][59] && !dino_refs[DINO_B][60]);
    /* Retriggering a held MIDI note replaces its old route, including a split edit. */
    dino_test_init(); dino_test_midi(0x90, 60, 90); dino_focus_set(1);
    dino_test_midi(0x90, 60, 100);
    assert(!dino_refs[DINO_A][60] && dino_refs[DINO_B][60] == 1);
    dino.mode = DINO_SPLIT; dino.split = 70; dino_test_midi(0x90, 60, 100);
    assert(dino_refs[DINO_A][60] == 1 && !dino_refs[DINO_B][60]);
    dino_test_midi(0x80, 60, 0); assert(!dino_refs[DINO_A][60]);
    /* Actual USB and TRS parsers preserve separate owners on the same channel. */
    dino_test_init(); dino_test_midi(0x90, 60, 90); dino_focus_set(1);
    dino_test_trs(0x90, 60, 100);
    assert(dino_refs[DINO_A][60] == 1 && dino_refs[DINO_B][60] == 1);
    dino_test_midi(0x80, 60, 0);
    assert(!dino_refs[DINO_A][60] && dino_refs[DINO_B][60] == 1);
    dino_test_trs(0x80, 60, 0); assert(!dino_refs[DINO_B][60]);
    dino_test_init();
    fm1_in.notes = 1u << 7; events_block(CTL);
    dino_test_midi(0x90, 60, 90);
    assert(dino_refs[DINO_A][60] == 2);
    fm1_in.notes = 0; events_block(CTL);
    assert(dino_refs[DINO_A][60] == 1);
    dino_test_midi(0xB0, 64, 127); dino_test_midi(0x80, 60, 0);
    assert(sus_wait[DINO_A][1] & (1u << 28));
    dino_focus_set(1); dino_test_midi(0xB0, 64, 0);
    assert(!sus_wait[DINO_A][1]);
    dino_test_midi(0xB0, 64, 127); dino_test_trs(0xB0, 64, 127);
    song.g[G_CH1] = 2;
    dino_test_midi(0xB0, 64, 0); assert(sus_on[DINO_A]);
    dino_test_trs(0xB0, 64, 0); assert(!sus_on[DINO_A] && !sus_on[DINO_B]);
    dino_test_init(); dino_test_midi(0xB0, 64, 127);
    panic_req = 1u << DINO_A; events_block(CTL);
    assert(sus_on[DINO_A] && sus_on[DINO_B]);
    dino_test_midi(0x90, 60, 90); dino_test_midi(0x80, 60, 0);
    assert(sus_wait[DINO_A][1] & (1u << 28));
    dino_test_midi(0xB0, 64, 0); assert(!sus_wait[DINO_A][1]);
    dino_test_init();
    fm1_in.enc_steps[0] = 50; events_block(CTL); assert(dino.mod == 50 && !fm1_in.enc_steps[0]);
    dino_test_midi(0xB0, 1, 127); assert(dino.mod == 127);
    dino_test_midi(0xE0, 0, 96); assert(dino.bend == 4096);
    fm1_in.enc_steps[1] = 5; events_block(CTL); assert(dino.bend == 4096); /* not held */
    fm1_in.buttons = dino.arp_button; fm1_in.enc_steps[1] = 2; events_block(CTL);
    assert(dino.bend == 4608 && trk[DINO_A].bend == 4608 && trk[DINO_B].bend == 4608);
    fm1_in.buttons = 0; events_block(CTL); assert(!dino.bend);
    fm1_in.buttons = dino.arp_button; events_block(CTL);
    dino_test_midi(0xE0, 0, 96); fm1_in.buttons = 0; events_block(CTL);
    assert(dino.bend == 4096); /* a newer MIDI wheel owns the shared bend */
    dino.wheel_owned = 0; fm1_in.enc_steps[0] = 3; events_block(CTL);
    assert(fm1_in.enc_steps[0] == 3 && dino.mod == 127);
    dino_test_midi(0xFA, 0, 0); assert(!song.playing && !transport_req);
    trk[0].p[P_AMODE] = 4; trk[0].p[P_CHORD] = 1; trk[0].p[P_AHOLD] = 1;
    events_block(CTL); assert(!trk[0].p[P_AMODE] && !trk[0].p[P_CHORD] && !trk[0].p[P_AHOLD]);
    dino_test_init(); dino.mode = DINO_DRUMS; dino_focus_set(0);
    dino_test_midi(0x99, 36, 100); assert(drums.v[0].active);
    dino_test_midi(0x90, 60, 100); assert(!dino_refs[DINO_A][60]);
    dino_test_init(); usb.config = 1; mo_r = mo_w = 0;
    fm1_in.notes = 1u << 7; events_block(CTL); assert(mo_w == 1);
    song.g[G_CH1] = 2; fm1_in.notes = 0; events_block(CTL);
    assert(mo_w == 2 && ((midi_out_q[1] >> 8) & 255u) == 0x80u);
    for (uint32_t pr = 1u << DINO_A; pr <= (1u << NTRK) - 1u; pr += 14u) {
        dino_test_init(); usb.config = 1; mo_r = mo_w = 0;
        fm1_in.notes = 1u << 7; events_block(CTL);
        panic_req = pr; events_block(CTL); fm1_in.notes = 0; events_block(CTL);
        assert(mo_w == 2 && ((midi_out_q[1] >> 8) & 255u) == 0x80u);
    }
    puts("DINO direct notes, ownership, sustain, controllers and disabled sequencer: OK");
    return 0;
}
