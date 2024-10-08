/*	SCCS Id: @(#)artilist.h 3.4	2003/02/12	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef MAKEDEFS_C
/* in makedefs.c, all we care about is the list of names */

#define A(nam,typ,mat,s1,s2,mt,atk,dfn,cry,inv,al,cl,rac,cost) nam

static const char *artifact_names[] = {
#else
/* in artifact.c, set up the actual artifact list structure */

#define A(nam,typ,mat,s1,s2,mt,atk,dfn,cry,inv,al,cl,rac,cost) \
 { typ, nam, s1, s2, mt, atk, dfn, cry, inv, mat, al, cl, rac, cost }

#define     NO_ATTK	{0,0,0,0}		/* no attack */
#define     NO_DFNS	{0,0,0,0}		/* no defense */
#define     NO_CARY	{0,0,0,0}		/* no carry effects */
#define     DFNS(c)	{0,c,0,0}
#define     CARY(c)	{0,c,0,0}
#define     PHYS(a,b)	{0,AD_PHYS,a,b}		/* physical */
#define     DRLI(a,b)	{0,AD_DRLI,a,b}		/* life drain */
#define     COLD(a,b)	{0,AD_COLD,a,b}
#define     FIRE(a,b)	{0,AD_FIRE,a,b}
#define     ELEC(a,b)	{0,AD_ELEC,a,b}		/* electrical shock */
#define     STUN(a,b)	{0,AD_STUN,a,b}		/* magical attack */

STATIC_OVL NEARDATA struct artifact artilist[] = {
#endif	/* MAKEDEFS_C */

/* Artifact cost rationale:
 * 1.  The more useful the artifact, the better its cost.
 * 2.  Quest artifacts are highly valued.
 * 3.  Chaotic artifacts are inflated due to scarcity (and balance).
 */

/* Arc	---		*/
/* Bar	Cleaver		*/
/* Cav	Skullcrusher	*/
/* Hea	---		*/
/* Kni	Excalibur	*/
/* Mon	---		*/
/* Pri	Disrupter	*/
/* Ran	---		*/
/* Rog	---		*/
/* Sam	Snickersnee	*/
/* Tou	---		*/
/* Val	Mjollnir	*/
/* Wiz	Magicbane	*/

/*  dummy element #0, so that all interesting indices are non-zero */
A("",				STRANGE_OBJECT,	0,
	0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 0L ),

A("Excalibur",			LONG_SWORD,	METAL,
	(SPFX_NOGEN|SPFX_SEEK|SPFX_DEFN|SPFX_INTEL|SPFX_SEARCH),0,0,
	PHYS(5,10),	DRLI(0,0),	NO_CARY,	0, A_LAWFUL, PM_KNIGHT, NON_PM, 4000L ),
/*
 *	Stormbringer only has a 2 because it can drain a level,
 *	providing 8 more.
 */
A("Stormbringer",		RUNESWORD,	0,
	(SPFX_ATTK|SPFX_DEFN|SPFX_INTEL), 0, 0,
	DRLI(5,2),	DRLI(0,0),	NO_CARY,	0, A_CHAOTIC, NON_PM, NON_PM, 8000L ),
/*
 *	Mjollnir will return to the hand of the wielder when thrown
 *	if the wielder is a Valkyrie wearing Gauntlets of Power.
 */
A("Mjollnir",			HEAVY_HAMMER,	0,	/* Mjo:llnir */
	(SPFX_ATTK),  0, 0,
	ELEC(0,20),	NO_DFNS,	NO_CARY,	0, A_NEUTRAL, PM_VALKYRIE, NON_PM, 4000L ),

A("Cleaver",			BATTLE_AXE,	0,
	0, 0, 0,
	PHYS(3,10),	NO_DFNS,	NO_CARY,	0, A_NEUTRAL, PM_BARBARIAN, NON_PM, 1500L ),

A("Grimtooth",			ORCISH_DAGGER,	0,
	0, 0, 0,
	PHYS(2,6),	NO_DFNS,	NO_CARY,	POISON_BLADE, A_CHAOTIC, NON_PM, PM_ORC, 300L ),
/*
 *	Orcrist and Sting have same alignment as elves.
 */
A("Orcrist",			ELVEN_BROADSWORD,	MITHRIL,
	(SPFX_WARN|SPFX_DFLAG2), 0, M2_ORC,
	PHYS(15,ADMG_DOUBLE),	NO_DFNS,	NO_CARY,	0, A_CHAOTIC, NON_PM, PM_ELF, 2000L ),

/*
 *	The combination of SPFX_WARN and M2_something on an artifact
 *	will trigger EWarn_of_mon for all monsters that have the appropriate
 *	M2_something flags.  In Sting's case it will trigger EWarn_of_mon
 *	for M2_ORC monsters.
 */
A("Sting",			ELVEN_DAGGER,	MITHRIL,
	(SPFX_WARN|SPFX_DFLAG2), 0, M2_ORC,
	PHYS(15,ADMG_DOUBLE),	NO_DFNS,	NO_CARY,	0, A_CHAOTIC, NON_PM, PM_ELF, 800L ),
/*
 *	Magicbane is a bit different!  Its magic fanfare
 *	unbalances victims in addition to doing some damage.
 */
A("Magicbane",			ATHAME,	0,
	(SPFX_ATTK|SPFX_DEFN), 0, 0,
	STUN(3,4),	DFNS(AD_MAGM),	NO_CARY,	0, A_NEUTRAL, PM_WIZARD, NON_PM, 3500L ),

A("Disrupter",			MACE,	SILVER,
	(SPFX_DFLAG2), 0, M2_UNDEAD,
	PHYS(10,30),	NO_DFNS,	NO_CARY,	0, A_NONE, PM_PRIEST, NON_PM, 2000L ),

A("Skullcrusher",			CLUB,	BONE,
	(SPFX_ATTK), 0, 0,
	PHYS(3,15),	NO_DFNS,	NO_CARY,	0, A_NONE, PM_CAVEMAN, NON_PM, 1000L ),

A("Frost Brand",		LONG_SWORD,	0,
	(SPFX_ATTK|SPFX_DEFN), 0, 0,
	COLD(5,ADMG_DOUBLE),	COLD(0,0),	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 3000L ),

A("Fire Brand",			LONG_SWORD,	0,
	(SPFX_ATTK|SPFX_DEFN), 0, 0,
	FIRE(5,ADMG_DOUBLE),	FIRE(0,0),	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 3000L ),

A("Dragonslayer",		BROADSWORD,	0,
	(SPFX_DCLAS|SPFX_REFLECT), 0, S_DRAGON,
	PHYS(10,ADMG_DOUBLE),	NO_DFNS,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 2500L ),

A("Demonbane",			LONG_SWORD,	SILVER,
	(SPFX_DFLAG2), 0, M2_DEMON,
	PHYS(10,ADMG_DOUBLE),	NO_DFNS,	NO_CARY,	0, A_LAWFUL, NON_PM, NON_PM, 2500L ),

A("Werebane",			REVOLVER,	0,
	(SPFX_DFLAG2), 0, M2_WERE,
	PHYS(20,ADMG_DOUBLE),	DFNS(AD_WERE),	NO_CARY,
	CREATE_AMMO, A_NONE, NON_PM, NON_PM, 5000L ),
A("Grayswandir",		SABER,		SILVER,
	(SPFX_HALRES), 0, 0,
	PHYS(3,12),	NO_DFNS,	NO_CARY,	0, A_LAWFUL, NON_PM, NON_PM, 8000L ),

A("Quick Blade",		ELVEN_SHORT_SWORD,	0,
	0, 0, 0,
	PHYS(10,0),	NO_DFNS,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 1500L ),

A("Giantkiller",		TWO_HANDED_SWORD,	0,
	(SPFX_DFLAG2), 0, M2_GIANT,
	PHYS(15,ADMG_DOUBLE),	NO_DFNS,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 200L ),

A("Ogresmasher",		LUCERN_HAMMER,	0,
	(SPFX_DCLAS), 0, S_OGRE,
	PHYS(15,ADMG_DOUBLE),	NO_DFNS,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 200L ),

A("Trollsbane",			MORNING_STAR,	0,
	(SPFX_DCLAS), 0, S_TROLL,
	PHYS(15,ADMG_DOUBLE),	NO_DFNS,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 800L ),
/*
 *	Two problems:  1) doesn't let trolls regenerate heads,
 *	2) doesn't give unusual message for 2-headed monsters (but
 *	allowing those at all causes more problems than worth the effort).
 */
A("Vorpal Blade",		LONG_SWORD,	0,
	(SPFX_BEHEAD), 0, 0,
	PHYS(15,0),	NO_DFNS,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 4000L ),
/*
 *	Ah, never shall I forget the cry,
 *		or the shriek that shrieked he,
 *	As I gnashed my teeth, and from my sheath
 *		I drew my Snickersnee!
 *			--Koko, Lord high executioner of Titipu
 *			  (From Sir W.S. Gilbert's "The Mikado")
 */
A("Snickersnee",		KATANA,	0,
	0, 0, 0,
	PHYS(10,8),	NO_DFNS,	NO_CARY,	0, A_LAWFUL, PM_SAMURAI, NON_PM, 1200L ),

A("Sunsword",			LONG_SWORD,	0,
	(SPFX_DFLAG2), 0, M2_UNDEAD,
	PHYS(10,ADMG_DOUBLE),	DRLI(0,0)/*DFNS(AD_BLND)*/,	NO_CARY,
	EMIT_LIGHT, A_LAWFUL, NON_PM, NON_PM, 1500L ),
/*
 *	Artifact gloves for Monks
 */
A("Gauntlets of Defense",	GAUNTLETS_OF_DEXTERITY,	0,
	(SPFX_HPHDAM|SPFX_INTEL), 0, 0,
	NO_ATTK,	NO_DFNS,	NO_CARY,
	0,		A_CROSSALIGNED, PM_MONK, NON_PM, 2000L ),

A("Fist of Fury",		GAUNTLETS,	SILVER,
	(SPFX_INTEL), 0,	0,
	PHYS(5,10),	NO_DFNS,	NO_CARY,
	0,		A_CROSSALIGNED, PM_MONK, NON_PM, 2000L ),

A("Ebony Lacquered Bow",         YUMI,	0,
	(SPFX_SEARCH), 0, 0,
	PHYS(5,0),	NO_DFNS,	NO_CARY,
	CREATE_AMMO,	A_LAWFUL, PM_MEDIUM, NON_PM, 2000L ),

/*
 *	The artifacts for the quest dungeon, all self-willed.
 */

A("The Orb of Detection",	CRYSTAL_BALL,	0,
	(SPFX_NOGEN|SPFX_INTEL), (SPFX_ESP|SPFX_HSPDAM), 0,
	NO_ATTK,	NO_DFNS,	CARY(AD_MAGM),
	INVIS,		A_LAWFUL, PM_ARCHEOLOGIST, NON_PM, 2500L ),

A("The Heart of Ahriman",	RUBY,	0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_LUCK), (SPFX_STLTH|SPFX_LWILL), 0,
	/* this stone does double damage if used as a projectile weapon */
	PHYS(5,0),	NO_DFNS,	NO_CARY,
	0,		A_NEUTRAL, PM_BARBARIAN, NON_PM, 2500L ),

A("The Sceptre of Might",	MACE,	0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_DALIGN), 0, 0,
	PHYS(5,ADMG_DOUBLE),	NO_DFNS,	CARY(AD_MAGM),
	CONFLICT,	A_LAWFUL, PM_CAVEMAN, NON_PM, 2500L ),

