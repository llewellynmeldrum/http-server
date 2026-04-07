#pragma once
#include "CWrappers.h"
#include "Debug.h"
#include "LString.h"
#include "StringView.h"
#include <assert.h>
#include <stdlib.h>
#define KEY_T StringView
#define VAL_T char
#define VAL_NULL '0'
#define KEY_NULL NULL_STRINGVIEW
#define KEY_EQ_FN(k1, k2) sv_equal(k1, k2)

struct MapNode {
    KEY_T           key;
    VAL_T           val;
    struct MapNode* next;
    bool            isHeadNode;
    bool            isValid;
};
typedef struct MapNode MapNode;

static inline MapNode* mn_make_null(void) {
    MapNode* res = calloc(1, sizeof(MapNode));
    *res = (MapNode){
        .key = KEY_NULL,
        .val = VAL_NULL,
        .next = nullptr,
        .isHeadNode = true,
        .isValid = false,
    };
    return res;
}
static inline MapNode* mn_make(KEY_T key, VAL_T val) {
    MapNode* res = calloc(1, sizeof(MapNode));
    *res = (MapNode){
        .key = key,
        .val = val,
        .next = nullptr,
        .isValid = true,
    };
    return res;
}

static constexpr size_t MAP_SIZE = 64;

// static hash map
struct StringMap {
    MapNode* buckets[MAP_SIZE];
    size_t   capacity;
    size_t   count;
    size_t   bucket_count;
};
typedef struct StringMap StringMap;
static inline void       mn_deleteRecursive(StringMap* map, MapNode* node) {
    if (node->next) {
        mn_deleteRecursive(map, node->next);
    }
    if (node) {
        if (node->isHeadNode) {
            map->bucket_count--;
        }
        map->count--;

        *node = (MapNode){};  // destroy object
        free(node);           // free memory
        node = nullptr;       // destroy reference
    }
}

static inline size_t sm_hash(StringView key) {
    // WARNING: If KEY_T is not a StringView, this needs to be reimplmented.
    ASSERT_TYPE_EQ(typeof(key), StringView);

    constexpr size_t BIG_ASS_PRIME = 1572869;
    size_t           sum = 0;
    for (size_t i = 0; i < key.len; i++) {
        sum += sv_at(key, i);
        sum *= BIG_ASS_PRIME;
    }
    size_t idx = sum % MAP_SIZE;
    //    printf("idx:%zu\n", idx);
    return idx;
}
static inline StringMap sm_make(void) {
    StringMap sm = (StringMap){
        .buckets = {},
        .bucket_count = 0,
        .count = 0,
        .capacity = MAP_SIZE,
    };

    for (size_t i = 0; i < MAP_SIZE; i++) {
        sm.buckets[i] = mn_make_null();
    };
    return sm;
}
static inline void sm_erase_idx(StringMap* sm, size_t idx) {
    mn_deleteRecursive(sm, sm->buckets[idx]);
}
static inline void sm_erase(StringMap* sm, KEY_T key) {
    sm_erase_idx(sm, sm_hash(key));
}
static inline void sm_delete(StringMap* sm) {
    for (size_t i = 0; i < sm->bucket_count; i++) {
        sm_erase_idx(sm, i);
    }
}

// Returns true if there was a collision (which triggers an allocation)
static inline bool sm_insert(StringMap* sm, KEY_T key, VAL_T val) {
    size_t   hash = sm_hash(key);
    MapNode* bucket = sm->buckets[hash];
    bool     collision_occured = false;
    sm->count++;
    if (bucket->isValid) {
        MapNode* cur = bucket;
        // traverse until next is nullptr
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = mn_make(key, val);
        cur->next->isHeadNode = false;
        collision_occured = true;
        //        printf("[A]: ");
    } else {
        bucket = mn_make(key, val);
        bucket->isHeadNode = true;
        sm->bucket_count++;
        collision_occured = false;
        //        printf("[B]: %d", bucket->isValid);
        *sm->buckets[hash] = *bucket;
    }
    //   printf("inserted (%zu) '%s' -> '%c'\n", hash, str_cstr(&key), val);
    return collision_occured;
}
static inline void sm_mapArrays(StringMap* sm, const KEY_T* keys, const VAL_T* vals, size_t len) {
    assert(len <= MAP_SIZE);
    for (size_t i = 0; i < len; i++) {
        sm_insert(sm, keys[i], vals[i]);
    }
}
static inline void sm_print(StringMap* sm) {
    printf("printing stuff:\n");
    for (size_t i = 0; i < sm->capacity; i++) {
        MapNode* cur = sm->buckets[i];
        if (cur->isValid) {
            const char* cstr = sv_cstr(cur->key);
            printf("[(%p)'%s'->'%c'], ", cstr, cstr, cur->val);
            while (cur->next) {
                const char* cstr = sv_cstr(cur->key);
                printf("[(%p)'%s'->'%c'], ", cstr, cstr, cur->val);
                cur = cur->next;
            }
            printf("\n");
        }
    }
}

static inline VAL_T sm_find(StringMap* sm, KEY_T key) {
    // must traverse to the matching element.
    // get the bucket
    MapNode* cur = sm->buckets[sm_hash(key)];
    if (!cur) {
        return VAL_NULL;
    } else {
        // traverse bucket until we find matching key
        while (cur) {
            if (KEY_EQ_FN(cur->key, key)) {
                return cur->val;
            }
            cur = cur->next;
        }
    }
    fprintf(stderr, "We really shouldnt be here. %s:%d", __FILE_NAME__, __LINE__);
    return VAL_NULL;  //
}
static inline VAL_T sm_contains(StringMap* sm, KEY_T key) {
    return sm_find(sm, key) != VAL_NULL;
}
