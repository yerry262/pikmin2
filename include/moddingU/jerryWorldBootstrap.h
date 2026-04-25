#ifndef _MODDINGU_JERRYWORLDBOOTSTRAP_H
#define _MODDINGU_JERRYWORLDBOOTSTRAP_H

#include "types.h"

namespace moddingU {
namespace jerryWorldBootstrap {

// Called once per frame from BaseGameSection::doUpdate, right next to the
// existing ModMenu tick. Detects entry/exit of course 4 (Jerry's World) and
// spawns the custom content set on first frame after entry.
void onUpdate();

} // namespace jerryWorldBootstrap
} // namespace moddingU

#endif
