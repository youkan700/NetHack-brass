/*	SCCS Id: @(#)pickup.c	3.4	2003/07/27	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *	Contains code for picking objects up, and container use.
 */

#include "hack.h"

STATIC_DCL void FDECL(simple_look, (struct obj *,BOOLEAN_P));
STATIC_DCL boolean FDECL(query_classes, (char *,boolean *,boolean *,
		const char *,struct obj *,BOOLEAN_P,BOOLEAN_P,int *));
STATIC_DCL void FDECL(check_here, (BOOLEAN_P));
STATIC_DCL boolean FDECL(n_or_more, (struct obj *));
STATIC_DCL boolean FDECL(all_but_uchain, (struct obj *));
#if 0 /* not used */
STATIC_DCL boolean FDECL(allow_cat_no_uchain, (struct obj *));
#endif
STATIC_DCL int FDECL(autopick, (struct obj*, int, menu_item **));
STATIC_DCL int FDECL(count_categories, (struct obj *,int));
STATIC_DCL long FDECL(carry_count,
		      (struct obj *,struct obj *,long,BOOLEAN_P,int *,int *));
STATIC_DCL int FDECL(lift_object, (struct obj *,struct obj *,long *,BOOLEAN_P));
STATIC_DCL boolean FDECL(mbag_explodes, (struct obj *,int));
STATIC_PTR int FDECL(in_container,(struct obj *));
STATIC_PTR int FDECL(ck_bag,(struct obj *));
STATIC_PTR int FDECL(out_container,(struct obj *));
STATIC_DCL long FDECL(mbag_item_gone, (int,struct obj *));
STATIC_DCL void FDECL(observe_quantum_cat, (struct obj *));
STATIC_DCL int FDECL(menu_loot, (int, struct obj *, BOOLEAN_P));
STATIC_DCL int FDECL(in_or_out_menu, (const char *,struct obj *, BOOLEAN_P, BOOLEAN_P));
STATIC_DCL boolean FDECL(able_to_loot, (int, int));
STATIC_DCL boolean FDECL(mon_beside, (int, int));
STATIC_DCL void NDECL(del_sokoprize);

/* define for query_objlist() and autopickup() */
#define FOLLOW(curr, flags) \
    (((flags) & BY_NEXTHERE) ? (curr)->nexthere : (curr)->nobj)

/*
 *  How much the weight of the given container will change when the given
 *  object is removed from it.  This calculation must match the one used
 *  by weight() in mkobj.c.
 */
#define DELTA_CWT(cont,obj)		\
    ((cont)->cursed ? (obj)->owt * 2 :	\
		      1 + ((obj)->owt / ((cont)->blessed ? 4 : 2)))
#define GOLD_WT(n)		(((n) + 50L) / 100L)
/* if you can figure this out, give yourself a hearty pat on the back... */
#define GOLD_CAPACITY(w,n)	(((w) * -100L) - ((n) + 50L) - 1L)

#ifndef JP
static const char moderateloadmsg[] = "You have a little trouble lifting";
static const char nearloadmsg[] = "You have much trouble lifting";
static const char overloadmsg[] = "You have extreme difficulty lifting";
#else
static const char moderateloadmsg[] = "を運ぶのは少々大変だ。";
static const char nearloadmsg[] = "を運ぶのはとても困難だ。";
static const char overloadmsg[] = "を運ぶのはほとんど不可能だ。";
#endif /*JP*/

/* BUG: this lets you look at cockatrice corpses while blind without
   touching them */
/* much simpler version of the look-here code; used by query_classes() */
STATIC_OVL void
simple_look(otmp, here)
struct obj *otmp;	/* list of objects */
boolean here;		/* flag for type of obj list linkage */
{
	/* Neither of the first two cases is expected to happen, since
	 * we're only called after multiple classes of objects have been
	 * detected, hence multiple objects must be present.
	 */
	if (!otmp) {
	    impossible("simple_look(null)");
	} else if (!(here ? otmp->nexthere : otmp->nobj)) {
	    pline("%s", doname(otmp));
	} else {
	    winid tmpwin = create_nhwindow(NHW_MENU);
	    putstr(tmpwin, 0, "");
	    do {
		putstr(tmpwin, 0, doname(otmp));
		otmp = here ? otmp->nexthere : otmp->nobj;
	    } while (otmp);
	    display_nhwindow(tmpwin, TRUE);
	    destroy_nhwindow(tmpwin);
	}
}

int
collect_obj_classes(ilets, otmp, here, incl_gold, filter, itemcount)
char ilets[];
register struct obj *otmp;
boolean here, incl_gold;
boolean FDECL((*filter),(OBJ_P));
int *itemcount;
{
	register int iletct = 0;
	register char c;

	*itemcount = 0;
	if (incl_gold)
	    ilets[iletct++] = def_oc_syms[COIN_CLASS];
	ilets[iletct] = '\0'; /* terminate ilets so that index() will work */
	while (otmp) {
	    c = def_oc_syms[(int)otmp->oclass];
	    if (!index(ilets, c) && (!filter || (*filter)(otmp)))
		ilets[iletct++] = c,  ilets[iletct] = '\0';
	    *itemcount += 1;
	    otmp = here ? otmp->nexthere : otmp->nobj;
	}

	return iletct;
}

/*
 * Suppose some '?' and '!' objects are present, but '/' objects aren't:
 *	"a" picks all items without further prompting;
 *	"A" steps through all items, asking one by one;
 *	"?" steps through '?' items, asking, and ignores '!' ones;
 *	"/" becomes 'A', since no '/' present;
 *	"?a" or "a?" picks all '?' without further prompting;
 *	"/a" or "a/" becomes 'A' since there aren't any '/'
 *	    (bug fix:  3.1.0 thru 3.1.3 treated it as "a");
 *	"?/a" or "a?/" or "/a?",&c picks all '?' even though no '/'
 *	    (ie, treated as if it had just been "?a").
 */
STATIC_OVL boolean
query_classes(oclasses, one_at_a_time, everything, action, objs,
	      here, incl_gold, menu_on_demand)
char oclasses[];
boolean *one_at_a_time, *everything;
const char *action;	/* "pick up", "take out", "put in" */
struct obj *objs;
boolean here, incl_gold;
int *menu_on_demand;
{
	char ilets[20], inbuf[BUFSZ];
	int iletct, oclassct;
	boolean not_everything;
	char qbuf[QBUFSZ];
	boolean m_seen;
	int itemcount;

	oclasses[oclassct = 0] = '\0';
	*one_at_a_time = *everything = m_seen = FALSE;
	iletct = collect_obj_classes(ilets, objs, here,
				     incl_gold,
				     (boolean FDECL((*),(OBJ_P))) 0, &itemcount);
	if (iletct == 0) {
		return FALSE;
	} else if (iletct == 1) {
		oclasses[0] = def_char_to_objclass(ilets[0]);
		oclasses[1] = '\0';
		if (itemcount && menu_on_demand) {
			ilets[iletct++] = 'm';
			*menu_on_demand = 0;
			ilets[iletct] = '\0';
		}
	} else  {	/* more than one choice available */
		const char *where = 0;
		register char sym, oc_of_sym, *p;
		/* additional choices */
		ilets[iletct++] = ' ';
		ilets[iletct++] = 'a';
		ilets[iletct++] = 'A';
		ilets[iletct++] = (objs == invent ? 'i' : ':');
		if (menu_on_demand) {
			ilets[iletct++] = 'm';
			*menu_on_demand = 0;
		}
		ilets[iletct] = '\0';
ask_again:
		oclasses[oclassct = 0] = '\0';
		*one_at_a_time = *everything = FALSE;
		not_everything = FALSE;
		Sprintf(qbuf,E_J("What kinds of thing do you want to %s? [%s]",
				"どの種類の品物を%sますか？ [%s]"),
			action, ilets);
		getlin(qbuf,inbuf);
		if (*inbuf == '\033') return FALSE;

		for (p = inbuf; (sym = *p++); ) {
		    /* new A function (selective all) added by GAN 01/09/87 */
		    if (sym == ' ') continue;
		    else if (sym == 'A') *one_at_a_time = TRUE;
		    else if (sym == 'a') *everything = TRUE;
		    else if (sym == ':') {
			simple_look(objs, here);  /* dumb if objs==invent */
			goto ask_again;
		    } else if (sym == 'i') {
			(void) display_inventory((char *)0, TRUE);
			goto ask_again;
		    } else if (sym == 'm') {
			m_seen = TRUE;
		    } else {
			oc_of_sym = def_char_to_objclass(sym);
			if (index(ilets,sym)) {
			    add_valid_menu_class(oc_of_sym);
			    oclasses[oclassct++] = oc_of_sym;
			    oclasses[oclassct] = '\0';
			} else {
			    if (!where)
#ifndef JP
				where = !strcmp(action,"pick up")  ? "here" :
					!strcmp(action,"take out") ?
							    "inside" : "";
#else
				where = !strcmp(action,"拾い")  ? "ここには" :
					!strcmp(action,"取り出し") ?
							    "その中には" : "";
#endif /*JP*/
			    if (*where)
#ifndef JP
				There("are no %c's %s.", sym, where);
#else
				pline("%s %c はない。", where, sym);
#endif /*JP*/
			    else
				You(E_J("have no %c's."," %c を持っていない。"), sym);
			    not_everything = TRUE;
			}
		    }
		}
		if (m_seen && menu_on_demand) {
			*menu_on_demand = (*everything || !oclassct) ? -2 : -3;
			return FALSE;
		}
		if (!oclassct && (!*everything || not_everything)) {
		    /* didn't pick anything,
		       or tried to pick something that's not present */
		    *one_at_a_time = TRUE;	/* force 'A' */
		    *everything = FALSE;	/* inhibit 'a' */
		}
	}
	return TRUE;
}

/* look at the objects at our location, unless there are too many of them */
STATIC_OVL void
check_here(picked_some)
boolean picked_some;
{
	register struct obj *obj;
	register int ct = 0;

	/* count the objects here */
	for (obj = level.objects[u.ux][u.uy]; obj; obj = obj->nexthere) {
	    if (obj != uchain)
		ct++;
	}

	/* If there are objects here, take a look. */
	if (ct) {
	    if (flags.run) nomul(0);
	    flush_screen(1);
	    (void) look_here(ct, picked_some);
	} else {
	    if (!is_skipreading_engr_at(u.ux, u.uy))
		read_engr_at(u.ux,u.uy);
	}
}

/* Value set by query_objlist() for n_or_more(). */
static long val_for_n_or_more;

/* query_objlist callback: return TRUE if obj's count is >= reference value */
STATIC_OVL boolean
n_or_more(obj)
struct obj *obj;
{
    if (obj == uchain) return FALSE;
    return (obj->quan >= val_for_n_or_more);
}

/* List of valid menu classes for query_objlist() and allow_category callback */
static char valid_menu_classes[MAXOCLASSES + 2];

void
add_valid_menu_class(c)
int c;
{
	static int vmc_count = 0;

	if (c == 0)  /* reset */
	  vmc_count = 0;
	else
	  valid_menu_classes[vmc_count++] = (char)c;
	valid_menu_classes[vmc_count] = '\0';
}

/* query_objlist callback: return TRUE if not uchain */
STATIC_OVL boolean
all_but_uchain(obj)
struct obj *obj;
{
    return (obj != uchain);
}

/* query_objlist callback: return TRUE */
/*ARGSUSED*/
boolean
allow_all(obj)
struct obj *obj;
{
    return TRUE;
}

boolean
allow_category(obj)
struct obj *obj;
{
    if (Role_if(PM_PRIEST)) obj->bknown = TRUE;
    if (((index(valid_menu_classes,'u') != (char *)0) && obj->unpaid) ||
	(index(valid_menu_classes, obj->oclass) != (char *)0))
	return TRUE;
    else if (((index(valid_menu_classes,'U') != (char *)0) &&
	(obj->oclass != COIN_CLASS && obj->bknown && !obj->blessed && !obj->cursed)))
	return TRUE;
    else if (((index(valid_menu_classes,'B') != (char *)0) &&
	(obj->oclass != COIN_CLASS && obj->bknown && obj->blessed)))
	return TRUE;
    else if (((index(valid_menu_classes,'C') != (char *)0) &&
	(obj->oclass != COIN_CLASS && obj->bknown && obj->cursed)))
	return TRUE;
    else if (((index(valid_menu_classes,'X') != (char *)0) &&
	(obj->oclass != COIN_CLASS && !obj->bknown)))
	return TRUE;
    else
	return FALSE;
}

