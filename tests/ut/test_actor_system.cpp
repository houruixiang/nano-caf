//
// Created by Darwin Yuan on 2020/7/24.
//

#include <catch.hpp>
#include <nano-caf/core/actor/message_element.h>
#include <nano-caf/core/actor/sched_actor.h>
#include <nano-caf/core/actor_system.h>
#include <iostream>
#include <nano-caf/core/actor/actor.h>

namespace {
   using namespace NANO_CAF_NS;

   struct my_message  {
      my_message(uint32_t value)
         : value{value} {}

      uint32_t value{};
   };

   SCENARIO("a do-nothing actor system") {
      actor_system system;
      system.start(5);
      system.shutdown();
   }

   struct my_actor : actor {
      std::vector<size_t> values;
      auto handle_message(const message_element& msg) noexcept -> void override {
         values.push_back(msg.body<my_message>()->value);
      }

      void clear() {
         values.clear();
      }
   };

   SCENARIO("actor system") {
      actor_system system;
      system.start(5);

      auto actor = system.spawn<my_actor>();

      actor.send<my_message>({1}, 1);

      system.power_off();
   }

   int pong_times = 0;
   struct pong_actor : actor {

      auto handle_message(const message_element& msg) noexcept -> void override {
         reply(1);
         pong_times++;
      }
   };

   struct ping_actor : actor {
      actor_handle pong;
      size_t times = 0;

      auto on_init() noexcept -> void override {
         pong = spawn<pong_actor>();
         send_to(pong, 1);
         times = 1;
      }

      auto handle_message(const message_element& msg) noexcept -> void override {
         if(times++ < 100) {
            //std::this_thread::sleep_for(std::chrono::microseconds{100000});
            send_to(pong, 1);
         } else {
            exit(exit_reason::normal);
         }
      }
   };

   SCENARIO("ping pang") {
      pong_times = 0;
      actor_system system;
      system.start(3);
      REQUIRE(system.get_num_of_actors() == 0);
      auto me = system.spawn<ping_actor>();
      REQUIRE(system.get_num_of_actors() == 2);
      me.release();
      system.shutdown();
      REQUIRE(system.get_num_of_actors() == 0);
      REQUIRE(pong_times == 100);
   }
}