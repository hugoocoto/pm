#include <assert.h>
#include <errno.h>
#include <openssl/evp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>

#include <curl/curl.h>
#include <curl/easy.h>

#define INCLUDE_CONF_IMPLEMENTATION
#include "conf.h"
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

// make directory if not exists, make parent if needed.
int
mkdirp(const char *path)
{
        char *c;
        char *p;
        int s;

        c = p = strdup(path);
        while ((c = strchr(c + 1, '/'))) {
                *c = 0;
                s = mkdir(p, 0755);
                *c = '/';
                if (s && errno != EEXIST) return s;
        }

        if (!c) {
                s = mkdir(p, 0755);
                if (s && errno != EEXIST) return s;
        }

        return 0;
}

int
copy(const char *file1, const char *file2)
{
        FILE *f1 = NULL, *f2 = NULL;
        char buf[4096];
        int status = 0;

        if ((f1 = fopen(file1, "rb")) == NULL) {
                perror(file1);
                status = 1;
                goto err;
        }

        if ((f2 = fopen(file2, "wb")) == NULL) {
                perror(file2);
                status = 1;
                goto err;
        }

        for (;;) {
                ssize_t n = fread(buf, 1, sizeof buf, f1);
                if (n < 0) {
                        perror("fread");
                        status = 1;
                        goto err;
                }
                if (n == 0) break;
                assert(fwrite(buf, 1, n, f2) == (size_t) n);
        }

err:
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return status;
}

int
touch(const char *file)
{
        int status = 0;
        if (utime(file, NULL)) {
                if (errno == ENOENT) {
                        FILE *f = fopen(file, "w");
                        if (f == NULL) perror(file);
                        status = f != NULL;
                        if (f) fclose(f);
                        return status;
                }
                perror(file);
                status = 1;
        }
        return status;
}

int
file_hash(const char *path, unsigned char out[EVP_MAX_MD_SIZE], unsigned int *out_len)
{
        unsigned char buf[4096];
        EVP_MD_CTX *ctx;
        size_t n;
        FILE *f;

        if (!(f = fopen(path, "rb"))) goto err;
        if (!(ctx = EVP_MD_CTX_new())) goto err;

        if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL)) goto err;

        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
                if (!EVP_DigestUpdate(ctx, buf, n)) goto err;
        }

        if (!EVP_DigestFinal_ex(ctx, out, out_len)) goto err;

        EVP_MD_CTX_free(ctx);
        fclose(f);
        return 0;

err:
        if (ctx) EVP_MD_CTX_free(ctx);
        if (f) fclose(f);
        return -1;
}

int
file_is_equal(const char *file1, const char *file2)
{
        struct stat sa, sb;
        unsigned char ha[EVP_MAX_MD_SIZE], hb[EVP_MAX_MD_SIZE];
        unsigned int la, lb;

        if (!stat(file1, &sa) && !stat(file2, &sb)) {
                if (sa.st_size != sb.st_size) return 0;
        }

        if (file_hash(file1, ha, &la) || file_hash(file2, hb, &lb)) return 0;
        if (la != lb) return 0;

        return memcmp(ha, hb, la) == 0;
}

bool
file_is_newer(const char *file, const char *reference)
{
        struct stat stat1, stat2;

        if (stat(reference, &stat1)) {
                if (errno != ENOENT) perror(reference);
                return false;
        }

        if (stat(file, &stat2)) {
                if (errno != ENOENT) perror(file);
                return false;
        }

        if (stat1.st_mtim.tv_sec < stat2.st_mtim.tv_sec ||
            (stat1.st_mtim.tv_sec == stat2.st_mtim.tv_sec &&
             stat1.st_mtim.tv_nsec < stat2.st_mtim.tv_nsec)) {
                return true;
        }

        return false;
}

time_t
file_get_modification_date(const char *file)
{
        struct stat stat1;
        int err = 0;
        if (stat(file, &stat1)) {
                if (errno == ENOENT) return 0;
                perror(file);
                err = -1;
        } else {
                err = 0;
        }
        return err ?: stat1.st_mtim.tv_sec;
}

int
file_exists(const char *file)
{
        /* idk if this works as expected */
        struct stat st;
        if (stat(file, &st)) {
                if (errno != ENOENT) perror(file);
                return false;
        }
        return true;
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
        char *buf =
        run_and_get(format("curl -sIL %s | grep -i last-modified", url)) ?:
        run_and_get(format("curl -sIL %s | grep -i date", url))          ?:
                                                                           0;
        struct tm tm = { 0 };
        char weekday[4], month_str[4];
        char t[128];

        if (sscanf(t, "last-modified: %3s, %d %3s %d %d:%d:%d GMT",
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
        char *c = strrchr(url, '/');
        if (c == NULL) return NULL; // no '/' in the url
        if (c[1] == 0) return NULL; // url ends with '/' -> no filename
        return strdup(c + 1);
}

int
try(const char *url)
{
        char *filename = url_get_filename(url);
        if (filename == NULL) goto err;

        char *path = format("%s", filename);
        if (path == NULL) goto err;

        if (!file_exists(path)) {
                if (download(url, path)) goto err;
                free(filename);
                free(path);
                return 0;
        }

        time_t t0 = url_file_get_modification_time(url);
        time_t t1 = file_get_modification_date(path);

        if (t0 > t1) {
                printf("File is newer! Download again\n");
        } else {
                printf("File is updated\n");
        }

err:
        free(filename);
        free(path);
        return 1;
}

int
main(int argc, char **argv)
{
        flag_program(.help = "~ pm by Hugo Coto");

        if (flag_parse(&argc, &argv)) {
                flag_show_help(STDOUT_FILENO);
                exit(1);
        }

        for (int i = 1; i < argc; i++) {
                try(argv[i]);
        }

        flag_free();
}
