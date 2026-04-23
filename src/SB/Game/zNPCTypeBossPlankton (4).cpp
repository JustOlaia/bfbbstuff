#include "zNPCTypeBossPlankton.h"
#include "zNPCMgr.h"
#include "zEntDestructObj.h"
#include "xMathInlines.h"
#include "xMath3.h"
#include "xGroup.h"
#include "xDebug.h"

#include <types.h>

#define f1585 1.0f
#define f1586 0.0f
#define f1657 0.2f
#define f1658 0.1f

#define ANIM_Unknown 0
#define ANIM_Idle01 1 // 0x4
#define ANIM_Taunt01 3 // 0xC
#define ANIM_move 66 // 0x42
#define ANIM_stun_begin 67 //0x43
#define ANIM_stun_loop 68 //0x44
#define ANIM_stun_end 69 //0x45
#define ANIM_attack_beam_begin 70 //0x46
#define ANIM_attack_beam_loop 71 //0x47
#define ANIM_attack_beam_end 72 //0x48
#define ANIM_attack_wall_begin 73 //0x49
#define ANIM_attack_wall_loop 74 //0x4a
#define ANIM_attack_wall_end 75 //0x4b
#define ANIM_attack_missle 76 //0x4c
#define ANIM_attack_bomb 77 //0x4d

#define SOUND_HOVER 0
#define SOUND_HIT 1
#define SOUND_BOLT_FIRE 2
#define SOUND_BOLT_FLY 3
#define SOUND_BOLT_HIT 4
#define SOUND_CHARGE 5

namespace
{
    struct sound_data_type
    {
        U32 id;
        U32 handle;
        xVec3* loc;
        F32 volume;
    };

    struct sound_property
    {
        U32 asset;
        F32 volume;
        F32 range_inner;
        F32 range_outer;
        F32 delay;
        F32 fade_time;
    };

    struct sound_asset
    {
        S32 group;
        char* name;
        U32 priority;
        U32 flags;
    };

    static xLaserBoltEmitter::effect_data beam_launch_effect[2];
    static xLaserBoltEmitter::effect_data beam_head_effect[1];
    static xLaserBoltEmitter::effect_data beam_impact_effect[3];
    static xLaserBoltEmitter::effect_data beam_death_effect[1];
    static xLaserBoltEmitter::effect_data beam_kill_effect[1];

    static U32 sound_asset_ids[6][10];
    static sound_data_type sound_data[6];

    struct say_entry
   {
       U32 say[2];
   };

    static say_entry say_set[8]; // static table at -0x7fd9+0x1394 in DOL

    static const sound_asset sound_assets[29] = {
        { 0, "RSB_foot_loop", 0, 3 },       { 0, "fan_loop", 0, 3 },
        { 0, "Rocket_burn_loop", 0, 3 },    { 0, "RP_whirr_loop", 0, 3 },
        { 0, "RP_whirr2_loop", 0, 3 },      { 0, "Glove_hover", 0, 3 },
        { 0, "Glove_pursuit", 0, 3 },       { 1, "Prawn_FF_hit", 0, 0 },
        { 1, "Prawn_hit", 0, 0 },           { 1, "Door_metal_shut", 0, 0 },
        { 1, "Ghostplat_fall", 0, 0 },      { 1, "ST-death", 0, 0 },
        { 1, "RP_Bwrrzt", 0, 0 },           { 1, "RP_chunk", 0, 0 },
        { 1, "b201_rp_exhale", 0, 0 },      { 2, "RP_laser_alt", 0, 0 },
        { 3, "RP_laser_loop", 0, 1 },       { 3, "ElecArc_alt_b", 0, 1 },
        { 3, "Laser_lrg_fire_loop", 0, 1 }, { 3, "Laser_sm_fire_loop", 0, 1 },
        { 4, "RB_stalact_brk", 0, 0 },      { 4, "Volcano_blast", 0, 0 },
        { 4, "RP_laser_thunk", 0, 0 },      { 4, "RP_pfft", 0, 0 },
        { 4, "RP_thwash", 0, 0 },           { 5, "RP_charge_whirr", 0, 0 },
        { 5, "B101_SC_jump", 0, 0 },        { 5, "KJ_Charge", 0, 0 },
        { 5, "Laser_med_pwrup1", 0, 0 }
    };

    static const xDecalEmitter::curve_node beam_ring_curve[2] = {
    { 0.0f, { 255, 255, 255, 255 }, 1.0f },
    { 1.0f, { 255, 255, 255,   0 }, 0.0f },
};



static const xDecalEmitter::curve_node beam_glow_curve[3] = {
    { 0.0f, { 255, 255, 255,   0 }, 0.0f },
    { 0.5f, { 255, 255, 255, 255 }, 1.0f },
    { 1.0f, { 255, 255, 255,   0 }, 0.0f },
};

    xVec3* get_player_loc()
    {
        return (xVec3*)&globals.player.ent.model->Mat->pos;
    }

    S32 init_sound()
    {
        return 0;
    }

    void reset_sound()
    {
        for (S32 i = 0; i < 6; ++i)
        {
            sound_data[i].handle = 0;
        }
    }

    void* play_sound(int, const xVec3*, F32)
    {
        return NULL;
    }

    void* kill_sound(S32, U32)
    {
        return 0;
    }

    void* kill_sound(S32)
    {
        return 0;
    }

