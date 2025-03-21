/*	SCCS Id: @(#)dog.c	3.4	2002/09/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"

#ifdef OVLB

STATIC_DCL int NDECL(pet_type);

void
initedog(mtmp)
register struct monst *mtmp;
{
	struct edog *ep = EDOG(mtmp);
	if (!ep) {
	    /* Somehow it doesn't have edog structure? */
	    impossible("initedog() without edog?");
	    mtmp->mtame = 0;
	    setmpeaceful(mtmp, TRUE);
	    return;
	}
	mtmp->mtame = is_domestic(mtmp->data) ? 10 : 5;
	setmpeaceful(mtmp, TRUE); /* recalc alignment now that it's tamed */
	mtmp->mleashed = 0;
	mtmp->meating = 0;
	ep->droptime = 0;
	ep->dropdist = 10000;
	ep->apport = 10;
	ep->whistletime = 0;
	ep->hungrytime = 1000 + monstermoves;
	ep->ogoal.x = -1;	/* force error if used before set */
	ep->ogoal.y = -1;
	ep->abuse = 0;
	ep->revivals = 0;
	ep->mhpmax_penalty = 0;
	ep->killed_by_u = 0;
}

STATIC_OVL int
pet_type()
{
	if (urole.petnum != NON_PM)
	    return (urole.petnum);
	else if (preferred_pet == 'c')
	    return (PM_KITTEN);
	else if (preferred_pet == 'd')
	    return (PM_LITTLE_DOG);
	else
	    return (rn2(2) ? PM_KITTEN : PM_LITTLE_DOG);
}

struct monst *
make_familiar(otmp,x,y,quietly)
register struct obj *otmp;
xchar x, y;
boolean quietly;
{
	struct permonst *pm;
	struct monst *mtmp = 0;
	int chance, trycnt = 100;

	do {
	    if (otmp) {	/* figurine; otherwise spell */
		int mndx = otmp->corpsenm;
		pm = &mons[mndx];
		/* activating a figurine provides one way to exceed the
		   maximum number of the target critter created--unless
		   it has a special limit (erinys, Nazgul) */
		if ((mvitals[mndx].mvflags & G_EXTINCT) &&
			mbirth_limit(mndx) != MAXMONNO) {
		    if (!quietly)
			/* have just been given "You <do something with>
			   the figurine and it transforms." message */
			pline(E_J("... into a pile of dust.",
				  "…そして塵の山となった。"));
		    break;	/* mtmp is null */
		}
	    } else if (!rn2(3)) {
		pm = &mons[pet_type()];
	    } else {
		pm = rndmonst();
		if (!pm) {
		  if (!quietly)
#ifndef JP
		    There("seems to be nothing available for a familiar.");
#else
		    pline("味方となってくれる怪物はもういないようだ。");
#endif /*JP*/
		  break;
		}
	    }

	    mtmp = makemon(pm, x, y, MM_EDOG|MM_IGNOREWATER|MM_ADJACENTOK);
	    add_xdat_mon(mtmp, XDAT_EDOG, 0);
	    if (otmp && !mtmp) { /* monster was genocided or square occupied */
	 	if (!quietly)
		   pline_The(E_J("figurine writhes and then shatters into pieces!",
				 "人形はねじくれ、粉々に砕けた！"));
		break;
	    }
	} while (!mtmp && --trycnt > 0);

	if (!mtmp) return (struct monst *)0;

	if (is_pool(mtmp->mx, mtmp->my) && minliquid(mtmp))
		return (struct monst *)0;

	initedog(mtmp);
	mtmp->msleeping = 0;
	if (otmp) { /* figurine; resulting monster might not become a pet */
	    chance = rn2(10);	/* 0==tame, 1==peaceful, 2==hostile */
	    if (chance > 2) chance = otmp->blessed ? 0 : !otmp->cursed ? 1 : 2;
	    /* 0,1,2:  b=80%,10,10; nc=10%,80,10; c=10%,10,80 */
	    if (chance > 0) {
		mtmp->mtame = 0;	/* not tame after all */
		if (chance == 2) { /* hostile (cursed figurine) */
		    if (!quietly)
		       You(E_J("get a bad feeling about this.",
			       "何か嫌な予感がした。"));
		    setmpeaceful(mtmp, FALSE);
		}
	    }
	    /* if figurine has been named, give same name to the monster */
	    if (otmp->has_name)
		mtmp = christen_monst(mtmp, ONAME(otmp));
	}
	set_malign(mtmp); /* more alignment changes */
	newsym(mtmp->mx, mtmp->my);

	/* must wield weapon immediately since pets will otherwise drop it */
	if (mtmp->mtame && attacktype(mtmp->data, AT_WEAP)) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp);
	}
	return mtmp;
}

