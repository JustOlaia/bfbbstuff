#ifndef ZNPCTYPEBOSSPLANKTON_H
#define ZNPCTYPEBOSSPLANKTON_H

#include "zNPCTypeBoss.h"
#include "zNPCTypeVillager.h"
#include "zNPCGoalCommon.h"
#include "zEntDestructObj.h"

#include "xDecal.h"
#include "xLaserBolt.h"
#include "xTimer.h"
#include "zNPCGoals.h"
#include "xParEmitter.h"
#include "xLaserBolt.h"

namespace auto_tweak
{
    template <class T1, class T2>
    void load_param(T1&, T2, T2, T2, xModelAssetParam*, U32, const char*);
};

struct zNPCBPlankton : zNPCBoss
{
    // Confirmed from DOL: update_move dispatches on this, MOVE_ORBIT = 3
    enum move_enum
    {
        MOVE_NONE,
        MOVE_ACCEL,
        MOVE_STOP,
        MOVE_ORBIT
    };

    // Confirmed from DOL: update_follow dispatches on this, FOLLOW_CAMERA = 2
    enum follow_enum
    {
        FOLLOW_NONE,
        FOLLOW_PLAYER,
        FOLLOW_CAMERA
    };

    enum mode_enum
    {
    	MODE_HARASS,
        MODE_BUDDY
    };

    // Confirmed from DOL offsets: dest=0x474, vel=0x480, accel=0x48c, max_vel=0x498
    struct move_info
    {
        xVec3 dest;     // 0x474
        xVec3 vel;      // 0x480
        xVec3 accel;    // 0x48c
        xVec3 max_vel;  // 0x498
    };

    // Confirmed from DOL:
    //   size = 0x3c per entry (mulli r3, r0, 0x3c in ParseLinks/reset_territories)
    //   +0x00 origin      (zMovePoint*)   - stw r29, 0x0(r31) in load_territory case 0xd
    //   +0x04 platform    (xEnt*)         - stw r29, 0x4(r31) in load_territory fallback
    //   +0x08 fuse        (zEntDestructObj*) - stw r29, 0x8(r31) in load_territory case 0x1b
    //   +0x0c timer       (xTimer*)       - stw r29, 0xc(r31) in load_territory case 0x2b
    //   +0x10 crony[8]    (zNPCCommon*[8])- stw r29, 0x10(r3) in load_territory NPC branch
    //   +0x30 crony_size  (S32)           - lwz r0, 0x30(r31) in cronies_dead/load_territory
    //   +0x34 fuse_destroyed (U8)         - stb r0, 0x34(r5) in reset_territories
    //   +0x35 fuse_detected  (U8)         - stb r0, 0x35(r5) in reset_territories
    //   NOTE: fuse_destroyed (0x34) comes BEFORE fuse_detected (0x35) - reversed vs old repo header
    //   +0x38 fuse_detect_time (F32)      - stfs f0, 0x38(r5) in reset_territories
    struct territory_data
    {
        zMovePoint* origin;         // +0x00
        xEnt* platform;             // +0x04
        zEntDestructObj* fuse;      // +0x08
        xTimer* timer;              // +0x0c
        zNPCCommon* crony[8];       // +0x10
        S32 crony_size;             // +0x30
        U8 fuse_destroyed;          // +0x34  NOTE: destroyed before detected
        U8 fuse_detected;           // +0x35
        F32 fuse_detect_time;       // +0x38
    };                              // sizeof = 0x3c

    // Flag struct confirmed from DOL:
    //   0x2b4 updated     (stb base for 0x10-byte memset in Reset)
    //   0x2b5 face_player (byte, confirmed from Process flag.face_player check)
    //   0x2b6 attacking   (lbz r0, 0x2b6)
    //   0x2b7 hunt        (lbz r0, 0x2b7 / stb r0, 0x2b7)
    //   0x2b8 aim_gun     (lbz r0, 0x2b8 in update_aim_gun)
    //   0x2bc move        (lwz r0, 0x2bc - full 4-byte enum, NOT a bitfield byte)
    //   0x2c0 follow      (lwz r0, 0x2c0 - full 4-byte enum)
    struct
    {
        bool updated;       // 0x2b4
        bool face_player;   // 0x2b5
        bool attacking;     // 0x2b6
        bool hunt;          // 0x2b7
        bool aim_gun;       // 0x2b8
        U8 _pad[3];         // 0x2b9-0x2bb (alignment to 0x2bc)
        move_enum move;     // 0x2bc - confirmed 4-byte lwz
        follow_enum follow; // 0x2c0 - confirmed 4-byte lwz
    } flag;

