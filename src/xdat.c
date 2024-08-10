/*	SCCS Id: @(#)japanese.c	3.4	2003/12/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "edog.h"
#include "epri.h"
#include "eshk.h"
#include "eportal.h"
#include "vault.h"

/*size of data chunk = oxlth, mxlth*/

#define ALIGNMENT 4

STATIC_DCL struct xdat *FDECL(new_xdat, (uchar, genericptr_t));
STATIC_DCL struct xdat *FDECL(extract_xdat, (uchar, struct xdat **));
STATIC_DCL void FDECL(dealloc_xdat, (struct xdat *));
STATIC_DCL genericptr_t FDECL(get_xdat, (struct xdat *, uchar));
STATIC_DCL void FDECL(del_xdat_all, (struct xdat *));

short xdat_reqsiz[] = {
	/* This must be aligned with XDAT_xxx defined in xdat.h */
	0,				/* XDAT_NONE */
	255, /*dummy*/			/* XDAT_NAME */
	sizeof(struct edog),		/* XDAT_EDOG */
	sizeof(struct epri),		/* XDAT_EPRI */
	sizeof(struct eshk),		/* XDAT_ESHK */
	sizeof(struct egd),		/* XDAT_EGD */
	sizeof(unsigned),		/* XDAT_M_ID */
	sizeof(struct permonst),	/* XDAT_PERMONST */
	sizeof(struct eportal)		/* XDAT_PORTAL */
};

/* create a new xdat with initial data (if any) */
STATIC_OVL struct xdat *
new_xdat(a_typ, dat)
uchar a_typ;
genericptr_t dat;
{
	struct xdat *xtmp;
	short siz;

	if (a_typ == XDAT_NAME) {
	    char *p = (char *)dat;
	    if (!p) {
		impossible("new_xdat(): Tried to name without a name?");
		p = "???";
	    }
#ifndef JP
	    siz = strlen(p) + 1;
	    if (siz > PL_PSIZ) siz = PL_PSIZ;
#else
	    siz = 1;
	    while (*p) {
		if (is_kanji(*p)) {
		    if (siz >= PL_PSIZ - 2) break;
		    siz += 2;
		    p += 2;
		} else {
		    if (siz >= PL_PSIZ - 1) break;
		    siz++;
		    p++;
		}
	    }
#endif
	} else {
	    siz = xdat_reqsiz[a_typ];
	}
	xtmp = (struct xdat *)alloc(XDATHSIZ + siz);
	xtmp->typ  = a_typ;
	xtmp->siz  = siz;
	xtmp->next = 0;

	if (dat) (void) memcpy((genericptr_t)xtmp->dat, dat, siz);

	if (a_typ == XDAT_NAME) {
	    char *p = (char *)(xtmp->dat);
	    p[siz - 1] = 0;
	}

	return xtmp;
}

/* Extract the given xdat type from the chain, following xdat chain. */
STATIC_OVL struct xdat *
extract_xdat(a_typ, head_ptr)
uchar a_typ;
struct xdat **head_ptr;
{
	struct xdat *curr, *prev;

	for (curr = *head_ptr; curr; curr = curr->next) {
	    if (curr->typ == a_typ) {
		*head_ptr = curr->next;
		break;
	    }
	    head_ptr = &(curr->next);
	}
	return curr;
}

STATIC_OVL void
dealloc_xdat(xtmp)
struct xdat *xtmp;
{
	free((genericptr_t)xtmp);
}

STATIC_OVL genericptr_t
get_xdat(xtmp, a_typ)
struct xdat *xtmp;
uchar a_typ;
{
	for (; xtmp; xtmp = xtmp->next)
	    if (xtmp->typ == a_typ)
		return (genericptr_t)(xtmp->dat);
	return 0;
}

STATIC_OVL void
del_xdat_all(xtmp)
struct xdat *xtmp;
{
	struct xdat *xnxt;
	while (xtmp) {
	    xnxt = xtmp->next;
	    dealloc_xdat(xtmp);
	    xtmp = xnxt;
	}
}

/*------------------------------------*/

/* Add a xdat chunk to a OBJ structure */
void
add_xdat_obj(otmp, a_typ, dat)
struct obj *otmp;
uchar a_typ;
genericptr_t dat;
{
	struct xdat *xtmp;
	if (!otmp) return;
	xtmp = new_xdat(a_typ, dat);
	xtmp->next = otmp->oextra;
	otmp->oextra = xtmp;
	if (a_typ == XDAT_NAME) otmp->has_name = 1;
}

/* Get a xdat chunk which has the specified XDAT type.
   Return value is a pointer to the contained data, not to its header */
genericptr_t
get_xdat_obj(otmp, a_typ)
struct obj *otmp;
uchar a_typ;
{
	if (!otmp->oextra) return 0;
	return get_xdat(otmp->oextra, a_typ);
}

