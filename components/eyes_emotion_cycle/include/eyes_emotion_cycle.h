#ifndef EYES_EMOTION_CYCLE_H
#define EYES_EMOTION_CYCLE_H

#include "eyes/player.h"

namespace eyes_cycle {

// Cycles through all defined emotions forever, playing each clip on the provided player.
// This function does not return unless a clip fails.
bool run_forever(const eyes::EyesClipPlayer &player, const eyes::ClipOptions &options);

} // namespace eyes_cycle

#endif

