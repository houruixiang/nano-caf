//
// Created by Darwin Yuan on 2020/7/26.
//

#ifndef NANO_CAF_ACTOR_CONTEXT_H
#define NANO_CAF_ACTOR_CONTEXT_H

#include <nano-caf/core/system_actor_context.h>

NANO_CAF_NS_BEGIN

struct actor_context {
   template<typename T, typename ... Ts>
   auto spawn(Ts&& ... args) noexcept -> actor_handle {
      return get_system_actor_context().template spawn<T>(std::forward<Ts>(args)...);
   }

   template<typename A, typename T, typename ... Ts>
   auto spawn_typed_actor(Ts&& ... args) noexcept -> typed_actor_handle<A> {
      return get_system_actor_context().template spawn_typed_actor<A, T>(std::forward<Ts>(args)...);
   }

   template<typename T, message::category CATEGORY = message::normal, typename ... Args>
   inline auto send(actor_handle& to, Args&& ... args) noexcept {
      return to.send<T, CATEGORY>(self_handle(), std::forward<Args>(args)...);
   }

   template<typename METHOD_ATOM, message::category CATEGORY = message::normal, typename INTERFACE_TYPE, typename ... Args>
   inline auto send(typed_actor_handle<INTERFACE_TYPE>& to, Args&& ... args) noexcept {
      return to.template send<METHOD_ATOM, CATEGORY>(self_handle(), std::forward<Args>(args)...);
   }

   virtual ~actor_context() = default;

protected:
   virtual auto get_system_actor_context() -> system_actor_context& = 0;
   virtual auto self_handle() const noexcept -> intrusive_actor_ptr = 0;
};

NANO_CAF_NS_END

#endif //NANO_CAF_SYSTEM_ACTOR_CONTEXT_H
