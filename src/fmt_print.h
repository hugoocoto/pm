extern int fmt_loffset;
extern int fmt_colorize;

#define ERR_C fmt_colorize ? "\033[31m" : ""      // error messages
#define WRN_C fmt_colorize ? "\033[33m" : ""      // warning messages
#define MSC_C fmt_colorize ? "\033[30m" : ""      // misc messages
#define C_C(x) fmt_colorize ? "\033[" #x "m" : "" // custom color
#define RST_C fmt_colorize ? "\033[0m" : ""
#define FMT_C "%s" // color format

#define print_error(fmt, ...)                                                                                         \
        do {                                                                                                          \
                fprintf(stderr, FMT_C "Error: " fmt ": %s" FMT_C "\n", ERR_C, ##__VA_ARGS__, strerror(errno), RST_C); \
        } while (0)

#define print_error_x(fmt, ...)                                                               \
        do {                                                                                  \
                fprintf(stderr, FMT_C "Error: " fmt FMT_C "\n", ERR_C, ##__VA_ARGS__, RST_C); \
        } while (0)

#define print_error_t(tag, fmt, ...)                                                        \
        do {                                                                                \
                print_error("[%-*.*s] " fmt, fmt_loffset, fmt_loffset, tag, ##__VA_ARGS__); \
        } while (0)

#define print_error_tx(tag, fmt, ...)                                                         \
        do {                                                                                  \
                print_error_x("[%-*.*s] " fmt, fmt_loffset, fmt_loffset, tag, ##__VA_ARGS__); \
        } while (0)

#define goto_err(lbl, fmt, ...)                  \
        do {                                     \
                print_error(fmt, ##__VA_ARGS__); \
                goto lbl;                        \
        } while (0)

#define goto_err_x(lbl, fmt, ...)                  \
        do {                                       \
                print_error_x(fmt, ##__VA_ARGS__); \
                goto lbl;                          \
        } while (0)

#define goto_err_t(tag, lbl, fmt, ...)                  \
        do {                                            \
                print_error_t(tag, fmt, ##__VA_ARGS__); \
                goto lbl;                               \
        } while (0)

#define goto_err_tx(tag, lbl, fmt, ...)                  \
        do {                                             \
                print_error_tx(tag, fmt, ##__VA_ARGS__); \
                goto lbl;                                \
        } while (0)

#define Info(fmt, ...)                                    \
        do {                                              \
                fprintf(stdout, fmt "\n", ##__VA_ARGS__); \
        } while (0)

#define Info_t(tag, fmt, ...)                                                                                                          \
        do {                                                                                                                           \
                fprintf(stdout, "[" FMT_C "%-*.*s" FMT_C "] " fmt "\n", C_C(34), fmt_loffset, fmt_loffset, tag, RST_C, ##__VA_ARGS__); \
        } while (0)

#define Warn(fmt, ...)                                              \
        do {                                                        \
                Info(FMT_C fmt FMT_C, WRN_C, ##__VA_ARGS__, RST_C); \
        } while (0)

#define Warn_t(tag, fmt, ...)                                              \
        do {                                                               \
                Info_t(tag, FMT_C fmt FMT_C, WRN_C, ##__VA_ARGS__, RST_C); \
        } while (0)

#define Misc(fmt, ...)                                              \
        do {                                                        \
                Info(FMT_C fmt FMT_C, MSC_C, ##__VA_ARGS__, RST_C); \
        } while (0)

#define Misc_t(tag, fmt, ...)                                              \
        do {                                                               \
                Info_t(tag, FMT_C fmt FMT_C, MSC_C, ##__VA_ARGS__, RST_C); \
        } while (0)

#define print_list_head(fmt, ...)             \
        do {                                  \
                Info(fmt ":", ##__VA_ARGS__); \
        } while (0)

#define print_list_entry(fmt, ...)             \
        do {                                   \
                Info("  " fmt, ##__VA_ARGS__); \
        } while (0)
