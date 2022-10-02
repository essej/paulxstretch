#pragma once

#include <cassert>

#include "reducing-param-queue.hh"

namespace clap { namespace helpers {
   template <typename K, typename V>
   ReducingParamQueue<K, V>::ReducingParamQueue() {
      reset();
   }

   template <typename K, typename V>
   void ReducingParamQueue<K, V>::reset() {
      for (auto &q : _queues)
         q.clear();

      _free = &_queues[0];
      _producer = &_queues[1];
      _consumer = nullptr;
   }

   template <typename K, typename V>
   void ReducingParamQueue<K, V>::setCapacity(size_t capacity) {
      for (auto &q : _queues)
         q.reserve(2 * capacity);
   }

   template <typename K, typename V>
   void ReducingParamQueue<K, V>::set(const key_type &key, const value_type &value) {
      _producer.load()->insert_or_assign(key, value);
   }

   template <typename K, typename V>
   void ReducingParamQueue<K, V>::setOrUpdate(const key_type &key, const value_type &value) {
      auto prod = _producer.load();
      auto res = prod->insert({key, value});
      if (!res.second && res.first != prod->end())
         res.first->second.update(value);
   }

   template <typename K, typename V>
   void ReducingParamQueue<K, V>::producerDone() {
      if (_consumer)
         return;

      auto tmp = _producer.load();
      _producer = _free.load();
      _free = nullptr;
      _consumer = tmp;

      assert(_producer);
   }

   template <typename K, typename V>
   void ReducingParamQueue<K, V>::consume(const consumer_type &consumer) {
      assert(consumer);

      if (!_consumer)
         return;

      for (auto &x : *_consumer)
         consumer(x.first, x.second);

      _consumer.load()->clear();
      if (_free)
         return;

      _free = _consumer.load();
      _consumer = nullptr;
   }
}} // namespace clap::helpers