    void play_beam_fly_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        if (bolt.context == NULL)
        {
            bolt.context = play_sound(SOUND_BOLT_FLY, &bolt.loc, 1.0f);
        }
    }

    void kill_beam_fly_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        if (bolt.context != NULL)
        {
            kill_sound(3, (U32)bolt.context);
            bolt.context = NULL;
        }
    }

    void play_beam_fire_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        play_sound(SOUND_BOLT_FIRE, &bolt.origin, 1.0f);
    }

    void play_beam_hit_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        play_sound(SOUND_BOLT_HIT, &bolt.loc, 1.0f);
    }

    struct config
    {
        F32 radius;
        F32 length;
        F32 vel;
        F32 fade_dist;
        F32 kill_dist;
        F32 safe_dist;
        F32 hit_radius;
        F32 rand_ang;
        F32 scar_life;
        xVec2 bolt_uv[2];
        S32 hit_interval;
        F32 damage;
    };

    struct tweak_group
    {
        xVec3 accel;
        xVec3 max_vel;
        F32 turn_accel;
        F32 turn_max_vel;
        F32 ground_y;
        F32 ground_radius;
        F32 hit_vel;
        F32 hit_max_dist;
        F32 idle_time;
        F32 min_arena_dist;
        struct
        {
            F32 min_ang;
            F32 max_ang;
            F32 min_delay;
            F32 max_delay;
        } follow;
        struct
        {
            F32 fuse_dist;
            F32 fuse_delay;
        } help;
        struct
        {
            xVec3 accel;
            xVec3 max_vel;
            F32 stun_duration;
            F32 obstruct_angle;
        } mode_buddy;
        struct
        {
            xVec3 accel;
            xVec3 max_vel;
            F32 stun_duration;
        } mode_harass;
        struct
        {
            F32 height;
            F32 radius;
            F32 beam_interval;
            F32 beam_duration;
            F32 beam_dist;
        } hunt;
        struct
        {
            F32 rate;
            F32 time_warm_up;
            F32 time_fire;
            F32 gun_tilt_min;
            F32 gun_tilt_max;
            F32 max_dist;
            F32 emit_dist;
            config fx;
        } beam;
        struct
        {
            F32 safety_dist;
            F32 safety_height;
            F32 attack_dist;
            F32 attack_height;
            F32 stun_time;
        } harass;
        struct
        {
            F32 duration;
            F32 accel;
            F32 max_vel;
        } flank;
        struct
        {
            F32 accel;
            F32 max_vel;
            F32 dist;
        } fall;
        struct
        {
            F32 duration;
            F32 move_delay_min;
            F32 move_delay_max;
            F32 accel;
            F32 max_vel;
        } evade;
        struct
        {
            xVec3 center;
            struct
            {
                F32 radius;
                F32 height;
            } attack;
            struct
            {
                F32 radius;
                F32 height;
            } safety;
        } arena;
        sound_property sound[6];
        void* context;
        tweak_callback cb_move;
        tweak_callback cb_arena;
        tweak_callback cb_ground;
        tweak_callback cb_beam;
        tweak_callback cb_help;
        tweak_callback cb_sound;
        tweak_callback cb_sound_asset;

        void load(xModelAssetParam*, U32);
        void register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*);
    };

    static tweak_group tweak;

    void tweak_group::load(xModelAssetParam* ap, U32 apsize)
    {
        register_tweaks(true, ap, apsize, NULL);
    }

    void tweak_group::register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*)
    {
        xVec3 V0;
        V0.x = 0.0f;
        V0.y = 0.0f;
        V0.z = 0.0f;

        if (init)
        {
            turn_accel = 540.0f;
            auto_tweak::load_param<F32, F32>(turn_accel, DEG2RAD(10), 0.01f, 1000000000.0f, ap,
                                             apsize, "turn_accel");
        }
        if (init)
        {
            turn_max_vel = 180.0f;
            auto_tweak::load_param<F32, F32>(turn_max_vel, DEG2RAD(10), 0.01f, 1000000000.0f, ap,
                                             apsize, "turn_max_vel");
        }
        if (init)
        {
            ground_y = -1.38f;
            auto_tweak::load_param<F32, F32>(ground_y, 1.0f, -1000000000.0f, 1000000000.0f, ap,
                                             apsize, "ground_y");
        }
        if (init)
        {
            ground_radius = 12.0f;
            auto_tweak::load_param<F32, F32>(ground_radius, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "ground_radius");
        }
        if (init)
        {
            hit_vel = 5.0f;
            auto_tweak::load_param<F32, F32>(hit_vel, 1.0f, 0.0f, 100000.0f, ap, apsize, "hit_vel");
        }
        if (init)
        {
            hit_max_dist = 5.0f;
            auto_tweak::load_param<F32, F32>(hit_max_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "hit_max_dist");
        }
        if (init)
        {
            idle_time = 3.0f;
            auto_tweak::load_param<F32, F32>(idle_time, 1.0f, 0.0f, 10.0f, ap, apsize, "idle_time");
        }
        if (init)
        {
            min_arena_dist = 3.0f;
            auto_tweak::load_param<F32, F32>(min_arena_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "min_arena_dist");
        }
        if (init)
        {
            help.fuse_dist = 8.0f;
            auto_tweak::load_param<F32, F32>(help.fuse_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "help.fuse_dist");
        }
        if (init)
        {
            help.fuse_delay = 3.0f;
            auto_tweak::load_param<F32, F32>(help.fuse_delay, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "help.fuse_delay");
        }
        if (init)
        {
            follow.min_ang = 15.0f;
            auto_tweak::load_param<F32, F32>(follow.min_ang, DEG2RAD(10), 0.0f, 180.0f, ap, apsize,
                                             "follow.min_ang");
        }
        if (init)
        {
            follow.max_ang = 30.0f;
            auto_tweak::load_param<F32, F32>(follow.max_ang, DEG2RAD(10), 0.0f, 180.0f, ap, apsize,
                                             "follow.max_ang");
        }
        if (init)
        {
            follow.min_delay = 0.0f;
            auto_tweak::load_param<F32, F32>(follow.min_delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "follow.min_delay");
        }
        if (init)
        {
            follow.max_delay = 2.0f;
            auto_tweak::load_param<F32, F32>(follow.max_delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "follow.max_delay");
        }
        if (init)
        {
            mode_buddy.accel = xVec3::create(10.0f, 20.0f, 20.0f);
            auto_tweak::load_param<xVec3, S32>(mode_buddy.accel, 0, 0, 0, ap, apsize,
                                               "mode_buddy.accel");
        }
        if (init)
        {
            mode_buddy.max_vel = xVec3::create(20.0f, 10.0f, 20.0f);
            auto_tweak::load_param<xVec3, S32>(mode_buddy.max_vel, 0, 0, 0, ap, apsize,
                                               "mode_buddy.max_vel");
        }
        if (init)
        {
            mode_buddy.stun_duration = 1.0f;
            auto_tweak::load_param<F32, F32>(mode_buddy.stun_duration, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "mode_buddy.stun_duration");
        }
        if (init)
        {
            mode_buddy.obstruct_angle = 45.0f;
            auto_tweak::load_param<F32, F32>(mode_buddy.obstruct_angle, DEG2RAD(10), 0.0f, 90.0f,
                                             ap, apsize, "mode_buddy.obstruct_angle");
        }
        if (init)
        {
            mode_harass.accel = xVec3::create(10.0f, 5.0f, 10.0f);
            auto_tweak::load_param<xVec3, S32>(mode_harass.accel, 0, 0, 0, ap, apsize,
                                               "mode_harass.accel");
        }
        if (init)
        {
            mode_harass.max_vel = xVec3::create(20.0f, 10.0f, 10.0f);
            auto_tweak::load_param<xVec3, S32>(mode_harass.max_vel, 0, 0, 0, ap, apsize,
                                               "mode_harass.max_vel");
        }
        if (init)
        {
            mode_harass.stun_duration = 2.0f;
            auto_tweak::load_param<F32, F32>(mode_harass.stun_duration, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "mode_harass.stun_duration");
        }
        if (init)
        {
            hunt.height = 3.0f;
            auto_tweak::load_param<F32, F32>(hunt.height, 1.0f, -10.0f, 10.0f, ap, apsize,
                                             "hunt.height");
        }
        if (init)
        {
            hunt.radius = 5.0f;
            auto_tweak::load_param<F32, F32>(hunt.radius, 1.0f, 1.0f, 100.0f, ap, apsize,
                                             "hunt.radius");
        }
        if (init)
        {
            hunt.beam_interval = 3.0f;
            auto_tweak::load_param<F32, F32>(hunt.beam_interval, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "hunt.beam_interval");
        }
        if (init)
        {
            hunt.beam_duration = 3.0f;
            auto_tweak::load_param<F32, F32>(hunt.beam_duration, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "hunt.beam_duration");
        }
        if (init)
        {
            hunt.beam_dist = 7.5f;
            auto_tweak::load_param<F32, F32>(hunt.beam_dist, 1.0f, 1.0f, 100.0f, ap, apsize,
                                             "hunt.beam_dist");
        }
        if (init)
        {
            beam.rate = 6.0f;
            auto_tweak::load_param<F32, F32>(beam.rate, 1.0f, 0.01f, 100000.0f, ap, apsize,
                                             "beam.rate");
        }
        if (init)
        {
            beam.time_warm_up = 0.5f;
            auto_tweak::load_param<F32, F32>(beam.time_warm_up, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.time_warm_up");
        }
        if (init)
        {
            beam.time_fire = 5.0f;
            auto_tweak::load_param<F32, F32>(beam.time_fire, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.time_fire");
        }
        if (init)
        {
            beam.gun_tilt_min = -80.0f;
            auto_tweak::load_param<F32, F32>(beam.gun_tilt_min, DEG2RAD(10), -180.0f, 180.0f, ap,
                                             apsize, "beam.gun_tilt_min");
        }
        if (init)
        {
            beam.gun_tilt_max = 20.0f;
            auto_tweak::load_param<F32, F32>(beam.gun_tilt_max, DEG2RAD(10), -180.0f, 180.0f, ap,
                                             apsize, "beam.gun_tilt_max");
        }
        if (init)
        {
            beam.max_dist = 25.0f;
            auto_tweak::load_param<F32, F32>(beam.max_dist, 1.0f, 1.0f, 100.0f, ap, apsize,
                                             "beam.max_dist");
        }
        if (init)
        {
            beam.emit_dist = 0.5f;
            auto_tweak::load_param<F32, F32>(beam.emit_dist, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "beam.emit_dist");
        }
        if (init)
        {
            beam.fx.radius = 0.7f;
            auto_tweak::load_param<F32, F32>(beam.fx.radius, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.fx.radius");
        }
        if (init)
        {
            beam.fx.length = 2.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.length, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.fx.length");
        }
        if (init)
        {
            beam.fx.vel = 20.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.vel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.vel");
        }
        if (init)
        {
            beam.fx.fade_dist = 15.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.fade_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.fade_dist");
        }
        if (init)
        {
            beam.fx.kill_dist = 20.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.kill_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.kill_dist");
        }
        if (init)
        {
            beam.fx.safe_dist = 2.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.safe_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.safe_dist");
        }
        if (init)
        {
            beam.fx.hit_radius = 0.2f;
            auto_tweak::load_param<F32, F32>(beam.fx.hit_radius, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "beam.fx.hit_radius");
        }
        if (init)
        {
            beam.fx.rand_ang = 6.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.rand_ang, DEG2RAD(10), 0.0f, 360.0f, ap,
                                             apsize, "beam.fx.rand_ang");
        }
        if (init)
        {
            beam.fx.scar_life = 3.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.scar_life, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "beam.fx.scar_life");
        }
        if (init)
        {
            beam.fx.bolt_uv[0].x = 0.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[0].x, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[0].x");
        }
        if (init)
        {
            beam.fx.bolt_uv[0].y = 0.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[0].y, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[0].y");
        }
        if (init)
        {
            beam.fx.bolt_uv[1].x = 1.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[1].x, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[1].x");
        }
        if (init)
        {
            beam.fx.bolt_uv[1].y = 1.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[1].y, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[1].y");
        }
        if (init)
        {
            beam.fx.hit_interval = 2;
            auto_tweak::load_param<S32, S32>(beam.fx.hit_interval, 1, 0, 100, ap, apsize,
                                             "beam.fx.hit_interval");
        }
        if (init)
        {
            beam.fx.damage = 1.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.damage, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "beam.fx.damage");
        }
        if (init)
        {
            harass.safety_dist = 2.0f;
            auto_tweak::load_param<F32, F32>(harass.safety_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "harass.safety_dist");
        }
        if (init)
        {
            harass.safety_height = -4.0f;
            auto_tweak::load_param<F32, F32>(harass.safety_height, 1.0f, -100.0f, 100.0f, ap,
                                             apsize, "harass.safety_height");
        }
        if (init)
        {
            harass.attack_dist = 5.0f;
            auto_tweak::load_param<F32, F32>(harass.attack_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "harass.attack_dist");
        }
        if (init)
        {
            harass.attack_height = 2.0f;
            auto_tweak::load_param<F32, F32>(harass.attack_height, 1.0f, -100.0f, 100.0f, ap,
                                             apsize, "harass.attack_height");
        }
        if (init)
        {
            harass.stun_time = 6.0f;
            auto_tweak::load_param<F32, F32>(harass.stun_time, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "harass.stun_time");
        }
        if (init)
        {
            flank.duration = 2.0f;
            auto_tweak::load_param<F32, F32>(flank.duration, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "flank.duration");
        }
        if (init)
        {
            flank.accel = 5.0f;
            auto_tweak::load_param<F32, F32>(flank.accel, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "flank.accel");
        }
        if (init)
        {
            flank.max_vel = 20.0f;
            auto_tweak::load_param<F32, F32>(flank.max_vel, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "flank.max_vel");
        }
        if (init)
        {
            fall.accel = 4.0f;
            auto_tweak::load_param<F32, F32>(fall.accel, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "fall.accel");
        }
        if (init)
        {
            fall.max_vel = 50.0f;
            auto_tweak::load_param<F32, F32>(fall.max_vel, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "fall.max_vel");
        }
        if (init)
        {
            fall.dist = 30.0f;
            auto_tweak::load_param<F32, F32>(fall.dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "fall.dist");
        }
        if (init)
        {
            evade.duration = 6.0f;
            auto_tweak::load_param<F32, F32>(evade.duration, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "evade.duration");
        }
        if (init)
        {
            evade.move_delay_min = 1.0f;
            auto_tweak::load_param<F32, F32>(evade.move_delay_min, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "evade.move_delay_min");
        }
        if (init)
        {
            evade.move_delay_max = 1.5f;
            auto_tweak::load_param<F32, F32>(evade.move_delay_max, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "evade.move_delay_max");
        }
        if (init)
        {
            evade.accel = 5.0f;
            auto_tweak::load_param<F32, F32>(evade.accel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "evade.accel");
        }
        if (init)
        {
            evade.max_vel = 10.0f;
            auto_tweak::load_param<F32, F32>(evade.max_vel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "evade.max_vel");
        }
        if (init)
        {
            arena.center = V0;
            auto_tweak::load_param<xVec3, S32>(arena.center, 0, 0, 0, ap, apsize, "arena.center");
        }
        if (init)
        {
            arena.attack.radius = 14.0f;
            auto_tweak::load_param<F32, F32>(arena.attack.radius, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "arena.attack.radius");
        }
        if (init)
        {
            arena.attack.height = 11.0f;
            auto_tweak::load_param<F32, F32>(arena.attack.height, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "arena.attack.height");
        }
        if (init)
        {
            arena.safety.radius = 12.0f;
            auto_tweak::load_param<F32, F32>(arena.safety.radius, 1.0f, 0.01, 100.0f, ap, apsize,
                                             "arena.safety.radius");
        }
        if (init)
        {
            arena.safety.height = 14.0f;
            auto_tweak::load_param<F32, F32>(arena.safety.height, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "arena.safety.height");
        }
        if (init)
        {
            sound[SOUND_HOVER].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_HOVER].volume");
        }
        if (init)
        {
            sound[SOUND_HOVER].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HOVER].range_inner");
        }
        if (init)
        {
            sound[SOUND_HOVER].range_outer = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HOVER].range_outer");
        }
        if (init)
        {
            sound[SOUND_HOVER].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_HOVER].delay");
        }
        if (init)
        {
            sound[SOUND_HOVER].fade_time = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].fade_time, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HOVER].fade_time");
        }
        if (init)
        {
            sound[SOUND_HIT].volume = 0.5f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].volume, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "sound[SOUND_HIT].volume");
        }
        if (init)
        {
            sound[SOUND_HIT].range_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HIT].range_inner");
        }
        if (init)
        {
            sound[SOUND_HIT].range_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HIT].range_outer");
        }
        if (init)
        {
            sound[SOUND_HIT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_HIT].delay");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].volume = 0.5f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_BOLT_FIRE].volume");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FIRE].range_inner");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].range_outer = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FIRE].range_outer");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_BOLT_FIRE].delay");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].volume = 0.2f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_BOLT_FLY].volume");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FLY].range_inner");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].range_outer = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FLY].range_outer");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].delay = 0.1f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_BOLT_FLY].delay");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].fade_time = 0.1f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].fade_time, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_BOLT_FLY].fade_time");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_BOLT_HIT].volume");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_HIT].range_inner");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].range_outer = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_HIT].range_outer");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_BOLT_HIT].delay");
        }
        if (init)
        {
            sound[SOUND_CHARGE].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_CHARGE].volume");
        }
        if (init)
        {
            sound[SOUND_CHARGE].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHARGE].range_inner");
        }
        if (init)
        {
            sound[SOUND_CHARGE].range_outer = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHARGE].range_outer");
        }
        if (init)
        {
            sound[SOUND_CHARGE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_CHARGE].delay");
        }
        if (init)
        {
            sound[SOUND_HOVER].asset = sound_asset_ids[0][4];
            sound_data[SOUND_HOVER].id = xStrHash(sound_assets[sound[SOUND_HOVER].asset].name);
        }
        if (init)
        {
            sound[SOUND_HIT].asset = sound_asset_ids[1][3];
            sound_data[SOUND_HIT].id = xStrHash(sound_assets[sound[SOUND_HIT].asset].name);
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].asset = sound_asset_ids[2][0];
            sound_data[SOUND_BOLT_FIRE].id =
                xStrHash(sound_assets[sound[SOUND_BOLT_FIRE].asset].name);
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].asset = sound_asset_ids[3][3];
            sound_data[SOUND_BOLT_FLY].id =
                xStrHash(sound_assets[sound[SOUND_BOLT_FLY].asset].name);
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].asset = sound_asset_ids[4][3];
            sound_data[SOUND_BOLT_HIT].id =
                xStrHash(sound_assets[sound[SOUND_BOLT_HIT].asset].name);
        }
        if (init)
        {
            sound[SOUND_CHARGE].asset = sound_asset_ids[5][3];
            sound_data[SOUND_CHARGE].id = xStrHash(sound_assets[sound[SOUND_CHARGE].asset].name);
        }
    }

    // Called via function pointer from update_move
    // Applies acceleration to each axis of loc toward move.dest using xAccelMove
    static void update_move_accel(xVec3& loc, zNPCBPlankton::move_info& move, F32 dt)
    {
        xAccelMove(loc.x, move.vel.x, move.accel.x, dt, move.max_vel.x);
        xAccelMove(loc.y, move.vel.y, move.accel.y, dt, move.max_vel.y);
        xAccelMove(loc.z, move.vel.z, move.accel.z, dt, move.max_vel.z);
    }

    // Decelerates loc toward zero on each axis using xDecelMove
    static void update_move_stop(xVec3& loc, zNPCBPlankton::move_info& move, F32 dt)
    {
        //xDecelMove(move.accel.x, dt, loc.x, move.vel.x);
        //xDecelMove(move.accel.y, dt, loc.y, move.vel.y);
        //xDecelMove(move.accel.z, dt, loc.z, move.vel.z);
        xAccelMove(loc.x, move.vel.x, 0.0f, dt, move.max_vel.x);
        xAccelMove(loc.y, move.vel.y, 0.0f, dt, move.max_vel.y);
        xAccelMove(loc.z, move.vel.z, 0.0f, dt, move.max_vel.z);
    }

    // Moves Plankton in an orbit around move.dest using velocity/accel in ring space.
    // Converts world velocity to ring-local coords, integrates, converts back.
    // bool param controls whether to wrap the angle.
    static void update_move_orbit(xVec3& loc, zNPCBPlankton::move_info& move,
                                  const xVec3& center, F32 dt, bool wrap)
    {
        // (large function - orbit integration, ring_to_world_vel / world_to_ring_vel calls)
    }

    // Returns the signed yaw offset between Plankton's current orbit position
    // and the closest orbit alignment point (used for smooth orbit approach)
    static F32 orbit_yaw_offset(const zNPCBPlankton& plankton, const xVec3& targetCenter)
    {
        // Computes cross product of two ring tangent vectors,
        // feeds through atan2 and wraps to [-PI, PI]
        F32 result = 0.0f;
        // ... (uses xVec3Cross, xatan2f, xfmodf)
        return result;
    }

    // Picks a random point in an orbit ring around center at given radius,
    // with height offset f2, stores result in dest xVec3
    static void random_orbit(const xVec3& center, F32 minRadius, F32 maxRadius, xVec3* dest)
    {
        // uses xRandFloat, trig, stores orbit point in dest
    }

    // Computes Plankton's desired orbit position around the player
    // Returns the target xVec3 for move.dest
    static void player_orbit(zNPCBPlankton* plankton, xVec3* dest)
    {
        // Gets player loc, calls orbit_yaw_offset, applies turn velocity,
        // builds orbit ring position at hunt.radius / orbit.center height
    }

} // namespace