struct monst *
makedog()
{
	register struct monst *mtmp;
#ifdef STEED
	register struct obj *otmp;
#endif
	const char *petname;
	int   pettype;
	static int petname_used = 0;

	if (preferred_pet == 'n') return((struct monst *) 0);

	pettype = pet_type();
	if (pettype == PM_LITTLE_DOG)
		petname = dogname;
	else if (pettype == PM_PONY)
		petname = horsename;
	else
		petname = catname;

	/* default pet names */
	if (!*petname && pettype == PM_LITTLE_DOG) {
	    /* All of these names were for dogs. */
	    if(Role_if(PM_CAVEMAN))   petname = E_J("Slasher","スラッシャー");	/* The Warrior */
	    if(Role_if(PM_SAMURAI))   petname = E_J("Hachi",  "ハチ公");	/* Shibuya Station */
	    if(Role_if(PM_BARBARIAN)) petname = E_J("Idefix", "イデフィクス");	/* Obelix */
	    if(Role_if(PM_RANGER))    petname = E_J("Sirius", "シリウス");	/* Orion's dog */
	}

	mtmp = makemon(&mons[pettype], u.ux, u.uy, MM_EDOG);
	if(!mtmp) return((struct monst *) 0); /* pets were genocided */

	add_xdat_mon(mtmp, XDAT_EDOG, 0);
#ifdef STEED
	/* Horses already wear a saddle */
	if (pettype == PM_PONY && !!(otmp = mksobj(SADDLE, TRUE, FALSE))) {
	    if (mpickobj(mtmp, otmp))
		panic("merged saddle?");
	    mtmp->misc_worn_check |= W_SADDLE;
	    otmp->dknown = otmp->bknown = otmp->rknown = 1;
	    otmp->owornmask = W_SADDLE;
	    otmp->leashmon = mtmp->m_id;
	    update_mon_intrinsics(mtmp, otmp, TRUE, TRUE);
	}
#endif

	if (!petname_used++ && *petname)
		mtmp = christen_monst(mtmp, petname);

	initedog(mtmp);
	return(mtmp);
}

/* record `last move time' for all monsters prior to level save so that
   mon_arrive() can catch up for lost time when they're restored later */
void
update_mlstmv()
{
	struct monst *mon;

	/* monst->mlstmv used to be updated every time `monst' actually moved,
	   but that is no longer the case so we just do a blanket assignment */
	for (mon = fmon; mon; mon = mon->nmon)
	    if (!DEADMONSTER(mon)) mon->mlstmv = monstermoves;
}

void
losedogs()
{
	register struct monst *mtmp, *mtmp0 = 0, *mtmp2;

	while ((mtmp = mydogs) != 0) {
		mydogs = mtmp->nmon;
		mon_arrive(mtmp, TRUE);
	}

	for(mtmp = migrating_mons; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (mtmp->mux == u.uz.dnum && mtmp->muy == u.uz.dlevel) {
		    if(mtmp == migrating_mons)
			migrating_mons = mtmp->nmon;
		    else
			mtmp0->nmon = mtmp->nmon;
		    mon_arrive(mtmp, FALSE);
		} else
		    mtmp0 = mtmp;
	}
}

/* called from resurrect() in addition to losedogs() */
void
mon_arrive(mtmp, with_you)
struct monst *mtmp;
boolean with_you;
{
	struct trap *t;
	xchar xlocale, ylocale, xyloc, xyflags, wander;
        int num_segs;
	boolean isdead = (mtmp->mhp <= 0);

	if (isdead) mtmp->mhp = 1;	/* maybe mon is smashed into trapdoor or lev teleporter... */

	mtmp->nmon = fmon;
	fmon = mtmp;
	if (mtmp->isshk)
	    set_residency(mtmp, FALSE);

	num_segs = mtmp->wormno;
	/* baby long worms have no tail so don't use is_longworm() */
	if ((mtmp->mnum == PM_LONG_WORM) &&
#ifdef DCC30_BUG
	    (mtmp->wormno = get_wormno(), mtmp->wormno != 0))
#else
	    (mtmp->wormno = get_wormno()) != 0)
#endif
	{
	    initworm(mtmp, num_segs);
	    /* tail segs are not yet initialized or displayed */
	} else mtmp->wormno = 0;

	/* update shape-changers in case protection against
	   them is different now than when the level was saved */
	restore_cham(mtmp);

	/* some monsters might need to do something special upon arrival
	   _after_ the current level has been fully set up; see dochug() */
	mtmp->mstrategy |= STRAT_ARRIVE;

	/* make sure mnexto(rloc_to(set_apparxy())) doesn't use stale data */
	mtmp->mux = u.ux,  mtmp->muy = u.uy;
	xyloc	= mtmp->mtrack[0].x;
	xyflags = mtmp->mtrack[0].y;
	xlocale = mtmp->mtrack[1].x;
	ylocale = mtmp->mtrack[1].y;
	mtmp->mtrack[0].x = mtmp->mtrack[0].y = 0;
	mtmp->mtrack[1].x = mtmp->mtrack[1].y = 0;

#ifdef STEED
	if (mtmp == u.usteed)
	    return;	/* don't place steed on the map */
#endif
#ifdef MONSTEED
	/* Beware! monrider must have appeared before monsteed appears! */
	if (is_mridden(mtmp)) {
	    /* restore the link to mrider from msteed */
	    mtmp->mlink->mlinktyp = MLT_STEED;
	    mtmp->mlink->mlink    = mtmp;
	    mtmp->mx = mtmp->mlink->mx;
	    mtmp->my = mtmp->mlink->my;
	    if (mtmp->mlstmv < monstermoves - 1L) {
		mon_catchup_elapsed_time(mtmp, monstermoves - 1L - mtmp->mlstmv);
		mtmp->mlstmv = monstermoves - 1L;
	    }
	    return;	/* don't place monsteed on the map */
	}
