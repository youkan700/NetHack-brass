/*	SCCS Id: @(#)trap.c	3.4	2003/10/20	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const char * const destroy_strings[];	/* from zap.c */

STATIC_DCL void FDECL(dofiretrap, (struct obj *));
STATIC_DCL void NDECL(domagictrap);
STATIC_DCL boolean FDECL(emergency_disrobe,(boolean *));
STATIC_DCL int FDECL(untrap_prob, (struct trap *ttmp));
STATIC_DCL void FDECL(move_into_trap, (struct trap *));
STATIC_DCL int FDECL(try_disarm, (struct trap *,BOOLEAN_P));
STATIC_DCL void FDECL(reward_untrap, (struct trap *, struct monst *));
STATIC_DCL int FDECL(disarm_holdingtrap, (struct trap *));
STATIC_DCL int FDECL(disarm_landmine, (struct trap *));
STATIC_DCL int FDECL(disarm_squeaky_board, (struct trap *));
STATIC_DCL int FDECL(disarm_shooting_trap, (struct trap *, int));
STATIC_DCL void FDECL(check_chest_trap, (struct obj *,BOOLEAN_P));
STATIC_DCL int FDECL(try_lift, (struct monst *, struct trap *, int, BOOLEAN_P));
STATIC_DCL int FDECL(help_monster_out, (struct monst *, struct trap *));
STATIC_DCL boolean FDECL(thitm, (int,struct monst *,struct obj *,int,BOOLEAN_P));
STATIC_DCL int FDECL(mkroll_launch,
			(struct trap *,XCHAR_P,XCHAR_P,SHORT_P,long));
STATIC_DCL boolean FDECL(isclearpath,(coord *, int, SCHAR_P, SCHAR_P));
#ifdef STEED
STATIC_OVL int FDECL(steedintrap, (struct trap *, struct obj *));
STATIC_OVL boolean FDECL(keep_saddle_with_steedcorpse,
			(unsigned, struct obj *, struct obj *));
#endif

STATIC_DCL int FDECL(rust_dmg_decide, (struct obj *, int, boolean));
STATIC_DCL void FDECL(rust_dmg_msg,
			(struct obj *, const char *, int, boolean, struct monst *, int));
STATIC_DCL int FDECL(getobj_filter_untrap_squeaky, (struct obj *));

#define RUST_NOEFFECT	0	/* obj was not damaged at all */
#define RUST_BLESSPRT	1	/* obj's blessing protected it */
#define RUST_GREASEPRT	2	/* obj's grease protected it */
#define RUST_GREASEOFF	3	/* obj's grease dissolved */
#define	RUST_PROTECTED	4	/* obj was rustproofed */
#define	RUST_PROTGONE	5	/* obj's protection was lost */
#define RUST_DAMAGED1	6	/* obj was damaged (1st time) */
#define RUST_DAMAGED2	7	/* obj was damaged further */
#define RUST_DAMAGED3	8	/* obj was completely damaged */
#define RUST_DAMAGED4	9	/* obj was damaged but no change */
#define RUST_BROKEN	10	/* obj was broken */

#ifndef OVLB
STATIC_VAR const char *a_your[2];
STATIC_VAR const char *A_Your[2];
STATIC_VAR const char tower_of_flame[];
STATIC_VAR const char *A_gush_of_water_hits;
STATIC_VAR const char * const blindgas[6];

#else

STATIC_VAR const char * const a_your[2] = { E_J("a",""), E_J("your","") };
STATIC_VAR const char * const A_Your[2] = { E_J("A",""), E_J("Your","") };
STATIC_VAR const char tower_of_flame[] = E_J("tower of flame","??????????????????????");
STATIC_VAR const char * const A_gush_of_water_hits = E_J("A gush of water hits","?????o????????");
STATIC_VAR const char * const blindgas[6] = 
#ifndef JP
	{"humid", "odorless", "pungent", "chilling", "acrid", "biting"};
#else
	{"??????", "???L??", "???????L??", "??????", "?@??????", "??????????"};
#endif /*JP*/

#endif /* OVLB */

#ifdef OVLB

/* called when you're hit by fire (dofiretrap,buzz,zapyourself,explode) */
boolean			/* returns TRUE if hit on torso */
burnarmor(victim)
struct monst *victim;
{
    struct obj *item;
    char buf[BUFSZ];
    int mat_idx;
    
    if (!victim) return 0;
#define burn_dmg(obj,descr) rust_dmg(obj, descr, 0, FALSE, victim)
    while (1) {
	switch (rn2(5)) {
	case 0:
	    item = (victim == &youmonst) ? uarmh : which_armor(victim, W_ARMH);
	    if (item) {
		mat_idx = get_material(item);
//	    	Sprintf(buf,"%s helmet", materialnm[mat_idx] );
	    	Sprintf(buf, E_J("%s %s","%s%s"),
			material_word_for_simplename(item), helm_simple_name(item) );
	    }
	    if (!burn_dmg(item, item ? buf : E_J("helmet","??"))) continue;
	    break;
	case 1:
	    item = (victim == &youmonst) ? uarmc : which_armor(victim, W_ARMC);
	    if (item) {
		(void) burn_dmg(item, cloak_simple_name(item));
		return TRUE;
	    }
	    item = (victim == &youmonst) ? uarm : which_armor(victim, W_ARM);
	    if (item) {
		(void) burn_dmg(item, xname(item));
		return TRUE;
	    }
	    item = (victim == &youmonst) ? uarmu : which_armor(victim, W_ARMU);
	    if (item)
		(void) burn_dmg(item, E_J("shirt","?V???c"));
	    return TRUE;
	case 2:
	    item = (victim == &youmonst) ? uarms : which_armor(victim, W_ARMS);
	    if (!burn_dmg(item, E_J("wooden shield","????????"))) continue;
	    break;
	case 3:
	    item = (victim == &youmonst) ? uarmg : which_armor(victim, W_ARMG);
	    if (!burn_dmg(item, E_J("gloves","????"))) continue;
	    break;
	case 4:
	    item = (victim == &youmonst) ? uarmf : which_armor(victim, W_ARMF);
	    if (!burn_dmg(item, E_J("boots","?C"))) continue;
	    break;
	}
	break; /* Out of while loop */
    }
    return FALSE;
#undef burn_dmg
}

/* Generic rust-armor function.  Returns TRUE if a message was printed;
 * "print", if set, means to print a message (and thus to return TRUE) even
 * if the item could not be rusted; otherwise a message is printed and TRUE is
 * returned only for rustable items.
 */
boolean
rust_dmg(otmp, ostr, type, print, victim)
register struct obj *otmp;
register const char *ostr;
int type;	/* 0:burn 1:rust 2:rot 3:erode */
boolean print;
struct monst *victim;
{
	boolean is_primary = (type == 0 || type == 1);	/* burn/rust */
	boolean youdefend = (victim == &youmonst);
	boolean mondefend = !youdefend && victim;
	boolean vismon = mondefend && canseemon(victim);
	boolean visobj = !victim && cansee(bhitpos.x, bhitpos.y); /* assume thrown */
	boolean rusted = TRUE;
	int res;

	if (!otmp) return(FALSE);
	if (youdefend) {
		if (type == 3/*erode*/ && is_full_resist(ACID_RES)) return(FALSE);
		if (type == 0/*burn*/  && is_full_resist(FIRE_RES)) return(FALSE);
	}

	res = rust_dmg_decide(otmp, type, youdefend);

	/* workaround: thrown obj must not be broken */
	if (!youdefend && !mondefend && res == RUST_BROKEN)
		res = RUST_DAMAGED4;

	if (youdefend || vismon || visobj)
		rust_dmg_msg(otmp, ostr, type, print, victim, res);

	switch (res) {

		case RUST_GREASEOFF:		/* obj's grease dissolved */
		    otmp->greased = 0;
		    if (youdefend) update_inventory();
		    /* fallthrough */
		case RUST_BLESSPRT:		/* obj's blessing protected it */
		case RUST_GREASEPRT:		/* obj's grease protected it */
		    break;

		case RUST_PROTECTED:
		    otmp->rknown = TRUE;
		    if (youdefend) update_inventory();
		    break;

		case RUST_PROTGONE:		/* obj's protection was lost */
		    otmp->oerodeproof = FALSE;
		    if (youdefend) update_inventory();
		    break;

		case RUST_DAMAGED1:		/* obj was damaged (1st time) */
		case RUST_DAMAGED2:		/* obj was damaged further */
		case RUST_DAMAGED3:		/* obj was completely damaged */
		    if (is_primary)
			otmp->oeroded++;
		    else
			otmp->oeroded2++;
		    if (youdefend) update_inventory();
		    break;

		case RUST_DAMAGED4:		/* obj was damaged but no change */
		    break;

		case RUST_BROKEN:		/* obj was broken */
		    if (youdefend) {
			remove_worn_item(otmp, FALSE);
			if (otmp == uball) unpunish();
			useupall(otmp);
			update_inventory();
		    } else if (mondefend) {
			long unwornmask;
			if ((unwornmask = otmp->owornmask) != 0L) {
			    victim->misc_worn_check &= ~unwornmask;
			    if (otmp->owornmask & W_WEP)
				setmnotwielded(victim,otmp);
			    otmp->owornmask = 0L;
			    update_mon_intrinsics(victim, otmp, FALSE, FALSE);
			}
			if (unwornmask & W_WEP)		/* wielded weapon is broken */
			    possibly_unwield(victim, FALSE);
			else if (unwornmask & W_ARMG)	/* worn gloves are broken */
			    mselftouch(victim, (const char *)0, TRUE);
			m_useup(victim, otmp);
		    } /*else {
			obj_extract_self(otmp);
			obfree(otmp, (struct obj *)0);
		    } */
		    break;

		case RUST_NOEFFECT:		/* obj was not damaged at all */
		default:
		    rusted = FALSE;
		    break;
	}
	return(rusted);
}

int
rust_dmg_decide(otmp, type, youdefend)
struct obj *otmp;
int type;
boolean youdefend;
{
	boolean vulnerable = FALSE;
	boolean grprot = FALSE;
	boolean is_primary = TRUE;
	int erosion;

	if (!otmp) return RUST_NOEFFECT;
	switch(type) {
		case 0: vulnerable = is_flammable(otmp);
			break;
		case 1: vulnerable = is_rustprone(otmp);
			grprot = TRUE;
			break;
		case 2: vulnerable = is_rottable(otmp);
			is_primary = FALSE;
			break;
		case 3: vulnerable = is_corrodeable(otmp);
			grprot = TRUE;
			is_primary = FALSE;
			break;
	}
	erosion = is_primary ? otmp->oeroded : otmp->oeroded2;

	if (!vulnerable) return RUST_NOEFFECT;

	/* blessing */
	if (otmp->blessed && !rn2(4)) {
	    if (otmp->greased) return RUST_GREASEPRT;
	    if (otmp->oerodeproof && otmp->rknown)  return RUST_PROTECTED;
	    return RUST_BLESSPRT;
	}

	/* grease */
	if (grprot && otmp->greased) {
	    int prob = rn2(100);
	    return (prob < 50) ? RUST_GREASEPRT : RUST_GREASEOFF;
	}

	if (erosion < MAX_ERODE) {
	    /* rustproof */
	    if (otmp->oerodeproof) {
		int prob = 10;
		if (youdefend) prob += Luck;
		if (prob < 4) prob = 4;
		return (!rn2(prob)) ? RUST_PROTGONE : RUST_PROTECTED;
	    }

	    /* not protected */
	    return RUST_DAMAGED1 + erosion;

	} else {
	    if (!otmp->oartifact && !rn2(5)) return RUST_BROKEN;
	    else return RUST_DAMAGED4;
	}
}

void
rust_dmg_msg(otmp, ostr, type, print, victim, msgtype)
register struct obj *otmp;
register const char *ostr;
int type;
boolean print;
struct monst *victim;
int msgtype;
{
#ifndef JP
	static NEARDATA const char * const action[] = { "smoulder", "rust", "rot", "corrode" };
	static NEARDATA const char * const msg[] =  { "burnt", "rusted", "rotten", "corroded" };
	static NEARDATA const char * const dmglvl[] =  { "", " further", " completely" };
	static const char grstxt[] = "protected by the layer of grease!";
#else
	static NEARDATA const char * const action[] = { "??????", "?K????", "??????", "???H????" };
	static NEARDATA const char * const msg[] =  { "??????????", "?K????????", "??????????", "???H????????" };
	static NEARDATA const char * const dmglvl[] =  { "", "??????", "???S??" };
	static const char grstxt[] = "?????h?????????????????I";
#endif /*JP*/
	char buf[BUFSZ];

	if (!otmp) return;

#ifndef JP
	if (victim == &youmonst) Sprintf(buf, "Your %s ", ostr);
	else if (victim && !is_animobj(victim->data))
	     Sprintf(buf, "%s %s ", s_suffix(Monnam(victim)), ostr);
	else Sprintf(buf, "The %s ", ostr);
#else
	if (victim == &youmonst) Sprintf(buf, "????????%s??", ostr);
	else if (victim && !is_animobj(victim->data))
	     Sprintf(buf, "%s??%s??", Monnam(victim), ostr);
	else Sprintf(buf, "%s??", ostr);
#endif /*JP*/

	switch (msgtype) {

	    case RUST_NOEFFECT:			/* obj was not damaged at all */
		if (!print) break;
		/* fallthru */
	    case RUST_PROTECTED:		/* obj was not damaged at all */
		if (flags.verbose)
#ifndef JP
			pline("%s%s not affected.", buf, vtense(ostr, "are"));
#else
			pline("%s?e?????????????????B", buf);
#endif /*JP*/
		break;

	    case RUST_BLESSPRT:			/* obj's blessing protected it */
		if (flags.verbose)
#ifndef JP
		    if (victim == &youmonst)
			pline("Somehow, your %s %s not affected.",
			      ostr, vtense(ostr, "are"));
		    else if (victim)
			pline("Somehow, %s %s %s not affected.",
			      s_suffix(mon_nam(victim)), ostr, vtense(ostr, "are"));
		    else
			pline("Somehow, the %s %s not affected.",
			      ostr, vtense(ostr, "are"));
#else
		    pline("???????A%s?e?????????????????B", buf);
#endif /*JP*/
		break;

	    case RUST_GREASEPRT:		/* obj's grease protected it */
		if (flags.verbose)
#ifndef JP
			pline("%s%s %s", buf, vtense(ostr, "are"), grstxt);
#else
			pline("%s%s", buf, grstxt);
#endif /*JP*/
		break;

	    case RUST_GREASEOFF:		/* obj's grease dissolved */
#ifndef JP
		pline("%s%s %s", buf, vtense(ostr, "are"), grstxt);
		pline_The("grease dissolves.");
#else
		pline("%s%s", buf, grstxt);
		pline("?????n?????????B");
#endif /*JP*/
		break;

	    case RUST_PROTGONE:			/* obj's protection was lost */
#ifndef JP
		pline("%s%s not affected, but %s no longer protected.", 
			buf, vtense(ostr, "are"), vtense(ostr, "are"));
#else
		pline("%s?e???????????????????A?????????Q?????????????????????B", buf);
#endif /*JP*/
		break;

	    case RUST_DAMAGED1:			/* obj was damaged (1st time) */
	    case RUST_DAMAGED2:			/* obj was damaged further */
	    case RUST_DAMAGED3:			/* obj was completely damaged */
#ifndef JP
		pline("%s%s%s!", buf,
			 vtense(ostr, action[type]), dmglvl[msgtype - RUST_DAMAGED1]);
#else
		pline("%s%s%s?I", buf, dmglvl[msgtype - RUST_DAMAGED1], action[type]);
#endif /*JP*/
		break;

	    case RUST_DAMAGED4:			/* obj was damaged but no change */
#ifndef JP
		pline("%s%s completely %s.", buf,
		     vtense(ostr, Blind ? "feel" : "look"),
		     msg[type]);
#else
		pline("%s???S??%s%s?B", buf, msg[type], Blind ? "??????" : "");
#endif /*JP*/
		break;

	    case RUST_BROKEN:			/* obj was broken */
		switch (type) {
		    case 0:
		    case 2:
#ifndef JP
			pline("%s%s away!", buf, vtense(ostr, type ? "rot" : "burn"));
#else
			pline("%s%s?I", buf, type ? "??????????" : "?R???s????");
#endif /*JP*/
			break;
		    case 1:
		    case 3:
#ifndef JP
			pline("%s%s apart!", buf, vtense(ostr, "break"));
#else
			pline("%s?????U?????I", buf);
#endif /*JP*/
			break;
		    default:
			break;
		}
		break;

	    default:
		break;
	}
}

void
grease_protect(otmp,ostr,victim)
register struct obj *otmp;
register const char *ostr;
struct monst *victim;
{
	static const char txt[] = E_J("protected by the layer of grease!",
				      "?????h?????????????????I");
	boolean vismon = victim && (victim != &youmonst) && canseemon(victim);

	if (ostr) {
#ifndef JP
	    if (victim == &youmonst)
		Your("%s %s %s", ostr, vtense(ostr, "are"), txt);
	    else if (vismon)
		pline("%s's %s %s %s", Monnam(victim),
		    ostr, vtense(ostr, "are"), txt);
#else
	    if (victim == &youmonst || vismon)
		pline("%s??%s??%s", vismon ? Monnam(victim) : "??????", ostr, txt);
#endif /*JP*/
	} else {
#ifndef JP
	    if (victim == &youmonst)
		Your("%s %s",aobjnam(otmp,"are"), txt);
	    else if (vismon)
		pline("%s's %s %s", Monnam(victim), aobjnam(otmp,"are"), txt);
#else
	    if (victim == &youmonst || vismon)
		pline("%s??%s??%s", vismon ? Monnam(victim) : "??????", xname(otmp), txt);
#endif /*JP*/
	}
	if (!rn2(2)) {
	    otmp->greased = 0;
	    if (carried(otmp)) {
		pline_The(E_J("grease dissolves.","?????n?????????B"));
		update_inventory();
	    }
	}
}

struct trap *
maketrap(x,y,typ)
register int x, y, typ;
{
	register struct trap *ttmp;
	register struct rm *lev;
	register boolean oldplace;

	if ((ttmp = t_at(x,y)) != 0) {
	    if (ttmp->ttyp == MAGIC_PORTAL) return (struct trap *)0;
	    oldplace = TRUE;
	    if (u.utrap && (x == u.ux) && (y == u.uy) &&
	      ((u.utraptype == TT_BEARTRAP && typ != BEAR_TRAP) ||
	      (u.utraptype == TT_WEB && typ != WEB) ||
	      (u.utraptype == TT_PIT && typ != PIT && typ != SPIKED_PIT)))
		    u.utrap = 0;
	} else {
	    oldplace = FALSE;
	    ttmp = newtrap();
	    ttmp->tx = x;
	    ttmp->ty = y;
	    ttmp->launch.x = -1;	/* force error if used before set */
	    ttmp->launch.y = -1;
	}
	ttmp->ttyp = typ;
	switch(typ) {
	    case STATUE_TRAP:	    /* create a "living" statue */
	      { struct monst *mtmp;
		struct obj *otmp, *statue;

		statue = mkcorpstat(STATUE, (struct monst *)0,
					&mons[rndmonnum()], x, y, FALSE);
		mtmp = makemon(&mons[statue->corpsenm], 0, 0, NO_MM_FLAGS);
		if (!mtmp) break; /* should never happen */
		while(mtmp->minvent) {
		    otmp = mtmp->minvent;
		    otmp->owornmask = 0;
		    obj_extract_self(otmp);
		    (void) add_to_container(statue, otmp);
		}
		if (In_Cocyutus(&u.uz)) change_material(statue, LIQUID);
		statue->owt = weight(statue);
		mongone(mtmp);
		break;
	      }
	    case ROLLING_BOULDER_TRAP:	/* boulder will roll towards trigger */
		(void) mkroll_launch(ttmp, x, y, BOULDER, 1L);
		break;
	    case HOLE:
	    case PIT:
	    case SPIKED_PIT:
	    case TRAPDOOR:
		lev = &levl[x][y];
		if (*in_rooms(x, y, SHOPBASE) &&
			((typ == HOLE || typ == TRAPDOOR) ||
			 IS_DOOR(lev->typ) || IS_WALL(lev->typ)))
		    add_damage(x, y,		/* schedule repair */
			       ((IS_DOOR(lev->typ) || IS_WALL(lev->typ))
				&& !flags.mon_moving) ? 200L : 0L);
		if (IS_GRAVE(lev->typ))
			dispose_grave(x, y);
		else if (IS_FLOOR(lev->typ))
			/* nothing */;
		else if (IS_ROOM(lev->typ)) /* && !IS_AIR(lev->typ) */
			lev->typ = ROOM;
		/*
		 * some cases which can happen when digging
		 * down while phazing thru solid areas
		 */
		else if (lev->typ == STONE || lev->typ == SCORR)
		    lev->typ = CORR;
		else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
		    lev->typ = level.flags.is_maze_lev ? ROOM :
			       level.flags.is_cavernous_lev ? CORR : DOOR;

		lev->doormask = 0;	/* subsumes altarmask, icedpool... */
		unearth_objs(x, y);
		break;
	    case RUST_TRAP:
		ttmp->vl.v_lasttrig = 0;
		break;
	}
	if (ttmp->ttyp == HOLE) ttmp->tseen = 1;  /* You can't hide a hole */
	else ttmp->tseen = 0;
	ttmp->once = 0;
	ttmp->madeby_u = 0;
	ttmp->dst.dnum = -1;
	ttmp->dst.dlevel = -1;
	if (!oldplace) {
	    ttmp->ntrap = ftrap;
	    ftrap = ttmp;
	}
	return(ttmp);
}

