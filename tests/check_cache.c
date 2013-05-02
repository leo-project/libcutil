#include <check.h>
#include "libcutil.h"

static void normal_test(lcu_cache_opt opt, size_t limit, double factor, size_t min, size_t max) {
    lcu_cache lc;
    lcu_cache_init(&lc, opt, limit, factor, min, max);
    byte keystr[16];
    byte valstr[16];
    String key = {keystr, 3};
    String val = {valstr, 0};
    // get from empty cache
    lcu_cache_get(&lc, key, &val);
    fail_unless(val.str == 0);
    fail_unless(val.len == 0);

    // put/get
    byte keystr2[16];
    byte valstr2[16];
    memcpy(keystr2, "key", 3);
    String key2 = {keystr2, 3};
    memcpy(valstr2, "val", 3);
    String val2 = {valstr2, 3};
    bool ret = lcu_cache_put(&lc, key2, val2);
    fail_unless(ret);
    fail_unless(lcu_cache_item_size(&lc)== 1);
    fail_unless(lcu_cache_mem_active_size(&lc)== 6);
    lcu_cache_get(&lc, key2, &val);
    fail_unless(val.str != NULL);
    fail_unless(val.str != val2.str);
    fail_unless(memcmp(val.str, val2.str, 3) == 0);
    fail_unless(val.len == 3);

    // eldest
    lcu_cache_eldest(&lc, &key, &val);
    fail_unless(key.str != NULL);
    fail_unless(val.str != NULL);
    fail_unless(memcmp(key.str, keystr2, 3) == 0);
    fail_unless(memcmp(val.str, valstr2, 3) == 0);
    fail_unless(key.len == 3);
    fail_unless(val.len == 3);
    
    // iterator/iterator_next 
    lcu_cache_iterator(&lc, &key, &val);
    fail_unless(key.str != NULL);
    fail_unless(val.str != NULL);
    fail_unless(memcmp(key.str, keystr2, 3) == 0);
    fail_unless(memcmp(val.str, valstr2, 3) == 0);
    lcu_cache_iterator_next(&lc, &key, &val);
    fail_unless(key.str == NULL);
    fail_unless(val.str == NULL);
    fail_unless(key.len == 0);
    fail_unless(val.len == 0);

    // remove
    lcu_cache_delete(&lc, key2);
    fail_unless(lcu_cache_item_size(&lc)== 0);
    fail_unless(lcu_cache_mem_active_size(&lc)== 0);
}

START_TEST(normal) {
    normal_test(auto_eject_on, 1024, 1.5, 16, 256);
}
END_TEST

START_TEST(normal2) {
    normal_test(auto_eject_off, 1024 * 1024 * 64, 2, 128, 1024 * 1024);
}
END_TEST

START_TEST(put_already_there) {
    lcu_cache lc;
    lcu_cache_init(&lc, auto_eject_on, 1024 * 1024 * 64, 2, 128, 1024 * 1024);
    byte* keystr = (byte*)"key";
    byte* valstr = (byte*)"val1";
    byte* valstr2 = (byte*)"val2";
    String key = {keystr, 3};
    String val = {valstr, 4};

    // put first
    bool ret = lcu_cache_put(&lc, key, val);
    fail_unless(ret);
    fail_unless(lcu_cache_item_size(&lc)== 1);
    fail_unless(lcu_cache_mem_active_size(&lc)== 7);

    // put already there 
    val.str = valstr2;
    ret = lcu_cache_put(&lc, key, val);
    fail_unless(ret);
    fail_unless(lcu_cache_item_size(&lc)== 1);
    fail_unless(lcu_cache_mem_active_size(&lc)== 7);
    // get(validate)
    lcu_cache_get(&lc, key, &val);
    fail_unless(val.str != NULL);
    fail_unless(val.str != valstr2);
    fail_unless(memcmp(val.str, valstr2, 4) == 0);
    fail_unless(val.len == 4);

}
END_TEST

