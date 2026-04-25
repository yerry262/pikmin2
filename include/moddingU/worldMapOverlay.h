#ifndef _MODDINGU_WORLDMAPOVERLAY_H
#define _MODDINGU_WORLDMAPOVERLAY_H

#include "types.h"

struct Graphics;
struct J2DPane;

namespace kh {
namespace Screen {
	struct WorldMap;
}
}

namespace moddingU {
namespace worldMapOverlay {

// Screen position of the Jerry dot (top-right, past Wistful Wild).
// Tunable here; used by the phantom pane and the draw hook.
static const f32 kJerryDotX = 520.0f;
static const f32 kJerryDotY = 140.0f;

// If the WorldMap's mCurrentCourseIndex is 4, overwrite the given pane pointers
// to point at a phantom pane located at (kJerryDotX, kJerryDotY).
// Called from WorldMap::update right after the native tagsWait/tagsPoint search.
void maybeReroutePanes(kh::Screen::WorldMap* wm, J2DPane*& cWaitPane, J2DPane*& cPointPane);

// Extend getTarget() navigation so that pressing right on Wistful Wild (course 3)
// moves cursor to Jerry (course 4), and pressing left on Jerry returns to Wistful.
// Returns the new course index if handled (caller should skip the native switch),
// or -2 if native handling should proceed.
int maybeHandleNavigation(kh::Screen::WorldMap* wm);

// Draw the Jerry dot + label + selection ring. Called from WorldMap::draw4th
// after gGame2DMgr->draw(gfx).
void draw(kh::Screen::WorldMap* wm, Graphics& gfx);

} // namespace worldMapOverlay
} // namespace moddingU

#endif