#endif
	if (with_you) {
	    /* When a monster accompanies you, sometimes it will arrive
	       at your intended destination and you'll end up next to
	       that spot.  This code doesn't control the final outcome;
	       goto_level(do.c) decides who ends up at your target spot
	       when there is a monster there too. */
	    if (!MON_AT(u.ux, u.uy) &&
		    !rn2(mtmp->mtame ? 10 : mtmp->mpeaceful ? 5 : 2))
		rloc_to(mtmp, u.ux, u.uy);
	    else
		mnexto(mtmp);
	    return;
	}
	/*
	 * The monster arrived on this level independently of the player.
	 * Its coordinate fields were overloaded for use as flags that
	 * specify its final destination.
	 */

	if (mtmp->mlstmv < monstermoves - 1L) {
	    /* heal monster for time spent in limbo */
	    long nmv = monstermoves - 1L - mtmp->mlstmv;

	    mon_catchup_elapsed_time(mtmp, nmv);
	    mtmp->mlstmv = monstermoves - 1L;

	    /* let monster move a bit on new level (see placement code below) */
	    wander = (xchar) min(nmv, 8);
	} else
	    wander = 0;

	switch (xyloc) {
	 case MIGR_APPROX_XY:	/* {x,y}locale set above */
		break;
	 case MIGR_EXACT_XY:	wander = 0;
		break;
	 case MIGR_NEAR_PLAYER:	xlocale = u.ux,  ylocale = u.uy;
		break;
	 case MIGR_STAIRS_UP:	xlocale = xupstair,  ylocale = yupstair;
		break;
	 case MIGR_STAIRS_DOWN:	xlocale = xdnstair,  ylocale = ydnstair;
		break;
	 case MIGR_LADDER_UP:	xlocale = xupladder,  ylocale = yupladder;
		break;
	 case MIGR_LADDER_DOWN:	xlocale = xdnladder,  ylocale = ydnladder;
		break;
	 case MIGR_SSTAIRS:	xlocale = sstairs.sx,  ylocale = sstairs.sy;
		break;
	 case MIGR_PORTAL:
		if (In_endgame(&u.uz)) {
		    /* there is no arrival portal for endgame levels */
		    /* BUG[?]: for simplicity, this code relies on the fact
		       that we know that the current endgame levels always
		       build upwards and never have any exclusion subregion
		       inside their TELEPORT_REGION settings. */
		    xlocale = rn1(updest.hx - updest.lx + 1, updest.lx);
		    ylocale = rn1(updest.hy - updest.ly + 1, updest.ly);
		    break;
		}
		/* find the arrival portal */
		for (t = ftrap; t; t = t->ntrap)
		    if (t->ttyp == MAGIC_PORTAL) break;
		if (t) {
		    xlocale = t->tx,  ylocale = t->ty;
		    break;
		} else {
		    impossible("mon_arrive: no corresponding portal?");
		} /*FALLTHRU*/
	 default:
	 case MIGR_RANDOM:	xlocale = ylocale = 0;
		    break;
	    }

	if (xlocale && wander) {
	    /* monster moved a bit; pick a nearby location */
	    /* mnearto() deals w/stone, et al */
	    char *r = in_rooms(xlocale, ylocale, 0);
	    if (r && *r) {
		coord c;
		/* somexy() handles irregular rooms */
		if (somexy(&rooms[*r - ROOMOFFSET], &c))
		    xlocale = c.x,  ylocale = c.y;
		else
		    xlocale = ylocale = 0;
	    } else {		/* not in a room */
		int i, j;
		i = max(1, xlocale - wander);
		j = min(COLNO-1, xlocale + wander);
		xlocale = rn1(j - i, i);
		i = max(0, ylocale - wander);
		j = min(ROWNO-1, ylocale + wander);
		ylocale = rn1(j - i, i);
	    }
	}	/* moved a bit */

	mtmp->mx = 0;	/*(already is 0)*/
	mtmp->my = xyflags;
	if (xlocale)
	    (void) mnearto(mtmp, xlocale, ylocale, FALSE);
	else {
	    if (!rloc(mtmp,TRUE)) {
		/*
		 * Failed to place migrating monster,
		 * probably because the level is full.
		 * Dump the monster's cargo and leave the monster dead.
		 */
	    	struct obj *obj, *corpse;
		while ((obj = mtmp->minvent) != 0) {
		    obj_extract_self(obj);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP)
			setmnotwielded(mtmp,obj);
		    obj->owornmask = 0L;
		    if (xlocale && ylocale)
			    place_object(obj, xlocale, ylocale);
		    else {
		    	rloco(obj);
			get_obj_location(obj, &xlocale, &ylocale, 0);
		    }
		}
		corpse = mkcorpstat(CORPSE, (struct monst *)0, mtmp->data,
				xlocale, ylocale, FALSE);

		if (mtmp->mgold) {
		    if (xlocale == 0 && ylocale == 0 && corpse) {
			(void) get_obj_location(corpse, &xlocale, &ylocale, 0);
			(void) mkgold(mtmp->mgold, xlocale, ylocale);
		    }
		    mtmp->mgold = 0L;
		}

#ifdef MONSTEED
		if (is_mriding(mtmp)) {
		    struct monst *mstd;
		    mstd = unlink_mlink(mtmp);
		    unlink_mlink(mstd);
		    mongone(mstd);
		}
#endif
		mongone(mtmp);
	    }
	}

	if (isdead) mondied(mtmp);
}