START_TEST(put_beyond_limit) {
    lcu_cache lc;
    lcu_cache_init(&lc, auto_eject_on, 64, 2, 16, 64);
    byte* keystr1 = (byte*)"key1";
    byte* keystr2 = (byte*)"key2";
    byte* keystr3 = (byte*)"key3";
    byte* valstr = (byte*)"0123456789012345678901234567890123456789";
    String key = {keystr1, 4};
    String val = {valstr, 40};

    // put first
    bool ret = lcu_cache_put(&lc, key, val);
    fail_unless(ret);
    fail_unless(lcu_cache_item_size(&lc)== 1);
    fail_unless(lcu_cache_mem_active_size(&lc)== 44);
    // put second(beyond limit) 
    key.str = keystr2;
    ret = lcu_cache_put(&lc, key, val);
    fail_unless(ret);
    fail_unless(lcu_cache_item_size(&lc)== 1);
    fail_unless(lcu_cache_mem_active_size(&lc)== 44);
    // put third(beyond limit)
    key.str = keystr3;
    ret = lcu_cache_put(&lc, key, val);
    fail_unless(ret);
    fail_unless(lcu_cache_item_size(&lc)== 1);
    fail_unless(lcu_cache_mem_active_size(&lc)== 44);

    // get(validate)
    key.str = keystr1;
    lcu_cache_get(&lc, key, &val);
    fail_unless(val.str == NULL);
    fail_unless(val.len == 0);
    key.str = keystr2;
    lcu_cache_get(&lc, key, &val);
    fail_unless(val.str == NULL);
    fail_unless(val.len == 0);
    key.str = keystr3;
    lcu_cache_get(&lc, key, &val);
    fail_unless(val.str != NULL);
    fail_unless(val.len == 40);

}
END_TEST

START_TEST(bulkload) {
    lcu_cache lc;
    lcu_cache_init(&lc, auto_eject_on, 1024 * 1024 * 64, 1.5, 128, 1024 * 1024);
    const uintgo NUM_ITEM = 4096;
    const uintgo LEN_ITEM = 16;
    byte keystr[LEN_ITEM];
    byte valstr[LEN_ITEM];
    String key;
    String val;
    uintgo i;
    memset(keystr, 0, sizeof(keystr));
    memset(valstr, 0, sizeof(valstr));
    key.str = keystr;
    key.len = LEN_ITEM;
    val.str = valstr;
    val.len = LEN_ITEM;
    for (i = 0; i < NUM_ITEM; i++) {
        *(uintgo*)keystr = i;
        *(uintgo*)valstr = i;
        //keystr[0] = i + 'a';
        //valstr[0] = i + 'A';
        // load
        bool ret = lcu_cache_put(&lc, key, val);
        fail_unless(ret);
        fail_unless(lcu_cache_item_size(&lc)== (i+1));
        fail_unless(lcu_cache_mem_active_size(&lc)== (LEN_ITEM*2*(i+1)));
    }
    fail_unless(lcu_cache_mem_alloc_size(&lc)== 1024*1024);

    // validate loaded values
    memset(keystr, 0, sizeof(keystr));
    for (i = 0; i < NUM_ITEM; i++) {
        *(uintgo*)keystr = i;
        lcu_cache_get(&lc, key, &val);
        fail_unless(val.str != NULL);
        fail_unless(memcmp(val.str, key.str, LEN_ITEM) == 0);
        fail_unless(val.len == LEN_ITEM);
    }
    // iterator/iterator_next 
    lcu_cache_iterator(&lc, &key, &val);
    fail_unless(key.str != NULL);
    fail_unless(val.str != NULL);
    i = 1;
    while (key.str != NULL) {
        lcu_cache_iterator_next(&lc, &key, &val);
        if (key.str != NULL) {
            fail_unless(val.str != NULL);
            fail_unless(val.len == LEN_ITEM);
            i++;
        }
        //printf("[iterator]i:%d key:%s val:%s \n", i, key.str, val.str);
    }
    fail_unless(i == NUM_ITEM);
    fail_unless(lcu_cache_item_size(&lc)== NUM_ITEM);
    fail_unless(lcu_cache_mem_alloc_size(&lc)== 1024*1024);

}
END_TEST

Suite* cache_suite(void) {
    Suite* s = suite_create("lcu_cache.c");
    TCase* tc_core = tcase_create("Core");
    tcase_add_test(tc_core, normal);
    tcase_add_test(tc_core, normal2);
    tcase_add_test(tc_core, bulkload);
    tcase_add_test(tc_core, put_already_there);
    tcase_add_test(tc_core, put_beyond_limit);
    suite_add_tcase(s, tc_core);
    return s;
}