void
fall_through(td)
boolean td;	/* td == TRUE : trap door or hole */
{
	d_level dtmp;
	char msgbuf[BUFSZ];
	const char *dont_fall = 0;
	register int newlevel = dunlev(&u.uz);

	/* KMH -- You can't escape the Sokoban level traps */
	if(Blind && Levitation && !In_sokoban(&u.uz)) return;

	do {
	    newlevel++;
	} while(!rn2(4) && newlevel < dunlevs_in_dungeon(&u.uz));

	/* don't fall into castle town unless you have found it */
	/*if ((u.uz.dnum == oracle_level.dnum) &&*/		/* in main dungeon */
	/*   newlevel == dunlevs_in_dungeon(&u.uz) &&*/	/* bottom level */
	/*    !u.uevent.ufound_town) newlevel--;*/ /*ctown*/

	if(td) {
	    struct trap *t=t_at(u.ux,u.uy);
	    seetrap(t);
	    if (!In_sokoban(&u.uz)) {
		if (t->ttyp == TRAPDOOR)
			pline(E_J("A trap door opens up under you!",
				  "?????????????????????????J?????I"));
		else 
			pline(E_J("There's a gaping hole under you!",
				  "?????????????????????????????J?????????I"));
	    }
	} else pline_The(E_J("%s opens up under you!","??????%s???????J?????I"), surface(u.ux,u.uy));

	if (In_sokoban(&u.uz) && Can_fall_thru(&u.uz))
	    ;	/* KMH -- You can't escape the Sokoban level traps */
	else if(Levitation || u.ustuck || !Can_fall_thru(&u.uz)
	   || Flying || is_clinger(youmonst.data)
	   || (Role_if(PM_ARCHEOLOGIST) && !Upolyd && uwep && uwep->otyp == BULLWHIP )
	   || (Inhell && !u.uevent.invoked &&
					newlevel == dunlevs_in_dungeon(&u.uz))
		) {
	    dont_fall = E_J("don't fall in.","?????????????B");
	} else if (can_levitate() &&
		   (yn(E_J("Levitate to avoid it?","???V?????????????????H")) == 'y')) {
	    in_steed_dismounting = TRUE;
	    start_levitation();
	    in_steed_dismounting = FALSE;
	    dont_fall = E_J("don't fall in.","?????????????B");
	} else if (youmonst.data->msize >= MZ_HUGE) {
	    dont_fall = E_J("don't fit through.","?????????????????B");
	} else if (!next_to_u()) {
	    dont_fall = E_J("are jerked back by your pet!","?y?b?g???????????????I");
	}
	if (dont_fall) {
	    if (Role_if(PM_ARCHEOLOGIST) && !Upolyd && uwep && uwep->otyp == BULLWHIP )
		pline(E_J("But thanks to your trusty whip ...",
			  "?????A?????????M?????????????????????c"));
	    You(dont_fall);
	    /* hero didn't fall through, but any objects here might */
	    impact_drop((struct obj *)0, u.ux, u.uy, 0);
	    if (!td) {
		display_nhwindow(WIN_MESSAGE, FALSE);
		pline_The(E_J("opening under you closes up.","???????J?????????????????B"));
	    }
	    return;
	}

	if(*u.ushops) shopdig(1);
	if (Is_stronghold(&u.uz)) {
	    find_hell(&dtmp);
	} else {
	    dtmp.dnum = u.uz.dnum;
	    dtmp.dlevel = newlevel;
	}
	if (!td)
	    Sprintf(msgbuf, E_J("The hole in the %s above you closes up.",
				"%s???J???????????????????????????B"),
		    ceiling(u.ux,u.uy));
	schedule_goto(&dtmp, FALSE, TRUE, 0,
		      (char *)0, !td ? msgbuf : (char *)0);
}

/*
 * Animate the given statue.  May have been via shatter attempt, trap,
 * or stone to flesh spell.  Return a monster if successfully animated.
 * If the monster is animated, the object is deleted.  If fail_reason
 * is non-null, then fill in the reason for failure (or success).
 *
 * The cause of animation is:
 *
 *	ANIMATE_NORMAL  - hero "finds" the monster
 *	ANIMATE_SHATTER - hero tries to destroy the statue
 *	ANIMATE_SPELL   - stone to flesh spell hits the statue
 *
 * Perhaps x, y is not needed if we can use get_obj_location() to find
 * the statue's location... ???
 */
struct monst *
animate_statue(statue, x, y, cause, fail_reason)
struct obj *statue;
xchar x, y;
int cause;
int *fail_reason;
{
	struct permonst *mptr;
	struct monst *mon = 0;
	struct obj *item;
	coord cc;
	boolean historic = (Role_if(PM_ARCHEOLOGIST) && !flags.mon_moving && (statue->spe & STATUE_HISTORIC));
	boolean frozen;
	char statuename[BUFSZ];

	Strcpy(statuename,E_J(the(xname(statue)),xname(statue)));

	if (statue->omon) {
	    cc.x = x,  cc.y = y;
	    mon = montraits(statue, &cc);
	    if (mon && mon->mtame && !mon->isminion)
		wary_dog(mon, TRUE);
	} else {
	    /* statue of any golem hit with stone-to-flesh becomes flesh golem */
	    if (is_golem(&mons[statue->corpsenm]) && cause == ANIMATE_SPELL)
	    	mptr = &mons[PM_FLESH_GOLEM];
	    else
		mptr = &mons[statue->corpsenm];
	    /*
	     * Guard against someone wishing for a statue of a unique monster
	     * (which is allowed in normal play) and then tossing it onto the
	     * [detected or guessed] location of a statue trap.  Normally the
	     * uppermost statue is the one which would be activated.
	     */
	    if ((mptr->geno & G_UNIQ) && cause != ANIMATE_SPELL) {
	        if (fail_reason) *fail_reason = AS_MON_IS_UNIQUE;
	        return (struct monst *)0;
	    }
	    if (cause == ANIMATE_SPELL &&
		((mptr->geno & G_UNIQ) || mptr->msound == MS_GUARDIAN)) {
		/* Statues of quest guardians or unique monsters
		 * will not stone-to-flesh as the real thing.
		 */
		mon = makemon(&mons[PM_DOPPELGANGER], x, y,
			NO_MINVENT|MM_NOCOUNTBIRTH|MM_ADJACENTOK);
		if (mon) {
			/* makemon() will set mon->cham to
			 * CHAM_ORDINARY if hero is wearing
			 * ring of protection from shape changers
			 * when makemon() is called, so we have to
			 * check the field before calling newcham().
			 */
			if (mon->cham == CHAM_DOPPELGANGER)
				(void) newcham(mon, mptr, FALSE, FALSE);
		}
	    } else
		mon = makemon(mptr, x, y, (cause == ANIMATE_SPELL) ?
			(NO_MINVENT | MM_ADJACENTOK) : NO_MINVENT);
	}

	if (!mon) {
	    if (fail_reason) *fail_reason = AS_NO_MON;
	    return (struct monst *)0;
	}
	frozen = (get_material(statue) == LIQUID && can_be_frozen(mon->data));

	/* in case statue is wielded and hero zaps stone-to-flesh at self */
	if (statue->owornmask) remove_worn_item(statue, TRUE);

	/* allow statues to be of a specific gender */
	if (statue->spe & STATUE_MALE)
	    mon->female = FALSE;
	else if (statue->spe & STATUE_FEMALE)
	    mon->female = TRUE;
	/* if statue has been named, give same name to the monster */
	if (statue->has_name)
	    mon = christen_monst(mon, ONAME(statue));
	/* transfer any statue contents to monster's inventory */
	while ((item = statue->cobj) != 0) {
	    obj_extract_self(item);
	    (void) add_to_minv(mon, item);
	}
	m_dowear(mon, TRUE);
	delobj(statue);

	/* mimic statue becomes seen mimic; other hiders won't be hidden */
	if (mon->m_ap_type) seemimic(mon);
	else mon->mundetected = FALSE;
	if ((x == u.ux && y == u.uy) || cause == ANIMATE_SPELL) {
	    const char *comes_to_life = nonliving(mon->data) ?
					E_J("moves","?????o????") : E_J("comes to life","????????????"); 
	    if (frozen) {
		if (canspotmon(mon))
#ifndef JP
		    pline("%s is still %s!", An(mon_nam(mon)),
			  nonliving(mon->data) ? "moving" : "alive");
#else
		    pline("%s??????%s???????I", mon_nam(mon),
			  nonliving(mon->data) ? "????" : "????");
#endif /*JP*/
		else pline_The(E_J("ice-frozen corpse disappears!","?X???????????????????????I"));
	    } else {
		if (cause == ANIMATE_SPELL)
		    pline(E_J("%s %s!","%s??%s?I"), E_J(upstart(statuename),statuename),
			    canspotmon(mon) ? comes_to_life : E_J("disappears","??????????"));
		else
		    pline_The(E_J("statue %s!","??????%s?I"),
			    canspotmon(mon) ? comes_to_life : E_J("disappears","??????????"));
		if (historic) {
		    You_feel(E_J("guilty that the historic statue is now gone.",
				 "???j?????????????????????????????????o?????B"));
		    adjalign(-1);
		}
	    }
	} else if (cause == ANIMATE_SHATTER)
	    pline(E_J("Instead of %sing, the %s suddenly %s!",
		      "%s?????????A%s?????R%s?I"),
		frozen ? E_J("melt","?Z????") : E_J("shatter","?????U??"),
		frozen ? E_J("corpse","????") : E_J("statue","????"),
		canspotmon(mon) ? E_J("comes to life","?????o????") : E_J("disappears","??????????"));
	else {/* cause == ANIMATE_NORMAL */
	    You(E_J("find %s posing as %s.","%s??%s???????????????????C???????B"),
		canspotmon(mon) ? a_monnam(mon) : something,
		frozen ? E_J("frozen","??????????") : E_J("a statue","??????"));
	    stop_occupation();
	}
	/* avoid hiding under nothing */
	if (x == u.ux && y == u.uy &&
		Upolyd && hides_under(youmonst.data) && !OBJ_AT(x, y))
	    u.uundetected = 0;

	if (fail_reason) *fail_reason = AS_OK;
	return mon;
}

/*
 * You've either stepped onto a statue trap's location or you've triggered a
 * statue trap by searching next to it or by trying to break it with a wand
 * or pick-axe.
 */
struct monst *
activate_statue_trap(trap, x, y, shatter)
struct trap *trap;
xchar x, y;
boolean shatter;
{
	struct monst *mtmp = (struct monst *)0;
	struct obj *otmp = sobj_at(STATUE, x, y);
	int fail_reason;

	/*
	 * Try to animate the first valid statue.  Stop the loop when we
	 * actually create something or the failure cause is not because
	 * the mon was unique.
	 */
	deltrap(trap);
	while (otmp) {
	    mtmp = animate_statue(otmp, x, y,
		    shatter ? ANIMATE_SHATTER : ANIMATE_NORMAL, &fail_reason);
	    if (mtmp || fail_reason != AS_MON_IS_UNIQUE) break;

	    while ((otmp = otmp->nexthere) != 0)
		if (otmp->otyp == STATUE) break;
	}

	if (Blind) feel_location(x, y);
	else newsym(x, y);
	return mtmp;
}

#ifdef STEED
STATIC_OVL boolean
keep_saddle_with_steedcorpse(steed_mid, objchn, saddle)
unsigned steed_mid;
struct obj *objchn, *saddle;
{
	if (!saddle) return FALSE;
	while(objchn) {
		if(objchn->otyp == CORPSE && objchn->omon) {
			struct monst *mtmp;
			mtmp = objchn->omon;
			if (mtmp && mtmp->m_id == steed_mid) {
				/* move saddle */
				xchar x,y;
				if (get_obj_location(objchn, &x, &y, 0)) {
					obj_extract_self(saddle);
					place_object(saddle, x, y);
					stackobj(saddle);
				}
				return TRUE;
			}
		}
		if (Has_contents(objchn) &&
		    keep_saddle_with_steedcorpse(steed_mid, objchn->cobj, saddle))
			return TRUE;
		objchn = objchn->nobj;
	}
	return FALSE;
}
#endif /*STEED*/

