//
// Created by Darwin Yuan on 2020/7/23.
//

#ifndef NANO_CAF_ACTOR_CONTROL_BLOCK_H
#define NANO_CAF_ACTOR_CONTROL_BLOCK_H

#include <nano-caf/nano-caf-ns.h>
#include <nano-caf/core/cache_line_size.h>
#include <nano-caf/util/intrusive_ptr.h>
#include <nano-caf/util/either.h>
#include <nano-caf/util/status_t.h>
#include <nano-caf/core/actor/exit_reason.h>
#include <cstddef>
#include <atomic>
#include <future>

NANO_CAF_NS_BEGIN

struct sched_actor;
struct system_actor_context;
struct intrusive_actor_ptr;

struct actor_control_block {
   using data_destructor = void (*)(sched_actor*);
   using block_destructor = void (*)(actor_control_block*);

   actor_control_block(system_actor_context& context, data_destructor data_dtor, block_destructor block_dtor)
      : strong_refs_{1}, weak_refs_{1}, data_dtor_{data_dtor}, block_dtor_{block_dtor}, context_(context)
   {}

   actor_control_block(const actor_control_block&) = delete;
   actor_control_block& operator=(const actor_control_block&) = delete;

   auto context() const -> system_actor_context& {
      return context_;
   }

   auto use_count() const noexcept -> std::size_t {
      return strong_refs_.load(std::memory_order_relaxed);
   }

   // XXX: only strong ref should call this
   auto get() noexcept -> sched_actor* {
      return reinterpret_cast<sched_actor*>(reinterpret_cast<char*>(this) + CACHE_LINE_SIZE);
   }

   auto wait_for_exit() -> exit_reason {
      return exit_promise_.get_future().get();
   }

   auto wait_exit(const std::chrono::nanoseconds& duration) -> either<exit_reason, status_t> {
      auto result = exit_promise_.get_future().wait_for(duration);
      if(result != std::future_status::ready) {
         return status_t::timeout;
      }

      return exit_promise_.get_future().get();
   }

   auto on_exit(exit_reason reason) {
      exit_promise_.set_value(reason);
   }

   ~actor_control_block() noexcept = default;

public:
   inline friend auto intrusive_ptr_add_weak_ref(actor_control_block* x) noexcept -> void {
      x->weak_refs_.fetch_add(1, std::memory_order_relaxed);
   }

   inline friend auto intrusive_ptr_add_ref(actor_control_block* x) noexcept -> void {
      x->strong_refs_.fetch_add(1, std::memory_order_relaxed);
   }

   friend auto intrusive_ptr_release_weak(actor_control_block* x) noexcept -> void;
   friend auto intrusive_ptr_release(actor_control_block* x) noexcept -> void;

   friend auto intrusive_ptr_upgrade_weak(actor_control_block* x) noexcept -> intrusive_actor_ptr;

private:
   std::atomic<size_t> strong_refs_;
   std::atomic<size_t> weak_refs_;
   const data_destructor data_dtor_;
   const block_destructor block_dtor_;
   std::promise<exit_reason> exit_promise_{};

protected:
   system_actor_context& context_;
};

NANO_CAF_NS_END

#endif //NANO_CAF_ACTOR_CONTROL_BLOCK_H
