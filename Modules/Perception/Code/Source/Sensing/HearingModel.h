#pragma once

#include <GOAT_Perception/PerceptionProfileAsset.h>

namespace GOAT_Perception
{
    //! What one sound does to one listener.
    struct HeardSound final
    {
        bool m_heard = false;
        //! Awareness the sound adds when heard.
        float m_fill = 0.0f;
    };

    //! Whether a listener hears a sound and how much it adds; a blocked path stands in for a wall and costs the wall cut once.
    HeardSound EvaluateSound(const PerceptionProfileAsset& profile, float distance, float loudness, bool blocked);
} // namespace GOAT_Perception
