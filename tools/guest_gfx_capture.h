/* Bounded, opt-in native graphics-state observer. Original IOSretroMob, MIT.
 *
 * Installed into the MBX core via s5l_mbx_set_draw_observer(). The core calls
 * gfx_observe() once per well-framed native TA stream, before the VGP/TA split
 * and before any draw parsing, passing the raw object-database words and the
 * render context. This observer then, for each textured draw it can locate in
 * those words, translates the source GPU VA through the guest GART and copies
 * a bounded window of the real texture bytes, recording SHA-256 and provenance.
 *
 * Observation only. It reads guest DRAM through the read-only host_ram path
 * and writes nothing the guest can see: no GPU register, no guest RAM, no IRQ,
 * no completion, no texture content is changed. With IOSRETROMOB_GFX_CAPTURE_DIR
 * unset it installs nothing and the core captures nothing.
 *
 * Purpose: capture the authentic bytes for the three campaign-8 blockers --
 * the 8x8 source-mode 0x32 draw, the Morton format-nibble-0 textures, and the
 * VGP profile whose matrix sits at offset 255 -- so their layout can be decoded
 * from evidence rather than guessed. */
#ifndef IOSRETROMOB_GUEST_GFX_CAPTURE_H
#define IOSRETROMOB_GUEST_GFX_CAPTURE_H
#include "sha256.h"
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>

/* Capture budget (ceilings, not targets). */
#define GFX_MAX_STREAMS   64u          /* well-framed native streams observed   */
#define GFX_MAX_WORDS     32768u       /* words retained per stream             */
#define GFX_MAX_RESOURCES 16u          /* textures captured in full             */
#define GFX_RES_CAP       (2u*1024u*1024u) /* 2 MiB per resource                */
#define GFX_TEX_BUDGET    (32u*1024u*1024u)/* 32 MiB total texture bytes        */
#define GFX_STATE_BUDGET  (4u*1024u*1024u) /* 4 MiB VGP state + words           */

static struct {
    bool enabled, failed, installed;
    char dir[PATH_MAX];
    FILE *index;
    uint64_t streams, state_bytes, tex_bytes, resources;
} GFX;

static void gfx_fail(void) {
    GFX.failed = true;
    fprintf(stderr, "GFX_CAPTURE_FAILURE: stopping the host run; guest state unchanged\n");
    fflush(stderr);
    raise(SIGINT);
}

/* Validate the capture directory exactly like guest_ta_capture.h: absolute,
 * canonical, no symlink in it or any ancestor. Returns false (do not arm) on
 * any doubt, so a bad path never silently disables capture or escapes .local. */
static bool gfx_valid_dir(const char *dir) {
    char canonical[PATH_MAX];
    struct stat st;
    if (*dir != '/' || strlen(dir) >= sizeof GFX.dir) return false;
    if (!realpath(dir, canonical) || strcmp(dir, canonical) != 0) return false;
    if (lstat(dir, &st) || !S_ISDIR(st.st_mode)) return false;
    strcpy(canonical, dir);
    for (char *q = canonical + strlen(canonical); q > canonical; q--)
        if (*q == '/' || *q == 0) {
            char saved = *q; *q = 0;
            if (*canonical && (lstat(canonical, &st) || S_ISLNK(st.st_mode))) {
                *q = saved; return false;
            }
            *q = saved;
        }
    return true;
}

/* Translate one guest GPU VA to its physical DRAM address through the eight
 * GART roots, mirroring mbx_gart_span's read-only page walk. Returns the
 * physical address and the bytes available in that page, or false if the VA is
 * outside the GART or its PTE is absent -- never a fabricated address. */
