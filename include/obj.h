/*	SCCS Id: @(#)obj.h	3.4	2002/01/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef OBJ_H
#define OBJ_H

/* #define obj obj_nh */ /* uncomment for SCO UNIX, which has a conflicting
			  * typedef for "obj" in <sys/types.h> */

union vptrs {
	    struct obj *v_nexthere;	/* floor location lists */
	    struct obj *v_ocontainer;	/* point back to container */
	    struct monst *v_ocarry;	/* point back to carrying monst */
};

struct obj {
	struct obj *nobj;
	union vptrs v;
#define nexthere	v.v_nexthere
#define ocontainer	v.v_ocontainer
#define ocarry		v.v_ocarry

	struct obj *cobj;	/* contents list for containers */
	unsigned o_id;
	xchar ox,oy;
	short otyp;		/* object class number */
	unsigned owt;
	long quan;		/* number of items */

	schar spe;		/* quality of weapon, armor or ring (+ or -)
				   number of charges for wand ( >= -1 )
				   marks your eggs, spinach tins
				   royal coffers for a court ( == 2)
				   tells which fruit a fruit is
				   special for uball and amulet
				   historic and gender for statues */
#define STATUE_HISTORIC 0x01
#define STATUE_MALE     0x02
#define STATUE_FEMALE   0x04
#define STATUE_ORACLE   0x08
	char	oclass;		/* object class */
	char	invlet;		/* designation in inventory */
	char	oartifact;	/* artifact array index */

	xchar where;		/* where the object thinks it is */
#define OBJ_FREE	0		/* object not attached to anything */
#define OBJ_FLOOR	1		/* object on floor */
#define OBJ_CONTAINED	2		/* object in a container */
#define OBJ_INVENT	3		/* object in the hero's inventory */
#define OBJ_MINVENT	4		/* object in a monster inventory */
#define OBJ_MIGRATING	5		/* object sent off to another level */
#define OBJ_BURIED	6		/* object buried */
#define OBJ_ONBILL	7		/* object on shk bill */
#define NOBJ_STATES	8
	xchar timed;		/* # of fuses (timers) attached to this obj */

	uchar color;		/* color of the obj */
	xchar reserved1;	/*  */

/*0*/
	Bitfield(cursed,1);
	Bitfield(blessed,1);
	Bitfield(unpaid,1);	/* on some bill */
	Bitfield(no_charge,1);	/* if shk shouldn't charge for this */
	Bitfield(known,1);	/* exact nature known */
	Bitfield(dknown,1);	/* color or text known */
	Bitfield(bknown,1);	/* blessing or curse known */
	Bitfield(rknown,1);	/* rustproof or not known */
/*1*/
	Bitfield(oeroded,2);	/* rusted/burnt weapon/armor */
	Bitfield(oeroded2,2);	/* corroded/rotted weapon/armor */
#define greatest_erosion(otmp) (int)((otmp)->oeroded > (otmp)->oeroded2 ? (otmp)->oeroded : (otmp)->oeroded2)
#define MAX_ERODE 3
#define orotten oeroded		/* rotten food */
#define odiluted oeroded	/* diluted potions */
#define norevive oeroded2
	Bitfield(oerodeproof,1); /* erodeproof weapon/armor */
	Bitfield(olocked,1);	/* object is locked */
#define sokoprize olocked	/* special flag for sokoban prize */
#define mcandles7 olocked	/* 7 magic candles are attached to candelabrum */
	Bitfield(obroken,1);	/* lock has been broken */
#define oshot obroken		/* a bullet is shot from a gun, not thrown */
	Bitfield(otrapped,1);	/* container is trapped */
				/* or accidental tripped rolling boulder trap */
#define opoisoned otrapped	/* object (weapon) is coated with poison */
/*2*/
	Bitfield(recharged,3);	/* number of times it's been recharged */
#define oprint recharged	/* print type of T-shirt */
#define TSHIRT_PRINT_NONE 0
#define TSHIRT_PRINT_TEXT 1
	Bitfield(lamplit,1);	/* a light-source -- can be lit */
#ifdef PICKUP_THROWN
	Bitfield(othrown,1);	/* object is thrown by you */
#endif
	Bitfield(greased,1);	/* covered with grease */
	Bitfield(has_name,1);	/* has an attached name */
/*3*/
	Bitfield(in_use,1);	/* for magic items before useup items */
	Bitfield(bypass,1);	/* mark this as an object to be skipped by bhito() */
	Bitfield(etherial,1);	/* magically created object */
	Bitfield(madeof,5);	/* material num if obj is made of unusual material */
/*4*/
	int	corpsenm;	/* type of corpse is mons[corpsenm] */
#define leashmon  corpsenm	/* gets m_id of attached pet */
#define spestudied corpsenm	/* # of times a spellbook has been studied */
#define fromsink  corpsenm	/* a potion from a sink */
#define prevotyp  corpsenm	/* previous otyp (plain dragon scale mail) */

