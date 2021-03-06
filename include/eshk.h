/*	SCCS Id: @(#)eshk.h	3.4	1997/05/01	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ESHK_H
#define ESHK_H

#define REPAIR_DELAY	5	/* minimum delay between shop damage & repair */

#define BILLSZ	200
#define MSTOLENSZ 10

struct bill_x {
	unsigned bo_id;
	boolean useup;
	long price;		/* price per unit */
	long bquan;		/* amount used up */
};
struct mstolen_x {
	unsigned o_id;
	unsigned m_id;
};

struct eshk {
	long robbed;		/* amount stolen by most recent customer */
	long credit;		/* amount credited to customer */
	long debit;		/* amount of debt for using unpaid items */
	long loan;		/* shop-gold picked (part of debit) */
	int shoptype;		/* the value of rooms[shoproom].rtype */
	schar shoproom;		/* index in rooms; set by inshop() */
	boolean dosweep;	/* shk starts collecting golds in shop if set */
	boolean following;	/* following customer since he owes us sth */
	boolean surcharge;	/* angry shk inflates prices */
	coord shk;		/* usual position shopkeeper */
	coord shd;		/* position shop door */
	coord floorgold;	/* target position where gold pieces are placed */
	d_level shoplevel;	/* level (& dungeon) of his shop */
	int billct;		/* no. of entries of bill[] in use */
	struct bill_x bill[BILLSZ];
	struct bill_x *bill_p;
	struct mstolen_x mstolen[MSTOLENSZ];
	int mstolen_i;
	int visitct;		/* nr of visits by most recent customer */
	char customer[PL_NSIZ]; /* most recent customer */
	char shknam[PL_NSIZ];
};

#define ESHK(mon)	((struct eshk *)get_xdat_mon(mon,XDAT_ESHK))

#define NOTANGRY(mon)	((mon)->mpeaceful)
#define ANGRY(mon)	(!NOTANGRY(mon))

#endif /* ESHK_H */
