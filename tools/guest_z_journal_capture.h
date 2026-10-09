/* Read-only, bounded recovery of our own diagnostic's ASCII journal from RAM.
 * This is not a filesystem extraction or a complete machine snapshot. MIT. */
#ifndef S5LBOX_GUEST_Z_JOURNAL_CAPTURE_H
#define S5LBOX_GUEST_Z_JOURNAL_CAPTURE_H
static void guest_z_journal_capture(const s5l8900_t *m) {
    const char *directory = getenv("S5LBOX_Z_JOURNAL_DIR");
    if (!directory || !*directory || strlen(directory) > 1000 || !m->ram) return;
    const char prefix[] = "RM_MAIN_ENTER\nRM_PID=";
    unsigned count = 0;
    for (size_t offset = 0; offset + sizeof prefix <= m->ram_size && count < 8; offset++) {
        if (memcmp(m->ram + offset, prefix, sizeof prefix - 1)) continue;
        size_t length = 0; bool marker = false;
        while (length < 32768 && offset + length < m->ram_size) {
            unsigned char c = m->ram[offset + length];
            if (!c || (c != '\n' && (c < 32 || c > 126))) break;
            if (offset + length + 10 <= m->ram_size &&
                !memcmp(m->ram + offset + length, "RMZ_BEGIN\n", 10)) marker = true;
            length++;
        }
        if (!marker) continue;
        char path[1100];
        int n = snprintf(path, sizeof path, "%s/zjournal-%u-at-%08x.log", directory,
                         count, (unsigned)(m->ram_base + offset));
        if (n < 0 || (size_t)n >= sizeof path) return;
        FILE *out = fopen(path, "wbx");
        if (!out) { fprintf(stderr, "ZJOURNAL_CAPTURE refused existing/unwritable output\n"); return; }
        bool ok = fwrite(m->ram + offset, 1, length, out) == length;
        if (fclose(out)) ok = false;
        fprintf(stderr, "ZJOURNAL_CAPTURE pa=%08x bytes=%zu success=%u path=%s\n",
                (unsigned)(m->ram_base + offset), length, ok, path);
        count++;
    }
    fprintf(stderr, "ZJOURNAL_CAPTURE matches=%u scope=own-ASCII-journal-only\n", count);
}
#endif