void
dotrap(trap, trflags)
register struct trap *trap;
unsigned trflags;
{
	register int ttype = trap->ttyp;
	register struct obj *otmp;
	boolean already_seen = trap->tseen;
	boolean webmsgok = (!(trflags & NOWEBMSG));
	boolean forcebungle = (trflags & FORCEBUNGLE);

	nomul(0);

	/* KMH -- You can't escape the Sokoban level traps */
	if (In_sokoban(&u.uz) &&
			(ttype == PIT || ttype == SPIKED_PIT || ttype == HOLE ||
			ttype == TRAPDOOR)) {
	    /* The "air currents" message is still appropriate -- even when
	     * the hero isn't flying or levitating -- because it conveys the
	     * reason why the player cannot escape the trap with a dexterity
	     * check, clinging to the ceiling, etc.
	     */
#ifndef JP
	    pline("Air currents pull you down into %s %s!",
	    	a_your[trap->madeby_u],
	    	defsyms[trap_to_defsym(ttype)].explanation);
#else
	    pline("???????C????????????%s???????????????I",
	    	defsyms[trap_to_defsym(ttype)].explanation);
#endif
	    /* then proceed to normal trap effect */
	} else if (already_seen) {
	    if ((Levitation || Flying) &&
		    (ttype == PIT || ttype == SPIKED_PIT || ttype == HOLE ||
		    ttype == BEAR_TRAP)) {
#ifndef JP
		You("%s over %s %s.",
		    Levitation ? "float" : "fly",
		    a_your[trap->madeby_u],
		    defsyms[trap_to_defsym(ttype)].explanation);
#else
		You("%s????%s?????B",
		    defsyms[trap_to_defsym(ttype)].explanation,
		    Levitation ? "????????" : "????????");
#endif
		return;
	    }
	    if(!Fumbling && ttype != MAGIC_PORTAL &&
		ttype != ANTI_MAGIC && !forcebungle &&
		(!rn2(5) ||
	    ((ttype == PIT || ttype == SPIKED_PIT) && is_clinger(youmonst.data)))) {
#ifndef JP
		You("escape %s %s.",
		    (ttype == ARROW_TRAP && !trap->madeby_u) ? "an" :
			a_your[trap->madeby_u],
		    defsyms[trap_to_defsym(ttype)].explanation);
#else
		You("%s???????????????B",
		    defsyms[trap_to_defsym(ttype)].explanation);
#endif /*JP*/
		return;
	    }
	}

#ifdef STEED
	if (u.usteed) u.usteed->mtrapseen |= (1 << (ttype-1));
#endif

	switch(ttype) {
	    case ARROW_TRAP:
		if (trap->once && trap->tseen && !rn2(15)) {
		    You_hear(E_J("a loud click!","!?@?B???d??????????"));
		    deltrap(trap);
		    newsym(u.ux,u.uy);
		    break;
		}
		trap->once = 1;
		seetrap(trap);
		pline(E_J("An arrow shoots out at you!","???????????????????????????I"));
		otmp = mksobj(ARROW, TRUE, FALSE);
		otmp->quan = 1L;
		otmp->owt = weight(otmp);
		otmp->opoisoned = 0;
#ifdef JP
		bullet_killername((struct monst *)0, otmp, 0);
#endif /*JP*/
#ifdef STEED
		if (u.usteed && !rn2(2) && steedintrap(trap, otmp)) /* nothing */;
		else
#endif
		if (thitu(8, dmgval(otmp, &youmonst), otmp, E_J("arrow","??"))) {
		    obfree(otmp, (struct obj *)0);
		} else {
		    place_object(otmp, u.ux, u.uy);
		    if (!Blind) otmp->dknown = 1;
		    stackobj(otmp);
		    newsym(u.ux, u.uy);
		}
		break;
	    case DART_TRAP:
		if (trap->once && trap->tseen && !rn2(15)) {
		    You_hear(E_J("a soft click.","?@?B???y??????????"));
		    deltrap(trap);
		    newsym(u.ux,u.uy);
		    break;
		}
		trap->once = 1;
		seetrap(trap);
		pline(E_J("A little dart shoots out at you!","???????_?[?c?????????????????????????I"));
		otmp = mksobj(DART, TRUE, FALSE);
		otmp->quan = 1L;
		otmp->owt = weight(otmp);
		if (!rn2(6)) otmp->opoisoned = 1;
#ifdef JP
		bullet_killername((struct monst *)0, otmp, 0);
#endif /*JP*/
#ifdef STEED
		if (u.usteed && !rn2(2) && steedintrap(trap, otmp)) /* nothing */;
		else
#endif
		if (thitu(7, dmgval(otmp, &youmonst), otmp, E_J("little dart","???????_?[?c"))) {
		    if (otmp->opoisoned)
			poisoned(E_J("dart","?_?[?c"), A_CON,
				 E_J("little dart","???_?[?c???E??????"), -10);
		    obfree(otmp, (struct obj *)0);
		} else {
		    place_object(otmp, u.ux, u.uy);
		    if (!Blind) otmp->dknown = 1;
		    stackobj(otmp);
		    newsym(u.ux, u.uy);
		}
		break;
	    case ROCKTRAP:
		if (trap->once && trap->tseen && !rn2(15)) {
		    pline(E_J("A trap door in %s opens, but nothing falls out!",
			      "%s?????????????J???????A?????????????????????I"),
			  E_J(the(ceiling(u.ux,u.uy)),ceiling(u.ux,u.uy)));
		    deltrap(trap);
		    newsym(u.ux,u.uy);
		} else {
		    int dmg = d(2,6); /* should be std ROCK dmg? */

		    trap->once = 1;
		    seetrap(trap);
		    otmp = mksobj_at(ROCK, u.ux, u.uy, TRUE, FALSE);
		    otmp->quan = 1L;
		    otmp->owt = weight(otmp);
//		    otmp->oeaten = 0;

#ifndef JP
		    pline("A trap door in %s opens and %s falls on your %s!",
			  the(ceiling(u.ux,u.uy)),
			  an(xname(otmp)),
			  body_part(HEAD));
#else
		    pline("%s?????????????J???A%s??????????%s?????????~?????????I",
			  ceiling(u.ux,u.uy), xname(otmp), body_part(HEAD));
#endif /*JP*/

		    if (uarmh) {
			if(is_metallic(uarmh) || get_material(uarmh) == GLASS) {
			    pline(E_J("Fortunately, you are wearing a hard helmet.",
				      "?K???A?????????d???????g?????????????B"));
			    dmg = 2;
			} else if (flags.verbose) {
			    Your(E_J("%s does not protect you.",
				     "%s???????????h???????????B"), xname(uarmh));
			}
		    }

		    if (!Blind) otmp->dknown = 1;
		    stackobj(otmp);
		    newsym(u.ux,u.uy);	/* map the rock */

		    losehp(dmg, E_J("falling rock","??????"), KILLED_BY_AN);
		    exercise(A_STR, FALSE);
		}
		break;

	    case SQKY_BOARD:	    /* stepped on a squeaky board */
		if (Levitation || Flying) {
		    if (!Blind) {
			seetrap(trap);
			if (Hallucination)
				You(E_J("notice a crease in the linoleum.",
					"???m???E?????????????????????????????C???????B"));
			else
				You(E_J("notice a loose board below you.",
					"???????????????????????????C???????B"));
		    }
		} else {
		    seetrap(trap);
		    pline(E_J("A board beneath you squeaks loudly.",
			      "?????????????????????????????a?????B"));
		    wake_nearby();
		}
		break;

	    case BEAR_TRAP:
		if(Levitation || Flying) break;
		seetrap(trap);
		if(amorphous(youmonst.data) || is_whirly(youmonst.data) ||
						    unsolid(youmonst.data)) {
#ifndef JP
		    pline("%s bear trap closes harmlessly through you.",
			    A_Your[trap->madeby_u]);
#else
		    pline("?g???o?T?~???????????????????????????B");
#endif /*JP*/
		    break;
		}
		if(
#ifdef STEED
		   !u.usteed &&
#endif
		   youmonst.data->msize <= MZ_SMALL) {
#ifndef JP
		    pline("%s bear trap closes harmlessly over you.",
			    A_Your[trap->madeby_u]);
#else
		    pline("?g???o?T?~???????????????????????B");
#endif /*JP*/
		    break;
		}
		u.utrap = rn1(4, 4);
		u.utraptype = TT_BEARTRAP;
#ifdef STEED
		if (u.usteed) {
#ifndef JP
		    pline("%s bear trap closes on %s %s!",
			A_Your[trap->madeby_u], s_suffix(mon_nam(u.usteed)),
			mbodypart(u.usteed, FOOT));
#else
		    pline("?g???o?T?~??%s??%s?????????????I",
			mon_nam(u.usteed), mbodypart(u.usteed, FOOT));
#endif /*JP*/
		} else
#endif
		{
#ifndef JP
		    pline("%s bear trap closes on your %s!",
			    A_Your[trap->madeby_u], body_part(FOOT));
		    if(u.umonnum == PM_OWLBEAR || u.umonnum == PM_BUGBEAR)
			You("howl in anger!");
#else
		    pline("?g???o?T?~??????????%s?????????????I", body_part(FOOT));
		    if(u.umonnum == PM_TIGER) /* beartrap???g???o?T?~?????????????A???? */
			You("?{???????K?????????I");
#endif /*JP*/
		}
		exercise(A_DEX, FALSE);
		break;

	    case SLP_GAS_TRAP:
		seetrap(trap);
		if(Sleep_resistance || breathless(youmonst.data)) {
		    You(E_J("are enveloped in a cloud of gas!","?????o?????K?X???????????I"));
		    break;
		}
		pline(E_J("A cloud of gas puts you to sleep!",
			  "?????o?????K?X?????????A???????????????????????????I"));
		fall_asleep(-rnd(25), TRUE);
#ifdef STEED
		(void) steedintrap(trap, (struct obj *)0);
#endif
		break;

	    case RUST_TRAP:
		seetrap(trap);
		if (trap->vl.v_lasttrig > monstermoves) {
		    pline(E_J("Some dump air hits you.",
			      "?????????C?????????????????????????????????B"));
		    break;
		}
		trap->vl.v_lasttrig = monstermoves + rn1(100,100);
		if (u.umonnum == PM_IRON_GOLEM) {
		    int dam = u.mhmax;

		    pline(E_J("%s you!","?????????????????I"), A_gush_of_water_hits);
		    You(E_J("are covered with rust!","?S?g?K???????????????I"));
		    if (Half_physical_damage) dam = (dam+1) / 2;
		    losehp(dam, E_J("rusting away","?K????????????"), KILLED_BY);
		    break;
		} else if (u.umonnum == PM_GREMLIN && rn2(3)) {
		    pline(E_J("%s you!","?????????????????I"), A_gush_of_water_hits);
		    (void)split_mon(&youmonst, (struct monst *)0);
		    break;
		} else if (u.umonnum == PM_FIRE_ELEMENTAL ||
			   u.umonnum == PM_FIRE_VORTEX ||
			   u.umonnum == PM_FLAMING_SPHERE) {
		    pline(E_J("%s you!","?????????????????I"), A_gush_of_water_hits);
		    losehp(rnd(20), E_J("gush of water","?????o??????"), E_J(KILLED_BY_AN,KILLED_SUFFIX));
		    break;
		}

	    /* Unlike monsters, traps cannot aim their rust attacks at
	     * you, so instead of looping through and taking either the
	     * first rustable one or the body, we take whatever we get,
	     * even if it is not rustable.
	     */
		switch (rn2(5)) {
		    case 0:
			pline(E_J("%s you on the %s!","%s????????%s???????????I"),
				A_gush_of_water_hits, body_part(HEAD));
			(void) rust_dmg(uarmh, helm_simple_name(uarmh), 1, TRUE, &youmonst);
			break;
		    case 1:
			pline(E_J("%s your left %s!","%s??????????%s???????????I"),
				A_gush_of_water_hits, body_part(ARM));
			if (rust_dmg(uarms, E_J("shield","??"), 1, TRUE, &youmonst))
			    break;
			if (u.twoweap || (uwep && bimanual(uwep)))
			    erode_obj(u.twoweap ? uswapwep : uwep, FALSE, TRUE);
glovecheck:		(void) rust_dmg(uarmg, E_J("gauntlets","?U??"), 1, TRUE, &youmonst);
			/* Not "metal gauntlets" since it gets called
			 * even if it's leather for the message
			 */
			break;
		    case 2:
			pline(E_J("%s your right %s!","%s?????????E%s???????????I"),
				A_gush_of_water_hits, body_part(ARM));
			erode_obj(uwep, FALSE, TRUE);
			goto glovecheck;
		    default:
			pline(E_J("%s you!","%s?????????????????I"), A_gush_of_water_hits);
			for (otmp=invent; otmp; otmp = otmp->nobj)
				    (void) snuff_lit(otmp);
			if (uarmc)
			    (void) rust_dmg(uarmc, cloak_simple_name(uarmc),
						1, TRUE, &youmonst);
			else if (uarm)
			    (void) rust_dmg(uarm, armor_simple_name(uarm), 1, TRUE, &youmonst);
			else if (uarmu)
			    (void) rust_dmg(uarmu, E_J("shirt","?V???c"), 1, TRUE, &youmonst);
		}
		update_inventory();
		break;

	    case FIRE_TRAP:
		seetrap(trap);
		dofiretrap((struct obj *)0);
		break;

	    case PIT:
	    case SPIKED_PIT:
		/* KMH -- You can't escape the Sokoban level traps */
		if (!In_sokoban(&u.uz) && (Levitation || Flying)) break;
		seetrap(trap);
		if (!In_sokoban(&u.uz) &&
		    (is_clinger(youmonst.data) || (can_levitate()
#ifdef STEED
		     && !u.usteed
#endif
		     ))) {
		    boolean iscl = is_clinger(youmonst.data);
		    if(already_seen) {
#ifndef JP
			You("see %s %spit below you%c", a_your[trap->madeby_u],
			    ttype == SPIKED_PIT ? "spiked " : "",
			    iscl ? '.' : '!');
#else
			Your("????%s??????????????%s",
			    ttype == SPIKED_PIT ? "??????????" : "",
			    iscl ? "?B" : "?I");
#endif /*JP*/
			if (!iscl) {
			    if (yn(E_J("Levitate to avoid it?","???V?????????????????H")) == 'y') {
				in_steed_dismounting = TRUE;
				start_levitation();
				in_steed_dismounting = FALSE;
			    } else
				goto pitfall;
			}
		    } else {
#ifndef JP
			pline("%s pit %sopens up under you!",
			    A_Your[trap->madeby_u],
			    ttype == SPIKED_PIT ? "full of spikes " : "");
			You("don't fall in!");
#else
			Your("?^????%s???????????????J?????I",
			    A_Your[trap->madeby_u],
			    ttype == SPIKED_PIT ? "??????????" : "");
			if (!iscl) {
			    if (yn(E_J("Levitate to avoid it?","???V?????????????????H")) == 'y') {
				in_steed_dismounting = TRUE;
				start_levitation();
				in_steed_dismounting = FALSE;
			    } else
				goto pitfall;
			}
			You("?????????????B");
#endif /*JP*/
		    }
		    break;
		}
		if (!In_sokoban(&u.uz)) {
		    char verbbuf[BUFSZ];
pitfall:
#ifdef STEED
		    if (u.usteed) {
		    	if ((trflags & RECURSIVETRAP) != 0)
			    Sprintf(verbbuf, E_J("and %s fall","??%s??????????????????"),
				x_monnam(u.usteed,
				    u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
				    (char *)0, SUPPRESS_SADDLE, FALSE));
			else
			    Sprintf(verbbuf,E_J("lead %s","??%s??????????????????"),
				x_monnam(u.usteed,
					 u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
				 	 E_J("poor","????????????"), SUPPRESS_SADDLE, FALSE));
		    } else
#endif
#ifndef JP
		    Strcpy(verbbuf,"fall");
		    You("%s into %s pit!", verbbuf, a_your[trap->madeby_u]);
#else
		    Strcpy(verbbuf,"??????????????????");
		    pline("??????%s?I", verbbuf);
#endif /*JP*/
		}
		/* wumpus reference */
		if (Role_if(PM_RANGER) && !trap->madeby_u && !trap->once &&
			In_quest(&u.uz) && Is_qlocate(&u.uz)) {
		    pline(E_J("Fortunately it has a bottom after all...",
			      "?K???A?????????????????????????c?B"));
		    trap->once = 1;
		}
#ifndef JP
		else if (u.umonnum == PM_PIT_VIPER ||
			u.umonnum == PM_PIT_FIEND)
		    pline("How pitiful.  Isn't that the pits?");
#endif /*JP*/
		if (ttype == SPIKED_PIT) {
		    const char *predicament = E_J("on a set of sharp iron spikes",
						  "?d?|???????????????????s???S????");
#ifdef STEED
		    if (u.usteed) {
#ifndef JP
			pline("%s lands %s!",
				upstart(x_monnam(u.usteed,
					 u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
					 E_J("poor","????????????"), SUPPRESS_SADDLE, FALSE)),
			      predicament);
#else
			pline("%s??%s???????h???????I", predicament,
				upstart(x_monnam(u.usteed,
					 u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
					 E_J("poor","????????????"), SUPPRESS_SADDLE, FALSE)));
#endif /*JP*/
		    } else
#endif
#ifndef JP
		    You("land %s!", predicament);
#else
		    pline("%s???????????????h?????I", predicament);
#endif /*JP*/
		}
		if (!Passes_walls)
		    u.utrap = rn1(6,2);
		u.utraptype = TT_PIT;
#ifdef STEED
		if (!steedintrap(trap, (struct obj *)0)) {
#endif
		if (ttype == SPIKED_PIT) {
		    losehp(rnd(10),E_J("fell into a pit of iron spikes",
				       "?S???????d?|????????????????????????"),
			E_J(NO_KILLER_PREFIX,KILLED_BY));
		    if (!rn2(6))
			poisoned(E_J("spikes","??"), A_STR,
				 E_J("fall onto poison spikes","?????h??????????????????????????"), 8);
		} else
		    losehp(rnd(6),E_J("fell into a pit","????????????????"), E_J(NO_KILLER_PREFIX,KILLED_BY));
		if (Punished && !carried(uball)) {
		    unplacebc();
		    ballfall();
		    placebc();
		}
		selftouch(E_J("Falling, you","???????A????????"));
		vision_full_recalc = 1;	/* vision limits change */
		exercise(A_STR, FALSE);
		exercise(A_DEX, FALSE);
#ifdef STEED
		}
#endif
		break;
	    case HOLE:
	    case TRAPDOOR:
		if (!Can_fall_thru(&u.uz)) {
		    seetrap(trap);	/* normally done in fall_through */
		    impossible("dotrap: %ss cannot exist on this level.",
			       defsyms[trap_to_defsym(ttype)].explanation);
		    break;		/* don't activate it after all */
		}
		fall_through(TRUE);
		break;

	    case TELEP_TRAP:
		seetrap(trap);
		tele_trap(trap);
		break;
	    case LEVEL_TELEP:
		seetrap(trap);
		level_tele_trap(trap);
		break;

	    case WEB: /* Our luckless player has stumbled into a web. */
		seetrap(trap);
		if (amorphous(youmonst.data) || is_whirly(youmonst.data) ||
						    unsolid(youmonst.data)) {
		    if (acidic(youmonst.data) || u.umonnum == PM_GELATINOUS_CUBE ||
			u.umonnum == PM_FIRE_ELEMENTAL) {
			if (webmsgok)
#ifndef JP
			    You("%s %s spider web!",
				(u.umonnum == PM_FIRE_ELEMENTAL) ? "burn" : "dissolve",
				a_your[trap->madeby_u]);
#else
			    You("?w????????%s?I",
				(u.umonnum == PM_FIRE_ELEMENTAL) ? "?R??????" : "?n??????",
				a_your[trap->madeby_u]);
#endif /*JP*/
			deltrap(trap);
			newsym(u.ux,u.uy);
			break;
		    }
		    if (webmsgok) You(E_J("flow through %s spider web.",
					  "%s?w???????????????????B"),
			    a_your[trap->madeby_u]);
		    break;
		}
		if (webmaker(youmonst.data)) {
		    if (webmsgok)
		    	pline(trap->madeby_u ?
				E_J("You take a walk on your web.","?????????????????????????????????B") :
				E_J("There is a spider web here.","?????????w?????????????B"));
		    break;
		}
		if (webmsgok) {
		    char verbbuf[BUFSZ];
		    verbbuf[0] = '\0';
#ifndef JP
#ifdef STEED
		    if (u.usteed)
		   	Sprintf(verbbuf,"lead %s",
				x_monnam(u.usteed,
					 u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
				 	 "poor", SUPPRESS_SADDLE, FALSE));
		    else
#endif
		    Sprintf(verbbuf, "%s", Levitation ? (const char *)"float" :
		      		locomotion(youmonst.data, "stumble"));
		    You("%s into %s spider web!",
			verbbuf, a_your[trap->madeby_u]);
#else /*JP*/
#ifdef STEED
		    if (u.usteed)
		   	Sprintf(verbbuf,"%s???w????????????????????",
				x_monnam(u.usteed,
					 u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
				 	 "??????", SUPPRESS_SADDLE, FALSE));
		    else
#endif
		    Strcpy(verbbuf, "?w????????????????????");
		    You("%s?I", verbbuf);
#endif /*JP*/
		}
		u.utraptype = TT_WEB;

		/* Time stuck in the web depends on your/steed strength. */
		{
		    register int str = ACURR(A_STR);

#ifdef STEED
		    /* If mounted, the steed gets trapped.  Use mintrap
		     * to do all the work.  If mtrapped is set as a result,
		     * unset it and set utrap instead.  In the case of a
		     * strongmonst and mintrap said it's trapped, use a
		     * short but non-zero trap time.  Otherwise, monsters
		     * have no specific strength, so use player strength.
		     * This gets skipped for webmsgok, which implies that
		     * the steed isn't a factor.
		     */
		    if (u.usteed && webmsgok) {
			/* mtmp location might not be up to date */
			u.usteed->mx = u.ux;
			u.usteed->my = u.uy;

			/* mintrap currently does not return 2(died) for webs */
			if (mintrap(u.usteed)) {
			    u.usteed->mtrapped = 0;
			    if (strongmonst(u.usteed->data)) str = 17;
			} else {
			    break;
			}

			webmsgok = FALSE; /* mintrap printed the messages */
		    }
#endif
		    if (str <= 3) u.utrap = rn1(6,6);
		    else if (str < 6) u.utrap = rn1(6,4);
		    else if (str < 9) u.utrap = rn1(4,4);
		    else if (str < 12) u.utrap = rn1(4,2);
		    else if (str < 15) u.utrap = rn1(2,2);
		    else if (str < 18) u.utrap = rnd(2);
		    else if (str < 69) u.utrap = 1;
		    else {
			u.utrap = 0;
			if (webmsgok)
			    You(E_J("tear through %s web!","%s?w?????????j?????????I"),
				a_your[trap->madeby_u]);
			deltrap(trap);
			newsym(u.ux,u.uy);	/* get rid of trap symbol */
		    }
		}
		break;

	    case STATUE_TRAP:
		(void) activate_statue_trap(trap, u.ux, u.uy, FALSE);
		break;

	    case MAGIC_TRAP:	    /* A magic trap. */
		seetrap(trap);
		if (!rn2(30)) {
		    deltrap(trap);
		    newsym(u.ux,u.uy);	/* update position */
		    You(E_J("are caught in a magical explosion!","?????????????????????????I"));
		    losehp(rnd(10), E_J("magical explosion","????????????"), KILLED_BY_AN);
		    Your(E_J("body absorbs some of the magical energy!",
			     "?g???????@???G?l???M?[???????????z???????I"));
		    u.uen = (u.uenmax += 2);
		    flags.botl = 1;
		} else domagictrap();
#ifdef STEED
		(void) steedintrap(trap, (struct obj *)0);
#endif
		break;

	    case ANTI_MAGIC:
		seetrap(trap);
		if(Antimagic) {
		    shieldeff(u.ux, u.uy);
		    You_feel(E_J("momentarily lethargic.","???u?E?????????????????B"));
		    damage_resistant_obj(ANTIMAGIC, rnd(3));
		} else drain_en(rnd(u.ulevel) + 1);
		if (u.uspellprot) {
		    pline_The(E_J("%s haze around you vanishes away!",
				  "??????????????%s???????????????I"), hcolor(NH_GOLDEN));
		    u.usptime = u.uspmtime = u.uspellprot = 0;
		    find_ac();
		}
		if (u.uspellit) {
		    extinguish_torch();
		    Your(E_J("magic torch is extinguished!","???@???????????????????I"));
		}
		break;

	    case POLY_TRAP: {
	        char verbbuf[BUFSZ];
		seetrap(trap);
#ifdef STEED
		if (u.usteed)
			Sprintf(verbbuf, "lead %s",
				x_monnam(u.usteed,
					 u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
				 	 (char *)0, SUPPRESS_SADDLE, FALSE));
		else
#endif
		 Sprintf(verbbuf,"%s",
		    Levitation ? (const char *)E_J("float","??????") :
		    locomotion(youmonst.data, E_J("step","????")));
		You(E_J("%s onto a polymorph trap!",
			"??????????%s???????I"), verbbuf);
		if(Antimagic || Unchanging) {
		    shieldeff(u.ux, u.uy);
		    You_feel(E_J("momentarily different.","???u???a?????o?????B"));
		    if (Antimagic) damage_resistant_obj(ANTIMAGIC, rnd(3));
		    /* Trap did nothing; don't remove it --KAA */
		} else {
#ifdef STEED
		    (void) steedintrap(trap, (struct obj *)0);
#endif
		    deltrap(trap);	/* delete trap before polymorph */
		    newsym(u.ux,u.uy);	/* get rid of trap symbol */
		    You_feel(E_J("a change coming over you.","???????K???????????????B"));
		    polyself(FALSE);
		}
		break;
	    }
	    case LANDMINE: {
#ifdef STEED
		unsigned steed_mid = 0;
		struct obj *saddle = 0;
#endif
		if (Levitation || Flying) {
		    if (!already_seen && rn2(3)) break;
		    seetrap(trap);
#ifndef JP
		    pline("%s %s in a pile of soil below you.",
			    already_seen ? "There is" : "You discover",
			    trap->madeby_u ? "the trigger of your mine" :
					     "a trigger");
#else /*JP*/
		    pline("%s???????y??????%s?n?????N?????u%s?B",
			    already_seen ? "" : "????????",
			    trap->madeby_u ? "???????d?|????" : "",
			    already_seen ? "????????" : "??????????");
#endif /*JP*/
		    if (already_seen && rn2(3)) break;
#ifndef JP
		    pline("KAABLAMM!!!  %s %s%s off!",
			  forcebungle ? "Your inept attempt sets" :
					"The air currents set",
			    already_seen ? a_your[trap->madeby_u] : "",
			    already_seen ? " land mine" : "it");
#else /*JP*/
		    pline("?h?O?I?H?H??!!! %s%s%s?I",
			  forcebungle ? "?????????y????????????????" :
					"???C????????????????",
			    trap->madeby_u ? "???????d?|????": "",
			    already_seen ? "?n?????N??????" : "???????????N??????");
#endif /*JP*/
		} else {
#ifdef STEED
		    /* prevent landmine from killing steed, throwing you to
		     * the ground, and you being affected again by the same
		     * mine because it hasn't been deleted yet
		     */
		    static boolean recursive_mine = FALSE;

		    if (recursive_mine) break;
#endif
		    seetrap(trap);
		    pline(E_J("KAABLAMM!!!  You triggered %s land mine!",
			      "?h?O?I?H?H??!!! ?????????n???????????????I"),
					    a_your[trap->madeby_u]);
#ifdef STEED
		    if (u.usteed) steed_mid = u.usteed->m_id;
		    recursive_mine = TRUE;
		    (void) steedintrap(trap, (struct obj *)0);
		    recursive_mine = FALSE;
		    saddle = sobj_at(SADDLE,u.ux, u.uy);
#endif
		    set_wounded_legs(LEFT_SIDE, rn1(35, 41));
		    set_wounded_legs(RIGHT_SIDE, rn1(35, 41));
		    exercise(A_DEX, FALSE);
		}
		blow_up_landmine(trap);
#ifdef STEED
		if (steed_mid && saddle && !u.usteed)
			(void)keep_saddle_with_steedcorpse(steed_mid, fobj, saddle);
#endif
		newsym(u.ux,u.uy);		/* update trap symbol */
		losehp(rnd(16), E_J("land mine","?n????????????"), KILLED_BY_AN);
		/* fall recursively into the pit... */
		if ((trap = t_at(u.ux, u.uy)) != 0) dotrap(trap, RECURSIVETRAP);
		fill_pit(u.ux, u.uy);
		break;
	    }
	    case ROLLING_BOULDER_TRAP: {
		int style = ROLL | (trap->tseen ? LAUNCH_KNOWN : 0);

		seetrap(trap);
		pline(E_J("Click! You trigger a rolling boulder trap!",
			  "?J?`???I ?????????]?????????????????????????I"));
		if(!launch_obj(BOULDER, trap->launch.x, trap->launch.y,
		      trap->launch2.x, trap->launch2.y, style)) {
		    deltrap(trap);
		    newsym(u.ux,u.uy);	/* get rid of trap symbol */
		    pline(E_J("Fortunately for you, no boulder was released.",
			      "?K???A???????]?????????????????B"));
		}
		break;
	    }
	    case MAGIC_PORTAL:
		seetrap(trap);
		domagicportal(trap);
		break;

	    default:
		seetrap(trap);
		impossible("You hit a trap of type %u", trap->ttyp);
	}
}

#ifdef STEED
STATIC_OVL int
steedintrap(trap, otmp)
struct trap *trap;
struct obj *otmp;
{
	struct monst *mtmp = u.usteed;
	struct permonst *mptr;
	int tt;
	boolean in_sight;
	boolean trapkilled = FALSE;
	boolean steedhit = FALSE;

	if (!u.usteed || !trap) return 0;
	mptr = mtmp->data;
	tt = trap->ttyp;
	mtmp->mx = u.ux;
	mtmp->my = u.uy;

	in_sight = !Blind;
	switch (tt) {
		case ARROW_TRAP:
			if(!otmp) {
				impossible("steed hit by non-existant arrow?");
				return 0;
			}
			if (thitm(8, mtmp, otmp, 0, FALSE)) trapkilled = TRUE;
			steedhit = TRUE;
			break;
		case DART_TRAP:
			if(!otmp) {
				impossible("steed hit by non-existant dart?");
				return 0;
			}
			if (thitm(7, mtmp, otmp, 0, FALSE)) trapkilled = TRUE;
			steedhit = TRUE;
			break;
		case SLP_GAS_TRAP:
		    if (!resists_sleep(mtmp) && !breathless(mptr) &&
				!mtmp->msleeping && mtmp->mcanmove) {
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = rnd(25);
			    if (in_sight) {
				pline(E_J("%s suddenly falls asleep!",
					  "%s?????R???????????????I"),
				      Monnam(mtmp));
			    }
			}
			steedhit = TRUE;
			break;
		case LANDMINE:
			if (thitm(0, mtmp, (struct obj *)0, rnd(16), FALSE))
			    trapkilled = TRUE;
			steedhit = TRUE;
			break;
		case PIT:
		case SPIKED_PIT:
			if (mtmp->mhp <= 0 ||
				thitm(0, mtmp, (struct obj *)0,
				      rnd((tt == PIT) ? 6 : 10), FALSE))
			    trapkilled = TRUE;
			steedhit = TRUE;
			break;
		case POLY_TRAP: 
		    if (!resists_magm(mtmp)) {
			if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
			(void) newcham(mtmp, (struct permonst *)0,
				       FALSE, FALSE);
			if (!can_saddle(mtmp) || !can_ride(mtmp)) {
				dismount_steed(DISMOUNT_POLY);
			} else {
				You(E_J("have to adjust yourself in the saddle on %s.",
					"%s???????????????????p???????????B"),
					x_monnam(mtmp,
					 mtmp->has_name ? ARTICLE_NONE : ARTICLE_A,
				 	 (char *)0, SUPPRESS_SADDLE, FALSE));
			}
		    }
		    steedhit = TRUE;
		    break;
		default:
			return 0;
	    }
	}
	if(trapkilled) {
		dismount_steed(DISMOUNT_POLY);
		return 2;
	}
	else if(steedhit) return 1;
	else return 0;
}
#endif /*STEED*/

/* some actions common to both player and monsters for triggered landmine */
void
blow_up_landmine(trap)
struct trap *trap;
{
	(void)scatter(trap->tx, trap->ty, 4,
		MAY_DESTROY | MAY_HIT | MAY_FRACTURE | VIS_EFFECTS,
		(struct obj *)0);
	del_engr_at(trap->tx, trap->ty);
	wake_nearto(trap->tx, trap->ty, 400);
	if (IS_DOOR(levl[trap->tx][trap->ty].typ))
	    levl[trap->tx][trap->ty].doormask = D_BROKEN;
	/* TODO: destroy drawbridge if present */
	/* caller may subsequently fill pit, e.g. with a boulder */
	trap->ttyp = PIT;		/* explosion creates a pit */
	trap->madeby_u = FALSE;		/* resulting pit isn't yours */
	seetrap(trap);			/* and it isn't concealed */
}

#endif /* OVLB */
#ifdef OVL3

/*
 * Move obj from (x1,y1) to (x2,y2)
 *
 * Return 0 if no object was launched.
 *        1 if an object was launched and placed somewhere.
 *        2 if an object was launched, but used up.
 */
