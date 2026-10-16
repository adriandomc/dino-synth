/* SPDX-License-Identifier: GPL-3.0-only */
/* Real DSP + real LCD renderer; minimal page state keeps this test independent of performance UI controls. */
#define FELUCCA_DINO 0
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#undef FELUCCA_DINO
#define FELUCCA_DINO 1
#define DINO_VIS_TEST 1

static uint16_t screen[240u * 240u];
static uint32_t blits;
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    uint32_t i, j;
    assert(x + w <= 240u && y + h <= 240u);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) screen[(y + j) * 240u + x + i] = p[j * w + i];
    blits++;
}
#include "../firmware/src/gfx.c"
static uint32_t st_crc32(const void *p, uint32_t n) { (void)p; (void)n; assert(!"theme storage is outside this visual test"); return 0; }
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    assert(x + w <= 240u && y + h <= 240u);
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) screen[(y + j) * 240u + x + i] = swap16(c);
}
static struct { uint32_t home, menu, force, frame, msg_t; char msg[64]; } ui;
static const struct { uint32_t scope; } page = {SC_TRK};
#define cur_page() (&page)
static uint8_t vis_on, vis_shown_last, vis_name_t, vis_style, settings_later;
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N], scope_bufr[SCOPE_N];
static uint32_t scope_w;
static uint32_t trk_level(uint32_t c) { return (uint32_t)(c == TRK_DRUM ? song.g[G_DRLVL] : trk[c].p[P_LEVEL]) & 127u; }
#include "../firmware/src/ui_vis.c"

