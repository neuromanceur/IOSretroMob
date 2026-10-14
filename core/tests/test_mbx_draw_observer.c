/* Opt-in draw observer fixtures. MIT, IOSretroMob.
 * Synthetic words and pixels only. Verifies that the campaign-9 graphics
 * observer (a) fires once per well-framed native stream with the raw words and
 * render context, (b) is never invoked when none is installed, and (c) leaves
 * the render decision and framebuffer byte-for-byte identical whether or not an
 * observer is present. No guest execution, no file I/O: the test observer only
 * records what the core handed it. */
#include "../src/soc/mbx.c"
#include <assert.h>
#include "snapshot.h"

static unsigned checks;
#define VERIFY(x) do { assert(x); checks++; } while(0)

/* A recording observer: no file I/O, no guest mutation, just bookkeeping. */
static struct {
    unsigned calls;
    uint32_t last_word_count;
    uint32_t last_word0;
    uint32_t last_word_last;
    uint32_t last_target, last_width, last_height, last_pixel_bytes;
    const uint32_t *seen_words;
    uint32_t seen_count;
} OBS;

static bool rec_observer(void *ctx, const s5l_mbx_t *m, const arm_bus_t *bus,
                         const s5l_mbx_draw_observation_t *obs) {
    (void)ctx; (void)m; (void)bus;
    OBS.calls++;
    OBS.last_word_count = obs->word_count;
    OBS.last_word0 = obs->words[0];
    OBS.last_word_last = obs->words[obs->word_count - 1u];
    OBS.last_target = obs->target;
    OBS.last_width = obs->target_width;
    OBS.last_height = obs->target_height;
    OBS.last_pixel_bytes = obs->pixel_bytes;
    OBS.seen_words = obs->words;
    OBS.seen_count = obs->word_count;
    return true;
}

#define RAM_BASE 0x08000000u
#define RAM_SIZE 0x08000000u
#define GART0    0x08001000u
#define GART2    0x08002000u
#define OBJ_PA   0x08040000u
#define TGT_PA   0x08100000u

static void map_page(s5l8900_t *m, uint32_t table, uint32_t gpu, uint32_t pa) {
    m->bus.write32(m->bus.ctx, table + (((gpu >> 12) & 0x3ffu) * 4u), pa);
}

/* Build one machine with a 320x480 RGB565 target and a GART, ready to stage a
 * TA stream. Shared by the observer-present and observer-absent replays so the
 * only difference between them is whether an observer is installed. */
static void setup_machine(s5l8900_t *m) {
    VERIFY(m && s5l8900_init(m, RAM_BASE, RAM_SIZE));
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x1000u, GART0);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x1008u, GART2);
    for (uint32_t p = 0; p < 64u * 0x1000u; p += 0x1000u)
        map_page(m, GART0, 0x100000u + p, OBJ_PA + p);
    for (uint32_t p = 0; p < 307200u; p += 0x1000u)
        map_page(m, GART2, 0x801000u + p, TGT_PA + p);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x083cu, 0x100000u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x0844u, 0x20000u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x0608u, 0x20000u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x060cu, 0x100000u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x061cu, 0x00020007u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x0650u, 9u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x0654u, 0x013f0000u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x0658u, 0x01df0000u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x065cu, 0x801000u);
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x0660u, 160u);
}

/* Stage a synthetic stream; framing alone does not imply render acceptance. */
static bool replay(s5l8900_t *m, const uint32_t *words, uint32_t n) {
    m->bus.write32(m->bus.ctx, S5L8900_MBX_BASE + 0x0800u, 1u);
    s5l_mbx_stage_ta_write(&m->mbx, &m->bus, S5L_MBX_TA_START, 1u);
    for (uint32_t i = 0; i < n; i++)
        s5l_mbx_stage_ta_write(&m->mbx, &m->bus, S5L_MBX_3D_DATA_FIFO, words[i]);
    return s5l_mbx_process_3d(&m->mbx, &m->bus, S5L_MBX_STARTRENDER, 1u,
                              &m->mbx_telemetry);
}

int main(void) {
    /* A well-framed synthetic stream with deliberately unsupported payload. */
    static const uint32_t stream[] = {
        0x10000010u, 0x12345678u, 0x89abcdefu, 0x0badc0deu, 0xf0000000u
    };

    uint8_t reference_pixels[307200];
    bool reference_accepted;
    uint32_t reference_status;

    /* 1. Observer absent: record the actual decision and target bytes. */
    {
        s5l8900_t m;
        OBS.calls = 0;
        setup_machine(&m);
        reference_accepted = replay(&m, stream, 5u);
        /* Whether this particular synthetic stream renders or is refused, the
         * observer-absent path must not have fired. */
        VERIFY(OBS.calls == 0);
        reference_status = m.bus.read32(m.bus.ctx, S5L8900_MBX_BASE + 0x012cu);
        memcpy(reference_pixels, m.ram + (TGT_PA - RAM_BASE), sizeof reference_pixels);
        s5l8900_free(&m);
    }

    /* 2. Observer present: replay the identical stream; the observer must fire
     * exactly once with the raw words and render context. */
    {
        s5l8900_t m;
        OBS.calls = 0;
        setup_machine(&m);
        VERIFY(s5l_mbx_set_draw_observer(&m.mbx, rec_observer, NULL));
        VERIFY(replay(&m, stream, 5u) == reference_accepted);
        VERIFY(m.bus.read32(m.bus.ctx, S5L8900_MBX_BASE + 0x012cu) == reference_status);
        VERIFY(!memcmp(reference_pixels, m.ram + (TGT_PA - RAM_BASE), sizeof reference_pixels));
        VERIFY(OBS.calls == 1);
        VERIFY(OBS.last_word_count == 5u);
        VERIFY(OBS.last_word0 == 0x10000010u);
        VERIFY(OBS.last_word_last == 0xf0000000u);
        VERIFY(OBS.last_target == 0x801000u);
        VERIFY(OBS.last_width == 320u);
        VERIFY(OBS.last_height == 480u);
        VERIFY(OBS.last_pixel_bytes == 2u);
        /* Clear the observer so later tests are unaffected. */
        VERIFY(s5l_mbx_set_draw_observer(&m.mbx, NULL, NULL));
        s5l8900_free(&m);
    }

    /* 3. set_draw_observer rejects a NULL machine and clears cleanly. */
    VERIFY(!s5l_mbx_set_draw_observer(NULL, rec_observer, NULL));

    printf("%u draw-observer checks passed; no guest execution\n", checks);
    return 0;
}