#if 0 /* not used */
/* query_objlist callback: return TRUE if valid category (class), no uchain */
STATIC_OVL boolean
allow_cat_no_uchain(obj)
struct obj *obj;
{
    if ((obj != uchain) &&
	(((index(valid_menu_classes,'u') != (char *)0) && obj->unpaid) ||
	(index(valid_menu_classes, obj->oclass) != (char *)0)))
	return TRUE;
    else
	return FALSE;
}
#endif

/* query_objlist callback: return TRUE if valid class and worn */
boolean
is_worn_by_type(otmp)
register struct obj *otmp;
{
	return((boolean)(!!(otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_TOOL | W_WEP | W_SWAPWEP | W_QUIVER)))
	        && (index(valid_menu_classes, otmp->oclass) != (char *)0));
}

/*
 * Have the hero pick things from the ground
 * or a monster's inventory if swallowed.
 *
 * Arg what:
 *	>0  autopickup
 *	=0  interactive
 *	<0  pickup count of something
 *
 * Returns 1 if tried to pick something up, whether
 * or not it succeeded.
 */
int
pickup(what)
int what;		/* should be a long */
{
	int i, n, res, count, n_tried = 0, n_picked = 0;
	menu_item *pick_list = (menu_item *) 0;
	boolean autopickup = what > 0;
	struct obj *objchain;
	int traverse_how;

	if (what < 0)		/* pick N of something */
	    count = -what;
	else			/* pick anything */
	    count = 0;

	if (!u.uswallow) {
		struct trap *ttmp = t_at(u.ux, u.uy);
		/* no auto-pick if no-pick move, nothing there, or in a pool */
		if (autopickup && (flags.nopick || !OBJ_AT(u.ux, u.uy) ||
			(is_pool(u.ux, u.uy) && !Underwater) || is_lava(u.ux, u.uy))) {
			if (!is_skipreading_engr_at(u.ux, u.uy))
				read_engr_at(u.ux, u.uy);
			return (0);
		}

		/* no pickup if levitating & not on air or water level */
		if (!can_reach_floor()) {
		    if ((multi && !flags.run) || (autopickup && !flags.pickup))
			if (!is_skipreading_engr_at(u.ux, u.uy))
				read_engr_at(u.ux, u.uy);
		    return (0);
		}
		if (ttmp && ttmp->tseen) {
		    /* Allow pickup from holes and trap doors that you escaped
		     * from because that stuff is teetering on the edge just
		     * like you, but not pits, because there is an elevation
		     * discrepancy with stuff in pits.
		     */
		    if ((ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) &&
			(!u.utrap || (u.utrap && u.utraptype != TT_PIT))) {
			read_engr_at(u.ux, u.uy);
			return(0);
		    }
		}
		/* multi && !flags.run means they are in the middle of some other
		 * action, or possibly paralyzed, sleeping, etc.... and they just
		 * teleported onto the object.  They shouldn't pick it up.
		 */
		if ((multi && !flags.run) || (autopickup && !flags.pickup)) {
		    check_here(FALSE);
		    return (0);
		}
		if (notake(youmonst.data)) {
		    if (!autopickup)
#ifndef JP
			You("are physically incapable of picking anything up.");
#else
			Your("身体は物を拾い上げることが可能な形態ではない。");
#endif /*JP*/
		    else
			check_here(FALSE);
		    return (0);
		}

		/* if there's anything here, stop running */
		if (OBJ_AT(u.ux,u.uy) && flags.run && flags.run != 8 && !flags.nopick) nomul(0);
	}

	add_valid_menu_class(0);	/* reset */
	if (!u.uswallow) {
		objchain = level.objects[u.ux][u.uy];
		traverse_how = BY_NEXTHERE;
	} else {
		objchain = u.ustuck->minvent;
		traverse_how = 0;	/* nobj */
	}
	/*
	 * Start the actual pickup process.  This is split into two main
	 * sections, the newer menu and the older "traditional" methods.
	 * Automatic pickup has been split into its own menu-style routine
	 * to make things less confusing.
	 */
	if (autopickup) {
	    n = autopick(objchain, traverse_how, &pick_list);
	    goto menu_pickup;
	}

	if (flags.menu_style != MENU_TRADITIONAL || iflags.menu_requested) {

	    /* use menus exclusively */
	    if (count) {	/* looking for N of something */
		char buf[QBUFSZ];
		Sprintf(buf, E_J("Pick %d of what?","何を%d個拾いますか？"), count);
		val_for_n_or_more = count;	/* set up callback selector */
		n = query_objlist(buf, objchain,
			    traverse_how|AUTOSELECT_SINGLE|INVORDER_SORT,
			    &pick_list, PICK_ONE, n_or_more);
		/* correct counts, if any given */
		for (i = 0; i < n; i++)
		    pick_list[i].count = count;
	    } else {
		n = query_objlist(E_J("Pick up what?","何を拾いますか？"), objchain,
			traverse_how|AUTOSELECT_SINGLE|INVORDER_SORT|FEEL_COCKATRICE,
			&pick_list, PICK_ANY, all_but_uchain);
	    }
menu_pickup:
	    n_tried = n;
	    for (n_picked = i = 0 ; i < n; i++) {
		res = pickup_object(pick_list[i].item.a_obj,pick_list[i].count,
					FALSE);
		if (res < 0) break;	/* can't continue */
		n_picked += res;
	    }
	    if (pick_list) free((genericptr_t)pick_list);

	} else {
	    /* old style interface */
	    int ct = 0;
	    long lcount;
	    boolean all_of_a_type, selective;
	    char oclasses[MAXOCLASSES];
	    struct obj *obj, *obj2;

	    oclasses[0] = '\0';		/* types to consider (empty for all) */
	    all_of_a_type = TRUE;	/* take all of considered types */
	    selective = FALSE;		/* ask for each item */

	    /* check for more than one object */
	    for (obj = objchain;
		  obj; obj = (traverse_how == BY_NEXTHERE) ? obj->nexthere : obj->nobj)
		ct++;

	    if (ct == 1 && count) {
		/* if only one thing, then pick it */
		obj = objchain;
		lcount = min(obj->quan, (long)count);
		n_tried++;
		if (pickup_object(obj, lcount, FALSE) > 0)
		    n_picked++;	/* picked something */
		goto end_query;

	    } else if (ct >= 2) {
		int via_menu = 0;

		There(E_J("are %s objects here.","%s品物がある。"),
		      (ct <= 10) ? E_J("several","いくつかの") : E_J("many","たくさんの"));
		if (!query_classes(oclasses, &selective, &all_of_a_type,
				   E_J("pick up","拾い"), objchain,
				   traverse_how == BY_NEXTHERE,
				   FALSE,
				   &via_menu)) {
		    if (!via_menu) return (0);
		    n = query_objlist(E_J("Pick up what?","何を拾いますか？"),
				  objchain,
				  traverse_how|(selective ? 0 : INVORDER_SORT),
				  &pick_list, PICK_ANY,
				  via_menu == -2 ? allow_all : allow_category);
		    goto menu_pickup;
		}
	    }

	    for (obj = objchain; obj; obj = obj2) {
		if (traverse_how == BY_NEXTHERE)
			obj2 = obj->nexthere;	/* perhaps obj will be picked up */
		else
			obj2 = obj->nobj;
		lcount = -1L;

		if (!selective && oclasses[0] && !index(oclasses,obj->oclass))
		    continue;

		if (!all_of_a_type) {
		    char qbuf[BUFSZ];
#ifndef JP
		    Sprintf(qbuf, "Pick up %s?",
			safe_qbuf("", sizeof("Pick up ?"), doname(obj),
					an(simple_typename(obj->otyp)), "something"));
#else
		    Sprintf(qbuf, "%sを拾いますか？",
			safe_qbuf("", sizeof("を拾いますか？"), doname(obj),
					simple_typename(obj->otyp), "これ"));
#endif /*JP*/
		    switch ((obj->quan < 2L) ? ynaq(qbuf) : ynNaq(qbuf)) {
		    case 'q': goto end_query;	/* out 2 levels */
		    case 'n': continue;
		    case 'a':
			all_of_a_type = TRUE;
			if (selective) {
			    selective = FALSE;
			    oclasses[0] = obj->oclass;
			    oclasses[1] = '\0';
			}
			break;
		    case '#':	/* count was entered */
			if (!yn_number) continue; /* 0 count => No */
			lcount = (long) yn_number;
			if (lcount > obj->quan) lcount = obj->quan;
			/* fall thru */
		    default:	/* 'y' */
			break;
		    }
		}
		if (lcount == -1L) lcount = obj->quan;

		n_tried++;
		if ((res = pickup_object(obj, lcount, FALSE)) < 0) break;
		n_picked += res;
	    }
end_query:
	    ;	/* semicolon needed by brain-damaged compilers */
	}

	if (!u.uswallow) {
		if (!OBJ_AT(u.ux,u.uy)) u.uundetected = 0;

		/* position may need updating (invisible hero) */
		if (n_picked) newsym(u.ux,u.uy);

		/* see whether there's anything else here, after auto-pickup is done */
		if (autopickup) check_here(n_picked > 0);
	}
	return (n_tried > 0);
}

#ifdef AUTOPICKUP_EXCEPTIONS
boolean
is_autopickup_exception(obj, grab)
struct obj *obj;
boolean grab;	 /* forced pickup, rather than forced leave behind? */
{
	/*
	 *  Does the text description of this match an exception?
	 */
	char *objdesc = makesingular(doname(obj));
	struct autopickup_exception *ape = (grab) ?
					iflags.autopickup_exceptions[AP_GRAB] :
					iflags.autopickup_exceptions[AP_LEAVE];
	while (ape) {
		if (pmatch(ape->pattern, objdesc)) return TRUE;
		ape = ape->next;
	}
	return FALSE;
}
#endif /* AUTOPICKUP_EXCEPTIONS */

/*
 * Pick from the given list using flags.pickup_types.  Return the number
 * of items picked (not counts).  Create an array that returns pointers
 * and counts of the items to be picked up.  If the number of items
 * picked is zero, the pickup list is left alone.  The caller of this
 * function must free the pickup list.
 */
STATIC_OVL int
autopick(olist, follow, pick_list)
struct obj *olist;	/* the object list */
int follow;		/* how to follow the object list */
menu_item **pick_list;	/* list of objects and counts to pick up */
{
	menu_item *pi;	/* pick item */
	struct obj *curr;
	int n;
	const char *otypes = flags.pickup_types;

	/* first count the number of eligible items */
	for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow))


#ifndef AUTOPICKUP_EXCEPTIONS
	    if (!*otypes || index(otypes, curr->oclass)
#else
	    if ((!*otypes || index(otypes, curr->oclass) ||
		 is_autopickup_exception(curr, TRUE)) &&
	    	 !is_autopickup_exception(curr, FALSE)
#endif
#ifdef PICKUP_THROWN
		|| (flags.pickup_thrown && curr->othrown)
#endif
		) n++;

	if (n) {
	    *pick_list = pi = (menu_item *) alloc(sizeof(menu_item) * n);
	    for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow))
#ifndef AUTOPICKUP_EXCEPTIONS
		if (!*otypes || index(otypes, curr->oclass)
#else
	    if ((!*otypes || index(otypes, curr->oclass) ||
		 is_autopickup_exception(curr, TRUE)) &&
	    	 !is_autopickup_exception(curr, FALSE)
#endif
#ifdef PICKUP_THROWN
		|| (flags.pickup_thrown && curr->othrown)
#endif
	       ) {
		    pi[n].item.a_obj = curr;
		    pi[n].count = curr->quan;
		    n++;
		}
	}
	return n;
}


