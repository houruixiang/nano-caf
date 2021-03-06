//
// Created by Darwin Yuan on 2020/7/24.
//

#include <nano-caf/core/msg/message.h>
#include <nano-caf/core/actor/sched_actor.h>
#include <nano-caf/core/actor_system.h>
#include <nano-caf/core/actor/actor.h>
#include <iostream>
#include <nano-caf/core/actor/behavior_based_actor.h>
#include "test_msgs.h"
#include <catch.hpp>


namespace {
   using namespace NANO_CAF_NS;

   SCENARIO("a do-nothing actor system") {
      actor_system system{5};
      system.shutdown();
   }

   struct my_actor : actor {
      std::vector<int> values;
      auto handle_message(message& msg) noexcept -> task_result override {
         values.push_back(msg.body<test_message>()->value);
         return task_result::resume;
      }

      void clear() {
         values.clear();
      }
   };

   SCENARIO("actor system") {
      actor_system system{5};

      auto actor = system.spawn<my_actor>();

      actor.send<test_message>(1);

      system.power_off();
   }

   int pong_times = 0;
   struct pong_actor : actor {
      auto handle_message(message& msg) noexcept -> task_result override {
         if(msg.msg_type_id_ == test_message::type_id) {
            reply<test_message>(msg.body<test_message>()->value);
            pong_times++;
         }
         else if(msg.msg_type_id_ == exit_msg::type_id) {
         }

         return task_result::resume;
      }
   };

   struct ping_actor : actor {
      actor_handle pong;
      size_t times = 0;

      auto on_init() noexcept -> void {
         pong = spawn<pong_actor>();
         send<test_message>(pong, 1);
         times = 1;
      }

      auto handle_message(message& msg) noexcept -> task_result override {
         if(msg.msg_type_id_ == test_message::type_id) {
            times++;
            send<test_message>(pong, msg.body<test_message>()->value + 1);
         } else if(msg.msg_type_id_ == exit_msg::type_id) {
            send<exit_msg>(pong, msg.body<exit_msg>()->reason);
         }
         return task_result::resume;
      }
   };

   SCENARIO("ping pang") {
      pong_times = 0;
      actor_system system{1};
      REQUIRE(system.get_num_of_actors() == 0);

      auto me = system.spawn<ping_actor>();
      //REQUIRE(system.get_num_of_actors() == 2);
      std::this_thread::sleep_for(std::chrono::seconds {1});
      me.send<exit_msg>(exit_reason::user_shutdown);
      REQUIRE(me.wait_for_exit() == exit_reason::user_shutdown);
      me.release();

      system.shutdown();
      //REQUIRE(pong_times == total_times);
      REQUIRE(system.get_num_of_actors() == 0);
   }

   struct future_actor : actor {
      const size_t value = 10;
      size_t final_result = 0;

      auto add(size_t a, size_t b) {
         size_t result = 0;
         for(size_t i = 0; i < 1000000; i++) {
            result += (a * b + value);
         }

         return result;
      }

      auto on_init() noexcept -> void {
         auto future1 = async(&future_actor::add, this, 5, 3);
         auto future2 = async([this]() {
            size_t result = 0;
             size_t a = 20;
             size_t b = 4;
            for(size_t i = 0; i < 1000000; i++) {
               result += (a * b + value);
            }

            return result;
         });
         future2.then([](auto) {
            CAF_DEBUG("async2 done");
         });

         auto result3 = with(future1, future2)
            .then([this](unsigned long r1, unsigned long r2) {
               CAF_DEBUG("async done");
               final_result = r1 + r2;
               if(final_result == 115000000) {
                  exit(exit_reason::normal);
               } else {
                  exit(exit_reason::unknown);
               }
            });

         REQUIRE(result3.valid());
      }

      auto handle_message(message&) noexcept -> task_result override {
         return task_result::resume;
      }
   };

   SCENARIO("async test") {
      actor_system system{3};

      auto me = system.spawn<future_actor>();
      me.send<test_message>(1);
      REQUIRE(me.wait_for_exit() == exit_reason::normal);
      me.release();

      system.shutdown();
      REQUIRE(system.get_num_of_actors() == 0);
   }

   int pong_times_2 = 0;
   struct pong_actor_1 : behavior_based_actor {
      auto get_behavior() -> behavior override {
         return {
            [&](test_message_atom, int value) {
               reply<test_message>(value);
               pong_times_2++;
            },
            [&](exit_msg_atom, exit_reason) {
            }
         };
      }
   };

   struct ping_actor_1 : behavior_based_actor {
      actor_handle pong;

      auto on_init() noexcept -> void {
         pong = spawn<pong_actor_1>();
         send<test_message>(pong, 1);
      }

      auto get_behavior() -> behavior override {
         return {
            [&](test_message_atom, int value) {
               send<test_message>(pong, value + 1);
            },
            [&](exit_msg_atom, exit_reason reason) {
               send<exit_msg>(pong, reason);
            },
         };
      }
   };

   SCENARIO("ping pang 2") {
      pong_times = 0;
      actor_system system{1};
      REQUIRE(system.get_num_of_actors() == 0);

      auto me = system.spawn<ping_actor_1>();
      //REQUIRE(system.get_num_of_actors() == 2);
      std::this_thread::sleep_for(std::chrono::seconds {1});
      me.send<exit_msg>(exit_reason::user_shutdown);
      REQUIRE(me.wait_for_exit() == exit_reason::user_shutdown);
      me.release();

      system.shutdown();
//      REQUIRE(pong_times_2 == total_times);
      REQUIRE(system.get_num_of_actors() == 0);
   }
}