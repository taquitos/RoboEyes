#ifndef EYES_TRANSITION_H
#define EYES_TRANSITION_H

#include "eyes/pose.h"

namespace eyes {

// Blends two poses. `alpha=0` yields `from`, `alpha=1` yields `to`.
EyesPose blend_poses(const EyesPose &from, const EyesPose &to, float alpha);

} // namespace eyes

#endif

