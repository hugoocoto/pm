/*
 * This file is part of pm (https://github.com/hugoocoto/pm).
 * Copyright (c) 2025 Hugo Coto.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 2

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct Package {
        char *name;
        char *git;
        char *recipe;
        char *branch;
} Package;

struct {
        Package *packages;
        int count;
} packarr = { 0 };

void pmupdateall();
void pmupdate(Package p);
void pmadd(Package p);
void pmlist();

static char *bin_path = "/home/hugo/.local/share/pm/bin";
static char *clone_path = "/home/hugo/.local/share/pm/cache";
static char *state_path = "/home/hugo/.local/share/pm/state";

#ifndef strdup
#define strdup(s) memcpy(malloc(strlen(s) + 1), (s), strlen(s) + 1);
#endif

char *
strjoin(char *buf, char *left, char *right, char sep)
{
        /* Unsafe */
        size_t ls = strlen(left);
        size_t rs = strlen(right);
        memcpy(buf, left, ls);
        memcpy(buf + ls + 1, right, rs);
        buf[ls + rs + 1] = 0;
        buf[ls] = sep;
        return buf;
}

char **
strsplit(char *str, char sep, char *tofree)
{
        char **arr = NULL;
        int arrlen = 0;
        char *c;
        char *s = tofree = strdup(str);
        while ((c = strchr(s, sep))) {
                *c = 0;
                arr = realloc(arr, sizeof(char *) * (arrlen + 1));
                arr[arrlen] = s;
                ++arrlen;
                s = c + 1;
        }
        arr = realloc(arr, sizeof(char *) * (arrlen + 2));
        arr[arrlen] = s;
        ++arrlen;

        arr[arrlen] = NULL;

        return arr;
}


int
create_path_if_not_exits(char *path)
{
        char *c;
        int s;
        char *p;

        c = p = strdup(path);
        while ((c = strchr(c + 1, '/'))) {
                *c = 0;
                s = mkdir(p, 0777);
                *c = '/';
                if (s && errno != EEXIST) return free(p), s;
        }

        if (*path) {
                s = mkdir(p, 0777);
                if (s && errno != EEXIST) return free(p), s;
        }

        return free(p), 0;
}

void
pmlist()
{
        int i = 0;
        puts("Installed Packages:");
        for (; i < packarr.count; i++) {
                printf("  - %s (%s)\n", packarr.packages[i].name, packarr.packages[i].git);
        }
}

void
pmadd(Package p)
{
        packarr.packages = realloc(packarr.packages,
                                   sizeof p * (packarr.count + 1));
        packarr.packages[packarr.count] = p;
        packarr.count++;
        return;
}

int
is_installed(Package p)
{
        struct stat s;
        if (stat(strjoin((char[128]) { 0 }, bin_path, p.name, '/'), &s)) {
                /* Should handle errors */
                return 0;
        }
        return 1;
}

int
is_cloned(Package p)
{
        struct stat s;
        if (stat(strjoin((char[128]) { 0 }, clone_path, p.name, '/'), &s)) {
                /* Should handle errors */
                return 0;
        }
        return 1;
}

int
run(char **argv)
{
        int pid;
        char **arg = argv;
        int status;

        printf("[RUN ]");
        do {
                printf(" %s", *arg);
        } while ((*++arg));
        putchar(10);

        switch (pid = fork()) {
        case -1:
                perror("fork");
                exit(1);
        case 0:
                execvp(argv[0], argv);
                perror("execlp");
                exit(0);

        default:
                waitpid(pid, &status, 0);
                return status;
        }
}

int
run_get(const char *cmd, char *out, size_t len)
{
        printf("[RUN ] %s\n", cmd);
        FILE *fp = popen(cmd, "r");
        if (!fp) return 1;

        if (fgets(out, len, fp) == NULL) {
                pclose(fp);
                return 1;
        }

        out[strcspn(out, "\n")] = '\0'; /* trim newline */
        pclose(fp);
        return 0;
}

int
chdircloned(Package p)
{
        char path[127];
        strjoin(path, clone_path, p.name, '/');
        if (chdir(path)) {
                perror("chdir");
                return 1;
        }
        return 0;
}

int
need_update(Package p)
{
        char local[128];
        char remote[128];
        char cmd[128];

        if (chdircloned(p)) return 0;

        strjoin(cmd, "git rev-parse origin", p.branch, '/');
        run((char *[]) { "git", "fetch", "--all", NULL });

        if (run_get("git rev-parse HEAD", local, sizeof(local)) ||
            run_get(cmd, remote, sizeof(remote))) {
                perror("RUN_GET");
                return 0;
        }

        return strcmp(local, remote);
}

void clone(Package p);
void build(Package p);

void
build(Package p)
{
        char path[127];
        char bp[128] = { 0 };
        char ep[128] = { 0 };
        char **recipel;
        char *b;

        if (!is_cloned(p)) clone(p);

        strjoin(path, clone_path, p.name, '/');
        if (chdir(path)) {
                perror("chdir");
                exit(0);
        }

        run(recipel = strsplit(p.recipe, ' ', b));
        free(recipel);
        free(b);

        strjoin(bp, bin_path, p.name, '/');
        strjoin(ep, path, p.name, '/');

        printf("[LINK] %s ---> %s\n", ep, bp);
        if (link(ep, bp)) {
                perror("link");
        }
}

void
clone(Package p)
{
        char path[127];
        if (is_cloned(p)) {
                printf("Package %s already downloaded\n", p.name);
                return;
        }

        strjoin(path, clone_path, p.name, '/');
        run((char *[]) { "git", "clone", "-b", p.branch, p.git, path, NULL });
}

void
pminstall(Package p)
{
        switch (fork()) {
        case -1:
                perror("fork");
                exit(0);
        case 0: break;
        default: return;
        }

        if (!is_cloned(p)) clone(p);
        build(p);
        exit(0);
}


void
update(Package p)
{
        char b[32];
        char path[64];

        strjoin(path, clone_path, p.name, '/');
        if (chdir(path)) {
                perror("chdir");
                exit(0);
        }

        strjoin(b, "origin", p.branch, '/');
        run((char *[]) { "git", "fetch", "--all", NULL });
        run((char *[]) { "git", "reset", "--hard", b, NULL });
}

void
pmupdate(Package p)
{
        switch (fork()) {
        case -1:
                perror("fork");
                exit(0);
        case 0: break;
        default: return;
        }

        if (!need_update(p)) {
                printf("[SKIP] package %s already updated\n", p.name);
                exit(0);
        }

        update(p);
        build(p);

        exit(0);
}

void
pmupdateall()
{
        int i = 0;
        for (; i < packarr.count; i++) {
                printf("[NAME] %s\n", packarr.packages[i].name);
                if (!is_installed(packarr.packages[i])) {
                        pminstall(packarr.packages[i]);
                        wait(NULL); // or not
                        continue;
                }
                pmupdate(packarr.packages[i]);
                wait(NULL); // or not
        }
        return;
}

#define PAK(...)                        \
        pmadd((Package) {               \
        .recipe = "make", /* default */ \
        .branch = "main", /* default */ \
        __VA_ARGS__ });

int
main()
{
        create_path_if_not_exits(bin_path);
        create_path_if_not_exits(clone_path);
        create_path_if_not_exits(state_path);

        PAK(.name = "pm",
            .git = "https://github.com/hugoocoto/pm")

/*   */ #include "pm.config"

        pmupdateall();
        pmlist();
}