/*
 * Put up a menu using the given object list.  Only those objects on the
 * list that meet the approval of the allow function are displayed.  Return
 * a count of the number of items selected, as well as an allocated array of
 * menu_items, containing pointers to the objects selected and counts.  The
 * returned counts are guaranteed to be in bounds and non-zero.
 *
 * Query flags:
 *	BY_NEXTHERE	  - Follow object list via nexthere instead of nobj.
 *	AUTOSELECT_SINGLE - Don't ask if only 1 object qualifies - just
 *			    use it.
 *	USE_INVLET	  - Use object's invlet.
 *	INVORDER_SORT	  - Use hero's pack order.
 *	SIGNAL_NOMENU	  - Return -1 rather than 0 if nothing passes "allow".
 */
int
query_objlist(qstr, olist, qflags, pick_list, how, allow)
const char *qstr;		/* query string */
struct obj *olist;		/* the list to pick from */
int qflags;			/* options to control the query */
menu_item **pick_list;		/* return list of items picked */
int how;			/* type of query */
boolean FDECL((*allow), (OBJ_P));/* allow function */
{
#ifdef SORTLOOT
	int i, j;
#endif
	int n;
	winid win;
	struct obj *curr, *last;
	struct monst *shkp = (struct monst *)0;
#ifdef SORTLOOT
	struct obj **oarray;
#endif
	char *pack;
	anything any;
	boolean printed_type_name;
	boolean see_equipment_by_IR = FALSE;
	char buf[BUFSZ], *pstr;

	*pick_list = (menu_item *) 0;
	if (!olist) return 0;

	/* avoid printing 'blue and green shield' and etc.
	   when the hero looks at monster's equipment by
	   infravision but not by normal vision */
	if (olist->where == OBJ_MINVENT) {
	    struct monst *mtmp = olist->ocarry;
	    if (see_with_infrared(mtmp) &&
		!(cansee(mtmp->mx, mtmp->my) && mon_visible(mtmp))) {
		u.uprops[BLINDED].intrinsic++;
		see_equipment_by_IR = TRUE;
	    }
	}

	if (qflags & BY_NEXTHERE) {
	    if (costly_spot(u.ux, u.uy)) {
		shkp = shop_keeper(inside_shop(u.ux, u.uy));
	    }
	}

	/* count the number of items allowed */
	for (n = 0, last = 0, curr = olist; curr; curr = FOLLOW(curr, qflags))
	    if ((*allow)(curr)) {
		last = curr;
		n++;
	    }

	if (n == 0)	/* nothing to pick here */
	    return (qflags & SIGNAL_NOMENU) ? -1 : 0;

	if (n == 1 && (qflags & AUTOSELECT_SINGLE)) {
	    *pick_list = (menu_item *) alloc(sizeof(menu_item));
	    (*pick_list)->item.a_obj = last;
	    (*pick_list)->count = last->quan;
	    return 1;
	}

#ifdef SORTLOOT
	/* Make a temporary array to store the objects sorted */
	oarray = (struct obj **)alloc(n*sizeof(struct obj*));

	/* Add objects to the array */
	i = 0;
	for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
	    if ((*allow)(curr)) {
		if (flags.sortloot == 'f' ||
			(flags.sortloot == 'l' && !(qflags & USE_INVLET)))
		{
		    /* Insert object at correct index */
		    for (j = i; j; j--)
		    {
			if (strcmpi(cxname2(curr), cxname2(oarray[j-1]))>0) break;
			oarray[j] = oarray[j-1];
		    }
		    oarray[j] = curr;
		    i++;
		} else {
		    /* Just add it to the array */
		    oarray[i++] = curr;
		}
	    }
	}
#endif /* SORTLOOT */

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	any.a_obj = (struct obj *) 0;

	/*
	 * Run through the list and add the objects to the menu.  If
	 * INVORDER_SORT is set, we'll run through the list once for
	 * each type so we can group them.  The allow function will only
	 * be called once per object in the list.
	 */
	pack = flags.inv_order;
	do {
	    printed_type_name = FALSE;
#ifdef SORTLOOT
	    for (i = 0; i < n; i++) {
		curr = oarray[i];
#else /* SORTLOOT */
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
#endif /* SORTLOOT */
		if ((qflags & FEEL_COCKATRICE) && curr->otyp == CORPSE &&
		     will_feel_cockatrice(curr, FALSE)) {
			destroy_nhwindow(win);	/* stop the menu and revert */
			(void) look_here(0, FALSE);
			return 0;
		}
		if ((!(qflags & INVORDER_SORT) || curr->oclass == *pack)
							&& (*allow)(curr)) {

		    /* if sorting, print type name (once only) */
		    if (qflags & INVORDER_SORT && !printed_type_name) {
			any.a_obj = (struct obj *) 0;
			add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
					let_to_name(*pack, FALSE), MENU_UNSELECTED);
			printed_type_name = TRUE;
		    }

		    if (shkp) {
			pstr = price_str(curr, shkp);
			if (pstr[0] == 0) {
			    /* no price string */
			    Strcpy(buf, doname(curr));
			} else {
			    Sprintf(buf, "%s, %s", doname(curr), pstr);
			}
		    } else {
			Strcpy(buf, doname(curr));
		    }

		    any.a_obj = curr;
		    add_menu(win, obj_to_glyph(curr), &any,
			    qflags & USE_INVLET ? curr->invlet : 0,
			    def_oc_syms[(int)objects[curr->otyp].oc_class],
			    ATR_NONE, buf, MENU_UNSELECTED);
		}
	    }
	    pack++;
	} while (qflags & INVORDER_SORT && *pack);

#ifdef SORTLOOT
	free(oarray);
#endif
	end_menu(win, qstr);
	n = select_menu(win, how, pick_list);
	destroy_nhwindow(win);

	if (see_equipment_by_IR) u.uprops[BLINDED].intrinsic--;

	if (n > 0) {
	    menu_item *mi;
	    int i;

	    /* fix up counts:  -1 means no count used => pick all */
	    for (i = 0, mi = *pick_list; i < n; i++, mi++)
		if (mi->count == -1L || mi->count > mi->item.a_obj->quan)
		    mi->count = mi->item.a_obj->quan;
	} else if (n < 0) {
	    n = 0;	/* caller's don't expect -1 */
	}
	return n;
}

/*
 * allow menu-based category (class) selection (for Drop,take off etc.)
 *
 */
int
query_category(qstr, olist, qflags, pick_list, how)
const char *qstr;		/* query string */
struct obj *olist;		/* the list to pick from */
int qflags;			/* behaviour modification flags */
menu_item **pick_list;		/* return list of items picked */
int how;			/* type of query */
{
	int n;
	winid win;
	struct obj *curr;
	char *pack;
	anything any;
	boolean collected_type_name;
	char invlet;
	int ccount;
	boolean do_unpaid = FALSE;
	boolean do_blessed = FALSE, do_cursed = FALSE, do_uncursed = FALSE,
	    do_buc_unknown = FALSE;
	int num_buc_types = 0;

	*pick_list = (menu_item *) 0;
	if (!olist) return 0;
	if ((qflags & UNPAID_TYPES) && count_unpaid(olist)) do_unpaid = TRUE;
	if ((qflags & BUC_BLESSED) && count_buc(olist, BUC_BLESSED)) {
	    do_blessed = TRUE;
	    num_buc_types++;
	}
	if ((qflags & BUC_CURSED) && count_buc(olist, BUC_CURSED)) {
	    do_cursed = TRUE;
	    num_buc_types++;
	}
	if ((qflags & BUC_UNCURSED) && count_buc(olist, BUC_UNCURSED)) {
	    do_uncursed = TRUE;
	    num_buc_types++;
	}
	if ((qflags & BUC_UNKNOWN) && count_buc(olist, BUC_UNKNOWN)) {
	    do_buc_unknown = TRUE;
	    num_buc_types++;
	}

	ccount = count_categories(olist, qflags);
	/* no point in actually showing a menu for a single category */
	if (ccount == 1 && !do_unpaid && num_buc_types <= 1 && !(qflags & BILLED_TYPES)) {
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if ((qflags & WORN_TYPES) &&
		    !(curr->owornmask & (W_ARMOR|W_RING|W_AMUL|W_TOOL|W_WEP|W_SWAPWEP|W_QUIVER)))
		    continue;
		break;
	    }
	    if (curr) {
		*pick_list = (menu_item *) alloc(sizeof(menu_item));
		(*pick_list)->item.a_int = curr->oclass;
		return 1;
	    } else {
#ifdef DEBUG
		impossible("query_category: no single object match");
#endif
	    }
	    return 0;
	}

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	pack = flags.inv_order;
	if ((qflags & ALL_TYPES) && (ccount > 1)) {
		invlet = 'a';
		any.a_void = 0;
		any.a_int = ALL_TYPES_SELECTED;
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
		       (qflags & WORN_TYPES) ? E_J("All worn types","着用中のものすべて") : E_J("All types","すべて"),
			MENU_UNSELECTED);
		invlet = 'b';
	} else
		invlet = 'a';
	do {
	    collected_type_name = FALSE;
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if (curr->oclass == *pack) {
		   if ((qflags & WORN_TYPES) &&
		   		!(curr->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL |
		    	W_WEP | W_SWAPWEP | W_QUIVER)))
			 continue;
		   if (!collected_type_name) {
			any.a_void = 0;
			any.a_int = curr->oclass;
			add_menu(win, NO_GLYPH, &any, invlet++,
				def_oc_syms[(int)objects[curr->otyp].oc_class],
				ATR_NONE, let_to_name(*pack, FALSE),
				MENU_UNSELECTED);
			collected_type_name = TRUE;
		   }
		}
	    }
	    pack++;
	    if (invlet >= 'u') {
		impossible("query_category: too many categories");
		return 0;
	    }
	} while (*pack);
	/* unpaid items if there are any */
	if (do_unpaid) {
		invlet = 'u';
		any.a_void = 0;
		any.a_int = 'u';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			E_J("Unpaid items","未払いの品物"), MENU_UNSELECTED);
	}
	/* billed items: checked by caller, so always include if BILLED_TYPES */
	if (qflags & BILLED_TYPES) {
		invlet = 'x';
		any.a_void = 0;
		any.a_int = 'x';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			E_J("Unpaid items already used up","未払いのまま使った品物"), MENU_UNSELECTED);
	}
	if (qflags & CHOOSE_ALL) {
		invlet = 'A';
		any.a_void = 0;
		any.a_int = 'A';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			(qflags & WORN_TYPES) ?
			E_J("Auto-select every item being worn","着用中の品物すべてを自動選択") :
			E_J("Auto-select every item","すべての品物を自動選択"), MENU_UNSELECTED);
	}
	/* items with b/u/c/unknown if there are any */
	if (do_blessed) {
		invlet = 'B';
		any.a_void = 0;
		any.a_int = 'B';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			E_J("Items known to be Blessed","祝福されていると判っている品物"), MENU_UNSELECTED);
	}
	if (do_cursed) {
		invlet = 'C';
		any.a_void = 0;
		any.a_int = 'C';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			E_J("Items known to be Cursed","呪われていると判っている品物"), MENU_UNSELECTED);
	}
	if (do_uncursed) {
		invlet = 'U';
		any.a_void = 0;
		any.a_int = 'U';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			E_J("Items known to be Uncursed","呪われていないと判っている品物"), MENU_UNSELECTED);
	}
	if (do_buc_unknown) {
		invlet = 'X';
		any.a_void = 0;
		any.a_int = 'X';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			E_J("Items of unknown B/C/U status","祝福・呪いの判っていない品物"),
			MENU_UNSELECTED);
	}
	end_menu(win, qstr);
	n = select_menu(win, how, pick_list);
	destroy_nhwindow(win);
	if (n < 0)
	    n = 0;	/* caller's don't expect -1 */
	return n;
}