#if 0	/* OBSOLETE */
A("The Palantir of Westernesse",	CRYSTAL_BALL,	0,
	(SPFX_NOGEN|SPFX_INTEL),
		(SPFX_ESP|SPFX_REGEN|SPFX_HSPDAM), 0,
	NO_ATTK,	NO_DFNS,	NO_CARY,
	TAMING,		A_CHAOTIC, NON_PM , PM_ELF, 8000L ),
#endif

A("The Staff of Aesculapius",	QUARTERSTAFF,	0,
	(SPFX_NOGEN|SPFX_ATTK|SPFX_INTEL|SPFX_REGEN), 0,0,
	DRLI(0,ADMG_DOUBLE),	DRLI(0,0),	NO_CARY,
	HEALING,	A_NEUTRAL, PM_HEALER, NON_PM, 5000L ),

A("The Magic Mirror of Merlin", MIRROR,	0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_SPEAK), SPFX_ESP, 0,
	NO_ATTK,	NO_DFNS,	CARY(AD_MAGM),
	0,		A_LAWFUL, PM_KNIGHT, NON_PM, 1500L ),

#if !defined(LENSES) && defined(MAGIC_GLASSES)
#define LENSES GLASSES_OF_TRUE_SIGHT
#endif
A("The Eyes of the Overworld",	LENSES, 0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_XRAY), 0, 0,
	NO_ATTK,	NO_DFNS,	CARY(AD_MAGM),
	ENLIGHTENING,	A_NEUTRAL,	 PM_MONK, NON_PM, 2500L ),

