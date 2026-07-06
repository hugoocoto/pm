#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <openssl/evp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include <curl/curl.h>
#include <curl/easy.h>

#define INCLUDE_CONF_IMPLEMENTATION
#include "conf.h"

#define FS_IMPLEMENTATION
#include "fs.h"

#include "flag.h"

char *
format(const char *fmt, ...)
{
        va_list ap;
        char *buf;
        va_start(ap, fmt);
        vasprintf(&buf, fmt, ap);
        va_end(ap);
        assert(buf);
        return buf;
}

size_t
_download_write(char *ptr, size_t size, size_t nmemb, void *stream)
{
        size_t n = fwrite(ptr, size, nmemb, (FILE *) stream);
        return n * size;
}

int
download(const char *url, const char *path)
{
        CURLcode result = !CURLE_OK;
        CURL *curl = NULL;
        FILE *f = NULL;

        if ((f = fopen(path, "wb")) == NULL) {
                perror(path);
                goto err;
        }

        if ((curl = curl_easy_init()) == NULL) {
                goto err;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* follow redirects */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /* disable progress var if verbose is not set */
        // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

        /* send all data to the write callback */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _download_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
                goto err;
        }

err:
        if (curl) curl_easy_cleanup(curl);
        if (f) fclose(f);
        return !(result == CURLE_OK);
}

char *
run_and_get(const char *cmd)
{
        char *t = malloc(4096);
        FILE *f = popen(cmd, "r");
        assert(f);
        char *s = fgets(t, sizeof t - 1, f);
        pclose(f);
        if (s == NULL) free(t);
        return s;
}

