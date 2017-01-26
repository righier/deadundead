#define _USE_MATH_DEFINES

#include <math.h>
#include "objects_data.h"
#include "physics.h"
#include "gameplay.h"
#include <stdlib.h>

const OtherData other_data[] = {
	{
		NULL//TODO
	}
};

//numero di armi melee che non spawnano, da aggiornare a mano; le tengo all'inizio
#define NUM_MELEE_NON_S 3
const WeaponDataMelee wd_melee[] = {
	{//PUGNO
		{{1.0f,1.0f},{1.0f,1.0f}},
		1.0f,
		NULL,
		1.0
	},
#define ID_CANDELABRO_ACCESO 1
	{//CANDELABRO ACCESO
		{{1.0f,1.0f},{1.0f,1.0f}},
		2.0f,
		"candelabro_acceso",
		1.0
	},
#define ID_SCOPA_ACCESA 2
	{//SCOPA ACCESA
		{{1.0f,1.0f},{1.0f,1.0f}},
		1.0f,
		"scopa_accesa",
		1.0
	},
#define ID_CANDELABRO 3
	{//CANDELABRO
		{{1.0f,1.0f},{1.0f,1.0f}},
		1.0f,
		"candelabro",
		1.0
	},
#define ID_SCOPA 4
	{//SCOPA
		{{1.0f,1.0f},{1.0f,1.0f}},
		1.0f,
		"scopa",
		1.0
	}
};

void func_bomba (Entity *self, Entity *first_hit, World *world);
void func_piatto (Entity *self, Entity *first_hit, World *world);
void func_p_balestra (Entity *self, Entity *first_hit, World *world);
void func_p_staffa (Entity *self, Entity *first_hit, World *world);

//numero di proiettili spawnabili (cio� non solo per shooter); da aggiornare a mano
#define NUM_PROJECTILE 2 
const WeaponDataProjectile wd_projectile[] = {//i non spawnabili vanno in fondo
	{//BOMBA
		{1.0f,1.0f},
		45,
		2.0f,
		"bomba",
		3.0,
		&func_bomba
	},
	{//PIATTO
		{1.0f,1.0f},
		30,
		2.0f,
		"piatto",
		0,
		&func_piatto
	},
#define ID_P_BALESTRA (0+NUM_PROJECTILE)
	{//PROIETTILE BALESTRA
		{1.0f,1.0f},
		0,
		4.0f,
		"balestra_dardo",
		0,
		&func_p_balestra
	},
#define ID_P_STAFFA (1+NUM_PROJECTILE)
	{//PROIETTILE STAFFA
		{1.0f,1.0f},
		0,
		2.0f,
		"staffa_scintilla",
		0,
		&func_p_staffa
	}
};

const WeaponDataShooter wd_shooter[] = {
	{//BALESTRA
		ID_P_BALESTRA,
		"balestra",
		1.0
	},
	{//STAFFA
		ID_P_STAFFA,
		"staffa",
		0.8
	}
};

//quantit� spawnabili
#define NUM_OTHER (sizeof(other_data)/sizeof(OtherData))
#define NUM_MELEE (sizeof(wd_melee)/sizeof(WeaponDataMelee) - NUM_MELEE_NON_S)
#define NUM_SHOOTER (sizeof(wd_shooter)/sizeof(WeaponDataShooter))

void pickupable_on_interact (Entity *source, Entity *target);
static void spawn_pickupable(float pos_x, float pos_y, int type, int id, World *world) {
	Entity *e = world_new_entity(world, RENDER | ACTIVE_EVENT);
	//TODO Texture e render_func;
	e->type_in_hand = type;
	e->id_in_hand = id;
	e->x = pos_x; e->y = pos_y;
	if (type == OTHER && other_data[id].on_interact != NULL) {
		e->on_interact = other_data[id].on_interact;
	} else {
		e->on_interact = &pickupable_on_interact;
	}
}

