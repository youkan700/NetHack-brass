/*	SCCS Id: @(#)objclass.h 3.4	1996/06/16	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef OBJCLASS_H
#define OBJCLASS_H

/* definition of a class of objects */

struct objclass {
	short	oc_name_idx;		/* index of actual name */
	short	oc_descr_idx;		/* description when name unknown */
	char *	oc_uname;		/* called by user */
	Bitfield(oc_name_known,1);
	Bitfield(oc_merge,1);	/* merge otherwise equal objects */
	Bitfield(oc_uses_known,1); /* obj->known affects full decription */
				/* otherwise, obj->dknown and obj->bknown */
				/* tell all, and obj->known should always */
				/* be set for proper merging behavior */
	Bitfield(oc_pre_discovered,1);	/* Already known at start of game; */
					/* won't be listed as a discovery. */
	Bitfield(oc_magic,1);	/* inherently magical object */
	Bitfield(oc_charged,1); /* may have +n or (n) charges */
	Bitfield(oc_unique,1);	/* special one-of-a-kind object */
	Bitfield(oc_nowish,1);	/* cannot wish for this object */
				/* 8 bits... */
	Bitfield(oc_big,1);
#define oc_bimanual	oc_big	/* for weapons & tools used as weapons */
#define oc_bulky	oc_big	/* for armor */
	Bitfield(oc_tough,1);	/* hard gems/rings */
#define oc_other_material	oc_tough
				/* can be made of unusual materials */

	Bitfield(oc_dir,2);
#define NODIR		1	/* for wands/spells: non-directional */
#define IMMEDIATE	2	/*		     directional */
#define RAY		3	/*		     zap beams */

#define PIERCE		1	/* for weapons & tools used as weapons */
#define SLASH		2	/* (latter includes iron ball & chain) */
#define WHACK		0

#define a_can	oc_dir		/* armor: used in mhitu.c */

	Bitfield(oc_rank,2);	/* rank of the object */
#define COMMON		0
#define FINE		1
#define EXCELLENT	2
#define SUPERB		3
	Bitfield(oc_rsvd1,2);	/* 2 free bits */
				/* 16 bits... */

	Bitfield(oc_rsvd2,3);	/* 3 free bits */

	Bitfield(oc_material,5);
				/* 24 bits... */
#define LIQUID		1	/* currently only for venom */
#define WAX		2
#define VEGGY		3	/* foodstuffs */
#define FLESH		4	/*   ditto    */
#define PAPER		5
#define CLOTH		6
#define LEATHER		7
#define WOOD		8
#define BONE		9
#define DRAGON_HIDE	10	/* not leather! */
#define IRON		11	/* Fe - includes steel */
#define METAL		12	/* Sn, &c. */
#define COPPER		13	/* Cu - includes brass */
#define SILVER		14	/* Ag */
#define GOLD		15	/* Au */
#define PLATINUM	16	/* Pt */
#define MITHRIL		17
#define PLASTIC		18
#define GLASS		19
#define GEMSTONE	20
#define MINERAL		21
#define MAX_MATERIAL	MINERAL

#define is_organic(otmp)	(get_material(otmp) <= WOOD) /* material patch */
#define is_metallic(otmp)	(get_material(otmp) >= IRON && \
				 get_material(otmp) <= MITHRIL) /* material patch */

/* primary damage: fire/rust/--- */
/* is_flammable(otmp), is_rottable(otmp) in mkobj.c */
#define is_rustprone(otmp)	(get_material(otmp) == IRON) /* material patch */

/* secondary damage: rot/acid/acid */
#define is_damageable(otmp) (is_rustprone(otmp) || is_flammable(otmp) || \
			      is_rottable(otmp) || is_corrodeable(otmp))
#define is_corrodeable(otmp)	(get_material(otmp) == COPPER || get_material(otmp) == IRON) /* material patch */

	schar	oc_subtyp;
#define oc_skill	oc_subtyp   /* Skills of weapons, spellbooks, tools, gems */
#define oc_armcat	oc_subtyp   /* for armor */
#define ARM_SHIELD	1	/* needed for special wear function */
#define ARM_HELM	2
#define ARM_GLOVES	3
#define ARM_BOOTS	4
#define ARM_CLOAK	5
#define ARM_SHIRT	6
#define ARM_SUIT	0
#define ARM_MAXNUM	7

	uchar	oc_oprop;		/* property (invis, &c.) conveyed */
	char	oc_class;		/* object class */
	schar	oc_delay;		/* delay when using such an object */
#define oc_wprop	oc_delay	/* weapon's property */
#define WP_THROWING	0x40		/* this weapon is also good for throwing */
#define WP_CONSUMABLE	0x20		/* used up if thrown and hit */
#define WP_POISONABLE	0x10		/* this weapon can be poisoned */

