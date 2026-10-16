/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* The DINO record occupies the existing four project objects; no sequence data. */
#define DINO_PERF_MAGIC 0x314E4944u
#define DINO_AUTO_MAGIC 0x31414944u
#define DINO_STORE_VERSION 1u
typedef struct {
    uint8_t engine, preset, user, reserved;
    int16_t p[P_COUNT];
    uint8_t fm6[128];
} dino_sound_t;
typedef struct {
    uint32_t magic, size;
    uint16_t version, np, ng, reserved;
    char name[13];
    uint8_t mode, focus, split;
    int8_t octave;
    uint8_t style, padding[2];
    int16_t g[G_COUNT];
    dino_sound_t sound[3];                     /* A, B, DRUMS */
    int8_t tune[16], decay[16];
    uint32_t sum;
} dino_performance_t;
_Static_assert(sizeof(dino_performance_t) <= 3840u, "DINO performance fits an existing project object");
_Static_assert(__builtin_offsetof(dino_performance_t, style) == 33u, "Editor backup preflight reads the visual style at byte 33");
static uint32_t dino_store_hash(const void *data, uint32_t n)
{
    const uint8_t *p = data;
    uint32_t h = 0x811C9DC5u;
    while (n--) h = (h ^ *p++) * 16777619u;
    return h;
}
static void dino_store_seal(dino_performance_t *p)
{
    p->sum = dino_store_hash(p, __builtin_offsetof(dino_performance_t, sum));
}
static int dino_store_valid(const dino_performance_t *p, uint32_t n, uint32_t magic)
{
    uint32_t i, j;
    if (n != sizeof *p || p->magic != magic || p->size != sizeof *p || p->version != DINO_STORE_VERSION ||
        p->np != P_COUNT || p->ng != G_COUNT || p->mode > 3u || p->focus > 1u || p->split > 127u ||
        p->octave < -3 || p->octave > 3 || p->style >= DINO_VIS_N || !p->name[0] || p->name[12] ||
        p->sum != dino_store_hash(p, __builtin_offsetof(dino_performance_t, sum)))
        return 0;
    for (i = 0; i < 12u && p->name[i]; i++)
        if ((uint8_t)p->name[i] < 32u || (uint8_t)p->name[i] > 126u) return 0;
    for (i = 0; i < 3u; i++) {
        if (p->sound[i].engine >= NENGINES || p->sound[i].user > UP_SLOTS) return 0;
        for (j = 0; j < 128u; j++) if (p->sound[i].fm6[j] > 127u) return 0;
    }
    for (i = 0; i < 16u; i++)
        if (p->tune[i] < -8 || p->tune[i] > 7 || p->decay[i] < -4 || p->decay[i] > 3) return 0;
    return 1;
}