STATIC_OVL int
count_categories(olist, qflags)
struct obj *olist;
int qflags;
{
	char *pack;
	boolean counted_category;
	int ccount = 0;
	struct obj *curr;

	pack = flags.inv_order;
	do {
	    counted_category = FALSE;
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if (curr->oclass == *pack) {
		   if ((qflags & WORN_TYPES) &&
		    	!(curr->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL |
		    	W_WEP | W_SWAPWEP | W_QUIVER)))
			 continue;
		   if (!counted_category) {
			ccount++;
			counted_category = TRUE;
		   }
		}
	    }
	    pack++;
	} while (*pack);
	return ccount;
}

/* could we carry `obj'? if not, could we carry some of it/them? */
STATIC_OVL long
carry_count(obj, container, count, telekinesis, wt_before, wt_after)
struct obj *obj, *container;	/* object to pick up, bag it's coming out of */
long count;
boolean telekinesis;
int *wt_before, *wt_after;
{
    boolean adjust_wt = container && carried(container),
	    is_gold = obj->oclass == COIN_CLASS;
    int wt, iw, ow, oow;
    long qq, savequan;
    unsigned saveowt;
    const char *verb, *prefx1, *prefx2, *suffx;
    char obj_nambuf[BUFSZ], where[BUFSZ];

    savequan = obj->quan;
    saveowt = obj->owt;

    iw = max_capacity();

    if (count != savequan) {
	obj->quan = count;
	obj->owt = (unsigned)weight(obj);
    }
    wt = iw + (int)obj->owt;
    if (adjust_wt)
	wt -= (container->otyp == BAG_OF_HOLDING) ?
		(int)DELTA_CWT(container, obj) : (int)obj->owt;
    if (is_gold)	/* merged gold might affect cumulative weight */
	wt -= (GOLD_WT(u.ugold) + GOLD_WT(count) - GOLD_WT(u.ugold + count));
    if (count != savequan) {
	obj->quan = savequan;
	obj->owt = saveowt;
    }
    *wt_before = iw;
    *wt_after  = wt;

    if (wt < 0)
	return count;

    /* see how many we can lift */
    if (is_gold) {
	iw -= (int)GOLD_WT(u.ugold);
	if (!adjust_wt) {
	    qq = GOLD_CAPACITY((long)iw, u.ugold);
	} else {
	    oow = 0;
	    qq = 50L - (u.ugold % 100L) - 1L;
	    if (qq < 0L) qq += 100L;
	    for ( ; qq <= count; qq += 100L) {
		obj->quan = qq;
		obj->owt = (unsigned)GOLD_WT(qq);
		ow = (int)GOLD_WT(u.ugold + qq);
		ow -= (container->otyp == BAG_OF_HOLDING) ?
			(int)DELTA_CWT(container, obj) : (int)obj->owt;
		if (iw + ow >= 0) break;
		oow = ow;
	    }
	    iw -= oow;
	    qq -= 100L;
	}
	if (qq < 0L) qq = 0L;
	else if (qq > count) qq = count;
	wt = iw + (int)GOLD_WT(u.ugold + qq);
    } else if (count > 1 || count < obj->quan) {
	/*
	 * Ugh. Calc num to lift by changing the quan of of the
	 * object and calling weight.
	 *
	 * This works for containers only because containers
	 * don't merge.		-dean
	 */
	for (qq = 1L; qq <= count; qq++) {
	    obj->quan = qq;
	    obj->owt = (unsigned)(ow = weight(obj));
	    if (adjust_wt)
		ow -= (container->otyp == BAG_OF_HOLDING) ?
			(int)DELTA_CWT(container, obj) : (int)obj->owt;
	    if (iw + ow >= 0)
		break;
	    wt = iw + ow;
	}
	--qq;
    } else {
	/* there's only one, and we can't lift it */
	qq = 0L;
    }
    obj->quan = savequan;
    obj->owt = saveowt;

    if (qq < count) {
	/* some message will be given */
	Strcpy(obj_nambuf, doname(obj));
	if (container) {
#ifndef JP
	    Sprintf(where, "in %s", the(xname(container)));
	    verb = "carry";
#else
	    Sprintf(where, "%sの中に", xname(container));
	    verb = "運ぶ";
#endif /*JP*/
	} else {
#ifndef JP
	    Strcpy(where, "lying here");
	    verb = telekinesis ? "acquire" : "lift";
#else
	    Strcpy(where, "ここに");
	    verb = telekinesis ? "入手する" : "持ち上げる";
#endif /*JP*/
	}
    } else {
	/* lint supppression */
	*obj_nambuf = *where = '\0';
	verb = "";
    }
    /* we can carry qq of them */
    if (qq > 0) {
	if (qq < count)
#ifndef JP
	    You("can only %s %s of the %s %s.",
		verb, (qq == 1L) ? "one" : "some", obj_nambuf, where);
#else
	    You("%sある%sのうち%sしか%sことができない。",
		where, obj_nambuf, (qq == 1L) ? "ひとつ" : "いくつか", verb);
#endif /*JP*/
	*wt_after = wt;
	return qq;
    }

    if (!container) Strcpy(where, E_J("here","ここに"));  /* slightly shorter form */
    if (invent || u.ugold) {
#ifndef JP
	prefx1 = "you cannot ";
	prefx2 = "";
	suffx  = " any more";
#else
	prefx1 = "あなたはこれ以上";
	prefx2 = "";
#endif /*JP*/
    } else {
#ifndef JP
	prefx1 = (obj->quan == 1L) ? "it " : "even one ";
	prefx2 = "is too heavy for you to ";
	suffx  = "";
#else
	prefx1 = (obj->quan == 1L) ? "" : "ひとつですら";
	prefx2 = "あなたには重すぎて";
#endif /*JP*/
    }
#ifndef JP
    There("%s %s %s, but %s%s%s%s.",
	  otense(obj, "are"), obj_nambuf, where,
	  prefx1, prefx2, verb, suffx);
#else
    pline("%sは%sがあるが、%s%s%sことはできない。",
	  where, obj_nambuf, prefx1, prefx2, verb);
#endif /*JP*/

 /* *wt_after = iw; */
    return 0L;
}

/* determine whether character is able and player is willing to carry `obj' */
STATIC_OVL
int 
lift_object(obj, container, cnt_p, telekinesis)
struct obj *obj, *container;	/* object to pick up, bag it's coming out of */
long *cnt_p;
boolean telekinesis;
{
    int result, old_wt, new_wt, prev_encumbr, next_encumbr;

    if (obj->otyp == BOULDER && In_sokoban(&u.uz)) {
	You(E_J("cannot get your %s around this %s.",
		"この%sのまわりに%sを回すことができない。"),
			body_part(HAND), xname(obj));
	return -1;
    }
    if (obj->otyp == STATUE && get_material(obj) == LIQUID) {
	pline_The(E_J("%s is firmly rooted to the %s.",
		      "この%sはしっかりと%sに貼り付いている。"),
		  xname(obj), surface(obj->ox, obj->oy));
	return -1;
    }
    if (obj->otyp == LOADSTONE ||
	    (obj->otyp == BOULDER && throws_rocks(youmonst.data)))
	return 1;		/* lift regardless of current situation */

    *cnt_p = carry_count(obj, container, *cnt_p, telekinesis, &old_wt, &new_wt);
    if (*cnt_p < 1L) {
	result = -1;	/* nothing lifted */
    } else if (obj->oclass != COIN_CLASS && inv_cnt() >= 52 &&
		!merge_choice(invent, obj)) {
	Your(E_J("knapsack cannot accommodate any more items.",
		 "ナップサックにはこれ以上品物を詰め込めない。"));
	result = -1;	/* nothing lifted */
    } else {
	result = 1;
	prev_encumbr = near_capacity();
	if (prev_encumbr < flags.pickup_burden)
		prev_encumbr = flags.pickup_burden;
	next_encumbr = calc_capacity(new_wt - old_wt);
	if (next_encumbr > prev_encumbr) {
	    if (telekinesis) {
		result = 0;	/* don't lift */
	    } else {
		char qbuf[BUFSZ];
		long savequan = obj->quan;

		obj->quan = *cnt_p;
#ifndef JP
		Strcpy(qbuf,
			(next_encumbr > HVY_ENCUMBER) ? overloadmsg :
			(next_encumbr > MOD_ENCUMBER) ? nearloadmsg :
			moderateloadmsg);
		Sprintf(eos(qbuf), " %s. Continue?",
			safe_qbuf(qbuf, sizeof(" . Continue?"),
				doname(obj), an(simple_typename(obj->otyp)), "something"));
#else
		{ char qbuf2[BUFSZ];
		Sprintf(qbuf2, "%%sを持ち上げる%s。続けますか？",
			(next_encumbr > HVY_ENCUMBER) ? "のはきわめて困難だ" :
			(next_encumbr > MOD_ENCUMBER) ? "にはかなりの苦労を伴う" :
							"のは少々大変だ");
		Sprintf(qbuf, qbuf2,
			safe_qbuf(qbuf, sizeof("。続けますか？"),
				doname(obj), simple_typename(obj->otyp), "それ"));
		}
#endif /*JP*/
		obj->quan = savequan;
		switch (ynq(qbuf)) {
		case 'q':  result = -1; break;
		case 'n':  result =  0; break;
		default:   break;	/* 'y' => result == 1 */
		}
		clear_nhwindow(WIN_MESSAGE);
	    }
	}
    }

    if (obj->otyp == SCR_SCARE_MONSTER && result <= 0 && !container)
	obj->spe = 0;
    return result;
}

/* To prevent qbuf overflow in prompts use planA only
 * if it fits, or planB if PlanA doesn't fit,
 * finally using the fallback as a last resort.
 * last_restort is expected to be very short.
 */
const char *
safe_qbuf(qbuf, padlength, planA, planB, last_resort)
const char *qbuf, *planA, *planB, *last_resort;
unsigned padlength;
{
	/* convert size_t (or int for ancient systems) to ordinary unsigned */
	unsigned len_qbuf = (unsigned)strlen(qbuf),
	         len_planA = (unsigned)strlen(planA),
	         len_planB = (unsigned)strlen(planB),
	         len_lastR = (unsigned)strlen(last_resort);
	unsigned textleft = QBUFSZ - (len_qbuf + padlength);

	if (len_lastR >= textleft) {
	    impossible("safe_qbuf: last_resort too large at %u characters.",
		       len_lastR);
	    return "";
	}
	return (len_planA < textleft) ? planA :
		    (len_planB < textleft) ? planB : last_resort;
}

/*
 * Pick up <count> of obj from the ground and add it to the hero's inventory.
 * Returns -1 if caller should break out of its loop, 0 if nothing picked
 * up, 1 if otherwise.
 */
