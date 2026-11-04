#define main sprite_runner_main
#include "../sprite_runner.c"
#undef main
#define GDS 0x600000
#define MAR 0x610000
#define STA 0x620000
#define STUB 0x700000
static int count;
static unsigned target[24];
static void rdfile(const char *path, unsigned at, unsigned max) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        exit(2);
    }
    fread(mem + at, 1, max, f);
    fclose(f);
}
int oracle_init(const char *module, const char *rest, const char *image) {
    if (!mem) {
        mem = calloc(MEM_SIZE, 1);
        written = calloc(SCR_W * SCR_H, 1);
    } else
        memset(mem, 0, MEM_SIZE);
    rdfile(image, 0, 0x20000);
    FILE *f = fopen(image, "rb");
    if (!f)
        return 0;
    fseek(f, 0x100000, SEEK_SET);
    fread(mem + DATA_BASE, 1, 0x2600, f);
    fclose(f);
    long n = load(module);
    rdfile(rest, STA, 0x4000);
    mem[RET_MAGIC] = 0x60;
    mem[RET_MAGIC + 1] = 0xfe;
    mem[STUB] = 0x70;
    mem[STUB + 1] = 0;
    mem[STUB + 2] = 0x4e;
    mem[STUB + 3] = 0x75;
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    /* Replace compiled pixel blitters and OS I/O wrappers, not simulation code. */
    for (unsigned i = 0; i + 4 < (unsigned)n; i += 2)
        if (mem[MOD_BASE + i] == 0x2f && mem[MOD_BASE + i + 1] == 9 &&
            mem[MOD_BASE + i + 2] == 0x22 && mem[MOD_BASE + i + 3] == 0x40) {
            mem[MOD_BASE + i] = 0x4e;
            mem[MOD_BASE + i + 1] = 0x75;
        }
    for (unsigned i = 0; i + 2 < (unsigned)n; i += 2)
        if (m68k_read_memory_16(MOD_BASE + i) == 0x4e40) {
            mem[MOD_BASE + i] = 0x02;
            mem[MOD_BASE + i + 1] = 0x3c;
            mem[MOD_BASE + i + 2] = 0;
            mem[MOD_BASE + i + 3] = 0xfe;
        }
    unsigned fn = 0;
    for (unsigned i = 0x30; i + 4 < (unsigned)n; i += 2) {
        if (m68k_read_memory_16(MOD_BASE + i) == 0x4e55)
            fn = i;
        if (fn && i - fn < 80 && m68k_read_memory_16(MOD_BASE + i) == 0x4e40) {
            mem[MOD_BASE + fn] = 0x4e;
            mem[MOD_BASE + fn + 1] = 0x75;
        }
    }
    memmove(mem + OBJ_BASE, mem + STA + 0xd4, 0x3b14);
    memmove(mem + STA, mem + STA + 0x40, 0x94);
    m68k_write_memory_32(GDS + 0x244, STA);
    m68k_write_memory_32(GDS + 0x248, OBJ_BASE);
    m68k_write_memory_32(GDS + 0x250, MAR);
    m68k_write_memory_32(GDS + 0x7c, SCR_BASE);
    m68k_write_memory_32(GDS + 0x80, SCR_BASE + 0x1aaac);
    for (int i = 0; i < 5; i++)
        m68k_write_memory_16(GDS + 0x49c + 2 * i, 72 + 36 * i);
    for (int i = 0; i < 7; i++)
        m68k_write_memory_16(GDS + 0x4a6 + 2 * i, 44 + 44 * i);
    for (int floor = 0; floor < 5; floor++)
        for (int c = 0; c < 7; c++)
            if (m68k_read_memory_16(STA + 0x12 + floor * 14 + c * 2) & 0xf000) {
                int center = (44 + 44 * c) / 2;
                for (int u = center - 5; u <= center + 5; u++)
                    mem[GDS + 0x4b4 + floor * 23 + (u >> 3)] |= 1 << (u & 7);
            }
    for (int i = 0x838; i <= 0x8c4; i += 4)
        m68k_write_memory_32(GDS + i, STUB);
    m68k_write_memory_32(DATA_BASE + 0x310, GDS);
    m68k_write_memory_16(GDS + 0x6a, 1);
    m68k_write_memory_16(GDS + 0x494, 1);
    count = m68k_read_memory_16(OBJ_BASE);
    if (count > 24)
        return 0;
    unsigned table = MOD_BASE + m68k_read_memory_32(MOD_BASE + 0x30);
    for (int i = 0; i < count; i++) {
        unsigned actor = OBJ_BASE + 0x34 + i * 0x274, type = m68k_read_memory_16(actor + 4);
        target[i] = 0;
        for (int j = 0; j < 25; j++) {
            unsigned id = m68k_read_memory_16(OBJ_BASE + 2 + j * 2);
            if (!id)
                break;
            if (id == type) {
                target[i] = m68k_read_memory_32(table + j * 8 + 4);
                break;
            }
        }
        if (!target[i])
            return 0;
        m68k_write_memory_32(actor, MOD_BASE + target[i]);
        m68k_write_memory_16(actor + 0x26, 0);
        m68k_write_memory_16(actor + 0x22, 0);
    }
    return count;
}
void oracle_setup(int h, int stage) {
    m68k_write_memory_16(GDS + 0x7a6, h);
    m68k_write_memory_16(GDS + 0x7a8, stage);
    m68k_write_memory_16(GDS + 0x7aa, 3);
    m68k_write_memory_32(GDS + 0x826, m68k_read_memory_32(STA + 4));
    m68k_write_memory_32(GDS + 0x82a, m68k_read_memory_32(STA + 4));
    for (int i = 0; i < count; i++)
        if (call(target[i], OBJ_BASE + 0x34 + i * 0x274, GDS) == -1) {
            fprintf(stderr, "oracle init runaway %d\n", i);
            exit(3);
        }
    m68k_write_memory_32(GDS + 0x826, 12345);
    m68k_write_memory_32(GDS + 0x82a, 12345);
}
int oracle_tick(int t, int mode) {
    int floor = (t / 80) % 5;
    m68k_write_memory_16(MAR + 0x10e, 44 + (t * 7) % 264);
    m68k_write_memory_16(MAR + 0x112, floor);
    m68k_write_memory_16(MAR + 0x110, 72 + 36 * floor - 30);
    m68k_write_memory_16(MAR + 0x11a, mode && t % 150 < 40 ? 4 : 0);
    m68k_write_memory_16(MAR + 0x11e, 0);
    m68k_write_memory_16(GDS + 0x46a, t * 6);
    m68k_write_memory_16(GDS + 0x494, 1 + t % 2);
    if (t == 700)
        m68k_write_memory_16(GDS + 0x53a, 1);
    for (int phase = 1; phase <= 2; phase++)
        for (int i = 0; i < count; i++) {
            unsigned a = OBJ_BASE + 0x34 + i * 0x274;
            m68k_write_memory_16(a + 0x22, phase);
            if (call(target[i], a, GDS) == -1)
                return 0;
            int type = m68k_read_memory_16(a + 4);
            if (mode && phase == 2 && (t == 90 || t == 91 || t == 300) && type > 9) {
                m68k_write_memory_16(a + 0x22, t == 300 ? 4 : 3);
                if (call(target[i], a, GDS) == -1)
                    return 0;
            }
        }
    if (t == 700)
        for (int i = 0; i < count; i++) {
            unsigned a = OBJ_BASE + 0x34 + i * 0x274;
            m68k_write_memory_16(a + 0x22, 4);
            if (call(target[i], a, GDS) == -1)
                return 0;
        }
    return 1;
}
int oracle_actor(int i, int off) {
    unsigned a = OBJ_BASE + 0x34 + i * 0x274 + off;
    return off >= 0x254 ? (int32_t)m68k_read_memory_32(a) : (int16_t)m68k_read_memory_16(a);
}
int oracle_gds(int off) {
    return off == 0x826 || off == 0x82a ? (int32_t)m68k_read_memory_32(GDS + off)
                                        : (int16_t)m68k_read_memory_16(GDS + off);
}

void oracle_seed(unsigned seed) {
    m68k_write_memory_32(GDS + 0x826, seed);
    m68k_write_memory_32(GDS + 0x82a, seed);
}
void oracle_patch(unsigned address) {
    m68k_write_memory_16(address, 0x7000);
    m68k_write_memory_16(address + 2, 0x4e75);
}
int oracle_main_call(unsigned address, unsigned d0, unsigned d1) {
    return call(address - MOD_BASE, d0, d1) != -1;
}
void oracle_lift_base(int upper) {
    int h = m68k_read_memory_16(GDS + 0x7a6);
    if (h >= 7 && !upper)
        for (int c = 0; c < 7; c++)
            if ((m68k_read_memory_16(STA + 0x12 + c * 2) & 0xf000) == 0xf000) {
                m68k_write_memory_16(STA + 0x12 + c * 2, 0xb000);
                break;
            }
    m68k_write_memory_16(GDS + 0x538, 1);
}
int oracle_cell(int i) {
    return m68k_read_memory_16(STA + 0x12 + i * 2);
}