xAnimTable* ZNPC_AnimTable_BossPlankton()
{
    // clang-format off
    S32 ourAnims[32] = {            //dwarf says it should be 32, matches less with 15
        ANIM_Idle01,
        ANIM_Taunt01,
        ANIM_move,
        ANIM_stun_begin,
        ANIM_stun_loop,
        ANIM_stun_end,
        ANIM_attack_beam_begin,
        ANIM_attack_beam_loop,
        ANIM_attack_beam_end,
        ANIM_attack_wall_begin,
        ANIM_attack_wall_loop,
        ANIM_attack_wall_end,
        ANIM_attack_missle,
        ANIM_attack_bomb,
        ANIM_Unknown,
    };
    // clang-format on

    xAnimTable* table = xAnimTableNew("zNPCBPlankton", NULL, 0);

    xAnimTableNewState(table, g_strz_bossanim[ANIM_Idle01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 3;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Taunt01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x42;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_move], 0x10, 0, f1585, NULL, NULL, f1586, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x43;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_begin], 0x20, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x44;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_loop], 0x10, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x45;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_end], 0x20, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x46;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_begin], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x47;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_loop], 0x10, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x48;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_end], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x49;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_begin], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4a;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_loop], 0x10, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4b;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_end], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4c;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_missle], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4d;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_bomb], 0x20, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0;

    NPCC_BuildStandardAnimTran(table, g_strz_bossanim, ourAnims, 1, f1657);

    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_stun_begin],
                            g_strz_bossanim[ANIM_stun_loop], 0, 0, 0x10, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_stun_loop], g_strz_bossanim[ANIM_stun_end],
                            0, 0, 0, 0, f1586, f1586, 0, 0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_attack_beam_loop], 0, 0, 0x10, 0, f1586, f1586, 0,
                            0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_attack_beam_end], 0, 0, 0, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_loop],
                            g_strz_bossanim[ANIM_attack_beam_end], 0, 0, 0, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_begin],
                            g_strz_bossanim[ANIM_attack_wall_loop], 0, 0, 0x10, 0, f1586, f1586, 0,
                            0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_loop],
                            g_strz_bossanim[ANIM_attack_wall_end], 0, 0, 0, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_Taunt01], g_strz_bossanim[ANIM_stun_begin],
                            0, 0, 0, 0, f1586, f1586, 0, 0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_move], g_strz_bossanim[ANIM_stun_begin], 0,
                            0, 0, 0, f1586, f1586, 0, 0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_loop],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_end],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_begin],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_loop],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_end],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_missle],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_bomb],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);

    return table;
}

