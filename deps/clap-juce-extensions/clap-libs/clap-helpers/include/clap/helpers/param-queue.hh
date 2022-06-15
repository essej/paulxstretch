#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>

namespace clap { namespace helpers {
   template <typename T, size_t CAPACITY>
   class ParamQueue {
   public:
      using value_type = T;

      ParamQueue() = default;

      void push(const T &value) {
         while (!tryPush(value))
            ;
      }

      bool tryPush(const T &value) {
         int w = _writeOffset;        // write element
         int wn = (w + 1) % CAPACITY; // next write element

         if (wn == _readOffset)
            return false;

         _data[w] = value;
         _writeOffset = wn;
         return true;
      }

      bool tryPeek(T &value) {
         int r = _readOffset;
         if (r == _writeOffset)
            return false; // empty

         value = _data[r];
         return true;
      }

      void consume() {
         int r = _readOffset;
         if (r == _writeOffset) {
            assert(false && "consume should not have been called");
            return; // empty
         }

         int rn = (r + 1) % CAPACITY;
         _readOffset = rn;
      }

      bool tryPop(T &value) {
         if (!tryPeek(value))
            return false;

         consume();
         return true;
      }

      void reset() {
         _writeOffset = 0;
         _readOffset = 0;
      }

   private:
      std::array<T, CAPACITY> _data;
      std::atomic<int> _writeOffset = {0};
      std::atomic<int> _readOffset = {0};
   };
}} // namespace clap::helpers