/*      SCCS Id: @(#)tech.h    3.2     98/Oct/30                  */
/* Original Code by Warren Cheung                                 */
/* Copyright 1986, M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TECH_H
#define TECH_H

#define NO_TECH 	0
#define T_BERSERK 	1
#define T_KIII 		2
#define T_RESEARCH 	3
#define T_SURGERY 	4
#define T_FLURRY 	5
#define T_PRACTICE 	6
#define T_HEAL_HANDS 	7
#define T_CALM_STEED 	8
#define T_TURN_UNDEAD 	9
#define T_CUTTHROAT 	10
#define T_BLESSING 	11
#define T_CRIT_STRIKE 	12
#define T_DISARM 	13
#define T_MAINTENANCE 	14
#define T_IDENTIFY 	15

#define MAXTECH 16

#ifndef MAKEDEFS_C

/* An array of this struct holds your current techs */
struct tech {
	int         t_id;                  /* tech id 
					    * 	Unique number for the tech
					    *	see the #defines below
					    */
	xchar       t_lev;                 /* power level 
					    *   This is the level 
					    *   you were when you got the tech
				            */

	int         t_tout;                /* timeout - time until tech can be 
					    * used again 
					    */
	int         t_inuse;               /* time till effect stops 
					    */
	long        t_intrinsic;           /* Source(s) */
#define OUTSIDE_LEVEL	TIMEOUT            /* Level "from outside" started at */
};

struct innate_tech {
	schar   ulevel; /* gains tech at level */
	short   tech_id; /* the tech unique ID */ 
	int	tech_lev; /* starts at level */
};

struct tech_prop {
	const char *name;
	int         cost1;		/* cost1 >  0 : timeout rn1(cost1,cost2) */
	int         cost2;		/* cost1 == 0 ; timeout (cost2)          */
					/* cost1 <  0 : magic power (cost2)      */
};

#define TECH(nam, c1, c2) {nam, c1, c2}
static struct tech_prop tech_props[] = {

#else /* MAKEDEFS_C */

#define TECH(nam, c1, c2) nam
static const char *tech_names[] = {

#endif /* MAKEDEFS_C */

	TECH("no technique", 0, 0),
	TECH("berserk",        1000,  500),
	TECH("kiii",           1000,  500),
	TECH("research",        250,  250),
	TECH("surgery",        1000,  500),
	TECH("missile flurry", 1000,  500),
	TECH("examine weapon",  250,  250),
	TECH("healing hands",     0, 3000),
	TECH("calm steed",     1000,  500),
	TECH("turn undead",      -1,   25),
	TECH("cutthroat",      1000,  500),
	TECH("blessing",       1000,  500),
	TECH("critical strike",1000,  500),
	TECH("disarm",           -1,   20),
	TECH("maintain arms",   250,  250),
	TECH("identify",        500, 1500),
};

#endif /* TECH_H */