void spawn_random_pickupable(float pos_x, float pos_y, World *world) {
	int val = rand()%(NUM_OTHER + NUM_PROJECTILE + NUM_MELEE + NUM_SHOOTER);
	if (val < NUM_OTHER) {
		spawn_pickupable(pos_x, pos_y, OTHER, val, world);
	} else if (val < NUM_OTHER+NUM_MELEE) {
		spawn_pickupable(pos_x, pos_y, MELEE, val - NUM_OTHER + NUM_MELEE_NON_S, world);
	} else if (val < NUM_OTHER+NUM_MELEE+NUM_SHOOTER) {
		spawn_pickupable(pos_x, pos_y, SHOOTER, val - (NUM_OTHER+NUM_MELEE), world);
	} else {
		spawn_pickupable(pos_x, pos_y, PROJECTILE, val - (NUM_OTHER+NUM_MELEE+NUM_SHOOTER), world);
	}
}

/************FUNZIONI*BASE*PER*PROIETTILI*************/
//applica hit_func a tutte le entit� hittable nel cerchio attorno al proiettile
static void hit_in_area(World *world, float radius, Entity *projectile, void (*hit_func) (Entity *e), int exclude_thrower) {
	Vec2 p_center = get_entity_center(projectile);
	EntityList *hittable_list = &world->lists[HITTABLE_LIST];

	for (int i = 0; i < hittable_list->count; i++) {
		Entity *e = world_get_entity(world, hittable_list->entity_id[i]);
		Rectangle er = get_entity_rectangle(e);
		if ((!exclude_thrower || e != projectile->thrower) && collides(&p_center, radius, &er)) {
			hit_func(e);
		}
	}
}

static void projectile_update_function(Entity *self, World *world, double delta) {
	//aggiorno posizione (collisioni gestite nel world update)
	if (self->type_in_hand == 1) {	//lo uso per sapere se applicare la fisica o no
		self -> old_x = self -> x;
		self -> x += (float) ((self->speed_x)*delta);
	} else {
#define DOWNWARD_ACCELLERATION	1//TODO
#define DOWNWARD_MAX_SPEED		-1//TODO
		self -> old_x = self -> x; self -> old_y = self -> y;
		self -> x += (float) ((self->speed_x)*delta); self -> y += (float) ((self->speed_x)*delta);
		self -> speed_y -= (float) (DOWNWARD_ACCELLERATION*delta);
		if (self -> speed_y < DOWNWARD_MAX_SPEED) {
			self -> speed_y = DOWNWARD_MAX_SPEED;
		}
#undef DOWNWARD_ACCELLERATION
#undef DOWNWARD_MAX_SPEED
	}

	//controllo se attivarmi
	if (self -> timer == 0) {
		//alla collisione
		Rectangle rect_self = {{self->x, self->y},{self->width, self->height}};
		EntityList *hittable_list = &world->lists[HITTABLE_LIST];

		for (int i = 0; i < hittable_list->count; i++) {
			Entity *e = world_get_entity(world, hittable_list->entity_id[i]);
			Rectangle rect_e = {{e->x, e->y},{e->width, e->height}};
			if (e != self -> thrower && collides(&rect_self, &rect_e)) {
				self -> on_collide(self, e, world);
				break;
			}
		}
	} else {
		//alla scadenza del timer
		self -> timer -= delta;
		if (self -> timer <= 0) {
			get_data_projectile(self->id_in_hand) -> hit_func(self, NULL, world);
		}
	}
}