static void ppm(const char *dir, uint32_t style, uint32_t frame)
{
    uint32_t i;
    char name[512];
    FILE *f;
    assert(snprintf(name, sizeof name, "%s/cosmic-%u-%02u.ppm", dir, style, frame) < (int)sizeof name);
    f = fopen(name, "wb"); assert(f);
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t p = swap16(screen[i]);
        uint8_t rgb[] = {(p >> 11) * 255 / 31, (p >> 5 & 63) * 255 / 63, (p & 31) * 255 / 31};
        assert(fwrite(rgb, 1, sizeof rgb, f) == sizeof rgb);
    }
    assert(!fclose(f));
}
static uint64_t pixels(void)
{
    uint64_t h = 1469598103934665603ull;
    uint32_t i;
    for (i = 0; i < 240u * 240u; i++) h = (h ^ screen[i]) * 1099511628211ull;
    return h;
}
static void draw(void) { vis_draw(); ui.force = 0; }
static void t_schedule(void)
{
    uint32_t before;
    fm1_ms = 1000; vis_style = 12; vis_open(); draw(); before = blits;
    fm1_ms += 99; draw(); assert(blits == before);
    fm1_ms++; draw(); assert(blits == before + 2u); before = blits;
    song.cpu_q8 = 192;
    fm1_ms += 100; draw(); assert(blits == before);
    fm1_ms += 100; draw(); assert(blits == before + 2u);
    song.cpu_q8 = 0;
    fm1_ms = UINT32_MAX - 30u; ui.force = 1; draw(); before = blits;
    fm1_ms = 50; draw(); assert(blits == before);
    fm1_ms = 70; draw(); assert(blits == before + 2u);
    vis_style = 15; vis_select(1); assert(vis_style == 0u);
    vis_select(-1); assert(vis_style == 15u);
    vis_style = 0; vis_select(12); assert(vis_style == 12u);
    vis_select(-29); assert(vis_style == 15u);
}
static void t_life(void)
{
    uint32_t x, y, generation, seed = rng_state;
    memset(&cosmic_life, 0, sizeof cosmic_life);
    for (y = 10; y < 12; y++) for (x = 10; x < 12; x++) cosmic_life.cell[y][x] = 128u;
    cosmic_life_step(); assert(cosmic_life.population == 4u);
    for (y = 10; y < 12; y++) for (x = 10; x < 12; x++) assert(cosmic_life.cell[y][x] & 128u);
    /* A fading dead neighbor must not participate in Conway's B3/S23 rule. */
    memset(&cosmic_life, 0, sizeof cosmic_life);
    cosmic_life.cell[9][10] = cosmic_life.cell[10][10] = cosmic_life.cell[11][10] = 128u;
    cosmic_life.cell[9][9] = 127u;
    cosmic_life_step(); assert(cosmic_life.population == 3u);
    assert((cosmic_life.cell[10][9] & cosmic_life.cell[10][10] & cosmic_life.cell[10][11]) & 128u);
    cosmic_life_step(); assert(cosmic_life.population == 3u);
    assert((cosmic_life.cell[9][10] & cosmic_life.cell[10][10] & cosmic_life.cell[11][10]) & 128u);
    /* The toroidal edge must behave exactly like an interior edge. */
    memset(&cosmic_life, 0, sizeof cosmic_life);
    cosmic_life.cell[31][0] = cosmic_life.cell[0][0] = cosmic_life.cell[1][0] = 128u;
    cosmic_life_step(); assert(cosmic_life.population == 3u);
    assert((cosmic_life.cell[0][31] & cosmic_life.cell[0][0] & cosmic_life.cell[0][1]) & 128u);
    /* A/B/drum hits queued between UI frames survive until the next generation. */
    for (uint32_t hit = 1u; hit <= 8u; hit <<= 1u) {
        if (hit == 2u) continue;
        memset(&cosmic_life, 0, sizeof cosmic_life);
        cosmic_life.ready = 1; cosmic_life.ms = cosmic_life.fed_ms = 1000;
        cosmic.heavy = 0; cosmic.energy = 0; cosmic.note_a = 48; cosmic.note_b = 72;
        cosmic_life_update(1050, hit); assert(!cosmic_life.generation && cosmic_life.hits == hit);
        cosmic_life_update(1100, 0); assert(cosmic_life.generation == 1u && !cosmic_life.hits);
        uint32_t live = 0;
        for (y = 0; y < 32; y++) for (x = 0; x < 32; x++) live += cosmic_life.cell[y][x] >> 7;
        assert(live == 5u);
    }
    cosmic.heavy = 1; generation = cosmic_life.generation;
    cosmic_life_update(1200, 0); assert(cosmic_life.generation == generation);
    cosmic_life_update(1300, 0); assert(cosmic_life.generation == generation + 1u);
    cosmic_life_update(1000000, 0); assert(cosmic_life.generation == generation + 2u);
    cosmic_life.ms = UINT32_MAX - 50u; generation = cosmic_life.generation;
    cosmic_life_update(149u, 0); assert(cosmic_life.generation == generation + 1u);
    assert(rng_state == seed);
    /* Even hard-panned B and drums drive energy, without needing a left-channel master peak. */
    memset(&vs, 0, sizeof vs); memset(&cosmic, 0, sizeof cosmic);
    vs.lvl[2] = 800; cosmic_update(1000, 4); assert(cosmic.energy == 800);
    vs.lvl[3] = 1000; cosmic_update(1100, 8); assert(cosmic.energy == 1000);
}
static void t_geometry(void)
{
    uint32_t style, pass, heavy, pitch, seed = rng_state;
    for (style = 0; style < 4u; style++)
        for (heavy = 0; heavy < 2u; heavy++)
            for (pitch = 0; pitch < 128u; pitch++) {
                uint64_t first;
                cosmic.ms = UINT32_MAX - pitch * 100u;
                cosmic.note_a = (uint8_t)pitch; cosmic.note_b = (uint8_t)(127u - pitch);
                cosmic.energy = cosmic.pulse = 1000; cosmic.heavy = (uint8_t)heavy;
                if (style == 3u) { cosmic_life.ready = 0; cosmic_life_update(cosmic.ms, 13); }
                uint32_t generation = cosmic_life.generation;
                uint8_t life_before[sizeof cosmic_life];
                memcpy(life_before, &cosmic_life, sizeof life_before);
                for (pass = 0; pass < 2u; pass++) {
                    cv_begin(240, 120, C_BLACK); cv_oy = -(int32_t)(pass * 120u);
                    cosmic_draw(style);
                    assert(cosmic_lines <= 120u && cosmic_points <= 224u);
                    cv_oy = 0; cv_blit(0, pass * 120u);
                }
                first = pixels();
                for (pass = 0; pass < 2u; pass++) {
                    cv_begin(240, 120, C_BLACK); cv_oy = -(int32_t)(pass * 120u);
                    cosmic_draw(style); cv_oy = 0; cv_blit(0, pass * 120u);
                }
                assert(first == pixels());
                assert(cosmic_life.generation == generation);
                assert(!memcmp(life_before, &cosmic_life, sizeof life_before));
            }
    assert(rng_state == seed);
}
static uint64_t audio_hash(int visuals)
{
    uint64_t h = 1469598103934665603ull;
    uint32_t block, i;
    int32_t o[CTL * 2u];
    for (block = 0; block < 1000u; block++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL * 2u; i++) h = (h ^ (uint32_t)o[i]) * 1099511628211ull;
        if (visuals && block % 40u == 0u) {
            for (i = 0; i < CTL; i += 2u) {
                scope_buf[scope_w & 511u] = vis_tap[i * 2u + 2u];
                scope_bufr[scope_w++ & 511u] = vis_tap[i * 2u + 3u];
            }
            fm1_ms += 100; ui.force = 1; vis_style = (uint8_t)(12u + block / 40u % 4u); draw();
        }
    }
    return h;
}
static void t_audio_unchanged(void)
{
    int fd[2], status;
    pid_t child;
    uint64_t reference, with_visuals;
    host_tracks_init(); host_preset(&trk[0], ENGI_NOISE, 0); trk_note_on(&trk[0], 60, 100);
    assert(!pipe(fd)); child = fork(); assert(child >= 0);
    if (!child) {
        uint64_t h;
        close(fd[0]); h = audio_hash(0);
        assert(write(fd[1], &h, sizeof h) == sizeof h); close(fd[1]); _exit(0);
    }
    close(fd[1]); with_visuals = audio_hash(1);
    assert(read(fd[0], &reference, sizeof reference) == sizeof reference); close(fd[0]);
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(reference == with_visuals);
}
static void bench(void)
{
    uint32_t style, frame, pass;
    for (style = 0; style < 4u; style++) {
        uint64_t t0 = now_ns();
        for (frame = 0; frame < 100u; frame++) {
            cosmic.ms += 100; cosmic.heavy = 0;
            if (style == 3u) cosmic_life_update(cosmic.ms, 5u);
            for (pass = 0; pass < 2u; pass++) {
                cv_begin(240, 120, C_BLACK); cv_oy = -(int32_t)(pass * 120u);
                cosmic_draw(style); cv_oy = 0; cv_blit(0, pass * 120u);
            }
        }
        printf("host LCD renderer %s: %.1f us/frame (SPI and audio preemption excluded)\n", VIS_NAME[12u + style],
               (now_ns() - t0) / 100000.0);
    }
}
static void previews(const char *dir)
{
    uint32_t style, frame, b, i;
    int32_t o[CTL * 2u];
    for (style = 0; style < 4u; style++) {
        memset(trk, 0, sizeof trk); memset(&drums, 0, sizeof drums);
        memset(dly_buf, 0, sizeof dly_buf); memset(cho_buf, 0, sizeof cho_buf);
        memset(rev_line, 0, sizeof rev_line); memset(rev_ap, 0, sizeof rev_ap); memset(&fx, 0, sizeof fx);
        host_tracks_init();
        host_preset(&trk[0], 0, 0); host_preset(&trk[2], ENGI_FM6, 0);
        for (i = 0; i < NPART; i++)
            trk[i].p[P_AMODE] = trk[i].p[P_DIST] = trk[i].p[P_CHOR] = trk[i].p[P_DLY] = trk[i].p[P_REV] = 0;
        vis_hit = vis_kick_hit = 0;
        memset(&cosmic, 0, sizeof cosmic); memset(&vs, 0, sizeof vs);
        memset(&cosmic_life, 0, sizeof cosmic_life);
        fm1_ms = 1000; vis_style = (uint8_t)(12u + style); vis_on = 1; vis_name_until = 0; ui.msg_t = 0;
        for (frame = 0; frame < 48u; frame++) {
            if (frame == 8u) trk_note_on(&trk[0], 48, 100);
            if (frame == 16u) { trk_note_off(&trk[0], 48); trk_note_on(&trk[2], 72, 100); }
            if (frame == 24u) trk_note_on(&trk[0], 55, 110);
            if (frame == 40u) { trk_note_off(&trk[0], 55); trk_note_off(&trk[2], 72); }
            for (b = 0; b < FS / 10u / CTL; b++) {
                mix_block(o, CTL);
                for (i = 1; i < CTL; i += 2u) {
                    scope_buf[scope_w & 511u] = vis_tap[i * 2u];
                    scope_bufr[scope_w++ & 511u] = vis_tap[i * 2u + 1u];
                }
            }
            song.cpu_q8 = frame >= 32u && frame < 40u ? 192u : 0u;
            fm1_ms += 100; ui.force = frame % 2u == 0u; draw(); ppm(dir, style, frame);
        }
    }
}
int main(int argc, char **argv)
{
    palette_set(2); host_tracks_init();
    t_schedule(); t_life(); t_geometry(); t_audio_unchanged(); bench();
    if (argc > 1) previews(argv[1]);
    puts("dino visuals: 16 styles, Conway rules/hits/trails, cadence/wrap, all pitches, bounded geometry, deterministic pixels, unchanged noise audio PASS");
    return 0;
}