int
pickup_object(obj, count, telekinesis)
struct obj *obj;
long count;
boolean telekinesis;	/* not picking it up directly by hand */
{
	int res, nearload;
	const char *where = (obj->ox == u.ux && obj->oy == u.uy) ?
			    E_J("here","ここ") : E_J("there","そこ");

	if (obj->quan < count) {
	    impossible("pickup_object: count %ld > quan %ld?",
		count, obj->quan);
	    return 0;
	}

	/* In case of auto-pickup, where we haven't had a chance
	   to look at it yet; affects docall(SCR_SCARE_MONSTER). */
	if (!Blind)
		obj->dknown = 1;

	if (obj == uchain) {    /* do not pick up attached chain */
	    return 0;
	} else if (obj->oartifact && !touch_artifact(obj,&youmonst,TRUE)) {
	    return 0;
	} else if (obj->oclass == COIN_CLASS) {
	    /* Special consideration for gold pieces... */
	    long iw = (long)max_capacity() - GOLD_WT(u.ugold);
	    long gold_capacity = GOLD_CAPACITY(iw, u.ugold);

	    if (gold_capacity <= 0L) {
		pline(
#ifndef JP
	       "There %s %ld gold piece%s %s, but you cannot carry any more.",
		      otense(obj, "are"),
		      obj->quan, plur(obj->quan), where);
#else
	       "%sには%ld枚の金貨があるが、あなたはこれ以上運べない。",
		      where, obj->quan);
#endif /*JP*/
		return 0;
	    } else if (gold_capacity < count) {
#ifndef JP
		You("can only %s %s of the %ld gold pieces lying %s.",
		    telekinesis ? "acquire" : "carry",
		    gold_capacity == 1L ? "one" : "some", obj->quan, where);
		pline("%s %ld gold piece%s.",
		    nearloadmsg, gold_capacity, plur(gold_capacity));
#else
		You("%sにある%ld枚の金貨のうち、%sしか%sことができない。",
		    where, obj->quan, gold_capacity == 1L ? "1枚" : "いくらか",
		    telekinesis ? "入手する" : "運ぶ");
		pline("%ld枚の金貨%s", gold_capacity, nearloadmsg);
#endif /*JP*/
		u.ugold += gold_capacity;
		obj->quan -= gold_capacity;
		costly_gold(obj->ox, obj->oy, gold_capacity);
	    } else {
		u.ugold += count;
		if ((nearload = near_capacity()) != 0)
#ifndef JP
		    pline("%s %ld gold piece%s.",
			  nearload < MOD_ENCUMBER ?
			  moderateloadmsg : nearloadmsg,
			  count, plur(count));
#else
		    pline("%ld枚の金貨%s", count,
			  nearload < MOD_ENCUMBER ?
			  moderateloadmsg : nearloadmsg);
#endif /*JP*/
		else
		    prinv((char *) 0, obj, count);
		costly_gold(obj->ox, obj->oy, count);
		if (count == obj->quan)
		    delobj(obj);
		else
		    obj->quan -= count;
	    }
	    flags.botl = 1;
	    if (flags.run) nomul(0);
	    return 1;

	} else if (obj->otyp == CORPSE) {
	    if ( (touch_petrifies(&mons[obj->corpsenm])) && !uarmg
				&& !Stone_resistance && !telekinesis) {
		if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
		    display_nhwindow(WIN_MESSAGE, FALSE);
		else {
			char kbuf[BUFSZ];

#ifndef JP
			Strcpy(kbuf, an(corpse_xname(obj, TRUE)));
			pline("Touching %s is a fatal mistake.", kbuf);
#else
			Strcpy(kbuf, corpse_xname(obj, TRUE));
			pline("%sに触ることは致命的な過ちだ。", kbuf);
#endif /*JP*/
			instapetrify(kbuf);
		    return -1;
		}
	    } else if (is_rider(&mons[obj->corpsenm])) {
#ifndef JP
		pline("At your %s, the corpse suddenly moves...",
			telekinesis ? "attempted acquisition" : "touch");
#else
		pline("あなたが%s、死体は急に動き出した…。",
			telekinesis ? "取り上げようとすると" : "触ったとたん");
#endif /*JP*/
		(void) revive_corpse(obj);
		exercise(A_WIS, FALSE);
		return -1;
	    }
	} else  if (obj->otyp == SCR_SCARE_MONSTER) {
	    if (obj->blessed) obj->blessed = 0;
	    else if (!obj->spe && !obj->cursed) obj->spe = 1;
	    else {
#ifndef JP
		pline_The("scroll%s %s to dust as you %s %s up.",
			plur(obj->quan), otense(obj, "turn"),
			telekinesis ? "raise" : "pick",
			(obj->quan == 1L) ? "it" : "them");
#else
		pline("%s上げようとしたとたん、巻物は塵となって崩れた。",
			telekinesis ? "持ち" : "拾い");
#endif /*JP*/
		if (!(objects[SCR_SCARE_MONSTER].oc_name_known) &&
				    !(objects[SCR_SCARE_MONSTER].oc_uname))
		    docall(obj);
		useupf(obj, obj->quan);
		return 1;	/* tried to pick something up and failed, but
				   don't want to terminate pickup loop yet   */
	    }
	}

	if ((res = lift_object(obj, (struct obj *)0, &count, telekinesis)) <= 0)
	    return res;

	if (obj->quan != count && obj->otyp != LOADSTONE)
	    obj = splitobj(obj, count);

	obj = pick_obj(obj);

	if (uwep && uwep == obj) mrg_to_wielded = TRUE;
	nearload = near_capacity();
	prinv(nearload == SLT_ENCUMBER ? moderateloadmsg : (char *) 0,
	      obj, count);
	mrg_to_wielded = FALSE;

	if (Is_sokoprize(obj)) {
	    makeknown(obj->otyp); /* obj is already known */
	    obj->sokoprize = 0; /* reset sokoprize flag */
	    del_sokoprize();	/* delete other sokoprizes */
	}
	return 1;
}

/*
 * Do the actual work of picking otmp from the floor or monster's interior
 * and putting it in the hero's inventory.  Take care of billing.  Return a
 * pointer to the object where otmp ends up.  This may be different
 * from otmp because of merging.
 */
struct obj *
pick_obj(otmp)
struct obj *otmp;
{
	obj_extract_self(otmp);
	if (!u.uswallow && otmp != uball && costly_spot(otmp->ox, otmp->oy)) {
	    char saveushops[5], fakeshop[2];

	    /* addtobill cares about your location rather than the object's;
	       usually they'll be the same, but not when using telekinesis
	       (if ever implemented) or a grappling hook */
	    Strcpy(saveushops, u.ushops);
	    fakeshop[0] = *in_rooms(otmp->ox, otmp->oy, SHOPBASE);
	    fakeshop[1] = '\0';
	    Strcpy(u.ushops, fakeshop);
	    /* sets obj->unpaid if necessary */
	    addtobill(otmp, TRUE, FALSE, FALSE);
	    Strcpy(u.ushops, saveushops);
	    /* if you're outside the shop, make shk notice */
	    if (!index(u.ushops, *fakeshop))
		remote_burglary(otmp->ox, otmp->oy);
	}
	if (otmp->no_charge)	/* only applies to objects outside invent */
	    otmp->no_charge = 0;
	newsym(otmp->ox, otmp->oy);
	return addinv(otmp);	/* might merge it with other objects */
}

/*
 * prints a message if encumbrance changed since the last check and
 * returns the new encumbrance value (from near_capacity()).
 */
int
encumber_msg()
{
    static int oldcap = UNENCUMBERED;
    int newcap = near_capacity();

    if(oldcap < newcap) {
	switch(newcap) {
#ifndef JP
	case 1: Your("movements are slowed slightly because of your load.");
		break;
	case 2: You("rebalance your load.  Movement is difficult.");
		break;
	case 3: You("%s under your heavy load.  Movement is very hard.",
		    stagger(youmonst.data, "stagger"));
		break;
	default: You("%s move a handspan with this load!",
		     newcap == 4 ? "can barely" : "can't even");
		break;
#else
	case 1: Your("動きは荷物の重みでやや遅くなった。");
		break;
	case 2: You("荷物の釣合を取り直した。動くのは難しい。");
		break;
	case 3: You("重い荷物の下で%s。動くのは非常に困難だ。",
		    stagger(youmonst.data, "よろめいた"));
		break;
	default: pline("この荷物の重さでは、あなたは%s動けない！",
			newcap == 4 ? "ほとんど" : "一歩も");
		break;
#endif /*JP*/
	}
	flags.botl = 1;
    } else if(oldcap > newcap) {
	switch(newcap) {
#ifndef JP
	case 0: Your("movements are now unencumbered.");
		break;
	case 1: Your("movements are only slowed slightly by your load.");
		break;
	case 2: You("rebalance your load.  Movement is still difficult.");
		break;
	case 3: You("%s under your load.  Movement is still very hard.",
		    stagger(youmonst.data, "stagger"));
		break;
#else
	case 0: Your("動きは軽くなった。");
		break;
	case 1: Your("動きは荷物の重みでまだやや遅い。");
		break;
	case 2: You("荷物の釣合を取り直した。動くのはまだ難しい。");
		break;
	case 3: You("重い荷物の下で%s。動くのはまだ非常に困難だ。",
		    stagger(youmonst.data, "よろめいた"));
		break;
#endif /*JP*/
	}
	flags.botl = 1;
    }
    if (flags.showweight) flags.botl = 1;

    oldcap = newcap;
    return (newcap);
}

/* Is there a container at x,y. Optional: return count of containers at x,y */
int
container_at(x, y, countem)
int x,y;
boolean countem;
{
	struct obj *cobj, *nobj;
	int container_count = 0;
	
	for(cobj = level.objects[x][y]; cobj; cobj = nobj) {
		nobj = cobj->nexthere;
		if(Is_container(cobj)) {
			container_count++;
			if (!countem) break;
		}
	}
	return container_count;
}

STATIC_OVL boolean
able_to_loot(x, y)
int x, y;
{
	if (!can_reach_floor()) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
			rider_cant_reach(); /* not skilled enough to reach */
		else
#endif
			You(E_J("cannot reach the %s.",
				"%sに届かない。"), surface(x, y));
		return FALSE;
	} else if (is_pool(x, y) || is_lava(x, y)) {
		/* at present, can't loot in water even when Underwater */
		You(E_J("cannot loot things that are deep in the %s.",
			"%sの中深く沈んでいるものを探ることはできない。"),
		    is_lava(x, y) ? E_J("lava","溶岩") : E_J("water","水"));
		return FALSE;
	} else if (nolimbs(youmonst.data)) {
		pline(E_J("Without limbs, you cannot loot anything.",
			"手がなくては、何かをあさることはできない。"));
		return FALSE;
	} else if (!freehand()) {
		pline(E_J("Without a free %s, you cannot loot anything.",
			"%sが空いていないため、何かをあさることはできない。"),
			body_part(HAND));
		return FALSE;
	}
	return TRUE;
}

STATIC_OVL boolean
mon_beside(x,y)
int x, y;
{
	int i,j,nx,ny;
	for(i = -1; i <= 1; i++)
	    for(j = -1; j <= 1; j++) {
	    	nx = x + i;
	    	ny = y + j;
		if(isok(nx, ny) && MON_AT(nx, ny))
			return TRUE;
	    }
	return FALSE;
}

