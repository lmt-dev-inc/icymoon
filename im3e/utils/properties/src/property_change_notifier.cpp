#include "property_change_notifier.h"

#include <im3e/utils/core/throw_utils.h>

using namespace im3e;

void PropertyChangeNotifier::registerOnChange(std::weak_ptr<std::function<void()>> pOnChangeCallback)
{
    throwIfArgNull(pOnChangeCallback.lock(), "Property cannot register null callback");
    throwIfArgNull(*pOnChangeCallback.lock(), "Property cannot register empty function");
    m_pOnChangeCallbacks.emplace_back(std::move(pOnChangeCallback));
}

void PropertyChangeNotifier::notifyChanged()
{
    auto it = m_pOnChangeCallbacks.begin();
    while (it != m_pOnChangeCallbacks.end())
    {
        if (auto pFct = it->lock(); pFct && *pFct)
        {
            (*pFct)();
            it++;
        }
        else
        {
            it = m_pOnChangeCallbacks.erase(it);
        }
    }
}
