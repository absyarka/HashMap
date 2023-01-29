#include <vector>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    class iterator {
    public:
        iterator() : hash_map(nullptr), index(0) {}

        iterator(HashMap<KeyType, ValueType, Hash> *_hash_map, size_t _index) : hash_map(_hash_map), index(_index) {}

        iterator &operator=(iterator other) {
            hash_map = other.hash_map;
            index = other.index;
            return *this;
        }

        std::pair<const KeyType, ValueType> &operator*() {
            if (index >= hash_map->size()) {
                throw std::out_of_range("Invalid iterator");
            }
            return hash_map->array[hash_map->data[hash_map->index_list[index]].array_id];
        }

        std::pair<const KeyType, ValueType> *operator->() {
            if (index >= hash_map->size()) {
                throw std::out_of_range("Invalid iterator");
            }
            return &(hash_map->array[hash_map->data[hash_map->index_list[index]].array_id]);
        }

        iterator &operator++() {
            ++index;
            return *this;
        }

        iterator operator++(int) {
            iterator return_value = *this;
            ++index;
            return return_value;
        }

        bool operator==(const HashMap<KeyType, ValueType, Hash>::iterator& other_iterator) const {
            return (this->hash_map) == (other_iterator.hash_map) && this->index == other_iterator.index;
        }

        bool operator!=(const HashMap<KeyType, ValueType, Hash>::iterator& other_iterator) const {
            return (this->hash_map) != (other_iterator.hash_map) || this->index != other_iterator.index;
        }

    private:
        HashMap<KeyType, ValueType, Hash> *hash_map;
        size_t index = 0;
    };

    class const_iterator {
    public:
        const_iterator() : hash_map(nullptr), index(0) {}

        const_iterator(const HashMap<KeyType, ValueType, Hash> *_hash_map, size_t _index) : hash_map(_hash_map),
                                                                                      index(_index) {}

        const_iterator &operator=(const_iterator other) {
            hash_map = other.hash_map;
            index = other.index;
            return *this;
        }

        const std::pair<const KeyType, ValueType> &operator*() const {
            if (index >= hash_map->size()) {
                throw std::out_of_range("Invalid iterator");
            }
            return hash_map->array[hash_map->data[hash_map->index_list[index]].array_id];
        }

        const std::pair<const KeyType, ValueType> *operator->() const {
            if (index >= hash_map->size()) {
                throw std::out_of_range("Invalid iterator");
            }
            return &(hash_map->array[hash_map->data[hash_map->index_list[index]].array_id]);
        }

        const_iterator &operator++() {
            ++index;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator return_value = *this;
            ++index;
            return return_value;
        }

        bool operator==(HashMap<KeyType, ValueType, Hash>::const_iterator other_iterator) const {
            return (this->hash_map) == (other_iterator.hash_map) && this->index == other_iterator.index;
        }

        bool operator!=(HashMap<KeyType, ValueType, Hash>::const_iterator other_iterator) const {
            return (this->hash_map) != (other_iterator.hash_map) || this->index != other_iterator.index;
        }

    private:
        const HashMap<KeyType, ValueType, Hash> *hash_map;
        size_t index = 0;
    };

    HashMap(const Hash &_hasher = Hash()) : hasher(_hasher), mod(12) {
        data.resize(mod);
        index_list.reserve(load_factor * mod + 1);
    }

    template<typename iteratorType>
    HashMap(const iteratorType &_begin, const iteratorType &_end, const Hash _hasher = Hash()) : hasher(_hasher) {
        mod = 12;
        data.resize(mod);
        index_list.reserve(load_factor * mod + 1);
        iteratorType current = _begin;
        while (current != _end) {
            insert(*current);
            ++current;
        }
    }

    HashMap(const std::initializer_list<std::pair<const KeyType, ValueType>> &initializer_list,
            const Hash _hasher = Hash()) : hasher(_hasher) {
        mod = 12;
        data.resize(mod);
        index_list.reserve(load_factor * mod + 1);
        for (const auto &element: initializer_list) {
            insert(element);
        }
    }

    HashMap<KeyType, ValueType, Hash> &operator=(HashMap<KeyType, ValueType, Hash> other) {
        hasher = other.hasher;
        mod = other.mod;
        array.clear();
        for (auto& el : other.array) {
            array.push_back(el);
        }
        data = other.data;
        index_list = other.index_list;
        return *this;
    }

    size_t size() const {
        return index_list.size();
    }

    bool empty() const {
        return size() == 0;
    }

    Hash hash_function() const {
        return hasher;
    }

    void insert(std::pair<const KeyType, ValueType> element) {
        size_t index = hasher(element.first) % mod;
        Data current_data{.robin_hood_counter = 1, .array_id = array.size(), .index_list_id = index_list.size()};
        array.push_back(element);
        index_list.push_back(index);
        while (data[index].robin_hood_counter >= 1) {
            if (array[current_data.array_id].first == array[data[index].array_id].first) {
                index_list.pop_back();
                array.pop_back();
                return;
            }
            if (current_data.robin_hood_counter > data[index].robin_hood_counter) {
                std::swap(current_data.robin_hood_counter, data[index].robin_hood_counter);
                std::swap(current_data.array_id, data[index].array_id);
            }
            ++current_data.robin_hood_counter;
            index = next_index(index);
            index_list.back() = next_index(index_list.back());
        }
        data[index] = current_data;
        if (load_factor * mod <= size()) {
            rebuild(2 * mod);
        }
    }

    void erase(const KeyType &key) {
        size_t index = hasher(key) % mod;
        bool flag = false;
        while (data[index].robin_hood_counter >= 1) {
            if (key != array[data[index].array_id].first) {
                index = next_index(index);
            } else {
                if (data[index].index_list_id != size() - 1) {
                    std::swap(data[index].index_list_id, data[index_list.back()].index_list_id);
                    std::swap(index_list[data[index].index_list_id], index_list[data[index_list.back()].index_list_id]);
                }
                index_list.pop_back();
                data[index] = Data{.robin_hood_counter = 0, .array_id = 0, .index_list_id = 0};
                flag = true;
                break;
            }
        }
        if (!flag) {
            return;
        }
        size_t prev_index = index;
        while (true) {
            index = next_index(index);
            if (data[index].robin_hood_counter <= 1) {
                break;
            }
            --data[index].robin_hood_counter;
            index_list[data[index].index_list_id] = prev_index;
            std::swap(data[index], data[prev_index]);
            prev_index = index;
        }
    }

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, size());
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    const_iterator end() const {
        return const_iterator(this, size());
    }

    iterator find(KeyType key) {
        size_t index = hasher(key) % mod;
        while (data[index].robin_hood_counter >= 1) {
            if (array[data[index].array_id].first == key) {
                return iterator(this, data[index].index_list_id);
            }
            index = next_index(index);
        }
        return iterator(this, size());
    }

    const_iterator find(KeyType key) const {
        size_t index = hasher(key) % mod;
        while (data[index].robin_hood_counter >= 1) {
            if (array[data[index].array_id].first == key) {
                return const_iterator(this, data[index].index_list_id);
            }
            index = next_index(index);
        }
        return const_iterator(this, size());
    }

    ValueType &operator[](KeyType key) {
        iterator it = find(key);
        if (it != iterator(this, size())) {
            return (*it).second;
        } else {
            insert(std::make_pair(key, ValueType()));
            return (*find(key)).second;
        }
    }

    ValueType &operator[](KeyType key) const {
        const_iterator it = find(key);
        if (it != const_iterator(this, size())) {
            return (*it).second;
        } else {
            throw std::out_of_range("This key doesn't exist");
        }
    }

    const ValueType &at(KeyType key) const {
        const_iterator it = find(key);
        if (it != end()) {
            return it->second;
        } else {
            throw std::out_of_range("This key doesn't exist");
        }
    }

    void clear() {
        array.clear();
        index_list.clear();
        data.clear();
        mod = 12;
        data.resize(mod);
        index_list.reserve(load_factor * mod + 1);
    }

private:
    struct Data {
        size_t robin_hood_counter = 0;
        size_t array_id = 0;
        size_t index_list_id = 0;
    };
    Hash hasher;
    size_t mod;
    std::vector<std::pair<const KeyType, ValueType>> array;
    std::vector<Data> data;
    std::vector<size_t> index_list;

    const long double load_factor = 0.65;

private:
    size_t next_index(size_t index) const {
        ++index;
        if (index == mod) {
            index = 0;
        }
        return index;
    }

    void rebuild(size_t new_mod) {
        std::vector<std::pair<const KeyType, ValueType>> tmp_array;
        std::vector<Data> tmp_data;
        std::vector<size_t> tmp_index_list;
        std::swap(tmp_array, array);
        std::swap(tmp_data, data);
        std::swap(tmp_index_list, index_list);
        mod = new_mod;
        data.resize(mod);
        index_list.reserve(load_factor * mod + 1);
        for (size_t &i: tmp_index_list) {
            insert(tmp_array[tmp_data[i].array_id]);
        }
    }
};
