#ifndef _LCU_HASHMAP_H_
#define _LCU_HASHMAP_H_

#include <libcutil.h>

enum {
    Structrnd = sizeof(uintptr)
};
enum
{
    PtrSize = sizeof(void*),
};

typedef struct Alg
{
    void    (*hash)(uintptr*, uintptr, void*);
    void    (*equal)(bool*, uintptr, void*, void*);
    void    (*copy)(uintptr, void*, void*);
} Alg;

typedef struct String
{
    byte*   str;
    intgo   len;
} String;

typedef struct Type
{
    uintptr size;
    uint8 align;
    Alg *alg;
} Type;

typedef struct MapType
{
    Type *key;
    Type *elem;
} MapType;

struct Hmap;
typedef struct Hmap Hmap;

extern  MapType StrMapType;


Hmap* lcu_map_init(MapType*, int64);
void lcu_map_destroy(Hmap*);
uintgo lcu_map_count(Hmap*);
void lcu_map_assign(MapType*, Hmap*, byte*, byte*);
void lcu_map_access(MapType*, Hmap*, byte*, byte*, bool*);

#endif
