#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

#define MAX_PATH 512

// delete the right space
char*
rtrim(char *name) {
    static char cleaned[DIRSIZ + 1];
    int i = DIRSIZ - 1;

    // find the last char
    while (i >= 0 && name[i] == ' ')
        i--;

    // copy the char
    memmove(cleaned, name, i + 1);
    cleaned[i + 1] = '\0';

    return cleaned;
}

// recursive search for files
void
find(const char *path, const char *target_name) {
    int fd;
    struct stat st;
    struct dirent de;
    char buf[MAX_PATH], *p;

    // open the target path
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // get the status
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // input path is a file:error
    if (st.type == T_FILE || st.type == T_DEVICE) {
        fprintf(2, "find: %s is not a directory\n", path);
        close(fd);
        return;
    }

    //ensure the length
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
        fprintf(2, "find: path too long\n");
        close(fd);
        return;
    }

    // copy the initial path
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    // traversing the path
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;

        char *clean_name = rtrim(de.name);
        if (strcmp(clean_name, ".") == 0 || strcmp(clean_name, "..") == 0)
            continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = '\0';

        if (stat(buf, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }

        if (st.type == T_FILE || st.type == T_DEVICE) {
            if (strcmp(clean_name, target_name) == 0) {
                printf("%s\n", buf);
            }
        } else if (st.type == T_DIR) {
            find(buf, target_name);  // recursive
        }
    }

    close(fd);
}

int
main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find <path> <filename>\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}