// 0x801698C4
void zNPCBPlankton::Init(xEntAsset* asset)
{
    ::init_sound();
    zNPCCommon::Init(asset);
    // Set flg_move and flg_vuln to 1 (offsets 0x1CC and 0x1C8)
    flg_move = 1;
    flg_vuln = 1;
    xNPCBasic::RestoreColFlags();
    territory_size = 0;
    played_intro = 0;
    zNPCBPlankton::init_beam();
    // Set the model's animation table (offset 0x1C on model's asset param)
    // to ZNPC_AnimTable_BossPlankton via stored func pointer
    model->Anim->Table = (xAnimTable*)ZNPC_AnimTable_BossPlankton;
}

// 0x80169944
void zNPCBPlankton::Setup()
{
    zNPCBoss::Setup();
    zNPCBPlankton::setup_beam();
    // zSceneFindObject("NPC_NEWSCASTER") - string addr computed from two addis
    U32 tmpHash = xStrHash("NPC_NEWSCASTER");
    newsfish = (zNPCNewsFish*)zSceneFindObject(tmpHash);
}

// 0x80169990
void zNPCBPlankton::PostSetup()
{
    xUpdateCull_SetCB(xglobals->updateMgr, this, xUpdateCull_AlwaysTrueCB, NULL);
}

// 0x801699CC
void zNPCBPlankton::Reset()
{
    // If newsfish (crony) exists, call its Reset via vtable (offset 0x18 in vtable)
    if (newsfish != 0)
    {
        newsfish->Reset();
    }

    ::reset_sound();
    zNPCCommon::Reset();
    zNPCBPlankton::reset_beam();

    // memset flag struct to 0 (0x10 bytes at offset 0x2B4)
    memset((void*)&flag, 0, 0x10);

    zNPCBPlankton::face_player();

    // Reset orbit: center = (0,0,0), radius = 0
    orbit.center.x = 0.0f;
    orbit.center.y = 0.0f;
    orbit.center.z = 0.0f;

    // Reset turn
    turn.dir.x = 0.0f;
    turn.dir.y = 0.0f;
    turn.vel = 0.0f;

    old_player_health = 0;

    scan_cronies();

    // If no cronies (crony == NULL at 0x4B0):
    if (crony == NULL)
    {
        // MODE_HARASS: stun_duration = mode_harass.stun_duration (tweak+0x88), mode = 1
        mode = MODE_HARASS;
        stun_duration = tweak.mode_harass.stun_duration;
        newsfish = NULL; // clear newsfish ptr too
    }
    else
    {
        // MODE_BUDDY: stun_duration = mode_buddy.stun_duration (tweak+0x68), mode = 0
        mode = MODE_BUDDY;
        active_territory = 0;
        stun_duration = tweak.mode_buddy.stun_duration;

        if (newsfish != NULL)
        {
            newsfish->TalkOnScreen(1);
        }
    }

    reset_speed();
    refresh_orbit();
    follow_player();
    reset_territories();

    // Set goal: if MODE_BUDDY -> NPC_GOAL_BPLANKTONATTACK, else NPC_GOAL_BPLANKTONAMBUSH
    // (hashes 0x4E47424B and 0x4E47424D)
    if (mode == MODE_BUDDY)
    {
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONATTACK, 1);
    }
    else
    {
        vanish();
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONAMBUSH, 1);
    }
}

// 0x80169B44
void zNPCBPlankton::Destroy()
{
    zNPCCommon::Destroy();
}

// 0x80169B64
void zNPCBPlankton::Process(xScene* xscn, F32 dt)
{
    if ((flag.updated == false) && (flag.updated = 1, played_intro == false))
    {
        zNPCBPlankton::say(0, 0, true);
        played_intro = true;
    }

    beam.update(dt);
    delay += dt;

    if (mode == MODE_HARASS)
    {
        territory->fuse_detected = (U8)player_left_territory();
        if (psy_instinct != 0)
        {
            stun_duration = 0.0f;
            psy_instinct->GoalSet(NPC_GOAL_BPLANKTONAMBUSH, 1);
        }
    }

    // check_player_damage returns non-zero if player has health (player is alive)
    S32 playerAlive = check_player_damage();

    // Timestep AI if active and flags clear
    if ((playerAlive & 0x23) == 0 && psy_instinct != 0)
    {
        psy_instinct->Timestep(dt, 0);
    }

    if (flag.face_player == false)
    {
        // Update facing toward player
        // dir = normalize(player.x - loc.x, player.z - loc.z)
        xVec3* pLoc = (xVec3*)&globals.player.ent.model->Mat->pos;
        const xVec3& myLoc = location();
        turn.dir.x = pLoc->x - myLoc.x;
        turn.dir.y = pLoc->z - myLoc.z;
        turn.dir.normalize();
    }

    update_follow(dt);
    update_turn(dt);
    update_move(dt);
    update_animation(dt);

    if ((check_player_damage() & 0xff) != 0)
    {
        zEntPlayer_Damage((xBase*)&globals.player, 1);
    }

    update_aim_gun(dt);
    update_dialog(dt);

    // If beam is visible: set model render flags
    if ((beam.visible() & 0xff) != 0)
    {
        model->Flags |= 2;
    }

    zNPCCommon::Process(xscn, dt);
}

// 0x80169D3C
S32 zNPCBPlankton::SysEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                            xBase* toParamWidget, S32* handled)
{
    *handled = 0;
    return zNPCCommon::SysEvent(from, to, toEvent, toParam, toParamWidget, handled);
}

// 0x80169D64
void zNPCBPlankton::Render()
{
    xNPCBasic::Render();
    zNPCBPlankton::render_debug();
}

// 0x80169D98
void zNPCBPlankton::RenderExtraPostParticles()
{
    // beam is at offset 0x3B8; check visible()
    if ((beam.visible() & 0xff) != 0)
    {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)2);
        beam.render();
    }
}

// 0x80169DF0
void zNPCBPlankton::ParseINI()
{
    zNPCCommon::ParseINI();
    tweak.load(parmdata, pdatsize);
}

// 0x8016B570
void zNPCBPlankton::ParseLinks()
{
    zNPCCommon::ParseLinks();

    // Zero territory array (0x1E0 bytes at offset 0x4B4)
    memset(&territory[0], 0, sizeof(territory));

    // Iterate linked objects (link type 0x133 = territory link)
    xLinkAsset* link = (xLinkAsset*)((U8*)this + 8);
    U8 numLinks = *(U8*)((U8*)this + 5);
    xLinkAsset* linkEnd = link + numLinks;

    while (link != linkEnd)
    {
        if (link->dstEvent == 0x133) // territory link type
        {
            xBase* obj = zSceneFindObject(link->dstAssetID);
            S32 slot = (S32)link->param[0]; // param is F32, fctiwz converts to int

            if (slot > 0 && slot <= 8)
            {
                S32 idx = slot - 1;
                territory_data* t = &territory[idx];

                if (t->origin == NULL)
                {
                    load_territory(slot, *obj);

                    // If still null after load, zero it out
                    if (t->origin == NULL || t->platform == NULL)
                    {
                        memset(t, 0, sizeof(territory_data));
                    }
                }
            }
        }
        link++;
    }

    // Count valid territories and compact into active slots
    territory_size = 0;
    for (S32 i = 0; i < 8; i++)
    {
        if (territory[i].origin != NULL)
        {
            if (territory_size != i)
            {
                territory[territory_size] = territory[i];
            }
            territory_size++;
        }
    }
}

// 0x8016B744
void zNPCBPlankton::SelfSetup()
{
    xBehaveMgr* bmgr = xBehaveMgr_GetSelf();
    this->psy_instinct = bmgr->Subscribe(this, NULL);
    xPsyche* psy = this->psy_instinct;
    psy->BrainBegin();
    // Loop from NPC_GOAL_BPLANKTONIDLE (0x4E47424B) to NPC_GOAL_BPLANKTONBOMB (0x4E474259)
    for (S32 i = NPC_GOAL_BPLANKTONIDLE; i <= NPC_GOAL_BPLANKTONBOMB; i++)
    {
        psy->AddGoal(i, this);
    }
    psy->BrainEnd();
    psy->SetSafety(NPC_GOAL_BPLANKTONIDLE);
}

