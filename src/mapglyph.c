/*	SCCS Id: @(#)mapglyph.c	3.4	2003/01/08	*/
/* Copyright (c) David Cohrs, 1991				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#if defined(TTY_GRAPHICS)
#include "wintty.h"	/* for prototype of has_color() only */
#endif
#include "color.h"
#define HI_DOMESTIC CLR_WHITE	/* monst.c */

int explcolors[] = {
	CLR_BLACK,	/* dark    */
	CLR_GREEN,	/* noxious */
	CLR_BROWN,	/* muddy   */
	CLR_BLUE,	/* wet     */
	CLR_MAGENTA,	/* magical */
	CLR_ORANGE,	/* fiery   */
	CLR_WHITE,	/* frosty  */
};

#if !defined(TTY_GRAPHICS)
#define has_color(n)  TRUE
#endif

#ifdef TEXTCOLOR
#define zap_color(n)  color = iflags.use_color ? zapcolors[n] : NO_COLOR
#define cmap_color(n) color = iflags.use_color ? defsyms[n].color : NO_COLOR
#define obj_color(n)  color = iflags.use_color ? objects[n].oc_color : NO_COLOR
#define mon_color(n)  color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define invis_color(n) color = NO_COLOR
#define pet_color(n)  color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define warn_color(n) color = iflags.use_color ? def_warnsyms[n].color : NO_COLOR
#define explode_color(n) color = iflags.use_color ? explcolors[n] : NO_COLOR
#define mat_color(n)  color = iflags.use_color ? materialcolor[n] : NO_COLOR
#define std_color(n)  color = iflags.use_color ? (n) : NO_COLOR
# if defined(REINCARNATION) && defined(ASCIIGRAPH)
#  define ROGUE_COLOR
# endif

#else	/* no text color */

#define zap_color(n)
#define cmap_color(n)
#define obj_color(n)
#define mon_color(n)
#define invis_color(n)
#define pet_color(c)
#define warn_color(n)
#define explode_color(n)
#define mat_color(n)
#define std_color(n)
#endif

#ifdef ROGUE_COLOR
# if defined(USE_TILES) && defined(MSDOS)
#define HAS_ROGUE_IBM_GRAPHICS (iflags.IBMgraphics && !iflags.grmode && \
	Is_rogue_level(&u.uz))
# else
#define HAS_ROGUE_IBM_GRAPHICS (iflags.IBMgraphics && Is_rogue_level(&u.uz))
# endif
#endif