	unsigned oeaten;	/* nutrition left in food, if partly eaten */
#define odamaged oeaten		/* how much object's special power worn out */
	long age;		/* creation date */

	long owornmask;
	struct monst *omon;	/* mtraits: stoned monster, corpse of reviver... */
	struct xdat *oextra;	/* extra information attached */
};

#define newobj()	(struct obj *)alloc(sizeof(struct obj))
#define ONAME(otmp)	((char *)get_xdat_obj((otmp),XDAT_NAME))
#define ONAMELTH(otmp)	(namelth_xdat_obj(otmp))

/* Weapons and weapon-tools */
/* KMH -- now based on skill categories.  Formerly:
 *	#define is_sword(otmp)	(otmp->oclass == WEAPON_CLASS && \
 *			 objects[otmp->otyp].oc_wepcat == WEP_SWORD)
 *	#define is_blade(otmp)	(otmp->oclass == WEAPON_CLASS && \
 *			 (objects[otmp->otyp].oc_wepcat == WEP_BLADE || \
 *			  objects[otmp->otyp].oc_wepcat == WEP_SWORD))
 *	#define is_weptool(o)	((o)->oclass == TOOL_CLASS && \
 *			 objects[(o)->otyp].oc_weptool)
 *	#define is_multigen(otyp) (otyp <= SHURIKEN)
 *	#define is_poisonable(otyp) (otyp <= BEC_DE_CORBIN)
 */
#define is_blade(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= P_DAGGER_GROUP && \
			 objects[otmp->otyp].oc_skill <= P_SABER_GROUP)
#define is_axe(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_skill == P_AXE_GROUP)
#define is_pick(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_skill == P_PICKAXE_GROUP)
#define is_sword(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill >= P_SHORT_BLADE_GROUP && \
			 objects[otmp->otyp].oc_skill <= P_SABER_GROUP)
#define is_pole(otmp)	((otmp->oclass == WEAPON_CLASS || \
			otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_skill == P_POLEARM_GROUP)
#define is_spear(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill == P_SPEAR_GROUP)
#define is_gun(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 objects[otmp->otyp].oc_skill == P_FIREARM_GROUP)
#define maxbullets(otmp) (otmp->otyp == REVOLVER ? 6 : \
                          otmp->otyp == MUSKET ? 1 : 0)
#define is_launcher(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 (objects[otmp->otyp].oc_wprop & WP_WEAPONTYPE) == WP_LAUNCHER)
#define is_ammo(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == GEM_CLASS) && \
			 (objects[otmp->otyp].oc_wprop & WP_WEAPONTYPE) == WP_AMMUNITION)
#define is_bullet(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 (objects[otmp->otyp].oc_wprop & (WP_WEAPONTYPE|WP_SUBTYPE)) == \
			 (WP_AMMUNITION|WP_BULLET))