int
doloot()	/* loot a container on the floor or loot saddle from mon. */
{
    register struct obj *cobj, *nobj;
    register int c = -1;
    int timepassed = 0;
    coord cc;
    boolean underfoot = TRUE;
    const char *dont_find_anything = "don't find anything";
    struct monst *mtmp;
    char qbuf[BUFSZ];
    int prev_inquiry = 0;
    boolean prev_loot = FALSE;

    if (check_capacity((char *)0)) {
	/* "Can't do that while carrying so much stuff." */
	return 0;
    }
    if (nohands(youmonst.data)) {
#ifndef JP
	You("have no hands!");	/* not `body_part(HAND)' */
#else
	pline("あなたには手がない！");
#endif /*JP*/
	return 0;
    }
    cc.x = u.ux; cc.y = u.uy;

lootcont:

    if (container_at(cc.x, cc.y, FALSE)) {
	boolean any = FALSE;

	if (!able_to_loot(cc.x, cc.y)) return 0;
	for (cobj = level.objects[cc.x][cc.y]; cobj; cobj = nobj) {
	    nobj = cobj->nexthere;

	    if (Is_container(cobj)) {
#ifndef JP
		Sprintf(qbuf, "There is %s here, loot it?",
			safe_qbuf("", sizeof("There is  here, loot it?"),
			     doname(cobj), an(simple_typename(cobj->otyp)),
			     "a container"));
#else
		Sprintf(qbuf, "ここには%sがある。中を探りますか？",
			safe_qbuf("", sizeof("ここにはがある。中を探りますか？"),
			     doname(cobj), simple_typename(cobj->otyp), "入れ物"));
#endif /*JP*/
openbox:	c = ynq(qbuf);
		if (c == 'q') return (timepassed);
		if (c == 'n') continue;
		any = TRUE;

		if (cobj->olocked && Is_box(cobj)) {
		    pline(E_J("Hmmm, it seems to be locked.",
			      "うーむ、鍵がかけられているようだ。"));
		    continue;
		}
		if (cobj->otyp == BAG_OF_TRICKS) {
		    int tmp;
		    You(E_J("carefully open the bag...",
			    "注意深く鞄を開けた…。"));
		    pline(E_J("It develops a huge set of teeth and bites you!",
			      "鞄は巨大な牙をむいてあなたに噛み付いた！"));
		    tmp = rnd(10);
		    if (Half_physical_damage) tmp = (tmp+1) / 2;
#ifndef JP
		    losehp(tmp, "carnivorous bag", KILLED_BY_AN);
#else
		    losehp(tmp, "人食い鞄に", KILLED_SUFFIX);
#endif /*JP*/
		    makeknown(BAG_OF_TRICKS);
		    timepassed = 1;
		    continue;
		}

#ifndef JP
		You("carefully open %s...", the(xname(cobj)));
#else
		You("注意深く%sを開けた…。", xname(cobj));
#endif /*JP*/
		timepassed |= use_container(cobj, 0);
		if (multi < 0) return 1;		/* chest trap */
	    }
	}
	if (any) c = 'y';
    } else if (Confusion) {
	if (u.ugold){
	    long contribution = rnd((int)min(LARGEST_INT,u.ugold));
	    struct obj *goldob = mkgoldobj(contribution);
	    if (IS_THRONE(levl[u.ux][u.uy].typ)){
		struct obj *coffers;
		int pass;
		/* find the original coffers chest, or any chest */
		for (pass = 2; pass > -1; pass -= 2)
		    for (coffers = fobj; coffers; coffers = coffers->nobj)
			if (coffers->otyp == CHEST && coffers->spe == pass)
			    goto gotit;	/* two level break */
gotit:
		if (coffers) {
	    verbalize(E_J("Thank you for your contribution to reduce the debt.",
			  "負債を減らすための寄付に感謝します。"));
		    (void) add_to_container(coffers, goldob);
		    coffers->owt = weight(coffers);
		} else {
		    struct monst *mon = makemon(courtmon(),
					    u.ux, u.uy, NO_MM_FLAGS);
		    if (mon) {
			mon->mgold += goldob->quan;
			delobj(goldob);
			pline(E_J("The exchequer accepts your contribution.",
				  "財務官はあなたの寄付を受け取った。"));
		    } else {
			dropx(goldob);
		    }
		}
	    } else {
		dropx(goldob);
		pline(E_J("Ok, now there is loot here.",
			  "OK、これでここに宝がある。"));
	    }
	}
    } else if (IS_GRAVE(levl[cc.x][cc.y].typ)) {
#ifndef JP
	You("need to dig up the grave to effectively loot it...");
#else
	pline("副葬品を盗み出すには、墓をあばく必要がある…。");
#endif /*JP*/
    }
    /*
     * 3.3.1 introduced directional looting for some things.
     */
    if (c != 'y' && mon_beside(u.ux, u.uy)) {
#ifndef JP
	if (!get_adjacent_loc("Loot in what direction?", "Invalid loot location",
#else
	if (!get_adjacent_loc("どの方向のものを出し入れしますか？", "無効な方向指定です。",
#endif /*JP*/
			u.ux, u.uy, &cc)) return 0;
	if (cc.x == u.ux && cc.y == u.uy) {
	    underfoot = TRUE;
	    if (container_at(cc.x, cc.y, FALSE))
		goto lootcont;
	} else
	    underfoot = FALSE;
	if (u.dz < 0) {
#ifndef JP
	    You("%s to loot on the %s.", dont_find_anything,
		ceiling(cc.x, cc.y));
#else
	    pline("%sには出し入れできるようなものはない。", ceiling(cc.x, cc.y));
#endif /*JP*/
	    timepassed = 1;
	    return timepassed;
	}
	mtmp = m_at(cc.x, cc.y);
	if (mtmp) timepassed = loot_mon(mtmp, &prev_inquiry, &prev_loot);

	/* Preserve pre-3.3.1 behaviour for containers.
	 * Adjust this if-block to allow container looting
	 * from one square away to change that in the future.
	 */
	if (!underfoot) {
	    if (container_at(cc.x, cc.y, FALSE)) {
		if (mtmp) {
#ifndef JP
		    You_cant("loot anything %sthere with %s in the way.",
			    prev_inquiry ? "else " : "", mon_nam(mtmp));
#else
		    pline("そこには%sがいるため、物を出し入れすることはできない。",
			  mon_nam(mtmp));
#endif /*JP*/
		    return timepassed;
		} else {
#ifndef JP
		    You("have to be at a container to loot it.");
#else
		    pline("物を出し入れするには、入れ物の真上にいなければならない。");
#endif /*JP*/
		}
	    } else {
#ifndef JP
		You("%s %sthere to loot.", dont_find_anything,
			(prev_inquiry || prev_loot) ? "else " : "");
#else
		pline("そこには%s出し入れできるようなものはない。",
			(prev_inquiry || prev_loot) ? "他に" : "");
#endif /*JP*/
		return timepassed;
	    }
	}
    } else if (c != 'y' && c != 'n') {
#ifndef JP
	You("%s %s to loot.", dont_find_anything,
		    underfoot ? "here" : "there");
#else
	pline("%sこには何も出し入れできるようなものはない。", underfoot ? "こ" : "そ");
#endif /*JP*/
    }
    return (timepassed);
}

/* loot_mon() returns amount of time passed.
 */
int
loot_mon(mtmp, passed_info, prev_loot)
struct monst *mtmp;
int *passed_info;
boolean *prev_loot;
{
    int c = -1;
    int timepassed = 0;
#ifdef STEED
    struct obj *otmp;
    char qbuf[QBUFSZ];

    /* 3.3.1 introduced the ability to remove saddle from a steed             */
    /* 	*passed_info is set to TRUE if a loot query was given.               */
    /*	*prev_loot is set to TRUE if something was actually acquired in here. */
    if (mtmp && mtmp != u.usteed && (otmp = which_armor(mtmp, W_SADDLE))) {
	long unwornmask;
	if (passed_info) *passed_info = 1;
	Sprintf(qbuf, E_J("Do you want to remove the saddle from %s?",
			  "%sから鞍を外しますか？"),
		x_monnam(mtmp, ARTICLE_THE, (char *)0, SUPPRESS_SADDLE, FALSE));
	if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		if (nolimbs(youmonst.data)) {
#ifndef JP
		    You_cant("do that without limbs."); /* not body_part(HAND) */
#else
		    pline("あなたには腕がないため、鞍を外すことができない。");
#endif /*JP*/
		    return (0);
		}
		if (otmp->cursed) {
		    You(E_J("can't. The saddle seems to be stuck to %s.",
			    "鞍を外せなかった。どうやら%sに張り付いてしまっているようだ。"),
			x_monnam(mtmp, ARTICLE_THE, (char *)0,
				SUPPRESS_SADDLE, FALSE));
			    
		    /* the attempt costs you time */
			return (1);
		}
		obj_extract_self(otmp);
		if ((unwornmask = otmp->owornmask) != 0L) {
		    mtmp->misc_worn_check &= ~unwornmask;
		    otmp->owornmask = 0L;
		    update_mon_intrinsics(mtmp, otmp, FALSE, FALSE);
		}
		otmp = hold_another_object(otmp, E_J("You drop %s!","%sを落としてしまった！"), doname(otmp),
					(const char *)0);
		timepassed = rnd(3);
		if (prev_loot) *prev_loot = TRUE;
	} else if (c == 'q') {
		return (0);
	}
    }
#endif	/* STEED */
    /* 3.4.0 introduced the ability to pick things up from within swallower's stomach */
    if (u.uswallow) {
	int count = passed_info ? *passed_info : 0;
	timepassed = pickup(count);
    }
    return timepassed;
}

/*
 * Decide whether an object being placed into a magic bag will cause
 * it to explode.  If the object is a bag itself, check recursively.
 */
STATIC_OVL boolean
mbag_explodes(obj, depthin)
    struct obj *obj;
    int depthin;
{
    /* these won't cause an explosion when they're empty */
    if ((obj->otyp == WAN_CANCELLATION || obj->otyp == BAG_OF_TRICKS) &&
	    obj->spe <= 0)
	return FALSE;

    /* odds: 1/1, 2/2, 3/4, 4/8, 5/16, 6/32, 7/64, 8/128, 9/128, 10/128,... */
    if ((Is_mbag(obj) || obj->otyp == WAN_CANCELLATION) &&
	(rn2(1 << (depthin > 7 ? 7 : depthin)) <= depthin))
	return TRUE;
    else if (Has_contents(obj)) {
	struct obj *otmp;

	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
	    if (mbag_explodes(otmp, depthin+1)) return TRUE;
    }
    return FALSE;
}

/* A variable set in use_container(), to be used by the callback routines   */
/* in_container(), and out_container() from askchain() and use_container(). */
static NEARDATA struct obj *current_container;
#define Icebox (current_container->otyp == ICE_BOX)

/* Returns: -1 to stop, 1 item was inserted, 0 item was not inserted. */
STATIC_PTR int
in_container(obj)
register struct obj *obj;
{
	boolean floor_container = !carried(current_container);
	boolean was_unpaid = FALSE;
	char buf[BUFSZ];

	if (!current_container) {
		impossible("<in> no current_container?");
		return 0;
	} else if (obj == uball || obj == uchain) {
		E_J(You("must be kidding."), pline("ご冗談を。"));
		return 0;
	} else if (obj == current_container) {
		pline(E_J("That would be an interesting topological exercise.",
			  "それは位相幾何学上の興味深い課題ですね。"));
		return 0;
	} else if (obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL)) {
#ifndef JP
		Norep("You cannot %s %s you are wearing.",
			Icebox ? "refrigerate" : "stash", something);
#else
		Norep("あなたは着用中のものを%sことはできない。",
			Icebox ? "冷凍する" : "しまう");
#endif /*JP*/
		return 0;
	} else if ((obj->otyp == LOADSTONE) && obj->cursed) {
		obj->bknown = 1;
#ifndef JP
	      pline_The("stone%s won't leave your person.", plur(obj->quan));
#else
	      pline("石はあなたから離れようとしない。");
#endif /*JP*/
		return 0;
	} else if (obj->otyp == AMULET_OF_YENDOR ||
		   obj->otyp == CANDELABRUM_OF_INVOCATION ||
		   obj->otyp == BELL_OF_OPENING ||
		   obj->otyp == SPE_BOOK_OF_THE_DEAD) {
	/* Prohibit Amulets in containers; if you allow it, monsters can't
	 * steal them.  It also becomes a pain to check to see if someone
	 * has the Amulet.  Ditto for the Candelabrum, the Bell and the Book.
	 */
#ifndef JP
	    pline("%s cannot be confined in such trappings.", The(xname(obj)));
#else
	    pline("%sはそのようなものに閉じ込められはしない。", xname(obj));
#endif /*JP*/
	    return 0;
	} else if (obj->otyp == LEASH && obj->leashmon != 0) {
#ifndef JP
		pline("%s attached to your pet.", Tobjnam(obj, "are"));
#else
		pline("%sはあなたのペットに繋がれている。", xname(obj));
#endif /*JP*/
		return 0;
	} else if (obj == uwep) {
		if (welded(obj)) {
			weldmsg(obj);
			return 0;
		}
		setuwep((struct obj *) 0);
		if (uwep) return 0;	/* unwielded, died, rewielded */
	} else if (obj == uswapwep) {
		setuswapwep((struct obj *) 0);
		if (uswapwep) return 0;     /* unwielded, died, rewielded */
	} else if (obj == uquiver) {
		setuqwep((struct obj *) 0);
		if (uquiver) return 0;     /* unwielded, died, rewielded */
	}

	if (obj->otyp == CORPSE) {
	    if ( (touch_petrifies(&mons[obj->corpsenm])) && !uarmg
		 && !Stone_resistance) {
		if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
		    display_nhwindow(WIN_MESSAGE, FALSE);
		else {
		    char kbuf[BUFSZ];

		    Strcpy(kbuf, E_J(an(corpse_xname(obj, TRUE)), corpse_xname(obj, TRUE)));
		    pline(E_J("Touching %s is a fatal mistake.",
			      "%sに触れることは致命的な過ちだ。"), kbuf);
		    instapetrify(kbuf);
		    return -1;
		}
	    }
	}

	/* boxes, boulders, and big statues can't fit into any container */
	if (obj->otyp == ICE_BOX || Is_box(obj) || obj->otyp == BOULDER ||
		(obj->otyp == STATUE && bigmonst(&mons[obj->corpsenm]))) {
		/*
		 *  xname() uses a static result array.  Save obj's name
		 *  before current_container's name is computed.  Don't
		 *  use the result of strcpy() within You() --- the order
		 *  of evaluation of the parameters is undefined.
		 */
#ifndef JP
		Strcpy(buf, the(xname(obj)));
		You("cannot fit %s into %s.", buf,
		    the(xname(current_container)));
#else
		Strcpy(buf, xname(obj));
		pline("%sは大きすぎて%sには入らない。", buf,
			xname(current_container));
#endif /*JP*/
		return 0;
	}

	freeinv(obj);

	if (obj_is_burning(obj))	/* this used to be part of freeinv() */
		(void) snuff_lit(obj);

	if (floor_container && costly_spot(u.ux, u.uy)) {
	    if (current_container->no_charge && !obj->unpaid) {
		/* don't sell when putting the item into your own container */
		obj->no_charge = 1;
	    } else if (obj->oclass != COIN_CLASS) {
		/* sellobj() will take an unpaid item off the shop bill
		 * note: coins are handled later */
		was_unpaid = obj->unpaid ? TRUE : FALSE;
		sellobj_state(SELL_DELIBERATE);
		sellobj(obj, u.ux, u.uy);
		sellobj_state(SELL_NORMAL);
	    }
	}
	if (Icebox && !age_is_relative(obj)) {
		freeze_obj(obj);
	} else if (Is_mbag(current_container) && mbag_explodes(obj, 0)) {
		/* explicitly mention what item is triggering the explosion */
		pline(
	    E_J("As you put %s inside, you are blasted by a magical explosion!",
		"%sを中に入れたとたん、魔法の爆発があなたを襲った！"),
		      doname(obj));
		/* did not actually insert obj yet */
		if (was_unpaid) addtobill(obj, FALSE, FALSE, TRUE);
		if (Has_contents(current_container)) {
		    struct obj *otmp, *otmpn;
		    boolean mes = FALSE;
		    for (otmp = current_container->cobj; otmp; otmp = otmpn) {
			otmpn = otmp->nobj;
			if (obj_resists(otmp, 5, 80)) {
			    obj_extract_self(otmp);
			    if (!mes) {
				pline(E_J("Something is spitted out from the bag.", "爆発した鞄から何かが転げ落ちた。"));
				mes = TRUE;
			    }
			    dropy(otmp);
			}
		    }
		}
		obfree(obj, (struct obj *)0);
		delete_contents(current_container);
		if (!floor_container)
			useup(current_container);
		else if (obj_here(current_container, u.ux, u.uy))
			useupf(current_container, obj->quan);
		else
			panic("in_container:  bag not found.");

		losehp(d(6,6),E_J("magical explosion","魔法の爆発に巻き込まれて"), KILLED_BY_AN);
		current_container = 0;	/* baggone = TRUE; */
	}

	if (current_container) {
#ifndef JP
	    Strcpy(buf, the(xname(current_container)));
	    You("put %s into %s.", doname(obj), buf);
#else
	    Strcpy(buf, xname(current_container));
	    You("%sを%sに入れた。", doname(obj), buf);
#endif /*JP*/

	    /* gold in container always needs to be added to credit */
	    if (floor_container && obj->oclass == COIN_CLASS)
		sellobj(obj, current_container->ox, current_container->oy);
	    (void) add_to_container(current_container, obj);
	    current_container->owt = weight(current_container);
	}
	/* gold needs this, and freeinv() many lines above may cause
	 * the encumbrance to disappear from the status, so just always
	 * update status immediately.
	 */
	bot();

	return(current_container ? 1 : -1);
}

