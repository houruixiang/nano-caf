//
// Created by Darwin Yuan on 2020/8/22.
//

#include <nano-caf/core/timer/actor_timer.h>
#include <nano-caf/core/msg/make_message.h>
#include <nano-caf/util/likely.h>
#include <nano-caf/core/actor/actor_handle.h>
#include <nano-caf/core/actor/task_list.h>
#include <nano-caf/util/caf_log.h>

NANO_CAF_NS_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::go_sleep() -> status_t {
   if(timers_.empty()) {
      cv_.wait([this]() {
         return shutdown_.shutdown_notified() || !msg_queue_.blocked();
      });
      return shutdown_.shutdown_notified() ? status_t::system_shutdown : status_t::ok;
   } else {
      auto result = timers_.wait_for_timer_due(cv_, shutdown_);
      return result;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////
inline auto actor_timer::try_go_sleep() -> status_t {
   if(msg_queue_.try_block()) return go_sleep();
   return status_t::ok;
}

/////////////////////////////////////////////////////////////////////////////////////////////
inline auto actor_timer::process(message* msgs) -> status_t {
   task_list queue{};
   while (msgs != nullptr) {
      auto ptr = msgs;
      msgs = msgs->next_;
      queue.push_front(ptr);
   }
   msgs = queue.take_all();
   return msgs == nullptr ? try_go_sleep() : timers_.handle_msgs(msgs, shutdown_);
}

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::schedule() -> void {
   while(process(msg_queue_.take_all()) != status_t::system_shutdown);
   msg_queue_.close();
   timers_.reset();
}

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::start() -> void {
   thread_ = std::thread([this]{ schedule(); });
}

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::stop() -> void {
   shutdown_.notify_shutdown();
   cv_.wake_up();
   thread_.join();
}

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::send_msg(message* msg) -> status_t {
   switch(msg_queue_.enqueue(msg)) {
      case enq_result::ok: {
         cv_.wake_up();
         return status_t::ok;
      }
      case enq_result::blocked: {
         cv_.wake_up();
         return status_t::ok;
      }
      case enq_result::null_msg: return status_t::null_msg;
      case enq_result::closed:   return status_t::msg_queue_closed;
      default: return status_t::failed;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::start_timer
      ( intrusive_actor_ptr sender
      , timer_spec const& spec
      , bool periodic
      , std::shared_ptr<timeout_callback_t> callback) -> result_t<timer_id_t> {
   if(__unlikely(!sender)) { return status_t::null_sender; }

   timer_id_t id{timer_id_.fetch_add(1, std::memory_order_relaxed)};
   auto status = send_msg(make_message<start_timer_msg>(
      id, std::move(sender), spec, std::chrono::steady_clock::now(), periodic, std::move(callback)));
   if(status != status_t::ok) return status;
   return id;
}

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::stop_timer(const intrusive_actor_ptr& self, timer_id_t id) -> status_t {
   if(__unlikely(!self)) { return status_t::null_sender; }
   return send_msg(make_message<stop_timer_msg>(self.actor_id(), id));
}

/////////////////////////////////////////////////////////////////////////////////////////////
auto actor_timer::clear_actor_timer(const intrusive_actor_ptr& self) -> status_t {
   if(__unlikely(!self)) { return status_t::null_sender; }
   return send_msg(make_message<clear_actor_timer_msg>(self.actor_id()));
}

NANO_CAF_NS_END
