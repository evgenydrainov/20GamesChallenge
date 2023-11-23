#pragma once

#include "common.h"
#include "Sprite.h"
#include "Items.h"
#include "minicoro.h"

typedef u32 instance_id;

#define NULL_INSTANCE_ID 0

enum struct ObjType : u32 {
	PLAYER,
	ENEMY,
	BULLET,
	PLAYER_BULLET,
	ALLY,
	CHEST
};

enum {
	FLAG_INSTANCE_DEAD = 1
};

enum {
	TYPE_ENEMY         = 1000,
	TYPE_ENEMY_SPREAD  = 1001,
	TYPE_ENEMY_MISSILE = 1002,

	TYPE_BOSS  = 2000
};

enum {
	ALLY_HEALING_DRONE
};

enum struct BulletType {
	NORMAL,
	HOMING,
	LAZER
};

enum {
	CHEST_ITEM,
	CHEST_ACTIVE_ITEM
};

struct Object {
	ObjType object_type;
	instance_id id;
	u32 flags;

	float x;
	float y;
	float hsp;
	float vsp;
	Sprite* sprite;
	float frame_index;
};

struct Player : Object {
	float radius = 8.0f;
	float dir;
	bool focus;

	float experience;
	int level;
	float money;

	float health = 100.0f;
	float max_health = 100.0f;
	float boost = 100.0f;
	float max_boost = 100.0f;

	float invincibility;
	float fire_timer;
	int fire_queue;
	int shot;

	u8 items[ITEM_COUNT];
	u8 active_item;
	float active_item_cooldown;
};

struct Enemy : Object {
	float radius = 15.0f;
	int type;
	float health = 50.0f;
	float max_health = 50.0f;
	float experience;
	float money;
	float angle;
	mco_coro* co;

	union {
		struct { // TYPE_ENEMY
			float catch_up_timer;
			float acc;
			float max_spd;
			bool stop_when_close_to_player;
			bool not_exact_player_dir;
		};
		struct { // TYPE_ENEMY+1
			float catch_up_timer;
			float acc;
			float max_spd;
			bool stop_when_close_to_player;
			bool not_exact_player_dir;
			instance_id lazer;
			bool emit_lazer;
		};
	};
};

struct Bullet : Object {
	BulletType type;
	float radius = 5.0f;
	float dmg;
	float lifespan = 1.5f * 60.0f;
	float lifetime;
	union {
		struct { // HOMING
			float dir;
			float t;
			float max_spd;
			float max_acc;
		};
	};
};

struct Ally : Object {
	int type;

};

struct Chest : Object {
	int type;
	float radius = 40.0f;
	float cost;
	bool opened;

	union {
		u8 item;      // CHEST_ITEM
		u8 active_item; // CHEST_ACTIVE_ITEM
	};
};
