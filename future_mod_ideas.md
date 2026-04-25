# Future Mod Ideas

Research dump from a 15-agent sweep of the Pikmin 2 decompilation looking for everything we *could* add to the mod menu but haven't yet. Each entry tries to give a label, a slider/toggle/action shape, and the file:line where the constant or struct lives so future-you can wire it up without re-doing the search.

**Already exposed (do not re-add):** Pikmin follow/carry/flower-carry speed, Pikmin HP, per-color attack damage, enemy/pellet search range, bomb throw distance, purple pound dmg, purple carry mult, purple lift mult, spicy duration/run-speed, whistle radius, throw distance/heights (R/B/Y/P/W), captain HP, captain run speed, max on field, time of day. Actions: kill all enemies, all piki → flower, kill all piki, all piki → onyon, onyon → captain, +5 each pik type, +$1000 pokos, toggle noclip, freeze day, refill sprays, restore defaults.

---

## 1. Quick wins (cheapest, highest impact)

These are single-bool gates or one-line writes — start here.

| Idea | Shape | Touch point |
|---|---|---|
| Invincible captain | TOGGLE | `Navi::damage` early-return |
| Hazard immunity ALL (fire/elec/gas/bubble/press) | TOGGLE | gates in `interactPiki.cpp` 334/441/499/527/574 |
| Infinite bomb rocks | TOGGLE | gate consume in piki bomb hold |
| Show enemy HP bars always | TOGGLE | `lifeGaugeMgr` render gate |
| Skip cutscenes | TOGGLE | auto-finish active `moviePlayer` clip |
| BGM master volume | SLIDER 0–1 | `PSGetSystemIF()->mBgmVolume` (`PSSystemIF.h:174`) |
| SFX master volume | SLIDER 0–1 | `PSGetSystemIF()->mSfxVolume` (`PSSystemIF.h:174`) |
| Reroll cave layout (live) | ACTION | `randMapMgr->create()` |
| Warp to sublevel N | ACTION | `CaveSaveData::mCurrentFloor` (`gamePlayData.h:111`) |
| Free-cam mode | TOGGLE | detach `cameraMgr` from captain |
| Spawn enemy at cursor (cycler) | ACTION | `generalEnemyMgr->birth(EnemyTypeID, EnemyBirthArg)` |
| Open all caves | ACTION | loop `playData->openCourse(i)` (clamp to real count!) |

---

## 2. Engine / GameSystem flags

`GameSystem.h:24-31` — each is one bit. Already noclip/freeze use these.

- **Toggle death counter** — `GAMESYS_DisableDeathCounter` (0x10). Hide pikmin death tally.
- **Disable game-world updates** — `GAMESYS_IsGameWorldActive` (0x20). Effective "pause everything but UI."
- **Disable pause menu** — `GAMESYS_DisablePause` (0x8). For runs / no-pause challenges.
- **Disable sound scene** — `GAMESYS_IsSoundSceneActive` (0x1). Mute audio scene transitions.
- **Force "is playing"** — `GAMESYS_IsPlaying` (0x2). Used to break/unbreak demo loops.

`Creature.h:77-85` flags — per-entity, but flippable globally if iterated:

- `CF_IsAtari` (0x1) — toggle hitbox active
- `CF_IsAlive` (0x2) — flip alive without death anim
- `CF_IsCollisionFlick` (0x4) — disable collision-flick reaction
- `CF_IsMovieActor` / `CF_IsMovieMotion` (0x10/0x20) — include in cutscenes
- `CF_IsDebugCollision` (0x80000000) — visualize collision debug

`MoviePlayer.h:69-80`:

- **Always-skippable cutscenes** — `MVP_DoSkip` always set
- **Hide pikmin / enemies / pellets / navi during movies** — `MVDP_DrawPikmin/Enemy/Pellet/Navi` toggles

`SingleGameSection.h`:

- **Treasure radar override** — `mTreasureRadarActive` (0x23D)
- **Force in-cave flag** — `mInCave` (0x23C)
- **Force "game started"** — `mIsGameStarted` (0x228)

---

## 3. Piki parameter sliders (PikiParms.h)

Beyond what's already exposed:

