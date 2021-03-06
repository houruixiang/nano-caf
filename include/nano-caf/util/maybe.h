//
// Created by Darwin Yuan on 2020/8/6.
//

#ifndef NANO_CAF_MAYBE_H
#define NANO_CAF_MAYBE_H

#include <nano-caf/nano-caf-ns.h>
#include <optional>

NANO_CAF_NS_BEGIN

constexpr auto nothing = std::nullopt;

template<typename T>
class maybe : private std::optional<T>{
   using parent = std::optional<T>;
public:
   using parent::parent;

   using parent::operator->;
   using parent::operator*;
   using parent::operator bool;
   using parent::has_value;
   using parent::value;
   using parent::value_or;
   using parent::reset;
   using parent::emplace;

   template<typename F_JUST, typename F_NOTHING>
   constexpr auto match(F_JUST&& f_just, F_NOTHING&& f_nothing) const noexcept {
      static_assert(std::is_invocable_v<F_JUST, const T&>, "f_just type mismatch");
      static_assert(std::is_invocable_v<F_NOTHING>,        "f_nothing type mismatch");
      static_assert(std::is_same_v<std::invoke_result_t<F_JUST, const T&>, std::invoke_result_t<F_NOTHING>>, "result type mismatch");
      return has_value() ? f_just(*value()) : f_nothing();
   }

   template<typename F>
   constexpr auto map(F&& f) const noexcept -> maybe<std::invoke_result_t<F, const T&>> {
      static_assert(std::is_invocable_v<F, const T&>, "map function type mismatch");
      static_assert(!std::is_same_v<std::invoke_result_t<F, const T&>, void>, "map function should not return void");
      return has_value() ? f(*value()) : nothing;
   }

   template<typename F>
   constexpr auto visit(F&& f) const noexcept -> void {
      static_assert(std::is_invocable_r_v<void, F, const T&>, "visit function type mismatch");
      if(has_value()) f(*value());
   }

   auto swap(maybe& __opt) {
      parent::swap(__opt);
   }

   template<typename U, typename V>
   friend inline constexpr auto operator==( const maybe<U>& lhs, const maybe<V>& rhs ) noexcept -> bool {
      return static_cast<const std::optional<U>&>(rhs) == static_cast<const std::optional<V>&>(rhs);
   }

   template<typename U, typename V>
   friend inline constexpr auto operator!=( const maybe<U>& lhs, const maybe<V>& rhs ) noexcept -> bool {
      return static_cast<const std::optional<U>&>(rhs) != static_cast<const std::optional<V>&>(rhs);
   }

   template<typename U, typename V>
   friend inline constexpr auto operator>( const maybe<U>& lhs, const maybe<V>& rhs ) noexcept -> bool {
      return static_cast<const std::optional<U>&>(rhs) > static_cast<const std::optional<V>&>(rhs);
   }

   template<typename U, typename V>
   friend inline constexpr auto operator>=( const maybe<U>& lhs, const maybe<V>& rhs ) noexcept -> bool {
      return static_cast<const std::optional<U>&>(rhs) >= static_cast<const std::optional<V>&>(rhs);
   }

   template<typename U, typename V>
   friend inline constexpr auto operator<( const maybe<U>& lhs, const maybe<V>& rhs ) noexcept -> bool {
      return static_cast<const std::optional<U>&>(rhs) < static_cast<const std::optional<V>&>(rhs);
   }

   template<typename U, typename V>
   friend inline constexpr auto operator<=( const maybe<U>& lhs, const maybe<V>& rhs ) noexcept -> bool {
      return static_cast<const std::optional<U>&>(rhs) <= static_cast<const std::optional<V>&>(rhs);
   }
};


NANO_CAF_NS_END

#endif //NANO_CAF_MAYBE_H