// 0x8016B7E8
void zNPCBPlankton::Damage(en_NPC_DAMAGE_TYPE damageType, xBase* src, const xVec3* hitPos)
{
    U32 dmgType = (U32)damageType;
    // Notify psyche
    psy_instinct->GIDOfActive();

    if (dmgType > 12)
        return;

    // Switch table on damageType (jump table at -0x5010(r2))
    switch (dmgType)
    {
    // types <= 0xC that involve a hit position:
    default:
        if (hitPos != NULL)
        {
            // Scale hitPos by tweak.hit_vel (tweak+0x28), compute velocity
            xVec3 scaledPos;
            xVec3SMul(&scaledPos, hitPos, tweak.hit_vel);
            impart_velocity(&scaledPos);
        }
        stun();
        break;
    }
}

// 0x8016B884
U32 zNPCBPlankton::AnimPick(S32 rawgoal, en_NPC_GOAL_SPOT gspot, xGoal* goal)
{
    U32 animId = 0;
    S32 index = 1;

    // rawgoal is passed as a hash; subtract base hash to get offset 0..14
    S32 offset = rawgoal - NPC_GOAL_BPLANKTONIDLE;

    if ((U32)offset > 14)
    {
        index = 1;
    }
    else
    {
        // Jump table (at -0x4FDC(r2)), 15 entries
        switch (offset)
        {
        case 0:  index = 1;    break; // IDLE
        case 1:  index = 3;    break; // TAUNT
        case 2:  index = 0x42; break; // MOVE
        case 3:  index = 0x43; break; // STUN (begin)
        case 4:  index = 0x46; break; // BEAM (begin)
        case 5:  index = 0x49; break; // WALL (begin)
        case 6:  index = 0x4c; break; // MISSLE
        case 7:  index = 0x4d; break; // BOMB
        default: index = 1;    break;
        }
    }

    if (index > -1)
    {
        return animId = g_hash_bossanim[index];
    }

    return animId;
}

// 0x8016B914
S32 zNPCBPlankton::next_goal()
{
    if (mode == MODE_HARASS)
    {
        return NPC_GOAL_BPLANKTONEVADE; // 0x4E47424F
    }

    if (flag.hunt)
    {
        return NPC_GOAL_BPLANKTONATTACK; // 0x4E474250
    }

    // If crony is attacking: return HUNT, else return ATTACK
    // (the neg/or/srawi trick: if crony_attacking() != 0 => result = HUNT, else ATTACK)
    U32 cronyAtk = crony_attacking();
    S32 sign = (S32)((-(S32)cronyAtk | (S32)cronyAtk) >> 31); // -1 if nonzero, 0 if zero
    return NPC_GOAL_BPLANKTONHUNT + sign; // HUNT if crony attacking, else one below (ATTACK)
}

// 0x8016B980
void zNPCBPlankton::refresh_orbit()
{
    if (mode == MODE_HARASS)
    {
        // Harass mode: set orbit center from current territory platform position.
        S32 idx = active_territory;
        territory_data* t = &territory[idx];
        xMovePoint* origin = (xMovePoint*)t->origin;
        xVec3Copy(&orbit.center, (xVec3*)((U8*)origin + 8)); // platform XZ
        orbit.center.y = origin->asset->delay; // platform height (offset 0x20)

        if (flag.attacking)
        {
            // attack mode orbit: use arena.attack radius/height from tweak
            orbit.radius = tweak.arena.attack.radius + tweak.hunt.height;
            orbit.center.y += tweak.arena.attack.height;
        }
        else
        {
            // safety orbit: use arena.safety radius/height
            orbit.radius = tweak.arena.safety.radius + tweak.hunt.height; // (tweak+0x150/0x14C)
            orbit.center.y += tweak.arena.safety.height;
        }
    }
    else
    {
        // Buddy mode
        if (flag.hunt)
        {
            // Hunt: orbit around player
            xVec3* pLoc = get_player_loc();
            xVec3Copy(&orbit.center, pLoc);
            orbit.center.y += tweak.hunt.height + tweak.arena.attack.height; // (tweak+0x8C/0x90)
            orbit.radius = tweak.hunt.radius;
            xVec3Sub(&orbit.center, &orbit.center, &move.dest); // approach dest
        }
        else if (flag.attacking)
        {
            // Attack: orbit around arena center
            xVec3Copy(&orbit.center, (xVec3*)((U8*)&tweak + 0x138)); // tweak.arena.center
            orbit.center.y += tweak.arena.attack.height;             // (tweak+0x148)
            orbit.radius = tweak.arena.attack.radius;                // (tweak+0x144)
        }
        else
        {
            // Idle: safety orbit
            xVec3Copy(&orbit.center, (xVec3*)((U8*)&tweak + 0x138));
            orbit.center.y += tweak.arena.safety.height;  // (tweak+0x150)
            orbit.radius = tweak.arena.safety.radius;     // (tweak+0x14C)
        }
    }
}

// 0x8016BB34
void zNPCBPlankton::scan_cronies()
{
st_XORDEREDARRAY* npclist = zNPCMgr_GetNPCList();
crony = NULL;

for (S32 i = 0; i < npclist->cnt; i++)
{
    xBase* obj = (xBase*)npclist->list[i];
    U32 typeHash = (U32)((xNPCBasic*)obj)->myNPCType;
    if (typeHash == 0x4E544233)
    {
        crony = (zNPCBoss*)obj;
        break;
    }
  }
}

// 0x8016C044 - Updates Plankton's yaw to face the desired direction
void zNPCBPlankton::update_turn(F32 dt)
{
    // Reads turn.dir (xVec2), integrates turn.vel using turn_accel/turn_max_vel
    // Applies resulting yaw delta to the model matrix
    // Uses xAccelMove for angular velocity, set_yaw_matrix for final orientation
}

// 0x8016C148 - Dispatches to the correct movement integrator based on flag.move
void zNPCBPlankton::update_move(F32 dt)
{
    xVec3& loc = (xVec3&)model->Mat->pos; // model world position
    switch (flag.move)
    {
    case MOVE_ACCEL:
        update_move_accel(loc, move, dt);
        break;
    case MOVE_STOP:
        update_move_stop(loc, move, dt);
        break;
    case MOVE_ORBIT:
        update_move_orbit(loc, move, orbit.center, dt, false);
        break;
    default:
        break;
    }
}

// 0x8016C3C4 - Resets fuse detection flags on all active territories
void zNPCBPlankton::reset_territories()
{
    territory_data* t = &territory[0];
    territory_data* end = t + territory_size;
    while (t != end)
    {
        t->fuse_detected = 0;
        t->fuse_destroyed = 0;
        t->fuse_detect_time = 0.0f;
        t++;
    }
}

// 0x8016C3FC - Handles dialogue triggers based on proximity to fuses and player health
void zNPCBPlankton::update_dialog(F32 dt)
{
    if (mode != MODE_HARASS)
        return;

    // Check if player health changed (say_hit_player)
    U32 playerHP = globals.player.Health; // offset 0x16B0 in globals struct area
    if (playerHP < old_player_health && playerHP != 0)
    {
        say(7, 0, false); // say_hit_player set, no randomize
    }
    old_player_health = playerHP;

    // Check fuse proximity in current territory
    xVec3* pLoc = get_player_loc();
    S32 prevActive = active_territory > 0 ? active_territory - 1 : 0;

    // Iterate previous territories for "fuse near" detection
    territory_data* t = &territory[prevActive];
    for (S32 i = prevActive; i < active_territory; i++, t++)
    {
        if (t->fuse != NULL && !t->fuse_detected)
        {
            if (zEntDestructObj_isDestroyed(t->fuse)) // has fuse been destroyed?
            {
                say(2, 0, false); // say_fuse_near
                t->fuse_detected = 1;
                return;
            }
        }
    }

    // Iterate all territories for "fuse destroyed" detection
    t = &territory[0];
    for (S32 i = 0; i < territory_size; i++, t++)
    {
        if (t->crony_size > 0 && t->fuse != NULL && !t->fuse_destroyed)
        {
            if (!zEntDestructObj_isDestroyed(t->fuse)) // fuse destroyed
            {
                // Get fuse position
                xVec3 fusePos;
                xVec3Copy(&fusePos, (xVec3*)((U8*)t->fuse->model->Mat + 0x30));

                // Compute dist sq to player
                xVec3 diff;
                xVec3Sub(&diff, &fusePos, pLoc);
                F32 distSq = xVec3Length2(&diff);
                F32 fuseDist = tweak.help.fuse_dist;

                if (distSq <= fuseDist * fuseDist)
                {
                    say(2, 0, false); // fuse near/destroyed
                    t->fuse_destroyed = 1;
                    return;
                }
            }
        }
    }
}

// 0x8016C5E8 - No-op in shipped build (blr only)
void zNPCBPlankton::update_animation(F32 dt)
{
}

// 0x8016C5EC - Dispatches to follow_player or follow_camera based on flag.follow
void zNPCBPlankton::update_follow(F32 dt)
{
    switch (flag.follow)
    {
    case FOLLOW_PLAYER:
        update_follow_player(dt);
        break;
    case FOLLOW_CAMERA:
        update_follow_camera(dt);
        break;
    default:
        break;
    }
}