static bool gfx_gart_span(const s5l_mbx_t *m, const arm_bus_t *bus,
                          uint32_t gpu_va, uint32_t *physical, uint32_t *avail) {
    uint32_t chunk = gpu_va >> 22;
    if (!bus || !bus->host_ram || chunk >= 8u) return false;
    uint32_t root = m->reg[(0x1000u / 4u) + chunk];
    uint32_t pte_off = ((gpu_va >> 12) & 0x3ffu) * 4u;
    if (!root || (root & 0xfffu) || root > UINT32_MAX - pte_off) return false;
    const uint8_t *entry = bus->host_ram(bus->ctx, root + pte_off, 4u);
    if (!entry) return false;
    uint32_t pte = (uint32_t)entry[0] | (uint32_t)entry[1] << 8 |
                   (uint32_t)entry[2] << 16 | (uint32_t)entry[3] << 24;
    if (!pte || (pte & 0xfffu)) return false;
    uint32_t in_page = gpu_va & 0xfffu;
    *physical = pte + in_page;
    *avail = 0x1000u - in_page;
    return true;
}

/* Copy len texture bytes from a GPU VA into dst via GART, page by page. Returns
 * false on any unmapped page (partial copy discarded); never invents bytes. */
static bool gfx_gart_read(const s5l_mbx_t *m, const arm_bus_t *bus,
                          uint32_t gpu_va, uint8_t *dst, uint32_t len) {
    while (len) {
        uint32_t pa, span;
        if (!gfx_gart_span(m, bus, gpu_va, &pa, &span)) return false;
        if (span > len) span = len;
        const uint8_t *src = bus->host_ram(bus->ctx, pa, span);
        if (!src) return false;
        memcpy(dst, src, span);
        dst += span; gpu_va += span; len -= span;
    }
    return true;
}



/* A plausible texture sampler word: the low 24 bits are one of the measured
 * sampler configurations; the top byte varies with the filtered/format class.
 * Deliberately permissive so a novel sampler is still captured. */
static bool gfx_sampler(uint32_t s) {
    uint32_t low = s & 0x00ffffffu;
    return low == 0x00084610u || low == 0x00081e10u || low == 0x00087610u;
}

/* Decode a texture source word for capture bookkeeping only. Returns the
 * decoded GPU VA and a conservative byte size for the observed regimes, or
 * false for a regime this observer does not yet label. Never guesses bytes. */
static bool gfx_source_va(uint32_t header, uint32_t source_word,
                          uint32_t width, uint32_t height,
                          uint32_t *gpu_va, uint32_t *bytes) {
    (void)header;
    uint32_t top7 = (source_word >> 24) & 0x7fu;
    if (top7 == 0x12u || top7 == 0x32u) {
        *gpu_va = (source_word & 0x0003ffffu) << 7;   /* twiddle addr field */
    } else if ((source_word & 0xff000000u) == 0x0e000000u ||
               (source_word & 0xff000000u) == 0x8e000000u) {
        *gpu_va = (source_word & 0x0003ffffu) << 7;   /* strided linear */
    } else {
        return false;
    }
    uint32_t w = width ? width : 8u, h = height ? height : 8u;
    uint64_t need = (uint64_t)w * h * 4u;      /* conservative until measured */
    if (need > GFX_RES_CAP) need = GFX_RES_CAP;
    *bytes = (uint32_t)need;
    return true;
}

static void gfx_hex(FILE *f, const uint8_t *p, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) fprintf(f, "%02x", p[i]);
}

