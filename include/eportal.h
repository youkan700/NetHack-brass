/*	SCCS Id: @(#)emin.h	3.4	1997/05/01	*/
/* Copyright (c) David Cohrs, 1990.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EPORTAL_H
#define EPORTAL_H

struct eportal_dest {
	d_level	dlev;		/* dungeon & level of the destination */
	xchar	dx, dy;		/* coodinate of the destination */
};

struct eportal {
	int			num_slots;	/* number of usable slots */
	struct	eportal_dest	dests[8];	/* destinations */
};

#define EPORTAL(obj)	((struct eportal *)get_xdat_obj(obj,XDAT_EPORTAL))

#define MAX_EPORTAL_SLOT 8

#endif /* EMIN_H */
