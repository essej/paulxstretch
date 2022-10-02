#include <clap/helpers/param-queue.hh>

#include <catch2/catch.hpp>

namespace {

   CATCH_TEST_CASE("ParamQueue - ctor") { clap::helpers::ParamQueue<uint32_t, 16> queue; }

   CATCH_TEST_CASE("ParamQueue - simple push") {
      clap::helpers::ParamQueue<uint32_t, 16> queue;

      queue.push(45);
      queue.push(46);
      uint32_t v = 0;
      CATCH_CHECK(queue.tryPeek(v));
      CATCH_CHECK(v == 45);
      CATCH_CHECK(queue.tryPeek(v));
      CATCH_CHECK(v == 45);
      CATCH_CHECK(queue.tryPop(v));
      CATCH_CHECK(v == 45);
      CATCH_CHECK(queue.tryPop(v));
      CATCH_CHECK(v == 46);
      CATCH_CHECK(!queue.tryPop(v)); // the queue is empty here
   }

   CATCH_TEST_CASE("ParamQueue - full") {
      clap::helpers::ParamQueue<uint32_t, 4> queue;

      CATCH_CHECK(queue.tryPush(44));
      CATCH_CHECK(queue.tryPush(45));
      CATCH_CHECK(queue.tryPush(46));
      CATCH_CHECK(!queue.tryPush(88)); // the queue is full here

      uint32_t v = 0;
      CATCH_CHECK(queue.tryPop(v));
      CATCH_CHECK(v == 44);
      CATCH_CHECK(queue.tryPop(v));
      CATCH_CHECK(v == 45);
      CATCH_CHECK(queue.tryPop(v));
      CATCH_CHECK(v == 46);
      CATCH_CHECK(!queue.tryPop(v)); // the queue is empty here
   }

   CATCH_TEST_CASE("ParamQueue - concurrent") {
      clap::helpers::ParamQueue<uint32_t, 64> queue;
      const constexpr uint32_t N = 64 * 1024 * 1024;

      bool failed = false;

      std::thread producer([&queue] {
         for (uint32_t i = 0; i < N; ++i)
            queue.push(i);
      });

      std::thread consumer([&queue, &failed] {
         uint32_t v = 0;
         for (uint32_t i = 0; i < N; ++i) {
            while (!queue.tryPop(v))
               std::this_thread::yield();
            failed |= (v != i);
         }
      });

      producer.join();
      consumer.join();

      uint32_t v = 0;
      CATCH_CHECK(!queue.tryPop(v)); // the queue is empty here
      CATCH_CHECK(!failed);
   }

} // namespace