int
launch_obj(otyp, x1, y1, x2, y2, style)
short otyp;
register int x1,y1,x2,y2;
int style;
{
	register struct monst *mtmp;
	register struct obj *otmp, *otmp2;
	register int dx,dy;
	struct obj *singleobj;
	boolean used_up = FALSE;
	boolean otherside = FALSE;
	int dist;
	int tmp;
	int delaycnt = 0;

	otmp = sobj_at(otyp, x1, y1);
	/* Try the other side too, for rolling boulder traps */
	if (!otmp && otyp == BOULDER) {
		otherside = TRUE;
		otmp = sobj_at(otyp, x2, y2);
	}
	if (!otmp) return 0;
	if (otherside) {	/* swap 'em */
		int tx, ty;

		tx = x1; ty = y1;
		x1 = x2; y1 = y2;
		x2 = tx; y2 = ty;
	}

	if (otmp->quan == 1L) {
	    obj_extract_self(otmp);
	    singleobj = otmp;
	    otmp = (struct obj *) 0;
	} else {
	    singleobj = splitobj(otmp, 1L);
	    obj_extract_self(singleobj);
	}
	newsym(x1,y1);
	/* in case you're using a pick-axe to chop the boulder that's being
	   launched (perhaps a monster triggered it), destroy context so that
	   next dig attempt never thinks you're resuming previous effort */
	if ((otyp == BOULDER || otyp == STATUE) &&
	    singleobj->ox == digging.pos.x && singleobj->oy == digging.pos.y)
	    (void) memset((genericptr_t)&digging, 0, sizeof digging);

	dist = distmin(x1,y1,x2,y2);
	bhitpos.x = x1;
	bhitpos.y = y1;
	dx = sgn(x2 - x1);
	dy = sgn(y2 - y1);
	switch (style) {
	    case ROLL|LAUNCH_UNSEEN:
			if (otyp == BOULDER) {
			    You_hear(Hallucination ?
				     E_J("someone bowling.","?????????{?E?????O????????????") :
				     E_J("rumbling in the distance.","???????????n??????"));
			}
			style &= ~LAUNCH_UNSEEN;
			goto roll;
	    case ROLL|LAUNCH_KNOWN:
			/* use otrapped as a flag to ohitmon */
			singleobj->otrapped = 1;
			style &= ~LAUNCH_KNOWN;
			/* fall through */
	    roll:
	    case ROLL:
			delaycnt = 2;
			/* fall through */
	    default:
			if (!delaycnt) delaycnt = 1;
			if (!cansee(bhitpos.x,bhitpos.y)) curs_on_u();
			tmp_at(DISP_FLASH, obj_to_glyph(singleobj));
			tmp_at(bhitpos.x, bhitpos.y);
	}

	/* Set the object in motion */
	while(dist-- > 0 && !used_up) {
		struct trap *t;
		tmp_at(bhitpos.x, bhitpos.y);
		tmp = delaycnt;

		/* dstage@u.washington.edu -- Delay only if hero sees it */
		if (cansee(bhitpos.x, bhitpos.y))
			while (tmp-- > 0) delay_output();

		bhitpos.x += dx;
		bhitpos.y += dy;
		t = t_at(bhitpos.x, bhitpos.y);
		
		if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
			if (otyp == BOULDER && throws_rocks(mtmp->data)) {
			    if (rn2(3)) {
				pline(E_J("%s snatches the boulder.",
					  "%s?????????????????????~?????B"),
					Monnam(mtmp));
				singleobj->otrapped = 0;
				(void) mpickobj(mtmp, singleobj);
				used_up = TRUE;
				break;
			    }
			}
			if (ohitmon(mtmp,singleobj,
					(style==ROLL) ? -1 : dist, FALSE)) {
				used_up = TRUE;
				break;
			}
		} else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
			int hit;
			if (multi) nomul(0);
#ifdef JP
			bullet_killername((struct monst *)0, singleobj, 0);
#endif /*JP*/
			if (hit = thitu(9 + singleobj->spe,
				  dmgval(singleobj, &youmonst),
				  singleobj, (char *)0))
			    stop_occupation();
			if (hit && otyp == BOULDER && uwep &&
			    uwep->oartifact == ART_GIANTKILLER) {
				place_object(singleobj, bhitpos.x, bhitpos.y);
				fracture_rock(singleobj);
				if (cansee(bhitpos.x,bhitpos.y))
					newsym(bhitpos.x,bhitpos.y);
			        used_up = TRUE;
			}
		}
		if (style == ROLL) {
		    if (down_gate(bhitpos.x, bhitpos.y) != -1) {
		        if(ship_object(singleobj, bhitpos.x, bhitpos.y, FALSE)){
				used_up = TRUE;
				break;
			}
		    }
		    if (t && otyp == BOULDER) {
			switch(t->ttyp) {
			case LANDMINE:
			    if (rn2(10) > 2) {
			  	pline(
				  E_J("KAABLAMM!!!%s","?h?O?I?H?H??!!!%s"),
				  cansee(bhitpos.x, bhitpos.y) ?
					E_J(" The rolling boulder triggers a land mine.",
					    " ?]???????????n?????N???????B") : "");
				deltrap(t);
				del_engr_at(bhitpos.x,bhitpos.y);
				place_object(singleobj, bhitpos.x, bhitpos.y);
				singleobj->otrapped = 0;
				fracture_rock(singleobj);
				(void)scatter(bhitpos.x,bhitpos.y, 4,
					MAY_DESTROY|MAY_HIT|MAY_FRACTURE|VIS_EFFECTS,
					(struct obj *)0);
				if (cansee(bhitpos.x,bhitpos.y))
					newsym(bhitpos.x,bhitpos.y);
			        used_up = TRUE;
			    }
			    break;		
			case LEVEL_TELEP:
			case TELEP_TRAP:
			    if (cansee(bhitpos.x, bhitpos.y))
			    	pline(E_J("Suddenly the rolling boulder disappears!",
					  "???R?A?]?????????????????????I"));
			    else
			    	You_hear(E_J("a rumbling stop abruptly.",
					     "?n?????????????~??????????"));
			    singleobj->otrapped = 0;
			    if (t->ttyp == TELEP_TRAP)
				rloco(singleobj);
			    else {
				int newlev = random_teleport_level();
				d_level dest;

				if (newlev == depth(&u.uz) || In_endgame(&u.uz))
				    continue;
				add_to_migration(singleobj);
				get_level(&dest, newlev);
				singleobj->ox = dest.dnum;
				singleobj->oy = dest.dlevel;
				singleobj->owornmask = (long)MIGR_RANDOM;
			    }
		    	    seetrap(t);
			    used_up = TRUE;
			    break;
			case PIT:
			case SPIKED_PIT:
			case HOLE:
			case TRAPDOOR:
			    /* the boulder won't be used up if there is a
			       monster in the trap; stop rolling anyway */
			    x2 = bhitpos.x,  y2 = bhitpos.y;  /* stops here */
			    if (flooreffects(singleobj, x2, y2, E_J("fall","??????")))
				used_up = TRUE;
			    dist = -1;	/* stop rolling immediately */
			    break;
			}
			if (used_up || dist == -1) break;
		    }
		    if (flooreffects(singleobj, bhitpos.x, bhitpos.y, E_J("fall","??????"))) {
			used_up = TRUE;
			break;
		    }
		    if (otyp == BOULDER &&
		       (otmp2 = sobj_at(BOULDER, bhitpos.x, bhitpos.y)) != 0) {
			const char *bmsg =
				     E_J(" as one boulder sets another in motion",
					 "????????????????????????????" );

#ifndef JP
			if (!isok(bhitpos.x + dx, bhitpos.y + dy) || !dist ||
			    IS_ROCK(levl[bhitpos.x + dx][bhitpos.y + dy].typ))
			    bmsg = " as one boulder hits another";
#endif /*JP*/

#ifndef JP
			You_hear("a loud crash%s!",
				cansee(bhitpos.x, bhitpos.y) ? bmsg : "");
#else
			if (cansee(bhitpos.x, bhitpos.y))
			    pline("%s?A?????????????????I", bmsg);
			else
			    You_hear("!????????????????????????");
#endif /*JP*/
			obj_extract_self(otmp2);
			/* pass off the otrapped flag to the next boulder */
			otmp2->otrapped = singleobj->otrapped;
			singleobj->otrapped = 0;
			place_object(singleobj, bhitpos.x, bhitpos.y);
			singleobj = otmp2;
			otmp2 = (struct obj *)0;
			wake_nearto(bhitpos.x, bhitpos.y, 10*10);
		    }
		}
		if (otyp == BOULDER && closed_door(bhitpos.x,bhitpos.y)) {
			if (cansee(bhitpos.x, bhitpos.y))
				pline_The(E_J("boulder crashes through a door.",
					      "???????????j?????????????????I"));
			levl[bhitpos.x][bhitpos.y].doormask = D_BROKEN;
			if (dist) unblock_point(bhitpos.x, bhitpos.y);
		}

		/* if about to hit iron bars, do so now */
		if (dist > 0 && isok(bhitpos.x + dx,bhitpos.y + dy) &&
			levl[bhitpos.x + dx][bhitpos.y + dy].typ == IRONBARS) {
		    x2 = bhitpos.x,  y2 = bhitpos.y;	/* object stops here */
		    if (hits_bars(&singleobj, x2, y2, !rn2(20), 0)) {
			if (!singleobj) used_up = TRUE;
			break;
		    }
		}
	}
	tmp_at(DISP_END, 0);
	if (!used_up) {
		singleobj->otrapped = 0;
		place_object(singleobj, x2,y2);
		newsym(x2,y2);
		return 1;
	} else
		return 2;
}

#endif /* OVL3 */
#ifdef OVLB

void
seetrap(trap)
	register struct trap *trap;
{
	if(!trap->tseen) {
	    trap->tseen = 1;
	    newsym(trap->tx, trap->ty);
	}
}

#endif /* OVLB */
#ifdef OVL3

STATIC_OVL int
mkroll_launch(ttmp, x, y, otyp, ocount)
struct trap *ttmp;
xchar x,y;
short otyp;
long ocount;
{
	struct obj *otmp;
	register int tmp;
	schar dx,dy;
	int distance;
	coord cc;
	coord bcc;
	int trycount = 0;
	boolean success = FALSE;
	int mindist = 4;

	if (ttmp->ttyp == ROLLING_BOULDER_TRAP) mindist = 2;
	distance = rn1(5,4);    /* 4..8 away */
	tmp = rn2(8);		/* randomly pick a direction to try first */
	while (distance >= mindist) {
		dx = xdir[tmp];
		dy = ydir[tmp];
		cc.x = x; cc.y = y;
		/* Prevent boulder from being placed on water */
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP
				&& is_pool(x+distance*dx,y+distance*dy))
			success = FALSE;
		else success = isclearpath(&cc, distance, dx, dy);
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
			boolean success_otherway;
			bcc.x = x; bcc.y = y;
			success_otherway = isclearpath(&bcc, distance,
						-(dx), -(dy));
			if (!success_otherway) success = FALSE;
		}
		if (success) break;
		if (++tmp > 7) tmp = 0;
		if ((++trycount % 8) == 0) --distance;
	}
	if (!success) {
	    /* create the trap without any ammo, launch pt at trap location */
		cc.x = bcc.x = x;
		cc.y = bcc.y = y;
	} else {
		otmp = mksobj(otyp, TRUE, FALSE);
		otmp->quan = ocount;
		otmp->owt = weight(otmp);
		place_object(otmp, cc.x, cc.y);
		stackobj(otmp);
	}
	ttmp->launch.x = cc.x;
	ttmp->launch.y = cc.y;
	if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
		ttmp->launch2.x = bcc.x;
		ttmp->launch2.y = bcc.y;
	} else
		ttmp->launch_otyp = otyp;
	newsym(ttmp->launch.x, ttmp->launch.y);
	return 1;
}

STATIC_OVL boolean
isclearpath(cc,distance,dx,dy)
coord *cc;
int distance;
schar dx,dy;
{
	uchar typ;
	xchar x, y;

	x = cc->x;
	y = cc->y;
	while (distance-- > 0) {
		x += dx;
		y += dy;
		typ = levl[x][y].typ;
		if (!isok(x,y) || !ZAP_POS(typ) || closed_door(x,y))
			return FALSE;
	}
	cc->x = x;
	cc->y = y;
	return TRUE;
}
#endif /* OVL3 */
#ifdef OVL1

