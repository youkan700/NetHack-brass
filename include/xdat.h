/*	SCCS Id: @(#)epri.h	3.4	1997/05/01	*/
/* Copyright (c) Izchak Miller, 1989.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef XDAT_H
#define XDAT_H

struct xdat {
	struct	xdat *next;
	uchar	typ;		/* xdat id */
	uchar	reserved;
	short	siz;		/* size of data not including the header */
	long	dat[1];
};

#define XDATHSIZ (sizeof(struct xdat) - sizeof(long))

/* The order should be aligned with xdat_reqsiz() in xdat.c */
#define	XDAT_NONE	0
#define	XDAT_NAME	1
#define	XDAT_EDOG	2
#define	XDAT_EPRI	3
#define	XDAT_ESHK	4
#define	XDAT_EGD	5
#define XDAT_M_ID	6	/* monst id in oextra */
#define	XDAT_PERMONST	7

#endif /* XDAT_H */
