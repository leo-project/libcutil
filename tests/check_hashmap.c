#include <check.h>
#include "libcutil.h"

static void normal_test(int64 hint) {
    Hmap* h = lcu_map_init(&StrMapType, hint);
    byte keystr[16];
    byte valstr[16];
    String key = {keystr, 0};
    String val = {valstr, 0};
    bool pres;
    // get from empty hashmap
    lcu_map_access(&StrMapType, h, (byte*)&key, (byte*)&val, &pres);
    fail_unless(pres == false);
    fail_unless(key.len == 0);
    fail_unless(val.len == 0);

    // put/get
    byte keystr2[16];
    byte valstr2[16];
    memcpy(keystr2, "key", 3);
    String key2 = {keystr2, 3};
    memcpy(valstr2, "val", 3);
    String val2 = {valstr2, 3};
    lcu_map_assign(&StrMapType, h, (byte*)&key2, (byte*)&val2);
    fail_unless(lcu_map_count(h)== 1);
    lcu_map_access(&StrMapType, h, (byte*)&key2, (byte*)&val, &pres);
    fail_unless(pres == true);
    fail_unless(memcmp(val.str, val2.str, 3) == 0);
    fail_unless(val.len == 3);

    // remove
    lcu_map_assign(&StrMapType, h, (byte*)&key2, nil);
    fail_unless(lcu_map_count(h) == 0);

}
START_TEST(lazy) {
    normal_test(0);
}
END_TEST

START_TEST(normal) {
    normal_test(128);
}
END_TEST

START_TEST(bulkload) {
    Hmap* h = lcu_map_init(&StrMapType, 0);
    const uintgo NUM_ITEM = 512;
    const uintgo LEN_ITEM = 16;
    byte keystr[NUM_ITEM][LEN_ITEM];
    byte valstr[NUM_ITEM][LEN_ITEM];
    String key[NUM_ITEM];
    String val[NUM_ITEM];
    uintgo i;
    for (i = 0; i < NUM_ITEM; i++) {
        *(uintgo*)&keystr[i][0] = i;
        *(uintgo*)&valstr[i][0] = i;
        key[i].str = keystr[i];
        key[i].len = LEN_ITEM;
        val[i].str = valstr[i];
        val[i].len = LEN_ITEM;
        // load
        lcu_map_assign(&StrMapType, h, (byte*)&key[i], (byte*)&val[i]);
        fail_unless(lcu_map_count(h) == (i+1));
    }

    // validate loaded values
    byte valstr2[16];
    String val2 = {valstr2, 0};
    bool pres;
    for (i = 0; i < NUM_ITEM; i++) {
        lcu_map_access(&StrMapType, h, (byte*)&key[i], (byte*)&val2, &pres);
        fail_unless(pres == true);
        fail_unless(memcmp(val[i].str, val2.str, LEN_ITEM) == 0);
        fail_unless(val2.len == LEN_ITEM);
    }
}
END_TEST

Suite* hashmap_suite(void) {
    Suite* s = suite_create("lcu_hashmap.c");
    TCase* tc_core = tcase_create("Core");
    tcase_add_test(tc_core, normal);
    tcase_add_test(tc_core, lazy);
    tcase_add_test(tc_core, bulkload);
    suite_add_tcase(s, tc_core);
    return s;
}
