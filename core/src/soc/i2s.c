/*
 * S5LBox — the two S5L8900 I2S controller windows.
 *
 * Honest storage for the seven offsets AppleS5L8900XI2SController writes, and
 * bounded visibility for anything else. Default behaviour is storage only.
 * An explicit development option adds an experimental clock/TX queue below;
 * its gates, rate, depth and frame layout are not proven hardware semantics.
 *
 * See the I2S block in soc.h for the enumeration of the seven write sites and
 * for the four checks that establish `readRegister` is dead code.
 *
 * Copyright (c) 2026 j0shua-SYSON. MIT licensed.
 */
#include "soc.h"
#include <string.h>

/*
 * The offsets, in this model's storage order. This is the whole register map:
 * every writeRegister dispatch in the class resolves to one of these seven, so
 * a driver that touched an eighth would be doing something no shipped code path
 * does — which is exactly what the unknown-offset log exists to make visible.
 */
static const uint32_t I2S_OFFSETS[S5L_I2S_REGS] = {
    0x00u, 0x04u, 0x08u, 0x30u, 0x34u, 0x3cu, 0x40u
};

uint32_t s5l_i2s_offset(unsigned index) {
    if (index >= S5L_I2S_REGS) return UINT32_MAX;
    return I2S_OFFSETS[index];
}

/*
 * The FIFO offsets, named. See the `dma-channels` decode in soc.h for where
 * both of them and the direction inference come from.
 *
 * This is deliberately a pure classifier rather than a branch inside
 * s5l_i2s_write(). Recognising an offset must not start excusing it: a CPU
 * store to a FIFO whose physical address the tree hands to the PL080 is the
 * driver doing programmed I/O where DMA was intended, and that stays counted
 * as an unknown-offset access below, where a census will see it. Naming it and
 * forgiving it are different changes and only the first one is made here.
 */
s5l_i2s_fifo_t s5l_i2s_fifo_role(uint32_t off) {
    if (off == S5L_I2S_TX_FIFO_OFF) return S5L_I2S_FIFO_TX;
    if (off == S5L_I2S_RX_FIFO_OFF) return S5L_I2S_FIFO_RX;
    return S5L_I2S_FIFO_NONE;
}

uint32_t s5l_i2s_fifo_pa(unsigned index, s5l_i2s_fifo_t which) {
    uint32_t base;
    if (index == 0u)      base = S5L8900_I2S0_BASE;
    else if (index == 1u) base = S5L8900_I2S1_BASE;
    else                  return UINT32_MAX;
    if (which == S5L_I2S_FIFO_TX) return base + S5L_I2S_TX_FIFO_OFF;
    if (which == S5L_I2S_FIFO_RX) return base + S5L_I2S_RX_FIFO_OFF;
    return UINT32_MAX;
}

/* Development H1: CLKCON power, TXCON sync-request bit 20 and LRCK-on
 * (TXCOM bit 0 clear) permit a free-running external frame pulse. TXCOM bit 2
 * is deliberately NOT required: the native driver sleeps before writing 6.
 * Bit20 gates native startTransfer's sync branch; its physical meaning and
 * pulse shape are unproved. Nominal frame rate is a caller-supplied hypothesis.
 * No GPL code, guest function addresses, sample production or DMA kick. */
static bool clock_condition(const s5l_i2s_t *s) {
    return s->experimental_hz && (s->regs[0]&1u) &&
        (s->regs[1]&(1u<<20)) && !(s->regs[2]&1u);
}
static uint64_t saturated_add(uint64_t a,uint64_t b) {
    return UINT64_MAX-a<b ? UINT64_MAX : a+b;
}
static void clock_configure(s5l_i2s_t *s) {
    bool running=clock_condition(s);
    if(running!=s->clock_running) {
        s->clock_running=running;s->frame_phase=0;
        s->clock_generation=saturated_add(s->clock_generation,1);
    }
}
bool s5l_i2s_set_experimental_clock(s5l_i2s_t *s,uint32_t hz) {
    if(!s || hz>192000u)return false;
    if(s->experimental_hz!=hz) {
        s->experimental_hz=hz;s->clock_running=false;s->frame_phase=0;
        /* No active queue may be dropped silently by an inactive snapshot. */
        memset(s->tx_fifo,0,sizeof s->tx_fifo);s->tx_head=s->tx_level=0;
        s->clock_generation=saturated_add(s->clock_generation,1);
    }
    clock_configure(s);return true;
}
uint32_t s5l_i2s_next_frame(const s5l_i2s_t *s) {
    if(!s || !s->clock_running)return 0;
    return (uint32_t)((S5L8900_TB_HZ-s->frame_phase+s->experimental_hz-1)/s->experimental_hz);
}
/* Historical TXCOM bit 1 enables DMA requests, bit 2 the TX interface.
 * A four-byte free watermark accommodates PL080's largest source chunk;
 * it prevents a split-width transfer losing staged bytes. Depth and the
 * four-byte frame consumption remain explicitly experimental assumptions. */