#define ammo_and_launcher(otmp,ltmp) \
			 (is_ammo(otmp) && (ltmp) && is_launcher(ltmp) && \
			 ((objects[(otmp)->otyp].oc_wprop & WP_SUBTYPE) == \
			  (objects[(ltmp)->otyp].oc_wprop & WP_SUBTYPE) ))
#define is_missile(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 (objects[otmp->otyp].oc_wprop & WP_THROWING) && \
			 (objects[otmp->otyp].oc_wprop & WP_CONSUMABLE) && \
			 (objects[otmp->otyp].oc_wprop & WP_WEAPONTYPE) == WP_NORMAL)
#define is_weptool(o)	((o)->oclass == TOOL_CLASS && \
			 objects[(o)->otyp].oc_skill != P_NONE)
#define bimanual(otmp)	((otmp->oclass == WEAPON_CLASS || \
			 otmp->oclass == TOOL_CLASS) && \
			 objects[otmp->otyp].oc_bimanual)
#define is_multigen(otmp)	(is_ammo(otmp) || is_missile(otmp))
#define is_poisonable(otmp)	(otmp->oclass == WEAPON_CLASS && \
			(objects[otmp->otyp].oc_wprop & WP_POISONABLE))
#define is_consumable(otmp)	((otmp->oclass == WEAPON_CLASS || otmp->oclass == GEM_CLASS) && \
			(objects[otmp->otyp].oc_wprop & WP_CONSUMABLE))
#define is_ranged(otmp)	((otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) && \
			 (objects[otmp->otyp].oc_wprop & WP_WEAPONTYPE) == WP_RANGED)
#define uslinging()	(uwep && uwep->oclass == WEAPON_CLASS && \
			(objects[uwep->otyp].oc_wprop & (WP_WEAPONTYPE|WP_SUBTYPE)) == (WP_LAUNCHER|WP_STONE))
#define is_hammer(otmp)	(otmp->oclass == WEAPON_CLASS && \
			 (objects[otmp->otyp].oc_skill == P_CRUSHING_GROUP || \
			  objects[otmp->otyp].oc_skill == P_FLAIL_GROUP || \
			  otmp->otyp == LUCERN_HAMMER || otmp->otyp == BEC_DE_CORBIN))
/* The amount added to the victim's total hit points to insure that the
   victim will be killed even after damage bonus/penalty adjustments.
   Most such penalties are small, and 200 is plenty; the exception is
   half physical damage.  3.3.1 and previous versions tried to use a very
   large number to account for this case; now, we just compute the fatal
   damage by adding it to 2 times the total hit points instead of 1 time.
   Note: this will still break if they have more than about half the number
   of hit points that will fit in a 15 bit integer. */
#define FATAL_DAMAGE_MODIFIER 200

/* Armor */
#define is_shield(otmp) (otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_SHIELD)
#define is_helmet(otmp) (otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_HELM)
#define is_boots(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_BOOTS)
#define is_gloves(otmp) (otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_GLOVES)
#define is_cloak(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_CLOAK)
#define is_shirt(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_SHIRT)
#define is_suit(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 objects[otmp->otyp].oc_armcat == ARM_SUIT)
#define is_robe(otmp)	(otmp->oclass == ARMOR_CLASS && \
			 otmp->otyp >= ROBE && otmp->otyp <= ROBE_OF_WEAKNESS)
#define is_clothes(otmp) (otmp->oclass == ARMOR_CLASS && \
			  (get_material(otmp) == CLOTH || otmp->otyp == LEATHER_JACKET))
#define is_elven_armor(otmp)	((otmp)->otyp == ELVEN_LEATHER_HELM\
				|| (otmp)->otyp == ELVEN_MITHRIL_COAT\
				|| (otmp)->otyp == ELVEN_CLOAK\
				|| (otmp)->otyp == ELVEN_SHIELD\
				|| (otmp)->otyp == ELVEN_BOOTS)