- **Leaf → Bud time** SLIDER 1–1000s default 10 — `PikiParms.h:70` `mSeedLeaftoBudTime`
- **Bud → Flower time** SLIDER 1–1000s default 10 — `PikiParms.h:71` `mSeedBudToFlowerTime`
- **Flower → Wither time** SLIDER 1–1000s default 10 — `PikiParms.h:72` `mSeedFlowerToWitherTime`
- **Wither → Leaf rebirth** SLIDER 1–1000s — `PikiParms.h:73` `mSeedWitherToLeaf`
- **Buried seed wait** SLIDER 0–300s — `PikiParms.h:75` `mBuriedSeedWaitTime`
- **Lost-child timeout** SLIDER 0–1000s default 10 — `PikiParms.h:49` `mLostChildTime`
- **Time to become free** SLIDER 1–1080s default 15 — `PikiParms.h:56` `mTimeToFree`
- **Knockdown recover min/max** SLIDER 0–10s — `PikiParms.h:66/68`
- **Drown frames** SLIDER 0–300 default 30 — `PikiParms.h:91` `mMaxDrowningFrames`
- **Drown frames variance** SLIDER 0–300 default 10 — `PikiParms.h:92`
- **Fire panic time** SLIDER 0–30s default 3 — `PikiParms.h:80` `mPanicMaxTime`
- **Crush time** SLIDER 0–10s default 5 — `PikiParms.h:29` `mCrushTime`
- **Unpluckable seed time** SLIDER 0–300s — `PikiParms.h:76` `mUnpluckableTime`
- **Grow-up to flower time** SLIDER 0–300s default 4 — `PikiParms.h:77` `mGrowUpToFlowerTime`
- **Withered hide time** SLIDER 0–300s — `PikiParms.h:79` `mWitheredHideTime`
- **Flower piki gravity** SLIDER 0–1 default 0.01 — `PikiParms.h:74` `mFlowerPikiGravity`
- **White search dist (squad)** SLIDER 0–2000 default 600 — `PikiParms.h:47` `mWhiteDistance`
- **White search dist (free)** SLIDER 0–2000 default 200 — `PikiParms.h:50` `mWhiteDistanceFree`
- **Gray (Louie) search dist (squad)** SLIDER 0–2000 default 1000 — `PikiParms.h:48`
- **Gray (Louie) search dist (free)** SLIDER 0–2000 default 800 — `PikiParms.h:51`
- **Nectar rock search range** SLIDER 10–1000 default 30 — `PikiParms.h:59`
- **Bridge search range** SLIDER 10–1000 default 100 — `PikiParms.h:62`
- **Clog search range** SLIDER 10–1000 default 30 — `PikiParms.h:63`
- **Spider-wort search range** SLIDER 10–1000 default 30 — `PikiParms.h:65`
- **Bud carry speed bonus** SLIDER 0–2x default 0.5 — `PikiParms.h:107`
- **White carry strength** SLIDER 0.1–5x default 1.5 — `PikiParms` (mWhiteCarryPower)

---

## 4. Navi / Captain parameter sliders (NaviParms.h)

- **Action radius** SLIDER 5–200 default 25 — `NaviParms.h:11` `mActionRadius`
- **Auto-pluck distance** SLIDER 50–500 default 200 — `NaviParms.h:12` `mAutopluckDistance`
- **Pluck-outside-onyon distance** SLIDER 0–500 default 15 — `NaviParms.h:13`
- **Whistle min radius** SLIDER 0–500 default 5 — `NaviParms.h:15` `mPikiCallMinRadius`
- **Whistle max-call time** SLIDER 0.05–2s default 0.45 — `NaviParms.h:16` `mMaxCallTime`
- **Whistle ring fade time** SLIDER 0–500s default 1 — `NaviParms.h:17` `mCircleDisappearTime`
- **Lose-numbness time** SLIDER 0–60s default 5 — `NaviParms.h:51` `mPikiLoseNumbnessTime`
- **Throw hold time cap** SLIDER 0.5–10s default 2.5 — `NaviParms.h:26` `mTimeLimitForThrowing`
- **Throw landing recovery** SLIDER 0.2–3s default 1 — `NaviParms.h:34` `mLandingTime`
- **Grab pikmin range** SLIDER 5–100 default 15 — `NaviParms.h:35`
- **Captain accel** SLIDER 0.01–1 default 0.1 — `NaviParms.h:150` `mCreatureProps.mProps.mAccel`
- **Captain friction** SLIDER 0.1–5 default 1 — `NaviParms.h:148` `mCreatureProps.mProps.mFriction`
- **Run-stick threshold** SLIDER 0.1–1 default 0.8 — `NaviParms.h:21-24` `mStick1`
- **Formation cap Lv0/1/2** SLIDER 1–100 — `NaviParms.h:66-68`
- **Sprite invincibility timer** SLIDER 0–255 frames — `Navi.h` (`mInvincibleTimer`)

---

## 5. Movement / speedrun tech

