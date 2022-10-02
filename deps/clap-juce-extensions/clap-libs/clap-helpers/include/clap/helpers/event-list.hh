#pragma once

#include <cstring>
#include <cstdint>
#include <vector>

#include <clap/events.h>

#include "heap.hh"

namespace clap { namespace helpers {

   class EventList {
   public:
      explicit EventList(uint32_t initialHeapSize = 4096,
                         uint32_t initialEventsCapacity = 128,
                         uint32_t maxEventSize = 1024)
         : _maxEventSize(maxEventSize), _heap(initialHeapSize) {
         _events.reserve(initialEventsCapacity);
      }

      EventList(const EventList &) = delete;
      EventList(EventList &&) = delete;
      EventList &operator=(const EventList &) = delete;
      EventList &operator=(EventList &&) = delete;

      static const constexpr size_t SAFE_ALIGN = 8;

      void reserveEvents(size_t capacity) { _events.reserve(capacity); }

      void reserveHeap(size_t size) { _heap.reserve(size); }

      clap_event_header *allocate(size_t align, size_t size) {
         assert(size >= sizeof(clap_event_header));
         if (size > _maxEventSize)
            return nullptr;

         // ensure we have space to store into the vector
         if (_events.capacity() == _events.size())
            _events.reserve(_events.capacity() * 2);

         auto ptr = _heap.allocate(align, size);
         _events.push_back(_heap.offsetFromBase(ptr));
         auto hdr = static_cast<clap_event_header *>(ptr);
         hdr->size = size;
         return hdr;
      }

      clap_event_header *tryAllocate(size_t align, size_t size) {
         assert(size >= sizeof(clap_event_header));
         if (size > _maxEventSize)
            return nullptr;

         // ensure we have space to store into the vector
         if (_events.capacity() == _events.size())
            return nullptr;

         auto ptr = _heap.tryAllocate(align, size);
         if (!ptr)
            return nullptr;

         _events.push_back(_heap.offsetFromBase(ptr));
         auto hdr = static_cast<clap_event_header *>(ptr);
         hdr->size = size;
         return hdr;
      }

      template <typename T>
      T *allocate() {
         return allocate(alignof(T), sizeof(T));
      }

      void push(const clap_event_header *h) {
         auto ptr = allocate(SAFE_ALIGN, h->size);
         if (!ptr)
            return;

         std::memcpy(ptr, h, h->size);
      }

      bool tryPush(const clap_event_header *h) {
         auto ptr = tryAllocate(SAFE_ALIGN, h->size);
         if (!ptr)
            return false;

         std::memcpy(ptr, h, h->size);
         return true;
      }

      clap_event_header *get(uint32_t index) const {
         const auto offset = _events.at(index);
         auto const ptr = _heap.ptrFromBase(offset);
         return static_cast<clap_event_header *>(ptr);
      }

      size_t size() const { return _events.size(); }

      bool empty() const { return _events.empty(); }

      void clear() {
         _heap.clear();
         _events.clear();
      }

      const clap_input_events *clapInputEvents() const noexcept { return &_inputEvents; }

      const clap_output_events *clapOutputEvents() const noexcept { return &_outputEvents; }

      const clap_output_events *clapBoundedOutputEvents() const noexcept {
         return &_boundedOutputEvents;
      }

   private:
      static uint32_t clapSize(const struct clap_input_events *list) {
         auto *self = static_cast<const EventList *>(list->ctx);
         return self->size();
      }

      static const clap_event_header_t *clapGet(const struct clap_input_events *list,
                                                uint32_t index) {
         auto *self = static_cast<const EventList *>(list->ctx);
         return self->get(index);
      }

      static bool clapPushBack(const struct clap_output_events *list,
                               const clap_event_header *event) {
         auto *self = static_cast<EventList *>(list->ctx);
         self->push(event);
         return true;
      }

      static bool clapBoundedPushBack(const struct clap_output_events *list,
                                      const clap_event_header_t *event) {
         auto *self = static_cast<EventList *>(list->ctx);
         return self->tryPush(event);
      }

      const clap_input_events _inputEvents = {this, &clapSize, &clapGet};
      const clap_output_events _outputEvents = {this, &clapPushBack};
      const clap_output_events _boundedOutputEvents = {this, &clapBoundedPushBack};

      const uint32_t _maxEventSize;

      Heap _heap;
      std::vector<uint32_t> _events;
   };

}} // namespace clap::helpers