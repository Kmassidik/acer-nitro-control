// Root-side EC fan ops (ioperm). Deployed as part of nitro-priv; this TU is
// linked into nitro-control only for the selftest/CLI probes — real duty
// changes go through the root helper so the GUI stays unprivileged.
#include "ecfan.h"

#include <unistd.h>
#include <sys/io.h>

namespace ecfan {

static bool s_ready = false;

bool init(void)
{
    if (s_ready)
        return true;
    s_ready = (ioperm(0x62, 1, 1) == 0) && (ioperm(0x66, 1, 1) == 0);
    return s_ready;
}

static void wibf(void){ while (inb(0x66) & 0x02) {} }
static void wobf(void){ int t = 100000; while (!(inb(0x66) & 0x01) && --t) {} }

static unsigned char rd(unsigned char r)
{
    wibf(); outb(0x80, 0x66);
    wibf(); outb(r, 0x62);
    wobf(); return inb(0x62);
}

static void wr(unsigned char r, unsigned char v)
{
    wibf(); outb(0x81, 0x66);
    wibf(); outb(r, 0x62);
    wibf(); outb(v, 0x62);
}

static void unlock(void) { wr(0x03, 0x51); }

int duty(int fanIdx)
{
    if (!init())
        return -1;
    unsigned w = fanIdx == 0 ? (rd(0x13) | (rd(0x14) << 8))
                             : (rd(0x15) | (rd(0x16) << 8));
    return (int)(w * 100.0 / 8500.0 + 0.5);
}

void setManual(int pct)
{
    if (!init())
        return;
    const unsigned char p = (unsigned char)(pct < 0 ? 0 : pct > 100 ? 100 : pct);
    unlock();
    wr(0x34, 0x0C);
    wr(0x33, 0x30);
    wr(0x37, p);
    wr(0x3A, p);
}

void setAuto(void)
{
    if (!init())
        return;
    unlock();
    wr(0x34, 0x00);
    wr(0x33, 0x00);
}

int tempC(void)
{
    if (!init())
        return -1;
    return rd(0x07);
}

} // namespace ecfan
