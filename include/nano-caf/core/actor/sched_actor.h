//
// Created by Darwin Yuan on 2020/7/23.
//

#ifndef NANO_CAF_SCHED_ACTOR_H
#define NANO_CAF_SCHED_ACTOR_H

#include <nano-caf/nano-caf-ns.h>
#include <nano-caf/core/resumable.h>
#include <nano-caf/core/actor/actor_inbox.h>
#include <nano-caf/core/cache_line_size.h>
#include "actor_control_block.h"

NANO_CAF_NS_BEGIN

struct sched_actor
   : resumable
   , private actor_inbox {
   using actor_inbox::enqueue;
   using actor_inbox::blocked;
   auto resume() noexcept  -> resumable::result override;

private:
   enum : uint32_t {
      exiting_flag = 0x0000'0001,
   };

   auto handle_message_internal(const message_element& msg) noexcept -> task_result;
   virtual auto handle_message(const message_element&) noexcept -> void {}

private:
   auto intrusive_ptr_add_ref_impl() -> void override {
      intrusive_ptr_add_ref(to_ctl());
   }
   auto intrusive_ptr_release_impl() -> void override {
      intrusive_ptr_release(to_ctl());
   }

private:
   auto to_ctl() -> actor_control_block* {
      return reinterpret_cast<actor_control_block*>((reinterpret_cast<char*>(this) - CACHE_LINE_SIZE));
   }

private:
   uint32_t flags{};
};

NANO_CAF_NS_END

#endif //NANO_CAF_SCHED_ACTOR_H
