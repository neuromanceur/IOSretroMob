/* Read-only CLCD native observation. IOSretroMob / S5LBox, MIT. */
#ifndef IOSRETROMOB_CLCD_MULTILAYER_CAPTURE_H
#define IOSRETROMOB_CLCD_MULTILAYER_CAPTURE_H
#include "soc.h"
#include <stddef.h>
#include <stdint.h>
typedef enum {
    CLCD_NATIVE_OK, CLCD_NATIVE_DISABLED, CLCD_NATIVE_FORMAT,
    CLCD_NATIVE_GEOMETRY, CLCD_NATIVE_STRIDE, CLCD_NATIVE_MEMORY,
    CLCD_NATIVE_RAW_ORDER
} clcd_native_status_t;
typedef struct {
    const uint8_t *pixels;
    size_t bytes; /* scanout span: row padding retained, no final-row padding */
    uint32_t width, height, stride, format, order, bytes_per_pixel, x, y;
    s5l_clcd_window_t registers;
} clcd_native_view_t;
/* On error, writes nothing to view. Format 7/order 0 is driver-declared
 * ARGB (native BGRA bytes); 3/order 0 is RGB565 LE. Known format with nonzero
 * order returns RAW_ORDER with a bounded byte span, not decoded colours.
 * Unknown formats still fail: never infer their layout from bit depth.
 * A retained alpha byte does NOT establish hardware blend semantics. */
clcd_native_status_t clcd_native_view(const s5l_clcd_t *, unsigned,
    const uint8_t *, uint32_t, size_t, clcd_native_view_t *);
const char *clcd_native_status_name(clcd_native_status_t);
/* Hash all window pixels/programming and relevant global registers. Frames,
 * interrupt status and instruction count do not trigger duplicate captures. */
bool clcd_multilayer_digest(const s5l8900_t *, uint8_t digest[32]);
/* Synchronous at one paused guest instruction. Create-only files; JSON is
 * written last and is the bundle completion record. Failed partial outputs
 * are retained. Invalid active windows appear with explicit status, no pixels.
 * reserve includes a bounded allowance for all metadata. No guest writes. */
bool clcd_multilayer_write(const s5l8900_t *, uint64_t instruction,
    const char *directory, unsigned sample, uint64_t reserve,
    uint64_t *written, char manifest_sha256[65]);
#endif