/* heal monster for time spent elsewhere */
void
mon_catchup_elapsed_time(mtmp, nmv)
struct monst *mtmp;
long nmv;		/* number of moves */
{
	int imv = 0;	/* avoid zillions of casts and lint warnings */

#if defined(DEBUG) || defined(BETA)
	if (nmv < 0L) {			/* crash likely... */
	    panic("catchup from future time?");
	    /*NOTREACHED*/
	    return;
	} else if (nmv == 0L) {		/* safe, but should'nt happen */
	    impossible("catchup from now?");
	} else
#endif
	if (nmv >= LARGEST_INT)		/* paranoia */
	    imv = LARGEST_INT - 1;
	else
	    imv = (int)nmv;

	/* force stop swallowing, to avoid complexity to
	   decide which monster had killed another...  */
	if (is_swallowing(mtmp))
	    mregurgitate(mtmp, FALSE, TRUE);

	/* might stop being afraid, blind or frozen */
	/* set to 1 and allow final decrement in movemon() */
	if (mtmp->mblinded) {
	    if (imv >= (int) mtmp->mblinded) mtmp->mblinded = 1;
	    else mtmp->mblinded -= imv;
	}
	if (mtmp->mfrozen) {
	    if (imv >= (int) mtmp->mfrozen) mtmp->mfrozen = 1;
	    else mtmp->mfrozen -= imv;
	}
	if (mtmp->mfleetim) {
	    if (imv >= (int) mtmp->mfleetim) mtmp->mfleetim = 1;
	    else mtmp->mfleetim -= imv;
	}

	/* might recover from temporary trouble */
	if (mtmp->mtrapped && rn2(imv + 1) > 40/2) mtmp->mtrapped = 0;
	if (mtmp->mconf    && rn2(imv + 1) > 50/2) mtmp->mconf = 0;
	if (mtmp->mstun    && rn2(imv + 1) > 10/2) mtmp->mstun = 0;

	/* might finish eating or be able to use special ability again */
	if (imv > mtmp->meating) mtmp->meating = 0;
	else mtmp->meating -= imv;
	if (imv > mtmp->mspec_used) mtmp->mspec_used = 0;
	else mtmp->mspec_used -= imv;

	/* reduce tameness for every 150 moves you are separated */
	if (mtmp->mtame) {
	    int wilder = (imv + 75) / 150;
	    if (mtmp->mtame > wilder) mtmp->mtame -= wilder;	/* less tame */
	    else if (mtmp->mtame > rn2(wilder)) mtmp->mtame = 0;  /* untame */
	    else mtmp->mtame = mtmp->mpeaceful = 0;		/* hostile! */
	}
	/* check to see if it would have died as a pet; if so, go wild instead
	 * of dying the next time we call dog_move()
	 */
	if (mtmp->mtame && !mtmp->isminion &&
			(carnivorous(mtmp->data) || herbivorous(mtmp->data))) {
	    struct edog *edog = EDOG(mtmp);

	    if ((monstermoves > edog->hungrytime + 500 && mtmp->mhp < 3) ||
		    (monstermoves > edog->hungrytime + 750))
		mtmp->mtame = mtmp->mpeaceful = 0;
	}

	if (!mtmp->mtame && mtmp->mleashed) {
	    /* leashed monsters should always be with hero, consequently
	       never losing any time to be accounted for later */
	    impossible("catching up for leashed monster?");
	    m_unleash(mtmp, FALSE);
	}

	/* recover lost hit points */
	if (!regenerates(mtmp->data)) imv /= 20;
	mlosehp(mtmp, -imv);
}

#endif /* OVLB */
#ifdef OVL2

