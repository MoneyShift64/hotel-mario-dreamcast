// Minimal CHD (CD) -> raw 2352-byte-sector .bin extractor using libchdr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libchdr/chd.h>

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s in.chd out.bin\n", argv[0]); return 1; }
    chd_file *chd; chd_error e = chd_open(argv[1], CHD_OPEN_READ, NULL, &chd);
    if (e) { fprintf(stderr, "open: %s\n", chd_error_string(e)); return 1; }
    const chd_header *h = chd_get_header(chd);
    printf("hunkbytes=%u totalhunks=%u unitbytes=%u\n", h->hunkbytes, h->totalhunks, h->unitbytes);
    char meta[512]; uint32_t len, tag; uint8_t fl;
    for (int i = 0; ; i++) {
        if (chd_get_metadata(chd, CDROM_TRACK_METADATA2_TAG, i, meta, sizeof meta, &len, &tag, &fl)) break;
        meta[len < sizeof meta ? len : sizeof meta - 1] = 0; printf("track: %s\n", meta);
    }
    FILE *o = fopen(argv[2], "wb");
    uint8_t *buf = malloc(h->hunkbytes);
    uint32_t frames_per_hunk = h->hunkbytes / h->unitbytes;
    for (uint32_t i = 0; i < h->totalhunks; i++) {
        if ((e = chd_read(chd, i, buf))) { fprintf(stderr, "read %u: %s\n", i, chd_error_string(e)); break; }
        for (uint32_t f = 0; f < frames_per_hunk; f++) fwrite(buf + f * h->unitbytes, 1, 2352, o);
    }
    fclose(o); chd_close(chd); return 0;
}
