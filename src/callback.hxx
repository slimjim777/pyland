// File included by callback.hpp

#include <stdint.h>
#include <functional>
#include <memory>
#include <set>

template <typename Ret, typename... Args>
class CallbackRegistry;


template <typename Ret, typename... Args>
uint64_t Callback<Ret, Args...>::uid_count = 0;


template <typename Ret, typename... Args>
Callback<Ret, Args...>::Callback(const std::function<Ret(Args...)>& func):
    func(func),
    registries(std::make_shared<std::set<CallbackRegistry<Ret, Args...>*>>()) {
        uid = uid_count;
        uid_count++;
}

template <typename Ret, typename... Args>
Callback<Ret, Args...>::Callback(const Ret (&func)(Args...)):
    Callback(std::function<Ret(Args...)>(func)) {
}

template <typename Ret, typename... Args>
Callback<Ret, Args...>::~Callback() {
    // Deleting registry entries is not our job! It is handled by
    // shared_ptr.
}


template <typename Ret, typename... Args>
bool Callback<Ret, Args...>::operator<(const Callback<Ret, Args...> other) const {
    return this->uid < other.uid;
}

template <typename Ret, typename... Args>
void Callback<Ret, Args...>::add_registry(CallbackRegistry<Ret, Args...>* registry) const {
    registries->insert(registry);
}


template <typename Ret, typename... Args>
void Callback<Ret, Args...>::remove_registry(CallbackRegistry<Ret, Args...>* registry) const {
    registries->erase(registry);
}


template <typename Ret, typename... Args>
void Callback<Ret, Args...>::unregister_everywhere() const {
    std::set<CallbackRegistry<Ret, Args...>*> registries_safe = *registries;
    for (CallbackRegistry<Ret, Args...>* registry : registries_safe) {
        registry->unregister_callback(this);
    }
    registries->clear();
}


template <typename Ret, typename... Args>
Ret Callback<Ret, Args...>::operator()(Args... args) const {
    return func(args...);
}
