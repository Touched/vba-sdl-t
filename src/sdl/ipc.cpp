#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include "../System.h"
#include "../gba/GBA.h"
#include "../Util.h"
#include "fex/fex.h"
#include "../../fex/fex/fex.h"
#include "../NLS.h"

char *szFile = NULL;
static int romSize = 0x2000000;

bool inputAvailable() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(0, &fds));
}

void ipc_init() {

}

void ipc_set_rom(char *file) {
    szFile = file;
}

fex_t *scan_arc(const char *file, bool (*accept)(const char *),
                char (&buffer)[2048]) {
    fex_t *fe;
    fex_err_t err = fex_open(&fe, file);
    if (!fe) {
        systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s: %s"), file, err);
        return NULL;
    }

    // Scan filenames
    bool found = false;
    while (!fex_done(fe)) {
        strncpy(buffer, fex_name(fe), sizeof buffer);
        buffer[sizeof buffer - 1] = '\0';

        utilStripDoubleExtension(buffer, buffer);

        if (accept(buffer)) {
            found = true;
            break;
        }

        fex_err_t err = fex_next(fe);
        if (err) {
            systemMessage(MSG_BAD_ZIP_FILE, N_("Cannot read archive %s: %s"), file, err);
            fex_close(fe);
            return NULL;
        }
    }

    if (!found) {
        systemMessage(MSG_NO_IMAGE_ON_ZIP,
                      N_("No image found in file %s"), file);
        fex_close(fe);
        return NULL;
    }
    return fe;
}

void ipc_iteration() {
    char strbuffer[100];

    if (inputAvailable()) {
        fgets(strbuffer, 100, stdin);
    }

    if (!strcmp(strbuffer, "reload\n")) {
        if (szFile) {
            // Replace with something that doesn't malloc
            //utilLoad(szFile, utilIsGBAImage, rom, romSize);
            char buffer[2048];
            fex_t *fe = scan_arc(szFile, utilIsGBAImage, buffer);

            fex_err_t err = fex_stat(fe);
            int fileSize = fex_size(fe);

            err = fex_read(fe, rom, fileSize <= romSize ? fileSize : romSize);
            fex_close(fe);
        }
        systemScreenMessage("Reloaded");
    }
}

void ipc_end() {

}