/* Remove a xdat chunk which has the specified XDAT type. */
void
del_xdat_obj(otmp, a_typ)
struct obj *otmp;
uchar a_typ;
{
	struct xdat *xtmp;
	if (!otmp->oextra) return;
	if (!a_typ) {
	    /* all */
	    del_xdat_all(otmp->oextra);
	    otmp->oextra = 0;
	    otmp->has_name = 0;
	} else {
	    xtmp = extract_xdat(a_typ, &otmp->oextra);
	    if (xtmp) dealloc_xdat(xtmp);
	    if (a_typ == XDAT_NAME) otmp->has_name = 0;
	}
}

void
copy_xdat_obj(obj_dst, obj_src)
struct obj *obj_dst, *obj_src;
{
	struct xdat *xtmp;
	if (obj_dst->oextra) panic("copy_xdat_obj");
	for (xtmp = obj_src->oextra; xtmp; xtmp = xtmp->next)
	    add_xdat_obj(obj_dst, xtmp->typ, xtmp->dat);
}

int
namelth_xdat_obj(otmp)
struct obj *otmp;
{
	struct xdat *xtmp;
	for (xtmp = otmp->oextra; xtmp; xtmp = xtmp->next)
	    if (xtmp->typ == XDAT_NAME)
		return (int)(xtmp->siz); /* length of the name + EOS */
	return 0;
}

/* called from mergable() to check compatibility of
   attachments of given objs. Currently, any attachments
   are incompatible except names. */
boolean
compare_xdat_obj(obj1, obj2)
struct obj *obj1, *obj2;
{
	struct xdat *xtmp;
	char *nam1, *nam2;
	nam1 = nam2 = 0;
	for (xtmp = obj1->oextra; xtmp; xtmp = xtmp->next) {
	    if (xtmp->typ == XDAT_NAME) {
		nam1 = (char *)xtmp->dat;
		continue;
	    }
	    return FALSE;
	}
	for (xtmp = obj2->oextra; xtmp; xtmp = xtmp->next) {
	    if (xtmp->typ == XDAT_NAME) {
		nam2 = (char *)xtmp->dat;
		continue;
	    }
	    return FALSE;
	}
	if (nam1 && nam2) return !strcmp(nam1, nam2);
	return TRUE;
}

void
refresh_xdat_obj(otmp)
struct obj *otmp;
{
	otmp->has_name = !!get_xdat_obj(otmp, XDAT_NAME);
}

/*------------------------------------*/

/* Add a xdat chunk to a MONST structure. */
void
add_xdat_mon(mtmp, a_typ, dat)
struct monst *mtmp;
uchar a_typ;
genericptr_t dat;
{
	struct xdat *xtmp;
	if (!mtmp) return;
	xtmp = new_xdat(a_typ, dat);
	xtmp->next = mtmp->mextra;
	mtmp->mextra = xtmp;
	if      (a_typ == XDAT_NAME) mtmp->has_name = 1;
	else if (a_typ == XDAT_PERMONST) mtmp->data = (struct permonst *)(xtmp->dat);
}

/* Get a xdat chunk which has the specified XDAT type.
   Return value is a pointer to the contained data, not to its header */
genericptr_t
get_xdat_mon(mtmp, a_typ)
struct monst *mtmp;
uchar a_typ;
{
	if (!mtmp->mextra) return 0;
	return get_xdat(mtmp->mextra, a_typ);
}

/* Remove a xdat chunk which has the specified XDAT type. */
void
del_xdat_mon(mtmp, a_typ)
struct monst *mtmp;
uchar a_typ;
{
	struct xdat *xtmp;
	if (!mtmp->mextra) return;
	if (!a_typ) {
	    /* all */
	    del_xdat_all(mtmp->mextra);
	    mtmp->has_name = 0;
	} else {
	    xtmp = extract_xdat(a_typ, &mtmp->mextra);
	    if (xtmp) dealloc_xdat(xtmp);
	    if (a_typ == XDAT_NAME) mtmp->has_name = 0;
	}
}

void
copy_xdat_mon(mon_dst, mon_src)
struct monst *mon_dst, *mon_src;
{
	struct xdat *xtmp;
	for (xtmp = mon_src->mextra; xtmp; xtmp = xtmp->next)
	    add_xdat_mon(mon_dst, xtmp->typ, xtmp->dat);
}

void
refresh_xdat_mon(mtmp)
struct monst *mtmp;
{
	struct permonst *ptr;
	mtmp->has_name = !!get_xdat_mon(mtmp, XDAT_NAME);
	if ((ptr = (struct permonst *)get_xdat_mon(mtmp, XDAT_PERMONST)) != 0)
	    mtmp->data = ptr;
}

/*------------------------------------*/