/* called when you move to another level */
void
keepdogs(pets_only)
boolean pets_only;	/* true for ascension or final escape */
{
	register struct monst *mtmp, *mtmp2;
	register struct obj *obj;
	int num_segs;
	boolean stay_behind;
#ifdef MONSTEED
	struct monst *mtmp3 = 0;
#endif

	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;

	    /* swallowing monster and its victim stay behind */
	    if (is_swallowed(mtmp)) {
		/* let the wizard escaped from the stomach */
		if (mtmp->iswiz) {
		    struct monst *magr;
		    magr = unlink_mlink(mtmp);
		    unlink_mlink(magr);
		    magr->meating = 0;
		    migrate_to_level(mtmp, ledger_no(&u.uz),
				     MIGR_APPROX_XY, (coord *)0);
		}
		continue;
	    }

	    if (DEADMONSTER(mtmp)) continue;
	    if (pets_only && !mtmp->mtame) continue;
#ifdef MONSTEED
	    if (is_mridden(mtmp)) continue; /* msteed is handled by mrider */
#endif

	    if (((monnear(mtmp, u.ux, u.uy) && levl_follower(mtmp)) ||
#ifdef STEED
			(mtmp == u.usteed) ||
#endif
		/* the wiz will level t-port from anywhere to chase
		   the amulet; if you don't have it, will chase you
		   only if in range. -3. */
			(u.uhave.amulet && mtmp->iswiz))
		&& ((!mtmp->msleeping && mtmp->mcanmove)
#ifdef STEED
		    /* eg if level teleport or new trap, steed has no control
		       to avoid following */
		    || (mtmp == u.usteed)
#endif
		    )
		/* monster won't follow if it hasn't noticed you yet */
		&& !(mtmp->mstrategy & STRAT_WAITFORU)) {
		stay_behind = FALSE;
		if (pets_only) {
			;
		} else if (is_swallowing(mtmp)) {
			pline(E_J("%s is still swallowing something.",
				  "%sは何かを飲み込んでいる最中だ。"), Monnam(mtmp));
			stay_behind = TRUE;
		} else if (mtmp->mtame && mtmp->meating) {
			if (canseemon(mtmp))
			    pline(E_J("%s is still eating.",
				      "%sはまだ食べている。"), Monnam(mtmp));
			stay_behind = TRUE;
		} else if (mon_has_amulet(mtmp)
#ifdef MONSTEED
			   || (is_mriding(mtmp) && mon_has_amulet(mtmp->mlink))
#endif
			   ) {
			if (canseemon(mtmp))
			    pline(E_J("%s seems very disoriented for a moment.",
				      "%sは一瞬自分がどこにいるか全く分からなくなったようだ。"),
				Monnam(mtmp));
			stay_behind = TRUE;
		} else if (mtmp->mtame && mtmp->mtrapped) {
			if (canseemon(mtmp))
			    pline(E_J("%s is still trapped.",
				      "%sはまだ罠にかかっている。"), Monnam(mtmp));
			stay_behind = TRUE;
		}
#ifdef STEED
		if (mtmp == u.usteed) stay_behind = FALSE;
#endif
		if (stay_behind) {
			if (mtmp->mleashed) {
#ifndef JP
				pline("%s leash suddenly comes loose.",
					humanoid(mtmp->data)
					    ? (mtmp->female ? "Her" : "His")
					    : "Its");
#else
				pline("%sを繋いでいた引き綱が突然緩んだ。",
					mon_nam(mtmp));
#endif /*JP*/
				m_unleash(mtmp, FALSE);
			}
			continue;
		}
#ifdef MONSTEED
		if (is_mriding(mtmp)) {
			/* Send msteed first, remove the link from mrider to msteed,
			   but keep the link from msteed to mrider.
			   See the comment in migrate_to_level() */
			mtmp3 = mtmp;
			mtmp = unlink_mlink(mtmp3);
		}
monstdloop:
#endif
		if (mtmp->isshk)
			set_residency(mtmp, TRUE);

		if (mtmp->wormno) {
		    register int cnt;
		    /* NOTE: worm is truncated to # segs = max wormno size */
		    cnt = count_wsegs(mtmp);
		    num_segs = min(cnt, MAX_NUM_WORMS - 1);
		    wormgone(mtmp);
		} else num_segs = 0;

		/* set minvent's obj->no_charge to 0 */
		for(obj = mtmp->minvent; obj; obj = obj->nobj) {
		    if (Has_contents(obj))
			picked_container(obj);	/* does the right thing */
		    obj->no_charge = 0;
		}

		relmon(mtmp);
		newsym(mtmp->mx,mtmp->my);
		mtmp->mx = mtmp->my = 0; /* avoid mnexto()/MON_AT() problem */
		mtmp->wormno = num_segs;
		mtmp->mlstmv = monstermoves;
		mtmp->nmon = mydogs;
		mydogs = mtmp;
#ifdef MONSTEED
		if (mtmp3) {
			mtmp = mtmp3;
			mtmp3 = (struct monst *)0;
			goto monstdloop;
		}
#endif
	    } else if (mtmp->iswiz) {
		/* we want to be able to find him when his next resurrection
		   chance comes up, but have him resume his present location
		   if player returns to this level before that time */
		migrate_to_level(mtmp, ledger_no(&u.uz),
				 MIGR_EXACT_XY, (coord *)0);
	    } else if (mtmp->mleashed) {
		/* this can happen if your quest leader ejects you from the
		   "home" level while a leashed pet isn't next to you */
#ifndef JP
		pline("%s leash goes slack.", s_suffix(Monnam(mtmp)));
#else
		pline("%sを繋いでいた引き綱が緩んだ。", mon_nam(mtmp));
#endif /*JP*/
		m_unleash(mtmp, FALSE);
	    }
	}
}

#endif /* OVL2 */
#ifdef OVLB

