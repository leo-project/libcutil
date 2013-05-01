#include <libcutil.h>


static void _lru_destroy_item(lcu_lru* lru, lcu_lru_item* item);

lcu_lru* lcu_lru_create() {
    lcu_lru* lru = malloc(sizeof(lcu_lru));
    TAILQ_INIT(&lru->tqh);
    return lru;
}

void lcu_lru_destroy(lcu_lru* lru) {
    lcu_lru_item* item;
    TAILQ_FOREACH(item, &lru->tqh, tqe) {
        _lru_destroy_item(lru, item);
    }
    free(lru);
}

lcu_lru_item* lcu_lru_insert(lcu_lru* lru, uint8_t* key, uint32_t keylen, uint8_t* value, uint32_t size, lcu_lru_destroy_cb cb, void* userdata) {
    lcu_lru_item* item;
    item = malloc(sizeof(lcu_lru_item));
    item->key = key;
    item->keylen = keylen;
    item->val = value;
    item->vallen = size;
    item->destroy = cb;
    item->userdata = userdata;
    TAILQ_INSERT_TAIL(&lru->tqh, item, tqe);
    return item;
}

int lcu_lru_eject_by_size(lcu_lru* lru, int size, lcu_lru_eject_cb eject, void* container) {
    int ejected = 0;
    lcu_lru_item* item;
    
    while(ejected < size) {
      item = lcu_lru_eldest(lru);
      if (NULL == item) {
        break;
      }
      ejected += lcu_lru_item_size(item);
      if (NULL != eject) {
        (*eject)(container, item->key, item->keylen);
      }
      lcu_lru_remove_and_destroy(lru, item);
    }
    return ejected;
}

void lcu_lru_touch(lcu_lru* lru, lcu_lru_item* item) {
    TAILQ_REMOVE(&lru->tqh, item, tqe);
    TAILQ_INSERT_TAIL(&lru->tqh, item, tqe);
}

void lcu_lru_remove_and_destroy(lcu_lru* lru, lcu_lru_item* item) {
    TAILQ_REMOVE(&lru->tqh, item, tqe);
    _lru_destroy_item(lru, item);
}

static void _lru_destroy_item(lcu_lru* lru, lcu_lru_item* item) {
    if (item->destroy != NULL) {
        (*(item->destroy))(item->userdata, item->key, item->keylen, item->val, item->vallen);
    }
    free(item);
}