/* save xdat chain */
void
savexdatchn(fd, xhp, mode)
int fd, mode;
struct xdat *xhp;
{
	struct xdat *xtmp;
	/* save the data */
	if (perform_bwrite(mode))
	    for (xtmp = xhp; xtmp; xtmp = xtmp->next)
		bwrite(fd, (genericptr_t) xtmp, XDATHSIZ + xtmp->siz);

	if (release_data(mode))
	    del_xdat_all(xhp);
}

struct xdat *
restxdatchn(fd, ghostly)
int fd;
boolean ghostly;
{
	struct xdat xh, *xhp, *xtmp, *xprev;
	int cnt;
	xhp = xprev = 0;
	/* restore the data */
	do {
	    /* first, read the xtra data header to get its size */
	    mread(fd, (genericptr_t) &xh, XDATHSIZ);
	    /* allocate the memory with (header + data) size */
	    xtmp = (struct xdat *)alloc(XDATHSIZ + xh.siz);
	    /* setup the header */
	    xtmp->typ  = xh.typ;
	    xtmp->siz  = xh.siz;
	    xtmp->next = 0;
	    /* read the body of the data */
	    mread(fd, (genericptr_t) xtmp->dat, xtmp->siz);
	    /* restore the link chain */
	    if (!xhp) xhp = xtmp;
	    if (xprev) xprev->next = xtmp;
	    xprev = xtmp;
	} while (xh.next);
	return xhp;
}

/*=====================================================================*/
#ifdef WIZARD
STATIC_DCL void FDECL(wiz_xdat_sub, (struct xdat *));

STATIC_OVL void
wiz_xdat_sub(xtmp)
struct xdat *xtmp;
{
	winid w;
	char buf[BUFSZ];
	static const char* xtypnam[] = {
	    "NONE", "NAME", "EDOG", "EPRI", "ESHK", "EGD", "M_ID", "PERMONST"
	};

	w = create_nhwindow(NHW_MENU);
	putstr(w, 0, "List of XDAT:");
	putstr(w, 0, "");
	while (xtmp) {
	    if (xtmp->typ == XDAT_NAME)
		Sprintf(buf, "Type: NAME, Size: %d, \'%s\'", xtmp->siz, (char *)xtmp->dat);
	    else
		Sprintf(buf, "Type: %s, Size: %d", xtypnam[xtmp->typ], xtmp->siz);
	    putstr(w, 0, buf);
	    xtmp = xtmp->next;
	}
	display_nhwindow(w, FALSE);
	destroy_nhwindow(w);
}

int
wiz_xdat()
{
	winid w;
	int i, n;
	int cnt;
	char buf[BUFSZ], *p;
	menu_item *selected;
	anything any;
	struct monst *mtmp;
	struct obj *otmp;
	boolean inv;

	switch (yn_function("Check extra data of MONST, OBJ or FLOOROBJ?", "mof", '\0')) {
	    case 'm':
		w = create_nhwindow(NHW_MENU);
		start_menu(w);
		cnt = 0;
		for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (mtmp->mextra) {
			any.a_void = (genericptr_t)mtmp;
			Sprintf(buf, "%s",
				x_monnam(mtmp, ARTICLE_NONE, (char *)0,
					 SUPPRESS_SADDLE|SUPPRESS_IT, FALSE));
			add_menu(w, NO_GLYPH, &any, 0, 0, ATR_NONE,
				 buf, MENU_UNSELECTED);
			cnt++;
		    }
		}
		end_menu(w, cnt ? "Pick a monster to check:" :
				  "No monster on this level has mextra data.");
		n = select_menu(w, PICK_ONE, &selected);
		destroy_nhwindow(w);
		if (n > 0) {
		    mtmp = (struct monst *)(selected[0].item.a_void);
		    free((genericptr_t)selected);
		    wiz_xdat_sub((struct xdat *)mtmp->mextra);
		}
		break;
	    case 'o':
		otmp = invent;
		inv = TRUE;
		goto show_objs;
	    case 'f':
		otmp = fobj;
		inv = FALSE;
show_objs:
		w = create_nhwindow(NHW_MENU);
		start_menu(w);
		cnt = 0;
		for (; otmp; otmp = otmp->nobj) {
		    if (otmp->oextra) {
			any.a_obj = otmp;
			Sprintf(buf, "%s", xname(otmp));
			add_menu(w, NO_GLYPH, &any, 0, 0, ATR_NONE,
				 buf, MENU_UNSELECTED);
			cnt++;
		    }
		}
		end_menu(w, cnt ? "Pick a object to check:" :
			    inv ? "No object in your inventory has oextra data." :
				  "No object on floor has oextra data.");
		n = select_menu(w, PICK_ONE, &selected);
		destroy_nhwindow(w);
		if (n > 0) {
		    otmp = selected[0].item.a_obj;
		    free((genericptr_t)selected);
		    wiz_xdat_sub((struct xdat *)otmp->oextra);
		}
		break;
	    default:
		return 0;
	}
	return 0;
}

#endif /*WIZARD*/

/*xdat.c*/
