//
// Created by Darwin Yuan on 2020/7/23.
//

#include <nano-caf/core/drr_cached_queue.h>
#include <nano-caf/core/message_element.h>

NANO_CAF_NS_BEGIN

auto drr_cached_queue::new_round(size_t quota, message_consumer f) noexcept -> new_round_result {
   if(task_list::empty()) return {0, false};

   deficit_ += quota;

   size_t consumed = 0;
   for(auto ptr = next(); ptr != nullptr; ptr = next()) {
      switch(task_result result = f(*ptr); result) {
         case task_result::skip:
            cache_.push_back(ptr.release());
            if(task_list::empty()) return {consumed, false};
            ++deficit_;
            break;
         case task_result::resume:
            ++consumed;
            task_list::prepend_list(cache_);
            if(task_list::empty()) {
               deficit_ = 0;
               return {consumed, false};
            }
            break;
         default:
            ++consumed;
            task_list::prepend_list(cache_);
            if(task_list::empty()) deficit_ = 0;
            return {consumed, result == task_result::stop_all};
      }
   }

   return {consumed, false};
}

NANO_CAF_NS_END