#define is_orcish_armor(otmp)	((otmp)->otyp == ORCISH_HELM\
				|| (otmp)->otyp == ORCISH_CHAIN_MAIL\
				|| (otmp)->otyp == ORCISH_RING_MAIL\
				|| (otmp)->otyp == ORCISH_CLOAK\
				|| (otmp)->otyp == URUK_HAI_SHIELD\
				|| (otmp)->otyp == ORCISH_SHIELD)
#define is_dwarvish_armor(otmp)	((otmp)->otyp == DWARVISH_IRON_HELM\
				|| (otmp)->otyp == DWARVISH_MITHRIL_COAT\
				|| (otmp)->otyp == DWARVISH_CLOAK\
				|| (otmp)->otyp == DWARVISH_ROUNDSHIELD)
#define is_gnomish_armor(otmp)	(FALSE)
#define is_heavy_armor(otmp) (is_suit(otmp) && objects[otmp->otyp].oc_weight >= 200)

#define WEP_ENCHANT_MAX  3
#define WEP_ENCHANT_WARN 3

/* Eggs and other food */
#define MAX_EGG_HATCH_TIME 200	/* longest an egg can remain unhatched */
#define stale_egg(egg)	((monstermoves - (egg)->age) > (2*MAX_EGG_HATCH_TIME))
#define ofood(o) ((o)->otyp == CORPSE || (o)->otyp == EGG || (o)->otyp == TIN)
#define polyfodder(obj) (ofood(obj) && \
			 pm_to_cham((obj)->corpsenm) != CHAM_ORDINARY)
#define mlevelgain(obj) (ofood(obj) && (obj)->corpsenm == PM_WRAITH)
#define mhealup(obj)	(ofood(obj) && (obj)->corpsenm == PM_NURSE)

/* Containers */
#define carried(o)	((o)->where == OBJ_INVENT)
#define mcarried(o)	((o)->where == OBJ_MINVENT)
#define Has_contents(o) ((o)->cobj != (struct obj *)0)
#define Is_container(o) ((o)->otyp >= LARGE_BOX && (o)->otyp <= BAG_OF_TRICKS)
#define Is_box(otmp)	(otmp->otyp == LARGE_BOX || otmp->otyp == CHEST)
#define Is_mbag(otmp)	(otmp->otyp == BAG_OF_HOLDING || \
			 otmp->otyp == BAG_OF_TRICKS)

#define Is_sokoprize(otmp)	((otmp)->sokoprize && !Is_box(otmp))
#ifdef MAGIC_GLASSES
#define Is_glasses(otmp)	((otmp)->otyp >= GLASSES_OF_MAGIC_READING && \
				 (otmp)->otyp <= GLASSES_OF_PHANTASMAGORIA)
#endif /*MAGIC_GLASSES*/

/* dragon gear */
#define Is_dragon_scales(obj)	((obj)->otyp >= GRAY_DRAGON_SCALES && \
				 (obj)->otyp <= CHROMATIC_DRAGON_SCALES)
#define Is_dragon_mail(obj)	((obj)->otyp >= GRAY_DRAGON_SCALE_MAIL && \
				 (obj)->otyp <= CHROMATIC_DRAGON_SCALE_MAIL)
#define Is_dragon_armor(obj)	(Is_dragon_scales(obj) || Is_dragon_mail(obj))
#define Dragon_scales_to_pm(obj) (&mons[(obj)->otyp == CHROMATIC_DRAGON_SCALES ? \
				       PM_CHROMATIC_DRAGON : \
				       PM_GRAY_DRAGON + (obj)->otyp - GRAY_DRAGON_SCALES])
#define Dragon_mail_to_pm(obj)	(&mons[(obj)->otyp == CHROMATIC_DRAGON_SCALE_MAIL ? \
				      PM_CHROMATIC_DRAGON :  \
				      PM_GRAY_DRAGON + (obj)->otyp - GRAY_DRAGON_SCALE_MAIL])
#define Dragon_to_scales(pm)	((pm- mons) == PM_CHROMATIC_DRAGON ? \
				 CHROMATIC_DRAGON_SCALES : GRAY_DRAGON_SCALES + (pm - mons))
