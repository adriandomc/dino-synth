/* SPDX-License-Identifier: GPL-3.0-only */
/* Integer contours traced from the user's cosmic dog; the LCD canvas remains the only pixel buffer. */
#pragma once

static const int16_t DOG_TAIL[][2] = {
    {152, 157}, {160, 154}, {167, 150}, {172, 144}, {175, 138}, {177, 135}, {179, 136}, {181, 141}, {180, 148}, {178, 156}, {174, 164}, {169, 170}, {162, 177}, {156, 179}
};
_Static_assert(sizeof DOG_TAIL / sizeof DOG_TAIL[0] <= 64u, "dog contour bound");
static const int16_t DOG_BODY[][2] = {
    {85, 101}, {90, 90}, {99, 81}, {109, 77}, {119, 76}, {128, 77}, {137, 81}, {143, 87}, {150, 96}, {156, 104}, {159, 110}, {158, 115}, {155, 118}, {150, 120}, {148, 120}, {148, 127}, {150, 139}, {151, 151}, {154, 165}, {158, 177}, {162, 187}, {165, 198}, {165, 203}, {162, 206}, {159, 205}, {156, 201}, {152, 195}, {149, 190}, {145, 187}, {143, 187}, {141, 194}, {139, 200}, {138, 203}, {135, 203}, {133, 201}, {131, 194}, {130, 187}, {128, 185}, {125, 186}, {123, 191}, {121, 198}, {119, 202}, {116, 204}, {113, 203}, {111, 198}, {109, 192}, {106, 187}, {104, 186}, {101, 189}, {99, 193}, {97, 198}, {94, 201}, {90, 202}, {87, 201}, {85, 197}, {83, 192}, {84, 188}, {86, 182}, {89, 177}, {91, 170}, {92, 163}, {92, 158}, {90, 154}
};
_Static_assert(sizeof DOG_BODY / sizeof DOG_BODY[0] <= 64u, "dog contour bound");
static const int16_t DOG_CROWN[][2] = {
    {101, 65}, {107, 56}, {117, 47}, {126, 39}, {137, 33}, {147, 30}, {157, 28}, {166, 27}, {162, 32}, {156, 39}, {151, 47}, {149, 56}, {151, 65}, {152, 72}
};
_Static_assert(sizeof DOG_CROWN / sizeof DOG_CROWN[0] <= 64u, "dog contour bound");
static const int16_t DOG_BRIM[][2] = {
    {72, 70}, {78, 67}, {87, 66}, {97, 66}, {107, 67}, {117, 69}, {128, 71}, {140, 73}, {150, 75}, {160, 79}, {168, 84}, {175, 91}, {179, 98}, {180, 101}, {176, 103}, {169, 104}, {163, 103}, {157, 100}, {151, 94}, {147, 89}, {143, 83}, {139, 80}, {133, 77}, {126, 76}, {119, 75}, {111, 76}, {103, 77}, {97, 81}, {91, 85}, {87, 89}, {83, 90}, {78, 88}, {73, 85}, {70, 80}, {69, 76}, {70, 73}
};
_Static_assert(sizeof DOG_BRIM / sizeof DOG_BRIM[0] <= 64u, "dog contour bound");
static const int16_t DOG_MUZZLE[][2] = {
    {59, 125}, {61, 116}, {67, 109}, {74, 105}, {82, 104}, {90, 104}, {99, 106}, {106, 110}, {111, 116}, {113, 124}, {114, 132}, {112, 140}, {108, 147}, {102, 153}, {94, 157}, {85, 158}, {77, 156}, {69, 153}, {63, 147}, {60, 140}, {58, 133}
};
_Static_assert(sizeof DOG_MUZZLE / sizeof DOG_MUZZLE[0] <= 64u, "dog contour bound");
static const int16_t DOG_EYE_A[][2] = {
    {92, 95}, {93, 94}, {95, 95}, {95, 97}, {94, 98}, {92, 99}, {91, 97}, {91, 96}
};
_Static_assert(sizeof DOG_EYE_A / sizeof DOG_EYE_A[0] <= 64u, "dog contour bound");
static const int16_t DOG_EYE_B[][2] = {
    {118, 99}, {119, 98}, {121, 99}, {121, 102}, {120, 104}, {118, 104}, {117, 103}, {117, 101}
};
_Static_assert(sizeof DOG_EYE_B / sizeof DOG_EYE_B[0] <= 64u, "dog contour bound");
static const int16_t DOG_NOSE_A[][2] = {
    {67, 120}, {70, 117}, {73, 117}, {75, 119}, {76, 121}, {75, 124}, {73, 127}, {70, 127}, {67, 126}, {66, 123}
};
_Static_assert(sizeof DOG_NOSE_A / sizeof DOG_NOSE_A[0] <= 64u, "dog contour bound");
static const int16_t DOG_NOSE_A_INNER[][2] = {
    {70, 121}, {72, 120}, {74, 121}, {73, 124}, {71, 125}, {69, 124}
};
_Static_assert(sizeof DOG_NOSE_A_INNER / sizeof DOG_NOSE_A_INNER[0] <= 64u, "dog contour bound");
static const int16_t DOG_NOSE_B[][2] = {
    {91, 125}, {93, 124}, {96, 123}, {99, 124}, {100, 126}, {101, 129}, {99, 131}, {97, 133}, {94, 133}, {92, 131}, {91, 128}
};
_Static_assert(sizeof DOG_NOSE_B / sizeof DOG_NOSE_B[0] <= 64u, "dog contour bound");
static const int16_t DOG_NOSE_B_INNER[][2] = {
    {94, 127}, {95, 126}, {98, 126}, {99, 128}, {97, 130}, {95, 130}, {93, 129}
};
_Static_assert(sizeof DOG_NOSE_B_INNER / sizeof DOG_NOSE_B_INNER[0] <= 64u, "dog contour bound");