void
migrate_to_level(mtmp, tolev, xyloc, cc)
	struct monst *mtmp;
	xchar tolev;	/* destination level */
	xchar xyloc;	/* MIGR_xxx destination xy location: */
	coord *cc;	/* optional destination coordinates */
{
	struct obj *obj;
	d_level new_lev;
	xchar xyflags;
	int num_segs = 0;	/* count of worm segments */

#ifdef MONSTEED
	/* mrider should appear earlier than msteed at arriving,
	   so send msteed first
	   because migrate_to_level/mon_arrive works as LIFO */
	if (is_mriding(mtmp)) {
	    migrate_to_level(mtmp->mlink, tolev, xyloc, cc);
	    /* and temporalily unlink the chain to msteed.
	       msteed restores the link again at an arrival.
	       This is because placing mrider breaks the
	       contents of msteed's mx/my/mux/muy... */
	    unlink_mlink(mtmp);
	}
#endif
	/* if mtmp is swallowing another monster,
	   force cancel it to avoid complexity */
	if (is_swallowing(mtmp)) {
	    struct monst *mtmp2;
	    mtmp2 = unlink_mlink(mtmp);
	    unlink_mlink(mtmp2);
	    mtmp->meating = 0;
	    migrate_to_level(mtmp2, tolev, xyloc, cc);
	}

	if (mtmp->isshk)
	    set_residency(mtmp, TRUE);

	if (mtmp->wormno) {
	    register int cnt;
	  /* **** NOTE: worm is truncated to # segs = max wormno size **** */
	    cnt = count_wsegs(mtmp);
	    num_segs = min(cnt, MAX_NUM_WORMS - 1);
	    wormgone(mtmp);
	}

	/* room guard */
	mtmp->mstrategy &= ~STRAT_ROOMGUARD;

	/* set minvent's obj->no_charge to 0 */
	for(obj = mtmp->minvent; obj; obj = obj->nobj) {
	    if (Has_contents(obj))
		picked_container(obj);	/* does the right thing */
	    obj->no_charge = 0;
	}

	if (mtmp->mleashed) {
		mtmp->mtame--;
		m_unleash(mtmp, TRUE);
	}
	relmon(mtmp);
	mtmp->nmon = migrating_mons;
	migrating_mons = mtmp;
	newsym(mtmp->mx,mtmp->my);

	new_lev.dnum = ledger_to_dnum((xchar)tolev);
	new_lev.dlevel = ledger_to_dlev((xchar)tolev);
	/* overload mtmp->[mx,my], mtmp->[mux,muy], and mtmp->mtrack[] as */
	/* destination codes (setup flag bits before altering mx or my) */
	xyflags = (depth(&new_lev) < depth(&u.uz));	/* 1 => up */
	mtmp->wormno = num_segs;
	mtmp->mlstmv = monstermoves;
	mtmp->mtrack[1].x = cc ? cc->x : mtmp->mx;
	mtmp->mtrack[1].y = cc ? cc->y : mtmp->my;
	mtmp->mtrack[0].x = xyloc;
	mtmp->mtrack[0].y = xyflags;
	mtmp->mux = new_lev.dnum;
	mtmp->muy = new_lev.dlevel;
	mtmp->mx = mtmp->my = 0;	/* this implies migration */
}

#endif /* OVLB */
#ifdef OVL1

/* return quality of food; the lower the better */
/* fungi will eat even tainted food */
int
dogfood(mon,obj)
struct monst *mon;
register struct obj *obj;
{
	boolean carni = carnivorous(mon->data);
	boolean herbi = herbivorous(mon->data);
	struct permonst *fptr = &mons[obj->corpsenm];
	boolean starving;

	if (is_quest_artifact(obj) || obj_resists(obj, 0, 95))
	    return (obj->cursed ? TABU : APPORT);

	switch(obj->oclass) {
	case FOOD_CLASS:
	    if (obj->otyp == CORPSE &&
		((touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon))
		 || is_rider(fptr)))
		    return TABU;

	    /* Ghouls only eat old corpses... yum! */
	    if (mon->mnum == PM_GHOUL)
		return (obj->otyp == CORPSE &&
			peek_at_iced_corpse_age(obj) + 50L <= monstermoves) ?
				DOGFOOD : TABU;

	    if (!carni && !herbi)
		    return (obj->cursed ? UNDEF : APPORT);

	    /* a starving pet will eat almost anything */
	    starving = (mon->mtame && !mon->isminion &&
			EDOG(mon)->mhpmax_penalty);

	    switch (obj->otyp) {
		case TRIPE_RATION:
		case MEATBALL:
		case MEAT_RING:
		case MEAT_STICK:
		case HUGE_CHUNK_OF_MEAT:
		    return (carni ? DOGFOOD : MANFOOD);
		case EGG:
		    if (touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon))
			return POISON;
		    return (carni ? CADAVER : MANFOOD);
		case CORPSE:
		   if ((peek_at_iced_corpse_age(obj) + 50L <= monstermoves
					    && obj->corpsenm != PM_LIZARD
					    && obj->corpsenm != PM_LICHEN
					    && mon->data->mlet != S_FUNGUS) ||
			(acidic(&mons[obj->corpsenm]) && !resists_acid(mon)) ||
			(poisonous(&mons[obj->corpsenm]) &&
						!resists_poison(mon)))
			return POISON;
		    else if (vegan(fptr))
			return (herbi ? CADAVER : MANFOOD);
		    else return (carni ? CADAVER : MANFOOD);
		case CLOVE_OF_GARLIC:
		    return (is_undead(mon->data) ? TABU :
			    ((herbi || starving) ? ACCFOOD : MANFOOD));
		case TIN:
		    return (metallivorous(mon->data) ? ACCFOOD : MANFOOD);
		case APPLE:
		case CARROT:
		    return (herbi ? DOGFOOD : starving ? ACCFOOD : MANFOOD);
		case BANANA:
		    return ((mon->data->mlet == S_YETI) ? DOGFOOD :
			    ((herbi || starving) ? ACCFOOD : MANFOOD));
		default:
		    if (starving) return ACCFOOD;
		    return (obj->otyp > SLIME_MOLD ?
			    (carni ? ACCFOOD : MANFOOD) :
			    (herbi ? ACCFOOD : MANFOOD));
	    }
	default:
	    if (obj->otyp == AMULET_OF_STRANGULATION ||
			obj->otyp == RIN_SLOW_DIGESTION)
		return TABU;
	    if (hates_silver(mon->data) &&
		get_material(obj) == SILVER)
		return(TABU);
	    if (herbi && obj->otyp == SHEAF_OF_STRAW)
		return CADAVER;
	    if (mon->mnum == PM_GELATINOUS_CUBE && is_organic(obj))
		return(ACCFOOD);
	    if (metallivorous(mon->data) && is_metallic(obj) && (is_rustprone(obj) || mon->mnum != PM_RUST_MONSTER)) {
		/* Non-rustproofed ferrous based metals are preferred. */
		return((is_rustprone(obj) && !obj->oerodeproof) ? DOGFOOD :
			ACCFOOD);
	    }
	    if(!obj->cursed && obj->oclass != BALL_CLASS &&
						obj->oclass != CHAIN_CLASS)
		return(APPORT);
	    /* fall into next case */
	case ROCK_CLASS:
	    return(UNDEF);
	}
}

