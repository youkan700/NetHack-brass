/*	SCCS Id: @(#)epri.h	3.4	1997/05/01	*/
/* Copyright (c) Izchak Miller, 1989.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EPRI_H
#define EPRI_H

struct epri {
	schar shroom;		/* index in rooms */
	coord shrpos;		/* position of shrine */
	d_level shrlevel;	/* level (& dungeon) of shrine */
};

#define EPRI(mon)	((struct epri *)get_xdat_mon(mon,XDAT_EPRI))

/* A priest without ispriest is a roaming priest without a shrine, so
 * the fields (except shralign, which becomes only the priest alignment)
 * are available for reuse.
 */
#define renegade shroom

#endif /* EPRI_H */
