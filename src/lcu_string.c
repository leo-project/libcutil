#include <libcutil.h>

void str_init(string *str) {
    str->len = 0;
    str->data = NULL;
};
void str_deinit(string *str) {
    if (str->data != NULL) {
        free((void*)(str->data));
        str_init(str);
    }
}
bool str_empty(const string *str) {
    return str->len == 0 ? true : false;
}
int str_dup(string *dst, const string *src) {
    dst->data = lcu_strndup(src->data, src->len + 1);
    if (dst->data == NULL) {
        return LCU_NG;
    }
    dst->len = src->len;
    dst->data[dst->len] = '\0';
    return LCU_OK;
}
int str_copy(string *dst, const uint8_t *src, uint32_t srclen) {
    dst->data = lcu_strndup(src, srclen + 1);
    if (dst->data == NULL) {
        return LCU_NG;
    }
    dst->len = srclen;
    dst->data[dst->len] = '\0';
    return LCU_OK;
}
int str_comp(const string *s1, const string *s2) {
    if (s1->len != s2->len) {
        return s1->len - s2->len > 0 ? 1 : -1;
    }
    return lcu_strncmp(s1->data, s2->data, s1->len);
}

