#pragma once

#include "common.h"
#include "Sprite.h"
#include "minicoro.h"

typedef u32 instance_id;

#define NULL_INSTANCE_ID 0xffffffffu

enum struct ObjType : u32 {
	PLAYER,
	ENEMY,
	BULLET,
	PLAYER_BULLET,
	ALLY
};

enum {
	FLAG_INSTANCE_DEAD = 1
};

enum {
	TYPE_ENEMY = 1000,
	TYPE_BOSS  = 2000
};

enum {
	ACTIVE_ITEM_NONE,
	ACTIVE_ITEM_HEAL
};

enum {
	ALLY_HEALING_DRONE
};

struct Object {
	ObjType type;
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

	float power;
	int power_level = 1;

	float health = 100.0f;
	float max_health = 100.0f;
	float boost = 100.0f;
	float max_boost = 100.0f;

	float invincibility;
	float fire_timer;
	int fire_queue;

	u8 items[10];
	u8 active_item = ACTIVE_ITEM_HEAL;
	float active_item_timer;
};

struct Enemy : Object {
	float radius = 15.0f;
	int enemy_type;
	float health = 50.0f;
	float max_health = 50.0f;
	float power = 1.0f;
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
	float radius = 5.0f;
	float dmg = 10.0f;
	float lifespan = 1.5f * 60.0f;
	float lifetime;
};

struct Ally : Object {
	int ally_type;

};
