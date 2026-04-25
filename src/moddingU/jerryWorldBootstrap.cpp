#include "moddingU/jerryWorldBootstrap.h"

#include "Dolphin/os.h"
#include "Dolphin/rand.h"

#include "Game/GameSystem.h"
#include "Game/BaseItem.h"
#include "Game/MapMgr.h"
#include "Game/EnemyBase.h"
#include "Game/EnemyMgrBase.h"
#include "Game/generalEnemyMgr.h"
#include "Game/enemyInfo.h"
#include "Game/gameStages.h"

namespace moddingU {
namespace jerryWorldBootstrap {

static bool sHasBootstrapped  = false;
static int  sLastCourseIndex  = -1;

static void spawnEnemy(int enemyId, f32 x, f32 y, f32 z)
{
	if (!Game::generalEnemyMgr) return;
	Game::EnemyBirthArg arg;
	arg.mFaceDir  = randFloat() * TAU;
	arg.mPosition = Vector3f(x, y, z);
	if (Game::mapMgr) {
		arg.mPosition.y = Game::mapMgr->getMinY(arg.mPosition);
	}
	Game::EnemyBase* enemy = Game::generalEnemyMgr->birth(enemyId, arg);
	if (enemy) {
		enemy->init(nullptr);
	}
}

static void bootstrapContent()
{
	// Anchor position matches jerry's start in stages.txt (copied from forest).
	const f32 cx = 381.724f;
	const f32 cy = -70.880f;
	const f32 cz = 2634.461f;

	// 6-10 enemies: 3 red bulborbs, 2 dwarf red bulborbs, 1 fiery bulblax,
	// 1 greater spotted jellyfloat, 1 breadbug.
	spawnEnemy(Game::EnemyTypeID::EnemyID_Chappy,      cx + 200.0f, cy, cz + 150.0f);
	spawnEnemy(Game::EnemyTypeID::EnemyID_Chappy,      cx - 250.0f, cy, cz + 100.0f);
	spawnEnemy(Game::EnemyTypeID::EnemyID_Chappy,      cx + 100.0f, cy, cz - 300.0f);
	spawnEnemy(Game::EnemyTypeID::EnemyID_Kochappy,    cx + 350.0f, cy, cz + 250.0f);
	spawnEnemy(Game::EnemyTypeID::EnemyID_Kochappy,    cx - 400.0f, cy, cz - 200.0f);
	spawnEnemy(Game::EnemyTypeID::EnemyID_FireChappy,  cx - 100.0f, cy, cz + 400.0f);
	spawnEnemy(Game::EnemyTypeID::EnemyID_OniKurage,   cx + 450.0f, cy, cz - 100.0f);
	spawnEnemy(Game::EnemyTypeID::EnemyID_PanModoki,   cx - 350.0f, cy, cz + 350.0f);

	OSReport("[MOD] Jerry's World bootstrap: spawned extra enemies\n");
}

void onUpdate()
{
	int courseIndex = -1;
	if (Game::mapMgr && Game::mapMgr->mCourseInfo) {
		courseIndex = Game::mapMgr->mCourseInfo->mCourseIndex;
	}

	// Reset bootstrap flag on course transitions.
	if (courseIndex != sLastCourseIndex) {
		sHasBootstrapped = false;
		sLastCourseIndex = courseIndex;
	}

	if (courseIndex != 4 || sHasBootstrapped) return;

	// Wait for managers to be ready before spawning.
	if (!Game::generalEnemyMgr || !Game::mapMgr || !Game::gameSystem) return;
	if (Game::gameSystem->paused()) return;

	bootstrapContent();
	sHasBootstrapped = true;
}

} // namespace jerryWorldBootstrap
} // namespace moddingU