int
mintrap(mtmp)
register struct monst *mtmp;
{
	register struct trap *trap = t_at(mtmp->mx, mtmp->my);
	boolean trapkilled = FALSE;
	struct permonst *mptr = mtmp->data;
	struct obj *otmp;

	if (!trap) {
	    mtmp->mtrapped = 0;	/* perhaps teleported? */
	} else if (mtmp->mtrapped) {	/* is currently in the trap */
	    if (!trap->tseen &&
		cansee(mtmp->mx, mtmp->my) && canseemon(mtmp) &&
		(trap->ttyp == SPIKED_PIT || trap->ttyp == BEAR_TRAP ||
		 trap->ttyp == HOLE || trap->ttyp == PIT ||
		 trap->ttyp == WEB)) {
		/* If you come upon an obviously trapped monster, then
		 * you must be able to see the trap it's in too.
		 */
		seetrap(trap);
	    }
		
	    if (!rn2(40)) {
		if (sobj_at(BOULDER, mtmp->mx, mtmp->my) &&
			(trap->ttyp == PIT || trap->ttyp == SPIKED_PIT)) {
		    if (!rn2(2)) {
			mtmp->mtrapped = 0;
			if (canseemon(mtmp))
			    pline(E_J("%s pulls free...", "%s???????o?????c?B"),
				  Monnam(mtmp));
			fill_pit(mtmp->mx, mtmp->my);
		    }
		} else {
		    mtmp->mtrapped = 0;
		}
	    } else if (metallivorous(mptr)) {
		if (trap->ttyp == BEAR_TRAP) {
		    if (canseemon(mtmp))
			pline(E_J("%s eats a bear trap!","%s???g???o?T?~???H?????????I"), Monnam(mtmp));
		    deltrap(trap);
		    mtmp->meating = 5;
		    mtmp->mtrapped = 0;
		} else if (trap->ttyp == SPIKED_PIT) {
		    if (canseemon(mtmp))
			pline(E_J("%s munches on some spikes!",
				  "%s???????????????????????I"), Monnam(mtmp));
		    trap->ttyp = PIT;
		    mtmp->meating = 5;
		}
	    }
	} else {
	    register int tt = trap->ttyp;
	    boolean in_sight, tear_web, see_it,
		    inescapable = ((tt == HOLE || tt == PIT) &&
				   In_sokoban(&u.uz) && !trap->madeby_u);
	    const char *fallverb;

#ifdef STEED
	    /* true when called from dotrap, inescapable is not an option */
	    if (mtmp == u.usteed) inescapable = TRUE;
#endif
	    if (!inescapable &&
		    ((mtmp->mtrapseen & (1 << (tt-1))) != 0 ||
			(tt == HOLE && !mindless(mtmp->data)))) {
		/* it has been in such a trap - perhaps it escapes */
		if(rn2(4)) return(0);
	    } else {
		mtmp->mtrapseen |= (1 << (tt-1));
	    }
	    /* Monster is aggravated by being trapped by you.
	       Recognizing who made the trap isn't completely
	       unreasonable; everybody has their own style. */
	    if (trap->madeby_u && rnl(5)) setmangry(mtmp);

	    in_sight = canseemon(mtmp);
	    see_it = cansee(mtmp->mx, mtmp->my);
#ifdef STEED
	    /* assume hero can tell what's going on for the steed */
	    if (mtmp == u.usteed) in_sight = TRUE;
#endif
	    switch (tt) {
		case ARROW_TRAP:
			if (trap->once && trap->tseen && !rn2(15)) {
			    if (in_sight && see_it)
				pline(E_J("%s triggers a trap but nothing happens.",
					  "%s?????????????????????????A?????N???????????B"),
				      Monnam(mtmp));
			    deltrap(trap);
			    newsym(mtmp->mx, mtmp->my);
			    break;
			}
			trap->once = 1;
			otmp = mksobj(ARROW, TRUE, FALSE);
			otmp->quan = 1L;
			otmp->owt = weight(otmp);
			otmp->opoisoned = 0;
			if (in_sight) seetrap(trap);
#ifdef MONSTEED
			if (is_mriding(mtmp) && rn2(3)) {
			    thitm(8, mtmp->mlink, otmp, 0, FALSE);
			    trapkilled = DEADMONSTER(mtmp);
			} else
#endif
			if (thitm(8, mtmp, otmp, 0, FALSE)) trapkilled = TRUE;
			break;
		case DART_TRAP:
			if (trap->once && trap->tseen && !rn2(15)) {
			    if (in_sight && see_it)
				pline(E_J("%s triggers a trap but nothing happens.",
					  "%s?????????????????????????A?????N???????????B"),
				      Monnam(mtmp));
			    deltrap(trap);
			    newsym(mtmp->mx, mtmp->my);
			    break;
			}
			trap->once = 1;
			otmp = mksobj(DART, TRUE, FALSE);
			otmp->quan = 1L;
			otmp->owt = weight(otmp);
			if (!rn2(6)) otmp->opoisoned = 1;
			if (in_sight) seetrap(trap);
#ifdef MONSTEED
			if (is_mriding(mtmp) && rn2(3)) {
			    thitm(7, mtmp->mlink, otmp, 0, FALSE);
			    trapkilled = DEADMONSTER(mtmp);
			} else
#endif
			if (thitm(7, mtmp, otmp, 0, FALSE)) trapkilled = TRUE;
			break;
		case ROCKTRAP:
			if (trap->once && trap->tseen && !rn2(15)) {
			    if (in_sight && see_it)
				pline(E_J("A trap door above %s opens, but nothing falls out!",
					  "%s?????????????????J???????A?????????????????????I"),
				      mon_nam(mtmp));
			    deltrap(trap);
			    newsym(mtmp->mx, mtmp->my);
			    break;
			}
			trap->once = 1;
			otmp = mksobj(ROCK, TRUE, FALSE);
			otmp->quan = 1L;
			otmp->owt = weight(otmp);
			if (in_sight) seetrap(trap);
			if (thitm(0, mtmp, otmp, d(2, 6), FALSE))
			    trapkilled = TRUE;
			break;

		case SQKY_BOARD:
			if(is_flying(mtmp)) break;
			/* stepped on a squeaky board */
			if (in_sight) {
			    pline(E_J("A board beneath %s squeaks loudly.",
				      "%s???????????????????????a?????B"), mon_nam(mtmp));
			    seetrap(trap);
			} else
			   You_hear(E_J("a distant squeak.","???????a??????"));
			/* wake up nearby monsters */
			wake_nearto(mtmp->mx, mtmp->my, 40);
			break;

		case BEAR_TRAP:
			if (is_flying(mtmp)) break;
#ifdef MONSTEED
			/* always msteed is caught */
			if (is_mriding(mtmp)) {
			    mtmp = mtmp->mlink;
			    mptr = mtmp->data;
			    in_sight = in_sight || canseemon(mtmp);
			}
#endif
			if(mptr->msize > MZ_SMALL && !amorphous(mptr) &&
				!is_whirly(mptr) && !unsolid(mptr)) {
			    mtmp->mtrapped = 1;
			    if(in_sight) {
				pline(E_J("%s is caught in %s bear trap!",
					  "%s??%s?g???o?T?~???????????I"),
				      Monnam(mtmp), a_your[trap->madeby_u]);
				seetrap(trap);
			    } else {
#ifndef JP
				if((mtmp->mnum == PM_OWLBEAR ||
				    mtmp->mnum == PM_BUGBEAR)
				   && flags.soundok)
				    You_hear("the roaring of an angry bear!");
#else
				if(mtmp->mnum == PM_TIGER && flags.soundok)
				    You_hear("!???????????????K??");
#endif /*JP*/
			    }
			}
			break;

		case SLP_GAS_TRAP:
#ifdef MONSTEED
		    if (is_mriding(mtmp)) mintrap(mtmp->mlink);
#endif
		    if (!resists_sleep(mtmp) && !breathless(mptr) &&
				!mtmp->msleeping && mtmp->mcanmove) {
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = rnd(25);
			    if (in_sight) {
				pline(E_J("%s suddenly falls asleep!",
					  "%s?????R???????????????I"),
				      Monnam(mtmp));
				seetrap(trap);
			    }
			}
			break;

		case RUST_TRAP:
		    {
			struct obj *target;
			boolean out_of_water = (trap->vl.v_lasttrig > monstermoves);

#ifdef MONSTEED
			if (is_mriding(mtmp) && !rn2(3)) {
			    mintrap(mtmp->mlink);
			    break;
			}
#endif
			if (in_sight) {
			    seetrap(trap);
			    if (out_of_water)
				pline(E_J("Some dump air hits %s.",
					  "?????????C??%s?????????????????????????B"),
					mon_nam(mtmp));
			}
			if (out_of_water) break;

			trap->vl.v_lasttrig = monstermoves + rn1(100,100);
			if (is_animobj(mtmp->data)) {
			    if (in_sight)
				pline(E_J("%s %s!","%s%s???????????I"), A_gush_of_water_hits,
				    mon_nam(mtmp));
			    erode_obj(MON_WEP(mtmp), FALSE, TRUE);
			    if (!MON_WEP(mtmp)) {
				mondied(mtmp);
				trapkilled = TRUE;
			    }
			    break;
			}
			switch (rn2(5)) {
			case 0:
			    if (in_sight)
				pline(E_J("%s %s on the %s!","%s%s??%s???????????I"),
				    A_gush_of_water_hits,
				    mon_nam(mtmp), mbodypart(mtmp, HEAD));
			    target = which_armor(mtmp, W_ARMH);
			    (void) rust_dmg(target, helm_simple_name(target), 1, TRUE, mtmp);
			    break;
			case 1:
			    if (in_sight)
				pline(E_J("%s %s's left %s!","%s%s????%s???????????I"),
				    A_gush_of_water_hits,
				    mon_nam(mtmp), mbodypart(mtmp, ARM));
			    target = which_armor(mtmp, W_ARMS);
			    if (rust_dmg(target, E_J("shield","??"), 1, TRUE, mtmp))
				break;
			    target = MON_WEP(mtmp);
			    if (target && bimanual(target))
				erode_obj(target, FALSE, TRUE);
glovecheck:		    target = which_armor(mtmp, W_ARMG);
			    (void) rust_dmg(target, E_J("gauntlets","?U??"), 1, TRUE, mtmp);
			    break;
			case 2:
			    if (in_sight)
				pline(E_J("%s %s's right %s!","%s%s???E%s???????????I"),
				    A_gush_of_water_hits,
				    mon_nam(mtmp), mbodypart(mtmp, ARM));
			    erode_obj(MON_WEP(mtmp), FALSE, TRUE);
			    goto glovecheck;
			default:
			    if (in_sight)
				pline(E_J("%s %s!","%s%s???????????I"), A_gush_of_water_hits,
				    mon_nam(mtmp));
			    for (otmp=mtmp->minvent; otmp; otmp = otmp->nobj)
				(void) snuff_lit(otmp);
			    target = which_armor(mtmp, W_ARMC);
			    if (target)
				(void) rust_dmg(target, cloak_simple_name(target),
						 1, TRUE, mtmp);
			    else {
				target = which_armor(mtmp, W_ARM);
				if (target)
				    (void) rust_dmg(target, E_J("armor","?Z"), 1, TRUE, mtmp);
				else {
				    target = which_armor(mtmp, W_ARMU);
				    (void) rust_dmg(target, E_J("shirt","?V???c"), 1, TRUE, mtmp);
				}
			    }
			}
			if (mtmp->mnum == PM_IRON_GOLEM) {
				if (in_sight)
				    pline(E_J("%s falls to pieces!",
					    "%s?????????????????????????????I"), Monnam(mtmp));
				else if(mtmp->mtame)
				    pline(E_J("May %s rust in peace.",
					    "%s???A?????????K???????????B"), mon_nam(mtmp));
				mondied(mtmp);
				if (mtmp->mhp <= 0)
					trapkilled = TRUE;
			} else if (mtmp->mnum == PM_GREMLIN && rn2(3)) {
				(void)split_mon(mtmp, (struct monst *)0);
			}
			break;
		    }
		case FIRE_TRAP:
 mfiretrap:
#ifdef MONSTEED
			if (is_mriding(mtmp)) { /* steed first */
			    mtmp = mtmp->mlink;
			    in_sight = canseemon(mtmp);
			}
#endif
			if (in_sight)
#ifndef JP
			    pline("A %s erupts from the %s under %s!",
				  tower_of_flame,
				  surface(mtmp->mx,mtmp->my),
				  mon_nam(mtmp));
#else
			    pline("%s??????%s?????R???????????????????????I",
				  mon_nam(mtmp), surface(mtmp->mx,mtmp->my));
#endif /*JP*/
			else if (see_it)  /* evidently `mtmp' is invisible */
#ifndef JP
			    You("see a %s erupt from the %s!",
				tower_of_flame, surface(mtmp->mx,mtmp->my));
#else
			    You("%s???????????????????????????I",
				surface(mtmp->mx,mtmp->my));
#endif /*JP*/
 mfiretrap_loop:
			if (resists_fire(mtmp)) {
			    if (in_sight) {
				shieldeff(mtmp->mx,mtmp->my);
				pline(E_J("%s is uninjured.","%s?????????????????B"), Monnam(mtmp));
			    }
			} else {
			    int num = d(2,4), alt;
			    boolean immolate = FALSE;

			    /* paper burns very fast, assume straw is tightly
			     * packed and burns a bit slower */
			    switch (monsndx(mtmp->data)) {
			    case PM_PAPER_GOLEM:   immolate = TRUE;
						   alt = mtmp->mhpmax; break;
			    case PM_STRAW_GOLEM:   alt = mtmp->mhpmax / 2; break;
			    case PM_WOOD_GOLEM:    alt = mtmp->mhpmax / 4; break;
			    case PM_LEATHER_GOLEM: alt = mtmp->mhpmax / 8; break;
			    default: alt = 0; break;
			    }
			    if (alt > num) num = alt;

			    if (thitm(0, mtmp, (struct obj *)0, num, immolate))
				trapkilled = TRUE;
			    else
				/* we know mhp is at least `num' below mhpmax,
				   so no (mhp > mhpmax) check is needed here */
				mtmp->mhpmax -= rn2(num + 1);
			}
			if (burnarmor(mtmp) || rn2(3)) {
			    (void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			    (void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
			    (void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			}
#ifdef MONSTEED
			/* if msteed died, mintrap() is called from mondead() */
			if (!trapkilled && is_mridden(mtmp)) {
			    mtmp = mtmp->mlink;
			    mptr = mtmp->data;
			    in_sight = canseemon(mtmp);
			    goto mfiretrap_loop;
			} else trapkilled = DEADMONSTER(mtmp);
#endif
			if (burn_floor_paper(mtmp->mx, mtmp->my, see_it, FALSE) &&
				!see_it && distu(mtmp->mx, mtmp->my) <= 3*3)
			    You(E_J("smell smoke.","?????L?????k?????B"));
			if (is_ice(mtmp->mx,mtmp->my))
			    melt_ice(mtmp->mx,mtmp->my);
			if (see_it) seetrap(trap);
			break;

		case PIT:
		case SPIKED_PIT:
			fallverb = E_J("falls","??????");
			if (is_flying(mtmp) || is_floating(mtmp) ||
				(mtmp->wormno && count_wsegs(mtmp) > 5) ||
				is_clinging(mtmp)) {
			    if (!inescapable) break;	/* avoids trap */
			    fallverb = E_J("is dragged","????????????????");	/* sokoban pit */
			}
#ifdef MONSTEED
			/* always msteed falls */
			if (is_mriding(mtmp)) {
			    mtmp = mtmp->mlink;
			    mptr = mtmp->data;
			    in_sight = in_sight && canseemon(mtmp);
			}
#endif
			if (!passes_walls(mptr))
			    mtmp->mtrapped = 1;
			if (in_sight) {
#ifndef JP
			    pline("%s %s into %s pit!",
				  Monnam(mtmp), fallverb,
				  a_your[trap->madeby_u]);
#else
			    pline("%s??%s??????????%s?I",
				  Monnam(mtmp),
				  trap->madeby_u ? "?????????@????" : "",
				  fallverb);
#endif /*JP*/
#ifndef JP
			    if (mtmp->mnum == PM_PIT_VIPER || mtmp->mnum == PM_PIT_FIEND)
				pline("How pitiful.  Isn't that the pits?");
#endif /*JP*/
			    seetrap(trap);
			}
			mselftouch(mtmp, E_J("Falling, ","???????A"), FALSE);
			if (mtmp->mhp <= 0 ||
				thitm(0, mtmp, (struct obj *)0,
				      rnd((tt == PIT) ? 6 : 10), FALSE))
			    trapkilled = TRUE;
			break;
		case HOLE:
		case TRAPDOOR:
			if (!Can_fall_thru(&u.uz)) {
			 impossible("mintrap: %ss cannot exist on this level.",
				    defsyms[trap_to_defsym(tt)].explanation);
			    break;	/* don't activate it after all */
			}
			if (is_flying(mtmp) || is_floating(mtmp) ||
				mtmp->mnum == PM_WUMPUS ||
				(mtmp->wormno && count_wsegs(mtmp) > 5) ||
				mptr->msize >= MZ_HUGE) {
			    if (inescapable) {	/* sokoban hole */
				if (in_sight) {
				    pline(E_J("%s seems to be yanked down!",
					      "%s?????????????????????????I"),
					  Monnam(mtmp));
				    /* suppress message in mlevel_tele_trap() */
				    in_sight = FALSE;
				    seetrap(trap);
				}
			    } else
				break;
			}
			/* Fall through */
		case LEVEL_TELEP:
		case MAGIC_PORTAL:
			{
			    int mlev_res;
			    mlev_res = mlevel_tele_trap(mtmp, trap,
							inescapable, in_sight);
			    if (mlev_res) return(mlev_res);
			}
			break;

		case TELEP_TRAP:
			mtele_trap(mtmp, trap, in_sight);
			break;

		case WEB:
			/* Monster in a web. */
#ifdef MONSTEED
			/* assume always msteed is caught */
			if (is_mriding(mtmp)) {
			    mtmp = mtmp->mlink;
			    mptr = mtmp->data;
			}
#endif
			if (webmaker(mptr)) break;
			if (amorphous(mptr) || is_whirly(mptr) || unsolid(mptr)){
			    if(acidic(mptr) ||
			       mtmp->mnum == PM_GELATINOUS_CUBE ||
			       mtmp->mnum == PM_FIRE_ELEMENTAL) {
				if (in_sight)
#ifndef JP
				    pline("%s %s %s spider web!",
					  Monnam(mtmp),
					  (mtmp->mnum == PM_FIRE_ELEMENTAL) ?
					    "burns" : "dissolves",
					  a_your[trap->madeby_u]);
#else
				    pline("%s??%s?w????????%s?I",
					  Monnam(mtmp),
					  trap->madeby_u ? "??????????????" : "",
					  (mtmp->mnum == PM_FIRE_ELEMENTAL) ?
					    "??????????" : "?n??????");
#endif /*JP*/
				deltrap(trap);
				newsym(mtmp->mx, mtmp->my);
				break;
			    }
			    if (in_sight) {
#ifndef JP
				pline("%s flows through %s spider web.",
				      Monnam(mtmp),
				      a_your[trap->madeby_u]);
#else
				pline("%s??%s?w???????????????????B",
				      Monnam(mtmp),
				      trap->madeby_u ? "??????????????" : "");
#endif /*JP*/
				seetrap(trap);
			    }
			    break;
			}
			tear_web = FALSE;
			switch (monsndx(mptr)) {
			    case PM_OWLBEAR: /* Eric Backus */
			    case PM_BUGBEAR:
				if (!in_sight) {
				    You_hear(E_J("the roaring of a confused bear!",
						 "!?????????F?????K??"));
				    mtmp->mtrapped = 1;
				    break;
				}
				/* fall though */
			    default:
				if (mptr->mlet == S_GIANT ||
				    (mptr->mlet == S_DRAGON &&
					extra_nasty(mptr)) || /* excl. babies */
				    (mtmp->wormno && count_wsegs(mtmp) > 5)) {
				    tear_web = TRUE;
				} else if (in_sight) {
#ifndef JP
				    pline("%s is caught in %s spider web.",
					  Monnam(mtmp),
					  a_your[trap->madeby_u]);
#else /*JP*/
				    pline("%s??%s?w?????????????????????B",
					  Monnam(mtmp),
					  trap->madeby_u ? "??????????????" : "");
#endif /*JP*/
				    seetrap(trap);
				}
				mtmp->mtrapped = tear_web ? 0 : 1;
				break;
			    /* this list is fairly arbitrary; it deliberately
			       excludes wumpus & giant/ettin zombies/mummies */
			    case PM_TITANOTHERE:
			    case PM_BALUCHITHERIUM:
			    case PM_PURPLE_WORM:
			    case PM_JABBERWOCK:
			    case PM_IRON_GOLEM:
			    case PM_BALROG:
			    case PM_KRAKEN:
			    case PM_MASTODON:
				tear_web = TRUE;
				break;
			}
			if (tear_web) {
			    if (in_sight)
#ifndef JP
				pline("%s tears through %s spider web!",
				      Monnam(mtmp), a_your[trap->madeby_u]);
#else
				pline("%s??%s?w?????????j?????????I",
				      Monnam(mtmp), trap->madeby_u ? "??????????????" : "");
#endif /*JP*/
			    deltrap(trap);
			    newsym(mtmp->mx, mtmp->my);
			}
			break;

		case STATUE_TRAP:
			break;

		case MAGIC_TRAP:
			/* A magic trap.  Monsters usually immune. */
			if (!rn2(21)) goto mfiretrap;
			break;
		case ANTI_MAGIC:
			break;

		case LANDMINE:
			if(rn2(3))
				break; /* monsters usually don't set it off */
			if(is_flying(mtmp)) {
				boolean already_seen = trap->tseen;
				if (in_sight && !already_seen) {
	pline(E_J("A trigger appears in a pile of soil below %s.",
		  "%s???????n?????N?????u?????????B"), mon_nam(mtmp));
					seetrap(trap);
				}
				if (rn2(3)) break;
				if (in_sight) {
					newsym(mtmp->mx, mtmp->my);
#ifndef JP
					pline_The("air currents set %s off!",
					  already_seen ? "a land mine" : "it");
#else
					pline("???C????????????????%s???????????I",
					  already_seen ? "?n??" : "????");
#endif /*JP*/
				}
			} else if(in_sight) {
			    newsym(mtmp->mx, mtmp->my);
#ifndef JP
			    pline("KAABLAMM!!!  %s triggers %s land mine!",
				Monnam(mtmp), a_your[trap->madeby_u]);
#else
			    pline("?h?O?I?H?H??!!!  %s??%s?n?????N???????I",
				Monnam(mtmp), trap->madeby_u ? "?????????d?|????" : "");
#endif /*JP*/
			}
			if (!in_sight)
				pline(E_J("Kaablamm!  You hear an explosion in the distance!",
					  "?h?S?H?H???I ?????????????????????????????I"));
			blow_up_landmine(trap);
#ifdef MONSTEED
			if (is_mriding(mtmp)) {
			    if (!thitm(0, mtmp->mlink, (struct obj *)0, rnd(16), FALSE))
				mintrap(mtmp->mlink);
			    trapkilled = DEADMONSTER(mtmp);
			    if (!trapkilled && thitm(0, mtmp, (struct obj *)0, rnd(16), FALSE))
				trapkilled = TRUE;
			} else
#endif
			if (thitm(0, mtmp, (struct obj *)0, rnd(16), FALSE))
				trapkilled = TRUE;
			else {
				/* monsters recursively fall into new pit */
				if (mintrap(mtmp) == 2) trapkilled=TRUE;
			}
			/* a boulder may fill the new pit, crushing monster */
			fill_pit(trap->tx, trap->ty);
			if (mtmp->mhp <= 0) trapkilled = TRUE;
			if (unconscious()) {
				multi = -1;
				nomovemsg=E_J("The explosion awakens you!",
					      "???????????????????o?????????I");
			}
			break;

		case POLY_TRAP:
#ifdef MONSTEED
		    /* monsteed gets polyed too */
		    if (is_mriding(mtmp)) mintrap(mtmp->mlink);
		    if (DEADMONSTER(mtmp)) {
			trapkilled = TRUE;
			break;
		    }
#endif
		    if (resists_magm(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
		    } else if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
			(void) newcham(mtmp, (struct permonst *)0,
				       FALSE, FALSE);
			if (in_sight) seetrap(trap);
		    }
		    break;

		case ROLLING_BOULDER_TRAP:
		    if (!is_flying(mtmp)) {
			int style = ROLL | (in_sight ? 0 : LAUNCH_UNSEEN);

		        newsym(mtmp->mx,mtmp->my);
			if (in_sight)
			    pline(E_J("Click! %s triggers %s.","?J?`???I %s??%s?????????????B"),
				  Monnam(mtmp),
				  trap->tseen ?
				  E_J("a rolling boulder trap","?]????????????") :
				  something);
			if (launch_obj(BOULDER, trap->launch.x, trap->launch.y,
				trap->launch2.x, trap->launch2.y, style)) {
			    if (in_sight) trap->tseen = TRUE;
			    if (mtmp->mhp <= 0) trapkilled = TRUE;
			} else {
			    deltrap(trap);
			    newsym(mtmp->mx,mtmp->my);
			}
		    }
		    break;

		default:
			impossible("Some monster encountered a strange trap of type %d.", tt);
	    }
	}
	if(trapkilled) return 2;
	return mtmp->mtrapped;
}

#endif /* OVL1 */
#ifdef OVLB

/* Combine cockatrice checks into single functions to avoid repeating code. */
void
instapetrify(str)
const char *str;
{
	if (Stone_resistance) return;
	if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
	    return;
	You(E_J("turn to stone...","???????????c?B"));
	killer_format = KILLED_BY;
	killer = str;
	done(STONING);
}

void
minstapetrify(mon,byplayer)
struct monst *mon;
boolean byplayer;
{
	if (resists_ston(mon)) return;
	if (poly_when_stoned(mon->data)) {
		mon_to_stone(mon);
		return;
	}

	/* give a "<mon> is slowing down" message and also remove
	   intrinsic speed (comparable to similar effect on the hero) */
	mon_adjust_speed(mon, -3, (struct obj *)0);

	if (cansee(mon->mx, mon->my))
		pline(E_J("%s turns to stone.","%s?????????????B"), Monnam(mon));
	if (byplayer) {
		stoned = TRUE;
		xkilled(mon,0);
	} else monstone(mon);
}

void
selftouch(arg)
const char *arg;
{
	char kbuf[BUFSZ];

	if(uwep && uwep->otyp == CORPSE && touch_petrifies(&mons[uwep->corpsenm])
			&& !Stone_resistance) {
#ifndef JP
		pline("%s touch the %s corpse.", arg,
		        mons[uwep->corpsenm].mname);
		Sprintf(kbuf, "%s corpse", an(mons[uwep->corpsenm].mname));
#else
		pline("%s%s?????????G?????????????B", arg,
		        JMONNAM(uwep->corpsenm));
		Sprintf(kbuf, "%s?????????G????", JMONNAM(uwep->corpsenm));
#endif /*JP*/
		instapetrify(kbuf);
	}
	/* Or your secondary weapon, if wielded */
	if(u.twoweap && uswapwep && uswapwep->otyp == CORPSE &&
			touch_petrifies(&mons[uswapwep->corpsenm]) && !Stone_resistance){
#ifndef JP
		pline("%s touch the %s corpse.", arg,
		        mons[uswapwep->corpsenm].mname);
		Sprintf(kbuf, "%s corpse", an(mons[uswapwep->corpsenm].mname));
#else
		pline("%s%s?????????G?????????????B", arg,
		        JMONNAM(uswapwep->corpsenm));
		Sprintf(kbuf, "%s?????????G????", JMONNAM(uswapwep->corpsenm));
#endif /*JP*/
		instapetrify(kbuf);
	}
}

void
mselftouch(mon,arg,byplayer)
struct monst *mon;
const char *arg;
boolean byplayer;
{
	struct obj *mwep = MON_WEP(mon);

	if (mwep && mwep->otyp == CORPSE && touch_petrifies(&mons[mwep->corpsenm])) {
		if (cansee(mon->mx, mon->my)) {
#ifndef JP
			pline("%s%s touches the %s corpse.",
			    arg ? arg : "", arg ? mon_nam(mon) : Monnam(mon),
			    mons[mwep->corpsenm].mname);
#else
			pline("%s%s??%s?????????G?????????????B",
			    arg ? arg : "", mon_nam(mon),
			    JMONNAM(mwep->corpsenm));
#endif /*JP*/
		}
		minstapetrify(mon, byplayer);
	}
}

void
fill_pit(x, y)
int x, y;
{
	struct obj *otmp;
	struct trap *t;

	if ((t = t_at(x, y)) &&
	    ((t->ttyp == PIT) || (t->ttyp == SPIKED_PIT)) &&
	    (otmp = sobj_at(BOULDER, x, y))) {
		obj_extract_self(otmp);
		(void) flooreffects(otmp, x, y, E_J("settle","?[??????"));
	}
}

STATIC_OVL void
dofiretrap(box)
struct obj *box;	/* null for floor trap */
{
	boolean see_it = !Blind;
	int num, alt;

/* Bug: for box case, the equivalent of burn_floor_paper() ought
 * to be done upon its contents.
 */

	if ((box && !carried(box)) ? is_pool(box->ox, box->oy) : Underwater) {
#ifndef JP
	    pline("A cascade of steamy bubbles erupts from %s!",
		    the(box ? xname(box) : surface(u.ux,u.uy)));
	    if (Fire_resistance) You("are uninjured.");
	    else losehp(rnd(3), "boiling water", KILLED_BY);
#else
	    pline("?????????????C???A??%s???????????????I",
		  box ? xname(box) : surface(u.ux,u.uy));
	    if (Fire_resistance) You("???????????????B");
	    else losehp(rnd(3), "?M????????????", KILLED_BY);
#endif /*JP*/
	    return;
	}
#ifndef JP
	pline("A %s %s from %s!", tower_of_flame,
	      box ? "bursts" : "erupts",
	      the(box ? xname(box) : surface(u.ux,u.uy)));
#else
	pline("????%s????????%s?I",
		box ? xname(box) : surface(u.ux,u.uy),
		box ? "?o????" : "????????");
#endif /*JP*/
	if (Fire_resistance) {
	    shieldeff(u.ux, u.uy);
	    num = is_full_resist(FIRE_RES) ? 0 : rn2(2);
	} else if (Upolyd) {
	    num = d(2,4);
	    switch (u.umonnum) {
	    case PM_PAPER_GOLEM:   alt = u.mhmax; break;
	    case PM_STRAW_GOLEM:   alt = u.mhmax / 2; break;
	    case PM_WOOD_GOLEM:    alt = u.mhmax / 4; break;
	    case PM_LEATHER_GOLEM: alt = u.mhmax / 8; break;
	    default: alt = 0; break;
	    }
	    if (alt > num) num = alt;
	    if (u.mhmax > mons[u.umonnum].mlevel)
		u.mhmax -= rn2(min(u.mhmax,num + 1)), flags.botl = 1;
	} else {
	    num = d(2,4);
	    if (u.uhpmax > u.ulevel)
		addhpmax(-rn2(min(u.uhpmax,num + 1)));
	}
	if (!num)
	    You(E_J("are uninjured.","???????????????B"));
	else
	    losehp(num, tower_of_flame, KILLED_BY_AN);
	burn_away_slime();

	if (burnarmor(&youmonst) || rn2(3)) {
	    destroy_item(SCROLL_CLASS, AD_FIRE);
	    destroy_item(SPBOOK_CLASS, AD_FIRE);
	    destroy_item(POTION_CLASS, AD_FIRE);
	    destroy_item(TOOL_CLASS, AD_FIRE);
	}
	if (!box && burn_floor_paper(u.ux, u.uy, see_it, TRUE) && !see_it)
	    You(E_J("smell paper burning.","?????R???????????k?????B"));
	if (is_ice(u.ux, u.uy))
	    melt_ice(u.ux, u.uy);
}

STATIC_OVL void
domagictrap()
{
	register int fate = rnd(20);

	/* What happened to the poor sucker? */

	/* C343-147: expels() -> spoteffects() -> dotrap() ->
	   domagictrap() -> tamedog(), this sequence moves the
	   swallower into a new monst structure, and makes the
	   caller of expels() access an obsolete pointer.
	   Now tamedog() won't move a monst structure, so that
	   we don't need to cancel the taming effect */

	if (fate < 10) {
	  /* Most of the time, it creates some monsters. */
	  register int cnt = rnd(4);

	  if (!resists_blnd(&youmonst)) {
		You(E_J("are momentarily blinded by a flash of light!",
			"?????????M???????????????I"));
		make_blinded((long)rn1(5,10),FALSE);
		if (!Blind) Your(vision_clears);
	  } else if (!Blind) {
		You(E_J("see a flash of light!","?????????M?????????I"));
	  }  else
		You_hear(E_J("a deafening roar!","!???????????????K??"));
	  while(cnt--)
		(void) makemon((struct permonst *) 0, u.ux, u.uy, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
	}
	else
	  switch (fate) {

	     case 10:
	     case 11:
		      /* sometimes nothing happens */
			break;
	     case 12: /* a flash of fire */
			dofiretrap((struct obj *)0);
			break;

	     /* odd feelings */
	     case 13:	pline(E_J("A shiver runs up and down your %s!",
				  "????????%s???????????????I" ),
			      body_part(SPINE));
			break;
	     case 14:	You_hear(Hallucination ?
				E_J("the moon howling at you.","?????????????????????i????????") :
				E_J("distant howling.","???i????"));
			break;
	     case 15:	if (on_level(&u.uz, &qstart_level))
			    You_feel(E_J("%slike the prodigal son.",
					 "%s?????????????q???????????????B"),
			      (flags.female || (Upolyd && is_neuter(youmonst.data))) ?
				     E_J("oddly ","?????????????A") : "");
			else
			    You(E_J("suddenly yearn for %s.",
				    "?}??%s???????????????B"),
				Hallucination ? E_J("Cleveland","????") :
			    (In_quest(&u.uz) || at_dgn_entrance("The Quest")) ?
				    E_J("your nearby homeland","??????????") :
				    E_J("your distant homeland","????????"));
			break;
	     case 16:   Your(E_J("pack shakes violently!","?w???????????????h?????I"));
			break;
	     case 17:	You(Hallucination ?
				E_J("smell hamburgers.","?n???o?[?K?[?????????k?????B") :
				E_J("smell charred flesh.","???????????L?????k?????B"));
			break;
	     case 18:	You_feel(E_J("tired.","???????B"));
			break;

	     /* very occasionally something nice happens. */

	     case 19:
		    /* tame nearby monsters */
		   {   register int i,j;
		       register struct monst *mtmp;

		       (void) adjattrib(A_CHA,1,FALSE);
		       for(i = -1; i <= 1; i++) for(j = -1; j <= 1; j++) {
			   if(!isok(u.ux+i, u.uy+j)) continue;
			   mtmp = m_at(u.ux+i, u.uy+j);
			   if(mtmp)
			       (void) tamedog(mtmp, (struct obj *)0);
		       }
		       break;
		   }

	     case 20:
		    /* uncurse stuff */
		   {	struct obj pseudo;
			long save_conf = HConfusion;

			pseudo = zeroobj;   /* neither cursed nor blessed */
			pseudo.otyp = SCR_REMOVE_CURSE;
			HConfusion = 0L;
			(void) seffects(&pseudo);
			HConfusion = save_conf;
			break;
		   }
	     default: break;
	  }
}

/*
 * Scrolls, spellbooks, potions, and flammable items
 * may get affected by the fire.
 *
 * Return number of objects destroyed. --ALI
 */
int
fire_damage(chain, force, here, x, y)
struct obj *chain;
boolean force, here;
xchar x, y;
{
    int chance;
    struct obj *obj, *otmp, *nobj, *ncobj;
    int retval = 0;
    int in_sight = !Blind && couldsee(x, y);	/* Don't care if it's lit */
    int dindx;

    for (obj = chain; obj; obj = nobj) {
	nobj = here ? obj->nexthere : obj->nobj;

	/* object might light in a controlled manner */
	if (catch_lit(obj))
	    continue;

	if (Is_container(obj)) {
	    switch (obj->otyp) {
	    case ICE_BOX:
		continue;		/* Immune */
		/*NOTREACHED*/
		break;
	    case CHEST:
		chance = 40;
		break;
	    case LARGE_BOX:
		chance = 30;
		break;
	    default:
		chance = 20;
		break;
	    }
	    if (!force && (Luck + 5) > rn2(chance))
		continue;
	    /* Container is burnt up - dump contents out */
	    if (in_sight) pline(E_J("%s catches fire and burns.",
				    "%s???????????A?R?????I"), Yname2(obj));
	    if (Has_contents(obj)) {
		if (in_sight) pline(E_J("Its contents fall out.",
					"???g???]?????????B"));
		for (otmp = obj->cobj; otmp; otmp = ncobj) {
		    ncobj = otmp->nobj;
		    obj_extract_self(otmp);
		    if (!flooreffects(otmp, x, y, ""))
			place_object(otmp, x, y);
		}
	    }
	    delobj(obj);
	    retval++;
	} else if (!force && (Luck + 5) > rn2(20)) {
	    /*  chance per item of sustaining damage:
	     *	max luck (full moon):	 5%
	     *	max luck (elsewhen):	10%
	     *	avg luck (Luck==0):	75%
	     *	awful luck (Luck<-4):  100%
	     */
	    continue;
	} else if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS) {
	    if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL)
		continue;
	    if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
#ifndef JP
		if (in_sight) pline("Smoke rises from %s.", the(xname(obj)));
#else
		if (in_sight) pline("%s?????????????????????B", xname(obj));
#endif /*JP*/
		continue;
	    }
	    dindx = (obj->oclass == SCROLL_CLASS) ? 2 : 3;
	    if (in_sight)
#ifndef JP
		pline("%s %s.", Yname2(obj), (obj->quan > 1) ?
		      destroy_strings[dindx*3 + 1] : destroy_strings[dindx*3]);
#else
		pline("%s%s?B", yname(obj), destroy_strings[dindx*2]);
#endif /*JP*/
	    delobj(obj);
	    retval++;
	} else if (obj->oclass == POTION_CLASS) {
	    dindx = 1;
	    if (in_sight)
#ifndef JP
		pline("%s %s.", Yname2(obj), (obj->quan > 1) ?
		      destroy_strings[dindx*3 + 1] : destroy_strings[dindx*3]);
#else
		pline("%s%s?B", yname(obj), destroy_strings[dindx*2]);
#endif /*JP*/
	    delobj(obj);
	    retval++;
	} else if (is_flammable(obj) && obj->oeroded < MAX_ERODE &&
		   !(obj->oerodeproof || (obj->blessed && !rnl(4)))) {
	    if (in_sight) {
#ifndef JP
		pline("%s %s%s.", Yname2(obj), otense(obj, "burn"),
		      obj->oeroded+1 == MAX_ERODE ? " completely" :
		      obj->oeroded ? " further" : "");
#else
		pline("%s??%s???????B", yname(obj),
		      obj->oeroded+1 == MAX_ERODE ? "???S??" :
		      obj->oeroded ? "??????" : "");
#endif /*JP*/
	    }
	    obj->oeroded++;
	}
    }

    if (retval && !in_sight)
	You(E_J("smell smoke.","?????????????k?????B"));
    return retval;
}

int
water_damage(obj, force, here)
register struct obj *obj;
register boolean force, here;
{
	struct obj *otmp;
	int retval = 0;

	/* Scrolls, spellbooks, potions, weapons and
	   pieces of armor may get affected by the water */
	for (; obj; obj = otmp) {
		otmp = here ? obj->nexthere : obj->nobj;

		(void) snuff_lit(obj);

		if(obj->otyp == CAN_OF_GREASE && obj->spe > 0) {
			continue;
		} else if(obj->greased) {
			if (force || !rn2(4)) obj->greased = 0;
		} else if(Is_container(obj) && !Is_box(obj) &&
			(obj->otyp != OILSKIN_SACK || (obj->cursed && !rn2(3)))) {
			water_damage(obj->cobj, force, FALSE);
		} else if (!force && (Luck + 5) > rn2(20)) {
			/*  chance per item of sustaining damage:
			 *	max luck (full moon):	 5%
			 *	max luck (elsewhen):	10%
			 *	avg luck (Luck==0):	75%
			 *	awful luck (Luck<-4):  100%
			 */
			continue;
		} else if (obj->oclass == SCROLL_CLASS) {
#ifdef MAIL
		    if (obj->otyp != SCR_MAIL)
#endif
		    {
			set_otyp(obj, SCR_BLANK_PAPER);
			obj->spe = 0;
		    }
		} else if (obj->oclass == SPBOOK_CLASS) {
			if (obj->otyp == SPE_BOOK_OF_THE_DEAD)
#ifndef JP
				pline("Steam rises from %s.", the(xname(obj)));
#else
				pline("%s???????C???????????????B", xname(obj));
#endif /*JP*/
			else set_otyp(obj, SPE_BLANK_PAPER);
		} else if (obj->oclass == POTION_CLASS) {
			if (obj->otyp == POT_ACID) {
				/* damage player/monster? */
				pline(E_J("A potion explodes!","?????????????I"));
				delobj(obj);
				retval++;
				continue;
			} else if (obj->odiluted) {
				set_otyp(obj, POT_WATER);
				obj->blessed = obj->cursed = 0;
				obj->odiluted = 0;
			} else if (obj->otyp != POT_WATER)
				obj->odiluted++;
		} else if (is_rustprone(obj) && obj->oeroded < MAX_ERODE &&
			  !(obj->oerodeproof || (obj->blessed && !rnl(4)))) {
			/* all metal stuff and armor except (body armor
			   protected by oilskin cloak) */
			if(obj->oclass != ARMOR_CLASS || obj != uarm ||
			   !uarmc || uarmc->otyp != OILSKIN_CLOAK ||
			   (uarmc->cursed && !rn2(3)))
				obj->oeroded++;
		}
	}
	return retval;
}

/*
 * This function is potentially expensive - rolling
 * inventory list multiple times.  Luckily it's seldom needed.
 * Returns TRUE if disrobing made player unencumbered enough to
 * crawl out of the current predicament.
 */
STATIC_OVL boolean
emergency_disrobe(lostsome)
boolean *lostsome;
{
	int invc = inv_cnt();

	while (near_capacity() > (Punished ? UNENCUMBERED : SLT_ENCUMBER)) {
	    register struct obj *obj, *otmp = (struct obj *)0;
	    register int i;

	    /* Pick a random object */
	    if (invc > 0) {
		i = rn2(invc);
		for (obj = invent; obj; obj = obj->nobj) {
		    /*
		     * Undroppables are: body armor, boots, gloves,
		     * amulets, and rings because of the time and effort
		     * in removing them + loadstone and other cursed stuff
		     * for obvious reasons.
		     */
		    if (!((obj->otyp == LOADSTONE && obj->cursed) ||
			  obj == uamul || obj == uleft || obj == uright ||
			  obj == ublindf || obj == uarm || obj == uarmc ||
			  obj == uarmg || obj == uarmf ||
			  obj == uarmu ||
			  (obj->cursed && (obj == uarmh || obj == uarms)) ||
			  welded(obj)))
			otmp = obj;
		    /* reached the mark and found some stuff to drop? */
		    if (--i < 0 && otmp) break;

		    /* else continue */
		}
	    }

	    if (!otmp) {
		/* Nothing available left to drop; try gold */
		if (u.ugold) {
		    pline("In desperation, you drop your purse.");
		    /* Hack: gold is not in the inventory, so make a gold object
		     * and put it at the head of the inventory list.
		     */
		    obj = mkgoldobj(u.ugold);    /* removes from u.ugold */
		    obj->in_use = TRUE;
		    u.ugold = obj->quan;         /* put the gold back */
		    assigninvlet(obj);           /* might end up as NOINVSYM */
		    obj->nobj = invent;
		    invent = obj;
		    *lostsome = TRUE;
		    dropx(obj);
		    continue;                    /* Try again */
		}
		/* We can't even drop gold! */
		return (FALSE);
	    }

	    if (otmp->owornmask) remove_worn_item(otmp, FALSE);
	    *lostsome = TRUE;
	    dropx(otmp);
	    invc--;
	}
	return(TRUE);
}

/*
 *  return(TRUE) == player relocated
 */
boolean
drown()
{
	boolean inpool_ok = FALSE, crawl_ok;
	int i, x, y;

	/* happily wading in the same contiguous pool */
	if (u.uinwater && is_pool(u.ux-u.dx,u.uy-u.dy) &&
	    (Swimming || Amphibious)) {
		/* water effects on objects every now and then */
		if (!rn2(5)) inpool_ok = TRUE;
		else return(FALSE);
	}

	if (!u.uinwater) {
#ifndef JP
	    You("%s into the water%c",
		Is_waterlevel(&u.uz) ? "plunge" :
		Swimming ? "swim" : "fall",
		Amphibious || Swimming ? '.' : '!');
	    if (!Swimming && !Is_waterlevel(&u.uz))
		    You("sink like %s.",
			Hallucination ? "the Titanic" : "a rock");
#else
	    You("????%s%s",
		Is_waterlevel(&u.uz) ? "??????????" :
		Swimming ? "??????" : "??????",
		Amphibious || Swimming ? "?B" : "?I");
	    if (!Swimming && !Is_waterlevel(&u.uz))
		    You("%s???????????????????B",
			Hallucination ? "?^?C?^?j?b?N??" : "??");
#endif /*JP*/
	}

	water_damage(invent, FALSE, FALSE);

	if (u.umonnum == PM_GREMLIN && rn2(3))
	    (void)split_mon(&youmonst, (struct monst *)0);
	else if (u.umonnum == PM_IRON_GOLEM) {
	    You(E_J("rust!","?K?????I"));
	    i = d(2,6);
	    if (u.mhmax > i) u.mhmax -= i;
	    losehp(i, E_J("rusting away","?K????????????"), KILLED_BY);
	}
	if (inpool_ok) return(FALSE);

	if ((i = number_leashed()) > 0) {
#ifndef JP
		pline_The("leash%s slip%s loose.",
			(i > 1) ? "es" : "",
			(i > 1) ? "" : "s");
#else
		pline("?????j???????????B");
#endif /*JP*/
		unleash_all();
	}

	if (Swimming && !Punished) return(FALSE);

	if (Amphibious || Swimming) {
		if (Amphibious) {
			if (flags.verbose)
				pline(E_J("But you aren't drowning.",
					  "?????A?????????M???????B"));
			if (!Is_waterlevel(&u.uz)) {
				if (Hallucination)
					Your(E_J("keel hits the bottom.",
						 "?D?????????????????????B"));
				else
					You(E_J("touch bottom.","?????????????B"));
			}
		}
		if (Punished) {
			unplacebc();
			placebc();
		}
		vision_recalc(2);	/* unsee old position */
		u.uinwater = 1;
		under_water(1);
		vision_full_recalc = 1;
		return(FALSE);
	}
	if (can_levitate() && !Is_waterlevel(&u.uz)

#ifdef STEED
	    && !u.usteed
#endif
	   ) {
		You(E_J("hurryingly invoke magical levitation.",
			"???????????V???????????o?????B"));
		start_levitation();
		return(FALSE);
	}
	if ((Teleportation || can_teleport(youmonst.data)) &&
		    !u.usleep && (Teleport_control || rn2(3) < Luck+2)) {
		You(E_J("attempt a teleport spell.",
			"?u?????????????????????????B"));	/* utcsri!carroll */
		if (!level.flags.noteleport) {
			(void) dotele();
			if(!is_pool(u.ux,u.uy))
				return(TRUE);
		} else pline_The(E_J("attempted teleport spell fails.",
				     "???????????????s?????B"));
	}
#ifdef STEED
	if (u.usteed) {
		dismount_steed(DISMOUNT_GENERIC);
		if(!is_pool(u.ux,u.uy))
			return(TRUE);
	}
#endif
	crawl_ok = FALSE;
	x = y = 0;		/* lint suppression */
	/* if sleeping, wake up now so that we don't crawl out of water
	   while still asleep; we can't do that the same way that waking
	   due to combat is handled; note unmul() clears u.usleep */
	if (u.usleep) unmul(E_J("Suddenly you wake up!",
				"???R?A?????????????o?????I"));
	/* can't crawl if unable to move (crawl_ok flag stays false) */
	if (multi < 0 || (Upolyd && !youmonst.data->mmove)) goto crawl;
	/* look around for a place to crawl to */
	for (i = 0; i < 100; i++) {
		x = rn1(3,u.ux - 1);
		y = rn1(3,u.uy - 1);
		if (goodpos(x, y, &youmonst, 0)) {
			crawl_ok = TRUE;
			goto crawl;
		}
	}
	/* one more scan */
	for (x = u.ux - 1; x <= u.ux + 1; x++)
		for (y = u.uy - 1; y <= u.uy + 1; y++)
			if (goodpos(x, y, &youmonst, 0)) {
				crawl_ok = TRUE;
				goto crawl;
			}
 crawl:
	if (crawl_ok) {
		boolean lost = FALSE;
		/* time to do some strip-tease... */
		boolean succ = Is_waterlevel(&u.uz) ? TRUE :
				emergency_disrobe(&lost);

		You(E_J("try to crawl out of the water.",
			"?????????????????????????B"));
		if (lost)
			You(E_J("dump some of your gear to lose weight...",
				"?d???????????????A?????????????????????B"));
		if (succ) {
			pline(E_J("Pheew!  That was close.",
				  "?n?@?n?@?A???????????I"));
			teleds(x,y,TRUE);
			return(TRUE);
		}
		/* still too much weight */
		pline(E_J("But in vain.","???????????????B"));
	}
	u.uinwater = 1;
	You(E_J("drown.","?M?????B"));
	killer_format = KILLED_BY_AN;
	killer = (levl[u.ux][u.uy].typ == POOL || Is_medusa_level(&u.uz)) ?
	    E_J("pool of water","????????????") : E_J("moat","?x????????");
	done(DROWNING);
	/* oops, we're still alive.  better get out of the water. */
	while (!safe_teleds(TRUE)) {
		pline(E_J("You're still drowning.",
			  "?????????????M?????????B"));
		done(DROWNING);
	}
	if (u.uinwater) {
	    u.uinwater = 0;
#ifndef JP
	    You("find yourself back %s.", Is_waterlevel(&u.uz) ?
		"in an air bubble" : "on land");
#else
	    You("????????????%s?????????????B", Is_waterlevel(&u.uz) ?
		"?C?A????" : "??????");
#endif /*JP*/
	}
	return(TRUE);
}

void
drain_en(n)
register int n;
{
	if (!u.uenmax) return;
	You_feel(E_J("your magical energy drain away!",
		     "?????????????D?????????????????????I"));
	u.uen -= n;
	if(u.uen < 0)  {
		u.uenmax += u.uen;
		if(u.uenmax < 0) u.uenmax = 0;
		u.uen = 0;
	}
	flags.botl = 1;
}

int
dountrap()	/* disarm a trap */
{
	if (near_capacity() >= HVY_ENCUMBER) {
	    pline(E_J("You're too strained to do that.",
		      "???????????????A???????????????d???????B"));
	    return 0;
	}
	if ((nohands(youmonst.data) && !webmaker(youmonst.data)) || !youmonst.data->mmove) {
	    pline(E_J("And just how do you expect to do that?",
		      "???A???????????????????????????????????????H"));
	    return 0;
	} else if (u.ustuck && sticks(youmonst.data)) {
	    pline(E_J("You'll have to let go of %s first.",
		      "?????????????A%s???????????????????????????????B"), mon_nam(u.ustuck));
	    return 0;
	}
	if (u.ustuck || (welded(uwep) && bimanual(uwep))) {
#ifndef JP
	    Your("%s seem to be too busy for that.",
		 makeplural(body_part(HAND)));
#else
	    Your("%s?????????????]?T?????????????B", body_part(HAND));
#endif /*JP*/
	    return 0;
	}
	return untrap(FALSE);
}
#endif /* OVLB */
#ifdef OVL2

/* Probability of disabling a trap.  Helge Hafting */
STATIC_OVL int
untrap_prob(ttmp)
struct trap *ttmp;
{
	int chance = 3;

	/* Only spiders know how to deal with webs reliably */
	if (ttmp->ttyp == WEB && !webmaker(youmonst.data))
	 	chance = 30;
	if (Confusion || Hallucination) chance++;
	if (Blind) chance++;
	if (Stunned) chance += 2;
	if (Fumbling) chance *= 2;
	/* Your own traps are better known than others. */
	if (ttmp && ttmp->madeby_u) chance--;
	if (Role_if(PM_ROGUE)) {
	    if (rn2(2 * MAXULEV) < u.ulevel) chance--;
	    if (u.uhave.questart && chance > 1) chance--;
	} else if (Role_if(PM_RANGER) && chance > 1) chance--;
	return rn2(chance);
}

/* Replace trap with object(s).  Helge Hafting */
void
cnv_trap_obj(otyp, cnt, ttmp)
int otyp;
int cnt;
struct trap *ttmp;
{
	struct obj *otmp = mksobj(otyp, TRUE, FALSE);
	otmp->quan=cnt;
	otmp->owt = weight(otmp);
	/* Only dart traps are capable of being poisonous */
	if (otyp != DART)
	    otmp->opoisoned = 0;
	place_object(otmp, ttmp->tx, ttmp->ty);
	/* Sell your own traps only... */
	if (ttmp->madeby_u) sellobj(otmp, ttmp->tx, ttmp->ty);
	stackobj(otmp);
	newsym(ttmp->tx, ttmp->ty);
	deltrap(ttmp);
}

/* while attempting to disarm an adjacent trap, we've fallen into it */
STATIC_OVL void
move_into_trap(ttmp)
struct trap *ttmp;
{
	int bc;
	xchar x = ttmp->tx, y = ttmp->ty, bx, by, cx, cy;
	boolean unused;

	/* we know there's no monster in the way, and we're not trapped */
	if (!Punished || drag_ball(x, y, &bc, &bx, &by, &cx, &cy, &unused,
		TRUE)) {
	    u.ux0 = u.ux,  u.uy0 = u.uy;
	    u.ux = x,  u.uy = y;
	    u.umoved = TRUE;
	    newsym(u.ux0, u.uy0);
	    vision_recalc(1);
	    check_leash(u.ux0, u.uy0);
	    if (Punished) move_bc(0, bc, bx, by, cx, cy);
	    spoteffects(FALSE);	/* dotrap() */
	    exercise(A_WIS, FALSE);
	}
}

/* 0: doesn't even try
 * 1: tries and fails
 * 2: succeeds
 */
STATIC_OVL int
try_disarm(ttmp, force_failure)
struct trap *ttmp;
boolean force_failure;
{
	struct monst *mtmp = m_at(ttmp->tx,ttmp->ty);
	int ttype = ttmp->ttyp;
	boolean under_u = (!u.dx && !u.dy);
	boolean holdingtrap = (ttype == BEAR_TRAP || ttype == WEB);
	
	/* Test for monster first, monsters are displayed instead of trap. */
	if (mtmp && (!mtmp->mtrapped || !holdingtrap)) {
		pline(E_J("%s is in the way.",
			  "????????????%s???????B"), Monnam(mtmp));
		return 0;
	}
	/* We might be forced to move onto the trap's location. */
	if (sobj_at(BOULDER, ttmp->tx, ttmp->ty)
				&& !Passes_walls && !under_u) {
		There(E_J("is a boulder in your way.",
			  "???????????A?????????s???????j?????????B"));
		return 0;
	}
	/* duplicate tight-space checks from test_move */
	if (u.dx && u.dy &&
	    bad_rock(youmonst.data,u.ux,ttmp->ty) &&
	    bad_rock(youmonst.data,ttmp->tx,u.uy)) {
	    if ((invent && (inv_weight() + weight_cap() > 600)) ||
		bigmonst(youmonst.data)) {
		/* don't allow untrap if they can't get thru to it */
		You(E_J("are unable to reach the %s!",
			"%s???????????????????????I"),
		    defsyms[trap_to_defsym(ttype)].explanation);
		return 0;
	    }
	}
	/* untrappable traps are located on the ground. */
	if (!can_reach_floor()) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
			You(E_J("aren't skilled enough to reach from %s.",
				"%s?????????????????????????????n?????????????B"),
				mon_nam(u.usteed));
		else
#endif
		You(E_J("are unable to reach the %s!",
			"%s???????????????????????I"),
			defsyms[trap_to_defsym(ttype)].explanation);
		return 0;
	}

	/* Will our hero succeed? */
	if (force_failure || untrap_prob(ttmp)) {
		if (rnl(5)) {
		    pline(E_J("Whoops...","?????????c?B"));
		    if (mtmp) {		/* must be a trap that holds monsters */
			if (ttype == BEAR_TRAP) {
			    if (mtmp->mtame) abuse_dog(mtmp);
			    if (mlosehp(mtmp, rnd(4)) <= 0) killed(mtmp);
			} else if (ttype == WEB) {
			    if (!webmaker(youmonst.data)) {
				struct trap *ttmp2 = maketrap(u.ux, u.uy, WEB);
				if (ttmp2) {
				    pline_The(E_J("webbing sticks to you. You're caught too!",
						  "?w???????????????????????????B?????????????????????????????I"));
				    dotrap(ttmp2, NOWEBMSG);
#ifdef STEED
				    if (u.usteed && u.utrap) {
					/* you, not steed, are trapped */
					dismount_steed(DISMOUNT_FELL);
				    }
#endif
				}
			    } else
				pline(E_J("%s remains entangled.",
					  "%s?????????????????????B"), Monnam(mtmp));
			}
		    } else if (under_u) {
			dotrap(ttmp, 0);
		    } else {
			move_into_trap(ttmp);
		    }
		} else {
#ifndef JP
		    pline("%s %s is difficult to %s.",
			  ttmp->madeby_u ? "Your" : under_u ? "This" : "That",
			  defsyms[trap_to_defsym(ttype)].explanation,
			  (ttype == WEB) ? "remove" : "disarm");
#else
		    pline("%s??%s??????????%s?????B",
			  ttmp->madeby_u ? "??????" : "??",
			  defsyms[trap_to_defsym(ttype)].explanation,
			  (ttype == WEB) ? "????????" : "????????");
#endif /*JP*/
		}
		return 1;
	}
	return 2;
}

