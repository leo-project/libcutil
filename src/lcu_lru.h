#ifndef _LCU_LRU_H_
#define _LCU_LRU_H_

#include <libcutil.h>

//the destroy callback, this is needed to free memory for shit
typedef void (*lcu_lru_destroy_cb)(void*, uint8_t*, uint32_t, uint8_t*, uint32_t);

//this is the callback for LRU ejection, so that upper layers can cleanup
//first arg is the containing struct.  can this be cleaner?
typedef void (*lcu_lru_eject_cb)(void*, uint8_t*, uint32_t);

typedef struct lcu_lru_item {
    uint8_t* key;
    uint32_t keylen;
    uint8_t* val;
    uint32_t vallen;
    TAILQ_ENTRY(lcu_lru_item) tqe;
    lcu_lru_destroy_cb destroy;
    void* userdata;
} lcu_lru_item;

TAILQ_HEAD(lcu_lru_item_tqh, lcu_lru_item);
typedef struct lcu_lru_item_tqh lcu_lru_item_tqh;

typedef struct lcu_lru {
    lcu_lru_item_tqh tqh;
    lcu_lru_item* iterator;
} lcu_lru;

#define lcu_lru_item_key(item) ((item)->key)
#define lcu_lru_item_keylen(item) ((item)->keylen)
#define lcu_lru_item_value(item) ((item)->val)
#define lcu_lru_item_vallen(item) ((item)->vallen)
#define lcu_lru_item_size(item) (lcu_lru_item_keylen(item) + lcu_lru_item_vallen(item))
#define lcu_lru_eldest(lru) (TAILQ_FIRST(&lru->tqh))
#define lcu_lru_iterator(lru) (lru->iterator = TAILQ_FIRST(&lru->tqh))
#define lcu_lru_iterator_next(lru) (lru->iterator ? lru->iterator = TAILQ_NEXT(lru->iterator, tqe): NULL)

lcu_lru* lcu_lru_create();
void lcu_lru_destroy(lcu_lru* lru);
lcu_lru_item* lcu_lru_insert(lcu_lru* lru, uint8_t* key, uint32_t keylen, uint8_t* value, uint32_t size, lcu_lru_destroy_cb cb, void* userdata);
int lcu_lru_eject_by_size(lcu_lru* lru, int size, lcu_lru_eject_cb eject, void* container);
void lcu_lru_touch(lcu_lru* lru, lcu_lru_item* item);
void lcu_lru_remove_and_destroy(lcu_lru* lru, lcu_lru_item* item);

#endif
