/* SPDX-License-Identifier: GPL-3.0-only */
/* Included after seq.c: direct notes reuse the proven voice, pedal and MIDI paths. */
static uint8_t dino_key_mask[27], dino_key_note[27][2], dino_key_channel[27][2];
static uint8_t dino_refs[NTRK][128];
static uint32_t dino_keys;
static uint8_t dino_arp_was;
/* USB and TRS can hold the same channel/note independently. USB reuses the old
 * compact owner table; TRS has the same four-bit destination mask per note. */
static uint8_t dino_trs_on[16][64];
static uint32_t dino_pedals;
static uint8_t dino_bend_owner = 255u;

static uint32_t dino_dest(uint32_t note)
{
    if (dino.mode == DINO_DRUMS) return 1u << TRK_DRUM;
    if (dino.mode == DINO_LAYER) return (1u << DINO_A) | (1u << DINO_B);
    if (dino.mode == DINO_SPLIT) return 1u << (note < dino.split ? DINO_A : DINO_B);
    return 1u << (dino.focus ? DINO_B : DINO_A);
}
static void dino_focus_set(uint32_t focus)
{
    dino.focus = (uint8_t)(focus != 0);
    song.sel = (uint8_t)(dino.mode == DINO_DRUMS ? TRK_DRUM : dino.focus ? DINO_B : DINO_A);
}
static void dino_mode_set(uint32_t mode)
{
    mode = mode <= DINO_DRUMS ? mode : DINO_SINGLE;
    if ((mode == DINO_DRUMS) != (dino.mode == DINO_DRUMS))
        panic_req = (1u << NTRK) - 1u;
    dino.mode = (uint8_t)mode;
    dino_focus_set(dino.focus);
}
static void dino_controllers_reset(void)
{
    dino.mod = 0;
    dino.bend = 0;
    dino_bend_owner = 255u;
}
static int32_t dino_enc_take(uint32_t index)
{
    int32_t value;
#ifndef DINO_HOST
    uint32_t irq = fm1_icfg();
    fm1_irq_off();
#endif
    value = fm1_in.enc_steps[index];
    fm1_in.enc_steps[index] = 0;
#ifndef DINO_HOST
    fm1_irq_restore(irq);
#endif
    return value;
}
static void dino_wheel_block(uint32_t n)
{
    uint32_t arp = (fm1_in.buttons & dino.arp_button) != 0;
    if (dino.wheel_owned) {
        int32_t m = dino_enc_take(dino.mod_enc) * dino.mod_dir;
        int32_t b = dino_enc_take(dino.bend_enc) * dino.bend_dir;
        if (dino.mode != DINO_DRUMS) {
            dino.mod = (uint8_t)clamp((int32_t)dino.mod + m, 0, 127);
            if (arp && b) {
                dino.bend = (int16_t)clamp(dino.bend + b * 256, -8192, 8191);
                dino_bend_owner = 255u;
            }
        }
    }
    if (dino_arp_was && !arp && dino_bend_owner == 255u) dino.bend = 0;
    dino_arp_was = (uint8_t)arp;
    dino.mod_s += ((int32_t)dino.mod * 256 - dino.mod_s) / 4;
    if (!dino.mod && dino.mod_s < 4) dino.mod_s = 0;
    dino.mod_phase += n * (uint32_t)(((uint64_t)5u << 32) / FS);
    dino.vibrato = mulq15(osc_sine(dino.mod_phase), dino.mod_s * 2048 / (127 * 256));
    trk[DINO_A].bend = trk[DINO_B].bend = dino.bend;
}
static void dino_on(uint32_t part, uint32_t note, uint32_t vel)
{
    if (part == TRK_DRUM) {
        dino.last_drum = (uint8_t)midi_drum_lane(note);
        trk_note_on(TDRUM, LANE_NOTE[dino.last_drum], vel);
        return;
    }
    if (dino_refs[part][note] < 255u && !dino_refs[part][note]++)
        input_on(&trk[part], note, vel);
}
static void dino_off(uint32_t part, uint32_t note)
{
    if (part != TRK_DRUM && dino_refs[part][note] && !--dino_refs[part][note])
        input_off(&trk[part], note);
}
static void dino_key_out_off(uint32_t key, uint32_t mask)
{
    if (mask & (1u << TRK_DRUM))
        midi_out_note(dino_key_channel[key][0], dino_key_note[key][0], 0);
    for (uint32_t j = 0; j < 2u; j++)
        if (mask & (1u << (j ? DINO_B : DINO_A)))
            midi_out_note(dino_key_channel[key][j], dino_key_note[key][j], 0);
}
static void dino_keyboard(void)
{
    uint32_t current = fm1_in.notes, change = current ^ dino_keys, k, j;
    for (k = 0; k < 27u; k++) {
        if (!((change >> k) & 1u)) continue;
        if ((current >> k) & 1u) {
            uint32_t raw = (uint32_t)clamp(53 + (int32_t)k + song.octave * 12, 0, 127);
            uint32_t mask = dino_dest(raw);
            dino_key_mask[k] = (uint8_t)mask;
            if (mask & (1u << TRK_DRUM)) {
                dino.last_drum = (uint8_t)lane_of_key(k);
                uint32_t note = LANE_NOTE[dino.last_drum];
                dino_key_note[k][0] = (uint8_t)note;
                dino_key_channel[k][0] = (uint8_t)trk_midi_ch(TRK_DRUM);
                trk_note_on(TDRUM, note, lvl_vel(key_lvl(), 100));
                midi_out_note(dino_key_channel[k][0], note, 100);
            } else for (j = 0; j < 2u; j++) {
                uint32_t part = j ? DINO_B : DINO_A;
                if (mask & (1u << part)) {
                    uint32_t note = (uint32_t)clamp((int32_t)raw + trk[part].p[P_TRANS], 0, 127);
                    dino_key_note[k][j] = (uint8_t)note;
                    dino_key_channel[k][j] = (uint8_t)trk_midi_ch(part);
                    dino_on(part, note, 100);
                    midi_out_note(dino_key_channel[k][j], note, 100);
                }
            }
        } else {
            uint32_t mask = dino_key_mask[k];
            dino_key_out_off(k, mask);
            for (j = 0; j < 2u; j++) {
                uint32_t part = j ? DINO_B : DINO_A;
                if (mask & (1u << part)) {
                    dino_off(part, dino_key_note[k][j]);
                }
            }
            dino_key_mask[k] = 0;
        }
    }
    dino_keys = current;
}
static int dino_midi_channel(uint32_t ch)
{
    if (song.g[G_ROUTE] == 1) return 0;
    if (song.g[G_ROUTE] >= 2) return ch == (uint32_t)song.g[G_ROUTE] - 2u;
    return ch + 1u == (uint32_t)(dino.mode == DINO_DRUMS ? song.g[G_DRCH] : song.g[G_CH1]);
}
static uint8_t *dino_midi_owner(uint32_t source, uint32_t ch, uint32_t note)
{
    return source ? &dino_trs_on[ch][note >> 1] : &midi_sel_on[ch][note >> 1];
}
static void dino_midi_release(uint32_t source, uint32_t ch, uint32_t note)
{
    uint8_t *owner = dino_midi_owner(source, ch, note);
    uint32_t shift = 4u * (note & 1u), mask = (*owner >> shift) & 15u, i;
    for (i = 0; i < NTRK; i++)
        if (mask & (1u << i)) dino_off(i, note);
    *owner &= (uint8_t)~(15u << shift);
}
static void dino_pedal_set(uint32_t owner, uint32_t on)
{
    if (on) dino_pedals |= 1u << owner;
    else dino_pedals &= ~(1u << owner);
    sus_set(&trk[DINO_A], dino_pedals != 0);
    sus_set(&trk[DINO_B], dino_pedals != 0);
}
static void dino_events_block(uint32_t n)
{
    uint32_t i, pr = panic_req;
    panic_req = 0;
    song.playing = song.rec = song.seq_mode = 0;
    transport_req = rec_wait = ci_on = ft_on = 0;
    if (pr) {
        for (i = 0; i < NTRK; i++) if ((pr >> i) & 1u) {
            track_t *t = &trk[i];
            if (!t->pold) trk_all_off(t);
            t->xp_n = 0;
            t->nheld = t->arp_phys = t->nmono = t->mono_note = 0;
            sus_on[i] = (uint8_t)(pr != (1u << NTRK) - 1u && dino_pedals && (i == DINO_A || i == DINO_B));
            memset(sus_wait[i], 0, sizeof sus_wait[i]);
            memset(dino_refs[i], 0, sizeof dino_refs[i]);
        }
        for (uint32_t c = 0; c < 16u; c++) for (uint32_t b = 0; b < 64u; b++) {
            midi_sel_on[c][b] &= (uint8_t)~(pr | (pr << 4));
            dino_trs_on[c][b] &= (uint8_t)~(pr | (pr << 4));
        }
        for (uint32_t k = 0; k < 27u; k++) {
            dino_key_out_off(k, dino_key_mask[k] & pr);
            dino_key_mask[k] &= (uint8_t)~pr;
        }
        if (pr == (1u << NTRK) - 1u) {
            dino_controllers_reset();
            dino_pedals = 0;
            seq_out_all_off();
        }
    }
    for (i = 0; i < NPART; i++) {
        dino_track_direct(&trk[i]);
        engine_block(&trk[i]);
    }
    dino_keyboard();
    drum_audition_poll();
    while (mi_r != mi_w) {
        uint32_t pkt = midi_in_q[mi_r++ % MQ], st = (pkt >> 8) & 0xF0u, ch = (pkt >> 8) & 15u;
        uint32_t d1 = (pkt >> 16) & 127u, d2 = (pkt >> 24) & 127u;
        uint32_t source = ((pkt >> 4) & 15u) != 0, owner = source * 16u + ch;
        if ((pkt & 15u) == 15u) {
            if (((pkt >> 8) & 255u) == 0xF8u)
                mclk_event(0xF8u, ((pkt >> 4) & 15u) ? 2u : 1u);
            continue;
        }
        if (st == 0x80u || (st == 0x90u && !d2)) {
            dino_midi_release(source, ch, d1);
            continue;
        }
        /* Release accepted controllers after changing the receive channel too. */
        if (st == 0xB0u && d1 == 64u && d2 < 64u && ((dino_pedals >> owner) & 1u)) {
            dino_pedal_set(owner, 0);
            continue;
        }
        if (st == 0xE0u && d1 == 0u && d2 == 64u && dino_bend_owner == owner) {
            dino.bend = 0;
            dino_bend_owner = 255u;
            continue;
        }
        if (!dino_midi_channel(ch)) continue;
        if (st == 0x90u) {
            uint32_t mask = dino_dest(d1);
            uint8_t *entry = dino_midi_owner(source, ch, d1);
            dino_midi_release(source, ch, d1);
            *entry |= (uint8_t)(mask << (4u * (d1 & 1u)));
            for (i = 0; i < NTRK; i++) if ((mask >> i) & 1u) dino_on(i, d1, d2);
        } else if (st == 0xE0u) {
            dino.bend = (int16_t)((int32_t)(d1 | d2 << 7) - 8192);
            dino_bend_owner = (uint8_t)owner;
        } else if (st == 0xB0u) {
            if (d1 == 1u) dino.mod = (uint8_t)d2;
            else if (d1 == 64u) {
                dino_pedal_set(owner, d2 >= 64u);
            } else if (d1 == 120u || d1 == 123u) panic_req = (1u << NTRK) - 1u;
            else if (d1 == 121u) {
                dino_controllers_reset();
                dino_pedals = 0;
                sus_set(&trk[DINO_A], 0);
                sus_set(&trk[DINO_B], 0);
            } else midi_cc(TSEL, d1, d2);
        } else if (st == 0xC0u) pc_req[song.sel] = (uint8_t)(d1 + 1u);
    }
    dino_wheel_block(n);
    clk_pos += n * (uint32_t)clamp(song.g[G_BPM], 40, 240);
    while (clk_pos >= BEAT_U) { clk_pos -= BEAT_U; clk_beat++; }
}