/* The observer the MBX core calls. Read-only; return value ignored by core. */
static bool gfx_observe(void *ctx, const s5l_mbx_t *m, const arm_bus_t *bus,
                        const s5l_mbx_draw_observation_t *obs) {
    (void)ctx;
    if (!GFX.enabled || GFX.failed || !GFX.index || !m || !bus || !obs) return false;
    if (!obs->words || obs->word_count < 3u) return false;
    if (GFX.streams >= GFX_MAX_STREAMS) return false;

    uint64_t at = G.hot_now;
    uint32_t n = obs->word_count;
    const uint32_t *w = obs->words;

    /* Retain the whole stream's words as VGP/profile/descriptor evidence,
     * bounded, with a SHA-256 over exactly the bytes written. */
    uint32_t keep = n < GFX_MAX_WORDS ? n : GFX_MAX_WORDS;
    size_t wbytes = (size_t)keep * 4u;
    uint8_t hash[32];
    if (keep && GFX.state_bytes + wbytes <= GFX_STATE_BUDGET) {
        uint8_t *packed = malloc(wbytes);
        if (!packed) { gfx_fail(); return false; }
        for (uint32_t i = 0; i < keep; i++)
            for (unsigned b = 0; b < 4u; b++)
                packed[4u * i + b] = (uint8_t)(w[i] >> (8u * b));
        if (!ios3_sha256(packed, wbytes, hash)) { free(packed); gfx_fail(); return false; }
        char name[96], path[PATH_MAX];
        snprintf(name, sizeof name, "stream-%04llu-words.bin",
                 (unsigned long long)GFX.streams);
        if (snprintf(path, sizeof path, "%s/%s", GFX.dir, name) >= (int)sizeof path) {
            free(packed); gfx_fail(); return false;
        }
        FILE *f = fopen(path, "wbx");
        if (!f) { free(packed); gfx_fail(); return false; }
        bool ok = fwrite(packed, 1, wbytes, f) == wbytes;
        if (fclose(f)) ok = false;
        free(packed);
        if (!ok) { gfx_fail(); return false; }
        GFX.state_bytes += wbytes;
        fprintf(GFX.index,
            "{\"kind\":\"stream\",\"at\":%llu,\"index\":%llu,\"word_count\":%u,"
            "\"kept_words\":%u,\"target\":%u,\"width\":%u,\"height\":%u,"
            "\"pixel_bytes\":%u,\"file\":\"%s\",\"sha256\":\"",
            (unsigned long long)at, (unsigned long long)GFX.streams, n, keep,
            obs->target, obs->target_width, obs->target_height, obs->pixel_bytes,
            name);
        gfx_hex(GFX.index, hash, 32);
        fputs("\"}\n", GFX.index);
        fflush(GFX.index);
        if (ferror(GFX.index)) gfx_fail();
    }

    /* Locate textured descriptors in the raw words and capture a bounded window
     * of the real texture bytes at each measured source GPU VA. */
    uint32_t captured = 0u;
    for (uint32_t i = 2u; i + 1u < n && captured < 8u; i++) {
        if (!gfx_sampler(w[i])) continue;
        uint32_t header = w[i - 2u], source_word = w[i - 1u];
        uint32_t wf = (header >> 24) & 7u, hf = (header >> 20) & 7u;
        uint32_t width = 8u << wf, height = 8u << hf;
        if (wf > 6u || hf > 6u) continue;
        uint32_t gpu_va, bytes;
        if (!gfx_source_va(header, source_word, width, height, &gpu_va, &bytes))
            continue;
        if (!gpu_va || !bytes) continue;
        if (GFX.resources >= GFX_MAX_RESOURCES) break;
        if (GFX.tex_bytes + bytes > GFX_TEX_BUDGET) continue;
        uint8_t *tex = malloc(bytes);
        if (!tex) { gfx_fail(); return false; }
        uint32_t first_pa = 0u, first_span = 0u;
        gfx_gart_span(m, bus, gpu_va, &first_pa, &first_span);
        if (gfx_gart_read(m, bus, gpu_va, tex, bytes) &&
            ios3_sha256(tex, bytes, hash)) {
            char name[96], path[PATH_MAX];
            snprintf(name, sizeof name, "tex-%04llu-w%u-%08x.bin",
                     (unsigned long long)GFX.streams, i, gpu_va);
            if (snprintf(path, sizeof path, "%s/%s", GFX.dir, name) >= (int)sizeof path) {
                free(tex); gfx_fail(); return false;
            }
            FILE *f = fopen(path, "wbx");
            if (!f) { free(tex); gfx_fail(); return false; }
            bool ok = fwrite(tex, 1, bytes, f) == bytes;
            if (fclose(f)) ok = false;
            free(tex);
            if (!ok) { gfx_fail(); return false; }
            GFX.tex_bytes += bytes;
            GFX.resources++;
            captured++;
            fprintf(GFX.index,
                "{\"kind\":\"texture\",\"at\":%llu,\"stream\":%llu,\"word\":%u,"
                "\"header\":\"%08x\",\"source\":\"%08x\",\"sampler\":\"%08x\","
                "\"src_mode\":\"%02x\",\"width\":%u,\"height\":%u,"
                "\"gpu_va\":%u,\"phys\":%u,\"bytes\":%u,\"file\":\"%s\",\"sha256\":\"",
                (unsigned long long)at, (unsigned long long)GFX.streams, i,
                header, source_word, w[i], (source_word >> 24) & 0x7fu,
                width, height, gpu_va, first_pa, bytes, name);
            gfx_hex(GFX.index, hash, 32);
            fputs("\"}\n", GFX.index);
        } else {
            /* Source not readable through the GART: record the refusal, never
             * substitute bytes. This is itself evidence for obstacle A/B. */
            free(tex);
            fprintf(GFX.index,
                "{\"kind\":\"texture_unreadable\",\"at\":%llu,\"stream\":%llu,"
                "\"word\":%u,\"header\":\"%08x\",\"source\":\"%08x\","
                "\"src_mode\":\"%02x\",\"gpu_va\":%u,\"bytes\":%u}\n",
                (unsigned long long)at, (unsigned long long)GFX.streams, i,
                header, source_word, (source_word >> 24) & 0x7fu, gpu_va, bytes);
        }
        fflush(GFX.index);
        if (ferror(GFX.index)) gfx_fail();
    }
    GFX.streams++;
    return true;
}


