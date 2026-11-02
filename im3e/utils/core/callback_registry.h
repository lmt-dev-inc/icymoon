#pragma once

#include <im3e/utils/core/throw_utils.h>

#include <functional>
#include <memory>
#include <vector>

namespace im3e {

class CallbackRegistration
{
public:
    CallbackRegistration() = default;
    CallbackRegistration(const CallbackRegistration&) = delete;
    CallbackRegistration& operator=(const CallbackRegistration&) = delete;
    CallbackRegistration(CallbackRegistration&&) = default;
    CallbackRegistration& operator=(CallbackRegistration&&) = default;

    explicit CallbackRegistration(std::shared_ptr<void> token)
      : m_token(std::move(token))
    {
    }

    void reset() { m_token.reset(); }

private:
    std::shared_ptr<void> m_token;
};

template <typename... Args>
class CallbackRegistry
{
public:
    using CallbackT = std::function<void(Args...)>;

    [[nodiscard]] auto registerCallback(CallbackT callback) -> CallbackRegistration
    {
        throwIfArgNull(callback, "Cannot register null callback");

        _removeExpired();

        auto callbackPtr = std::make_shared<CallbackT>(std::move(callback));
        auto registration = CallbackRegistration{callbackPtr};
        m_callbacks.emplace_back(callbackPtr);
        return registration;
    }

    void invokeAll(Args... args)
    {
        // Iterate a copy of the callbacks in case any callback trigger a registration:
        const auto callbacks = m_callbacks;
        for (const auto& callbackWeakPtr : callbacks)
        {
            if (auto callback = callbackWeakPtr.lock())
            {
                (*callback)(args...);
            }
        }
        _removeExpired();
    }

private:
    void _removeExpired()
    {
        std::erase_if(m_callbacks, [](const auto& callback) { return callback.expired(); });
    }

    std::vector<std::weak_ptr<CallbackT>> m_callbacks;
};

}  // namespace im3e