#include <check.h>
#include "libcutil.h"

START_TEST(create_and_insert) {
    lcu_lru* lru;
    lcu_lru_item* item;
    lru = lcu_lru_create();
    lcu_lru_insert(lru, (uint8_t*)"key", 3, (uint8_t*)"value", 5, NULL, NULL);
    item = lcu_lru_eldest(lru);
    fail_unless(NULL != item);
    fail_unless(strncmp("key", (const char*)lcu_lru_item_key(item), 3) == 0);
    lcu_lru_destroy(lru);
}
END_TEST

START_TEST(touch) {
    lcu_lru* lru;
    lcu_lru_item* one;
    
    lru = lcu_lru_create();
    one = lcu_lru_insert(lru, (uint8_t*)"one", 3, (uint8_t*)"one", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"two", 3, (uint8_t*)"two", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"three", 5, (uint8_t*)"three", 5, NULL, NULL);
    lcu_lru_touch(lru, one);
    fail_unless(one == TAILQ_LAST(&lru->tqh, lcu_lru_item_tqh));
    lcu_lru_destroy(lru);
}
END_TEST

START_TEST(eject_one) {
    lcu_lru* lru;
    int ejected = 0;
    lru = lcu_lru_create();
    lcu_lru_insert(lru, (uint8_t*)"one", 3, (uint8_t*)"one", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"two", 3, (uint8_t*)"two", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"three", 5, (uint8_t*)"three", 5, NULL, NULL);
    ejected = lcu_lru_eject_by_size(lru, 5, NULL, NULL);
    fail_unless(ejected == 6);
}
END_TEST

START_TEST(eject_multiple) {
    lcu_lru* lru;
    int ejected = 0;
    lcu_lru_item* three;
    mark_point();
    lru = lcu_lru_create();
    mark_point();
    lcu_lru_insert(lru, (uint8_t*)"one", 3, (uint8_t*)"one", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"two", 3, (uint8_t*)"two", 3, NULL, NULL);
    three = lcu_lru_insert(lru, (uint8_t*)"three", 5, (uint8_t*)"three", 5, NULL, NULL);
    ejected = lcu_lru_eject_by_size(lru, 12, NULL, NULL);
    fail_unless(TAILQ_LAST(&lru->tqh, lcu_lru_item_tqh) == three);
    fail_unless(ejected == 12);
    lcu_lru_destroy(lru);
}
END_TEST

START_TEST(eject_all) {
    lcu_lru* lru;
    int ejected = 0;
    lcu_lru_item* item;
    mark_point();
    lru = lcu_lru_create();
    mark_point();
    lcu_lru_insert(lru, (uint8_t*)"one", 3, (uint8_t*)"one", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"two", 3, (uint8_t*)"two", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"three", 5, (uint8_t*)"three", 5, NULL, NULL);
    ejected = lcu_lru_eject_by_size(lru, 30, NULL, NULL);
    item = lcu_lru_eldest(lru);
    fail_unless(NULL == item);
    fail_unless(ejected == 22);
    lcu_lru_destroy(lru);
}
END_TEST

START_TEST(iterator) {
    lcu_lru* lru;
    lcu_lru_item* item;
    int i;
    mark_point();
    lru = lcu_lru_create();
    mark_point();
    lcu_lru_insert(lru, (uint8_t*)"one", 3, (uint8_t*)"one", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"two", 3, (uint8_t*)"two", 3, NULL, NULL);
    lcu_lru_insert(lru, (uint8_t*)"three", 5, (uint8_t*)"three", 5, NULL, NULL);
    for (i = 0, item = lcu_lru_iterator(lru); item != NULL; i++, item = lcu_lru_iterator_next(lru)) {
        fail_unless(NULL != item);
        switch (i) {
            case 0:
                fail_unless(strncmp("one", (const char*)lcu_lru_item_key(item), 3) == 0);
                break;
            case 1:
                fail_unless(strncmp("two", (const char*)lcu_lru_item_key(item), 3) == 0);
                break;
            case 2:
                fail_unless(strncmp("three", (const char*)lcu_lru_item_key(item), 5) == 0);
            default:
                ;
        }
    }
    lcu_lru_destroy(lru);
}
END_TEST

Suite* lru_suite(void) {
    Suite* s = suite_create("lcu_lru.c");
    TCase* tc_core = tcase_create("Core");
    tcase_add_test(tc_core, create_and_insert);
    tcase_add_test(tc_core, touch);
    tcase_add_test(tc_core, eject_one);
    tcase_add_test(tc_core, eject_multiple);
    tcase_add_test(tc_core, eject_all);
    tcase_add_test(tc_core, iterator);
    suite_add_tcase(s, tc_core);
    return s;
}