/* Arm the observer if IOSRETROMOB_GFX_CAPTURE_DIR names a validated directory.
 * Returns false (guest runs unobserved) if unset or invalid. Installs the core
 * observer only on success. */
static bool gfx_capture_arm(void) {
    const char *dir = getenv("IOSRETROMOB_GFX_CAPTURE_DIR");
    if (!dir) return false;
    if (!gfx_valid_dir(dir)) return false;
    char path[PATH_MAX];
    if (snprintf(path, sizeof path, "%s/gfx-index.jsonl", dir) >= (int)sizeof path)
        return false;
    GFX.index = fopen(path, "wx");
    if (!GFX.index) return false;
    strcpy(GFX.dir, dir);
    GFX.enabled = true;
    GFX.streams = GFX.state_bytes = GFX.tex_bytes = GFX.resources = 0;
    GFX.failed = false;
    fprintf(GFX.index,
        "{\"kind\":\"config\",\"max_streams\":%u,\"max_words\":%u,"
        "\"max_resources\":%u,\"res_cap\":%u,\"tex_budget\":%u,\"state_budget\":%u,"
        "\"scope\":\"read-only GART texture + stream-word capture, no guest mutation\"}\n",
        GFX_MAX_STREAMS, GFX_MAX_WORDS, GFX_MAX_RESOURCES, GFX_RES_CAP,
        GFX_TEX_BUDGET, GFX_STATE_BUDGET);
    fflush(GFX.index);
    if (ferror(GFX.index)) { gfx_fail(); return false; }
    if (g_mach) s5l_mbx_set_draw_observer(&g_mach->mbx, gfx_observe, NULL);
    GFX.installed = true;
    return true;
}

static void gfx_capture_finish(void) {
    if (!GFX.index) return;
    fprintf(GFX.index,
        "{\"kind\":\"summary\",\"streams\":%llu,\"resources\":%llu,"
        "\"state_bytes\":%llu,\"tex_bytes\":%llu,\"failed\":%s}\n",
        (unsigned long long)GFX.streams, (unsigned long long)GFX.resources,
        (unsigned long long)GFX.state_bytes, (unsigned long long)GFX.tex_bytes,
        GFX.failed ? "true" : "false");
    if (fclose(GFX.index)) gfx_fail();
    GFX.index = NULL;
    GFX.enabled = false;
}

#endif
