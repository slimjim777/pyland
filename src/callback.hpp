#ifndef CALLBACK_H
#define CALLBACK_H

#include <functional>
#include <memory>
#include <set>


template <typename Ret, typename... Args>
class CallbackRegistry;



///
/// Allows registering a function with a callback registry.
///
template <typename Ret, typename... Args>
class Callback {
private:
    ///
    /// UIDs are assigned through the use of abundant integers.
    ///
    static uint64_t uid_count;

    ///
    /// UID of the callback
    ///
    uint64_t uid;
protected:
    friend class CallbackRegistry<Ret, Args...>;

    ///
    /// The callback function.
    ///
    std::function<Ret(Args...)> func;
    ///
    /// A set of all registries the callback is registered to.
    ///
    std::shared_ptr<std::set<CallbackRegistry<Ret, Args...>*>> registries;

    ///
    /// Notify about addition to a registry.
    ///
    void add_registry(CallbackRegistry<Ret, Args...>* registry) const;

    ///
    /// Notify about removal from a registry.
    ///
    void remove_registry(CallbackRegistry<Ret, Args...>* registry) const;
public:
    ///
    /// Construct a callback using the specified function.
    ///
    Callback(const std::function<Ret(Args...)>& func);
    ///
    /// Construct a callback using the specified function.
    ///
    Callback(const Ret (&func)(Args...));
    ~Callback();

    ///
    /// Completely remove the callback from all associated registries.
    ///
    void unregister_everywhere() const;

    ///
    /// Calls the callback function.
    ///
    Ret operator()(Args... args) const;

    bool operator<(const Callback<Ret, Args...> other) const;
};



#include "callback.hxx"

#endif