void create_projectile(World *world, Entity *source, int projectile_id) {
	const WeaponDataProjectile *data = get_data_projectile(projectile_id);
	Entity *p = world_new_entity(world, UPDATE | RENDER | DYNAMIC_COLLIDE);

	p->x = source->x + source->width/2; p->y = source->y + source->height/2;
	p->width = data->size.x; p->height = data->size.y;
	p->old_x = p->x; p->old_y = p->y;
	if (data->angle == 0) {
		p->speed_x = data->speed; p->speed_y = 0;
		p->type_in_hand = 1;
	} else {
		p->speed_x = (data->speed) * (float) cos(M_PI*(data->angle)/180.0);
		p->speed_y = (data->speed) * (float) sin(M_PI*(data->angle)/180.0);
		p->type_in_hand = 0;
	}
	if (!source->is_facing_right) {
		p->speed_x = -p->speed_x;
	}
	p->texture = load_texture(data->texture);
	//int is_on_floor;
	p->is_facing_right = source->is_facing_right;
	//int player_id;
	p->bounce_coeff = 0.9f;

	//PlayerStatus status;
	//double score;
	//float skill_bar;
	p->timer = data->timer;
	p->id_in_hand = projectile_id;
	//SpriterInstance *animation; //per settare quale animazione fare

	p->thrower = source;

	p->update = &projectile_update_function;
	p->render = &wall_render; //TODO
	if (data->timer == 0) {
		p->on_collide = data->hit_func;
	} else {
		p->on_collide = NULL;
	}
	p->on_hit = NULL;
	p->on_enter = NULL;
	p->on_interact = NULL;
}

//applica hit a e se � un hittable non player, hit_player se � un player, altrimenti non fa niente
void hit_if_hittable(Entity *e, void (*hit) (Entity *), void (*hit_player) (Entity *)) {
	if ((e->tag & HITTABLE) != 0) {
		if ((e->tag & PLAYER) != 0) {
			if (hit_player != NULL) {
				hit_player(e);
			}
		} else if (hit != NULL) {
			hit(e);
		}
	}
}

//danneggia l'entit� se hittable poi si distrugge
void simple_projectile(Entity *e, float damage, Entity *self, World *world) {
	if ((e->tag & HITTABLE) != 0) {
		damage_entity(e, damage);
	}
	world_remove_entity(world, self->id);
}
/************FUNZIONI*SPECIFICHE**********************/
void func_bomba_danni(Entity *e) {damage_entity(e, 3.0f);}
void func_bomba (Entity *self, Entity *first_hit, World *world) {
	hit_in_area(world, 5.0f, self, &func_bomba_danni, 0);
	world_remove_entity(world, self->id);
}

void func_piatto_hit(Entity *e) {damage_entity(e, 1.5f);}
void func_piatto_hit_player(Entity *e) {func_piatto_hit(e); stun_entity(e, 1.0);}
void func_piatto (Entity *self, Entity *first_hit, World *world) {
	hit_if_hittable(first_hit, &func_piatto_hit, &func_piatto_hit_player);
	world_remove_entity(world, self->id);
}

void func_p_balestra(Entity *self, Entity *first_hit, World *world) {simple_projectile(first_hit, 1.0f, self, world);}

void func_p_staffa(Entity *self, Entity *first_hit, World *world) {simple_projectile(first_hit, 1.5f, self, world);}

/***/
const WeaponDataMelee *get_data_melee(int id) {return &(wd_melee[id]);}
const WeaponDataProjectile *get_data_projectile(int id){return &(wd_projectile[id]);}
const WeaponDataShooter *get_data_shooter(int id){return &(wd_shooter[id]);}

void do_other_attack(World *world, Entity *holder) {
	if (other_data[holder->id_in_hand].on_attack != NULL) {
		other_data[holder->id_in_hand].on_attack(holder, world);
	}
}
void do_other_update(Entity *holder) {
	if (other_data[holder->id_in_hand].on_update != NULL) {
		other_data[holder->id_in_hand].on_update(holder);
	}
}

/********************************FUNZIONI*PER*INTERAZIONE******************************************/
//da mettere su entit� che rappresentano oggetti da raccogliere e tenere in mano
//source � il player che ha interagito, target � l'oggetto raccolto
//elimina il vecchio oggetto tenuto dal player e ci mette i suoi dati
static void pickupable_on_interact (Entity *source, Entity *target) {
	if (source->status == ATTACKING) {
		source->timer = 0;
	}
	source->type_in_hand = target->type_in_hand;
	source->id_in_hand = target->id_in_hand;
}