- **Climb speed** SLIDER 50–500 default 100 — hardcoded at `naviState.cpp:3536`
- **Throw momentum scale** SLIDER 0–3x — `navi.cpp:5951` (non-matching)
- **Throw gravity override** SLIDER 1–20 — `navi.cpp:5929` reads `_aiConstants->mGravity`
- **Throw hand-grab distance** SLIDER 10–100 default 32.5 — `naviState.cpp:4986`
- **Throw arc time** SLIDER 0.2–3s default 1 — `mLandingTime/2` at `navi.cpp:5915`
- **Pressed (crushed) recover time** SLIDER 0.1–5s default 2 — `naviState.cpp:6740`
- **Global gravity** SLIDER 50–400 — `AIConstants.h:16` (affects all throws + falls)
- **Formation base radius (spacing)** SLIDER 5–50 default 10 — `gameCPlate.cpp:75`

---

## 6. Enemy globals & boss tunables

### Global multipliers (need new `gEnemy*Mult` globals patched into `EnemyBase` paths)

- **Enemy HP mult** SLIDER 0.1–5x — `EnemyParmsBase.h:105` `mHealth`
- **Enemy attack damage mult** SLIDER 0.1–5x — `EnemyParmsBase.h:137`
- **Enemy move speed mult** SLIDER 0.1–2x — `EnemyParmsBase.h:117`
- **Enemy sight/search range mult** SLIDER 0.25–4x — `EnemyParmsBase.h:123-126`
- **Shake-off resistance mult** SLIDER 0.1–5x — `EnemyParmsBase.h:129-130`

### Per-boss tunables

| Boss | File | Suggested entries |
|---|---|---|
| Waterwraith (BlackMan) | `Entities/BlackMan.h:233` | Force phase (`mStartPhase`), Escape speed (`mEscapeSpeed` 258), Travel speed (244), Freeze timer (267), Disable flick invuln, Submerged-Castle chase timer |
| Titan Dweevil (BigTreasure) | `Entities/BigTreasure.h:305` | Each element attack duration (fp20–fp23), Discharge count per arm (fe07/17/27/37), Arm-swap interval (fp10–13) |
| Emperor Bulblax (KingChappy) | `Entities/KingChappy.h:47` | Big-mode scale (fp15), Big-mode HP (fp16), Force big mode toggle (`mDoForceBig`), Ground-pound damage (fp05), Trample range (fp08), Underground hide time (`mPeriodOfIncubation`) |
| Pileated Snagret (SnakeWhole) | `Entities/SnakeWhole.h:193` | Time between dives (fp11), Underground duration (fp12), Poison damage (fp21), Disable jump-attack toggle |
| Burrowing Snagret (SnakeCrow) | `Entities/SnakeCrow.h:193` | Wait time (fp11), Fast-appear chance (fp01) |
| Beady/Raging Long Legs (UmiMushi) | `Entities/UmiMushi.h:72` | Leg-stomp interval (`mBlindWaitTime` fp13), Blind-mode HP threshold (fp12), Force blind toggle |
| Man-at-Legs (Houdai) | `Entities/Houdai.h:200` | Minigun burst on/off times (fp10–13), Leg-swing power (fp06), Walking territory radius (fp20) |
| Hermit Crawmad (Jigumo) | `Entities/Jigumo.h:94` | Attack-frame window (`mSAttackActiveFrame`), Hide duration (ip01), Disable press-kill / climbing |
| Empress Bulblax (Queen) | `Entities/Queen.h` | Egg spawn interval, egg explosion delay (need cpp scan) |

### Notable non-bosses

- **Bulborb sleep / wake radius** — `ChappyBase.h:121` `mBulborbWakeRadius` (0–1000)
- **Bulbear (KumaChappy) respawn timer** — header has `mReviveTimer`
- **Dwarf Bulbear stay-near-parent radius** — `KumaKochappy.h:106`
- **Spectralid (ShijimiChou) nectar drop rate** — `Entities/ShijimiChou.h` `mNectarRate`
- **Bomb Beetle (BombSarai) fuse timer** — `BombSarai.cpp:35-50`

### Actions

- **Kill current boss only** (vs. existing kill-all-enemies)
- **Spawn boss at cursor** — cycler over EnemyID_BlackMan / BigTreasure / KingChappy / SnakeWhole / Houdai / Damagumo / etc.

---

## 7. Spawnables — master ID list

For "spawn at cursor" cycler. Spawn via:
```cpp
EnemyBirthArg arg;
arg.mPosition = naviCursor;
arg.mTypeID   = EnemyTypeID::EnemyID_<NAME>;
generalEnemyMgr->birth(arg.mTypeID, arg);
```

### Enemies (EnemyTypeID → English name)