time_t
url_file_get_modification_time(const char *url)
{
        struct tm tm = { 0 };
        char weekday[4], month_str[4];
        char *buf =
        run_and_get(format("curl -sIL %s | grep -i last-modified", url)) ?:
        run_and_get(format("curl -sIL %s | grep -i date", url))          ?:
                                                                           0;
        assert(buf);

        if (sscanf(buf, "last-modified: %3s, %d %3s %d %d:%d:%d GMT",
                   weekday, &tm.tm_mday, month_str, &tm.tm_year,
                   &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 7) {
                free(buf);
                return -1;
        }

        const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        tm.tm_mon = -1;
        for (int i = 0; i < 12; i++) {
                if (strncmp(month_str, months[i], 3) == 0) {
                        tm.tm_mon = i;
                        break;
                }
        }
        if (tm.tm_mon == -1) {
                free(buf);
                return -1;
        }
        tm.tm_year -= 1900;

        free(buf);
        return timegm(&tm);
}

char *
url_get_filename(const char *url)
{
        const char *c = strrchr(url, '/');
        if (c == NULL) return NULL; // no '/' in the url
        if (c[1] == 0) return NULL; // url ends with '/' -> no filename
        return strdup(c + 1);
}

#define FIELD_LIST() \
        FIELD(url)   \
        FIELD(build) \
        FIELD(name)


int
process_package(const char *url, const char *build, const char *name, const char *build_path)
{
        if (url == NULL) {
                if (
#define FIELD(x) x ||
                FIELD_LIST()
#undef FIELD
                0) // print the error msg only if at least one field is set.
                        printf("  Error: URL not present\n");
                return 1;
        }

        char *filename = name ? strdup(name) : url_get_filename(url);
        assert(filename);

        char *dir = format("%s/%s/", build_path, filename);
        assert(dir);
        if (mkdirp(dir, 0755)) {
                perror("  mkdir");
                goto err;
        }

        char *file = format("%s/%s", dir, filename);
        assert(file);

        char *cwd = get_current_dir_name();
        assert(cwd);

        if (!file_exists(file)) {
                printf("  File not found. Downloading...\n");
                if (download(url, file)) goto err;
                goto build;
        }

        time_t t0 = url_file_get_modification_time(url);
        time_t t1 = file_get_modification_date(file);

        if (t0 > t1) {
                printf("  File is newer. Downloading...\n");
        } else {
                printf("  File is updated. Skipping...\n");
                goto exit;
        }

build:
        if (build) {
                if (chdir(dir)) {
                        perror("  Build error (chdir)");
                        goto err;
                }
                if (system(build)) {
                        perror("  Build error (system)");
                        goto err;
                }
                if (chdir(cwd)) {
                        perror("  Build error (chdir)");
                        goto err;
                }
        }

        int status = 0;
exit:
        status = 1;
err:
        assert(filename && file && cwd && dir);
        free(filename);
        free(file);
        free(cwd);
        free(dir);
        return status;
}

void
dump_config(const char *file)
{
        FILE *f = fopen(file, "w");
#define $(x, ...) fprintf(f, x "\n", ##__VA_ARGS__)
        $("--[[");
        $("    ~ pm. A package manager by Hugo Coto.");
        $("]] --");
        $("");
        $("-- Add ~/.config/pm/ to the path");
        $("package.path = package.path .. \";\" .. os.getenv(\"HOME\") .. \"/.config/pm/?.lua\"");
        $("");
        $("-- Add ~/.config/pm/ to the path");
        $("Build = {");
        $("    path = os.getenv(\"HOME\") .. \"/.local/share/pm/cache/\"");
        $("}");
        $("");
        $("--[[");
        $("    Package possible fields:");
#define FIELD(x) $("    - " #x);
        FIELD_LIST();
#undef FIELD
        $("]] --");
        $("Packages = {");
        $("    {");
        $("        -- pm bootstraping. Keep pm updated.");
        $("        url = \"https://github.com/hugoocoto/pm/releases/download/nightly/pm.tar.gz\",");
        $("        name = pm,");
        $("        build = \"tar -xzf pm.tar.gz && make && mv ./pm ~/.local/bin/pm\",");
        $("    },");
        $("}");
#undef $
        fclose(f);
}

int
load_config(const char *file)
{
        Conf conf;
        int len;
#define FIELD(x) const char *x = NULL;
        FIELD_LIST();
#undef FIELD

        if (Conf_open(&conf, file)) return 1;

        // build path (Lua Build.path)
        const char *build_path = NULL;
        if (Conf_get_str(conf, &build_path, "Build.path")) {
                printf("Error: field `Build = { path = "
                       " }` is required. In %s\n",
                       file);
        }
        printf("Build path: %s\n", build_path);
        if (mkdirp(build_path, 0755)) {
                perror(build_path);
                exit(1);
        }

        if (Conf_get_len(conf, &len, "Packages")) return 1;
        printf("packages: %d\n", len);

        for (int i = 1; i <= len; i++) {
                printf("== PACKAGE %d ==\n", i);
#define FIELD(x)                                                       \
        if (Conf_get_str(conf, &x, "Packages.%d." #x, i) == CONF_OK) { \
                printf("  " #x ": %s\n", x ?: "undefined");            \
                x = x ? strdup(x) : NULL;                              \
        }
                FIELD_LIST();
#undef FIELD

                process_package(
#define FIELD(x) x,
                FIELD_LIST()
#undef FIELD
                build_path);

#define FIELD(x)                  \
        if (x) {                  \
                free((void *) x); \
                x = NULL;         \
        }
                FIELD_LIST();
#undef FIELD
        }
        if (Conf_close(conf)) return 1;
        return 0;
}

int
main(int argc, char **argv)
{
        const char *init_config;
        const char *desc = format("~ pm by Hugo Coto. Built on %s %s", __DATE__, __TIME__);
        flag_program(.help = desc);
        flag_add(&init_config, "--init", .help = "Create init.lua");

        if (flag_parse(&argc, &argv)) {
                flag_show_help(STDOUT_FILENO);
                exit(1);
        }

        char *file = format("%s/.config/pm/init.lua", getenv("HOME") ?: "");
        assert(file);

        if (init_config) {
                char *cpyfile = strdup(file);
                assert(cpyfile);
                char *bn = dirname(cpyfile);
                mkdirp(bn, 0755);
                free(cpyfile);
                dump_config(file);
                printf("New config file: %s\n", file);
        }

        if (load_config(file)) {
                printf("Error loading config file %s\n", file);
        }

        free(file);

        flag_free();
        free((void *) desc);
        return 0;
}