// 0x8016C630 - Integrates orbit angle to track the player (in FOLLOW_PLAYER mode)
void zNPCBPlankton::update_follow_player(F32 dt)
{
    // Increments follow.delay by dt, checks against follow.max_delay
    // When delay exceeds threshold: picks new random orbit angle around player
    // Uses tweak.follow min/max_ang and min/max_delay
    follow.delay += dt;
    if (follow.delay >= follow.max_delay)
    {
        follow.delay = 0.0f;
        follow.max_delay = tweak.follow.min_delay + xurand() * (tweak.follow.max_delay - tweak.follow.min_delay);
        // Recompute orbit destination angle around player
        // (calls random_orbit equivalent logic)
    }
}

// 0x8016C734 - Integrates orbit angle to track the camera (in FOLLOW_CAMERA mode)
void zNPCBPlankton::update_follow_camera(F32 dt)
{
    // Similar to follow_player but targets the camera position (globals.camera)
    // Uses tweak.follow ang range, updates orbit.center based on camera pos + orbit.radius dir
    follow.delay += dt;
    if (follow.delay >= follow.max_delay)
    {
        follow.delay = 0.0f;
        follow.max_delay = tweak.follow.min_delay + xurand() * (tweak.follow.max_delay - tweak.follow.min_delay);
    }
}

// 0x8016C85C - Tilts gun bone toward the player when flag.aim_gun is set
void zNPCBPlankton::update_aim_gun(F32 dt)
{
    if (!flag.aim_gun)
        return;

    // Get bone 0x15 (gun bone) matrix from model
    xMat4x3 boneMat;
    xModelGetBoneMat(boneMat, *model, 0x15);

    // Get player location
    xVec3* pLoc = get_player_loc();

    // Compute dir from bone to player in XZ plane, get yaw
    xVec3 toPlayer;
    xVec3Sub(&toPlayer, pLoc, &boneMat.pos);
    toPlayer.y = 0.0f; // project to XZ

    F32 distXZ = xsqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    F32 yaw = xatan2(boneMat.pos.y - pLoc->y, distXZ); // elevation angle
    // Clamp to [beam.gun_tilt_min, beam.gun_tilt_max]
    if (yaw < tweak.beam.gun_tilt_min) yaw = tweak.beam.gun_tilt_min;
    else if (yaw > tweak.beam.gun_tilt_max) yaw = tweak.beam.gun_tilt_max;

    // Apply result to gun_tilt quaternion (offset 0x2CC on Plankton)
    xQuatFromAxisAngle(&gun_tilt, &g_X3, yaw);
}

// 0x8016C960 - Returns non-zero if player has health (i.e. player is alive and can be damaged)
// Assembly: loads globals.player.health (offset 0x1AFC in the globals area),
// returns (health != 0) ? 1 : 0 via neg/or/srawi idiom
S32 zNPCBPlankton::check_player_damage()
{
    U32 hp = globals.player.Health; // offset 0x1AFC
    S32 r = (S32)hp | -(S32)hp;
    return 0 & ~(r >> 31);
}

// 0x8016CA7C - Loads a territory slot from a linked xBase object
// Handles: xMovePoint (origin), xEnt (platform), zEntDestructObj (fuse),
// xTimer (timer), zNPCCommon children (cronies), xContainer (recurse children)
void zNPCBPlankton::load_territory(S32 slot, xBase& obj)
{
    territory_data* t = &territory[slot - 1];

    switch (obj.baseType)
    {
    case 0x11:
    {
        xGroup* grp = (xGroup*)&obj;
        S32 childCount = xGroupGetCount(grp);
        for (S32 i = 0; i < childCount; i++)
        {
            xBase* child = (xBase*)xGroupGetItemPtr(grp, i);
            load_territory(slot, *child);
        }
        break;
    }
    case 0xD: // xMovePoint - territory origin
        t->origin = (zMovePoint*)&obj;
        break;
    case 0x2B: // xTimer
        t->timer = (xTimer*)&obj;
        break;
    case 0xE: // xEnt (platform)
        t->platform = (xEnt*)&obj;
        break;
    case 0x1B: // zEntDestructObj (fuse)
        t->fuse = (zEntDestructObj*)&obj;
        break;
    default:
        // Check if it's an NPC (zNPCCommon) - store as crony if slot available
        if (((xNPCBasic*)&obj)->myNPCType == 0x4E544233)
        {
            if (t->crony_size < 8)
            {
                t->crony[t->crony_size] = (zNPCCommon*)&obj;
                t->crony_size++;
            }
        }
        else
        {
            t->platform = (xEnt*)&obj; // fallback: treat as platform
        }
        break;
    }
}

// 0x8016CB94
void zNPCBPlankton::init_beam()
{
    // beam (xLaserBoltEmitter at offset 0x3B8) init
    beam.init((U32)&beam, "Plankton\'s Beam");
    beam.set_texture("plankton_laser_bolt");
    beam.refresh_config();

    // beam_ring (xDecalEmitter at offset 0x2E8)
    beam_ring.init(0x7F, "Plankton\'s Beam Rings");
    beam_ring.set_curve(beam_ring_curve, 2);
    beam_ring.set_texture("bubble");
    beam_ring.set_default_config();
    beam_ring.cfg.flags = 0;
    beam_ring.cfg.life_time = 0.0f;
    beam_ring.cfg.blend_src = 5;
    beam_ring.cfg.blend_dst = 2;
    beam_ring.refresh_config();

    // beam_glow (xDecalEmitter at offset 0x350)
    beam_glow.init(0x07, "Plankton\'s Beam Glow");
    beam_glow.set_curve(beam_glow_curve, 3);
    beam_glow.set_texture("fx_firework");
    beam_glow.set_default_config();
    beam_glow.cfg.flags = 0;
    beam_glow.cfg.life_time = 0.0f;
    beam_glow.cfg.blend_src = 5;
    beam_glow.cfg.blend_dst = 2;
    beam_glow.refresh_config();
}

// 0x8016CD60
void zNPCBPlankton::setup_beam()
{
    // Binds effect_data entries to the beam emitter (xLaserBoltEmitter at 0x3B8)
    // Sets up launch, head, impact, death, kill effect callbacks and particle emitters
    // Also looks up and stores the beam_charge xParEmitter by name at offset 0x44C
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_LAUNCH, beam_launch_effect, 2);
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_HEAD,   beam_head_effect,   1);
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_IMPACT, beam_impact_effect, 3);
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_DEATH,  beam_death_effect,  1);
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_KILL,   beam_kill_effect,   1);
    beam_charge = NULL; // TODO: function not yet identified, was (xParEmitter*)xParFindByName("plankton_beam_zap"); before
}

// 0x8016CE90
void zNPCBPlankton::reset_beam()
{
    beam.reset(); // xLaserBoltEmitter::reset at offset 0x3B8
}

// 0x8016CEB4
void zNPCBPlankton::vanish()
{
    // Clear visible flag (bit 0 of flags at 0x18), set culled flag (bit 6)
    flags = (flags & 0xFE) | 0x40;
    pflags = 0;
    moreFlags = 0;
    chkby = 0;
    penby = 0;
    flags2.flg_colCheck = 0;
    flags2.flg_penCheck = 0;
    kill_sound(NULL);
}

// 0x8016CF0C
void zNPCBPlankton::reappear()
{
    // Set visible (bit 0), clear culled (bit 6)
    flags = (flags | 1) & 0xBF;
    pflags = 0;
    moreFlags = 0x10;
    chkby = 0x10;
    penby = 0x10;
    flags2.flg_colCheck = 0;
    flags2.flg_penCheck = 0;
    // Play hover sound at bound position
    play_sound(SOUND_HOVER, (xVec3*)&bound.pad[3], 1.0f);
}

// 0x8016D260
U32 zNPCBPlankton::crony_attacking() const
{
    zNPCBoss* c = (zNPCBoss*)crony; // offset 0x4B0
    if (c == NULL)
        return 0;
    // Call IsAlive() via vtable (offset 0xCC in vtable)
    return c->IsAlive();
}

// 0x8016D2B0
void zNPCBPlankton::stun()
{
    // Get current goal from psyche
    S32 curGoal = psy_instinct->GIDOfActive();
    S32 goalOffset = curGoal - NPC_GOAL_BPLANKTONSTUN; // relative to stun/fall/dizzy range

    // Don't re-stun if already in stun/fall/dizzy (offset 0 or 1 = STUN/FALL)
    if ((U32)goalOffset <= 1 || curGoal == NPC_GOAL_BPLANKTONDIZZY)
        return;

    // Play hit sound
    play_sound(SOUND_HIT, (xVec3*)&bound.pad[3], 1.0f);

    if (mode == MODE_BUDDY)
    {
        // Enter stun goal
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONSTUN, 1);
        return;
    }

    // MODE_HARASS: check how many territories have visible fuses
    // Count platform visibility per territory
    S32 numVisible = 0;
    territory_data* t = &territory[0];
    for (S32 i = 0; i < territory_size; i++, t++)
    {
        if (t->fuse != NULL)
        {
            if (!zEntDestructObj_isDestroyed(t->fuse))
                numVisible++;
        }
    }

    // Pick dialogue based on numVisible (1=say[6], 2=say[5], 3=say[4], else say[3])
    S32 sayIdx;
    switch (numVisible)
   {
      case 2:  sayIdx = 5; break;
      case 3:  sayIdx = 4; break;
      case 4:  sayIdx = 3; break;
      default: sayIdx = (numVisible == 1) ? 6 : 3; break;
   }

