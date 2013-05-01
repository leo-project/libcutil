#ifndef _LCU_STRING_H_
#define _LCU_STRING_H_

#include <string.h>
#include <libcutil.h>

typedef struct string {
    uint32_t len;   /* string length */
    uint8_t  *data; /* string data */
} string;

#define str(_str)   { sizeof(_str) - 1, (uint8_t *)(_str) }
#define null_str    { 0, NULL }

#define str_set_text(_str, _text) do {       \
    (_str)->len = (uint32_t)(sizeof(_text) - 1);\
    (_str)->data = (uint8_t *)(_text);          \
} while (0);

#define str_set_raw(_str, _raw) do {         \
    (_str)->len = (uint32_t)(lcu_strlen(_raw));  \
    (_str)->data = (uint8_t *)(_raw);           \
} while (0);

void str_init(string *str);
void str_deinit(string *str);
bool str_empty(const string *str);
int str_dup(string *dst, const string *src);
int str_copy(string *dst, const uint8_t *src, uint32_t srclen);
int str_comp(const string *s1, const string *s2);

#define lcu_memcpy(_d, _c, _n)           \
    memcpy(_d, _c, (size_t)(_n))

#define lcu_memmove(_d, _c, _n)          \
    memmove(_d, _c, (size_t)(_n))

#define lcu_memchr(_d, _c, _n)           \
    memchr(_d, _c, (size_t)(_n))

#define lcu_strlen(_s)                   \
    strlen((char *)(_s))

#define lcu_strncmp(_s1, _s2, _n)        \
    strncmp((char *)(_s1), (char *)(_s2), (size_t)(_n))

#define lcu_strchr(_p, _l, _c)           \
    _lcu_strchr((uint8_t *)(_p), (uint8_t *)(_l), (uint8_t)(_c))

#define lcu_strrchr(_p, _s, _c)          \
    _lcu_strrchr((uint8_t *)(_p),(uint8_t *)(_s), (uint8_t)(_c))

#define lcu_strndup(_s, _n)              \
    (uint8_t *)strndup((char *)(_s), (size_t)(_n));

#define lcu_snprintf(_s, _n, ...)        \
    snprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define lcu_scnprintf(_s, _n, ...)       \
    _scnprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define lcu_vscnprintf(_s, _n, _f, _a)   \
    _vscnprintf((char *)(_s), (size_t)(_n), _f, _a)

#endif