    mode_enum mode;         // 0x2c4 - confirmed: cmpwi r0, 0 -> MODE_BUDDY
    F32 delay;              // 0x2c8
    xQuat gun_tilt;         // 0x2cc (4 floats = 0x10 bytes)
    F32 ambush_delay;       // 0x2dc
    F32 stun_duration;      // 0x2e4 (NOTE: beam_duration not confirmed, stfs at 0x2dc = ambush_delay)

    // 0x2e8: confirmed xDecalEmitter (init'd with li r4, 0x7f, string "Plankton's Beam Rings")
    xDecalEmitter beam_ring;  // 0x2e8

    // 0x350: confirmed xDecalEmitter (init'd with li r4, 7, string "Plankton's Beam Glow")
    xDecalEmitter beam_glow;  // 0x350

    // 0x3b8: confirmed xLaserBoltEmitter (init'd with li r4, 0x1f, string "Plankton's Beam")
    xLaserBoltEmitter beam;   // 0x3b8

    // 0x44c: confirmed stw r3, 0x44c(r30) in setup_beam after xParFindByName
    xParEmitter* beam_charge; // 0x44c

    // Orbit struct: center=0x450 (xVec3), radius=0x45c (F32)
    struct
    {
        xVec3 center;   // 0x450
        F32 radius;     // 0x45c
    } orbit;

    // Turn struct: confirmed offsets from reset_speed and update_aim_gun
    //   0x460 dir   (xVec2 - normalize call target in update_aim_gun flow)
    //   0x468 vel   (stfs f0, 0x468 in Reset)
    //   0x46c accel (stfs f0, 0x46c in reset_speed)
    //   0x470 max_vel (stfs f0, 0x470 in reset_speed)
    struct
    {
        xVec2 dir;      // 0x460
        F32 accel;      // 0x46c
        F32 max_vel;    // 0x470
    } turn;

    // Move info: confirmed addi r3, r31, 0x474 for dest, 0x48c for accel, 0x498 for max_vel
    move_info move;     // 0x474 (dest), 0x480 (vel), 0x48c (accel), 0x498 (max_vel)

    // Follow timers: confirmed lfs f0, 0x4a4 / lfs f0, 0x4a8 in update_follow_player
    struct
    {
    	F32 max_delay;  // 0x4a8
        F32 delay;      // 0x4a4
    } follow;

    // 0x4ac-0x4af: padding/alignment between follow and crony
    U8 _pad_4ac[4];     // 0x4ac

    // 0x4b0: confirmed lwz r3, 0x4b0(r3) in crony_attacking, take_control, give_control
    zNPCBoss* crony;    // 0x4b0

    // 0x4b4: confirmed addi r3, r31, 0x4b4 as territory[0] base
    // 8 slots * 0x3c = 0x1e0 bytes -> ends at 0x694
    territory_data territory[8]; // 0x4b4

    S32 territory_size;       // 0x694 - confirmed lwz r0, 0x694
    S32 active_territory;     // 0x698 - confirmed lwz r0, 0x698
    zNPCNewsFish* newsfish;   // 0x69c - confirmed lwz r3, 0x69c
    U32 old_player_health;    // 0x6a0 - confirmed lwz r5, 0x6a0
    U8 played_intro;          // 0x6a4 - confirmed stb r0, 0x6a4

    zNPCBPlankton(S32 myType);

    // Virtual overrides
    void Init(xEntAsset*);                                              // 0x801698C4
    void Setup();                                                       // 0x80169944
    void PostSetup();                                                   // 0x80169990
    void Reset();                                                       // 0x801699CC
    void Destroy();                                                     // 0x80169B44
    void Process(xScene*, F32);                                         // 0x80169B64
    S32 SysEvent(xBase*, xBase*, unsigned int, const F32*, xBase*, int*); // 0x80169D3C
    void Render();                                                      // 0x80169D64
    void RenderExtraPostParticles();                                    // 0x80169D98
    void ParseINI();                                                    // 0x80169DF0
    void ParseLinks();                                                  // 0x8016B570
    U32 AnimPick(int, en_NPC_GOAL_SPOT, xGoal*);                       // 0x8016B744
    void Damage(en_NPC_DAMAGE_TYPE, xBase*, const xVec3*);             // 0x8016B7E8
    void SelfSetup();                                                   // 0x8016B884