```
0  Pelplant         Pellet Posy
1  Kochappy         Dwarf Red Bulborb
2  Chappy           Red Bulborb
3  BluePom          Lapis Lazuli Candypop
4  RedPom           Crimson Candypop
5  YellowPom        Golden Candypop
6  BlackPom         Violet Candypop
7  WhitePom         Ivory Candypop
8  RandPom          Queen Candypop
9  Kogane           Iridescent Flint Beetle
10 Wealthy          Iridescent Glint Beetle
11 Fart             Doodlebug
12 UjiA             Female Sheargrub
13 UjiB             Male Sheargrub
14 Tobi             Shearwig
15 Armor            Cloaking Burrow-nit
16 Qurione          Honeywisp
17 Frog             Yellow Wollywog
18 MaroFrog         Wollywog
19 Rock             Falling boulder
20 Hiba             Fire geyser hazard
21 GasHiba          Gas pipe hazard
22 ElecHiba         Electric wire hazard
23 Sarai            Swooping Snitchbug
24 Tank             Fiery Blowhog
25 Wtank            Watery Blowhog
26 Catfish          Water Dumple
27 Tadpole          Wogpole
28 ElecBug          Anode Beetle
29 Mar              Puffy Blowhog
30 Queen            Empress Bulblax
31 Baby             Bulborb Larva
32 Demon            Bumbling Snitchbug
33 FireChappy       Fiery Bulblax
34 SnakeCrow        Burrowing Snagret
35 KumaChappy       Spotty Bulbear
36 Bomb             Bomb-rock
37 Egg              Egg
38 PanModoki        Breadbug
39 PanModokiNest    Breadbug nest
40 OoPanModoki      Giant Breadbug
41 Fuefuki          Antenna Beetle
42 BlueChappy       Orange Bulborb
43 YellowChappy     Hairy Bulborb
44 BlueKochappy     Dwarf Orange Bulborb
45 YellowKochappy   Snow Bulborb
46-52 Plants (Tanpopo/Clover/HikariKinoko/Ooinu/Wakame s+l)
53 KingChappy       Emperor Bulblax
54 Miulin           Mamuta
55 Hanachirashi     Withering Blowhog
56 Damagumo         Beady Long Legs
57 Kurage           Lesser Spotted Jellyfloat
58 BombSarai        Careening Dirigibug
59 FireOtakara      Fiery Dweevil
60 WaterOtakara     Caustic Dweevil
61 GasOtakara       Munge Dweevil
62 ElecOtakara      Anode Dweevil
63 Jigumo           Hermit Crawmad
64 JigumoNest       Hermit Crawmad nest
65 Imomushi         Ravenous Whiskerpillar
66 Houdai           Man-at-Legs
67 LeafChappy       Bulbmin
68 TamagoMushi      Mitite
69 BigFoot          Raging Long Legs
70 SnakeWhole       Pileated Snagret
71 UmiMushi         Ranging Bloyster
72 OniKurage        Greater Spotted Jellyfloat
73 BigTreasure      Titan Dweevil
75 Kabuto           Armored Cannon Beetle Larva
76 KumaKochappy     Dwarf Bulbear
77 ShijimiChou      Unmarked Spectralid
78 MiniHoudai       Gatling Groink
84 Hana             Creeping Chrysanthemum
85-92 Glowstems / Margaret / Foxtail / Chigoyami / Fiddlehead / KareOoinu
93 BombOtakara      Volatile Dweevil
94 DangoMushi       Segmented Crawbster
95 Rkabuto          Decorated Cannon Beetle
96 Fkabuto          Armored Cannon Beetle (burrowed)
97 FminiHoudai      Pedestal Gatling Groink
98 Tyre             Waterwraith roller
99 BlackMan         Waterwraith
101 UmiMushiBlind   Toady Bloyster
105 Sokkuri         Skitter Leaf
106 Tukushi         Horsetail
107 Watage          Seeding Dandelion
```

Non-spawnable (`EFlag_CanBeSpawned=0`): 82 Pom (base), 100 UmiMushiBase.

### Items (4-CC) — `BaseItemMgr::generatorBirth`

```
trsr Treasure         brdg Bridge          gate Gate
hole Hole/warp        cave Cave entrance   weed Sprout
plnt Plant/mushroom   onyn Onyon           mitu Honey/nectar
rock Bomb-rock        pkhd Piki sprout     ujms Ujamushi
warp/dwfl Downfloor   barl Barrel          dgat Electric gate
bfdw Big fountain (geyser)
```

### Pellets (`PelletMgr::birth`)

- Number pellets: weights 1, 5, 10, 20, 30, 40, 50, 100; colors PELCOLOR_RED/BLUE/YELLOW
- Berries (PelletFruit): SPICY/BITTER
- Treasures (PelletOtakara) — `OtakaraItemCode` s16, kind=3
- Carcasses, upgrade items (PelletItem)

