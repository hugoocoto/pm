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
} Package;

struct {
        Package *packages;
        int count;
} packarr = { 0 };

int pmupdate();       /* Update/install all packages */
int pmadd(Package p); /* Add a package to package list */
void pmlist();        /* Print package list */

static char *bin_path = "/home/hugo/.local/share/pm/bin/";
static char *state_path = "/home/hugo/.local/share/pm/state/";

#ifndef strdup
#define strdup(s) memcpy(malloc(strlen(s) + 1), (s), strlen(s) + 1);
#endif

char *
join(char *buf, char *left, char *right, char sep)
{
        /* Unsafe */
        size_t ls = strlen(left);
        size_t rs = strlen(right);
        memcpy(buf, left, ls);
        memcpy(buf + ls + 1, right, rs);
        buf[ls + rs + 1] = 0;
        buf[ls] = sep;
        printf("join %s and %s with %c ==> %s\n", left, right, sep, buf);
        return buf;
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
                s = mkdir(p, 0755);
                *c = '/';
                if (s && errno != EEXIST) return free(p), s;
        }

        if (*path) {
                s = mkdir(p, 0600);
                if (s && errno != EEXIST) return free(p), s;
        }

        return free(p), 0;
}

void
add_path_to_PATH(char *path)
{
        char *epath = strdup(getenv("PATH") ?: "");
        epath = realloc(epath, strlen(epath) + strlen(path) + 1);
        strcat(epath, path);
        free(epath);
}

void
pmlist()
{
        int i = 0;
        for (; i < packarr.count; i++) {
                printf("Name: %s (%s)\n", packarr.packages[i].name, packarr.packages[i].git);
        }
}

int
pmadd(Package p)
{
        packarr.packages = realloc(packarr.packages,
                                   sizeof p * (packarr.count + 1));
        packarr.packages[packarr.count] = p;
        packarr.count++;
        return 0;
}

int
is_installed(Package p)
{
        struct stat s;
        if (stat(join((char[128]) { 0 }, bin_path, p.name, '/'), &s)) {
                /* Should handle errors */
                return 0;
        }
        return 1;
}

int
pminstall(Package p)
{
        char path[64];
        int pid;

        if (is_installed(p)) {
                printf("Package %s already installed. Skip update check\n",
                       p.name);
                return 1;
        }

        sprintf(path, "%s/%s", bin_path, p.name);
        switch (pid = fork()) {
        case -1:
                perror("fork");
                exit(1);
        case 0:
                printf("Run: %s %s %s %s %s\n", "git", "git", "clone", p.git, path);
                execlp("git", "git", "clone", p.git, path, NULL);
                perror("execlp");
                exit(0);

        default:
                waitpid(pid, NULL, 0);
        }


        return 0;
}

int
pmupdate()
{
        int i = 0;
        for (; i < packarr.count; i++) {
                if (is_installed(packarr.packages[i])) {
                        printf("Package %s already installed. Skip update check\n",
                               packarr.packages[i].name);
                        continue;
                }
                pminstall(packarr.packages[i]);
        }
        return 0;
}

int
main()
{
        create_path_if_not_exits(bin_path);
        create_path_if_not_exits(state_path);

        add_path_to_PATH(bin_path);

        pmadd((Package) {
        .name = "st",
        .git = "https://git.suckless.org/st",
        .recipe = "make",
        });

        pmadd((Package) {
        .name = "vicel",
        .git = "https://github.com/hugoocoto/vicel",
        .recipe = "make",
        });

        pmlist();
        pmupdate();
}
