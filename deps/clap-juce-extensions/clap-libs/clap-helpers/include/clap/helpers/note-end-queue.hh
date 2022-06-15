#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include <clap/clap.h>

namespace clap { namespace helpers {
   class NoteEndQueue {
   public:
      explicit NoteEndQueue(std::size_t initialBucketSize = 19)
         : _voicesToKill(initialBucketSize) {}

      void onNoteOn(int32_t noteId, int16_t port, int16_t channel, int16_t key) {
         assert(checkValidEntry(noteId, port, channel, key));
         _voicesToKill.erase(Entry(noteId, port, channel, key));
      }

      void onNoteEnd(int32_t noteId, int16_t port, int16_t channel, int16_t key) {
         assert(checkValidEntry(noteId, port, channel, key));
         _voicesToKill.insert(Entry(noteId, port, channel, key));
      }

      void flush(const clap_output_events *out) {
         clap_event_note ev;
         ev.velocity = 0;
         ev.header.flags = 0;
         ev.header.size = sizeof(ev);
         ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
         ev.header.time = 0; // time is irrelevant here
         ev.header.type = CLAP_EVENT_NOTE_END;

         for (auto &e : _voicesToKill) {
            ev.port_index = e._port;
            ev.channel = e._channel;
            ev.key = e._key;
            ev.note_id = e._noteId;
            out->try_push(out, &ev.header);
         }

         _voicesToKill.clear();
      }

   private:
      bool checkValidEntry(int32_t noteId, int16_t port, int16_t channel, int16_t key) {
         assert(noteId >= -1);
         assert(port >= 0);
         assert(channel >= 0 && channel < 16);
         assert(key >= 0 && key < 127);
         return true;
      }

      struct Entry {
         constexpr Entry(int32_t noteId, int16_t port, int16_t channel, int16_t key)
            : _noteId(noteId), _port(port), _channel(channel), _key(key) {}

         constexpr bool operator==(const Entry &o) const noexcept {
            return _port == o._port && _channel == o._channel && _key == o._key;
         }

         const int32_t _noteId;
         const int16_t _port;
         const int16_t _channel;
         const int16_t _key;
      };

      struct EntryHash {
         std::size_t operator()(const Entry &e) const noexcept {
            return e._key ^ (e._channel << 8) ^ (e._port << 16) ^ (int64_t(e._noteId) << 32LL);
         }
      };

      std::unordered_set<Entry, EntryHash> _voicesToKill;
   };
}} // namespace clap::helpers