STATIC_OVL void
reward_untrap(ttmp, mtmp)
struct trap *ttmp;
struct monst *mtmp;
{
	if (!ttmp->madeby_u) {
	    if (rnl(10) < 8 && !mtmp->mpeaceful &&
		    !mtmp->msleeping && !mtmp->mfrozen &&
		    !mindless(mtmp->data) &&
		    mtmp->data->mlet != S_HUMAN) {
		setmpeaceful(mtmp, TRUE);	/* reset alignment */
		pline(E_J("%s is grateful.","%s???????????????B"), Monnam(mtmp));
	    }
	    /* Helping someone out of a trap is a nice thing to do,
	     * A lawful may be rewarded, but not too often.  */
	    if (!rn2(3) && !rnl(8) && u.ualign.type == A_LAWFUL) {
		adjalign(1);
		You_feel(E_J("that you did the right thing.",
			     "?A?????????????s?????????????????B"));
	    }
	}
}

STATIC_OVL int
disarm_holdingtrap(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
	struct monst *mtmp;
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return fails;

	/* ok, disarm it. */

	/* untrap the monster, if any.
	   There's no need for a cockatrice test, only the trap is touched */
	if ((mtmp = m_at(ttmp->tx,ttmp->ty)) != 0) {
		mtmp->mtrapped = 0;
#ifndef JP
		You("remove %s %s from %s.", the_your[ttmp->madeby_u],
			(ttmp->ttyp == BEAR_TRAP) ? "bear trap" : "webbing",
			mon_nam(mtmp));
#else
		You("%s????%s%s???????????????B",
			mon_nam(mtmp), the_your[ttmp->madeby_u],
			(ttmp->ttyp == BEAR_TRAP) ? "?g???o?T?~" : "?w??????");
#endif /*JP*/
		reward_untrap(ttmp, mtmp);
	} else {
		if (ttmp->ttyp == BEAR_TRAP) {
			You(E_J("disarm %s bear trap.","%s?g???o?T?~???????????B"),
			the_your[ttmp->madeby_u]);
			cnv_trap_obj(BEARTRAP, 1, ttmp);
		} else /* if (ttmp->ttyp == WEB) */ {
			You(E_J("succeed in removing %s web.",
				"%s?w???????????????????B"),
			    the_your[ttmp->madeby_u]);
			deltrap(ttmp);
		}
	}
	newsym(u.ux + u.dx, u.uy + u.dy);
	return 1;
}

