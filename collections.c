#include "9cc.h"

//
// 単方向リスト
//

typedef struct ListNode ListNode;

struct ListNode {
    void *val;
    ListNode *next;
};

struct List {
    ListNode *head;
    ListNode *tail;
    int size;
};

List list_new() {
    List list = calloc(1, sizeof(struct List));
    return list;
}

void list_push_front(List list, void *val) {
    ListNode *node = calloc(1, sizeof(ListNode));
    node->val = val;
    list->size++;
    if (!list->head) {
        list->head = node;
        list->tail = node;
        return;
    }
    node->next = list->head;
    list->head = node;
}

void list_push_back(List list, void *val) {
    ListNode *node = calloc(1, sizeof(ListNode));
    node->val = val;
    list->size++;
    if (!list->head) {
        list->head = node;
        list->tail = node;
        return;
    }
    list->tail = list->tail->next = node;
}

ListIter list_begin(List list) { return (ListIter)list->head; }

ListIter list_next(ListIter it) { return (ListIter)((ListNode *)it)->next; }

int list_size(List list) { return list->size; }

//
// ハッシュマップ(オープンアドレス法)
//

#define HASHMAP_INIT_SIZE 16
#define HASHMAP_MAX_LOAD 75
#define TOMBSTONE ((void *)-1)

typedef struct {
    char *key;
    int keylen;
    void *val;
} HashmapEntry;

struct Hashmap {
    HashmapEntry *buckets;
    int capacity;
    int used;
};

Hashmap hashmap_new() {
    Hashmap map = calloc(1, sizeof(struct Hashmap));
    map->capacity = HASHMAP_INIT_SIZE;
    map->buckets = calloc(HASHMAP_INIT_SIZE, sizeof(HashmapEntry));
    return map;
}

static uint64_t fnv_hash(char *key, int keylen) {
    uint64_t hash = 0xcbf29ce484222325;
    for (int i = 0; i < keylen; i++) {
        hash *= 0x100000001b3;
        hash ^= *key++;
    }
    return hash;
}

static void rehash(Hashmap map) {
    int oldcap = map->capacity;
    HashmapEntry *oldbuckets = map->buckets;

    map->capacity *= 2;
    map->buckets = calloc(map->capacity, sizeof(HashmapEntry));
    map->used = 0;

    for (int i = 0; i < oldcap; i++) {
        HashmapEntry *e = &oldbuckets[i];
        if (e->key && e->key != TOMBSTONE) {
            hashmap_put(map, e->key, e->keylen, e->val);
        }
    }
}

static bool match(HashmapEntry *entry, char *key, int keylen) {
    return entry->key != TOMBSTONE && entry->keylen == keylen &&
           !memcmp(entry->key, key, keylen);
}

static HashmapEntry *get_entry(Hashmap map, char *key, int keylen) {
    uint64_t hash = fnv_hash(key, keylen);

    for (uint64_t i = hash % map->capacity; i < map->capacity;
         i = (i + 1) % map->capacity) {
        HashmapEntry *e = &map->buckets[i];
        if (!e->key) {
            return NULL;
        }
        if (match(e, key, keylen)) {
            return e;
        }
    }

    unreachable();
}

static HashmapEntry *get_or_insert_entry(Hashmap map, char *key, int keylen) {
    if (map->used * 100 / map->capacity >= HASHMAP_MAX_LOAD) {
        rehash(map);
    }

    uint64_t hash = fnv_hash(key, keylen);

    for (uint64_t i = hash % map->capacity;; i = (i + 1) % map->capacity) {
        HashmapEntry *e = &map->buckets[i];
        if (!e->key) {
            map->used++;
            e->key = key;
            e->keylen = keylen;
            return e;
        }
        if (e->key == TOMBSTONE) {
            e->key = key;
            e->keylen = keylen;
            return e;
        }
        if (match(e, key, keylen)) {
            return e;
        }
    }
}

void *hashmap_get(Hashmap map, char *key, int keylen) {
    HashmapEntry *e = get_entry(map, key, keylen);
    return e ? e->val : NULL;
}

void hashmap_put(Hashmap map, char *key, int keylen, void *val) {
    HashmapEntry *e = get_or_insert_entry(map, key, keylen);
    e->val = val;
}

void hashmap_delete(Hashmap map, char *key, int keylen) {
    HashmapEntry *e = get_entry(map, key, keylen);
    if (e) {
        e->key = TOMBSTONE;
    }
}

int hashmap_test() {
    Hashmap map = hashmap_new();

    for (int i = 0; i < 5000; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }
    for (int i = 1000; i < 2000; i++) {
        char *key = format("key %d", i);
        hashmap_delete(map, key, (int)strlen(key));
    }
    for (int i = 1500; i < 1600; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }
    for (int i = 6000; i < 7000; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }

    for (int i = 0; i < 1000; i++) {
        char *key = format("key %d", i);
        assert((size_t)hashmap_get(map, key, (int)strlen(key)) == i);
    }
    for (int i = 1000; i < 1500; i++) {
        assert(!hashmap_get(map, "no such key", 11));
    }
    for (int i = 1500; i < 1600; i++) {
        char *key = format("key %d", i);
        assert((size_t)hashmap_get(map, key, (int)strlen(key)) == i);
    }
    for (int i = 1600; i < 2000; i++) {
        assert(!hashmap_get(map, "no such key", 11));
    }
    for (int i = 2000; i < 5000; i++) {
        char *key = format("key %d", i);
        assert((size_t)hashmap_get(map, key, (int)strlen(key)) == i);
    }
    for (int i = 5000; i < 6000; i++) {
        assert(!hashmap_get(map, "no such key", 11));
    }
    for (int i = 6000; i < 7000; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }

    assert(!hashmap_get(map, "no such key", 11));

    exit(0);
}

//
// 動的配列
//

struct Vector {
    void **data;
    int capacity;
    int size;
};

Vector vector_new() {
    Vector vec = calloc(1, sizeof(struct Vector));
    vec->capacity = 8;
    vec->data = calloc(vec->capacity, sizeof(void *));
    return vec;
}

void vector_push(Vector vec, void *val) {
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->size++] = val;
}

void *vector_get(Vector vec, int idx) { return vec->data[idx]; }

int vector_size(Vector vec) { return vec->size; }