STATIC_PTR int
ck_bag(obj)
struct obj *obj;
{
	return current_container && obj != current_container;
}

/* Returns: -1 to stop, 1 item was removed, 0 item was not removed. */
STATIC_PTR int
out_container(obj)
register struct obj *obj;
{
	register struct obj *otmp;
	boolean is_gold = (obj->oclass == COIN_CLASS);
	int res, loadlev;
	long count;

	if (!current_container) {
		impossible("<out> no current_container?");
		return -1;
	} else if (is_gold) {
		obj->owt = weight(obj);
	}

	if(obj->oartifact && !touch_artifact(obj,&youmonst,TRUE)) return 0;

	if (obj->otyp == CORPSE) {
	    if ( (touch_petrifies(&mons[obj->corpsenm])) && !uarmg
		 && !Stone_resistance) {
		if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
		    display_nhwindow(WIN_MESSAGE, FALSE);
		else {
		    char kbuf[BUFSZ];

#ifndef JP
		    Strcpy(kbuf, an(corpse_xname(obj, TRUE)));
		    pline("Touching %s is a fatal mistake.", kbuf);
#else
		    Strcpy(kbuf, corpse_xname(obj, TRUE));
		    pline("%sに触れることは致命的な過ちだ。", kbuf);
#endif /*JP*/
		    instapetrify(kbuf);
		    return -1;
		}
	    }
	}

	count = obj->quan;
	if ((res = lift_object(obj, current_container, &count, FALSE)) <= 0)
	    return res;

	if (obj->quan != count && obj->otyp != LOADSTONE)
	    obj = splitobj(obj, count);

	/* Remove the object from the list. */
	obj_extract_self(obj);
	current_container->owt = weight(current_container);

	if (Icebox && !age_is_relative(obj)) {
		defrost_obj(obj);
	}
	/* simulated point of time */

	if(!obj->unpaid && !carried(current_container) &&
	     costly_spot(current_container->ox, current_container->oy)) {
		obj->ox = current_container->ox;
		obj->oy = current_container->oy;
		addtobill(obj, FALSE, FALSE, FALSE);
	}
	/* pick-axes are now allowed in a shop
	if (is_pick(obj) && !obj->unpaid && *u.ushops && shop_keeper(*u.ushops))
		verbalize("You sneaky cad! Get out of here with that pick!");*/

	otmp = addinv(obj);
	loadlev = near_capacity();
	prinv(loadlev ?
	      (loadlev < MOD_ENCUMBER ?
#ifndef JP
	       "You have a little trouble removing" :
	       "You have much trouble removing") : (char *)0,
#else
	       "を取り出すのは少々大変だ。" :
	       "を取り出すのはとても困難だ。") : (char *)0,
#endif /*JP*/
	      otmp, count);

	if (is_gold) {
		dealloc_obj(obj);
		bot();	/* update character's gold piece count immediately */
	}
	return 1;
}

/* an object inside a cursed bag of holding is being destroyed */
STATIC_OVL long
mbag_item_gone(held, item)
int held;
struct obj *item;
{
    struct monst *shkp;
    long loss = 0L;

    if (item->dknown)
#ifndef JP
	pline("%s %s vanished!", Doname2(item), otense(item, "have"));
#else
	pline("%sは消滅した！", doname(item));
#endif /*JP*/
    else
#ifndef JP
	You("%s %s disappear!", Blind ? "notice" : "see", doname(item));
#else
	You("%sが消えたことに気づいた！", doname(item));
#endif /*JP*/

    if (*u.ushops && (shkp = shop_keeper(*u.ushops)) != 0) {
	if (held ? (boolean) item->unpaid : costly_spot(u.ux, u.uy))
	    loss = stolen_value(item, u.ux, u.uy,
				(boolean)shkp->mpeaceful, TRUE);
    }
    obfree(item, (struct obj *) 0);
    return loss;
}

STATIC_OVL void
observe_quantum_cat(box)
struct obj *box;
{
    static NEARDATA const char sc[] = E_J("Schroedinger's Cat","シュレディンガーの猫");
    struct obj *deadcat;
    struct monst *livecat;
    xchar ox, oy;

    box->spe = 0;		/* box->owt will be updated below */
    if (get_obj_location(box, &ox, &oy, 0))
	box->ox = ox, box->oy = oy;	/* in case it's being carried */

    /* this isn't really right, since any form of observation
       (telepathic or monster/object/food detection) ought to
       force the determination of alive vs dead state; but basing
       it just on opening the box is much simpler to cope with */
    livecat = rn2(2) ? makemon(&mons[PM_HOUSECAT],
			       box->ox, box->oy, NO_MINVENT) : 0;
    if (livecat) {
	setmpeaceful(livecat, TRUE);
	if (!canspotmon(livecat))
	    You(E_J("think %s brushed your %s.",
		    "%sが%sをくすぐったような気がした。"), something, body_part(FOOT));
	else
	    pline(E_J("%s inside the box is still alive!",
		      "箱の中の猫はまだ生きていた！"), Monnam(livecat));
	(void) christen_monst(livecat, sc);
    } else {
	deadcat = mk_named_object(CORPSE, &mons[PM_HOUSECAT],
				  box->ox, box->oy, sc);
	if (deadcat) {
	    obj_extract_self(deadcat);
	    (void) add_to_container(box, deadcat);
	}
	pline_The(E_J("%s inside the box is dead!",
		      "箱の中の%sは死んでいた！"),
	    Hallucination ? rndmonnam() : E_J("housecat","猫"));
    }
    box->owt = weight(box);
    return;
}

#undef Icebox