bool s5l_i2s_tx_ready(const s5l_i2s_t *s,unsigned width) {
    return s && s->experimental_hz && s->clock_running &&
        (s->regs[2]&6u)==6u && (width==1u||width==2u||width==4u) &&
        s->tx_level<=sizeof s->tx_fifo-4u;
}
bool s5l_i2s_tx_write(s5l_i2s_t *s,uint32_t value,unsigned width) {
    if(!s || !s->experimental_hz)return false;
    if((width!=1u&&width!=2u&&width!=4u) || width>sizeof s->tx_fifo-s->tx_level) {
        s->tx_refused=saturated_add(s->tx_refused,1);return false;
    }
    for(unsigned i=0;i<width;i++) {
        s->tx_fifo[(s->tx_head+s->tx_level)%sizeof s->tx_fifo]=(uint8_t)(value>>(8*i));
        s->tx_level++;
    }
    s->tx_bytes_in=saturated_add(s->tx_bytes_in,width);return true;
}
void s5l_i2s_tick(s5l_i2s_t *s,uint32_t tb,s5l_gpioic_t *g,unsigned line) {
    if(!s || !s->clock_running || !tb)return;
    s->clock_ticks=saturated_add(s->clock_ticks,tb);
    uint64_t scaled=s->frame_phase+(uint64_t)tb*s->experimental_hz;
    uint64_t frames=scaled/S5L8900_TB_HZ;s->frame_phase=scaled%S5L8900_TB_HZ;
    s->frame_edges=saturated_add(s->frame_edges,frames);
    if(frames && (s->regs[2]&4u)) {
        uint64_t need=frames*4u;
        unsigned drained=need<s->tx_level?(unsigned)need:s->tx_level;
        uint64_t complete=s->tx_level/4u;
        if(frames>complete)s->tx_underflow_frames=saturated_add(s->tx_underflow_frames,frames-complete);
        s->tx_head=(s->tx_head+drained)%sizeof s->tx_fifo;s->tx_level-=drained;
        s->tx_bytes_out=saturated_add(s->tx_bytes_out,drained);
    }
    if(frames && g) {
        /* GPIO has one edge latch; elapsed edges coalesce while unserviced.
         * WFI forecast prevents sleeping across the first deliverable edge.
         * Pulse/ack at GPIO, no invented IIS register W1C semantics. */
        s5l_gpioic_set_line(g,line,false);
        s5l_gpioic_set_line(g,line,true);
        s5l_gpioic_set_line(g,line,false);
    }
}

static int slot_for(uint32_t off) {
    for (unsigned i = 0; i < S5L_I2S_REGS; i++)
        if (I2S_OFFSETS[i] == off) return (int)i;
    return -1;
}

void s5l_i2s_reset(s5l_i2s_t *i2s) {
    if (!i2s) return;
    memset(i2s, 0, sizeof *i2s);
}

static void note_unknown(s5l_i2s_t *i2s, uint32_t off) {
    for (unsigned i = 0; i < i2s->unknown_off_count; i++)
        if (i2s->unknown_off[i] == off) return;
    if (i2s->unknown_off_count < S5L_I2S_UNKNOWN_OFF)
        i2s->unknown_off[i2s->unknown_off_count++] = off;
}

uint32_t s5l_i2s_read(s5l_i2s_t *i2s, uint32_t off) {
    if (!i2s) return 0u;
    i2s->reads++;
    int slot = slot_for(off);
    if (slot >= 0) return i2s->regs[slot];
    /*
     * A read is already off the established path — the stock driver issues
     * none at all — so every one of them is worth recording, including reads of
     * the FIFO offsets 0x10 and 0x38. Those two are the PL080's, delivered to
     * it as physical addresses out of the device tree, and a CPU read of them
     * would mean something is doing programmed I/O where DMA was intended.
     */
    i2s->unknown_reads++;
    note_unknown(i2s, off);
    return 0u;
}

void s5l_i2s_write(s5l_i2s_t *i2s, uint32_t off, uint32_t val) {
    if (!i2s) return;
    i2s->writes++;
    int slot = slot_for(off);
    if (slot >= 0) {
        i2s->regs[slot] = val;
        clock_configure(i2s);
        return;
    }
    i2s->unknown_writes++;
    note_unknown(i2s, off);
}