### Suggested ACTIONS

- Spawn 1/5/10/20-pellet at cursor (color cycler)
- Spawn red/blue/yellow/purple/white/queen candypop at cursor
- Spawn cave-exit hole or geyser at cursor
- Spawn onyon at cursor (cycle color — useful for "unlock onyon mid-cave")
- Spawn random treasure at cursor

---

## 8. Hazards & obstacles

### Pikmin damage

- **Drown frames** (already noted in Piki section)
- **Fire panic time** (already noted)
- **All pikis swim** TOGGLE — new `gAllSwim`. Gate transit at `piki.cpp:303-310` and `piki.cpp:528-531`
- **Auto-rescue from water** TOGGLE — new `gAutoRescue`. Walk pikiMgr in mod tick, blow non-blue/bulbmin out of `mWaterBox`
- **Fire immunity** TOGGLE — early-return `InteractFire::actPiki` (`interactPiki.cpp:441`)
- **Electric immunity (all colors)** TOGGLE — early-return `InteractDenki::actPiki` (`interactPiki.cpp:334`)
- **Gas immunity (all colors)** TOGGLE — early-return `InteractGas::actPiki` (`interactPiki.cpp:527`)
- **Bubble immunity** TOGGLE — `interactPiki.cpp:499`
- **Press immunity** TOGGLE — `interactPiki.cpp:574`
- **Hazard immunity ALL** — single switch flipping all five above

### Hazard objects

- **Fire-Hiba active / wait time** SLIDER 0–30s — `Hiba.h:97-98`
- **Gas-Hiba active / wait / attack-start** SLIDER 0–30s — `GasHiba.h:111-113`
- **Elec-Hiba active / wait / warning** SLIDER 0–30s — `ElecHiba.h:138-140`
- **Geyser HP** SLIDER 1–5000 default 1000 — `ItemBigFountain.h:121` `FountainParms::mHealth`
- **Open all geysers** ACTION — iterate `itemMgr` for OBJTYPE_BigFountain, transit to `BIGFOUNTAIN_Wait` (instant cave exit anywhere)
- **Cardboard piki-to-push** SLIDER 1–100 default 10 — `ItemDownFloor.h:170` `mBagMaxWeight`
- **Spiderwort grow time** SLIDER 0–2000 default 10 — `ItemPlant.h:163` `mGrowTimeToMedium/mGrowTimeToLarge`
- **Spiderwort respawn time** SLIDER 0–2000 default 10 — `mBearFruitTime`
- **Berry plant damage-to-drop** SLIDER 0–2500 default 300 — `mDamageToDrop`
- **Bomb-rock barrel HP** SLIDER 1–5000 default 100 — `ItemBarrel.h:112` `BarrelParms::mHealth`

(Pitfall, bouncy mushroom, whirligig, sticky tar, generic boulder spawner — none exist as discrete items in P2.)

---

## 9. Cave generator & sublevel control

### Live (no re-enter)

- **Warp to sublevel N** ACTION — `CaveSaveData::mCurrentFloor` (`gamePlayData.h:111`)
- **Reroll cave layout (re-seed)** ACTION — `randMapMgr->create()` (`RandMapMgr.cpp:58`)
- **Instant-clear floor** ACTION — trigger `setGoalSlot()` early exit
- **Open all caves** ACTION — set `PDCF_Open` per cave (clamp to real count)

### Re-enter required (write before `RandMapMgr::create`)

- **Min/max enemies per floor** SLIDER 0–128 — `FloorInfo::mParms::mTekiMax` (`Info.h:158`)
- **Min/max treasures per floor** SLIDER 0–128 — `mItemMax` (`Info.h:159`)
- **Max gates per floor** SLIDER 0–32 — `mGateMax`
- **Force hole vs geyser** TOGGLE — `mHasEscapeFountain` (`RandMapUnit.h:65`)
- **Force final-floor flag (boss spawn)** TOGGLE — `mIsFinalFloor` (`RandMapUnit.h:64`)
- **Min/max rooms per floor** SLIDER — `mParms::mRoomCount` (`Info.h:162`)
- **Door connection ratio** SLIDER 0–1 — `mRouteRatio` (`Info.h:163`)
- **Cap-enemy count** SLIDER — `RandCapEnemyUnit::mEnemyCounts[2]`
- **Floor alpha hazard type** SLIDER 0–6 — `mFloorAlphaType` (`Info.h:169`)
- **Floor beta type** SLIDER 0–3 — `mFloorBetaType` (`Info.h:170`)
- **Toggle hidden floor** TOGGLE — `mFloorHidden`
- **Waterwraith timer (Submerged Castle only)** SLIDER — `mWaterwraithTimer` (`Info.h:173`)

