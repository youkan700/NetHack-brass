/*	SCCS Id: @(#)spell.h	3.4	1995/06/01	*/
/* Copyright 1986, M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SPELL_H
#define SPELL_H

struct spell {
    short	sp_id;			/* spell id (== object.otyp) */
    long	sp_active;		/* remaining turns of active spell */
};

/* levels of memory destruction with a scroll of amnesia */
#define ALL_MAP		0x1
#define ALL_SPELLS	0x2

#define spellid(spell)		spl_book[spell].sp_id

#endif /* SPELL_H */