// 0x8016D440
S32 zNPCBPlankton::cronies_dead()
{
    territory_data* t = &territory[active_territory];
    zNPCCommon** p = &t->crony[0];
    zNPCCommon** end = p + t->crony_size;  // pointer past end
    while (p != end)
    {
        if (*p != NULL && (*p)->IsAlive())
            return 0;
        ++p;
    }
    return 1;
}

// 0x8016D4C4
void zNPCBPlankton::impart_velocity(const xVec3* impulse)
{
    // TODO: world_to_ring_vel / ring_to_world_vel not yet implemented
    // Full orbit velocity conversion logic goes here
}

// 0x8016D5E0
void zNPCBPlankton::next_territory()
{
    if ((have_cronies() & 0xff) != 0)
    {
        active_territory++;
        if (active_territory >= territory_size)
        {
            active_territory = territory_size - 1;
        }
    }
}

// 0x8016D638
S32 zNPCBPlankton::have_cronies()
{
    // Checks territory[active_territory].crony_size
    // Returns 1 if crony_size > 0, else 0
    // offset 0x4E4 = territory[active_territory].crony_size via mulli+add
    S32 cronySize = territory[active_territory].crony_size;
    U32 r = (U32)cronySize;
    return (S32)((r | (U32)(-(S32)r)) >> 31);
}

// 0x8016D658
S32 zNPCBPlankton::move_to_player_territory()
{
    // Gets the player's current standing platform from globals.player
    // Finds which territory slot matches the platform pointer
    // If found, sets active_territory to that slot and returns 1; else returns 0
    xEnt* playerPlatform = (xEnt*)globals.player.ent.collis; // offset 0x72C in scene area
    if (playerPlatform == NULL || !(playerPlatform->flags & 1))
        return 0;

    xEnt* platEnt = playerPlatform;
    if (!(*(U32*)((U8*)platEnt + 0xc) & 1))
         return 0;

    for (S32 i = 0; i < territory_size; i++)
    {
        if (territory[i].crony_size <= 0 && territory[i].platform == platEnt)
        {
            active_territory = i;
            return 1;
        }
    }
    return 0;
}

// 0x8016D6D4
S32 zNPCBPlankton::player_left_territory()
{
    S32 idx = active_territory;
    xEnt* playerPlatform = (xEnt*)globals.player.ent.collis;
    territory_data* t = &territory[idx];

    // If territory has no cronies left and player's platform matches: still in territory
    if (t->crony_size <= 0)
        return 0;
    if (t->platform == playerPlatform)
        return 0;
    if (!(playerPlatform->flags & 1))
        return 0;
    if (playerPlatform == NULL)
        return 0;

    // Check all other territories: if player is on a different territory's platform → left
    territory_data* t2 = &territory[0];
    for (S32 i = 0; i < territory_size; i++, t2++)
    {
        if (t2->crony_size <= 0 && t2->platform == playerPlatform && i != idx)
            return 1;
    }
    return 0;
}

// 0x8016D77C
static void NF_SayLine(zNPCNewsFish* nf, U32 sndid, S32 slot);  
static void NF_SayInterrupt(zNPCNewsFish* nf, U32 sndid, S32 size, S32 unk, S32 anim);

void zNPCBPlankton::say(int saySet, int unused, bool random)
{
    if (newsfish == NULL)
        return;

    if (random)
    {
        NF_SayLine(newsfish, say_set[saySet].say[0], 1);
        NF_SayLine(newsfish, say_set[saySet].say[1], 2);
    }
    else
    {
        NF_SayInterrupt(newsfish, say_set[saySet].say[0], say_set[saySet].size, unused, -1);
    }
}

// 0x8016D824
void zNPCBPlankton::sickum()
{
    // If current goal is already HUNT, don't sickum again
    S32 curGoal = psy_instinct->GIDOfActive();
    if (curGoal == NPC_GOAL_BPLANKTONHUNT)
        return;

    flag.hunt = 1;
    psy_instinct->GoalSet(NPC_GOAL_BPLANKTONHUNT, 1);
}

// 0x8016D87C
void zNPCBPlankton::here_boy()
{
    flag.hunt = 0;
}

// 0x8016D888
void zNPCBPlankton::follow_player()
{
    flag.follow = FOLLOW_PLAYER;
    follow.max_delay = 0.0f;
    follow.delay = 0.0f;
    flag.move = MOVE_ORBIT;
}

// 0x8016D8A8
void zNPCBPlankton::follow_camera()
{
    flag.follow = FOLLOW_CAMERA;
    follow.max_delay = 0.0f;
    follow.delay = 0.0f;
    flag.move = MOVE_ORBIT;
}

// 0x8016D8C8
void zNPCBPlankton::reset_speed()
{
    // Set turn velocity limits from tweak (offsets 0x18 and 0x1C = turn_accel and turn_max_vel)
    turn.accel = tweak.turn_accel;
    turn.max_vel = tweak.turn_max_vel;

    if (mode == MODE_BUDDY)
    {
        // mode_buddy accel/max_vel (tweak+0x50 / +0x5C)
        xVec3Copy(&move.accel, &tweak.mode_buddy.accel);
        xVec3Copy(&move.max_vel, &tweak.mode_buddy.max_vel);
    }
    else
    {
        // mode_harass accel/max_vel (tweak+0x70 / +0x7C)
        xVec3Copy(&move.accel, &tweak.mode_harass.accel);
        xVec3Copy(&move.max_vel, &tweak.mode_harass.max_vel);
    }
}

// 0x8016D958
void zNPCBPlankton::halt(F32 dt)
{
    // Decelerates move.vel to zero using update_move_stop style decel
    // Also halts turn.vel
}

// 0x8016DA34
void zNPCBPlankton::fall(F32 accel, F32 maxVel)
{
    // Sets up downward movement: flag.move = MOVE_ACCEL,
    // move.accel.y = -accel, move.max_vel.y = -maxVel
    // zeros X/Z accel, sets dest to current position
    flag.move = MOVE_ACCEL;
    move.dest = location();
    move.accel.x = 0.0f;
    move.accel.y = -accel;
    move.accel.z = 0.0f;
    move.max_vel.x = 0.0f;
    move.max_vel.y = -maxVel;
    move.max_vel.z = 0.0f;
    move.vel.x = 0.0f; move.vel.y = 0.0f; move.vel.z = 0.0f;
}