#endif /* OVL1 */
#ifdef OVLB

struct monst *
tamedog(mtmp, obj)
register struct monst *mtmp;
register struct obj *obj;
{
	struct edog *ep;
	/* The Wiz, Medusa and the quest nemeses aren't even made peaceful. */
	if (mtmp->iswiz || mtmp->mnum == PM_MEDUSA
				|| (mtmp->data->mflags3 & M3_WANTSARTI))
		return((struct monst *)0);
#ifdef MONSTEED
	/* currently mrider/msteed cannot be tamed...
	   otherwise it goes too complicated */
	if (is_mriding(mtmp) || is_mridden(mtmp))
	    return((struct monst *)0);
#endif
	if (mtmp->isshk && !obj) {
	    make_happy_shk(mtmp, FALSE, FALSE);
	    return((struct monst *)0);
	}

	/* worst case, at least it'll be peaceful. */
	setmpeaceful(mtmp, TRUE);

	if(flags.moonphase == FULL_MOON && night() && rn2(6) && obj
						&& mtmp->data->mlet == S_DOG)
		return((struct monst *)0);

	/* If we cannot tame it, at least it's no longer afraid. */
	mtmp->mflee = 0;
	mtmp->mfleetim = 0;

	/* make grabber let go now, whether it becomes tame or not */
	if (mtmp == u.ustuck) {
	    if (u.uswallow)
		expels(mtmp, mtmp->data, TRUE);
	    else if (!(Upolyd && sticks(youmonst.data)))
		unstuck(mtmp);
	}

	ep = EDOG(mtmp);

	/* feeding it treats makes it tamer */
	if (mtmp->mtame && obj && !mtmp->isminion) {
	    int tasty;

	    if (!ep) {
		impossible("tamedog: A tamed monster has no edog?");
		return (struct monst *)0;
	    }
	    if (mtmp->mcanmove && !mtmp->mconf && !mtmp->meating &&
		((tasty = dogfood(mtmp, obj)) == DOGFOOD ||
		 (tasty <= ACCFOOD && ep->hungrytime <= monstermoves))) {
		/* pet will "catch" and eat this thrown food */
		if (canseemon(mtmp)) {
		    boolean big_corpse = (obj->otyp == CORPSE &&
					  obj->corpsenm >= LOW_PM &&
				mons[obj->corpsenm].msize > mtmp->data->msize);
#ifndef JP
		    pline("%s catches %s%s",
			  Monnam(mtmp), the(xname(obj)),
			  !big_corpse ? "." : ", or vice versa!");
#else
		    pline("%sは%sをキャッチした。%s",
			  mon_nam(mtmp), xname(obj),
			  !big_corpse ? "" : "(いや、逆かな？)");
#endif /*JP*/
		} else if (cansee(mtmp->mx,mtmp->my))
#ifndef JP
		    pline("%s.", Tobjnam(obj, "stop"));
#else
		    pline("%sは止まった。", xname(obj));
#endif /*JP*/
		/* dog_eat expects a floor object */
		place_object(obj, mtmp->mx, mtmp->my);
		(void) dog_eat(mtmp, obj, mtmp->mx, mtmp->my, FALSE);
		/* eating might have killed it, but that doesn't matter here;
		   a non-null result suppresses "miss" message for thrown
		   food and also implies that the object has been deleted */
		return mtmp;
	    } else
		return (struct monst *)0;
	}

	if (mtmp->mtame || !mtmp->mcanmove ||
	    /* monsters with conflicting structures cannot be tamed */
	    mtmp->isshk || mtmp->isgd || mtmp->ispriest || mtmp->isminion ||
	    is_covetous(mtmp->data) || is_human(mtmp->data) ||
	    (is_demon(mtmp->data) && !is_demon(youmonst.data)) ||
	    (obj && dogfood(mtmp, obj) >= MANFOOD)) return (struct monst *)0;

	if (mtmp->m_id == quest_status.leader_m_id)
	    return((struct monst *)0);

	if (!ep) {
	    /* add the pet extension */
	    add_xdat_mon(mtmp, XDAT_EDOG, (struct edog *)0);
	}
	    /* the monster has an obsolete edog structure. just re-use it */

	initedog(mtmp);

	if (obj) {		/* thrown food */
	    /* defer eating until the edog extension has been set up */
	    place_object(obj, mtmp->mx, mtmp->my);	/* put on floor */
	    /* devour the food (might grow into larger, genocided monster) */
	    if (dog_eat(mtmp, obj, mtmp->mx, mtmp->my, TRUE) == 2)
		return mtmp;		/* oops, it died... */
	    /* `obj' is now obsolete */
	}

	newsym(mtmp->mx, mtmp->my);
	if (attacktype(mtmp->data, AT_WEAP)) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp);
	}
	return(mtmp);
}

