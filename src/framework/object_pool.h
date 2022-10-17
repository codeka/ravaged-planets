#pragma once

#include <stack>
#include <memory>
#include <optional>

namespace fw {

// SharedObjectPool can be used to store a pool of objects of type T. This can help avoid lots of allocations and
// deallocations when objects are reused frequently. The returned std::unique_ptr will automatically return the object
// to the pool when it is destroyed.
//
// Usage:
// 
//   ObjectPool<Foo> pool;
//   {
//     auto f1 = pool.get_or_new();
//     fi->do_something();
//   } // f1 returned to pool
// 
//   std::shared_ptr<Foo> f2 = pool.get_or_new(); // shared_ptr will return instance to pool when count reaches 0.
//
// This class is not thread-safe. All methods should be called on the same thread (or externally locked).
template<typename T>
class ObjectPool {
private:
  // List of available objects.
  std::stack<T*> available_;

  // This shared_ptr allows us to track in the Deleter class when we've been destroyed, so the deleter knows whether
  // to return objects to the pool or just destroy them, too.
  std::shared_ptr<ObjectPool<T>*> pool_;

  struct Deleter {
    explicit Deleter(std::weak_ptr<ObjectPool<T>*> pool) : pool_(pool) {}

    void operator()(T* ptr) {
      auto pool = pool_.lock();
      if (pool) {
        (*pool)->available_.push(ptr);
      } else {
        // The pool's been destroyed, just delete the object.
        std::default_delete<T>{}(ptr);
      }
    }

  private:
    std::weak_ptr<ObjectPool<T>*> pool_;
  };

public:
  // This is the type of the object we'll return.
  using object_ptr = std::unique_ptr<T, Deleter>;

  ObjectPool(int initial_size = 0);
  ~ObjectPool() = default;

  // Get an instance from the pool or create a new one if the pool is empty.
  object_ptr get_or_new();

  // Get an instance from the pool, returns nullptr if the pool is empty.
  object_ptr get();

  // Returns the current size of the pool (number of objects pooled), not counting objects currently in use.
  size_t size() const;
};

template<typename T>
ObjectPool<T>::ObjectPool(int initial_size/*= 0*/)
  : pool_(new ObjectPool<T>*(this)) {
  for (int i = 0; i < initial_size; i++) {
    available_.push(new T());
  }
}

template<typename T>
typename ObjectPool<T>::object_ptr ObjectPool<T>::get() {
  if (size() == 0) {
    return object_ptr(nullptr, Deleter(std::weak_ptr<ObjectPool<T>*>(pool_)));
  }

  object_ptr tmp(available_.top(), Deleter(std::weak_ptr<ObjectPool<T>*>(pool_)));
  available_.pop();
  return std::move(tmp);
}

template<typename T>
typename ObjectPool<T>::object_ptr ObjectPool<T>::get_or_new() {
  auto value = get();
  if (!value) {
    return object_ptr(new T(), Deleter(std::weak_ptr<ObjectPool<T>*>(pool_)));
  }

  return std::move(value);
}

template<typename T>
size_t ObjectPool<T>::size() const {
  return available_.size();
}

}  // namespace fw
