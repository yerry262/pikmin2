#include "moddingU/worldMapOverlay.h"

#include "Controller.h"
#include "Graphics.h"
#include "Dolphin/os.h"

#include "Game/gamePlayData.h"
#include "kh/khWorldMap.h"

#include "JSystem/J2D/J2DPane.h"
#include "JSystem/J2D/J2DPrint.h"
#include "JSystem/JFramework/JFWSystem.h"
#include "JSystem/JUtility/TColor.h"

namespace moddingU {
namespace worldMapOverlay {

// Phantom pane: a J2DPane whose global matrix is pinned to the Jerry dot's
// screen position. The native WorldMap state machine treats it exactly like
// any BLO-backed pane, so rocketMove() / postureControl() / getPaneCenterXY
// all Just Work for course 4 without modifying the native [4] arrays.
struct PhantomPane : public J2DPane {
	PhantomPane()
	    : J2DPane()
	{
	}

	void pinTo(f32 x, f32 y)
	{
		// getGlbVtx computes (x*m00+y*m01+m03, x*m10+y*m11+m13). With mBounds
		// zeroed by the default ctor, x and y inputs are both 0, so only the
		// translation columns matter. Zero everything then set identity scale
		// + translation to (x,y).
		for (int r = 0; r < 3; ++r) {
			for (int c = 0; c < 4; ++c) {
				mGlobalMtx[r][c] = 0.0f;
			}
		}
		mGlobalMtx[0][0] = 1.0f;
		mGlobalMtx[1][1] = 1.0f;
		mGlobalMtx[2][2] = 1.0f;
		mGlobalMtx[0][3] = x;
		mGlobalMtx[1][3] = y;
	}
};

// The phantom lives in .bss — allocated once, re-pinned every update.
static PhantomPane* sPhantomPane = nullptr;

static PhantomPane* ensurePhantomPane()
{
	if (!sPhantomPane) {
		sPhantomPane = new PhantomPane();
	}
	sPhantomPane->pinTo(kJerryDotX, kJerryDotY);
	return sPhantomPane;
}

void maybeReroutePanes(kh::Screen::WorldMap* wm, J2DPane*& cWaitPane, J2DPane*& cPointPane)
{
	if (!wm) return;
	if (wm->mCurrentCourseIndex != 4) return;

	J2DPane* phantom = ensurePhantomPane();
	cWaitPane        = phantom;
	cPointPane       = phantom;
}

int maybeHandleNavigation(kh::Screen::WorldMap* wm)
{
	if (!wm || !wm->mInitArg.mController) return -2;
	if (!Game::playData || !Game::playData->courseOpen(4)) return -2;

	Controller* pad = wm->mInitArg.mController;

	// From Wistful Wild, right goes to Jerry.
	if (wm->mCurrentCourseIndex == kh::Screen::WorldMap::COURSE_Last) {
		if (pad->isButtonHeld(Controller::PRESS_RIGHT)) {
			return 4;
		}
	}

	// From Jerry, left returns to Wistful Wild.
	if (wm->mCurrentCourseIndex == 4) {
		if (pad->isButtonHeld(Controller::PRESS_LEFT)) {
			return kh::Screen::WorldMap::COURSE_Last;
		}
		// Otherwise Jerry is a leaf: no other navigation keys apply.
		return -1; // COURSE_NULL — stay put
	}

	return -2; // Not handled; fall through to native logic.
}

void draw(kh::Screen::WorldMap* wm, Graphics& gfx)
{
	if (!wm) return;
	if (!Game::playData || !Game::playData->courseOpen(4)) return;
	if (!JFWSystem::systemFont) return;

	gfx.mOrthoGraph.setPort();

	// Dot: 14x14 cyan/yellow filled box centered on the target coord.
	const f32 dotHalf = 7.0f;
	gfx.mOrthoGraph.setColor(JUtility::TColor(255, 210, 90, 255));
	gfx.mOrthoGraph.fillBox(JGeometry::TBox2f(
	    kJerryDotX - dotHalf, kJerryDotY - dotHalf,
	    kJerryDotX + dotHalf, kJerryDotY + dotHalf));

	// Selection ring when Jerry is the current target.
	if (wm->mCurrentCourseIndex == 4) {
		const f32 ringHalf = 12.0f;
		gfx.mOrthoGraph.setColor(JUtility::TColor(255, 255, 255, 200));
		gfx.mOrthoGraph.fillBox(JGeometry::TBox2f(
		    kJerryDotX - ringHalf, kJerryDotY - ringHalf,
		    kJerryDotX + ringHalf, kJerryDotY - ringHalf + 2.0f));
		gfx.mOrthoGraph.fillBox(JGeometry::TBox2f(
		    kJerryDotX - ringHalf, kJerryDotY + ringHalf - 2.0f,
		    kJerryDotX + ringHalf, kJerryDotY + ringHalf));
		gfx.mOrthoGraph.fillBox(JGeometry::TBox2f(
		    kJerryDotX - ringHalf, kJerryDotY - ringHalf,
		    kJerryDotX - ringHalf + 2.0f, kJerryDotY + ringHalf));
		gfx.mOrthoGraph.fillBox(JGeometry::TBox2f(
		    kJerryDotX + ringHalf - 2.0f, kJerryDotY - ringHalf,
		    kJerryDotX + ringHalf, kJerryDotY + ringHalf));
	}

	// Label below the dot.
	J2DPrint print(JFWSystem::systemFont, 0.0f);
	print.initiate();
	print.setCharColor(JUtility::TColor(255, 210, 90, 255));
	print.setGradColor(JUtility::TColor(255, 255, 200, 255));
	print.print(kJerryDotX - 40.0f, kJerryDotY + 18.0f, "Jerry's World");
}

} // namespace worldMapOverlay
} // namespace moddingU