/* Elven gear */
#define is_elven_weapon(otmp)	((otmp)->otyp == ELVEN_ARROW\
				|| (otmp)->otyp == ELVEN_SPEAR\
				|| (otmp)->otyp == ELVEN_DAGGER\
				|| (otmp)->otyp == ELVEN_SHORT_SWORD\
				|| (otmp)->otyp == ELVEN_BROADSWORD\
				|| (otmp)->otyp == ELVEN_BOW)
#define is_elven_obj(otmp)	(is_elven_armor(otmp) || is_elven_weapon(otmp))

/* Orcish gear */
#define is_orcish_obj(otmp)	(is_orcish_armor(otmp)\
				|| (otmp)->otyp == ORCISH_ARROW\
				|| (otmp)->otyp == ORCISH_SPEAR\
				|| (otmp)->otyp == ORCISH_DAGGER\
				|| (otmp)->otyp == ORCISH_SHORT_SWORD\
				|| (otmp)->otyp == ORCISH_BOW)

/* Dwarvish gear */
#define is_dwarvish_obj(otmp)	(is_dwarvish_armor(otmp)\
				|| (otmp)->otyp == DWARVISH_SPEAR\
				|| (otmp)->otyp == DWARVISH_SHORT_SWORD\
				|| (otmp)->otyp == DWARVISH_MATTOCK)

/* Gnomish gear */
#define is_gnomish_obj(otmp)	(is_gnomish_armor(otmp))

#define is_special_armor(obj)	(is_elven_armor(obj) || \
			(Role_if(PM_WIZARD) && (obj)->otyp == CORNUTHAUM))


/* Light sources */
#define Is_candle(otmp) (otmp->otyp == TALLOW_CANDLE || \
			 otmp->otyp == WAX_CANDLE || \
			 otmp->otyp == MAGIC_CANDLE)
#define MAX_OIL_IN_FLASK 400	/* maximum amount of oil in a potion of oil */

/* MAGIC_LAMP intentionally excluded below */
/* age field of this is relative age rather than absolute */
#define age_is_relative(otmp)	((otmp)->otyp == BRASS_LANTERN\
				|| (otmp)->otyp == OIL_LAMP\
				|| (otmp)->otyp == CANDELABRUM_OF_INVOCATION\
				|| (otmp)->otyp == TALLOW_CANDLE\
				|| (otmp)->otyp == WAX_CANDLE\
				|| (otmp)->otyp == POT_OIL)
/* object can be ignited */
#define ignitable(otmp)	((otmp)->otyp == BRASS_LANTERN\
				|| (otmp)->otyp == OIL_LAMP\
				|| (otmp)->otyp == CANDELABRUM_OF_INVOCATION\
				|| (otmp)->otyp == TALLOW_CANDLE\
				|| (otmp)->otyp == WAX_CANDLE\
				|| (otmp)->otyp == POT_OIL)

/* special stones */
#define is_graystone(obj)	((obj)->otyp == LUCKSTONE || \
				 (obj)->otyp == LOADSTONE || \
				 (obj)->otyp == FLINT     || \
				 (obj)->otyp == TOUCHSTONE)

/* misc */
#ifdef KOPS
#define is_flimsy(otmp)		(get_material(otmp) <= LEATHER || \
				 (otmp)->otyp == RUBBER_HOSE)
#else
#define is_flimsy(otmp)		(get_material(otmp) <= LEATHER) /* material patch */
#endif

#define has_color_variation(otmp) ((otmp)->otyp == HAWAIIAN_SHIRT || \
				   (otmp)->otyp == T_SHIRT)

/* helpers, simple enough to be macros */
#define is_plural(o)	((o)->quan > 1 || \
			 (o)->oartifact == ART_EYES_OF_THE_OVERWORLD)

/* Flags for get_obj_location(). */
#define CONTAINED_TOO	0x1
#define BURIED_TOO	0x2

#endif /* OBJ_H */