#define WP_WEAPONTYPE	0x0c		/* weapon type mask */
#define WP_NORMAL	0x00		/* short ranged or thrown without launcher */
#define WP_RANGED	0x04		/* long range weapon (polearms) */
#define WP_LAUNCHER	0x08		/* launcher (bow, crossbow, sling, ...) */
#define WP_AMMUNITION	0x0c		/* ammunition (arrow, bolt, stone, ...) */

#define WP_SUBTYPE	0x03		/* additional weapon info */
#define WP_ARROW	0x00		/* launcher & missle types */
#define WP_BOLT		0x01
#define WP_STONE	0x02
#define WP_BULLET	0x03

	uchar	oc_color;		/* color of the object */

	short	oc_prob;		/* probability, used in mkobj() */
	unsigned short	oc_weight;	/* encumbrance (1 cn = 0.1 lb.) */
	short	oc_cost;		/* base cost in shops */
/* Check the AD&D rules!  The FIRST is small monster damage. */
/* for weapons, and tools, rocks, and gems useful as weapons */
	uchar	oc_wsdam, oc_wldam;	/* max small/large monster damage */
	uchar	oc_oc1, oc_oc2;
#define oc_hitbon	oc_oc1		/* weapons: "to hit" bonus */

#define a_ac		oc_oc1	/* armor class, used in ARM_BONUS in do.c */
//#define a_can		oc_oc2		/* armor: used in mhitu.c */
#define oc_level	oc_oc2		/* books: spell level */
#define oc_dambon	oc_oc2		/* weapons: damage bonus(S<<4|L) */

	unsigned short	oc_nutrition;	/* food value */
};

struct objdescr {
	const char *oc_name;		/* actual name */
	const char *oc_descr;		/* description when name unknown */
};

extern NEARDATA struct objclass objects[];
extern NEARDATA struct objdescr obj_descr[];

/*
 * All objects have a class. Make sure that all classes have a corresponding
 * symbol below.
 */
#define RANDOM_CLASS	 0	/* used for generating random objects */
#define ILLOBJ_CLASS	 1
#define WEAPON_CLASS	 2
#define ARMOR_CLASS	 3
#define RING_CLASS	 4
#define AMULET_CLASS	 5
#define TOOL_CLASS	 6
#define FOOD_CLASS	 7
#define POTION_CLASS	 8
#define SCROLL_CLASS	 9
#define SPBOOK_CLASS	10	/* actually SPELL-book */
#define WAND_CLASS	11
#define COIN_CLASS	12
#define GEM_CLASS	13
#define ROCK_CLASS	14
#define BALL_CLASS	15
#define CHAIN_CLASS	16
#define VENOM_CLASS	17
#define MAXOCLASSES	18

#define ALLOW_COUNT	(MAXOCLASSES+1) /* Can be used in the object class */
#define ALL_CLASSES	(MAXOCLASSES+2) /* input to getobj().		   */
#define ALLOW_NONE	(MAXOCLASSES+3) /*				   */
#define ALLOW_FLOOR	(MAXOCLASSES+4) /*				   */

#define BURNING_OIL	(MAXOCLASSES+1) /* Can be used as input to explode. */
#define MON_EXPLODE	(MAXOCLASSES+2) /* Exploding monster (e.g. gas spore) */

#if 0	/* moved to decl.h so that makedefs.c won't see them */
extern const char def_oc_syms[MAXOCLASSES];	/* default class symbols */
extern uchar oc_syms[MAXOCLASSES];		/* current class symbols */
#endif

/* Default definitions of all object-symbols (must match classes above). */

#define ILLOBJ_SYM	']'	/* also used for mimics */
#define WEAPON_SYM	')'
#define ARMOR_SYM	'['
#define RING_SYM	'='
#define AMULET_SYM	'"'
#define TOOL_SYM	'('
#define FOOD_SYM	'%'
#define POTION_SYM	'!'
#define SCROLL_SYM	'?'
#define SPBOOK_SYM	'+'
#define WAND_SYM	'/'
#define GOLD_SYM	'$'
#define GEM_SYM		'*'
#define ROCK_SYM	'`'
#define BALL_SYM	'0'
#define CHAIN_SYM	'_'
#define VENOM_SYM	'.'

struct fruit {
	char fname[PL_FSIZ];
	int fid;
	struct fruit *nextf;
};
#define newfruit() (struct fruit *)alloc(sizeof(struct fruit))
#define dealloc_fruit(rind) free((genericptr_t) (rind))

#define use_japanese_name() (Role_if(PM_SAMURAI) || Role_if(PM_MEDIUM))

#define OBJ_NAME(obj)  (obj_descr[(obj).oc_name_idx].oc_name)
#define OBJ_DESCR(obj) (obj_descr[(obj).oc_descr_idx].oc_descr)

#ifdef JP
#define JOBJ_NAME(obj)  (jobj_descr[(obj).oc_name_idx].oc_name)
#define JOBJ_DESCR(obj) (jobj_descr[(obj).oc_descr_idx].oc_descr)
#endif /*JP*/

#endif /* OBJCLASS_H */
