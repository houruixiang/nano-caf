//
// Created by Darwin Yuan on 2020/7/22.
//

#ifndef NANO_CAF_DRR_CACHED_QUEUE_H
#define NANO_CAF_DRR_CACHED_QUEUE_H

#include <nano-caf/nano-caf-ns.h>
#include <nano-caf/core/msg/message.h>
#include <nano-caf/core/actor/task_list.h>
#include <nano-caf/core/thread_pool/new_round_result.h>
#include <cstddef>
#include <functional>
#include <nano-caf/core/actor/task_result.h>

NANO_CAF_NS_BEGIN

using message_consumer = std::function<auto (const message&) -> task_result>;

struct drr_cached_queue : private task_list {
   using task_list::append_list;
   auto new_round(size_t quota, message_consumer f) noexcept -> new_round_result;
   auto inc_deficit(size_t quota) noexcept -> void;
   auto empty() const noexcept -> bool {
      return task_list::empty() && cache_.empty();
   }

   auto deficit() noexcept -> size_t {
      auto result = deficit_;
      deficit_ = 0;
      return result;
   }

private:
   auto next() noexcept -> std::unique_ptr<message>;
   auto flush_cache() noexcept -> void;

private:
   task_list cache_{};
   size_t deficit_{};
};

NANO_CAF_NS_END

#endif //NANO_CAF_DRR_CACHED_QUEUE_H