A("The Holy Dagger",	KNIFE,	SILVER,
	(SPFX_NOGEN|SPFX_REFLECT|SPFX_INTEL|SPFX_DFLAG2), SPFX_HPHDAM, M2_UNDEAD,
	PHYS(5,ADMG_DOUBLE),	NO_DFNS,	CARY(AD_MAGM),
	0,	A_LAWFUL, PM_MEDIUM, NON_PM, 1500L ),

A("The Mitre of Holiness",	HELM_OF_BRILLIANCE,	0,
	(SPFX_NOGEN|SPFX_DFLAG2|SPFX_INTEL), 0, M2_UNDEAD,
	NO_ATTK,	NO_DFNS,	CARY(AD_FIRE),
	ENERGY_BOOST,	A_LAWFUL, PM_PRIEST, NON_PM, 2000L ),

A("The Longbow of Diana", BOW, 0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_REFLECT), SPFX_ESP, 0,
	PHYS(5,3),	NO_DFNS,	NO_CARY,
	CREATE_AMMO, A_CHAOTIC, PM_RANGER, NON_PM, 4000L ),

A("The Master Key of Thievery", SKELETON_KEY, 0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_SPEAK),
		(SPFX_WARN|SPFX_TCTRL|SPFX_HPHDAM), 0,
	NO_ATTK,	NO_DFNS,	NO_CARY,
	UNTRAP,		A_CHAOTIC, PM_ROGUE, NON_PM, 3500L ),