    // AI / movement
    S32 next_goal();                                                    // 0x8016B914
    void refresh_orbit();                                               // 0x8016B980
    void scan_cronies();                                                // 0x8016BB34
    void update_turn(F32);                                              // 0x8016C044
    void update_move(F32);                                              // 0x8016C148
    void reset_territories();                                           // 0x8016C3C4
    void update_dialog(F32);                                            // 0x8016C3FC
    void update_animation(F32);                                         // 0x8016C5E8 (blr only - nop)
    void update_follow(F32);                                            // 0x8016C5EC
    void update_follow_player(F32);                                     // 0x8016C630
    void update_follow_camera(F32);                                     // 0x8016C734
    void update_aim_gun(F32);                                           // 0x8016C85C

    // Beam
    void init_beam();                                                   // 0x8016CB94
    void setup_beam();                                                  // 0x8016CD60
    void reset_beam();                                                  // 0x8016CE90

    // Visibility
    void vanish();                                                      // 0x8016CEB4
    void reappear();                                                    // 0x8016CF0C

    // Combat / damage
    S32 check_player_damage();                                          // 0x8016C960
    void load_territory(S32, xBase&);                                  // 0x8016CA7C
    U32 crony_attacking() const;                                        // 0x8016D260
    void stun();                                                        // 0x8016D2B0
    S32 cronies_dead();                                           // 0x8016D440
    void impart_velocity(const xVec3*);                                 // 0x8016D4C4

    // Territory management
    void next_territory();                                              // 0x8016D5E0
    S32 have_cronies();                                           // 0x8016D638
    S32 move_to_player_territory();                                     // 0x8016D658
    S32 player_left_territory();                                  // 0x8016D6D4

    // Dialogue
    void say(int, int, bool);                                           // 0x8016D77C

    // Goal helpers
    void sickum();                                                      // 0x8016D824
    void here_boy();                                                    // 0x8016D87C
    void follow_player();                                               // 0x8016D888
    void follow_camera();                                               // 0x8016D8A8
    void reset_speed();                                                 // 0x8016D8C8
    void halt(F32);                                                     // 0x8016D958
    void fall(F32, F32);                                                // 0x8016DA34
    void aim_gun(xAnimPlay*, xQuat*, xVec3*, int);                     // 0x8016DAEC

    // Crony control
    void take_control();                                                // 0x8016FCFC
    void give_control();                                                // 0x80170014

    // Misc
    xVec3& location() const;
    void face_player();
    void render_debug();
    void enable_emitter(xParEmitter&) const;
    void disable_emitter(xParEmitter&) const;

    // Inline flags - confirmed return values from DOL stubs at end of give_control block:
    //   0x80170234: li r3, 0 -> ColChkFlags
    //   0x80170244: li r3, 0x10 -> ColChkByFlags / ColPenByFlags
    //   0x80170254: li r3, 3 -> PhysicsFlags
    //   0x80170260: li r3, 1 -> IsAlive
    U8 ColChkFlags() const;    // returns 0
    U8 ColPenFlags() const;    // returns 0
    U8 ColChkByFlags() const;  // returns 0x10
    U8 ColPenByFlags() const;  // returns 0x10
    U8 PhysicsFlags() const;   // returns 3
    S32 IsAlive();             // returns 1
};

struct zNPCGoalBPlanktonIdle : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonIdle(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);

    S32 get_yaw(F32&, F32&) const;
    S32 apply_yaw(F32);
};

struct zNPCGoalBPlanktonAttack : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonAttack(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonAmbush : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonAmbush(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonFlank : zNPCGoalCommon
{
    F32 accel;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonFlank(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonEvade : zNPCGoalCommon
{
    F32 evade_delay;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonEvade(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonHunt : zNPCGoalCommon
{
    // player_loc confirmed: Enter stores result of get_player_loc() here (12 bytes before owner)
    xVec3 player_loc;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonHunt(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonTaunt : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonTaunt(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonMove : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonMove(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonStun : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonStun(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonFall : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonFall(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonDizzy : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonDizzy(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonBeam : zNPCGoalCommon
{
    // Confirmed from DOL: emitted is compared as integer (cmpwi), substate drives dispatch
    // SS_WARM_UP=0, SS_FIRE=1, SS_COOL_DOWN=2, SS_DONE=3
    enum substate_enum
    {
        SS_WARM_UP,
        SS_FIRE,
        SS_COOL_DOWN,
        SS_DONE
    };

    F32 emitted;            // NOTE: stored/read as integer in repo cpp (emitted = 0),
                            // but beam.emit uses it as float count - keep F32 per repo
    substate_enum substate;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonBeam(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
    S32 update_warm_up(F32);
    S32 update_fire(F32);
    S32 update_cool_down(F32);
};

struct zNPCGoalBPlanktonWall : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonWall(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonMissle : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonMissle(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonBomb : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonBomb(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc) {}

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

xAnimTable* ZNPC_AnimTable_BossPlankton();

#endif
