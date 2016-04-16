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
struct Choice: public Choice<N+1, M> { };

template<int N>
struct Choice<N, N> { };

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

    Signal<E>& get(details::ETag<E>) {
        return signal;
    }

    template<class C>
    auto reg(details::Choice<S-(sizeof...(O)+1), S>, std::weak_ptr<C> ptr)
    -> decltype(std::declval<C>().receive(std::declval<E>())) {
        signal.template add<C, &C::receive>(ptr);
        Base::reg(details::Choice<S-sizeof...(O), S>{}, ptr);
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
    std::size_t size() const noexcept { return 0; }
};

template<class... T>
class Bus: public BusBase<sizeof...(T), T...> {
    using Base = BusBase<sizeof...(T), T...>;

public:
    using Base::size;

    template<class E, void(*F)(const E &)>
    void add() {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        signal.template add<F>();
    }

    template<class C, template<typename> class P>
    typename std::enable_if<std::is_convertible<P<C>, std::weak_ptr<C>>::value, void>::type
    reg(P<C> &ptr) {
        auto wptr = static_cast<std::weak_ptr<C>>(ptr);
        Base::reg(details::Choice<0, sizeof...(T)>{}, wptr);
    }

    template<class E, class C, void(C::*M)(const E &) = &C::receive, template<typename> class P>
    typename std::enable_if<std::is_convertible<P<C>, std::weak_ptr<C>>::value, void>::type
    add(P<C> &ptr) {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        auto wptr = static_cast<std::weak_ptr<C>>(ptr);
        signal.template add<C, M>(wptr);
    }

    template<class E, void(*F)(const E &)>
    void remove() {
        Signal<E> &signal = Base::get(details::ETag<E>{});
        signal.template remove<F>();
    }

    template<class E, class C, void(C::*M)(const E &) = &C::receive, template<typename> class P>
    typename std::enable_if<std::is_convertible<P<C>, std::weak_ptr<C>>::value, void>::type
    remove(P<C> &ptr) {
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
