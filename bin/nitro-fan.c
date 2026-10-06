// nitro-fan — tiny root-owned EC fan duty setter/reader, exec'd by nitro-priv.
// Compiled C (ioperm) because shell can't do port I/O.
// Usage: nitro-fan <0..100|auto|status>
// Protocol verified on Acer Nitro AN515-58 (identical to nbfc's writes):
//   unlock reg 0x03=0x51 → manual 0x34=0x0C (CPU), 0x33=0x30 (GPU)
//   duty bytes 0x37 (CPU), 0x3A (GPU), 0..100 percent
//   tach: word 0x13/0x14 (CPU), 0x15/0x16 (GPU) — duty-of-8500 scale
//   auto: manual bits back to 0x00, EC firmware curve resumes
// status prints: "<cpuWord> <gpuWord> <autoFlag>"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>

static void wibf(void){ while (inb(0x66) & 0x02) {} }
static void wobf(void){ int t = 100000; while (!(inb(0x66) & 0x01) && --t) {} }
static unsigned char rd(unsigned char r)
{
    wibf(); outb(0x80, 0x66);
    wibf(); outb(r, 0x62);
    wobf(); return inb(0x62);
}
// EC re-locks after EVERY write — the unlock (0x03=0x51) must precede each
// register write, not once per command batch (nbfc re-unlocks every cycle;
// a single unlock made only the first write stick, the rest silently dropped).
static void ecwr(unsigned char r, unsigned char v)
{
    // unlock: cmd 0x81, reg 0x03, value 0x51 — the EC re-locks after EVERY
    // write, so this must precede each register write (a single unlock at
    // batch start made only the first write stick — the stuck-fan bug).
    wibf(); outb(0x81, 0x66);
    wibf(); outb(0x03, 0x62);
    wibf(); outb(0x51, 0x62);
    // then the real write
    wibf(); outb(0x81, 0x66);
    wibf(); outb(r, 0x62);
    wibf(); outb(v, 0x62);
}

int main(int argc, char **argv)
{
    if (geteuid() != 0) { fprintf(stderr, "root only\n"); return 1; }
    if (argc != 2) { fprintf(stderr, "usage: nitro-fan <0..100|auto|status>\n"); return 2; }
    if (ioperm(0x62, 1, 1) || ioperm(0x66, 1, 1)) { perror("ioperm"); return 1; }

    if (strcmp(argv[1], "status") == 0) {
        // Tach words are trustworthy; the 0x33/0x34 mode registers are NOT —
        // they span EC banks and every read returns a different view
        // (observed: 0x00→0x0C→0x03 within seconds with no writer alive).
        // So the mode we report = the mode we last commanded, persisted.
        const unsigned cw = rd(0x13) | (rd(0x14) << 8);
        const unsigned gw = rd(0x15) | (rd(0x16) << 8);
        int isAuto = 0;
        FILE *f = fopen("/var/lib/nitro-control/fanmode", "r");
        if (f) {
            int v = 0;
            if (fscanf(f, "%d", &v) == 1)
                isAuto = v;
            fclose(f);
        }
        FILE *sf = fopen("/var/lib/nitro-control/fanduty", "r");
        int cmd = -9;
        if (sf) { if (fscanf(sf, "%d", &cmd) != 1) cmd = -9; fclose(sf); }
        printf("%u %u %d %d\n", cw, gw, isAuto, cmd);
        return 0;
    }
    if (strcmp(argv[1], "auto") == 0) {
        ecwr(0x03, 0x51);   // unlock
        ecwr(0x34, 0x04);   // CPU fan → auto (firmware curve; nbfc's reset value)
        ecwr(0x33, 0x10);   // GPU fan → auto
        FILE *f = fopen("/var/lib/nitro-control/fanmode", "w");
        if (f) { fprintf(f, "1"); fclose(f); }
        f = fopen("/var/lib/nitro-control/fanduty", "w");
        if (f) { fprintf(f, "-1"); fclose(f); }
        printf("fan: auto (EC curve)\n");
        return 0;
    }
    const int pct = atoi(argv[1]);
    if (pct < 0 || pct > 100) { fprintf(stderr, "pct 0..100\n"); return 2; }
    const unsigned char p = (unsigned char)pct;

    ecwr(0x03, 0x51);       // unlock
    ecwr(0x34, 0x0C);       // CPU fan manual
    ecwr(0x33, 0x30);       // GPU fan manual
    ecwr(0x37, p);          // CPU duty
    ecwr(0x3A, p);          // GPU duty
    FILE *f = fopen("/var/lib/nitro-control/fanmode", "w");
    if (f) { fprintf(f, "0"); fclose(f); }
    f = fopen("/var/lib/nitro-control/fanduty", "w");
    if (f) { fprintf(f, "%d", pct); fclose(f); }
    printf("fan: %d%%\n", pct);
    return 0;
}