STATIC_OVL int
disarm_landmine(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return fails;
	You(E_J("disarm %s land mine.","%s?n?????????????B"), the_your[ttmp->madeby_u]);
	cnv_trap_obj(LAND_MINE, 1, ttmp);
	return 1;
}

/* getobj will filter down to cans of grease and known potions of oil */
static NEARDATA const char oil[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS, 0 };

int
getobj_filter_untrap_squeaky(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	if (otyp == CAN_OF_GREASE ||
	    (otyp == POT_OIL && otmp->dknown &&
	     objects[POT_OIL].oc_name_known))
		return GETOBJ_CHOOSEIT;
	return 0;
}

/* it may not make much sense to use grease on floor boards, but so what? */
STATIC_OVL int
disarm_squeaky_board(ttmp)
struct trap *ttmp;
{
	struct obj *obj;
	boolean bad_tool;
	int fails;
#ifdef JP
	static const struct getobj_words utw = { 0, "???g????", "????????????", "??????????" };
#endif /*JP*/

	obj = getobj(oil, E_J("untrap with",&utw), getobj_filter_untrap_squeaky);
	if (!obj) return 0;

	bad_tool = (obj->cursed ||
			((obj->otyp != POT_OIL || obj->lamplit) &&
			 (obj->otyp != CAN_OF_GREASE || !obj->spe)));

	fails = try_disarm(ttmp, bad_tool);
	if (fails < 2) return fails;

	/* successfully used oil or grease to fix squeaky board */
	if (obj->otyp == CAN_OF_GREASE) {
	    consume_obj_charge(obj, TRUE);
	} else {
	    useup(obj);	/* oil */
	    makeknown(POT_OIL);
	}
	You(E_J("repair the squeaky board.","?a?????????C???????B"));	/* no madeby_u */
	deltrap(ttmp);
	newsym(u.ux + u.dx, u.uy + u.dy);
	more_experienced(1, 5);
	newexplevel();
	return 1;
}

/* removes traps that shoot arrows, darts, etc. */
STATIC_OVL int
disarm_shooting_trap(ttmp, otyp)
struct trap *ttmp;
int otyp;
{
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return fails;
	You(E_J("disarm %s trap.","%s?????????????B"), the_your[ttmp->madeby_u]);
	cnv_trap_obj(otyp, 50-rnl(50), ttmp);
	return 1;
}

/* Is the weight too heavy?
 * Formula as in near_capacity() & check_capacity() */
STATIC_OVL int
try_lift(mtmp, ttmp, wt, stuff)
struct monst *mtmp;
struct trap *ttmp;
int wt;
boolean stuff;
{
	int wc = weight_cap();

	if (((wt * 2) / wc) >= HVY_ENCUMBER) {
	    pline(E_J("%s is %s for you to lift.",
		      "%s????????????????%s?B"), Monnam(mtmp),
		  stuff ? E_J("carrying too much","????????????????????") : E_J("too heavy","?d??????"));
	    if (!ttmp->madeby_u && !mtmp->mpeaceful && mtmp->mcanmove &&
		    !mindless(mtmp->data) &&
		    mtmp->data->mlet != S_HUMAN && rnl(10) < 3) {
		setmpeaceful(mtmp, TRUE);		/* reset alignment */
		pline(E_J("%s thinks it was nice of you to try.",
			  "%s???????????w???????????????????B"), Monnam(mtmp));
	    }
	    return 0;
	}
	return 1;
}

/* Help trapped monster (out of a (spiked) pit) */
STATIC_OVL int
help_monster_out(mtmp, ttmp)
struct monst *mtmp;
struct trap *ttmp;
{
	int wt;
	struct obj *otmp;
	boolean uprob;

	/*
	 * This works when levitating too -- consistent with the ability
	 * to hit monsters while levitating.
	 *
	 * Should perhaps check that our hero has arms/hands at the
	 * moment.  Helping can also be done by engulfing...
	 *
	 * Test the monster first - monsters are displayed before traps.
	 */
	if (!mtmp->mtrapped) {
		pline(E_J("%s isn't trapped.","%s?????????????????????B"), Monnam(mtmp));
		return 0;
	}
	/* Do you have the necessary capacity to lift anything? */
	if (check_capacity((char *)0)) return 1;

	/* Will our hero succeed? */
	if ((uprob = untrap_prob(ttmp)) && !mtmp->msleeping && mtmp->mcanmove) {
#ifndef JP
		You("try to reach out your %s, but %s backs away skeptically.",
			makeplural(body_part(ARM)),
			mon_nam(mtmp));
#else
		You("%s???????????????A%s?????b?????????????????B",
			body_part(ARM), mon_nam(mtmp));
#endif /*JP*/
		return 1;
	}


	/* is it a cockatrice?... */
	if (touch_petrifies(mtmp->data) && !uarmg && !Stone_resistance) {
#ifndef JP
		You("grab the trapped %s using your bare %s.",
				mtmp->data->mname, makeplural(body_part(HAND)));
#else
		You("????????????%s???f?????????????????????B",	JMONNAM(monsndx(mtmp->data)));
#endif /*JP*/

		if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
			display_nhwindow(WIN_MESSAGE, FALSE);
		else {
			char kbuf[BUFSZ];

#ifndef JP
			Sprintf(kbuf, "trying to help %s out of a pit",
					an(mtmp->data->mname));
#else
			Sprintf(kbuf, "%s????????????????????????????",
					JMONNAM(monsndx(mtmp->data)));
#endif /*JP*/
			instapetrify(kbuf);
			return 1;
		}
	}
	/* need to do cockatrice check first if sleeping or paralyzed */
	if (uprob) {
	    You(E_J("try to grab %s, but cannot get a firm grasp.",
		    "%s???????????????????????A?????????????????????????????????B"),
		mon_nam(mtmp));
	    if (mtmp->msleeping) {
		mtmp->msleeping = 0;
		pline(E_J("%s awakens.","%s???????o???????B"), Monnam(mtmp));
	    }
	    return 1;
	}

#ifndef JP
	You("reach out your %s and grab %s.",
	    makeplural(body_part(ARM)), mon_nam(mtmp));
#else
	You("%s???????????????A%s???????????B",
	    body_part(ARM), mon_nam(mtmp));
#endif /*JP*/

	if (mtmp->msleeping) {
	    mtmp->msleeping = 0;
	    pline(E_J("%s awakens.","%s???????o???????B"), Monnam(mtmp));
	} else if (mtmp->mfrozen && !rn2(mtmp->mfrozen)) {
	    /* After such manhandling, perhaps the effect wears off */
	    mtmp->mcanmove = 1;
	    mtmp->mfrozen = 0;
	    pline(E_J("%s stirs.","%s???g?k???????B"), Monnam(mtmp));
	}

	/* is the monster too heavy? */
	wt = inv_weight() + mtmp->data->cwt;
#ifdef MONSTEED
	if (is_mriding(mtmp)) wt += mtmp->mlink->data->cwt;
#endif
	if (!try_lift(mtmp, ttmp, wt, FALSE)) return 1;

	/* is the monster with inventory too heavy? */
	for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		wt += otmp->owt;
#ifdef MONSTEED
	if (is_mriding(mtmp))
	    for (otmp = mtmp->mlink->minvent; otmp; otmp = otmp->nobj)
		wt += otmp->owt;
#endif
	if (!try_lift(mtmp, ttmp, wt, TRUE)) return 1;

	You(E_J("pull %s out of the pit.",
		"%s???????????????????????o?????B"), mon_nam(mtmp));
	mtmp->mtrapped = 0;
	fill_pit(mtmp->mx, mtmp->my);
	reward_untrap(ttmp, mtmp);
	return 1;
}

int
untrap(force)
boolean force;
{
	register struct obj *otmp;
	register boolean confused = (Confusion > 0 || Hallucination > 0);
	register int x,y;
	int ch;
	struct trap *ttmp;
	struct monst *mtmp;
	boolean trap_skipped = FALSE;
	boolean box_here = FALSE;
	boolean deal_with_floor_trap = FALSE;
	char the_trap[BUFSZ], qbuf[QBUFSZ];
	int containercnt = 0;

	if(!getdir((char *)0)) return(0);
	x = u.ux + u.dx;
	y = u.uy + u.dy;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere) {
		if(Is_box(otmp) && !u.dx && !u.dy) {
			box_here = TRUE;
			containercnt++;
			if (containercnt > 1) break;
		}
	}

	if ((ttmp = t_at(x,y)) && ttmp->tseen) {
		deal_with_floor_trap = TRUE;
#ifndef JP
		Strcpy(the_trap, the(defsyms[trap_to_defsym(ttmp->ttyp)].explanation));
#endif /*JP*/
		if (box_here) {
			if (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) {
#ifndef JP
			    You_cant("do much about %s%s.",
					the_trap, u.utrap ?
					" that you're stuck in" :
					" while standing on the edge of it");
#else
			    You("%s?????????????????????B",
				u.utrap ?
				"??????????????????????????????????" :
				"????????????????????????????");
#endif /*JP*/
			    trap_skipped = TRUE;
			    deal_with_floor_trap = FALSE;
			} else {
#ifndef JP
			    Sprintf(qbuf, "There %s and %s here. %s %s?",
				(containercnt == 1) ? "is a container" : "are containers",
				an(defsyms[trap_to_defsym(ttmp->ttyp)].explanation),
				ttmp->ttyp == WEB ? "Remove" : "Disarm", the_trap);
#else
			    Sprintf(qbuf, "????????%s????%s???????B%s??%s???????H",
				(containercnt == 1) ? "" : "??????????",
				defsyms[trap_to_defsym(ttmp->ttyp)].explanation,
				defsyms[trap_to_defsym(ttmp->ttyp)].explanation,
				ttmp->ttyp == WEB ? "????????" : "??????");
#endif /*JP*/
			    switch (ynq(qbuf)) {
				case 'q': return(0);
				case 'n': trap_skipped = TRUE;
					  deal_with_floor_trap = FALSE;
					  break;
			    }
			}
		}
		if (deal_with_floor_trap) {
		    if (u.utrap) {
#ifndef JP
			You("cannot deal with %s while trapped%s!", the_trap,
				(x == u.ux && y == u.uy) ? " in it" : "");
#else
			pline("?????????????????????A%s???????????????I",
				defsyms[trap_to_defsym(ttmp->ttyp)].explanation);
#endif /*JP*/
			return 1;
		    }
		    switch(ttmp->ttyp) {
			case BEAR_TRAP:
			case WEB:
				return disarm_holdingtrap(ttmp);
			case LANDMINE:
				return disarm_landmine(ttmp);
			case SQKY_BOARD:
				return disarm_squeaky_board(ttmp);
			case DART_TRAP:
				return disarm_shooting_trap(ttmp, DART);
			case ARROW_TRAP:
				return disarm_shooting_trap(ttmp, ARROW);
			case PIT:
			case SPIKED_PIT:
				if (!u.dx && !u.dy) {
				    You(E_J("are already on the edge of the pit.",
					    "?????????????????????????B"));
				    return 0;
				}
				if (!(mtmp = m_at(x,y))) {
				    pline(E_J("Try filling the pit instead.",
					      "?????????A???????????????????????B"));
				    return 0;
				}
				return help_monster_out(mtmp, ttmp);
			default:
#ifndef JP
				You("cannot disable %s trap.", (u.dx || u.dy) ? "that" : "this");
#else
				You("%s?????????????????????????????B", (u.dx || u.dy) ? "??" : "??");
#endif /*JP*/
				return 0;
		    }
		}
	} /* end if */

	if(!u.dx && !u.dy) {
	    for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(Is_box(otmp)) {
#ifdef STEED
		    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
			You(E_J("aren't skilled enough to reach from %s.",
				"%s???????????????????????????n???K?n???????????B"),
				mon_nam(u.usteed));
			return(0);
		    }
#endif
		    if (otmp->otrapped && otmp->rknown) {
#ifndef JP
			Sprintf(qbuf, "There is %s with %s trap here. Disarm the trap?",
				xname(otmp), an(chest_trap_name(otmp)));
#else
			Sprintf(qbuf, "????????%s?????d?|????????%s???????B?????????????????H",
				chest_trap_name(otmp), xname(otmp));
#endif /*JP*/
			switch (ynq(qbuf)) {
			    case 'q': return(0);
			    case 'n': continue;
			}
			goto chest_trap_known;
		    }
#ifndef JP
		    Sprintf(qbuf, "There is %s here. Check it for traps?",
			safe_qbuf("", sizeof("There is  here. Check it for traps?"),
				doname(otmp), an(simple_typename(otmp->otyp)), "a box"));
#else
		    Sprintf(qbuf, "????????%s???????B???????????????H",
			safe_qbuf("", sizeof("???????????????B???????????????H"),
				doname(otmp), simple_typename(otmp->otyp), "??"));
#endif /*JP*/
		    switch (ynq(qbuf)) {
			case 'q': return(0);
			case 'n': continue;
		    }

		    if((otmp->otrapped && (force || otmp->rknown ||
			(!confused && rn2(MAXULEV + 1 - u.ulevel) < 10)))
		       || (!force && confused && !rn2(3))) {
		        if (otmp->rknown) {
#ifndef JP
			    pline("%s trap is on %s.",
				  An(chest_trap_name(otmp)),
				  the(xname(otmp)));
#else
			    pline("%s????%s?????d?|?????????????B",
				  xname(otmp), chest_trap_name(otmp));
#endif /*JP*/
			} else {
#ifndef JP
			    You("find %s trap on %s!",
				an(chest_trap_name(confused ? 0 : otmp)),
				the(xname(otmp)));
#else
			    pline("%s???d?|????????????????%s???????????I",
				xname(otmp), chest_trap_name(confused ? 0 : otmp));
#endif /*JP*/
			    if (!confused) {
				exercise(A_WIS, TRUE);
				otmp->rknown = 1;
			    }
			}

			switch (ynq(E_J("Disarm it?","?????????????H"))) {
			    case 'q': return(1);
			    case 'n': trap_skipped = TRUE;  continue;
			}
chest_trap_known:
			if(otmp->otrapped) {
			    exercise(A_DEX, TRUE);
			    ch = ACURR(A_DEX) + u.ulevel;
			    if (Role_if(PM_ROGUE)) ch *= 2;
			    if(!force && (confused || Fumbling ||
				rnd(75+level_difficulty()/2) > ch)) {
				(void) chest_trap(otmp, FINGER, TRUE);
			    } else {
				You(E_J("disarm it!","?????????????I"));
				otmp->otrapped = 0;
			    }
			} else pline(E_J("That %s was not trapped.",
					 "????%s?????????d?|???????????????????B"),
					xname(otmp));
			return(1);
		    } else if (otmp->otrapped && !force && (confused || Fumbling ||
				!rn2((ACURR(A_DEX)+(Role_if(PM_ROGUE) ? 10 : 0))*2))) {
			(void) chest_trap(otmp, FINGER, FALSE);
			return(1);
		    } else {
#ifndef JP
			You("find no traps on %s.", the(xname(otmp)));
#else
			pline("%s?????????????????????B", xname(otmp));
#endif /*JP*/
			return(1);
		    }
		}

#ifndef JP
	    You(trap_skipped ? "find no other traps here."
			     : "know of no traps here.");
#else
	    pline(trap_skipped ? "?????????????????????B"
			     : "???????????????????????????B");
#endif /*JP*/
	    return(0);
	}

	if ((mtmp = m_at(x,y))				&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp);
	    return(1);
	}

	if (!IS_DOOR(levl[x][y].typ)) {
	    if ((ttmp = t_at(x,y)) && ttmp->tseen)
		You(E_J("cannot disable that trap.","???????????????????????????????B"));
	    else
		E_J(You("know of no traps there."),pline("???????????????????????????B"));
	    return(0);
	}

	switch (levl[x][y].doormask) {
	    case D_NODOOR:
#ifndef JP
		You("%s no door there.", Blind ? "feel" : "see");
#else
		pline("????????????????%s?B", Blind ? "??????" : "");
#endif /*JP*/
		return(0);
	    case D_ISOPEN:
		pline(E_J("This door is safely open.","???????????S???J?????????B"));
		return(0);
	    case D_BROKEN:
		pline(E_J("This door is broken.","?????????????????????B"));
		return(0);
	}

	if ((levl[x][y].doormask & D_TRAPPED
	     && (force ||
		 (!confused && rn2(MAXULEV - u.ulevel + 11) < 10)))
	    || (!force && confused && !rn2(3))) {
		You(E_J("find a trap on the door!","?????d?|?????????????????????I"));
		exercise(A_WIS, TRUE);
		if (ynq(E_J("Disarm it?","?????????????H")) != 'y') return(1);
		if (levl[x][y].doormask & D_TRAPPED) {
		    ch = 15 + (Role_if(PM_ROGUE) ? u.ulevel*3 : u.ulevel);
		    exercise(A_DEX, TRUE);
		    if(!force && (confused || Fumbling ||
				     rnd(75+level_difficulty()/2) > ch)) {
			You(E_J("set it off!","???????????????????????I"));
			b_trapped(E_J("door","??"), FINGER);
			levl[x][y].doormask = D_NODOOR;
			unblock_point(x, y);
			newsym(x, y);
			/* (probably ought to charge for this damage...) */
			if (*in_rooms(x, y, SHOPBASE)) add_damage(x, y, 0L);
		    } else {
			You(E_J("disarm it!","?????????????I"));
			levl[x][y].doormask &= ~D_TRAPPED;
		    }
		} else pline(E_J("This door was not trapped.",
				 "???????????????d?|???????????????B"));
		return(1);
	} else {
		E_J(You("find no traps on the door."),
		    pline("???????????????????????????B"));
		return(1);
	}
}
#endif /* OVL2 */
#ifdef OVLB

