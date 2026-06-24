#include "moddingU/modMenu.h"

#include "Controller.h"
#include "Graphics.h"
#include "Dolphin/os.h"
#include "Dolphin/pad.h"

#include "Game/PikiMgr.h"
#include "Game/PikiParms.h"
#include "Game/Piki.h"
#include "Game/Navi.h"
#include "Game/NaviParms.h"
#include "Game/EnemyBase.h"
#include "Game/generalEnemyMgr.h"
#include "Game/EnemyMgrBase.h"
#include "Game/enemyInfo.h"
#include "Game/MapMgr.h"
#include "Game/GameSystem.h"
#include "Game/gamePlayData.h"
#include "Game/MoviePlayer.h"
#include "Game/gameStages.h"
#include "Game/gameStat.h"
#include "Dolphin/rand.h"
#include "PikiAI.h"

#include "JSystem/J2D/J2DPrint.h"
#include "JSystem/JFramework/JFWSystem.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "P2JME/P2JME.h"

namespace moddingU {

// Backing storage lives in .bss so we don't depend on whatever JKRHeap is
// current mid-frame (the game freely rewinds section-level heaps and the
// menu object would get its fields zeroed out from under it).
static ModMenu sModMenuStorage;
ModMenu* gModMenu = &sModMenuStorage;
u32      gFieldCap = 100;
u32      gPurpleLiftScale = 10;
bool     gAutoSkipCutscenes  = false;
bool     gCutscenesSkippable = false;

// Boss-spawn cycler: each press of the "Spawn boss" action spawns the entry
// at sBossCycleIdx, then advances the index (wrapping). The value column
// shows the *next* boss to be spawned so the user can preview before pressing.
struct BossEntry {
	int         mTypeID;
	const char* mShortName;
};
// "+5 pikmin" cycler: L/R picks color (or Random = 5 of each), A spawns at active captain.
// Menu order: 0=Random, 1=Blue, 2=White, 3=Purple, 4=Yellow, 5=Red.
static u32 sSpawnPikiKindIdx = 0;
static const char* sSpawnPikiKindNames[] = { "Random", "Blue", "White", "Purple", "Yellow", "Red" };
static const int sSpawnPikiKindCount = sizeof(sSpawnPikiKindNames) / sizeof(sSpawnPikiKindNames[0]);

static const char* displaySpawnPikiKind()
{
	if (sSpawnPikiKindIdx >= (u32)sSpawnPikiKindCount) return "?";
	return sSpawnPikiKindNames[sSpawnPikiKindIdx];
}

static const BossEntry sBossCycle[] = {
	{ Game::EnemyTypeID::EnemyID_BlackMan,    "Wraith"  }, // Waterwraith
	{ Game::EnemyTypeID::EnemyID_BigTreasure, "Titan"   }, // Titan Dweevil
	{ Game::EnemyTypeID::EnemyID_KingChappy,  "Empblax" }, // Emperor Bulblax
	{ Game::EnemyTypeID::EnemyID_SnakeWhole,  "PSnagrt" }, // Pileated Snagret
	{ Game::EnemyTypeID::EnemyID_SnakeCrow,   "BSnagrt" }, // Burrowing Snagret
	{ Game::EnemyTypeID::EnemyID_Damagumo,    "BLLegs"  }, // Beady Long Legs
	{ Game::EnemyTypeID::EnemyID_UmiMushi,    "RBloyst" }, // Ranging Bloyster
	{ Game::EnemyTypeID::EnemyID_Houdai,      "ManLegs" }, // Man-at-Legs
	{ Game::EnemyTypeID::EnemyID_OoPanModoki, "GBread"  }, // Giant Breadbug
	{ Game::EnemyTypeID::EnemyID_Queen,       "Empress" }, // Empress Bulblax
	{ Game::EnemyTypeID::EnemyID_BigFoot,     "RLLegs"  }, // Raging Long Legs
	{ Game::EnemyTypeID::EnemyID_DangoMushi,  "Crawbst" }, // Segmented Crawbster
};
static const int sBossCycleCount = sizeof(sBossCycle) / sizeof(sBossCycle[0]);
static int       sBossCycleIdx   = 0;

static const char* displayBossName()
{
	return sBossCycle[sBossCycleIdx].mShortName;
}

static void action_killAllEnemies();
static void action_giveAllPikiFlower();
static void action_killAllPikmin();
static void action_allPikiToOnyon();
static void action_onyonToCaptain();
static void action_spawnFiveEach();
static void action_addPokos();
static void action_toggleNoclip();
static void action_toggleFreezeDay();
static void action_refillSprays();
static void action_restoreDefaults();
static void action_toggleAutoSkipCutscenes();
static void action_toggleCutscenesSkippable();
static void action_cycleSpawnBoss();
static void action_unlockAllOnyons();
static void action_discoverAllPikmin();
static void action_markAllTreasures();

// Returns how many real seconds have elapsed since the drop (day start).
// Uses the same ratio TimeMgr::setTime uses: hours / 24 * dayLengthSeconds.
static f32 getElapsedSeconds()
{
	if (!Game::gameSystem || !Game::gameSystem->mTimeMgr) return 0.0f;
	Game::TimeMgr* tm = Game::gameSystem->mTimeMgr;
	f32 secsPerHour = tm->mParms.mParms.mDayLengthSeconds.mValue / 24.0f;
	f32 elapsed = (tm->mCurrentTimeOfDay - tm->mParms.mParms.mDayStartTime.mValue) * secsPerHour;
	return elapsed < 0.0f ? 0.0f : elapsed;
}

static void applyTimeOfDay(f32 elapsedSeconds)
{
	if (!Game::gameSystem || !Game::gameSystem->mTimeMgr) return;
	Game::TimeMgr* tm = Game::gameSystem->mTimeMgr;
	f32 secsPerHour = tm->mParms.mParms.mDayLengthSeconds.mValue / 24.0f;
	f32 timeOfDay = tm->mParms.mParms.mDayStartTime.mValue + elapsedSeconds / secsPerHour;
	tm->setTime(timeOfDay);
	OSReport("[MOD] time set: %ds -> %.2fh\n", (int)elapsedSeconds, timeOfDay);
}

static int displayPokoCount()
{
	return Game::playData ? Game::playData->getPokoCount() : 0;
}

void ensureCreated()
{
	// No-op; gModMenu points at the .bss-resident singleton. Kept for
	// symmetry with the old hook call-site.
}

void teardown()
{
	// Nothing to free; storage is static.
}

void onBaseGameUpdate(Controller* p1)
{
	ensureCreated();
	if (gModMenu) {
		gModMenu->update(p1);
	}

	// Cutscene helpers — re-applied each frame so they survive across new
	// cutscene starts (MoviePlayer::play() resets MVP_DoSkip to 0).
	if (Game::moviePlayer && Game::moviePlayer->isFlag(Game::MVP_IsActive)) {
		if (gCutscenesSkippable) {
			Game::moviePlayer->setFlag(Game::MVP_DoSkip);
		}
		if (gAutoSkipCutscenes && Game::moviePlayer->mDemoState == Game::DEMOSTATE_Playing) {
			Game::moviePlayer->skip();
		}
	}

	// Captain HP sync. The Captain HP slider writes mMaxHealth (the parm), but
	// Navi::mHealth (current HP) is set once at init from the parm and never
	// re-syncs. That makes the gauge fraction (mHealth / mMaxHealth) shrink as
	// the user raises the cap — looks "inverted." Detect when mMaxHealth has
	// changed and resync each Navi's mHealth + the playData snapshot so the
	// captain actually has the new HP and the bar shows it as full.
	if (Game::naviMgr && Game::naviMgr->mNaviParms) {
		static f32 sLastMaxHealth = -1.0f;
		f32 cur = Game::naviMgr->mNaviParms->mNaviParms.mMaxHealth.mValue;
		if (cur != sLastMaxHealth) {
			sLastMaxHealth = cur;
			for (int i = 0; i < Game::naviMgr->getMax(); ++i) {
				if (!Game::naviMgr->getFlag(i)) {
					Game::Navi* navi = Game::naviMgr->getAt(i);
					if (navi) navi->mHealth = cur;
				}
			}
			if (Game::playData) {
				Game::playData->mNaviLifeMax[NAVIID_Olimar] = cur;
				Game::playData->mNaviLifeMax[NAVIID_Louie]  = cur;
			}
		}
	}
}

void onBaseGameDraw(Graphics& gfx)
{
	if (gModMenu) {
		gModMenu->draw(gfx);
	}
}

ModMenu::ModMenu()
    : mOpen(false)
    , mInitialized(false)
    , mCursor(0)
    , mSliderCount(0)
    , mHoldFrames(0)
    , mVerticalHoldFrames(0)
{
	for (int i = 0; i < kMaxSliders; ++i) {
		mSliders[i].mLabel        = "";
		mSliders[i].mKind         = kModSlider_Float;
		mSliders[i].mTarget       = nullptr;
		mSliders[i].mMin          = 0.0f;
		mSliders[i].mMax          = 0.0f;
		mSliders[i].mStep         = 0.0f;
		mSliders[i].mAction       = nullptr;
		mSliders[i].mOriginal     = 0.0f;
		mSliders[i].mPendingValue = 0.0f;
		mSliders[i].mEnabled      = false;
		mSliders[i].mDirty        = false;
		mSliders[i].mGreenLabel        = false;
		mSliders[i].mNoValue           = false;
		mSliders[i].mOrangeIfMenuDirty = false;
		mSliders[i].mGetDisplayInt     = nullptr;
		mSliders[i].mGetDisplayStr     = nullptr;
	}
}

void ModMenu::toggle()
{
	if (!mInitialized) {
		buildSliders();
		snapshotOriginals();
		mInitialized = true;
	}
	mOpen = !mOpen;
	OSReport("[MOD] menu %s\n", mOpen ? "OPEN" : "closed");
}

bool ModMenu::isMenuDirty() const
{
	for (int i = 0; i < mSliderCount; ++i) {
		const ModSlider& s = mSliders[i];
		if (s.mOrangeIfMenuDirty) continue; // don't let the restore button count itself
		if ((s.mKind == kModSlider_Float || s.mKind == kModSlider_Int) && s.mDirty)
			return true;
		if (s.mKind == kModSlider_Action && s.mEnabled)
			return true;
	}
	return false;
}

void ModMenu::buildSliders()
{
	mSliderCount = 0;
	if (!Game::pikiMgr || !Game::pikiMgr->mParms) {
		OSReport("[MOD] pikiMgr/parms not ready, cannot build sliders\n");
		return;
	}

	Game::PikiParms* parms = Game::pikiMgr->mParms;

	// Float sliders — point at the Parm<f32>::mValue of each tunable.
#define ADD_PARM(label_, parmRef_, min_, max_, step_)                                 \
	do {                                                                              \
		ModSlider& s = mSliders[mSliderCount++];                                      \
		s.mLabel  = label_;                                                           \
		s.mKind   = kModSlider_Float;                                                 \
		s.mTarget = &((parmRef_).mValue);                                             \
		s.mMin    = (min_);                                                           \
		s.mMax    = (max_);                                                           \
		s.mStep   = (step_);                                                          \
	} while (0)

	ADD_PARM("Pikmin follow spd", parms->mPikiParms.mWalkSpeed,          10.0f,  500.0f, 10.0f);
	ADD_PARM("Pikmin carry spd",  parms->mPikiParms.mRunSpeed,           10.0f,  500.0f, 10.0f);
	ADD_PARM("Flower carry spd",  parms->mPikiParms.mFlowerRunSpeed,     10.0f,  500.0f, 10.0f);
	ADD_PARM("Pikmin HP",         parms->mPikiParms.mHealth,              1.0f, 1000.0f, 10.0f);

	// +5 pikmin cycler — D-pad L/R picks color (or Random), A spawns at active captain.
	{
		ModSlider& s = mSliders[mSliderCount++];
		s.mLabel         = "+5 pikmin";
		s.mKind          = kModSlider_Cycler;
		s.mTarget        = &sSpawnPikiKindIdx;
		s.mMin           = 0.0f;
		s.mMax           = (f32)(sSpawnPikiKindCount - 1);
		s.mStep          = 1.0f;
		s.mAction        = &action_spawnFiveEach;
		s.mGetDisplayStr = &displaySpawnPikiKind;
		s.mPendingValue  = (f32)sSpawnPikiKindIdx;
	}

	ADD_PARM("Blue attack",       parms->mPikiParms.mBlueAttackDamage,    1.0f, 1000.0f,  5.0f);
	ADD_PARM("Red attack",        parms->mPikiParms.mRedAttackDamage,     1.0f, 1000.0f,  5.0f);
	ADD_PARM("Yellow attack",     parms->mPikiParms.mYellowAttackDamage,  1.0f, 1000.0f,  5.0f);
	ADD_PARM("Purple attack",     parms->mPikiParms.mPurpleAttackDamage,  1.0f,   30.0f,  1.0f);
	ADD_PARM("White attack",      parms->mPikiParms.mWhiteAttackDamage,   1.0f,   30.0f,  1.0f);
	ADD_PARM("Enemy search rng",  parms->mPikiParms.mEnemySearchRange,    1.0f, 1000.0f, 10.0f);
	ADD_PARM("Pellet search rng", parms->mPikiParms.mPelletSearchRange,   1.0f, 1000.0f, 10.0f);
	ADD_PARM("Bomb throw max",    parms->mPikiParms.mMaxBombThrowDistance, 1.0f, 1000.0f, 10.0f);

	// Visual + purple-power bundle (v1)
	ADD_PARM("Purple pound dmg",  parms->mPikiParms.mPoundDamage,        0.0f, 1000.0f, 10.0f);
	ADD_PARM("Purple carry spd mult",  parms->mPikiParms.mPurpleCarryPower,   0.1f,    5.0f,  0.1f);

#undef ADD_PARM

	// Purple lift multiplier — feeds getColorTransportScale (int).
	{
		ModSlider& s = mSliders[mSliderCount++];
		s.mLabel  = "Purple lift mult";
		s.mKind   = kModSlider_Int;
		s.mTarget = &gPurpleLiftScale;
		s.mMin    = 1.0f;
		s.mMax    = 20.0f;
		s.mStep   = 1.0f;
	}

#define ADD_PARM(label_, parmRef_, min_, max_, step_)                                 \
	do {                                                                              \
		ModSlider& s = mSliders[mSliderCount++];                                      \
		s.mLabel  = label_;                                                           \
		s.mKind   = kModSlider_Float;                                                 \
		s.mTarget = &((parmRef_).mValue);                                             \
		s.mMin    = (min_);                                                           \
		s.mMax    = (max_);                                                           \
		s.mStep   = (step_);                                                          \
	} while (0)

	// Spicy spray bundle (v1)
	ADD_PARM("Spicy duration",    parms->mPikiParms.mDopeMaxDuration,    5.0f,  200.0f,  5.0f);
	ADD_PARM("Spicy run speed",   parms->mPikiParms.mDopeRunSpeed,      50.0f,  220.0f, 10.0f);

#undef ADD_PARM

	// Navi parms (v1 Bundle 2). Shared captain parms live on naviMgr->mNaviParms.
	if (Game::naviMgr && Game::naviMgr->mNaviParms) {
		Game::NaviParms::Parms& np = Game::naviMgr->mNaviParms->mNaviParms;
#define ADD_NAVI_PARM(label_, parmRef_, min_, max_, step_)                            \
		do {                                                                          \
			ModSlider& s = mSliders[mSliderCount++];                                  \
			s.mLabel  = label_;                                                       \
			s.mKind   = kModSlider_Float;                                             \
			s.mTarget = &((parmRef_).mValue);                                         \
			s.mMin    = (min_);                                                       \
			s.mMax    = (max_);                                                       \
			s.mStep   = (step_);                                                      \
		} while (0)

		ADD_NAVI_PARM("Whistle radius",   np.mPikiCallMaxRadius, 10.0f,  500.0f, 10.0f);
		ADD_NAVI_PARM("Throw dist max",   np.mThrowDistanceMax,  10.0f, 1000.0f, 10.0f);
		ADD_NAVI_PARM("Red/Blue throw ht", np.mThrowHeightMax,     10.0f,  200.0f,  5.0f);
		ADD_NAVI_PARM("Yellow throw ht",  np.mThrowHeightYellow,  10.0f,  200.0f,  5.0f);
		ADD_NAVI_PARM("Purple throw ht",  np.mThrowBlackHeight,   10.0f,  200.0f,  5.0f);
		ADD_NAVI_PARM("White throw ht",   np.mThrowWhiteHeight,   10.0f,  200.0f,  5.0f);
		// Captain HP — slider writes mMaxHealth; onBaseGameUpdate then resyncs each
		// Navi::mHealth and playData->mNaviLifeMax so the gauge fraction stays correct.
		ADD_NAVI_PARM("Captain HP",       np.mMaxHealth,         10.0f, 1000.0f, 10.0f);
		ADD_NAVI_PARM("Captain run spd",  np.mRunSpeed,          10.0f,  500.0f, 10.0f);

#undef ADD_NAVI_PARM
	}

	// Max on field (int) — caps onyon withdrawal UI and whistle-out count.
	{
		ModSlider& s = mSliders[mSliderCount++];
		s.mLabel  = "Max on field";
		s.mKind   = kModSlider_Int;
		s.mTarget = &gFieldCap;
		s.mMin    = 1.0f;
		s.mMax    = (f32)MAX_PIKI_COUNT;
		s.mStep   = 5.0f;
	}

	// Time of day — live-synced when not selected, scrubable when selected.
	// Max is computed from timeMgr parms; default day = 780s (12 game hours * 65 real s/hr).
	{
		f32 todMax = 780.0f;
		if (Game::gameSystem && Game::gameSystem->mTimeMgr) {
			Game::TimeMgr* tm = Game::gameSystem->mTimeMgr;
			f32 secsPerHour = tm->mParms.mParms.mDayLengthSeconds.mValue / 24.0f;
			todMax = tm->mGameDayLength * secsPerHour;
		}
		ModSlider& s = mSliders[mSliderCount++];
		s.mLabel  = "Time of day";
		s.mKind   = kModSlider_TimeOfDay;
		s.mTarget = nullptr;
		s.mMin    = 0.0f;
		s.mMax    = todMax;
		s.mStep   = 5.0f;
	}

	// Day speed multiplier — TimeMgr::mSpeedFactor. 1.0 = normal, <1 slow, >1 fast.
	if (Game::gameSystem && Game::gameSystem->mTimeMgr) {
		ModSlider& s = mSliders[mSliderCount++];
		s.mLabel  = "Day speed mult";
		s.mKind   = kModSlider_Float;
		s.mTarget = &Game::gameSystem->mTimeMgr->mSpeedFactor;
		s.mMin    = 0.1f;
		s.mMax    = 5.0f;
		s.mStep   = 0.1f;
	}

	// Actions — each registered the same way.
#define ADD_ACTION(label_, fn_)                                           \
	do {                                                                  \
		ModSlider& s = mSliders[mSliderCount++];                          \
		s.mLabel  = label_;                                               \
		s.mKind   = kModSlider_Action;                                    \
		s.mTarget = nullptr;                                              \
		s.mAction = (fn_);                                                \
	} while (0)

	ADD_ACTION("Kill all enemies",      &action_killAllEnemies);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("All piki -> flower",    &action_giveAllPikiFlower);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("Kill all pikmin",       &action_killAllPikmin);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("All piki -> onyon",     &action_allPikiToOnyon);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("Onyon -> Captain",      &action_onyonToCaptain);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("+$1,000 Pokos",         &action_addPokos);
	mSliders[mSliderCount - 1].mGreenLabel    = true;
	mSliders[mSliderCount - 1].mGetDisplayInt = &displayPokoCount;
	ADD_ACTION("Toggle noclip",         &action_toggleNoclip);
	ADD_ACTION("Freeze day",            &action_toggleFreezeDay);
	ADD_ACTION("Skip cutscenes",        &action_toggleAutoSkipCutscenes);
	ADD_ACTION("Cutscenes skippable",   &action_toggleCutscenesSkippable);
	ADD_ACTION("Refill sprays",         &action_refillSprays);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("Spawn boss",            &action_cycleSpawnBoss);
	mSliders[mSliderCount - 1].mGetDisplayStr = &displayBossName;
	ADD_ACTION("Unlock all onyons",     &action_unlockAllOnyons);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("Discover all piki",     &action_discoverAllPikmin);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("Mark all treasures",    &action_markAllTreasures);
	mSliders[mSliderCount - 1].mNoValue = true;
	ADD_ACTION("Restore defaults",      &action_restoreDefaults);
	mSliders[mSliderCount - 1].mNoValue           = true;
	mSliders[mSliderCount - 1].mOrangeIfMenuDirty = true;

#undef ADD_ACTION

	OSReport("[MOD] built %d sliders\n", mSliderCount);
}

void ModMenu::snapshotOriginals()
{
	for (int i = 0; i < mSliderCount; ++i) {
		ModSlider& s = mSliders[i];
		if (!s.mTarget) continue;
		if (s.mKind == kModSlider_Float) {
			s.mOriginal = *reinterpret_cast<f32*>(s.mTarget);
		} else if (s.mKind == kModSlider_Int) {
			s.mOriginal = (f32)(*reinterpret_cast<u32*>(s.mTarget));
		}
		s.mPendingValue = s.mOriginal;
	}
}

void ModMenu::restoreAll()
{
	for (int i = 0; i < mSliderCount; ++i) {
		ModSlider& s = mSliders[i];
		if (!s.mTarget) continue;
		if (s.mKind == kModSlider_Float) {
			*reinterpret_cast<f32*>(s.mTarget) = s.mOriginal;
		} else if (s.mKind == kModSlider_Int) {
			*reinterpret_cast<u32*>(s.mTarget) = (u32)s.mOriginal;
		}
		s.mPendingValue = s.mOriginal;
		s.mDirty        = false;
		s.mEnabled      = false;
	}
	OSReport("[MOD] restored %d parms\n", mSliderCount);
}

f32 ModMenu::readValue(const ModSlider& s) const
{
	if (!s.mTarget) return 0.0f;
	if (s.mKind == kModSlider_Float) {
		return *reinterpret_cast<const f32*>(s.mTarget);
	}
	if (s.mKind == kModSlider_Int) {
		return (f32)(*reinterpret_cast<const u32*>(s.mTarget));
	}
	return 0.0f;
}

void ModMenu::writeValue(ModSlider& s, f32 v)
{
	if (!s.mTarget) return;
	if (v < s.mMin) v = s.mMin;
	if (v > s.mMax) v = s.mMax;
	if (s.mKind == kModSlider_Float) {
		*reinterpret_cast<f32*>(s.mTarget) = v;
	} else if (s.mKind == kModSlider_Int) {
		*reinterpret_cast<u32*>(s.mTarget) = (u32)v;
	}
	s.mDirty = true;
}

void ModMenu::bumpCurrent(f32 direction)
{
	if (mSliderCount <= 0) return;
	ModSlider& s = mSliders[mCursor];
	if (s.mKind == kModSlider_Cycler) {
		int n = (int)(s.mMax - s.mMin) + 1;
		if (n <= 0) return;
		int idx = (int)s.mPendingValue + (int)direction - (int)s.mMin;
		idx %= n;
		if (idx < 0) idx += n;
		s.mPendingValue = (f32)(idx + (int)s.mMin);
		if (s.mTarget) *reinterpret_cast<u32*>(s.mTarget) = (u32)s.mPendingValue;
		return;
	}
	if (s.mKind != kModSlider_Float && s.mKind != kModSlider_Int && s.mKind != kModSlider_TimeOfDay) return;
	f32 v = s.mPendingValue + direction * s.mStep;
	if (v < s.mMin) v = s.mMin;
	if (v > s.mMax) v = s.mMax;
	s.mPendingValue = v;
}

void ModMenu::update(Controller* pad)
{
	if (!pad) return;

	// Defensive: if anything ever stomped our state, clamp before using it.
	if (mSliderCount < 0 || mSliderCount > kMaxSliders) mSliderCount = 0;
	if (mSliderCount > 0) {
		if (mCursor < 0)              mCursor = 0;
		if (mCursor >= mSliderCount)  mCursor = mSliderCount - 1;
	} else {
		mCursor = 0;
	}

	// Toggle: hold BOTH L and R, then press Z.
	// isButtonHeld tests bit-ANY (returns true if any bit in the mask is held),
	// so we test the full chord with an explicit == mask.
	const u32 chord = PAD_TRIGGER_L | PAD_TRIGGER_R;
	if ((pad->getButton() & chord) == chord && pad->isButtonDown(PAD_TRIGGER_Z)) {
		toggle();
	}

	if (!mOpen) return;

	// Live-sync TimeOfDay sliders every frame except when the cursor is on them.
	// When selected, the user controls the pending value manually via L/R.
	for (int i = 0; i < mSliderCount; ++i) {
		if (i == mCursor) continue;
		if (mSliders[i].mKind == kModSlider_TimeOfDay) {
			f32 live = getElapsedSeconds();
			if (live < mSliders[i].mMin) live = mSliders[i].mMin;
			if (live > mSliders[i].mMax) live = mSliders[i].mMax;
			mSliders[i].mPendingValue = live;
		}
	}

	if (mSliderCount == 0 && mInitialized) {
		// Parms weren't ready when first opened — try again.
		buildSliders();
		snapshotOriginals();
	}
	if (mSliderCount == 0) return;

	// Hold-to-scroll on Up/Down with acceleration. Fires once on press, pauses
	// briefly to debounce taps, then auto-repeats — slow at first, then medium,
	// then fast — so a long hold sweeps to the far end of the menu quickly.
	bool upHeld   = (pad->getButton() & PAD_BUTTON_UP)   != 0;
	bool downHeld = (pad->getButton() & PAD_BUTTON_DOWN) != 0;
	if (upHeld || downHeld) {
		int interval;
		if (mVerticalHoldFrames < 16)      interval = 0; // initial press window
		else if (mVerticalHoldFrames < 46) interval = 6; // slow
		else if (mVerticalHoldFrames < 91) interval = 3; // medium
		else                               interval = 1; // fast
		bool fire = (mVerticalHoldFrames == 0)
		         || (interval > 0 && (mVerticalHoldFrames % interval) == 0);
		if (fire) {
			if (upHeld) {
				mCursor = (mCursor - 1 + mSliderCount) % mSliderCount;
			} else {
				mCursor = (mCursor + 1) % mSliderCount;
			}
		}
		mVerticalHoldFrames++;
	} else {
		mVerticalHoldFrames = 0;
	}

	// Hold-to-repeat on left/right: fire immediately on press, then every 4 frames while held.
	bool leftHeld  = (pad->getButton() & PAD_BUTTON_LEFT)  != 0;
	bool rightHeld = (pad->getButton() & PAD_BUTTON_RIGHT) != 0;
	if (leftHeld || rightHeld) {
		if (mHoldFrames == 0 || (mHoldFrames > 10 && (mHoldFrames % 2) == 0)) {
			bumpCurrent(leftHeld ? -1.0f : 1.0f);
		}
		mHoldFrames++;
	} else {
		mHoldFrames = 0;
	}

	ModSlider& cur = mSliders[mCursor];
	if (pad->isButtonDown(PAD_BUTTON_A)) {
		if (cur.mKind == kModSlider_Action) {
			if (cur.mAction) cur.mAction();
			if (!cur.mNoValue) cur.mEnabled = !cur.mEnabled;
		} else if (cur.mKind == kModSlider_Cycler) {
			if (cur.mAction) cur.mAction();
		} else if ((cur.mKind == kModSlider_Float || cur.mKind == kModSlider_Int) && cur.mTarget) {
			writeValue(cur, cur.mPendingValue);
			cur.mDirty = (cur.mPendingValue != cur.mOriginal);
		} else if (cur.mKind == kModSlider_TimeOfDay) {
			applyTimeOfDay(cur.mPendingValue);
		}
	}
}

// Matches cBtnIconColor[11] in messageRendering.cpp: [id][0]=.a field, [id][1]=.b field.
// TEVREG0 = .b color, TEVREG1 = .a color (the game swaps them when assigning mImageColorA/B).
static const u8 sBtnIconColors[8][2][4] = {
    { {255,255,255,255}, {  0,166,  0,  0} }, // 0: A (green)
    { {255,255,255,255}, {255,  0,  0,  0} }, // 1: B (red)
    { {  0,  0,  0,255}, {255,255,  0,  0} }, // 2: C (yellow)
    { {  0,  0,  0,255}, {200,200,200,  0} }, // 3: X (gray)
    { {  0,  0,  0,255}, {200,200,200,  0} }, // 4: Y (gray)
    { {  0,  0,255,255}, {255,255,255,  0} }, // 5: Z (blue)
    { {  0,  0,  0,255}, {200,200,200,  0} }, // 6: L (gray)
    { {  0,  0,  0,255}, {200,200,200,  0} }, // 7: R (gray)
};

// Draw one GC controller button icon using the game's own textures + TEV pipeline.
// Replicates TRenderingProcessor::setImageGX (colorized path) + drawImage exactly.
// btnId: 0=A 1=B 5=Z 6=L 7=R
static void drawBtnIcon(Graphics& gfx, f32 x, f32 y, f32 size, int btnId)
{
    if (!gP2JMEMgr) return;
    JUTTexture* img = gP2JMEMgr->getImage(P2JME::ImageGroup::ID0, btnId);
    if (!img || (u32)btnId >= 8) return;

    // TEVREG0 = .b color, TEVREG1 = .a color
    GXColor reg0, reg1;
    reg0.r = sBtnIconColors[btnId][1][0]; reg0.g = sBtnIconColors[btnId][1][1];
    reg0.b = sBtnIconColors[btnId][1][2]; reg0.a = sBtnIconColors[btnId][1][3];
    reg1.r = sBtnIconColors[btnId][0][0]; reg1.g = sBtnIconColors[btnId][0][1];
    reg1.b = sBtnIconColors[btnId][0][2]; reg1.a = sBtnIconColors[btnId][0][3];

    gfx.mOrthoGraph.setPort();

    // setImageGX — colorized path (stage 0 blends two TEV registers through the texture,
    // stage 1 multiplies by vertex raster color for the top/bottom gradient).
    GXSetNumChans(1);
    GXSetNumTevStages(2);
    GXSetNumTexGens(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetTevColor(GX_TEVREG0, reg0);
    GXSetTevColor(GX_TEVREG1, reg1);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_C1, GX_CC_TEXC, GX_CC_ZERO);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_A1, GX_CA_TEXA, GX_CA_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_CPREV, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_APREV, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_POS_XYZ, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_POS_XYZ, GX_S8, 4);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    // drawImage quad: (left,bottom)→colorB, (left,top)→colorA, ...
    // colorA = top gradient (brighter), colorB = bottom gradient (slightly dimmed).
    f32 left = x, right = x + size, top = y, bottom = y + size, zero = 0.0f;
    img->load(GX_TEXMAP0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(left,  bottom, zero); GXColor4u8(205,205,205,255); GXTexCoord2u8( 0, 16);
    GXPosition3f32(left,  top,    zero); GXColor4u8(255,255,255,255); GXTexCoord2u8( 0,  0);
    GXPosition3f32(right, top,    zero); GXColor4u8(255,255,255,255); GXTexCoord2u8(16,  0);
    GXPosition3f32(right, bottom, zero); GXColor4u8(205,205,205,255); GXTexCoord2u8(16, 16);
}

// Solid-color quad helper. Caller must have already set up the flat-color TEV
// pipeline below (drawCheckerBox does this once before issuing many quads).
static void drawFlatQuad(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a)
{
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(x,     y + h, 0.0f); GXColor4u8(r, g, b, a);
    GXPosition3f32(x,     y,     0.0f); GXColor4u8(r, g, b, a);
    GXPosition3f32(x + w, y,     0.0f); GXColor4u8(r, g, b, a);
    GXPosition3f32(x + w, y + h, 0.0f); GXColor4u8(r, g, b, a);
}

// Green checker-pattern panel behind the mod menu. Evokes the in-game story
// message window (cave entrance, first treasure, first bestiary entry).
// Three layers: outer border, dark forest-green base, lighter checker tiles.
static void drawCheckerBox(Graphics& gfx, f32 x, f32 y, f32 w, f32 h)
{
    gfx.mOrthoGraph.setPort();

    // Vertex-color-only TEV pipeline (no texture, no lighting).
    GXSetNumChans(1);
    GXSetNumTevStages(1);
    GXSetNumTexGens(0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_POS_XYZ, GX_RGBA8, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    // Layer 1: outer border (bright forest green, like the in-game message frame).
    drawFlatQuad(x - 4.0f, y - 4.0f, w + 8.0f, h + 8.0f, 60, 170, 65, 240);
    // Layer 2: very dark teal-green base, near-opaque.
    drawFlatQuad(x, y, w, h, 14, 38, 26, 232);

    // Layer 3: scrolling scan-bands — three parallax layers moving in opposite
    // directions, like the file-select bottom bar's animated UV-scroll texture.
    // Each draw() call increments the phase, so this animates frame-by-frame.
    static u32 sBgPhase = 0;
    sBgPhase++;

    // Layer 3a: thick slow bands scrolling DOWN (cycle 24 px, advance 1 px / 4 frames).
    {
        const f32 cycle = 24.0f;
        f32 off = (f32)((sBgPhase / 4) % (u32)cycle);
        for (f32 yy = y - cycle + off; yy < y + h; yy += cycle) {
            f32 by = yy, bh = 4.0f;
            if (by < y) { bh -= (y - by); by = y; }
            if (by + bh > y + h) bh = (y + h) - by;
            if (bh > 0.0f) drawFlatQuad(x, by, w, bh, 80, 180, 90, 50);
        }
    }
    // Layer 3b: thin fast bands scrolling UP (cycle 14 px, advance 1 px / 2 frames).
    {
        const f32 cycle = 14.0f;
        f32 off = (f32)((sBgPhase / 2) % (u32)cycle);
        for (f32 yy = y + h - off; yy > y - cycle; yy -= cycle) {
            f32 by = yy, bh = 1.5f;
            if (by < y) { bh -= (y - by); by = y; }
            if (by + bh > y + h) bh = (y + h) - by;
            if (bh > 0.0f) drawFlatQuad(x, by, w, bh, 130, 220, 140, 35);
        }
    }
    // Layer 3c: bright sweeping highlight band — single wide bar that travels
    // top-to-bottom across the panel and wraps. Gives the "active screen" feel.
    {
        const f32 cycle = h + 60.0f;
        f32 off = (f32)((sBgPhase / 1) % (u32)cycle);
        f32 by = y - 30.0f + off;
        f32 bh = 18.0f;
        if (by < y) { bh -= (y - by); by = y; }
        if (by + bh > y + h) bh = (y + h) - by;
        if (bh > 0.0f) drawFlatQuad(x, by, w, bh, 100, 200, 120, 25);
    }
}

// Format a float without trailing zeros: 100.00->"100", 0.50->"0.5", 1.20->"1.2"
static const char* fmtFloat(char* buf, f32 v)
{
    sprintf(buf, "%.2f", v);
    char* dot = buf;
    while (*dot && *dot != '.') dot++;
    if (*dot == '.') {
        char* end = dot;
        while (*end) end++;
        end--;
        while (end > dot && *end == '0') { *end = '\0'; end--; }
        if (end == dot) *end = '\0';
    }
    return buf;
}

void ModMenu::draw(Graphics& gfx)
{
	if (!mOpen) return;
	if (!JFWSystem::systemFont) return;

	// Defensive: same clamp as update() in case update() hasn't run yet.
	if (mSliderCount < 0 || mSliderCount > kMaxSliders) mSliderCount = 0;
	if (mSliderCount > 0) {
		if (mCursor < 0)              mCursor = 0;
		if (mCursor >= mSliderCount)  mCursor = mSliderCount - 1;
	} else {
		mCursor = 0;
		return; // nothing to draw
	}

	J2DPrint print(JFWSystem::systemFont, 0.0f);
	print.initiate();
	print.setCharColor(JUtility::TColor(255, 255, 255, 255));
	print.setGradColor(JUtility::TColor(180, 220, 255, 255));

	const f32 x    = 120.0f;
	f32       y    = 32.0f; // a bit lower than before to give the big title headroom
	const f32 dy   = 18.0f;
	const f32 titleGap = dy * 1.1f; // tight: title-to-first-row spacing tuned by user feedback

	// Default font metrics + 2.25x size for the title line.
	const f32 baseFontW = JFWSystem::systemFont->getWidth();
	const f32 baseFontH = JFWSystem::systemFont->getHeight();
	const f32 bigFontW  = baseFontW * 2.25f;
	const f32 bigFontH  = baseFontH * 2.25f;

	// Measure widths so we can position the bottom controls row.
	const f32 iconSize   = 16.0f;
	const f32 sepW       = print.getWidth("+");
	const f32 closeW     = print.getWidth(" close, ");
	const f32 selectW    = print.getWidth(" select");
	const f32 rowMeasW   = print.getWidth("  Pikmin follow spd     999.00");

	// Bottom controls row: [L]+[R]+[Z] close, [A] select  (left-aligned at x).
	const f32 lx = x;
	const f32 rx = lx + iconSize + sepW;
	const f32 zx = rx + iconSize + sepW;
	const f32 ax = zx + iconSize + closeW;
	const f32 controlsRowW = (ax + iconSize + selectW) - x;

	// Big title width (measured at the larger font size).
	print.setFontSize(bigFontW, bigFontH);
	const f32 titleW = print.getWidth("MOD MENU");
	print.setFontSize(baseFontW, baseFontH);

	// Cursor-window scroll: show up to kVisibleRows rows, centered on cursor.
	int first = mCursor - (kVisibleRows / 2);
	if (first < 0) first = 0;
	int last = first + kVisibleRows;
	if (last > mSliderCount) {
		last  = mSliderCount;
		first = last - kVisibleRows;
		if (first < 0) first = 0;
	}
	int firstActionIdx = -1;
	for (int j = 0; j < mSliderCount; ++j) {
		if (mSliders[j].mKind == kModSlider_Action) { firstActionIdx = j; break; }
	}
	int contentRows = (last - first) + 3; // sliders + (N/M) + gap + controls row
	if (first == 0 && mSliderCount > 0) contentRows++; // "Main Variable Mods" header
	if (firstActionIdx >= 0 && first <= firstActionIdx && firstActionIdx < last) contentRows++;

	// Background box: fits the widest of (title, sliders, controls row).
	f32 maxRowW = rowMeasW;
	if (titleW        > maxRowW) maxRowW = titleW;
	if (controlsRowW  > maxRowW) maxRowW = controlsRowW;
	const f32 padX     = 12.0f;
	const f32 padTop   = 24.0f; // big title ascender + a bit
	const f32 padBot   = 10.0f;
	const f32 boxLeft   = x - padX;
	const f32 boxTop    = y - padTop;
	const f32 boxWidth  = maxRowW + padX * 2.0f;
	const f32 boxHeight = padTop + titleGap + (contentRows * dy) + padBot;
	drawCheckerBox(gfx, boxLeft, boxTop, boxWidth, boxHeight);

	// Restore J2DPrint GX state after the box draws changed it.
	print.initiate();
	print.setCharColor(JUtility::TColor(255, 255, 255, 255));
	print.setGradColor(JUtility::TColor(180, 220, 255, 255));

	// === Title row: just "MOD MENU" centered horizontally, in larger font. ===
	print.setFontSize(bigFontW, bigFontH);
	f32 titleX = x + (rowMeasW - titleW) * 0.5f;
	print.setCharColor(JUtility::TColor(255, 40, 40, 255));
	print.print(titleX, y, "MOD MENU");
	print.setFontSize(baseFontW, baseFontH);
	print.setCharColor(JUtility::TColor(255, 255, 255, 255));

	y += titleGap;

	// Section header support: "Main Variable Mods" above the first slider,
	// "Toggle Mods" above the first action slider. Centered horizontally on a
	// representative row width.
	const f32 rowContentW = rowMeasW;
	const JUtility::TColor headerCol(255, 220, 100, 255);

	for (int i = first; i < last; ++i) {
		const ModSlider& s = mSliders[i];

		if (i == 0) {
			const char* h = "Main Variable Mods";
			f32 hx = x + (rowContentW - print.getWidth(h)) * 0.5f;
			print.setCharColor(headerCol);
			print.print(hx, y, h);
			y += dy;
		}
		if (i == firstActionIdx) {
			const char* h = "Toggle Mods";
			f32 hx = x + (rowContentW - print.getWidth(h)) * 0.5f;
			print.setCharColor(headerCol);
			print.print(hx, y, h);
			y += dy;
		}

		const char* marker = (i == mCursor) ? "> " : "  ";
		bool isApplied = (s.mKind == kModSlider_Float || s.mKind == kModSlider_Int)
		                 && s.mTarget && s.mDirty;
		bool isPending = (s.mKind == kModSlider_Float || s.mKind == kModSlider_Int)
		                 && s.mTarget && !s.mDirty && (s.mPendingValue != s.mOriginal);

		// Determine label color and value color independently.
		JUtility::TColor labelCol, valueCol;
		if (i == mCursor) {
			labelCol = valueCol = JUtility::TColor(80, 160, 255, 255);
		} else if (s.mOrangeIfMenuDirty) {
			labelCol = valueCol = isMenuDirty() ? JUtility::TColor(255, 160, 0, 255)
			                                    : JUtility::TColor(255, 255, 255, 255);
		} else if (s.mKind == kModSlider_Action) {
			labelCol = s.mGreenLabel ? JUtility::TColor(80, 220, 80, 255)
			                         : (s.mEnabled ? JUtility::TColor(80, 220, 80, 255)
			                                       : JUtility::TColor(255, 255, 255, 255));
			valueCol = s.mEnabled ? JUtility::TColor(80, 220, 80, 255)
			                      : JUtility::TColor(255, 255, 255, 255);
		} else if (isApplied) {
			labelCol = valueCol = JUtility::TColor(80, 220, 80, 255);
		} else if (isPending) {
			labelCol = JUtility::TColor(255, 255, 255, 255);
			valueCol = JUtility::TColor(220, 60, 60, 255);
		} else {
			labelCol = valueCol = JUtility::TColor(255, 255, 255, 255);
		}

		// Print label portion, then value portion with separate colors.
		char labelBuf[24];
		sprintf(labelBuf, "%s%-18s ", marker, s.mLabel);
		f32 valueX = x + print.getWidth(labelBuf);

		print.setCharColor(labelCol);
		print.print(x, y, "%s%-18s ", marker, s.mLabel);

		if (!s.mNoValue) {
			print.setCharColor(valueCol);
			if (s.mKind == kModSlider_Float && s.mTarget) {
				char vbuf[16];
				print.print(valueX, y, "%7s", fmtFloat(vbuf, s.mPendingValue));
			} else if (s.mKind == kModSlider_Int && s.mTarget) {
				print.print(valueX, y, "%7d", (int)s.mPendingValue);
			} else if (s.mKind == kModSlider_TimeOfDay) {
				print.print(valueX, y, "%5ds", (int)s.mPendingValue);
			} else if (s.mKind == kModSlider_Cycler) {
				if (s.mGetDisplayStr) print.print(valueX, y, "%7s", s.mGetDisplayStr());
			} else if (s.mKind == kModSlider_Action) {
				if (s.mGetDisplayStr) {
					print.print(valueX, y, "%7s", s.mGetDisplayStr());
				} else if (s.mGetDisplayInt) {
					print.print(valueX, y, "%7d", s.mGetDisplayInt());
				} else {
					print.print(valueX, y, "%7s", s.mEnabled ? "On" : "Off");
				}
			}
		}
		y += dy;
	}

	// Fixed footer line 1: cursor indicator (N/M).
	print.setCharColor(JUtility::TColor(80, 160, 255, 255));
	print.print(x, y, "(%d/%d)", mCursor + 1, mSliderCount);
	y += dy;

	// Blank gap row to visually separate the footer controls from the menu content.
	y += dy;

	// === Footer line 2: [L]+[R]+[Z] close, [A] select  (icons + text on the same row) ===
	// Baseline: icons at exact same y as text. Adjust this offset based on what
	// the user actually sees in-game (icon-to-text alignment via mOrthoGraph
	// vs J2DPrint may need a constant nudge).
	const f32 ctrlIconY = y - 15.0f;
	drawBtnIcon(gfx, lx, ctrlIconY, iconSize, 6); // L
	drawBtnIcon(gfx, rx, ctrlIconY, iconSize, 7); // R
	drawBtnIcon(gfx, zx, ctrlIconY, iconSize, 5); // Z
	drawBtnIcon(gfx, ax, ctrlIconY, iconSize, 0); // A

	// Restore J2DPrint state after the icon draws changed it.
	print.initiate();
	print.setCharColor(JUtility::TColor(255, 255, 255, 255));
	print.setGradColor(JUtility::TColor(180, 220, 255, 255));

	print.print(lx + iconSize, y, "+");
	print.print(rx + iconSize, y, "+");
	print.print(zx + iconSize, y, " close, ");
	print.print(ax + iconSize, y, " select");
}

// ---------- Actions ----------

static void action_killAllEnemies()
{
	// generalEnemyMgr dispatches the kill to every per-enemy manager.
	if (Game::generalEnemyMgr) {
		Game::generalEnemyMgr->killAll();
		OSReport("[MOD] killAll() dispatched\n");
	}
}

static void action_giveAllPikiFlower()
{
	// Promote every active pikmin to flower stage (EPikiHappa::Flower = 2).
	// mOpenIds[i] == true means slot is empty; == false means active.
	if (!Game::pikiMgr) return;
	int promoted = 0;
	const int cap = Game::pikiMgr->getMax();
	for (int i = 0; i < cap; ++i) {
		if (Game::pikiMgr->getFlag(i)) continue; // slot empty
		Game::Piki* p = Game::pikiMgr->getAt(i);
		if (!p || !p->isAlive()) continue;
		p->changeHappa(Game::Flower);
		promoted++;
	}
	OSReport("[MOD] promoted %d piki to flower\n", promoted);
}

static void action_killAllPikmin()
{
	if (Game::pikiMgr) {
		Game::pikiMgr->killAllPikmins();
		OSReport("[MOD] killAllPikmins() dispatched\n");
	}
}

static void action_allPikiToOnyon()
{
	if (Game::pikiMgr) {
		Game::pikiMgr->forceEnterPikmins(0);
		OSReport("[MOD] forceEnterPikmins() dispatched\n");
	}
}

static void action_onyonToCaptain()
{
	if (!Game::pikiMgr || !Game::playData || !Game::naviMgr) return;

	// Get captain 0 (Olimar) as the formation target.
	Game::Navi* navi = nullptr;
	for (int i = 0; i < Game::naviMgr->getMax(); ++i) {
		if (!Game::naviMgr->getFlag(i)) {
			navi = Game::naviMgr->getAt(i);
			break;
		}
	}
	if (!navi) return;

	Vector3f spawnPos = navi->getPosition();

	int toSpawn = (int)gFieldCap - (int)Game::GameStat::alivePikis;
	if (toSpawn <= 0) return;

	int spawned = 0;
	for (int color = 0; color < Game::PikiColorCount && toSpawn > 0; ++color) {
		// Prefer higher growth stages (Flower > Bud > Leaf).
		for (int happa = Game::Flower; happa >= Game::Leaf && toSpawn > 0; --happa) {
			int& count = Game::playData->mPikiContainer.getCount(color, happa);
			while (count > 0 && toSpawn > 0) {
				Game::Piki* piki = Game::pikiMgr->birth();
				if (!piki) goto done;

				count--;
				piki->init(nullptr);
				piki->changeShape(color);
				piki->changeHappa(happa);

				Vector3f vel(randWeightFloat(20.0f) - 10.0f, 60.0f + randWeightFloat(20.0f),
				             randWeightFloat(20.0f) - 10.0f);
				piki->setPosition(spawnPos, false);
				piki->setVelocity(vel);

				piki->mNavi = navi;
				PikiAI::ActFormationInitArg arg(navi);
				piki->mBrain->start(PikiAI::ACT_Formation, &arg);

				--toSpawn;
				++spawned;
			}
		}
	}
done:
	OSReport("[MOD] Onyon -> Captain: spawned %d pikmin\n", spawned);
}

static void action_spawnFiveEach()
{
	if (!Game::pikiMgr || !Game::naviMgr) return;

	Game::Navi* navi = Game::naviMgr->getActiveNavi();
	if (!navi) return;

	Vector3f spawnPos = navi->getPosition();
	int spawned   = 0;
	bool poolFull = false;

	// Menu idx → engine PikiColor (Blue=0, Red=1, Yellow=2, Purple=3, White=4).
	// idx 0 = Random/each (handled separately).
	static const int kIdxToColor[]
	    = { -1, Game::Blue, Game::White, Game::Purple, Game::Yellow, Game::Red };
	const int kIdxToColorN = sizeof(kIdxToColor) / sizeof(kIdxToColor[0]);

	int firstColor, lastColor;
	if (sSpawnPikiKindIdx == 0) {
		// Random: spawn 5 of each storable color.
		firstColor = 0;
		lastColor  = (int)Game::StoredPikiCount - 1;
	} else if ((int)sSpawnPikiKindIdx < kIdxToColorN) {
		firstColor = lastColor = kIdxToColor[sSpawnPikiKindIdx];
	} else {
		return;
	}

	for (int color = firstColor; color <= lastColor && !poolFull; ++color) {
		for (int n = 0; n < 5 && !poolFull; ++n) {
			if ((int)Game::GameStat::alivePikis >= (int)gFieldCap) { poolFull = true; break; }

			Game::Piki* piki = Game::pikiMgr->birth();
			if (!piki) { poolFull = true; break; }

			piki->init(nullptr);
			piki->changeShape(color);
			piki->changeHappa(Game::Flower);

			Vector3f vel(randWeightFloat(20.0f) - 10.0f, 60.0f + randWeightFloat(20.0f),
			             randWeightFloat(20.0f) - 10.0f);
			piki->setPosition(spawnPos, false);
			piki->setVelocity(vel);

			piki->mNavi = navi;
			PikiAI::ActFormationInitArg arg(navi);
			piki->mBrain->start(PikiAI::ACT_Formation, &arg);
			++spawned;
		}
	}
	OSReport("[MOD] +5 spawn (%s): %d pikmin at active captain\n",
	         sSpawnPikiKindNames[sSpawnPikiKindIdx], spawned);
}

static void action_addPokos()
{
	if (Game::playData) {
		Game::playData->addPokos(1000);
		OSReport("[MOD] +1000 pokos\n");
	}
}

static void action_toggleNoclip()
{
	if (!Game::gameSystem) return;
	if (Game::gameSystem->isFlag(Game::GAMESYS_DisableCollision)) {
		Game::gameSystem->resetFlag(Game::GAMESYS_DisableCollision);
		OSReport("[MOD] noclip OFF\n");
	} else {
		Game::gameSystem->setFlag(Game::GAMESYS_DisableCollision);
		OSReport("[MOD] noclip ON\n");
	}
}

static void action_toggleFreezeDay()
{
	// GAMESYS_DisablePause stops time-of-day advancement per GameSystem.h comment.
	if (!Game::gameSystem) return;
	if (Game::gameSystem->isFlag(Game::GAMESYS_DisablePause)) {
		Game::gameSystem->resetFlag(Game::GAMESYS_DisablePause);
		OSReport("[MOD] day clock UNFROZEN\n");
	} else {
		Game::gameSystem->setFlag(Game::GAMESYS_DisablePause);
		OSReport("[MOD] day clock FROZEN\n");
	}
}

static void action_toggleAutoSkipCutscenes()
{
	gAutoSkipCutscenes = !gAutoSkipCutscenes;
	OSReport("[MOD] auto-skip cutscenes %s\n", gAutoSkipCutscenes ? "ON" : "OFF");
}

static void action_toggleCutscenesSkippable()
{
	gCutscenesSkippable = !gCutscenesSkippable;
	OSReport("[MOD] cutscenes skippable %s\n", gCutscenesSkippable ? "ON" : "OFF");
}

static void action_cycleSpawnBoss()
{
	if (!Game::generalEnemyMgr || !Game::naviMgr) {
		OSReport("[MOD] spawn-boss: generalEnemyMgr or naviMgr null\n");
		return;
	}
	Game::Navi* active = Game::naviMgr->getActiveNavi();
	if (!active) {
		OSReport("[MOD] spawn-boss: no active navi\n");
		return;
	}

	const BossEntry& entry = sBossCycle[sBossCycleIdx];

	// Spawn at the whistle (throw cursor) so it lands where the captain is aiming.
	// Floor-snap so the boss doesn't fall through or float in the air.
	Game::EnemyBirthArg arg;
	arg.mPosition = active->mWhistle ? active->mWhistle->getPosition()
	                                 : active->getPosition();
	if (Game::mapMgr) {
		arg.mPosition.y = Game::mapMgr->getMinY(arg.mPosition);
	}
	arg.mFaceDir = 0.0f;

	// generalEnemyMgr->birth() returns null if the per-type EnemyMgr isn't loaded
	// for the current cave. To make the cycler still feel responsive, walk forward
	// through the boss list until we find one that *is* loaded for this stage.
	Game::EnemyBase* spawned = nullptr;
	int triedName = sBossCycleIdx;
	for (int tries = 0; tries < sBossCycleCount; ++tries) {
		const BossEntry& e = sBossCycle[(sBossCycleIdx + tries) % sBossCycleCount];
		spawned = Game::generalEnemyMgr->birth(e.mTypeID, arg);
		if (spawned) {
			spawned->init(nullptr);
			OSReport("[MOD] spawned %s (id %d) at whistle (%.1f, %.1f, %.1f)\n",
			         e.mShortName, e.mTypeID,
			         arg.mPosition.x, arg.mPosition.y, arg.mPosition.z);
			triedName = (sBossCycleIdx + tries) % sBossCycleCount;
			break;
		}
	}
	if (!spawned) {
		OSReport("[MOD] spawn-boss: no boss in sBossCycle is loaded for the current stage\n");
	}

	sBossCycleIdx = (triedName + 1) % sBossCycleCount;
}

static void action_unlockAllOnyons()
{
	if (!Game::playData) return;
	// Five colors: Blue=0, Red=1, Yellow=2, Purple=3, White=4 (NaviContainerFlag bits).
	for (int color = 0; color < 5; ++color) {
		Game::playData->setContainer(color);
	}
	// Boot onions (Red/Blue/Yellow only — Purple/White stay cave-only).
	Game::playData->setBootContainer(0);
	Game::playData->setBootContainer(1);
	Game::playData->setBootContainer(2);
	OSReport("[MOD] all onyons unlocked\n");
}

static void action_discoverAllPikmin()
{
	if (!Game::playData) return;
	for (int color = 0; color < 5; ++color) {
		Game::playData->setMeetPikmin(color);
	}
	OSReport("[MOD] all pikmin colors discovered\n");
}

static void action_markAllTreasures()
{
	if (!Game::playData) return;
	// Flips the high-level story flag the ending uses; doesn't itself add the
	// treasures to the hoard, but it tells the game the player has them.
	Game::playData->setStoryFlag(Game::STORY_AllTreasuresCollected);
	OSReport("[MOD] STORY_AllTreasuresCollected set\n");
}

static void action_refillSprays()
{
	if (!Game::playData) return;
	Game::playData->mSprayCount[0] += 10;
	Game::playData->mSprayCount[1] += 10;
	OSReport("[MOD] +10 sprays added (spicy=%d bitter=%d)\n",
	         Game::playData->mSprayCount[0], Game::playData->mSprayCount[1]);
}

static void action_restoreDefaults()
{
	if (gModMenu) gModMenu->restoreAll();
}

} // namespace moddingU