/*ARGSUSED*/
void
mapglyph(glyph, ochar, ocolor, ospecial, x, y)
int glyph, *ocolor, x, y;
int *ochar;
unsigned *ospecial;
{
	register int offset;
#if defined(TEXTCOLOR) || defined(ROGUE_COLOR)
	int color = NO_COLOR;
#endif
	uchar ch;
	unsigned special = 0;
	int tmp;

    if (glyph & GLYPH_ATTR_GOODPOS) special |= MG_GOODPOS;

    /*
     *  Map the glyph back to a character and color.
     *
     *  Warning:  For speed, this makes an assumption on the order of
     *		  offsets.  The order is set in display.h.
     */
    switch (glyph & GLYPH_TYPE_MASK) {

	/* a monster */
	case GLYPH_TYPE_MON:
	    offset = (glyph & GLYPH_MAIN_MASK);
	    ch = monsyms[(int)mons[offset].mlet];
#ifdef ROGUE_COLOR
	    if (HAS_ROGUE_IBM_GRAPHICS && iflags.use_color) {
		if (x == u.ux && y == u.uy)
		    /* actually player should be yellow-on-gray if in a corridor */
		    color = CLR_YELLOW;
		else
		    color = NO_COLOR;
	    } else
#endif
	    {
		mon_color(offset);
		/* special case the hero for `showrace' option */
#ifdef TEXTCOLOR
		if (iflags.use_color && x == u.ux && y == u.uy &&
		    iflags.showrace && !Upolyd)
		    color = HI_DOMESTIC;
#endif
	    }
	    if (glyph & GLYPH_ATTR_RIDING) special |= MG_RIDING;
	    if (glyph & GLYPH_ATTR_FRIEND) special |= MG_PET;
	    if (glyph & GLYPH_ATTR_DETECT) special |= MG_DETECT;
	    break;

	/* a corpse */
	case GLYPH_TYPE_BODY:
	    offset = (glyph & GLYPH_MAIN_MASK);
	    ch = oc_syms[(int)objects[CORPSE].oc_class];
#ifdef ROGUE_COLOR
	    if (HAS_ROGUE_IBM_GRAPHICS && iflags.use_color)
		color = CLR_RED;
	    else
#endif
		mon_color(offset);
	    special |= MG_CORPSE;
	    break;

	/* a statue */
	case GLYPH_TYPE_STATUE:
	    offset = (glyph & GLYPH_MAIN_MASK);
	    ch = monsyms[(int)mons[offset].mlet];
#ifdef ROGUE_COLOR
	    if (HAS_ROGUE_IBM_GRAPHICS)
		color = NO_COLOR;	/* no need to check iflags.use_color */
	    else
#endif
	    {
		/* check if color is overriden by material patch etc. */
		tmp = ((glyph & GLYPH_SUB_MASK) >> GLYPH_SUB_SHIFT);
		if (tmp == 0)
		    obj_color(STATUE);
		else if (tmp <= MAX_MATERIAL)
		    mat_color(tmp);
		else if (tmp >= 32 && tmp < (32+CLR_MAX))
		    std_color(tmp - 32);
	    }
	    if (glyph & GLYPH_ATTR_RIDING) special |= MG_RIDING;
	    if (glyph & GLYPH_ATTR_FRIEND) special |= MG_PET;
	    if (glyph & GLYPH_ATTR_DETECT) special |= MG_DETECT;
	    break;

	/* swallow */
	case GLYPH_TYPE_SWALLOW:
	    /* see swallow_to_glyph() in display.c */
	    offset = (glyph & GLYPH_MAIN_MASK);
	    tmp =   ((glyph & GLYPH_SUB_MASK) >> GLYPH_SUB_SHIFT);
	    ch = (uchar) showsyms[S_sw_tl + tmp];
#ifdef ROGUE_COLOR
	    if (HAS_ROGUE_IBM_GRAPHICS && iflags.use_color)
		color = NO_COLOR;
	    else
#endif
		mon_color(offset);
	    break;

	/* object */
	case GLYPH_TYPE_OBJ:
	    offset = (glyph & GLYPH_MAIN_MASK);
	    if (offset == BOULDER && iflags.bouldersym) ch = iflags.bouldersym;
	    else ch = oc_syms[(int)objects[offset].oc_class];
#ifdef ROGUE_COLOR
	    if (HAS_ROGUE_IBM_GRAPHICS && iflags.use_color) {
		switch(objects[offset].oc_class) {
		    case COIN_CLASS: color = CLR_YELLOW; break;
		    case FOOD_CLASS: color = CLR_RED; break;
		    default: color = (iflags.IBMrogue ? CLR_BRIGHT_BLUE : CLR_CYAN); break;
		}
	    } else
#endif
	    {
		tmp = ((glyph & GLYPH_SUB_MASK) >> GLYPH_SUB_SHIFT);
		if (tmp == 0)
		    obj_color(offset);
		else if (tmp <= MAX_MATERIAL)
		    mat_color(tmp);
		else if (tmp >= 32 && tmp < (32+CLR_MAX))
		    std_color(tmp - 32);
	    }
	    if (glyph & GLYPH_ATTR_RIDING) special |= MG_RIDING;
	    if (glyph & GLYPH_ATTR_FRIEND) special |= MG_PET;
	    if (glyph & GLYPH_ATTR_DETECT) special |= MG_DETECT;
	    break;

	case GLYPH_TYPE_OTHERS:
	    offset = (glyph & GLYPH_MAIN_MASK);
	    tmp =   ((glyph & GLYPH_SUB_MASK) >> GLYPH_SUB_SHIFT);
	    if (offset < GLYPH_EXPLODE_OFF) {

		/* cmap */
		ch = showsyms[offset];
#ifdef ROGUE_COLOR
		if (HAS_ROGUE_IBM_GRAPHICS && iflags.use_color) {
		    if (offset >= S_vwall && offset <= S_hcdoor)
			color = CLR_BROWN;
		    else if (offset >= S_arrow_trap && offset <= S_polymorph_trap)
			color = CLR_MAGENTA;
		    else if (offset == S_corr || offset == S_litcorr)
			color = CLR_GRAY;
		    else if (!iflags.IBMrogue && (offset == S_upstair || offset == S_dnstair))
			color = CLR_BROWN;
		    else if (offset >= S_room && offset <= S_water)
			color = CLR_GREEN;
		    else
			color = NO_COLOR;
		} else
#endif
#ifdef TEXTCOLOR
		    /* provide a visible difference if normal and lit corridor
		     * use the same symbol */
		    if (iflags.use_color &&
			offset == S_litcorr && ch == showsyms[S_corr])
			color = CLR_WHITE;
		    else
#endif
			cmap_color(offset);

	    } else if (offset < GLYPH_ZAP_OFF) {

		/* explosion */
		offset -= GLYPH_EXPLODE_OFF;
		ch = showsyms[(offset % MAXEXPCHARS) + S_explode1];
		explode_color(offset / MAXEXPCHARS);

	    } else if (offset < GLYPH_WARNING_OFF) {

		/* see zapdir_to_glyph() in display.c */
		offset -= GLYPH_ZAP_OFF;
		ch = showsyms[S_vbeam + (offset & 0xf)];
#ifdef ROGUE_COLOR
		if (HAS_ROGUE_IBM_GRAPHICS && iflags.use_color)
		    color = NO_COLOR;
		else
#endif
		    zap_color((offset >> 4));

	    } else if (offset < GLYPH_INVIS_OFF) {

		/* a warning flash */
		ch = warnsyms[tmp];
# ifdef ROGUE_COLOR
		if (HAS_ROGUE_IBM_GRAPHICS)
		    color = NO_COLOR;
		else
# endif
		    warn_color(tmp);

	    } else if (offset < GLYPH_CLOUD_OFF) {

		/* invisible */
		ch = DEF_INVISIBLE;
#ifdef ROGUE_COLOR
		if (HAS_ROGUE_IBM_GRAPHICS)
		    color = NO_COLOR;	/* no need to check iflags.use_color */
		else
#endif
		    invis_color(offset);
		special |= MG_INVIS;

	    } else if (offset < GLYPH_FIREBALL_OFF) {

		/* gas cloud */
		offset -= GLYPH_CLOUD_OFF;
		ch = (uchar) showsyms[S_thinpcloud + offset];
# ifdef ROGUE_COLOR
		if (HAS_ROGUE_IBM_GRAPHICS) {
		    ch = '#';
		    color = NO_COLOR;
		} else
# endif
		    cmap_color(S_thinpcloud + offset);

	    } else {

		/* fireball */
		offset -= GLYPH_FIREBALL_OFF;
		ch = (uchar) showsyms[S_fireball];
# ifdef ROGUE_COLOR
		if (HAS_ROGUE_IBM_GRAPHICS) {
		    color = NO_COLOR;
		} else
# endif
		    zap_color(offset);

	    }
	    break;

	default:
	    impossible("Unexpected glyph: %08X", glyph);
	    ch = '?';
	    break;
    }

#ifdef TEXTCOLOR
    /* Turn off color if no color defined, or rogue level w/o PC graphics. */
# ifdef REINCARNATION
#  ifdef ASCIIGRAPH
    if (!has_color(color) || (Is_rogue_level(&u.uz) && !HAS_ROGUE_IBM_GRAPHICS))
#  else
    if (!has_color(color) || Is_rogue_level(&u.uz))
#  endif
# else
    if (!has_color(color))
# endif
	color = NO_COLOR;
#endif

    *ochar = (int)ch;
    *ospecial = special;
#ifdef TEXTCOLOR
    *ocolor = color;
#endif
    return;
}

/*mapglyph.c*/