/* only called when the player is doing something to the chest directly */
boolean
chest_trap(obj, bodypart, disarm)
register struct obj *obj;
register int bodypart;
boolean disarm;
{
	register struct obj *otmp = obj, *otmp2;
	char	buf[80];
	const char *msg;
	coord cc;
	int cttyp = otmp->corpsenm;
	int tele = (bodypart < 0);
	struct monst *mtmp;

	if (get_obj_location(obj, &cc.x, &cc.y, 0))	/* might be carried */
	    obj->ox = cc.x,  obj->oy = cc.y;

	otmp->otrapped = 0;	/* trap is one-shot; clear flag first in case
				   chest kills you and ends up in bones file */
	if (tele) {
#ifndef JP
	    You("trigger a trap on the box!");
#else
	    pline("?????d?|?????????????N???????I");
#endif /*JP*/
	    mtmp = m_at(obj->ox, obj->oy);
	} else {
#ifndef JP
	    You(disarm ? "set it off!" : "trigger a trap!");
#else
	    You(disarm ? "???????????????????????I" : "?????????????????I");
#endif /*JP*/
	    display_nhwindow(WIN_MESSAGE, FALSE);
	}
	if (!tele && Luck > -13 && rn2(13+Luck) > 10) {	/* saved by luck */
	    /* trap went off, but good luck prevents damage */
	    switch (cttyp) {
		case BOXTRAP_EXPLODING:
		    msg = E_J("explosive charge is a dud",
			      "??????????????????");
		    break;
		case BOXTRAP_ELECTROCUTION:
		    msg = E_J("electric charge is grounded",
			      "?d?????A?[?X??????");
		    break;
		case BOXTRAP_FIRE:
		    msg = E_J("flame fizzles out",
			      "???????X??????????????????");
		    break;
		case BOXTRAP_POISON_NEEDLE:
		    msg = E_J("poisoned needle misses",
			      "???j???O????");
		    break;
		case BOXTRAP_HALLU_GAS:
		case BOXTRAP_GAS_BOMB:
		    msg = E_J("gas cloud blows away",
			      "?K?X???_????????????????");
		    break;
		case BOXTRAP_PARALYZER:
		    msg = E_J("paralysis needle misses",
			      "?????????j???O????");
		    break;
		case BOXTRAP_ALARM:
		    msg = E_J("alarm doesn't sound",
			      "?x????????????????");
		    break;
		default:  impossible("chest disarm bug");  msg = (char *)0;
			  break;
	    }
	    if (msg) pline(E_J("But luckily the %s!","?????K?^?????????A%s?I"), msg);
	} else {
	    switch(cttyp) {

		case BOXTRAP_EXPLODING: {
			  struct monst *shkp = 0;
			  long loss = 0L;
			  boolean costly, insider;
			  register xchar ox = obj->ox, oy = obj->oy;
			  int cnt = 0;

			  /* the obj location need not be that of player */
			  costly = (costly_spot(ox, oy) &&
				   (shkp = shop_keeper(*in_rooms(ox, oy,
				    SHOPBASE))) != (struct monst *)0);
			  insider = (*u.ushops && inside_shop(u.ux, u.uy) &&
				    *in_rooms(ox, oy, SHOPBASE) == *u.ushops);

#ifndef JP
			  pline("%s!", Tobjnam(obj, "explode"));
			  Sprintf(buf, "exploding %s", xname(obj));
#else
			  pline("%s???????????I", xname(obj));
			  Sprintf(buf, "%s????????", xname(obj));
#endif /*JP*/

			  /* temporalily place the contents to the floor */
			  if (Has_contents(obj)) {
			      while (obj->cobj) {
				  otmp2 = obj->cobj;
				  obj_extract_self(otmp2);
				  place_object(otmp2, u.ux, u.uy);
				  stackobj(otmp2);
			      }
			  }

			  if(costly)
			      loss += stolen_value(obj, ox, oy,
						(boolean)shkp->mpeaceful, TRUE);

			  /* we're about to delete all things at this location,
			   * which could include the ball & chain.
			   * If we attempt to call unpunish() in the
			   * for-loop below we can end up with otmp2
			   * being invalid once the chain is gone.
			   * Deal with ball & chain right now instead.
			   */
			  if (Punished && !carried(uball) &&
				((uchain->ox == u.ux && uchain->oy == u.uy) ||
				 (uball->ox == u.ux && uball->oy == u.uy)))
				unpunish();

			  for(otmp = level.objects[ox][oy];
							otmp; otmp = otmp2) {
			      otmp2 = otmp->nexthere;
			      if (otmp != obj &&
				  obj_resists(otmp, 25, 95)) continue;
			      if(costly)
				  loss += stolen_value(otmp, otmp->ox,
					  otmp->oy, (boolean)shkp->mpeaceful,
					  TRUE);
			      if (otmp != obj) cnt++;
			      delobj(otmp);
			  }
			  wake_nearby();
			  if (!tele) {
			      losehp(d(6,6), buf, KILLED_BY_AN);
			      exercise(A_STR, FALSE);
			  } else {
			      if (mtmp) {
				  pline(E_J("%s is caught in the explosion!",
					    "%s?????????????????????I"), Monnam(mtmp));
				  mlosehp(mtmp, d(6,6));
				  if (mtmp->mhp < 0) {
				      xkilled(mtmp, 0);
				      /* maybe the shopkeeper is killed...  */
				      costly = (costly_spot(ox, oy) &&
					       (shkp = shop_keeper(*in_rooms(ox, oy,
						SHOPBASE))) != (struct monst *)0);
				  } else wakeup(mtmp);
			      }
			  }
			  if(costly && loss) {
			      if(insider)
			      You(E_J("owe %ld %s for objects destroyed.",
				      "???????i?? %ld%s?????????????????B"),
							loss, currency(loss));
			      else {
				  You(E_J("caused %ld %s worth of damage!",
					  "%ld%s?????????????Q???????N???????I"),
							loss, currency(loss));
				  make_angry_shk(shkp, ox, oy);
			      }
			  } else if (cnt)
#ifndef JP
			      pline_The("object%s around the box %s destroyed by the explosion!",
				    (cnt > 1) ? "s" : "", (cnt > 1) ? "are" : "is");
#else
			      pline("?????????????????i?????????????A?j?????????I");
#endif /*JP*/
			  (void)scatter(ox, oy, 4,
					MAY_HIT | MAY_FRACTURE | VIS_EFFECTS,
					(struct obj *)0);

			  return TRUE;
			}

		case BOXTRAP_GAS_BOMB:
#ifndef JP
			pline("A cloud of noxious gas billows from %s.",
							the(xname(obj)));
//			poisoned("gas cloud", A_STR, "cloud of poison gas",15);
#else
			pline("%s???????K?X???????o?????B", xname(obj));
//			poisoned("?K?X", A_STR, "???K?X???z??????????",15);
#endif /*JP*/
			(void) create_stinking_cloud(u.ux, u.uy, 3, 8, TRUE);
			exercise(A_CON, FALSE);
			break;

		case BOXTRAP_POISON_NEEDLE:
			if (!tele) {
#ifndef JP
			    You_feel("a needle prick your %s.",body_part(bodypart));
			    poisoned("needle", A_CON, "poisoned needle",10);
#else
			    pline("?j??????????%s???h???????????B",body_part(bodypart));
			    poisoned("?j", A_CON, "???j???h????????????",10);
#endif /*JP*/
			    exercise(A_CON, FALSE);
			}
			break;

		case BOXTRAP_FIRE:
			dofiretrap(obj);
			break;

		case BOXTRAP_ELECTROCUTION: {
			int dmg;

			You(E_J("are jolted by a surge of electricity!",
				"???????d?C???????????????I"));
			if(is_full_resist(SHOCK_RES)) {
			    shieldeff(u.ux, u.uy);
			    You(E_J("don't seem to be affected.",
				    "?e???????????????????????B"));
			    dmg = 0;
			} else if (Shock_resistance)  {
			    shieldeff(u.ux, u.uy);
			    You(E_J("resist the shock.",
				    "?d???????????????B"));
			    dmg = rnd(4);
			} else {
			    dmg = d(4, 4);
			    destroy_item(RING_CLASS, AD_ELEC);
			    destroy_item(WAND_CLASS, AD_ELEC);
			}
			if (dmg) losehp(dmg, E_J("electric shock","?d?C?V???b?N??"), KILLED_BY_AN);
			break;
		      }

		case BOXTRAP_PARALYZER:
			if (!Free_action) {
			    pline(E_J("Suddenly you are frozen in place!",
				      "???R?A???????????????????????I"));
			    nomul(-d(5, 6));
			    exercise(A_DEX, FALSE);
			    nomovemsg = You_can_move_again;
			} else You(E_J("momentarily stiffen.","???u?????????B"));
			break;

		case BOXTRAP_HALLU_GAS:
#ifndef JP
			pline("A cloud of %s gas billows from %s.",
				Blind ? blindgas[rn2(SIZE(blindgas))] :
				rndcolor(), the(xname(obj)));
#else
			pline("%s????%s?????????o?????B",
				Blind ? blindgas[rn2(SIZE(blindgas))] :
				rndcolor(), xname(obj));
#endif /*JP*/
			if(!Stunned) {
			    if (Hallucination)
				pline(E_J("What a groovy feeling!",
					  "?????C?C?C?????I"));
			    else if (Blind)
#ifndef JP
				You("%s and get dizzy...",
				    stagger(youmonst.data, "stagger"));
#else
				You("%s?A???????????c?B",
				    stagger(youmonst.data, "????????"));
#endif /*JP*/
			    else
#ifndef JP
				You("%s and your vision blurs...",
				    stagger(youmonst.data, "stagger"));
#else
				You("%s?A???E???????????c?B",
				    stagger(youmonst.data, "????????"));
#endif /*JP*/
			}
			make_stunned(HStun + rn1(7, 16),FALSE);
			(void) make_hallucinated(HHallucination + rn1(5, 16),FALSE,0L);
			break;

		case BOXTRAP_ALARM: {
			struct monst *mtmp;
			pline(E_J("An alarm sounds!","?x???????????????I"));
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			    if (!DEADMONSTER(mtmp) && mtmp->msleeping) mtmp->msleeping = 0;
			break;
		}

		default: impossible("bad chest trap");
			break;
	    }
	    bot();			/* to get immediate botl re-display */
	}

	return FALSE;
}

/* This must be consistent with the definiton in trap.h */
static struct chest_trap_info {
	char	*nam;
	int	minlev;
} chest_trap_infotbl[BOXTRAP_MAXNUM+1] = {
    { E_J("weird",		"??????"),	 0 },
    { E_J("poison needle",	"???j??"),	 0 },
    { E_J("hallucinogenic gas",	"???o?K?X??"),	 0 },
    { E_J("paralyzer",		"????????"),	 3 },
    { E_J("fire",		"????"),	 5 },
    { E_J("electrocution",	"?d?C???Y??"),	 8 },
    { E_J("gas bomb",		"?K?X???e??"),	 5 },
    { E_J("exploding box",	"??????"),	12 },
    { E_J("alarm",		"?x????"),	 5 },
};

int
wiz_select_chest_trap()
{
	char buf[BUFSZ];
	anything any;
	winid win;
	menu_item *selected;
	int i, n;

	any.a_void = 0;
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	for (i=1; i<=BOXTRAP_MAXNUM; i++) {
	    any.a_int = i;
	    Sprintf(buf, E_J("%s trap", "%s??"), chest_trap_infotbl[i].nam);
	    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
		     buf, MENU_UNSELECTED);
	}
	end_menu(win, "Which type do you trap the box?");
	n = select_menu(win, PICK_ONE, &selected);
	destroy_nhwindow(win);
	if (n > 0) {
	    n = selected[0].item.a_int;
	    free((genericptr_t) selected);
	}
	return n;
}

char *
chest_trap_name(otmp)
struct obj *otmp;
{
	int i = 0;
	if (!otmp)
	    i = rnd(BOXTRAP_MAXNUM);
	else if (otmp->otrapped && otmp->corpsenm)
	    i = otmp->corpsenm;
	if (i > BOXTRAP_MAXNUM) {
	    impossible("bad chest trap");
	    i = 0;
	}
	return chest_trap_infotbl[i].nam;
}

void
trap_chest(otmp)
struct obj *otmp;
{
	int tryct, t, d;
	if (!Is_box(otmp)) return;
	d = depth(&u.uz);
	for (tryct = 3; tryct; tryct--) {
	    t = rnd(BOXTRAP_MAXNUM);
	    if (chest_trap_infotbl[t].minlev <= d) break;
	}
	otmp->corpsenm = t;
}


#endif /* OVLB */
#ifdef OVL0

struct trap *
t_at(x,y)
register int x, y;
{
	register struct trap *trap = ftrap;
	while(trap) {
		if(trap->tx == x && trap->ty == y) return(trap);
		trap = trap->ntrap;
	}
	return((struct trap *)0);
}

#endif /* OVL0 */
#ifdef OVLB

void
deltrap(trap)
register struct trap *trap;
{
	register struct trap *ttmp;

	if(trap == ftrap)
		ftrap = ftrap->ntrap;
	else {
		for(ttmp = ftrap; ttmp->ntrap != trap; ttmp = ttmp->ntrap) ;
		ttmp->ntrap = trap->ntrap;
	}
	dealloc_trap(trap);
}

boolean
delfloortrap(ttmp)
register struct trap *ttmp;
{
	/* Destroy a trap that emanates from the floor. */
	/* some of these are arbitrary -dlc */
	if (ttmp && ((ttmp->ttyp == SQKY_BOARD) ||
		     (ttmp->ttyp == BEAR_TRAP) ||
		     (ttmp->ttyp == LANDMINE) ||
		     (ttmp->ttyp == FIRE_TRAP) ||
		     (ttmp->ttyp == PIT) ||
		     (ttmp->ttyp == SPIKED_PIT) ||
		     (ttmp->ttyp == HOLE) ||
		     (ttmp->ttyp == TRAPDOOR) ||
		     (ttmp->ttyp == TELEP_TRAP) ||
		     (ttmp->ttyp == LEVEL_TELEP) ||
		     (ttmp->ttyp == WEB) ||
		     (ttmp->ttyp == MAGIC_TRAP) ||
		     (ttmp->ttyp == ANTI_MAGIC))) {
	    register struct monst *mtmp;

	    if (ttmp->tx == u.ux && ttmp->ty == u.uy) {
		u.utrap = 0;
		u.utraptype = 0;
	    } else if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
		mtmp->mtrapped = 0;
	    }
	    deltrap(ttmp);
	    return TRUE;
	} else
	    return FALSE;
}

/* used for doors (also tins).  can be used for anything else that opens. */
void
b_trapped(item, bodypart)
register const char *item;
register int bodypart;
{
	register int lvl = level_difficulty();
	int dmg = rnd(5 + (lvl < 5 ? lvl : 2+lvl/2));

#ifndef JP
	pline("KABOOM!!  %s was booby-trapped!", The(item));
#else
	pline("?h?J?[??!! %s?????u?[?r?[?g???b?v???d?|?????????????I", item);
#endif /*JP*/
	wake_nearby();
	losehp(dmg, E_J("explosion","??????????????????"), KILLED_BY_AN);
	exercise(A_STR, FALSE);
	if (bodypart) exercise(A_CON, FALSE);
	make_stunned(HStun + dmg, TRUE);
}

/* Monster is hit by trap. */
/* Note: doesn't work if both obj and d_override are null */
STATIC_OVL boolean
thitm(tlev, mon, obj, d_override, nocorpse)
int tlev;
struct monst *mon;
struct obj *obj;
int d_override;
boolean nocorpse;
{
	int strike;
	boolean trapkilled = FALSE;

	if (d_override) strike = 1;
	else if (obj) strike = (find_mac(mon) + tlev + obj->spe <= rnd(20));
	else strike = (find_mac(mon) + tlev <= rnd(20));

	/* Actually more accurate than thitu, which doesn't take
	 * obj->spe into account.
	 */
	if(!strike) {
		if (obj && cansee(mon->mx, mon->my))
#ifndef JP
		    pline("%s is almost hit by %s!", Monnam(mon), doname(obj));
#else
		    pline("%s??%s???????????I", doname(obj), mon_nam(mon));
#endif /*JP*/
	} else {
		int dam = 1;

		if (obj && cansee(mon->mx, mon->my))
#ifndef JP
			pline("%s is hit by %s!", Monnam(mon), doname(obj));
#else
			pline("%s??%s???????????I", doname(obj), mon_nam(mon));
#endif /*JP*/
		if (d_override) dam = d_override;
		else if (obj) {
			dam = dmgval(obj, mon);
			if (dam < 1) dam = 1;
		}
		if (mlosehp(mon, dam) <= 0) {
			int xx = mon->mx;
			int yy = mon->my;

			monkilled(mon, "", nocorpse ? -AD_RBRE : AD_PHYS);
			if (mon->mhp <= 0) {
				newsym(xx, yy);
				trapkilled = TRUE;
			}
		}
	}
	if (obj && (!strike || d_override)) {
		place_object(obj, mon->mx, mon->my);
		stackobj(obj);
	} else if (obj) dealloc_obj(obj);

	return trapkilled;
}

boolean
unconscious()
{
#ifndef JP
	return((boolean)(multi < 0 && (!nomovemsg ||
		u.usleep ||
		!strncmp(nomovemsg,"You regain con", 14) ||
		!strncmp(nomovemsg,"You are consci", 14))));
#else
	return((boolean)(multi < 0 && (!nomovemsg ||
		u.usleep ||
		!strncmp(nomovemsg,"??????????????", 14))));
#endif /*JP*/
}

static const char lava_killer[] = E_J("molten lava","???????????n?????M??");

boolean
lava_effects()
{
    register struct obj *obj, *obj2;
    int dmg;
    boolean usurvive;

    burn_away_slime();
    if (likes_lava(youmonst.data)) return FALSE;

    if (Wwalking && u.utrap && u.utraptype == TT_LAVA) {
	You(E_J("slowly rise up above the lava.",
		"???????????n?????\???????????????????B"));
	u.utrap = 0;
	u.utraptype = 0;
    }

    if (!Fire_resistance) {
	if(Wwalking) {
	    dmg = d(6,6);
	    pline_The(E_J("lava here burns you!","?n?????????????????s?????I"));
	    if(dmg < u.uhp) {
		losehp(dmg, lava_killer, KILLED_BY);
		goto burn_stuff;
	    }
	} else
	    You(E_J("fall into the lava!","?n?????????????????I"));

	usurvive = Lifesaved || discover;
#ifdef WIZARD
	if (wizard) usurvive = TRUE;
#endif
	for(obj = invent; obj; obj = obj2) {
	    obj2 = obj->nobj;
	    if(is_organic(obj) && !obj->oerodeproof) {
		if(obj->owornmask) {
		    if (usurvive)
#ifndef JP
			Your("%s into flame!", aobjnam(obj, "burst"));
#else
			Your("%s???R???s?????I", xname(obj));
#endif /*JP*/
		}
		useupall(obj);
	    }
	}

	/* s/he died... */
	u.uhp = -1;
	killer_format = KILLED_BY;
	killer = lava_killer;
	You(E_J("burn to a crisp...","?????Y?????????c?B"));
	done(BURNING);
	while (!safe_teleds(TRUE)) {
		pline(E_J("You're still burning.","?????????????R?????????B"));
		done(BURNING);
	}
	You(E_J("find yourself back on solid %s.",
		"????????????????%s?????????????????B"), surface(u.ux, u.uy));
	return(TRUE);
    }

    if (!Wwalking) {
	u.utrap = rn1(4, 4) + (rn1(4, 12) << 8);
	u.utraptype = TT_LAVA;
	You(E_J("sink into the lava, but it only burns slightly!",
		"?n???????????????????????A???????????????R???????????I"));
	if (u.uhp > 1)
	    losehp(1, lava_killer, KILLED_BY);
    }
    /* just want to burn boots, not all armor; destroy_item doesn't work on
       armor anyway */
burn_stuff:
    if(uarmf && !uarmf->oerodeproof && is_organic(uarmf)) {
	/* save uarmf value because Boots_off() sets uarmf to null */
	obj = uarmf;
	Your(E_J("%s bursts into flame!","%s???R???s?????I"), xname(obj));
	(void) Boots_off();
	useup(obj);
    }
    destroy_item(SCROLL_CLASS, AD_FIRE);
    destroy_item(SPBOOK_CLASS, AD_FIRE);
    destroy_item(POTION_CLASS, AD_FIRE);
    destroy_item(TOOL_CLASS, AD_FIRE);
    return(FALSE);
}

boolean
swamp_effects()
{
	int i;
	boolean swampok;

	swampok = (Wwalking || (uarmf && objects[uarmf->otyp].oc_descr_idx == MUD_BOOTS));
	if (!swampok) {
		if (u.utraptype != TT_SWAMP) {
		    if (!Swimming && !Amphibious) {
			You(E_J("step into muddy swamp.",
				"?D?????????????????B"));
			u.utrap = rnd(3);
			u.utraptype = TT_SWAMP;
		    } else
			Norep(E_J("You are swimming in the muddy water.",
				  "?????????D?????????j?????????B"));
		}
	}

	if (!swampok) {
	    if (!rn2(5)) {
		Your(E_J("baggage gets wet.","???????????????????B"));
		water_damage(invent, FALSE, FALSE);
	    } else if (uarmf)
		rust_dmg(uarmf, E_J("boots","?C"), 1/*rust*/, TRUE, &youmonst);
	}

	if (u.umonnum == PM_GREMLIN && rn2(3))
	    (void)split_mon(&youmonst, (struct monst *)0);
	else if (u.umonnum == PM_IRON_GOLEM) {
	    You(E_J("rust!","?K?????I"));
	    i = rnd(6);
	    if (u.mhmax > i) u.mhmax -= i;
	    losehp(i, E_J("rusting away","?K????????????"), KILLED_BY);
	}
	return(FALSE);
}

#endif /* OVLB */

/*trap.c*/