typedef struct { const int16_t (*p)[2]; uint8_t n, color; } cosmic_shape_t;
#define DOG_SHAPE(name, color) {DOG_##name, sizeof DOG_##name / sizeof DOG_##name[0], color}
static const cosmic_shape_t DOG_SHAPES[] = {
    DOG_SHAPE(TAIL, 1), DOG_SHAPE(BODY, 1), DOG_SHAPE(CROWN, 2), DOG_SHAPE(BRIM, 3),
    DOG_SHAPE(MUZZLE, 4), DOG_SHAPE(EYE_A, 0), DOG_SHAPE(EYE_B, 0),
    DOG_SHAPE(NOSE_A, 0), DOG_SHAPE(NOSE_A_INNER, 4), DOG_SHAPE(NOSE_B, 0), DOG_SHAPE(NOSE_B_INNER, 4)
};
#undef DOG_SHAPE

static struct {
    uint32_t ms;
    int32_t energy, pulse;
    uint8_t note_a, note_b, heavy;
} cosmic;
#ifdef DINO_VIS_TEST
static uint32_t cosmic_lines, cosmic_points;
#endif

static void cosmic_update(uint32_t now, uint32_t hit)
{
    uint32_t dt = now - cosmic.ms;
    int32_t fall;
    if (dt > 200u) dt = 200u;
    cosmic.ms = now;
    fall = (int32_t)dt * 3;
    cosmic.energy = vs.lvl[4] > cosmic.energy ? vs.lvl[4] : clamp(cosmic.energy - fall, 0, 1000);
    cosmic.pulse = hit & 5u ? 1000 : clamp(cosmic.pulse - (int32_t)dt * 4, 0, 1000);
    cosmic.note_a = vis_note[0];
    cosmic.note_b = vis_note[2];
    cosmic.heavy = song.cpu_q8 >= 192u;
}

/* A local hash, never the synth's rng(): changing pictures must not change a sound. */
static uint32_t cosmic_hash(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    return x ^ (x >> 16);
}
static void cosmic_line(int32_t x, int32_t y, int32_t xx, int32_t yy, uint16_t color)
{
#ifdef DINO_VIS_TEST
    cosmic_lines++;
#endif
    cv_line(x, y + 1, xx, yy + 1, mix565(0, color, 64));
    cv_line(x, y, xx, yy, color);
}
static void cosmic_point(int32_t x, int32_t y, uint16_t color)
{
#ifdef DINO_VIS_TEST
    cosmic_points++;
#endif
    cv_pset(x, y, color);
}