### Caveinfo files

`orig/GPVE01/files/user/Mukki/mapunits/caveinfo/` — `caveinfo_*.txt`. Each FloorInfo has `f000-f017` keys (floor range, teki/item/gate/cap max, route ratio, escape flag, alpha/beta type, hidden, waterwraith timer). Useful as a "save preset" target for mod presets.

---

## 10. Pellet / nectar / candypop / berry economy

- **Poko-per-1/5/10/20-pellet** SLIDER — `pelletConfig.cpp` (`PelletConfig::mMoney`)
- **Treasure poko global multiplier** SLIDER 0.25–4x
- **Pellet matched-color sprout count mult** SLIDER 0.5–5x
- **Berries-per-spray ratio** SLIDER 1–50 default 10 — `gamePlayData.cpp` (`addDopeFruit`)
- **Spray held cap** SLIDER 1–9999 default 99 — `gamePlayData.cpp` `mSprayCount`
- **Bitter spray duration** SLIDER 5–300s — needs PikiParms entry (currently spicy only)
- **Onyon storage cap per color** SLIDER 10–500 default 100 — `itemOnyon.cpp`
- **Candypop yield per use** SLIDER 1–20 default 5 — `Pom::Parms::mNormalMaxSlots` (`Pom.h`)
- **Queen-candypop yield** SLIDER 1–20 default 1 — `mQueenMaxSlots`
- **Candypop remain-open time** SLIDER — `mRemainOpenTime` default 30s
- **Random-candypop color-change rate** SLIDER — `mColorChangeTime` default 1.25s

### Actions

- Spawn red/blue/yellow/purple/white/queen candypop at cursor
- Set onyon storage to N (50/100/200 presets)
- Refill all berries
- +10 spicy / +10 bitter sprays

---

## 11. Save data & progression unlocks

`gamePlayData.h`. **WARNING:** out-of-range writes corrupt the GCI; clamp every index. (See `known_pitfalls.md`.)

### Areas / caves

- **Unlock all areas** — loop `playData->openCourse(i)` (`gamePlayData.cpp:1429`), set `PDCF_Open`
- **Mark all areas visited** — `mBitfieldPerCourse[i] |= PDCF_Visited` (0x4)
- **Visit all caves** — `playData->setCaveVisit(courseIdx, caveID)` (`gamePlayData.cpp:1165`)
- **Replay area intros** — set `PDCF_JustOpen` (0x2)

### Treasure Hoard / Piklopedia

- **Mark all treasures collected** — `mZukanStat->mOtakara(i)` set bit 0x2 ("first-carried")
- **Mark all enemies seen** — `mTekiStatMgr.getTekiInfo(i)->mKilledTekiCount = max(1, …)` for all 64 IDs

### Pikmin colors / onyons

- **Unlock all onyons** — `mHasContainerFlags = 0x1F` (`gamePlayData.cpp:601`)
- **Discover all pikmin colors** — `mMeetPikminFlags = 0x1F` (`:615`)
- **Set boot onyons** — `mHasBootContainerFlags` (R/B/Y) (`:629`)
- **Fill all onyon reserves** — `PikiContainer::getCount(color, happa)` (`:498`)

### Story

- **Mark debt paid** — `mStoryFlags |= STORY_DebtPaid` (0x1)
- **Mark all treasures found** — `STORY_AllTreasuresCollected` (0x2)
- **View all president messages** — `mDebtProgressFlags` (note inverted byte indexing: `1 - byte`)

### Cutscenes

- **Unlock all 57 demo flags** — loop `mDemoFlags.setFlag(i)` 0–56 (`gamePlayData.h:27-84`). Includes `DEMO_White_Candypop`, `DEMO_Purple_Candypop`, `DEMO_Unlock_Captain_Switch`, `DEMO_First_Spicy_Spray_Made`, etc. — i.e. unlocks midgame mechanics.

### Currency

- **Set total pokos** SLIDER 0–999999 — `mPokoCount`
- **Max sprays** ACTION — `mSprayCount[0/1] = 999`
- **Fill berry reserves** — `mBerryCount[0/1]`

### Olimar's exploration kit

- **Collect all 13 ODII items** — `mOlimarData[0].getItem(ODII_*)` (`gamePlayData.cpp:339`)

---

## 12. VS / Challenge / 2-player co-op

### VS Mode

