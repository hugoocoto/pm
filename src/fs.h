#ifndef FS_H_
#define FS_H_

#include <errno.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <sys/types.h>

int file_exists(const char *file);
time_t file_get_modification_date(const char *file);
bool file_is_newer(const char *file, const char *reference);
int file_is_equal(const char *file1, const char *file2);
int file_hash(const char *path, unsigned char out[EVP_MAX_MD_SIZE], unsigned int *out_len);
int touch(const char *file);
int copy(const char *file1, const char *file2);
int mkdirp(const char *path, mode_t perms);

#ifdef FS_IMPLEMENTATION

// make directory if not exists, make parent if needed.
int
mkdirp(const char *path, mode_t perms)
{
        char *c;
        char *p;
        int s;

        c = p = strdup(path);
        while ((c = strchr(c + 1, '/'))) {
                *c = 0;
                s = mkdir(p, perms);
                *c = '/';
                if (s && errno != EEXIST) {
                        free(p);
                        return s;
                }
        }

        if (!c) {
                s = mkdir(p, perms);
                if (s && errno != EEXIST) {
                        free(p);
                        return s;
                }
        }

        free(p);
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
                size_t n = fread(buf, 1, sizeof buf, f1);
                if (n == 0) {
                        if (ferror(f1)) {
                                perror("fread");
                                status = 1;
                                goto err;
                        }
                        break;
                }
                if (fwrite(buf, 1, n, f2) != n) {
                        perror("fwrite");
                        status = 1;
                        goto err;
                }
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
        EVP_MD_CTX *ctx = NULL;
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

#endif // !FS_IMPLEMENTATION
#endif // !FS_H_