A("The Tsurugi of Muramasa",	TSURUGI, 0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_BEHEAD|SPFX_LUCK), 0, 0,
	PHYS(0,8),	NO_DFNS,	NO_CARY,
	0,		A_LAWFUL, PM_SAMURAI, NON_PM, 4500L ),

A("The Platinum Yendorian Express Card", CREDIT_CARD, 0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_DEFN),
		(SPFX_ESP|SPFX_HSPDAM), 0,
	NO_ATTK,	NO_DFNS,	CARY(AD_MAGM),
	CHARGE_OBJ,	A_NEUTRAL, PM_TOURIST, NON_PM, 7000L ),

A("The Orb of Fate",		CRYSTAL_BALL, 0,
	(SPFX_NOGEN|SPFX_INTEL|SPFX_LUCK),
		(SPFX_WARN|SPFX_HSPDAM|SPFX_HPHDAM), 0,
	NO_ATTK,	NO_DFNS,	NO_CARY,
	LEV_TELE,	A_NEUTRAL, PM_VALKYRIE, NON_PM, 3500L ),

A("The Eye of the Aethiopica",	AMULET_OF_ESP, 0,
	(SPFX_NOGEN|SPFX_INTEL), (SPFX_EREGEN|SPFX_HSPDAM), 0,
	NO_ATTK,	NO_DFNS,	CARY(AD_MAGM),
	CREATE_PORTAL,	A_NEUTRAL, PM_WIZARD, NON_PM, 4000L ),

/*
 *  terminator; otyp must be zero
 */
A(0, 0, 0, 0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 0L )

};	/* artilist[] (or artifact_names[]) */

#undef	A

#ifndef MAKEDEFS_C
#undef	NO_ATTK
#undef	NO_DFNS
#undef	DFNS
#undef	PHYS
#undef	DRLI
#undef	COLD
#undef	FIRE
#undef	ELEC
#undef	STUN
#endif

/*artilist.h*/
