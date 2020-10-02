//
// Created by Darwin Yuan on 2020/9/21.
//

#ifndef NANO_CAF_FUTURE_OBJECT_BASE_H
#define NANO_CAF_FUTURE_OBJECT_BASE_H

#include <nano-caf/core/await/abstract_future.h>
#include <nano-caf/core/actor/on_actor_context.h>
#include <nano-caf/core/await/detail/future_observer.h>
#include <nano-caf/core/await/failure_handler.h>
#include <nano-caf/util/status_t.h>
#include <memory>
#include <deque>

NANO_CAF_NS_BEGIN namespace detail {

template<typename T> struct future_callback_trait {
   using type = std::function<auto(T) -> void>;
};

template<> struct future_callback_trait<void> {
   using type = std::function<auto() -> void>;
};

template<typename T>
struct future_object_base : abstract_future, promise_done_notifier {
   using callback_type = typename future_callback_trait<T>::type;
   using observer_type = future_observer*;

   explicit future_object_base(on_actor_context& context) noexcept
      : registered_{0}, ready_{0}, context_{context} {}

   auto set_fail_handler(failure_handler&& handler) noexcept -> void {
      if(f_on_fail_) return;
      if(!ready_) { f_on_fail_ = std::move(handler); }
      else if(!present_) { handler(failure_); }
   }

   auto move_observers(future_object_base<T>& to) noexcept -> void {
      for (auto &observer: observers_) { to.add_observer(observer); }
      observers_.clear();
      if(f_on_fail_) { to.set_fail_handler(std::move(f_on_fail_)); }
   }

   auto add_observer(observer_type observer) noexcept -> void {
      if(ready_) { notify_observer_(observer); }
      else { observers_.emplace_back(std::move(observer)); }
   }

   auto commit() noexcept -> void override {
      if (ready_ ) return;
      ready_ = true;
      if(!present_ && static_cast<bool>(f_on_fail_)) { f_on_fail_(failure_); }
      notify_observers();
      destroy();
   }

   virtual auto cancel(status_t cause) noexcept -> void { do_cancel(cause); }

   auto deregister_observer(future_observer* observer, status_t cause) noexcept -> void {
      auto result = std::find(observers_.begin(), observers_.end(), observer);
      if(result != observers_.end()) {
         observers_.erase(result);
         if(observers_.empty()) { cancel(cause); }
      }
   }

   auto on_fail(status_t cause) noexcept -> void {
      if (present_ || ready_) return;
      failure_ = cause;
   }

   auto ready() const noexcept -> bool { return ready_; }

   auto destroy() noexcept -> void {
      if(!registered_) return;
      context_.remove_future(this);
      registered_ = false;
   }

   auto on_registered() noexcept -> void override { registered_ = true; }
   auto get_context() const noexcept -> on_actor_context* { return &context_; }

   virtual ~future_object_base() noexcept = default;

protected:
   auto cancel_observers(status_t cause) noexcept -> void {
      for (auto &observer: observers_) {
         cancel_observer(observer, cause);
      }
      observers_.clear();
   }

   auto notify_observers() noexcept -> void {
      for (auto &observer: observers_) { notify_observer_(observer); }
      observers_.clear();
   }

private:
   auto do_cancel(status_t cause) noexcept -> void {
      if(ready_) return;
      ready_ = true;
      if(f_on_fail_) f_on_fail_(cause);
      cancel_observers(cause);
      destroy();
   }

   auto cancel_observer(observer_type &observer, status_t cause) noexcept -> void {
      observer->on_future_fail(cause);
   }

   auto notify_observer_(observer_type &observer) noexcept -> void {
      if(present_) observer->on_future_ready();
      else observer->on_future_fail(failure_);
   }

private:
   on_actor_context& context_;
   std::deque<observer_type> observers_;
   status_t failure_{};
   failure_handler f_on_fail_;

protected:
   bool present_{false};
   uint8_t registered_:1;
   uint8_t ready_:1;
};

} NANO_CAF_NS_END

#endif //NANO_CAF_FUTURE_OBJECT_BASE_H
