//
// Created by 吴凡 on 2018/8/14.
//

#ifndef LOOKUPTABLE_LOOKUPTABLE_H
#define LOOKUPTABLE_LOOKUPTABLE_H

#include <thread>
#include <memory>
#include <vector>
#include <list>
#include <map>

// LookupTable 是一个线程安全的哈希表，支持快速查询、插入及删除操作。
// LookupTable 内部由固定大小的Bucket数组实现，并基于链表处理哈希冲突。
// 由于数组大小固定，我们只需要保证每个Bucket的同步即可。对不同Bucket的操作是线程安全的。
// 来源: <<Cpp Concurrency In Action>> 6.3.1章节

template<class Key, class Value, class Hash = std::hash<Key>>
class LookupTable {
 public:
  using key_type = Key;
  using mapped_type = Value;
  using hash_type = Hash;
 private:
  class Bucket {
   public:
    using bucket_value = std::pair<key_type, mapped_type>;
    using bucket_data = std::list<bucket_value>;
    using bucket_iterator = typename bucket_data::iterator;
    friend class LookupTable;
   public:
    void Insert(key_type key, mapped_type value) {
      std::unique_lock<std::mutex> lock(mutex_);
      auto iterator = FindEntryForKey(key);
      if (iterator != bucket_data_.end()) {
        *iterator = std::make_pair(std::move(key), std::move(value));
      } else {
        bucket_data_.insert(iterator, std::make_pair(std::move(key), std::move(value)));
      }
    }

    mapped_type Query(const key_type &key, const mapped_type &default_value) {
      std::unique_lock<std::mutex> lock(mutex_);
      auto iterator = FindEntryForKey(key);
      return iterator == bucket_data_.end() ? default_value : iterator->second;
    }

    void Delete(const key_type &key) {
      std::unique_lock<std::mutex> lock(mutex_);
      auto iterator = FindEntryForKey(key);
      if (iterator != bucket_data_.end()) {
        bucket_data_.erase(iterator);
      }
    }

   private:
    bucket_iterator FindEntryForKey(const key_type &key) {
      return std::find_if(bucket_data_.begin(), bucket_data_.end(),
                          [&key](const bucket_value &v) {
                            return v.first == key;
                          });
    }
   private:
    std::mutex mutex_; // C++17或Boost可以使用读写锁
    bucket_data bucket_data_;
  };
 public:
  enum { DEFAULT_CAPACITY = 64 };
 public:
  explicit LookupTable(size_t capacity = DEFAULT_CAPACITY, const Hash &hash = Hash())
      : buckets_(capacity), hash_(hash) {
    for (int i = 0; i < capacity; i++) {
      buckets_[i].reset(new Bucket);
    }
  }

  LookupTable(const LookupTable &) = delete;
  LookupTable &operator=(const LookupTable &) = delete;

  void Insert(key_type key, mapped_type value) {
    BucketForKey(key).Insert(std::move(key), std::move(value));
  }

  Value Query(const key_type &key, const mapped_type &default_value) {
    return BucketForKey(key).Query(key, default_value);
  }

  void Delete(const key_type &key) {
    BucketForKey(key).Delete(key);
  }

  std::map<key_type, mapped_type> ToMap() {
    std::map<key_type, mapped_type> map;
    std::vector<std::unique_lock<std::mutex>> locks;
    for (const auto &bucket_ptr : buckets_) {
      locks.push_back(std::unique_lock<std::mutex>(bucket_ptr->mutex_));
    }
    for (const auto &bucket_ptr : buckets_) {
      for (const auto &pair : bucket_ptr->bucket_data_) {
        map[pair.first] = pair.second;
      }
    }
    return map;
  };

 private:
  Bucket &BucketForKey(const Key &key) {
    size_t index = hash_(key) % buckets_.size();
    return *buckets_[index];
  }

 private:
  std::vector<std::unique_ptr<Bucket>> buckets_;
  Hash hash_;
};

#endif //LOOKUPTABLE_LOOKUPTABLE_H