/*
 * Called during pet revival or pet life-saving.
 * If you killed the pet, it revives wild.
 * If you abused the pet a lot while alive, it revives wild.
 * If you abused the pet at all while alive, it revives untame.
 * If the pet wasn't abused and was very tame, it might revive tame.
 */
void
wary_dog(mtmp, was_dead)
struct monst *mtmp;
boolean was_dead;
{
    struct edog *edog;
    boolean quietly = was_dead;

    mtmp->meating = 0;
    if (!mtmp->mtame) return;
    edog = !mtmp->isminion ? EDOG(mtmp) : 0;

    /* if monster was starving when it died, undo that now */
    if (edog && edog->mhpmax_penalty) {
	mtmp->mhpmax += edog->mhpmax_penalty;
	mtmp->mhp += edog->mhpmax_penalty;	/* heal it */
	edog->mhpmax_penalty = 0;
    }

    if (edog && (edog->killed_by_u == 1 || edog->abuse > 2)) {
	mtmp->mpeaceful = mtmp->mtame = 0;
	if (edog->abuse >= 0 && edog->abuse < 10)
	    if (!rn2(edog->abuse + 1)) mtmp->mpeaceful = 1;
	if(!quietly && cansee(mtmp->mx, mtmp->my)) {
	    if (haseyes(youmonst.data)) {
		if (haseyes(mtmp->data))
#ifndef JP
			pline("%s %s to look you in the %s.",
				Monnam(mtmp),
				mtmp->mpeaceful ? "seems unable" :
					    "refuses",
				body_part(EYE));
#else
			pline("%sはあなたと%sを合わせ%s。",
				mon_nam(mtmp), body_part(EYE),
				mtmp->mpeaceful ? "られないようだ" :
					    "ることを拒んだ");
#endif /*JP*/
		else 
			pline(E_J("%s avoids your gaze.",
				  "%sはあなたの視線を避けた。"),
				Monnam(mtmp));
	    }
	}
    } else {
	/* chance it goes wild anyway - Pet Semetary */
	if (!rn2(mtmp->mtame)) {
	    mtmp->mpeaceful = mtmp->mtame = 0;
	}
    }
    if (!mtmp->mtame) {
#ifdef STEED
	if (u.usteed == mtmp)
	    dismount_steed(DISMOUNT_THROWN);
#endif
	newsym(mtmp->mx, mtmp->my);
	/* a life-saved monster might be leashed;
	   don't leave it that way if it's no longer tame */
	if (mtmp->mleashed) m_unleash(mtmp, TRUE);
    }

    /* if its still a pet, start a clean pet-slate now */
    if (edog && mtmp->mtame) {
	edog->revivals++;
	edog->killed_by_u = 0;
	edog->abuse = 0;
	edog->ogoal.x = edog->ogoal.y = -1;
	if (was_dead || edog->hungrytime < monstermoves + 500L)
	    edog->hungrytime = monstermoves + 500L;
	if (was_dead) {
	    edog->droptime = 0L;
	    edog->dropdist = 10000;
	    edog->whistletime = 0L;
	    edog->apport = 5;
	} /* else lifesaved, so retain current values */
    }
}

void
abuse_dog(mtmp)
struct monst *mtmp;
{
	if (!mtmp->mtame) return;

	if (Aggravate_monster || Conflict) mtmp->mtame /=2;
	else mtmp->mtame--;

	if (mtmp->mtame && !mtmp->isminion)
	    EDOG(mtmp)->abuse++;

	if (!mtmp->mtame && mtmp->mleashed)
	    m_unleash(mtmp, TRUE);

	/* don't make a sound if pet is in the middle of leaving the level */
	/* newsym isn't necessary in this case either */
	if (mtmp->mx != 0) {
	    if (mtmp->mtame && rn2(mtmp->mtame)) yelp(mtmp);
	    else growl(mtmp);	/* give them a moment's worry */

	    if (!mtmp->mtame) newsym(mtmp->mx, mtmp->my);
	}
}

#endif /* OVLB */

/*dog.c*/