int
use_container(obj, held)
register struct obj *obj;
register int held;
{
	struct obj *curr, *otmp;
	struct obj *u_gold = (struct obj *)0;
	boolean one_by_one, allflag, quantum_cat = FALSE,
		loot_out = FALSE, loot_in = FALSE;
	char select[MAXOCLASSES+1];
	char qbuf[BUFSZ], emptymsg[BUFSZ], pbuf[QBUFSZ];
	long loss = 0L;
	int cnt = 0, used = 0,
	    menu_on_request;

	emptymsg[0] = '\0';
	if (nohands(youmonst.data)) {
#ifndef JP
		You("have no hands!");	/* not `body_part(HAND)' */
#else
		pline("あなたには手がない！");
#endif /*JP*/
		return 0;
	} else if (!freehand()) {
#ifndef JP
		You("have no free %s.", body_part(HAND));
#else
		Your("%sはふさがっている。", body_part(HAND));
#endif /*JP*/
		return 0;
	}
	if (obj->olocked && Is_box(obj)) {
#ifndef JP
	    pline("%s to be locked.", Tobjnam(obj, "seem"));
	    if (held) You("must put it down to unlock.");
#else
	    pline("この%sには鍵がかけられているようだ。");
	    if (held) pline("鍵を外すには、いったん床に置く必要がある。");
#endif /*JP*/
	    return 0;
	} else if (obj->otrapped) {
#ifndef JP
	    if (held) You("open %s...", the(xname(obj)));
#else
	    if (held) You("%sを開けた…。", xname(obj));
#endif /*JP*/
	    (void) chest_trap(obj, HAND, FALSE);
	    /* even if the trap fails, you've used up this turn */
	    if (multi >= 0) {	/* in case we didn't become paralyzed */
		nomul(-1);
		nomovemsg = "";
	    }
	    return 1;
	}
	current_container = obj;	/* for use by in/out_container */

	if (obj->spe == -1) {
	    observe_quantum_cat(obj);
	    used = 1;
	    quantum_cat = TRUE;	/* for adjusting "it's empty" message */
	}
	/* Count the number of contained objects. Sometimes toss objects if */
	/* a cursed magic bag.						    */
	for (curr = obj->cobj; curr; curr = otmp) {
	    otmp = curr->nobj;
	    if (Is_mbag(obj) && obj->cursed && !obj_resists(curr, 92, 98)) {
		obj_extract_self(curr);
		loss += mbag_item_gone(held, curr);
		used = 1;
	    } else {
		cnt++;
	    }
	}

	if (loss)	/* magic bag lost some shop goods */
	    You(E_J("owe %ld %s for lost merchandise.",
		    "失われた商品の代金 %ld %s の支払い義務を負った。"), loss, currency(loss));
	obj->owt = weight(obj);	/* in case any items were lost */

	if (!cnt)
	    Sprintf(emptymsg, E_J("%s is %sempty.","%sは%s空だ。"), Yname2(obj),
		    quantum_cat ? E_J("now ","もう") : "");

	if (cnt || flags.menu_style == MENU_FULL) {
#ifndef JP
	    Strcpy(qbuf, "Do you want to take something out of ");
	    Sprintf(eos(qbuf), "%s?",
		    safe_qbuf(qbuf, 1, yname(obj), ysimple_name(obj), "it"));
#else
	    Sprintf(qbuf, "%sの中身を取り出しますか？",
		    safe_qbuf(qbuf, sizeof("の中身を取り出しますか？"),
		    yname(obj), ysimple_name(obj), "こ"));
#endif /*JP*/
	    if (flags.menu_style != MENU_TRADITIONAL) {
		if (flags.menu_style == MENU_FULL) {
		    int t;
		    char menuprompt[BUFSZ];
		    boolean outokay = (cnt != 0);
		    boolean inokay = (invent != 0) || (u.ugold != 0);
		    if (!outokay && !inokay) {
			pline("%s", emptymsg);
			You(E_J("don't have anything to put in.",
				"中に入れるものを何も持っていない。"));
			return used;
		    }
		    menuprompt[0] = '\0';
		    if (!cnt) Sprintf(menuprompt, "%s ", emptymsg);
		    Strcat(menuprompt, E_J("Do what?","どうしますか？"));
		    t = in_or_out_menu(menuprompt, current_container, outokay, inokay);
		    if (t <= 0) return 0;
		    loot_out = (t & 0x01) != 0;
		    loot_in  = (t & 0x02) != 0;
		} else {	/* MENU_COMBINATION or MENU_PARTIAL */
		    loot_out = (yn_function(qbuf, "ynq", 'n') == 'y');
		}
		if (loot_out) {
		    add_valid_menu_class(0);	/* reset */
		    used |= menu_loot(0, current_container, FALSE) > 0;
		}
	    } else {
		/* traditional code */
ask_again2:
		menu_on_request = 0;
		add_valid_menu_class(0);	/* reset */
		Strcpy(pbuf, ":ynq");
		if (cnt) Strcat(pbuf, "m");
		switch (yn_function(qbuf, pbuf, 'n')) {
		case ':':
		    container_contents(current_container, FALSE, FALSE);
		    goto ask_again2;
		case 'y':
		    if (query_classes(select, &one_by_one, &allflag,
				      E_J("take out","取り出し"), current_container->cobj,
				      FALSE,
				      FALSE,
				      &menu_on_request)) {
			if (askchain((struct obj **)&current_container->cobj,
				     (one_by_one ? (char *)0 : select),
				     allflag, out_container,
				     (int FDECL((*),(OBJ_P)))0,
				     0, "nodot"))
			    used = 1;
		    } else if (menu_on_request < 0) {
			used |= menu_loot(menu_on_request,
					  current_container, FALSE) > 0;
		    }
		    /*FALLTHRU*/
		case 'n':
		    break;
		case 'm':
		    menu_on_request = -2; /* triggers ALL_CLASSES */
		    used |= menu_loot(menu_on_request, current_container, FALSE) > 0;
		    break;
		case 'q':
		default:
		    return used;
		}
	    }
	} else {
	    pline("%s", emptymsg);		/* <whatever> is empty. */
	}

	if (!invent && u.ugold == 0) {
	    /* nothing to put in, but some feedback is necessary */
	    You(E_J("don't have anything to put in.",
		    "中に入れるものを何も持っていない。"));
	    return used;
	}
	if (flags.menu_style != MENU_FULL) {
	    Sprintf(qbuf, E_J("Do you wish to put %s in?",
			      "中に%sを入れますか？"), something);
	    Strcpy(pbuf, ynqchars);
	    if (flags.menu_style == MENU_TRADITIONAL && invent && inv_cnt() > 0)
		Strcat(pbuf, "m");
	    switch (yn_function(qbuf, pbuf, 'n')) {
		case 'y':
		    loot_in = TRUE;
		    break;
		case 'n':
		    break;
		case 'm':
		    add_valid_menu_class(0);	  /* reset */
		    menu_on_request = -2; /* triggers ALL_CLASSES */
		    used |= menu_loot(menu_on_request, current_container, TRUE) > 0;
		    break;
		case 'q':
		default:
		    return used;
	    }
	}
	/*
	 * Gone: being nice about only selecting food if we know we are
	 * putting things in an ice chest.
	 */
	if (loot_in) {
	    if (u.ugold) {
		/*
		 * Hack: gold is not in the inventory, so make a gold object
		 * and put it at the head of the inventory list.
		 */
		u_gold = mkgoldobj(u.ugold);	/* removes from u.ugold */
		u_gold->in_use = TRUE;
		u.ugold = u_gold->quan;		/* put the gold back */
		assigninvlet(u_gold);		/* might end up as NOINVSYM */
		u_gold->nobj = invent;
		invent = u_gold;
	    }
	    add_valid_menu_class(0);	  /* reset */
	    if (flags.menu_style != MENU_TRADITIONAL) {
		used |= menu_loot(0, current_container, TRUE) > 0;
	    } else {
		/* traditional code */
		menu_on_request = 0;
		if (query_classes(select, &one_by_one, &allflag, E_J("put in","入れる"),
				   invent, FALSE,
				   (u.ugold != 0L),
				   &menu_on_request)) {
		    (void) askchain((struct obj **)&invent,
				    (one_by_one ? (char *)0 : select), allflag,
				    in_container, ck_bag, 0, "nodot");
		    used = 1;
		} else if (menu_on_request < 0) {
		    used |= menu_loot(menu_on_request,
				      current_container, TRUE) > 0;
		}
	    }
	}

	if (u_gold && invent && invent->oclass == COIN_CLASS) {
	    /* didn't stash [all of] it */
	    u_gold = invent;
	    invent = u_gold->nobj;
	    u_gold->in_use = FALSE;
	    dealloc_obj(u_gold);
	}

	return used;
}

/* Loot a container (take things out, put things in), using a menu. */
STATIC_OVL int
menu_loot(retry, container, put_in)
int retry;
struct obj *container;
boolean put_in;
{
    int n, i, n_looted = 0;
    boolean all_categories = TRUE, loot_everything = FALSE;
    char buf[BUFSZ];
    const char *takeout = E_J("Take out","取り出し"), *putin = E_J("Put in","中に入れ");
    struct obj *otmp, *otmp2;
    menu_item *pick_list;
    int mflags, res;
    long count;

    if (retry) {
	all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
	all_categories = FALSE;
	Sprintf(buf,E_J("%s what type of objects?",
			"どの種類の品物を%sますか？"), put_in ? putin : takeout);
	mflags = put_in ? ALL_TYPES | BUC_ALLBKNOWN | BUC_UNKNOWN :
		          ALL_TYPES | CHOOSE_ALL | BUC_ALLBKNOWN | BUC_UNKNOWN;
	n = query_category(buf, put_in ? invent : container->cobj,
			   mflags, &pick_list, PICK_ANY);
	if (!n) return 0;
	for (i = 0; i < n; i++) {
	    if (pick_list[i].item.a_int == 'A')
		loot_everything = TRUE;
	    else if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
		all_categories = TRUE;
	    else
		add_valid_menu_class(pick_list[i].item.a_int);
	}
	free((genericptr_t) pick_list);
    }

    if (loot_everything) {
	for (otmp = container->cobj; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;
	    res = out_container(otmp);
	    if (res < 0) break;
	}
    } else {
	mflags = INVORDER_SORT;
	if (put_in && flags.invlet_constant) mflags |= USE_INVLET;
	Sprintf(buf,E_J("%s what?","何を%sますか？"), put_in ? putin : takeout);
	n = query_objlist(buf, put_in ? invent : container->cobj,
			  mflags, &pick_list, PICK_ANY,
			  all_categories ? allow_all : allow_category);
	if (n) {
		n_looted = n;
		for (i = 0; i < n; i++) {
		    otmp = pick_list[i].item.a_obj;
		    count = pick_list[i].count;
		    if (count > 0 && count < otmp->quan) {
			otmp = splitobj(otmp, count);
			/* special split case also handled by askchain() */
		    }
		    res = put_in ? in_container(otmp) : out_container(otmp);
		    if (res < 0) {
			if (otmp != pick_list[i].item.a_obj) {
			    /* split occurred, merge again */
			    (void) merged(&pick_list[i].item.a_obj, &otmp);
			}
			break;
		    }
		}
		free((genericptr_t)pick_list);
	}
    }
    return n_looted;
}

STATIC_OVL int
in_or_out_menu(prompt, obj, outokay, inokay)
const char *prompt;
struct obj *obj;
boolean outokay, inokay;
{
    winid win;
    anything any;
    menu_item *pick_list;
    char buf[BUFSZ];
    int n;
    const char *menuselector = iflags.lootabc ? "abc" : "oib";

    any.a_void = 0;
    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    if (outokay) {
	any.a_int = 1;
#ifndef JP
	Sprintf(buf,"Take %s out of %s", something, the(xname(obj)));
#else
	Sprintf(buf,"%sから物を取り出す", xname(obj));
#endif /*JP*/
	add_menu(win, NO_GLYPH, &any, *menuselector, 0, ATR_NONE,
			buf, MENU_UNSELECTED);
    }
    menuselector++;
    if (inokay) {
	any.a_int = 2;
#ifndef JP
	Sprintf(buf,"Put %s into %s", something, the(xname(obj)));
#else
	Sprintf(buf,"%sに物を入れる", xname(obj));
#endif /*JP*/
	add_menu(win, NO_GLYPH, &any, *menuselector, 0, ATR_NONE, buf, MENU_UNSELECTED);
    }
    menuselector++;
    if (outokay && inokay) {
	any.a_int = 3;
	add_menu(win, NO_GLYPH, &any, *menuselector, 0, ATR_NONE,
			E_J("Both of the above","両方"), MENU_UNSELECTED);
    }
    end_menu(win, prompt);
    n = select_menu(win, PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
	n = pick_list[0].item.a_int;
	free((genericptr_t) pick_list);
    }
    return n;
}

void
freeze_obj(corp)
struct obj *corp;
{
	/* stop any corpse timeouts when frozen */
	if (corp->otyp == CORPSE) {
	    corp->age = monstermoves - corp->age; /* actual age */
	    if (corp->timed) {
		long rot_alarm = stop_timer(ROT_CORPSE, (genericptr_t)corp);
		(void) stop_timer(REVIVE_MON, (genericptr_t)corp);
		/* mark a non-reviving corpse as such */
		if (rot_alarm) corp->norevive = 1;
	    }
	}
}

void
defrost_obj(corp)
struct obj *corp;
{
	/* corpses start to rot */
	if (corp->otyp == CORPSE) {
		corp->age = monstermoves - corp->age; /* actual age */
		start_corpse_timeout(corp);
	}
}

STATIC_OVL void
del_sokoprize()
{
	int x, y, cnt = 0;
	struct obj *otmp, *onext;
	/* check objs on floor */
	for (otmp = fobj; otmp; otmp = onext) {
	    onext = otmp->nobj; /* otmp may be destroyed */
	    if (Is_sokoprize(otmp)) {
		boolean plu = is_plural(otmp);
		x = otmp->ox;
		y = otmp->oy;
		obj_extract_self(otmp);
		if (cansee(x, y)) {
#ifndef JP
		    You("see %s pop%s and vanish%s.", an(xname(otmp)),
			plu ? "" : "s", plu ? "" : "es");
#else
		    pline("%sははじけて消えた。", xname(otmp));
#endif /*JP*/
		    newsym(x, y);
		} else cnt++;
		obfree(otmp, (struct obj *)0);
	    }
	}
	if (cnt && flags.soundok) You_hear(E_J("something pops.","何かがはじける音を"));
	/* check buried objs... do we need this? */
	for (otmp = level.buriedobjlist; otmp; otmp = onext) {
	    onext = otmp->nobj; /* otmp may be destroyed */
	    if (Is_sokoprize(otmp)) {
		obj_extract_self(otmp);
		obfree(otmp, (struct obj *)0);
	    }
	}
}

/*pickup.c*/
