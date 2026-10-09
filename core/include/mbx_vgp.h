/* Bounded, measured fixed-function VGP pipeline. S5LBox, MIT. */
#ifndef S5LBOX_MBX_VGP_H
#define S5LBOX_MBX_VGP_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    double xyz[3], rgba[4], uv[2];
} mbx_vgp_vertex;
typedef struct {
    /* The TA uniform is row-major, unlike the public GL matrix array. */
    double matrix[16];
    mbx_vgp_vertex vertex[6];
    /* Transient decoded scene, never serialized machine state. Zero count
     * preserves the historical three-vertex callers. */
    uint32_t vertex_count;
    uint32_t texture_source, texture_width, texture_height;
    const uint8_t *texture_rgba; /* Fully staged canonical RGBA8, opaque. */
    uint32_t texture_bytes;
    /* Transient only. Enabled solely by measured untextured RGBA envelopes. */
    bool colour_alpha, blend_source_alpha;
    bool texture_linear; /* Only a measured GL_LINEAR envelope enables this. */
    double centre_x, scale_x, centre_y, scale_y;
    uint32_t clear_left, clear_top, clear_right, clear_bottom, clear_bgra;
} mbx_vgp_scene;

/* Recognizes six complete, measured program/state envelopes. Unknown code or
 * state is rejected. This is NOT a general VGP instruction interpreter. */
bool mbx_vgp_decode(const uint32_t *words, uint32_t count,
                    mbx_vgp_scene *scene, const char **why);
/* Pure staged BGRA8 rasterization. Validate before touching any pixel; the
 * caller separately validates GART and commits the staged surface atomically.
 * Opaque vertex colours; optional canonical RGBA8 nearest/linear clamp-to-edge.
 * Texture decoding/resources are independently validated by the caller.
 * Bounded straight-alpha RGBA8 blending when the decoded envelope enables it.
 * No depth or dithering. */
bool mbx_vgp_raster(const mbx_vgp_scene *scene, uint8_t *pixels,
                    uint32_t width, uint32_t height,
                    uint32_t left, uint32_t top,
                    uint32_t right, uint32_t bottom,
                    uint32_t *written, const char **why);
#endif