- **Marble win threshold** SLIDER 1–8 default 4 — `VsGameSection.h:23` `VS_WIN_YELLOW_MARBLE_NUM`
- **Cherry max count** SLIDER 1–20 default 10 — `VsGameSection.h:15` `VS_CHERRY_MAX_COUNT`
- **Marble respawn delay** SLIDER 0–30s — `vsGameSection.cpp:1064-1084`
- **Score multiplier (per player)** SLIDER 0.5–5x — `vsGameSection.cpp:1396-1397`
- **Time limit override** SLIDER 0–600s — `VsGameSection.h:165` `mTimeLimit`
- **Instant win (Red/Blue)** ACTION — set `mVsWinner` (`vsGS_Game.cpp:220-225`)
- **Sudden-death timer** SLIDER 30–120s
- **Freeze match timer** TOGGLE — gate decrement at `vsGS_Game.cpp:296`
- **P1/P2 handicap** SLIDER 0–10 — `vsGameSection.h:149-150` `mOlimarHandicap` / `mLouieHandicap`
- **VS stage selector** — `mVsStageNum` (`vsGameSection.h:340`)

### Challenge Mode

- **Time limit override** SLIDER 0–600s — `ChallengeGame.h:23` `mTimeLimit`
- **Starting bitter / spicy pikmin** SLIDER 0–100 — `ChallengeGame.h:24-25`
- **Floor time extension** SLIDER 0–60s — `ChallengeGame.h:29` `mFloorTimerExtensions[]`
- **Otakara threshold** SLIDER 0–50 — `ChallengeGame.h:27` `mOtakaraNum`
- **Challenge stage selector** — `mChallengeStageNum` (`vsGameSection.h:338`)

### 2P co-op

- **Force single-screen (no split)** TOGGLE — `baseGameSection.cpp:550-551`
- **Captain swap distance** SLIDER 10–200
- **Friendly-fire toggle**
- **Dual respawn delay** SLIDER 0–10s

### Universal

- **Skip result screen** ACTION — vsGS_Result transition

---

## 13. Audio

Safe (direct global writes):

- **BGM volume** SLIDER 0–1 — `PSGetSystemIF()->mBgmVolume`
- **SFX volume** SLIDER 0–1 — `PSGetSystemIF()->mSfxVolume`
- **Pikmin death scream off** TOGGLE — gate `playMessageVoice` (`PSSe.cpp:46`)
- **Mute SFX category** TOGGLE × 6 — `SoundTable::CategoryMgr::mPerspInfo[cat]->mDisabled` (`SoundTable.h:35`); cats 0=Navi, 1=System, 2=Piki, 3=Item, 4=Env, 5=Enemy
- **Test SFX (ID input)** ACTION — `PSGetSystemIF()->playSystemSe(id, 0)`
- **Audio debug dump** ACTION — print conductor / mask state
- **Tempo** SLIDER 60–200 BPM — `SeqTrackRoot::setTempo`

Medium-risk (need null-checks):

- **BGM fade speed** ACTION — `ModParamWithFade::mFadeDuration`
- **BGM pitch shift** SLIDER ±0.12 — `pitchModulation` (`PSBgmTask.cpp:68`)
- **Playback speed** SLIDER 0.6–1.5x — `tempoChange`
- **Pikmin chorus on/off** TOGGLE — `MeloArrBase::mIsEnabled` (`MeloArr.h:26`)
- **Smooth BGM transitions** TOGGLE — `muteOffAndFadeIn` on scene init

Risky (mid-frame mutex hazard):

- **Boss-layer disable** ACTION — `JumpBgmSeq::mJumpPort->mCurrentTrackId = 0` (only safe at boss-fight start)

---

## 14. World / camera / lighting / UI

### Camera (`CameraMgr.h:134-154`)

- **Zoom dist Near/Mid/Far** SLIDERS 10–5000 — `mNearLowDist`, `mMidLowDist`, `mFarLowDist`
- **FOV per zoom** SLIDERS 1–60° — `mNearLowFOV`, etc.
- **Pitch per zoom** SLIDERS 0–90° — `mNearLowAngle`, etc.
- **Vertical offset per zoom** SLIDERS 10–500 — `mNearLowOffset`, etc.
- **Follow smoothness per zoom** SLIDERS — `mNearLowWeight`, etc.
- **Far clip plane** SLIDER 100–12800 — `playCamera.cpp:34`
- **Near clip plane** SLIDER 0.1–100 — `playCamera.cpp:32`

### Lighting (`GameLightSetting.h`)

- **Ambient RGB** 3× SLIDERS 0–255 — `:111-117`
- **Fog near plane** SLIDER 0–15000 — `:77`
- **Fog far plane** SLIDER 0–15000 — `:78`
- **Fog RGB** 3× SLIDERS — `:61-64`

