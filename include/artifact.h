/*	SCCS Id: @(#)artifact.h 3.4	1995/05/31	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ARTIFACT_H
#define ARTIFACT_H

#define SPFX_NONE   0x00000000L	/* no special effects, just a bonus */
#define SPFX_NOGEN  0x00000001L	/* item is special, bequeathed by gods */
//SPFX_RESTR is obsolete, now all artifacts can't be named
//#define SPFX_RESTR  0x00000002L	/* item is restricted - can't be named */
#define SPFX_INTEL  0x00000004L	/* item is self-willed - intelligent */
#define SPFX_SPEAK  0x00000008L	/* item can speak (not implemented) */
#define SPFX_SEEK   0x00000010L	/* item helps you search for things */
#define SPFX_WARN   0x00000020L	/* item warns you of danger */
#define SPFX_ATTK   0x00000040L	/* item has a special attack (attk) */
#define SPFX_DEFN   0x00000080L	/* item has a special defence (defn) */
#define SPFX_LWILL  0x00000100L	/* levitate at will */
#define SPFX_SEARCH 0x00000200L	/* helps searching */
#define SPFX_BEHEAD 0x00000400L	/* beheads monsters */
#define SPFX_HALRES 0x00000800L	/* blocks hallucinations */
#define SPFX_ESP    0x00001000L	/* ESP (like amulet of ESP) */
#define SPFX_STLTH  0x00002000L	/* Stealth */
#define SPFX_REGEN  0x00004000L	/* Regeneration */
#define SPFX_EREGEN 0x00008000L	/* Energy Regeneration */
#define SPFX_HSPDAM 0x00010000L	/* 1/2 spell damage (on player) in combat */
#define SPFX_HPHDAM 0x00020000L	/* 1/2 physical damage (on player) in combat */
#define SPFX_TCTRL  0x00040000L	/* Teleportation Control */
#define SPFX_LUCK   0x00080000L	/* Increase Luck (like Luckstone) */
#define SPFX_DMONS  0x00100000L	/* attack bonus on one monster type */
#define SPFX_DCLAS  0x00200000L	/* attack bonus on monsters w/ symbol mtype */
#define SPFX_DFLAG1 0x00400000L	/* attack bonus on monsters w/ mflags1 flag */
#define SPFX_DFLAG2 0x00800000L	/* attack bonus on monsters w/ mflags2 flag */
#define SPFX_DALIGN 0x01000000L	/* attack bonus on non-aligned monsters  */
#define SPFX_DBONUS 0x01F00000L	/* attack bonus mask */
#define SPFX_XRAY   0x02000000L	/* gives X-RAY vision to player */
#define SPFX_REFLECT 0x04000000L /* Reflection */
#define SPFX_DPROP  0x08000000L /* prop in defn */


struct artifact {
	short	    otyp;
	short	    weight;	/* if artifact weighs differently */
	/*const*/ char  *name;
	unsigned long spfx;	/* special effect from wielding/wearing */
	unsigned long cspfx;	/* special effect just from carrying obj */
	unsigned long mtype;	/* monster type, symbol, or flag */
	struct attack attk, defn, cary;
	uchar	    inv_prop;	/* property obtained by invoking artifact */
	uchar	    material;	/* if artifact is made of special material */
	uchar	    reserved;
	aligntyp    alignment;	/* alignment of bequeathing gods */
	short	    role;	/* character role associated with */
	short	    race;	/* character race associated with */
	long        cost;	/* price when sold to hero (default 100 x base cost) */
	uchar       ego_desc;   /* ego description */
	uchar       ego_type;   /* ego type */
};

/* invoked properties with special powers */
#define TAMING		(LAST_PROP+1)
#define HEALING		(LAST_PROP+2)
#define ENERGY_BOOST	(LAST_PROP+3)
#define UNTRAP		(LAST_PROP+4)
#define CHARGE_OBJ	(LAST_PROP+5)
#define LEV_TELE	(LAST_PROP+6)
#define CREATE_PORTAL	(LAST_PROP+7)
#define ENLIGHTENING	(LAST_PROP+8)
#define CREATE_AMMO	(LAST_PROP+9)
#define EMIT_LIGHT	(LAST_PROP+10)
#define POISON_BLADE	(LAST_PROP+11)

/* additional alighment */
#define	A_CROSSALIGNED	(-127)

/* special damage bonus identifiers */
#define ADMG_DOUBLE	255
#define ADMG_MAX	254

/* ego and custom weapons */
#define ART_NONAME      (NROFARTIFACTS+1)
#define ART_CUSTOM      (NROFARTIFACTS+2)

#define SPFX_IDENTIFIED SPFX_NOGEN

#define EGO_NONE        0
#define EGO_SLICE       1
#define EGO_CRUSH       2
#define EGO_PIERCE      3
#define EGO_LAUNCHER    4
#define EGO_FIRE        5
#define EGO_COLD        6
#define EGO_ELEC        7
#define EGO_LIGHTWEIGHT 8
#define EGO_HIT         9

#endif /* ARTIFACT_H */
