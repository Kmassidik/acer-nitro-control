// nitro-fan — tiny root-owned EC fan duty setter/reader, exec'd by nitro-priv.
// Compiled C (ioperm) because shell can't do port I/O.
// Usage:
//   nitro-fan status                          → "cpuWord gpuWord auto flag"
//   nitro-fan auto                            → firmware curve
//   nitro-fan <0..100>                        → BOTH fans at pct
//   nitro-fan cpu <0..100>                    → CPU fan only
//   nitro-fan gpu <0..100>                    → GPU fan only
// Protocol verified on Acer Nitro AN515-58 (identical to nbfc's writes):
//   unlock reg 0x03=0x51 → manual 0x34=0x0C (CPU), 0x33=0x30 (GPU)
//   duty bytes 0x37 (CPU), 0x3A (GPU), 0..100 percent
//   tach: word 0x13/0x14 (CPU), 0x15/0x16 (GPU) — duty-of-8500 scale
//   auto: manual bits → 0x04/0x10 (nbfc's documented reset values), curve resumes
// Persisted state: /var/lib/nitro-control/{fanmode,fanduty} = last commanded.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <ctime>

static void wibf(void){ while (inb(0x66) & 0x02) {} }
static void wobf(void){ int t = 100000; while (!(inb(0x66) & 0x01) && --t) {} }
static unsigned char rd(unsigned char r)
{
    wibf(); outb(0x80, 0x66);
    wibf(); outb(r, 0x62);
    wobf(); return inb(0x62);
}
static void ecwr(unsigned char r, unsigned char v)
{
    wibf(); outb(0x81, 0x66);
    wibf(); outb(r, 0x62);
    wibf(); outb(v, 0x62);
}
static void save(const char *which, const char *val)
{
    char path[64];
    snprintf(path, sizeof path, "/var/lib/nitro-control/%s", which);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%s", val); fclose(f); }
}

int main(int argc, char **argv)
{
    if (geteuid() != 0) { fprintf(stderr, "root only\n"); return 1; }
    if (argc < 2 || argc > 3) { fprintf(stderr, "usage: nitro-fan <0..100|auto|cpu PCT|gpu PCT|status>\n"); return 2; }
    if (ioperm(0x62, 1, 1) || ioperm(0x66, 1, 1)) { perror("ioperm"); return 1; }

    // SERVER-SIDE auto-guard: after 'auto' is committed, manual duty writes
    // are rejected for 5 s (UI bugs can't fight the curve — the helper is
    // the kernel-side single point of truth). Late check readdir mtime.
    struct stat st{};
    const int isManualWrite = (strcmp(argv[1], "auto") != 0 && strcmp(argv[1], "status") != 0);
    if (isManualWrite && stat("/var/lib/nitro-control/fanmode", &st) == 0) {
        FILE *f = fopen("/var/lib/nitro-control/fanmode", "r");
        int mode = 0;
        if (f) { if (fscanf(f, "%d", &mode) != 1) mode = 0; fclose(f); }
        const double age = difftime(time(nullptr), st.st_mtime);
        if (mode == 1 && age < 5.0) {
            fprintf(stderr, "rejected: auto just applied (%.1fs ago)\n", age);
            return 3;
        }
    }

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
        int cmd = -9;
        f = fopen("/var/lib/nitro-control/fanduty", "r");
        if (f) { if (fscanf(f, "%d", &cmd) != 1) cmd = -9; fclose(f); }
        int cmdC = -9, cmdG = -9;
        f = fopen("/var/lib/nitro-control/fanduty_cpu", "r");
        if (f) { if (fscanf(f, "%d", &cmdC) != 1) cmdC = -9; fclose(f); }
        f = fopen("/var/lib/nitro-control/fanduty_gpu", "r");
        if (f) { if (fscanf(f, "%d", &cmdG) != 1) cmdG = -9; fclose(f); }
        printf("%u %u %d %d %d %d\n", cw, gw, isAuto, cmd, cmdC, cmdG);
        return 0;
    }
    if (strcmp(argv[1], "auto") == 0) {
        ecwr(0x03, 0x51);   // unlock
        ecwr(0x34, 0x04);   // CPU fan → auto (firmware curve; nbfc's reset value)
        ecwr(0x33, 0x10);   // GPU fan → auto
        save("fanmode", "1");
        save("fanduty", "-1");
        save("fanduty_cpu", "-1");
        save("fanduty_gpu", "-1");
        printf("fan: auto (EC curve)\n");
        return 0;
    }
    if (strcmp(argv[1], "cpu") == 0 || strcmp(argv[1], "gpu") == 0) {
        if (argc != 3) { fprintf(stderr, "usage: nitro-fan %s <0..100>\n", argv[1]); return 2; }
        const int pct = atoi(argv[2]);
        if (pct < 0 || pct > 100) { fprintf(stderr, "pct 0..100\n"); return 2; }
        const unsigned char p = (unsigned char)pct;
        const int isCpu = strcmp(argv[1], "cpu") == 0;
        ecwr(0x03, 0x51);           // unlock
        ecwr(isCpu ? 0x34 : 0x33, isCpu ? 0x0C : 0x30);   // THIS fan manual
        ecwr(isCpu ? 0x37 : 0x3A, p);                     // THIS fan duty
        char b[8];
        snprintf(b, sizeof b, "%d", pct);
        save(isCpu ? "fanduty_cpu" : "fanduty_gpu", b);
        save("fanmode", "0");
        printf("fan %s: %d%%\n", isCpu ? "cpu" : "gpu", pct);
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
    char b[8];
    snprintf(b, sizeof b, "%d", pct);
    save("fanmode", "0");
    save("fanduty", b);
    save("fanduty_cpu", b);
    save("fanduty_gpu", b);
    printf("fan: %d%%\n", pct);
    return 0;
}
