/* SPDX-License-Identifier: GPL-3.0-only */
/* DINO protocol v13: direct performance and asynchronous SAVE. Included by editor.c. */
#if FELUCCA_DINO
#define ED_DINO 78u
static int ed_dino_retired(uint32_t scope, uint32_t id)
{
    if (!scope) return (id >= P_AMODE && id <= P_QUANT) || (id >= P_SLEN && id <= P_SGATE) ||
                       id == P_CHORD || id == P_STRUM || id == P_VLEAD;
    return id == G_SWING || id == G_CLOCK || (id >= G_SLOT && id <= G_SAVE) || id == G_ENGGO ||
           id == G_CLRSEQ || id == G_ROLL || id == G_NEWPRJ || id == G_CLKOUT;
}
static void ed_dino(const uint8_t *a, uint32_t n)
{
    uint32_t op = n ? a[0] : 0u, rc = 0, i;
    switch (op) {
    case 0: break;
    case 1:
        if (n < 5u || a[1] > DINO_DRUMS || a[2] > 127u || a[3] > 1u || a[4] >= DINO_VIS_N || (n > 5u && a[5] > 6u)) rc = 1;
        else {
            dino_mode_set(a[1]); dino.split = a[2]; dino_focus_set(a[3]); vis_style = a[4];
            if (n > 5u) song.octave = (int8_t)a[5] - 3;
            ui.force = 1;
        }
        break;
    case 2: rc = n != 2u ? 1u : dino_performance_load(a[1]); break;
    case 3: rc = n != 3u ? 1u : dino_save_request(a[1], a[2]); break;
    case 4: dino_save_cancel(); break;
    case 5:
        if (n < 4u || a[n - 1u]) rc = 1;
        else rc = dino_performance_rename(a[1], a + 2, n - 3u);
        break;
    case 6: rc = n != 2u ? 1u : dino_save_replace_bank(a[1]); break;
    default: rc = 1; break;
    }
    ed_b(op); ed_b(rc); ed_b(dino.mode); ed_b(dino.split); ed_b(dino.focus);
    ed_b((uint32_t)(song.octave + 3)); ed_b(vis_style); ed_b(dino_save_status());
    for (i = 0; i < 4u; i++) {
        const dino_performance_t *p = (const dino_performance_t *)&proj_slot[i];
        ed_b(dino_store_valid(p, sizeof *p, DINO_PERF_MAGIC) ? 1u : proj_ok(&proj_slot[i]) ? 2u : 0u);
        ed_str(dino_performance_label(i), 12);
    }
}
#endif