### Time (`TimeMgr.h`)

- **Day length seconds** SLIDER 300–3600 — `mDayLengthSeconds`
- **Day speed multiplier** SLIDER 0.1–5x — `mSpeedFactor` (`:97`)
- **Sunrise / sunset hours** SLIDERS 0–24 — `mMorningStartTime`, `mEveningStartTime`, `mEveningEndTime`
- **Day count** SLIDER 1–100 — `mDayCount` (`:96`)

### Water / particles / shadows

- **Water level** SLIDER ±1000 — `WaterBox.h:98` `mWaterTop`
- **Water depth** SLIDER 1–5000 — `WaterBox.h:99` `mWaterHeight`
- **Max particle count** SLIDER 0–1000 — `particleMgr.cpp:64`
- **Disable shadows** TOGGLE — `shadowMgr.h:267` `mEnabled`
- **Shadow LOD distances** SLIDERS — `mLodNear`, `mLodFar` (`:37, 43`)

### HUD

- **Show coords / FPS overlay** TOGGLE — `BaseGameSection::sOptDraw` (`baseGameSection.cpp:63`)
- **Show treasure value next to icon** TOGGLE — `carryInfoMgr` 2D
- **Fast text / instant J2DPrint** TOGGLE
- **Force always-skippable cutscenes** (engine flags section)

---

## 15. Debug / dev / unused content

- **Toggle death counter** — `GAMESYS_DisableDeathCounter` (already in flags section)
- **Re-implement treasure-detector HUD** — `test_draw_treasure_detector` is an empty stub at `baseGameSection.cpp:1409`
- **Spawn Sarai (Swooping Snitchbug, ID 23)** — fully implemented but rare; cycler bait
- **Spawn Houdai (Man-at-Legs, ID 66)** — `Houdai.cpp:66` `TODO: Finish` but works
- **Load test_map** — `stages.txt:176-217` defines an internal test course with 9 placeholder otakara
- **Play any movie by ID 0–11** — `THPPlayer::load(EMovieIndex)`. List: OPENING_GameStart (PLAY_1-6 idle demos), OPENING_PostDebtStart, ENDING_PayDebt, ENDING_AllTreasures, STAFF_ROLL, **CRIME** (unused, ID 11; demo build had `/thp/crime.ini` captions)
- **RNG seed override** — `TRandomEnough_::setSeed(u32)` (`random.cpp:19`); useful for deterministic runs and same-seed cave reproduction
- **Beta gate cutscene** — `navi_demoCheck.cpp:72` has unused leftover that triggered when near a specific gate
- **`test_createUjas`** — `itemUjamushi.cpp:2040` spawns test bomb creatures
- **Floor-beta-type strings** unused: `通常 / ボス / やすらぎ` (Normal/Boss/Rest) at `gameCaveInfo.cpp:342`

---

## 16. Implementation notes & cautions

- **MAX_PIKI_COUNT stays at 100.** Sliders that allocate more piki must clamp to 100 (see `CLAUDE.md`).
- **GCI corruption.** `openCourse(i)` etc. with out-of-range `i` corrupts `~/.var/app/org.DolphinEmu.dolphin-emu/data/dolphin-emu/GC/USA/Card A/01-GPVE-Pikmin2_SaveData.gci` and crashes on world-map load. Always clamp to the real course count of the active stage list.
- **Non-matching only.** Many of these (especially physics/momentum, FSM gates, struct adds) break the SHA1 match. Use `python configure.py --non-matching` (already required for the mod menu lib).
- **Pattern.** Most simple parm sliders just need an `ADD_PARM`-style line in `buildSliders()` in `src/moddingU/modMenu.cpp`. New globals follow the `gFieldCap` / `gPurpleLiftScale` pattern: add to `include/moddingU/modMenu.h`, default-init in `modMenu.cpp`, read at the patch site in the relevant cpp file.
- **README table.** When you add a slider/action here to the menu, also update the "Available Mods" table in `README.md` (per the project rule).
- **Live vs. requires-reload.** Cave-gen sliders that affect floor layout (`mTekiMax`, `mItemMax`, room counts) only take effect after the next sublevel transition. Live ones (sublevel warp, reroll-via-`create()`) apply instantly. Tag them so the menu UI hints "(reloads)" if you wire it in.
- **Live geyser/exit action.** "Open all geysers" → instant warp out is one of the highest-utility/lowest-risk additions; reuses `BIGFOUNTAIN_Wait` transit that already exists in the FSM.
- **Spawn-at-cursor** needs the `Navi` reticle position; `naviMgr->getActiveNavi()` and its lookat target give a usable spawn location.
