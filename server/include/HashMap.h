#pragma once
#include "CWrappers.h"
#include "Debug.h"
#include "LString.h"
#include "StringView.h"
#include <assert.h>
#include <stdlib.h>

struct HashMapNode {
    void* key;
    void* val;
    bool  isHeadNode;
    bool  isValid;

    struct HashMapNode* next;
};
typedef struct HashMapNode HashMapNode;

static inline HashMapNode* hmn_make_null(void) {
    HashMapNode* res = calloc(1, sizeof(HashMapNode));
    *res = (HashMapNode){
        .key = nullptr,
        .val = nullptr,
        .next = nullptr,
        .isHeadNode = true,
        .isValid = false,
    };
    return res;
}

static inline HashMapNode* hmn_make(void* key, void* val) {
    HashMapNode* res = calloc(1, sizeof(HashMapNode));
    *res = (HashMapNode){
        .key = key,
        .val = val,
        .next = nullptr,
        .isValid = true,
    };
    return res;
}

// A function which takes a pointer to two keys and returns true if they are 'equal'
typedef bool (*KeyEqualFunction)(void* key1, void* key2);
// A function which takes a pointer to a single key and produces a hash value.
typedef size_t (*HashFunction)(void* key);
static constexpr size_t MAP_SIZE = 80;  // mime types im handling

// static hash map
struct HashMap {
    HashMapNode*     buckets[MAP_SIZE];
    size_t           capacity;
    size_t           count;
    size_t           bucket_count;
    KeyEqualFunction key_equal;
    HashFunction     hash;
};
typedef struct HashMap HashMap;
static inline HashMap  hm_make(HashFunction hash_fn, KeyEqualFunction key_equal_fn) {

    HashMap hm = (HashMap){
        .buckets = {},
        .bucket_count = 0,
        .count = 0,
        .capacity = MAP_SIZE,
        .hash = hash_fn,
        .key_equal = key_equal_fn,
    };

    for (size_t i = 0; i < MAP_SIZE; i++) {
        hm.buckets[i] = hmn_make_null();
    };
    return hm;
}
static inline void hmn_delete(HashMapNode* node) {
    *node = (HashMapNode){};  // destroy object
    free(node);               // free memory
}
static inline void hmn_deleteRecursive(HashMap* map, HashMapNode* node) {
    if (node->next) {
        hmn_deleteRecursive(map, node->next);
    }
    if (node) {
        if (node->isHeadNode) {
            map->bucket_count--;
        }
        map->count--;
        hmn_delete(node);
    }
}

static inline void hm_erase_idx(HashMap* hm, size_t idx) {
    hmn_deleteRecursive(hm, hm->buckets[idx]);
}
static inline void hm_erase(HashMap* hm, void* key) {
    // when erasing a key, we must go to the matching bucket
    // then traverse the list, properly doing a linked list deletion.
    // We update the isHead of the new head if that happens
    size_t idx = hm->hash(key) % MAP_SIZE;
    if (!hm->buckets[idx]->isValid) {
        return;  // key doesnt exist
    }
    if (hm->buckets[idx]->key == key) {
        // entry is the head
        if (hm->buckets[idx]->next) {
            // replace
            HashMapNode* newhead = hm->buckets[idx]->next;
            hmn_delete(hm->buckets[idx]);
            hm->buckets[idx] = newhead;
            newhead->isHeadNode = true;
        }
    }
    // else, the key is a non head node somewhere in the chain (or perhaps its a collision and it
    // doesnt exist)
    HashMapNode* cur = hm->buckets[idx]->next;
    HashMapNode* prev = hm->buckets[idx];
    while (cur) {
        if (cur->key == key) {
            // found our entry, now do the deletion and stitch shit back together
            prev->next = cur->next;  // 1->2->3 ==> 1->3
            hmn_delete(cur);
            break;
        }
        cur = cur->next;
        prev = prev->next;
    }
}

static inline void hm_delete(HashMap* hm) {
    for (size_t i = 0; i < hm->bucket_count; i++) {
        hm_erase_idx(hm, i);
    }
}
// Returns true if there was a collision (which triggers an allocation)
static inline bool hm_insert(HashMap* hm, void* key, void* val) {
    size_t       hash = hm->hash(key) % MAP_SIZE;
    HashMapNode* bucket = hm->buckets[hash];
    bool         collision_occured = false;
    hm->count++;
    if (bucket->isValid) {
        HashMapNode* cur = bucket;
        // traverse until next is nullptr
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = hmn_make(key, val);
        cur->next->isHeadNode = false;
        collision_occured = true;
        //        printf("[A]: ");
    } else {
        bucket = hmn_make(key, val);
        bucket->isHeadNode = true;
        hm->bucket_count++;
        collision_occured = false;
        //        printf("[B]: %d", bucket->isValid);
        *hm->buckets[hash] = *bucket;
    }
    //   printf("inserted (%zu) '%s' -> '%c'\n", hash, str_cstr(&key), val);
    return collision_occured;
}

/*
 * Could reimplment this with a fptr to a print function
static inline void hm_print(HashMap* hm) {
    for (size_t i = 0; i < hm->capacity; i++) {
        HashMapNode* cur = hm->buckets[i];
        if (cur->isValid) {
            while (cur->next) {
                cur = cur->next;
            }
        }
    }
}
*/

// finds first match
static inline void* hm_find(HashMap* hm, void* key) {
    // get the bucket
    HashMapNode* cur = hm->buckets[hm->hash(key) % MAP_SIZE];
    if (!cur) {
        // no bucket entry exists, therefore key definitely not in the map
        return nullptr;
    } else {
        // bucket entry exists, therefore key is somewhere in the chain
        // traverse bucket until we find matching key
        while (cur) {
            if (hm->key_equal(cur->key, key)) {
                return cur->val;
            }
            cur = cur->next;
        }
    }
    fprintf(stderr, "We really shouldnt be here. %s:%d", __FILE_NAME__, __LINE__);
    return nullptr;
}

static inline bool hm_contains(HashMap* hm, void* key) {
    return hm_find(hm, key) != nullptr;
}