/* The body has concave feet: a bounded scan-line fill preserves its silhouette. */
static void cosmic_polygon(const cosmic_shape_t *shape, int32_t sn, int32_t cs, int32_t scale, int32_t lift,
                           uint16_t color, int outline)
{
    int32_t px[64], py[64], y, first = 240, last = 0;
    uint32_t i, n = shape->n;
    for (i = 0; i < n; i++) {
        int32_t dx = shape->p[i][0] - 120, dy = shape->p[i][1] - 140;
        px[i] = 120 * 16 + ((dx * cs - dy * sn) * scale >> 19);
        py[i] = (140 - lift) * 16 + ((dx * sn + dy * cs) * scale >> 19);
        if (py[i] / 16 < first) first = py[i] / 16;
        if (py[i] / 16 + 1 > last) last = py[i] / 16 + 1;
    }
    if (first < -cv_oy) first = -cv_oy;
    if (last > (int32_t)cv_h - cv_oy) last = (int32_t)cv_h - cv_oy;
    for (y = first; y < last; y++) {
        int32_t xs[64], count = 0, a, b, sy = y * 16 + 8;
        for (i = 0; i < n; i++) {
            uint32_t j = (i + 1u) % n;
            if ((py[i] <= sy && py[j] > sy) || (py[j] <= sy && py[i] > sy))
                xs[count++] = px[i] + (sy - py[i]) * (px[j] - px[i]) / (py[j] - py[i]);
        }
        for (a = 1; a < count; a++)
            for (b = a; b > 0 && xs[b - 1] > xs[b]; b--) {
                int32_t tmp = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = tmp;
            }
        for (a = 0; a + 1 < count; a += 2)
            cv_rect((xs[a] + 8) / 16, y, (xs[a + 1] + 8) / 16 - (xs[a] + 8) / 16, 1, color);
    }
    if (outline)
        for (i = 0; i < n; i++) {
            uint32_t j = (i + 1u) % n;
            int32_t x = px[i] / 16, yy = py[i] / 16, xx = px[j] / 16, yj = py[j] / 16;
            cv_line(x - 1, yy, xx - 1, yj, 0);
            cv_line(x + 1, yy, xx + 1, yj, 0);
            cv_line(x, yy - 1, xx, yj - 1, 0);
            cv_line(x, yy + 1, xx, yj + 1, 0);
            cv_line(x, yy, xx, yj, 0);
        }
}
static void cosmic_dog(void)
{
    static const uint16_t colors[] = {0, RGB(61, 195, 51), RGB(78, 68, 126), RGB(51, 43, 96), RGB(226, 247, 49)};
    int32_t angle = isin(cosmic.ms / 9u) * (4 + cosmic.pulse / 125) >> 15;
    int32_t sn = isin((uint32_t)angle), cs = icos((uint32_t)angle);
    uint32_t i;
    for (i = 0; i < sizeof DOG_SHAPES / sizeof DOG_SHAPES[0]; i++) {
        const cosmic_shape_t *s = &DOG_SHAPES[i];
        cosmic_polygon(s, sn, cs, 256 + cosmic.energy / 80, cosmic.energy / 250, colors[s->color], i < 5u);
    }
    /* The cheek's curl is as distinctive as the hat and the two nostrils. */
    {
        static const int16_t cheek[][2] = {{137, 105}, {136, 110}, {137, 114}, {140, 118}, {145, 121}, {149, 121}};
        int32_t x = 0, y = 0;
        for (i = 0; i < sizeof cheek / sizeof cheek[0]; i++) {
            int32_t dx = cheek[i][0] - 120, dy = cheek[i][1] - 140;
            int32_t xx = 120 + ((dx * cs - dy * sn) >> 15), yy = 140 + ((dx * sn + dy * cs) >> 15) - cosmic.energy / 250;
            if (i) vis_thick(x, y, xx, yy, 0);
            x = xx; y = yy;
        }
    }
}
static void cosmic_stars(uint32_t count, uint16_t color)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        uint32_t h = cosmic_hash(i + 7919u), a = (h & 1023u) + cosmic.ms / (20u + (h >> 12 & 31u));
        int32_t r = 34 + (int32_t)(h >> 18) % 80;
        int32_t x = 120 + (icos(a) * r >> 15), y = 120 + (isin(a) * r >> 15);
        cosmic_point(x, y, mix565(RGB(8, 12, 30), color, 64 + (int32_t)((h + cosmic.ms / 20u) & 127u)));
    }
}
static void cosmic_draw(uint32_t style)
{
    uint32_t i, j, phase = cosmic.ms / 12u + cosmic.note_a * 7u;
    uint16_t color = mix565(RGB(35, 181, 245), RGB(202, 70, 255), cosmic.note_a % 12u * 256 / 11);
#ifdef DINO_VIS_TEST
    cosmic_lines = cosmic_points = 0;
#endif
    /* shortcut: fixed geometry caps suit this 12 MHz LCD; raise them only after device timing tests. */
    if (style == 0u) {
        cosmic_stars(cosmic.heavy ? 24u : 48u, color);
        for (i = 0; i < 4u; i++) {
            int32_t x = 0, y = 0;
            uint32_t count = cosmic.heavy ? 12u : 24u;
            for (j = 0; j <= count; j++) {
                uint32_t a = phase + i * 256u + j * (1024u / count);
                int32_t r = 32 + (int32_t)j * 78 / (int32_t)count + cosmic.pulse / 125;
                int32_t xx = 120 + (icos(a) * r >> 15), yy = 120 + (isin(a) * r >> 15);
                if (j) cosmic_line(x, y, xx, yy, mix565(RGB(10, 16, 42), color, 40 + (int32_t)j * 160 / (int32_t)count));
                x = xx; y = yy;
            }
        }
    } else if (style == 1u) {
        cosmic_stars(cosmic.heavy ? 32u : 64u, color);
        for (i = 0; i < 8u; i++) {
            int16_t x[16], y[16];
            uint16_t angle[16];
            uint32_t count = cosmic.heavy ? 8u : 16u;
            angle[0] = (uint16_t)(phase + i * 128u);
            x[0] = (int16_t)(120 + (icos(angle[0]) * 58 >> 15));
            y[0] = (int16_t)(120 + (isin(angle[0]) * 58 >> 15));
            for (j = 1; j < count; j++) {
                uint32_t parent = j / 2u, depth = j < 2u ? 0u : j < 4u ? 1u : j < 8u ? 2u : 3u;
                int32_t spread = 42 + cosmic.note_b % 12u * 4, length = 21 - (int32_t)depth * 5;
                angle[j] = (uint16_t)(angle[parent] + (j == 1u ? 0 : j & 1u ? spread : -spread));
                x[j] = (int16_t)(x[parent] + (icos(angle[j]) * length >> 15));
                y[j] = (int16_t)(y[parent] + (isin(angle[j]) * length >> 15));
                cosmic_line(x[parent], y[parent], x[j], y[j], mix565(color, RGB(235, 233, 255), (int32_t)depth * 48));
            }
        }
    } else {
        uint32_t count = cosmic.heavy ? 96u : 192u;
        for (i = 0; i < count; i++) {
            uint32_t h = cosmic_hash(i + 104729u), a = (h & 1023u) + phase + i * (cosmic.note_b % 5u + 1u);
            int32_t r = 44 + (int32_t)(h >> 16) % 67 + cosmic.energy / 200;
            int32_t x = 120 + (icos(a) * r >> 15), y = 120 + (isin(a * 3u + (h >> 24)) * r >> 15);
            uint16_t particle = mix565(color, RGB(242, 104, 235), (int32_t)(h >> 24));
            cosmic_point(x, y, particle);
            if (!(i % 12u)) {
                cosmic_point(x + 1, y, mix565(0, particle, 96));
                cosmic_point(x, y + 1, mix565(0, particle, 96));
            }
        }
        for (i = 0; i < 3u; i++) {
            int32_t x = 0, y = 0;
            for (j = 0; j <= 16u; j++) {
                uint32_t a = j * 64u + phase + i * 341u;
                int32_t xx = 120 + (icos(a) * (90 + (int32_t)i * 9) >> 15);
                int32_t yy = 120 + (isin(a * 2u + cosmic.note_b * 7u) * (67 + (int32_t)i * 12) >> 15);
                if (j) cosmic_line(x, y, xx, yy, mix565(RGB(8, 14, 35), color, 72 + (int32_t)i * 40));
                x = xx; y = yy;
            }
        }
    }
    cosmic_dog();
}
