#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <unordered_map>

#include <clap/private/macros.h>

namespace clap { namespace helpers {

#ifdef CLAP_HAS_CXX20
   template <typename T>
   concept UpdatableValue = requires(T &a, const T &b) {
      // update a with b, b being newer than a
      { a.update(b) };
   };
#endif

   // TODO: when switching to C++20
   // template <typename K, UpdatableValue V>
   template <typename K, typename V>
   class ReducingParamQueue {
   public:
      using key_type = K;
      using value_type = V;
      using queue_type = std::unordered_map<key_type, value_type>;
      using consumer_type = const std::function<void(const key_type &key, const value_type &value)>;

      ReducingParamQueue();

      void setCapacity(size_t capacity);

      void set(const key_type &key, const value_type &value);
      void setOrUpdate(const key_type &key, const value_type &value);
      void producerDone();

      void consume(const consumer_type &consumer);

      void reset();

   private:
      queue_type _queues[2];
      std::atomic<queue_type *> _free = nullptr;
      std::atomic<queue_type *> _producer = nullptr;
      std::atomic<queue_type *> _consumer = nullptr;
   };
}} // namespace clap::helpers