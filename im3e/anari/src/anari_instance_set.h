#pragma once

#include "anari_device.h"

#include <anari/anari.h>

#include <memory>
#include <mutex>
#include <unordered_set>

namespace im3e {

class AnariInstanceSet
{
public:
    AnariInstanceSet(std::shared_ptr<AnariDevice> pAnDevice, ANARIWorld anWorld);

    void insert(ANARIInstance anInstance);
    void remove(ANARIInstance anInstance);

    /// @brief Update the world if any changes in instances.
    /// @return True if the world was changed (and needs committing), false otherwise.
    auto updateWorld() -> bool;

private:
    std::shared_ptr<AnariDevice> m_pAnDevice;
    ANARIWorld m_anWorld;

    mutable std::mutex m_mutex;
    std::unordered_set<ANARIInstance> m_anInstances;
    bool m_changed{};
};

}  // namespace im3e