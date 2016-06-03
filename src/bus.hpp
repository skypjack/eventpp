#pragma once

#include <memory>
#include <utility>
#include <type_traits>
#include "signal.hpp"

namespace eventpp {

namespace details {

template<class E>
struct ETag { using type = E; };

template<int N, int M>
struct Choice: public Choice<N+1, M> {
    static_assert(N<M, "!");
};

template<int N>
struct Choice<N, N> { };

template<typename... T>
using VoidType = void;

template<typename, typename, typename = VoidType<>>
struct HasReceiveMember: std::false_type { };

template<typename C, typename E>
struct HasReceiveMember
<
    C, E,
    VoidType<decltype(std::declval<C>().receive(std::declval<E>()))>
>: std::true_type { };

template<typename C, typename E>
constexpr bool HasReceiveMemberValue = HasReceiveMember<C, E>::value;

}

template<class D>
struct Receiver: public std::enable_shared_from_this<D>
{ };

template<int S, class... T>
class BusBase;

template<int S, class E, class... O>
class BusBase<S, E, O...>: public BusBase<S, O...> {
    using Base = BusBase<S, O...>;

protected:
    using Base::get;
    using Base::reg;
    using Base::unreg;

    Signal<E>& get(details::ETag<E>) {
        return signal;
    }

    template<class C>
    std::enable_if_t<details::HasReceiveMemberValue<C, E>>
    reg(details::Choice<S-(sizeof...(O)+1), S>, std::weak_ptr<C> &ptr) {
        signal.template add<C, &C::receive>(ptr);
        Base::reg(details::Choice<S-sizeof...(O), S>{}, ptr);
    }

    template<class C>
    std::enable_if_t<details::HasReceiveMemberValue<C, E>>
    unreg(details::Choice<S-(sizeof...(O)+1), S>, std::weak_ptr<C> &ptr) {
        signal.template remove<C, &C::receive>(ptr);
        Base::unreg(details::Choice<S-sizeof...(O), S>{}, ptr);
    }

    std::size_t size() const noexcept {
        return signal.size() + Base::size();
    }

private:
    Signal<E> signal;
};

template<int S>
class BusBase<S> {
protected:
    virtual ~BusBase() { }
    void get();
    void reg(details::Choice<S, S>, std::weak_ptr<void>) { }
    void unreg(details::Choice<S, S>, std::weak_ptr<void>) { }
    std::size_t size() const noexcept { return 0; }
};

template<class... T>
class Bus: public BusBase<sizeof...(T), T...> {
    using Base = BusBase<sizeof...(T), T...>;

public:
    using Base::size;

    template<class C>
    void reg(std::shared_ptr<C> &ptr) {
        auto wptr = static_cast<std::weak_ptr<C>>(ptr);
        Base::reg(details::Choice<0, sizeof...(T)>{}, wptr);
    }

    template<class C>
    void unreg(std::shared_ptr<C> &ptr) {
        auto wptr = static_cast<std::weak_ptr<C>>(ptr);
        Base::unreg(details::Choice<0, sizeof...(T)>{}, wptr);
    }

    template<class E, void(*F)(const E &)>
    void add() {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        signal.template add<F>();
    }

    template<class E, class C, void(C::*M)(const E &) = &C::receive>
    void add(std::shared_ptr<C> &ptr) {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        auto wptr = static_cast<std::weak_ptr<C>>(ptr);
        signal.template add<C, M>(wptr);
    }

    template<class E, void(*F)(const E &)>
    void remove() {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        signal.template remove<F>();
    }

    template<class E, class C, void(C::*M)(const E &) = &C::receive>
    void remove(std::shared_ptr<C> &ptr) {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        auto wptr = static_cast<std::weak_ptr<C>>(ptr);
        signal.template remove<C, M>(wptr);
    }

    template<class E, class... A>
    void publish(A&&... args) {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        E e(std::forward<A>(args)...);
        signal.publish(e);
    }
};

}
