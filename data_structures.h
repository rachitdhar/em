//
// data_structures.h
//

/*

here i am implementing some of the data structures that
are being used in several places all over this project.

the goal is to minimize the use of any standard libraries
that are unnecessarily large or unoptimized, or where there
is a way to write a uniquely specialized implementation.

*/

/*
implementation for hash maps from std::string -> T

replaces: std::unordered_map<K,V>
with:     smap<V>

functions that are needed:

    - insert(std::string, T)
    - [] operator (std::string)
*/

#include <string>

#define SMAP_CAPACITY_INIT 8


// an implementation for the FNV-1a hash function
inline size_t fnv1a_hash(const std::string& s) {
    size_t hash = 1469598103934665603ull;
    for (unsigned char c : s)
        hash = (hash ^ c) * 1099511628211ull;
    return hash;
}

template <typename T>
struct smap_pair {
    std::string key;
    T value;

    bool occupied = false;
    bool deleted = false;
};

template <typename T>
struct smap {
    smap_pair<T> *data = nullptr;
    size_t size = 0;        // used space
    size_t capacity = 0;    // allocated space

    smap(size_t initial_capacity = SMAP_CAPACITY_INIT) {
        capacity = initial_capacity;
        data = new smap_pair<T>[capacity];
    }

    ~smap() {
        delete[] data;
    }

    void insert(const std::string& key, const T& value);
    T operator[](const std::string& key);
    void rehash();
    void resize(size_t new_capacity);
};

template <typename T>
inline void smap<T>::insert(const std::string& key, const T& value) {
    if ((size + 1.0) / capacity > 0.75) {
        resize(capacity * 2);
    }

    size_t hash = fnv1a_hash(key);
    size_t index = hash & (capacity - 1);

    while (1) {
        if (!data[index].occupied || data[index].deleted) {
            data[index].key = key;
            data[index].value = value;
            data[index].occupied = true;
            data[index].deleted = false;
            ++size;
            return;
        } else if (data[index].key == key) {
            data[index].value = value;
            return;
        }
        index = (index + 1) & (capacity - 1);
    }
}

template <typename T>
inline T smap<T>::operator[](const std::string& key) {
    size_t hash = fnv1a_hash(key);
    size_t index = hash & (capacity - 1);

    while (data[index].occupied) {
        if (!data[index].deleted && data[index].key == key) return data[index].value;
        index = (index + 1) & (capacity - 1);
    }
    return NULL;
}

template <typename T>
inline void smap<T>::resize(size_t new_capacity) {
    smap_pair<T> *old_data = data;
    size_t old_capacity = capacity;

    data = new smap_pair<T>[new_capacity];
    capacity = new_capacity;
    size = 0;

    for (size_t i = 0; i < old_capacity; ++i) {
        if (old_data[i].occupied && !old_data[i].deleted) {
            insert(old_data[i].key, old_data[i].value);
        }
    }

    delete[] old_data;
}