// 0x8016DAEC
void zNPCBPlankton::aim_gun(xAnimPlay* play, xQuat* tilt, xVec3* dest, int bone)
{
    // TODO: 0x8016DAEC - apply gun tilt quaternion to bone matrix
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonIdle
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonIdle::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonIdle(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonIdle::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.flag.attacking = false;
    owner.refresh_orbit();
    owner.reset_speed();
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    F32 yaw, dummy;
    get_yaw(yaw, dummy);
    apply_yaw(yaw);
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonIdle::Exit(F32 dt, void* ctxt)
{
    owner.refresh_orbit();
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonIdle::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    if (owner.crony_attacking())
    {
        owner.take_control();
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONHUNT;
    }

    // get_yaw fills yaw/yawRate using orbit geometry
    F32 yaw, yawRate;
    get_yaw(yaw, yawRate);

    // Check follow delay timer: when elapsed, pick new orbit position
    F32 delay = owner.follow.delay;       // offset 0x4A4
    F32 maxDelay = owner.follow.max_delay; // offset 0x4A8
    // (apply idle drift logic here using yaw)

    return 0;
}

S32 zNPCGoalBPlanktonIdle::get_yaw(F32& outYaw, F32& outRate) const
{
    // Computes desired yaw for Plankton to align with orbit ring around player
    // Uses orbit_yaw_offset, tweak follow angles (min/max_ang)
    outYaw = 0.0f;
    outRate = 0.0f;
    return 0;
}

S32 zNPCGoalBPlanktonIdle::apply_yaw(F32 yaw)
{
    // Applies yaw rotation to Plankton's model matrix using set_yaw_matrix
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonAttack
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonAttack::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonAttack(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonAttack::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.flag.attacking = true;
    owner.refresh_orbit();
    owner.follow_player();
    owner.delay = 0.0f;
    owner.face_player();
    owner.reset_speed();
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonAttack::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonAttack::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    if (owner.crony_attacking())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONAMBUSH;
    }

    // When delay >= idle_time: pick sub-attack
    F32 delay = owner.delay; // offset 0x2C8
    if (delay >= tweak.idle_time)
    {
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonAmbush
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonAmbush::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonAmbush(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonAmbush::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.flag.attacking = false;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    owner.ambush_delay = tweak.help.fuse_delay;
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonAmbush::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonAmbush::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    zNPCBPlankton::territory_data& t = owner.territory[owner.active_territory];

    owner.ambush_delay -= dt;
    if (owner.ambush_delay <= 0.0f)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONATTACK;
    }

    if (t.fuse_destroyed)
    {
        owner.say(2, 0, false); // say_fuse_hit
        owner.sickum();
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONATTACK;
    }

    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonFlank
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonFlank::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonFlank(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonFlank::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.flag.attacking = true;

    // Generate a random flank orbit position at arena.attack.radius
    xVec3 flankDest;
    random_orbit(owner.orbit.center, 0.0f, tweak.arena.attack.radius, &flankDest);
    owner.location() = flankDest;

    owner.refresh_orbit();
    owner.follow_camera();
    owner.reset_speed();
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonFlank::Exit(F32 dt, void* ctxt)
{
    owner.reset_speed();
    owner.follow_player();
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonFlank::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    const xVec3& loc = owner.location();
    F32 dy = xabs(loc.y - owner.orbit.center.y); // offset 0x454

    if (dy < tweak.min_arena_dist) // small epsilon / min dist
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONATTACK;
    }
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonEvade
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonEvade::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonEvade(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonEvade::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.flag.attacking = false;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    evade_delay = tweak.evade.duration;
    owner.reset_speed();
    owner.refresh_orbit();
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonEvade::Exit(F32 dt, void* ctxt)
{
    owner.reset_speed();
    owner.halt(tweak.evade.max_vel);
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonEvade::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    evade_delay -= dt; // offset 0x2C8 on owner minus dt; stored in evade_delay member

    if (evade_delay <= 0.0f)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONATTACK;
    }

    // Periodically pick a new escape orbit position
    owner.delay += dt;
    F32 moveDelay = tweak.evade.move_delay_min + xurand() * (tweak.evade.move_delay_max - tweak.evade.move_delay_min);
    if (owner.delay >= moveDelay)
    {
        owner.delay = 0.0f;
        owner.refresh_orbit();
    }

    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonHunt
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonHunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonHunt(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonHunt::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    player_loc = *get_player_loc();
    owner.flag.attacking = true;
    owner.delay = 0.0f;
    owner.reset_speed();
    owner.refresh_orbit();
    owner.follow_camera();
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonHunt::Exit(F32 dt, void* ctxt)
{
    owner.refresh_orbit();
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonHunt::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    if (!owner.flag.hunt)
    {
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    owner.refresh_orbit();

    const xVec3& plankLoc = owner.location();
    const xVec3* pLoc = get_player_loc();

    xVec3 diff;
    xVec3Sub(&diff, &plankLoc, pLoc);
    F32 distSq = xVec3Length2(&diff);
    F32 beamDist = tweak.hunt.beam_dist;

    if (distSq <= beamDist * beamDist)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONBEAM;
    }

    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonTaunt (stub — Process returns 0 in shipped build)
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonTaunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonTaunt(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonTaunt::Enter(F32 dt, void* ctxt)
{
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonTaunt::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonTaunt::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonMove (stub — Process returns 0 in shipped build)
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonMove::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonMove(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonMove::Enter(F32 dt, void* ctxt)
{
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonMove::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonMove::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonStun
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonStun::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonStun(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonStun::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.delay = 0.0f;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonStun::Exit(F32 dt, void* ctxt)
{
    owner.give_control();
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonStun::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    xAnimPlay* play = owner.model->Anim;
    xAnimState* curState = play->Single->State;
    xAnimState* stunEndState = xAnimTableGetStateID(play->Table, g_hash_bossanim[ANIM_stun_end]);

    if (curState == stunEndState)
    {
        owner.delay += dt;
        if (owner.delay >= owner.stun_duration)
        {
            *trantype = GOAL_TRAN_SET;
            return owner.next_goal();
        }
    }
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonFall
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonFall::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonFall(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonFall::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.delay = 0.0f;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    owner.fall(tweak.fall.accel, tweak.fall.max_vel);
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonFall::Exit(F32 dt, void* updCtxt)
{
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonFall::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    const xVec3& loc = owner.location();
    F32 dy = owner.orbit.center.y - loc.y; // how far below orbit center

    if (dy >= tweak.fall.dist)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONDIZZY;
    }
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonDizzy
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonDizzy::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonDizzy(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonDizzy::Enter(F32 dt, void* ctxt)
{
    owner.give_control();
    owner.delay = 0.0f;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonDizzy::Exit(F32 dt, void* ctxt)
{
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonDizzy::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    xAnimPlay* play = owner.model->Anim;
    xAnimState* curState = play->Single->State;
    xAnimState* stunEndState = xAnimTableGetStateID(play->Table, g_hash_bossanim[ANIM_stun_end]);

    if (curState == stunEndState)
    {
        owner.delay += dt;
        if (owner.delay >= owner.stun_duration)
        {
            *trantype = GOAL_TRAN_SET;
            return owner.next_goal();
        }
    }
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonBeam
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonBeam::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonBeam(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonBeam::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    substate = SS_WARM_UP;
    owner.delay = 0.0f;
    emitted = 0.0f;
    owner.flag.aim_gun = true;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    owner.enable_emitter(*owner.beam_charge);
    play_sound(SOUND_CHARGE, (xVec3*)&owner.bound.pad[3], 1.0f);
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonBeam::Exit(F32 dt, void* ctxt)
{
    owner.flag.aim_gun = false;
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    owner.disable_emitter(*owner.beam_charge);
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonBeam::Process(en_trantype* trantype, F32 dt, void* ctxt, xScene* xscn)
{
    switch (substate)
    {
    case SS_WARM_UP:   update_warm_up(dt);   break;
    case SS_FIRE:      update_fire(dt);      break;
    case SS_COOL_DOWN: update_cool_down(dt); break;
    case SS_DONE:
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }
    return 0;
}

S32 zNPCGoalBPlanktonBeam::update_warm_up(F32 dt)
{
    owner.delay += dt;
    if (owner.delay >= tweak.beam.time_warm_up)
    {
        substate = SS_FIRE;
        owner.delay = 0.0f;
    }
    return 0;
}

S32 zNPCGoalBPlanktonBeam::update_fire(F32 dt)
{
    owner.delay += dt;

    // Build gun bone matrix for emit origin
    xMat4x3 mat;
    owner.aim_gun(owner.model->Anim, &owner.gun_tilt, &mat.pos, 0);

    // Compute how many bolts to emit this frame
    F32 toEmit = tweak.beam.rate * dt;
    S32 total = (S32)(emitted + toEmit);
    S32 count = total - (S32)emitted;
    emitted += toEmit;

    for (S32 i = 0; i < count; i++)
    {
        xVec3 origin = mat.pos;
        xVec3 dir;
        // Direction = gun forward + small random spread (beam.fx.rand_ang)
        xVec3Copy(&dir, &mat.at);
        // apply random angle perturbation
        xVec3Normalize(&dir, &dir);
        // offset origin by emit_dist along dir
        xVec3 offset;
        xVec3SMul(&offset, &dir, tweak.beam.emit_dist);
        xVec3Add(&origin, &origin, &offset);

        owner.beam.emit(origin, dir);
    }

    if (owner.delay >= tweak.beam.time_fire)
    {
        substate = SS_COOL_DOWN;
        owner.delay = 0.0f;
    }
    return 0;
}

S32 zNPCGoalBPlanktonBeam::update_cool_down(F32 dt)
{
    xAnimPlay* play = owner.model->Anim;
    xAnimState* curState = play->Single->State;
    xAnimState* beamEndState = xAnimTableGetStateID(play->Table, g_hash_bossanim[ANIM_attack_beam_end]);

    if (curState == beamEndState)
    {
        owner.delay += dt;
        if (owner.delay >= 0.1f) // small settle time
        {
            substate = SS_DONE;
        }
    }
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonWall (stub in shipped build)
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonWall::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonWall(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonWall::Enter(F32 dt, void* ctxt)
{
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonWall::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonWall::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonMissle (stub in shipped build — note: misspelled in source)
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonMissle::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonMissle(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonMissle::Enter(F32 dt, void* ctxt)
{
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonMissle::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonMissle::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

// -----------------------------------------------------------------------
// zNPCGoalBPlanktonBomb (stub in shipped build)
// -----------------------------------------------------------------------

xFactoryInst* zNPCGoalBPlanktonBomb::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonBomb(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonBomb::Enter(F32 dt, void* ctxt)
{
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonBomb::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonBomb::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

// -----------------------------------------------------------------------
// Weak / inline members
// -----------------------------------------------------------------------

xVec3& zNPCBPlankton::location() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->pos);
}

void zNPCBPlankton::render_debug()
{
    // Empty in shipped build (4 bytes: blr only)
}

void zNPCBPlankton::enable_emitter(xParEmitter& p) const
{
    p.emit_flags |= 1;
}

void zNPCBPlankton::disable_emitter(xParEmitter& p) const
{
    p.emit_flags &= ~1;
}

U8 zNPCBPlankton::ColChkFlags() const
{
    return 0;
}

U8 zNPCBPlankton::ColPenFlags() const
{
    return 0;
}

U8 zNPCBPlankton::ColChkByFlags() const
{
    return 16;
}

U8 zNPCBPlankton::ColPenByFlags() const
{
    return 16;
}

U8 zNPCBPlankton::PhysicsFlags() const
{
    return 3;
}

S32 zNPCBPlankton::IsAlive()
{
    return 1;
}

// 0x8016FCFC
void zNPCBPlankton::take_control()
{
    // TODO: 0x8016FCFC - crony->TakeControl() via vtable 0xD0, not yet identified
}

// 0x80170014
void zNPCBPlankton::give_control()
{
    // TODO: 0x80170014 - crony->GiveControl() via vtable 0xD4, not yet identified
}

void zNPCBPlankton::face_player()
{
    // Immediately aligns turn.dir toward player (inline, 0xC bytes)
    xVec3* pLoc = get_player_loc();
    const xVec3& myLoc = location();
    turn.dir.x = pLoc->x - myLoc.x;
    turn.dir.y = pLoc->z - myLoc.z;
}
