/*	SCCS Id: @(#)monmove.c	3.4	2002/04/06	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"
#include "artifact.h"
#include "epri.h"

extern boolean notonhead;

#ifdef OVL0

STATIC_DCL int FDECL(disturb,(struct monst *));
STATIC_DCL void FDECL(distfleeck,(struct monst *,int *,int *,int *));
STATIC_DCL int FDECL(m_arrival, (struct monst *));
STATIC_DCL void FDECL(watch_on_duty,(struct monst *));
STATIC_DCL boolean FDECL(likes_midrange, (struct monst *));

#endif /* OVL0 */
#ifdef OVLB

boolean /* TRUE : mtmp died */
mb_trapped(mtmp)
register struct monst *mtmp;
{
	if (flags.verbose) {
	    if (cansee(mtmp->mx, mtmp->my))
		pline(E_J("KABOOM!!  You see a door explode.",
			  "ボカーン!!　あなたは扉が爆発するのを見た。"));
	    else if (flags.soundok)
		You_hear(E_J("a distant explosion.","遠くに爆発音を"));
	}
	wake_nearto(mtmp->mx, mtmp->my, 7*7);
	mtmp->mstun = 1;
	mlosehp(mtmp, rnd(15));
	if(mtmp->mhp <= 0) {
		mondied(mtmp);
		if (mtmp->mhp > 0) /* lifesaved */
			return(FALSE);
		else
			return(TRUE);
	}
	return(FALSE);
}

#endif /* OVLB */
#ifdef OVL0

STATIC_OVL void
watch_on_duty(mtmp)
register struct monst *mtmp;
{
	int	x, y;

	if(mtmp->mpeaceful && in_town(u.ux+u.dx, u.uy+u.dy) &&
	   mtmp->mcansee && m_canseeu(mtmp) && !rn2(3)) {

	    if(picking_lock(&x, &y) && IS_DOOR(levl[x][y].typ) &&
	       (levl[x][y].doormask & D_LOCKED)) {

		if(couldsee(mtmp->mx, mtmp->my)) {

		  pline(E_J("%s yells:","%sは叫んだ:"), Amonnam(mtmp));
		  if(levl[x][y].looted & D_WARNED) {
			verbalize(E_J("Halt, thief!  You're under arrest!",
				      "動くな、泥棒め！ お前を逮捕する！"));
			(void) angry_guards(!(flags.soundok));
		  } else {
			verbalize(E_J("Hey, stop picking that lock!",
				      "おい、錠をこじ開けるのを止めろ！"));
			levl[x][y].looted |=  D_WARNED;
		  }
		  stop_occupation();
		}
	    } else if (is_digging()) {
		/* chewing, wand/spell of digging are checked elsewhere */
		watch_dig(mtmp, digging.pos.x, digging.pos.y, FALSE);
	    }
	}
}

#endif /* OVL0 */
#ifdef OVL1

int
dochugw(mtmp)
	register struct monst *mtmp;
{
	register int x = mtmp->mx, y = mtmp->my;
	boolean already_saw_mon = !occupation ? 0 : canspotmon(mtmp);
	int rd = dochug(mtmp);
#if 0
	/* part of the original warning code which was replaced in 3.3.1 */
	int dd;

	if(Warning && !rd && !mtmp->mpeaceful &&
			(dd = distu(mtmp->mx,mtmp->my)) < distu(x,y) &&
			dd < 100 && !canseemon(mtmp)) {
	    /* Note: this assumes we only want to warn against the monster to
	     * which the weapon does extra damage, as there is no "monster
	     * which the weapon warns against" field.
	     */
	    if (spec_ability(uwep, SPFX_WARN) && spec_dbon(uwep, mtmp, 1))
		warnlevel = 100;
	    else if ((int) (mtmp->m_lev / 4) > warnlevel)
		warnlevel = (mtmp->m_lev / 4);
	}
#endif /* 0 */

	/* a similar check is in monster_nearby() in hack.c */
	/* check whether hero notices monster and stops current activity */
	if (occupation && !rd && !Confusion &&
	    (!mtmp->mpeaceful || Hallucination) &&
	    /* it's close enough to be a threat */
	    distu(mtmp->mx,mtmp->my) <= (BOLT_LIM+1)*(BOLT_LIM+1) &&
	    /* and either couldn't see it before, or it was too far away */
	    (!already_saw_mon || !couldsee(x,y) ||
		distu(x,y) > (BOLT_LIM+1)*(BOLT_LIM+1)) &&
	    /* can see it now, or sense it and would normally see it */
	    (canseemon(mtmp) ||
		(sensemon(mtmp) && couldsee(mtmp->mx,mtmp->my))) &&
	    mtmp->mcanmove &&
	    !noattacks(mtmp->data) && !onscary(u.ux, u.uy, mtmp))
		stop_occupation();

	return(rd);
}

#endif /* OVL1 */
#ifdef OVL2

boolean
onscary(x, y, mtmp)
int x, y;
struct monst *mtmp;
{
	if (mtmp->isshk || mtmp->isgd || mtmp->iswiz || !mtmp->mcansee ||
	    mtmp->mpeaceful || mtmp->data->mlet == S_HUMAN ||
	    is_lminion(mtmp) || mtmp->mnum == PM_ANGEL ||
	    is_rider(mtmp->data) || mtmp->mnum == PM_MINOTAUR)
		return(FALSE);

	return (boolean)(sobj_at(SCR_SCARE_MONSTER, x, y)
#ifdef ELBERETH
			 || sengr_at("Elbereth", x, y)
#endif
			 || (mtmp->data->mlet == S_VAMPIRE
			     && IS_ALTAR(levl[x][y].typ)));
}

#endif /* OVL2 */
#ifdef OVL0

/* regenerate lost hit points */
void
mon_regen(mon, digest_meal)
struct monst *mon;
boolean digest_meal;
{
	if (mon->mhp < mon->mhpmax &&
	    (moves % 20 == 0 || (regenerates(mon->data) && !mon->mcan))) mlosehp(mon, -1);
	if (mon->mspec_used) mon->mspec_used--;
	if (digest_meal) {
	    if (mon->meating) mon->meating--;
	}
}

/*
 * Possibly awaken the given monster.  Return a 1 if the monster has been
 * jolted awake.
 */
STATIC_OVL int
disturb(mtmp)
	register struct monst *mtmp;
{
	/*
	 * + Ettins are hard to surprise.
	 * + Nymphs, jabberwocks, and leprechauns do not easily wake up.
	 *
	 * Wake up if:
	 *	in direct LOS						AND
	 *	within 10 squares					AND
	 *	not stealthy or (mon is an ettin and 9/10)		AND
	 *	(mon is not a nymph, jabberwock, or leprechaun) or 1/50	AND
	 *	Aggravate or mon is (dog or human) or
	 *	    (1/7 and mon is not mimicing furniture or object)
	 */
	if(couldsee(mtmp->mx,mtmp->my) &&
		distu(mtmp->mx,mtmp->my) <= 100 &&
		(!Stealth || (mtmp->mnum == PM_ETTIN && rn2(10))) &&
		(!(mtmp->data->mlet == S_NYMPH
			|| mtmp->mnum == PM_JABBERWOCK
#if 0	/* DEFERRED */
			|| mtmp->mnum == PM_VORPAL_JABBERWOCK
#endif
			|| mtmp->data->mlet == S_LEPRECHAUN) || !rn2(50)) &&
		(Aggravate_monster
			|| (mtmp->data->mlet == S_DOG ||
				mtmp->data->mlet == S_HUMAN)
			|| (!rn2(7) && mtmp->m_ap_type != M_AP_FURNITURE &&
				mtmp->m_ap_type != M_AP_OBJECT) )) {
		mtmp->msleeping = 0;
		return(1);
	}
	return(0);
}

/* monster begins fleeing for the specified time, 0 means untimed flee
 * if first, only adds fleetime if monster isn't already fleeing
 * if fleemsg, prints a message about new flight, otherwise, caller should */
void
monflee(mtmp, fleetime, first, fleemsg)
struct monst *mtmp;
int fleetime;
boolean first;
boolean fleemsg;
{
	if (u.ustuck == mtmp) {
	    if (u.uswallow)
		expels(mtmp, mtmp->data, TRUE);
	    else if (!sticks(youmonst.data)) {
		unstuck(mtmp);	/* monster lets go when fleeing */
		You(E_J("get released!","解放された！"));
	    }
	}

	if (!first || !mtmp->mflee) {
	    /* don't lose untimed scare */
	    if (!fleetime)
		mtmp->mfleetim = 0;
	    else if (!mtmp->mflee || mtmp->mfleetim) {
		fleetime += mtmp->mfleetim;
		/* ensure monster flees long enough to visibly stop fighting */
		if (fleetime == 1) fleetime++;
		mtmp->mfleetim = min(fleetime, 127);
	    }
	    if (!mtmp->mflee && fleemsg && canseemon(mtmp) && !mtmp->mfrozen)
		pline(E_J("%s turns to flee!","%sは逃げ出した！"), (Monnam(mtmp)));
	    mtmp->mflee = 1;
	}
}

STATIC_OVL void
distfleeck(mtmp,inrange,nearby,scared)
register struct monst *mtmp;
int *inrange, *nearby, *scared;
{
	int seescaryx, seescaryy;

	*inrange = (dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <=
							(BOLT_LIM * BOLT_LIM));
	*nearby = *inrange && monnear(mtmp, mtmp->mux, mtmp->muy);

	/* Note: if your image is displaced, the monster sees the Elbereth
	 * at your displaced position, thus never attacking your displaced
	 * position, but possibly attacking you by accident.  If you are
	 * invisible, it sees the Elbereth at your real position, thus never
	 * running into you by accident but possibly attacking the spot
	 * where it guesses you are.
	 */
	if (!mtmp->mcansee || (Invis && !perceives(mtmp->data))) {
		seescaryx = mtmp->mux;
		seescaryy = mtmp->muy;
	} else {
		seescaryx = u.ux;
		seescaryy = u.uy;
	}
	*scared = (*nearby && (onscary(seescaryx, seescaryy, mtmp) ||
			       (!mtmp->mpeaceful &&
				    in_your_sanctuary(mtmp, 0, 0))));

	if(*scared) {
		if (rn2(7))
		    monflee(mtmp, rnd(10), TRUE, TRUE);
		else
		    monflee(mtmp, rnd(100), TRUE, TRUE);
	}

}

/* perform a special one-time action for a monster; returns -1 if nothing
   special happened, 0 if monster uses up its turn, 1 if monster is killed */
STATIC_OVL int
m_arrival(mon)
struct monst *mon;
{
	mon->mstrategy &= ~STRAT_ARRIVE;	/* always reset */

	return -1;
}

/* returns 1 if monster died moving, 0 otherwise */
/* The whole dochugw/m_move/distfleeck/mfndpos section is serious spaghetti
 * code. --KAA
 */
int
dochug(mtmp)
register struct monst *mtmp;
{
	register struct permonst *mdat;
	register int tmp=0;
	int inrange, nearby, scared;
	int dontmove;

/*	Pre-movement adjustments	*/

	mdat = mtmp->data;

	/* check if the monster can change its location */
	if (mdat->mpermove) {
	    if (!mtmp->permove) mtmp->permove = mdat->mpermove;
	    dontmove = !!(--(mtmp->permove));
	} else {
	    dontmove = 1;
	}

#ifdef MONSTEED
	if (is_mridden(mtmp) ||
	     (is_mriding(mtmp) &&
		(mtmp->mlink->mtrapped || !mtmp->mlink->mcanmove ||
		 mtmp->mlink->msleeping || mtmp->mlink->permove))) {
	    dontmove = 1;
	    if (is_mriding(mtmp) && mtmp->mlink->msleeping &&
		mtmp->mcanmove && !mtmp->msleeping &&
		!monnear(mtmp, mtmp->mux, mtmp->muy)) {
		if (canspotmon(mtmp)) {
		    char nstd[BUFSZ];
		    if (canspotmon(mtmp->mlink))
			Sprintf(nstd, E_J("%s %s","%s%s"), mhis(mtmp),
				x_monnam(mtmp->mlink, ARTICLE_NONE, 0, SUPPRESS_SADDLE, FALSE));
		    else Sprintf(nstd, E_J("invisible steed","見えない乗騎"));
		    if (canseemon(mtmp)) pline(E_J("%s kicks %s.","%sは%sを蹴った。"), Monnam(mtmp), nstd);
		    wakeup(mtmp->mlink);
		    if (mtmp->mlink->mcanmove && canseemon(mtmp->mlink))
			pline(E_J("%s wakes up.","%sは目を覚ました。"), Monnam(mtmp->mlink));
		} else {
		    wakeup(mtmp->mlink);
		    if (mtmp->mlink->mcanmove && canseemon(mtmp->mlink))
			pline(E_J("%s suddenly wakes up.","%sは突然目を覚ました。"), Monnam(mtmp->mlink));
		}
		return(0);
	    }
	}
#endif /*MONSTEED*/

	if (mtmp->mstrategy & STRAT_ARRIVE) {
	    int res = m_arrival(mtmp);
	    if (res >= 0) return res;
	}

	/* check for waitmask status change */
	if ((mtmp->mstrategy & STRAT_WAITFORU) &&
		(m_canseeu(mtmp) || mtmp->mhp < mtmp->mhpmax))
	    mtmp->mstrategy &= ~STRAT_WAITFORU;

	/* update quest status flags */
	quest_stat_check(mtmp);

	if (!mtmp->mcanmove || (mtmp->mstrategy & STRAT_WAITMASK)) {
	    if (Hallucination) newsym(mtmp->mx,mtmp->my);
	    if (mtmp->mcanmove && (mtmp->mstrategy & STRAT_CLOSE) &&
	       !mtmp->msleeping && monnear(mtmp, u.ux, u.uy))
		quest_talk(mtmp);	/* give the leaders a chance to speak */
	    return(0);	/* other frozen monsters can't do anything */
	}

	/* there is a chance we will wake it */
	if (mtmp->msleeping && !disturb(mtmp)) {
		if (Hallucination) newsym(mtmp->mx,mtmp->my);
		return(0);
	}

	/* not frozen or sleeping: wipe out texts written in the dust */
	wipe_engr_at(mtmp->mx, mtmp->my, 1);

	/* confused monsters get unconfused with small probability */
	if (mtmp->mconf && !rn2(50)) mtmp->mconf = 0;

	/* stunned monsters get un-stunned with larger probability */
	if (mtmp->mstun && !rn2(10)) mtmp->mstun = 0;

	/* some monsters teleport */
	if (mtmp->mflee && !rn2(40) && can_teleport(mdat) && !mtmp->iswiz &&
	    !level.flags.noteleport) {
		(void) rloc(mtmp, FALSE);
		return(0);
	}
	if (mdat->msound == MS_SHRIEK && !um_dist(mtmp->mx, mtmp->my, 1))
	    m_respond(mtmp);
	if (mtmp->mnum == PM_MEDUSA && couldsee(mtmp->mx, mtmp->my))
	    m_respond(mtmp);
	if (mtmp->mhp <= 0) return(1); /* m_respond gaze can kill medusa */

	/* fleeing monsters might regain courage */
	if (mtmp->mflee && !mtmp->mfleetim
	   && mtmp->mhp == mtmp->mhpmax && !rn2(25)) mtmp->mflee = 0;

	set_apparxy(mtmp);
	/* Must be done after you move and before the monster does.  The
	 * set_apparxy() call in m_move() doesn't suffice since the variables
	 * inrange, etc. all depend on stuff set by set_apparxy().
	 */

	/* Monsters that want to acquire things */
	/* may teleport, so do it before inrange is set */
	if(is_covetous(mdat)) (void) tactics(mtmp);

	/* check distance and scariness of attacks */
	distfleeck(mtmp,&inrange,&nearby,&scared);

	if(find_defensive(mtmp)) {
		if (use_defensive(mtmp) != 0)
			return 1;
	} else if(find_misc(mtmp)) {
		if (use_misc(mtmp) != 0)
			return 1;
	}

	/* Demonic Blackmail! */
	if(nearby && mdat->msound == MS_BRIBE &&
	   mtmp->mtrybribe && !mtmp->mtame && !u.uswallow) {
		if (mtmp->mux != u.ux || mtmp->muy != u.uy) {
			pline(E_J("%s whispers at thin air.",
				  "%sは何もない空間に向かって囁いた。"),
			    cansee(mtmp->mux, mtmp->muy) ? Monnam(mtmp) : E_J("It","何者か"));

			if (is_demon(youmonst.data)) {
			  /* "Good hunting, brother" */
			    if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
			} else {
			    mtmp->minvis = mtmp->perminvis = 0;
			    /* Why?  For the same reason in real demon talk */
			    pline(E_J("%s gets angry!","%sは怒った！"), Amonnam(mtmp));
			    setmpeaceful(mtmp, FALSE);
			    /* since no way is an image going to pay it off */
			}
		} else if(demon_talk(mtmp)) return(1);	/* you paid it off */
	}

	/* the watch will look around and see if you are up to no good :-) */
	if (mtmp->mnum == PM_WATCHMAN || mtmp->mnum == PM_WATCH_CAPTAIN)
		watch_on_duty(mtmp);

	else if (is_mind_flayer(mdat) && !rn2(20)) {
		struct monst *m2, *nmon = (struct monst *)0;

		if (canseemon(mtmp))
			pline(E_J("%s concentrates.","%sは精神を集中した。"), Monnam(mtmp));
		if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM ||
		    u.uinvulnerable) {
			You(E_J("sense a faint wave of psychic energy.",
				"かすかな精神エネルギーの波を感じ取った。"));
			goto toofar;
		}
		pline(E_J("A wave of psychic energy pours over you!",
			  "精神エネルギーの奔流があなたを包み込んだ！"));
		if (mtmp->mpeaceful &&
		    (!Conflict || resist(mtmp, RING_CLASS, 0, 0)))
			pline(E_J("It feels quite soothing.",
				  "心がなごんだ。"));
		else {
			register boolean m_sen = sensemon(mtmp);

			if (m_sen || (Blind_telepat && rn2(2)) || !rn2(10)) {
				int dmg;
				pline(E_J("It locks on to your %s!",
					  "それはあなたの%sを直撃した！"),
					m_sen ? E_J("telepathy","精神感応力") :
					Blind_telepat ? E_J("latent telepathy","潜在的な精神感応力") :
					    E_J("mind","精神"));
				dmg = rnd(15);
				if (Half_spell_damage) dmg = (dmg+1) / 2;
				losehp(dmg, E_J("psychic blast","サイキック・ブラスト"), KILLED_BY_AN);
			}
		}
		for(m2=fmon; m2; m2 = nmon) {
			nmon = m2->nmon;
			if (DEADMONSTER(m2)) continue;
			if (m2->mpeaceful == mtmp->mpeaceful) continue;
			if (mindless(m2->data)) continue;
			if (m2 == mtmp) continue;
			if ((telepathic(m2->data) &&
			    (rn2(2) || m2->mblinded)) || !rn2(10)) {
				if (cansee(m2->mx, m2->my))
				    pline(E_J("It locks on to %s.","精神波は%sを捉えた。"), mon_nam(m2));
				mlosehp(m2, rnd(15));
				if (m2->mhp <= 0)
				    monkilled(m2, "", AD_DRIN);
				else
				    m2->msleeping = 0;
			}
		}
	}
toofar:

	/* If monster is nearby you, and has to wield a weapon, do so.   This
	 * costs the monster a move, of course.
	 */
	if((!mtmp->mpeaceful || Conflict) && inrange &&
	   dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 8
	   && attacktype(mdat, AT_WEAP)) {
	    struct obj *mw_tmp;

	    /* The scared check is necessary.  Otherwise a monster that is
	     * one square near the player but fleeing into a wall would keep	
	     * switching between pick-axe and weapon.  If monster is stuck
	     * in a trap, prefer ranged weapon (wielding is done in thrwmu).
	     * This may cost the monster an attack, but keeps the monster
	     * from switching back and forth if carrying both.
	     */
	    mw_tmp = MON_WEP(mtmp);
	    if (!(scared && mw_tmp && is_pick(mw_tmp)) &&
		mtmp->weapon_check == NEED_WEAPON &&
		!(mtmp->mtrapped && !nearby && select_rwep(mtmp))) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		if (mon_wield_item(mtmp) != 0) return(0);
	    }
	}

#ifdef MONSTEED
	/* if mon is dismounted, try riding */
	if (wants_steed(mdat) && !is_mriding(mtmp) && !mtmp->mconf &&
	    !mtmp->mstun && !rn2(3)) {
	    struct monst *mstd;
	    mstd = msearchsteed(mtmp);
	    if (mstd) {
		if (cansee(mstd->mx, mstd->my)) {
		    char stdnam[BUFSIZ];
		    Strcpy(stdnam, x_monnam(mstd, ARTICLE_THE, 0, SUPPRESS_SADDLE, FALSE));
		    pline(E_J("%s mounts on %s.","%sは%sに乗った。"), Monnam(mtmp), stdnam);
		}
		remove_monster(mstd->mx, mstd->my);
		mstd->mlink    = mtmp;
		mstd->mlinktyp = MLT_RIDER;
		mtmp->mlink    = mstd;
		mtmp->mlinktyp = MLT_STEED;
		rloc_to(mtmp, mstd->mx, mstd->my);
		return(0);
	    }
	}
#endif

	/* monster cannot move by some reasons */
	if (dontmove) {
	    if (mtmp->mconf) {
		fightm(mtmp); /* arbitrary... */
		return (DEADMONSTER(mtmp));
	    }
	    if ((!nearby && !ranged_attk(mdat) && !find_offensive(mtmp)) ||
		mtmp->mflee || scared || (mtmp->mstun && !rn2(3))) return (0);
	    else goto attack;
	}

/*	Now the actual movement phase	*/
	if(!nearby || mtmp->mflee || scared ||
	   mtmp->mconf || mtmp->mstun || (mtmp->minvis && !rn2(3)) ||
	   likes_midrange(mtmp) ||
	   (mdat->mlet == S_LEPRECHAUN && !u.ugold && (mtmp->mgold || rn2(2))) ||
	   (is_wanderer(mdat) && !rn2(4)) || (Conflict && !mtmp->iswiz) ||
	   (mtmp->mstrategy && (mtmp->mstrategy & STRAT_STRATMASK) != STRAT_PLAYER) ||
	   (!mtmp->mcansee && !rn2(4)) || mtmp->mpeaceful) {
		/* Possibly cast an undirected spell if not attacking you */
		/* note that most of the time castmu() will pick a directed
		   spell and do nothing, so the monster moves normally */
		/* arbitrary distance restriction to keep monster far away
		   from you from having cast dozens of sticks-to-snakes
		   or similar spells by the time you reach it */
		if (dist2(mtmp->mx, mtmp->my, u.ux, u.uy) <= 49 && !mtmp->mspec_used) {
		    struct attack *a;

		    for (a = &mdat->mattk[0]; a < &mdat->mattk[NATTK]; a++) {
			if (a->aatyp == AT_MAGC && (a->adtyp == AD_SPEL || a->adtyp == AD_CLRC)) {
			    if (castmu(mtmp, a, FALSE, FALSE)) {
				tmp = 3;
				break;
			    }
			}
		    }
		}

		tmp = m_move(mtmp, 0);
		distfleeck(mtmp,&inrange,&nearby,&scared);	/* recalc */

		switch (tmp) {
		    case 0:	/* no movement, but it can still attack you */
		    case 3:	/* absolutely no movement */
				/* for pets, case 0 and 3 are equivalent */
			/* vault guard might have vanished */
			if (mtmp->isgd && (mtmp->mhp < 1 ||
					    (mtmp->mx == 0 && mtmp->my == 0)))
			    return 1;	/* behave as if it died */
			/* During hallucination, monster appearance should
			 * still change - even if it doesn't move.
			 */
			if(Hallucination) newsym(mtmp->mx,mtmp->my);
			break;
		    case 1:	/* monster moved */
			/* Maybe it stepped on a trap and fell asleep... */
			if (mtmp->msleeping || !mtmp->mcanmove) return(0);
			if(!nearby &&
			  (ranged_attk(mdat) || find_offensive(mtmp)))
			    break;
 			else if(u.uswallow && mtmp == u.ustuck) {
			    /* a monster that's digesting you can move at the
			     * same time -dlc
			     */
			    return(mattacku(mtmp));
			} else
				return(0);
			/*NOTREACHED*/
			break;
		    case 2:	/* monster died */
			return(1);
		}
	}

/*	Now, attack the player if possible - one attack set per monst	*/
attack:
	if (!mtmp->mpeaceful ||
	    (Conflict && !resist(mtmp, RING_CLASS, 0, 0))) {
	    if(inrange && !noattacks(mdat) && u.uhp > 0 && !scared && tmp != 3)
		if(mattacku(mtmp)) return(1); /* monster died (e.g. exploded) */

	    if(mtmp->wormno) wormhitu(mtmp);
	}
	/* special speeches for quest monsters */
	if (!mtmp->msleeping && mtmp->mcanmove && inrange/*nearby*/)
	    quest_talk(mtmp);
	/* extra emotional attack for vile monsters */
	if (inrange && mtmp->data->msound == MS_CUSS && !mtmp->mpeaceful &&
		couldsee(mtmp->mx, mtmp->my) && !mtmp->minvis && !rn2(5))
	    cuss(mtmp);

	return(tmp == 2);
}

static NEARDATA const char practical[] = { WEAPON_CLASS, ARMOR_CLASS, GEM_CLASS, FOOD_CLASS, 0 };
static NEARDATA const char magical[] = {
	AMULET_CLASS, POTION_CLASS, SCROLL_CLASS, WAND_CLASS, RING_CLASS,
	SPBOOK_CLASS, 0 };
static NEARDATA const char indigestion[] = { BALL_CLASS, ROCK_CLASS, 0 };
static NEARDATA const char boulder_class[] = { ROCK_CLASS, 0 };
static NEARDATA const char gem_class[] = { GEM_CLASS, 0 };

boolean
itsstuck(mtmp)
register struct monst *mtmp;
{
	if (sticks(youmonst.data) && mtmp==u.ustuck && !u.uswallow) {
		pline(E_J("%s cannot escape from you!",
			  "%sはあなたから逃れられない！"), Monnam(mtmp));
		return(TRUE);
	}
	return(FALSE);
}

/* Return values:
 * 0: did not move, but can still attack and do other stuff.
 * 1: moved, possibly can attack.
 * 2: monster died.
 * 3: did not move, and can't do anything else either.
 */
int
m_move(mtmp, after)
register struct monst *mtmp;
register int after;
{
	register int appr;
	xchar gx,gy,nix,niy,chcnt;
	int chi;	/* could be schar except for stupid Sun-2 compiler */
	boolean likegold=0, likegems=0, likeobjs=0, likemagic=0, conceals=0;
	boolean likerock=0, can_tunnel=0;
	boolean can_open=0, can_unlock=0, doorbuster=0;
	boolean uses_items=0, setlikes=0;
	boolean avoid=FALSE;
	struct permonst *ptr;
	struct monst *mtoo;
	schar mmoved = 0;	/* not strictly nec.: chi >= 0 will do */
	long info[9];
	long flag;
	int  omx = mtmp->mx, omy = mtmp->my;
	struct obj *mw_tmp;

	if (is_swamp(mtmp->mx, mtmp->my) && rn2(3) &&
	    !is_flying(mtmp) && !is_floating(mtmp) &&
	    !is_clinging(mtmp) && !is_swimming(mtmp) &&
	    !amphibious(mtmp->data) && !likes_swamp(mtmp->data))
		return(0);

	if(mtmp->mtrapped) {
	    int i = mintrap(mtmp);
	    if(i >= 2) { newsym(mtmp->mx,mtmp->my); return(2); }/* it died */
	    if(i == 1) return(0);	/* still in trap, so didn't move */
	}
	ptr = mtmp->data; /* mintrap() can change mtmp->data -dlc */

	if (mtmp->meating) {
	    mtmp->meating--;
	    return 3;			/* still eating */
	}
	if (hides_under(ptr) && OBJ_AT(mtmp->mx, mtmp->my) && rn2(10))
	    return 0;		/* do not leave hiding place */

	set_apparxy(mtmp);
	/* where does mtmp think you are? */
	/* Not necessary if m_move called from this file, but necessary in
	 * other calls of m_move (ex. leprechauns dodging)
	 */
#ifdef REINCARNATION
	if (!Is_rogue_level(&u.uz))
#endif
	    can_tunnel = tunnels(ptr);
	can_open = !(nohands(ptr) || verysmall(ptr));
	can_unlock = ((can_open && m_carrying(mtmp, SKELETON_KEY)) ||
		      mtmp->iswiz || is_rider(ptr));
	doorbuster = is_giant(ptr);
	if(mtmp->wormno) goto not_special;
	/* my dog gets special treatment */
	if(mtmp->mtame) {
	    mmoved = dog_move(mtmp, after);
	    goto postmov;
	}

	/* likewise for shopkeeper */
	if(mtmp->isshk) {
	    mmoved = shk_move(mtmp);
	    if(mmoved == -2) return(2);
	    if(mmoved >= 0) goto postmov;
	    mmoved = 0;		/* follow player outside shop */
	}

	/* and for the guard */
	if(mtmp->isgd) {
	    mmoved = gd_move(mtmp);
	    if(mmoved == -2) return(2);
	    if(mmoved >= 0) goto postmov;
	    mmoved = 0;
	}

	/* and the acquisitive monsters get special treatment */
	if(is_covetous(ptr)) {
	    struct monst *intruder;
	    if (mtmp->mstrategy == STRAT_NONE) goto not_special;
	    gx = STRAT_GOALX(mtmp->mstrategy);
	    gy = STRAT_GOALY(mtmp->mstrategy);
	    appr = 1;
	    if (mtmp->mstrategy == STRAT_HEAL) {
		gx = mtmp->mux;
		gy = mtmp->muy;
		appr = -1;
		goto goal_is_set;
	    }
	    intruder = m_at(gx, gy);
	    /*
	     * if there's a monster on the object or in possesion of it,
	     * attack it.
	     */
	    if((dist2(mtmp->mx, mtmp->my, gx, gy) < 2) &&
	       intruder && (intruder != mtmp)) {

		notonhead = (intruder->mx != gx || intruder->my != gy);
#ifdef MONSTEED
		intruder = target_rider_or_steed(mtmp, intruder);
#endif
		if(mattackm(mtmp, intruder) == 2) return(2);
		mmoved = 1;
		goto postmov;
	    }
	    goto goal_is_set;
	}

	/* and for the priest */
	if(mtmp->ispriest) {
	    mmoved = pri_move(mtmp);
	    if(mmoved == -2) return(2);
	    if(mmoved >= 0) goto postmov;
	    mmoved = 0;
	}

	/* room guard */
	if((mtmp->mstrategy & STRAT_ROOMGUARD) != 0) {
	    if (m_canseeu(mtmp)) goto not_special;
	    gx = STRAT_GOALX(mtmp->mstrategy);
	    gy = STRAT_GOALY(mtmp->mstrategy);
	    appr = 1;
	    if (!rn2(1+distmin(gx, gy, mtmp->mx, mtmp->my))) {
		appr = 0;
	    }
	    goto goal_is_set;
	}

#ifdef MAIL
	if(mtmp->mnum == PM_MAIL_DAEMON) {
	    if(flags.soundok && canseemon(mtmp))
		verbalize(E_J("I'm late!","遅れた！"));
	    mongone(mtmp);
	    return(2);
	}
#endif

	/* teleport if that lies in our nature */
	if(mtmp->mnum == PM_TENGU && !rn2(5) && !mtmp->mcan &&
	   !tele_restrict(mtmp)) {
	    if(mtmp->mhp < 7 || mtmp->mpeaceful || rn2(2))
		(void) rloc(mtmp, FALSE);
	    else
		mnexto(mtmp);
	    mmoved = 1;
	    goto postmov;
	}

not_special:
	if(u.uswallow && !mtmp->mflee && u.ustuck != mtmp) return(1);
	gx = mtmp->mux;
	gy = mtmp->muy;
	appr = mtmp->mflee ? -1 : 1;
goal_is_set:
	omx = mtmp->mx;
	omy = mtmp->my;
	if (mtmp->mconf || (u.uswallow && mtmp == u.ustuck)
#ifdef MONSTEED
	    || (is_mriding(mtmp) &&
		(mtmp->mlink->mconf || (appr == 1 && mtmp->mlink->mflee)))
#endif
	   ) appr = 0;
	else {
		boolean should_see = (couldsee(omx, omy) &&
				      (levl[gx][gy].lit ||
				       !levl[omx][omy].lit) &&
				      (dist2(omx, omy, gx, gy) <= 36));
		boolean can_shoot = dist2(omx, omy, gx, gy) <= 18 && lined_up2(mtmp);

		if (!mtmp->mcansee ||
		    (should_see && Invis && !perceives(ptr) && rn2(11)) ||
		    (youmonst.m_ap_type == M_AP_OBJECT && youmonst.mappearance == STRANGE_OBJECT) || u.uundetected ||
		    (youmonst.m_ap_type == M_AP_OBJECT && youmonst.mappearance == GOLD_PIECE && !likes_gold(ptr)) ||
		    (mtmp->mpeaceful && !mtmp->isshk) ||  /* allow shks to follow */
		    ((monsndx(ptr) == PM_STALKER || ptr->mlet == S_BAT ||
		      ptr->mlet == S_LIGHT) && !rn2(3)))
			appr = 0;

		if(monsndx(ptr) == PM_LEPRECHAUN && (appr == 1) &&
		   (mtmp->mgold > u.ugold))
			appr = -1;

		/* keep distance from hero if mon has missiles */
		if ((mw_tmp = MON_WEP(mtmp)) && is_launcher(mw_tmp) && can_shoot) {
		    struct obj *otmp;
		    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
			if (ammo_and_launcher(otmp, mw_tmp)) appr = rn2(7) ? -1 : 0;
			break;
		    }
		}
		if (can_breathe(mtmp->data) && can_shoot) {
		    appr = (mtmp->mspec_used <= 5) && rn2(7) ? -1 : 1;
		}

		if (!should_see && can_track(ptr)) {
			register coord *cp;

			cp = gettrack(omx,omy);
			if (cp) {
				gx = cp->x;
				gy = cp->y;
			}
		}
	}

	if ((!mtmp->mpeaceful || !rn2(10))
#ifdef REINCARNATION
				    && (!Is_rogue_level(&u.uz))
#endif
							    ) {
	    boolean in_line = lined_up2(mtmp) &&
		(distmin(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <=
		    (throws_rocks(youmonst.data) ? 20 : ACURRSTR/2+1)
		);

	    if (appr != 1 || !in_line) {
		/* Monsters in combat won't pick stuff up, avoiding the
		 * situation where you toss arrows at it and it has nothing
		 * better to do than pick the arrows up.
		 */
		register int pctload = (curr_mon_load(mtmp) * 100) /
			max_mon_load(mtmp);

		/* look for gold or jewels nearby */
		likegold = (likes_gold(ptr) && pctload < 95);
		likegems = (likes_gems(ptr) && pctload < 85);
		uses_items = (!mindless(ptr) && !is_animal(ptr)
			&& pctload < 75);
		likeobjs = (likes_objs(ptr) && pctload < 75);
		likemagic = (likes_magic(ptr) && pctload < 85);
		likerock = (throws_rocks(ptr) && pctload < 50 && !In_sokoban(&u.uz));
		conceals = hides_under(ptr);
		setlikes = TRUE;
	    }
	}

#define SQSRCHRADIUS	5

      { register int minr = SQSRCHRADIUS;	/* not too far away */
	register struct obj *otmp;
	register int xx, yy;
	int oomx, oomy, lmx, lmy;

	/* cut down the search radius if it thinks character is closer. */
	if(distmin(mtmp->mux, mtmp->muy, omx, omy) < SQSRCHRADIUS &&
	    !mtmp->mpeaceful) minr--;
	/* guards shouldn't get too distracted */
	if(!mtmp->mpeaceful && is_mercenary(ptr)) minr = 1;

	if((likegold || likegems || likeobjs || likemagic || likerock || conceals)
	      && (!*in_rooms(omx, omy, SHOPBASE) || (!rn2(25) && !mtmp->isshk))) {
	look_for_obj:
	    oomx = min(COLNO-1, omx+minr);
	    oomy = min(ROWNO-1, omy+minr);
	    lmx = max(1, omx-minr);
	    lmy = max(0, omy-minr);
	    for(otmp = fobj; otmp; otmp = otmp->nobj) {
		/* monsters may pick rocks up, but won't go out of their way
		   to grab them; this might hamper sling wielders, but it cuts
		   down on move overhead by filtering out most common item */
		if (otmp->otyp == ROCK) continue;
		xx = otmp->ox;
		yy = otmp->oy;
		/* Nymphs take everything.  Most other creatures should not
		 * pick up corpses except as a special case like in
		 * searches_for_item().  We need to do this check in
		 * mpickstuff() as well.
		 */
		if(xx >= lmx && xx <= oomx && yy >= lmy && yy <= oomy) {
		    /* don't get stuck circling around an object that's underneath
		       an immobile or hidden monster; paralysis victims excluded */
		    if ((mtoo = m_at(xx,yy)) != 0 &&
			(mtoo->msleeping || mtoo->mundetected ||
			 (mtoo->mappearance && !mtoo->iswiz) ||
			 !mtoo->data->mmove)) continue;

		    /* ignore an object if the monster cannot reach it */
		    if (is_pool(xx, yy) && !is_swimmer(ptr) &&
			!amphibious(ptr)) continue;
		    /* ignore sokoban prize */
		    if (Is_sokoprize(otmp)) continue;
#ifdef PICKUP_THROWN
		    /* ignore recently-thrown ammo */
		    if (otmp->othrown &&
			(monstermoves - otmp->age) < 50) continue;
#endif /*PICKUP_THROWN*/

		    if(((likegold && otmp->oclass == COIN_CLASS) ||
		       (likeobjs && index(practical, otmp->oclass) &&
			(otmp->otyp != CORPSE || (ptr->mlet == S_NYMPH
			   && !is_rider(&mons[otmp->corpsenm])))) ||
		       (likemagic && index(magical, otmp->oclass)) ||
		       (uses_items && searches_for_item(mtmp, otmp)) ||
		       (likerock && otmp->otyp == BOULDER) ||
		       (likegems && otmp->oclass == GEM_CLASS &&
			get_material(otmp) != MINERAL) ||
		       (conceals && !cansee(otmp->ox,otmp->oy)) ||
		       (mtmp->mnum == PM_GELATINOUS_CUBE &&
			!index(indigestion, otmp->oclass) &&
			!(otmp->otyp == CORPSE &&
			  touch_petrifies(&mons[otmp->corpsenm])))
		      ) && touch_artifact(otmp,mtmp,FALSE)) {
			if(can_carry(mtmp,otmp) &&
			   (throws_rocks(ptr) ||
				!sobj_at(BOULDER,xx,yy)) &&
			   (!is_unicorn(ptr) ||
			    get_material(otmp) == GEMSTONE) &&
			   /* Don't get stuck circling an Elbereth */
			   !(onscary(xx, yy, mtmp))) {
			    minr = distmin(omx,omy,xx,yy);
			    oomx = min(COLNO-1, omx+minr);
			    oomy = min(ROWNO-1, omy+minr);
			    lmx = max(1, omx-minr);
			    lmy = max(0, omy-minr);
			    gx = otmp->ox;
			    gy = otmp->oy;
			    if (gx == omx && gy == omy) {
				mmoved = 3; /* actually unnecessary */
				goto postmov;
			    }
			}
		    }
		}
	    }
	} else if(likegold) {
	    /* don't try to pick up anything else, but use the same loop */
	    uses_items = 0;
	    likegems = likeobjs = likemagic = likerock = conceals = 0;
	    goto look_for_obj;
	}

	if(minr < SQSRCHRADIUS && appr == -1) {
	    if(distmin(omx,omy,mtmp->mux,mtmp->muy) <= 3) {
		gx = mtmp->mux;
		gy = mtmp->muy;
	    } else
		appr = 1;
	}
      }

	/* don't tunnel if hostile and close enough to prefer a weapon */
	if (can_tunnel && needspick(ptr) &&
	    ((!mtmp->mpeaceful || Conflict) &&
	     dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 8))
	    can_tunnel = FALSE;

	nix = omx;
	niy = omy;
	flag = 0L;
	if (mtmp->mpeaceful && (!Conflict || resist(mtmp, RING_CLASS, 0, 0)))
	    flag |= (ALLOW_SANCT | ALLOW_SSM);
	else flag |= ALLOW_U;
	if (is_minion(ptr) || is_rider(ptr)) flag |= ALLOW_SANCT;
	/* unicorn may not be able to avoid hero on a noteleport level */
	if (is_unicorn(ptr) && !level.flags.noteleport) flag |= NOTONL;
	if (passes_walls(ptr)) flag |= (ALLOW_WALL | ALLOW_ROCK);
	if (passes_bars(ptr) || metallivorous(ptr)) flag |= ALLOW_BARS;
	if (can_tunnel) flag |= ALLOW_DIG;
	if (is_human(ptr) || mtmp->mnum == PM_MINOTAUR) flag |= ALLOW_SSM;
	if (is_undead(ptr) && ptr->mlet != S_GHOST) flag |= NOGARLIC;
	if (throws_rocks(ptr)) flag |= ALLOW_ROCK;
	if (can_open) flag |= OPENDOOR;
	if (can_unlock) flag |= UNLOCKDOOR;
	if (doorbuster) flag |= BUSTDOOR;
	if (mtmp->mconf) flag |= (ALLOW_M|ALLOW_TM);
	{
	    register int i, j, nx, ny, nearer;
	    int jcnt, cnt;
	    int ndist, nidist;
	    register coord *mtrk;
	    coord poss[9];

	    cnt = mfndpos(mtmp, poss, info, flag);
	    chcnt = 0;
	    jcnt = min(MTSZ, cnt-1);
	    chi = -1;
	    nidist = dist2(nix,niy,gx,gy);
	    /* allow monsters be shortsighted on some levels for balance */
	    if(!mtmp->mpeaceful && level.flags.shortsighted &&
	       nidist > (couldsee(nix,niy) ? 144 : 36) && appr == 1) appr = 0;
	    if (is_unicorn(ptr) && level.flags.noteleport) {
		/* on noteleport levels, perhaps we cannot avoid hero */
		for(i = 0; i < cnt; i++)
		    if(!(info[i] & NOTONL)) avoid=TRUE;
	    }

	    for(i=0; i < cnt; i++) {
		if (avoid && (info[i] & NOTONL)) continue;
		nx = poss[i].x;
		ny = poss[i].y;

		if (appr != 0) {
		    mtrk = &mtmp->mtrack[0];
		    for(j=0; j < jcnt; mtrk++, j++)
			if(nx == mtrk->x && ny == mtrk->y)
			    if(rn2(4*(cnt-j)))
				goto nxti;
		}

		nearer = ((ndist = dist2(nx,ny,gx,gy)) < nidist);

		if((appr == 1 && nearer) || (appr == -1 && !nearer) ||
		   (!appr && !rn2(++chcnt)) || !mmoved) {
		    nix = nx;
		    niy = ny;
		    nidist = ndist;
		    chi = i;
		    mmoved = 1;
		}
	    nxti:	;
	    }
	}

	if(mmoved) {
	    register int j;

	    if (mmoved==1 && (u.ux != nix || u.uy != niy) && itsstuck(mtmp))
		return(3);

	    if (((IS_ROCK(levl[nix][niy].typ) && may_dig(nix,niy)) ||
		 closed_door(nix, niy)) &&
		mmoved==1 && can_tunnel && needspick(ptr)) {
		if (closed_door(nix, niy)) {
		    if (!(mw_tmp = MON_WEP(mtmp)) ||
			!is_pick(mw_tmp) || !is_axe(mw_tmp))
			mtmp->weapon_check = NEED_PICK_OR_AXE;
		} else if (IS_TREE(levl[nix][niy].typ)) {
		    if (!(mw_tmp = MON_WEP(mtmp)) || !is_axe(mw_tmp))
			mtmp->weapon_check = NEED_AXE;
		} else if (!(mw_tmp = MON_WEP(mtmp)) || !is_pick(mw_tmp)) {
		    mtmp->weapon_check = NEED_PICK_AXE;
		}
		if (mtmp->weapon_check >= NEED_PICK_AXE) {
		    if (mon_wield_item(mtmp)) return(3);
		    can_tunnel = 0;
		}
	    }
	    /* If ALLOW_U is set, either it's trying to attack you, or it
	     * thinks it is.  In either case, attack this spot in preference to
	     * all others.
	     */
	/* Actually, this whole section of code doesn't work as you'd expect.
	 * Most attacks are handled in dochug().  It calls distfleeck(), which
	 * among other things sets nearby if the monster is near you--and if
	 * nearby is set, we never call m_move unless it is a special case
	 * (confused, stun, etc.)  The effect is that this ALLOW_U (and
	 * mfndpos) has no effect for normal attacks, though it lets a confused
	 * monster attack you by accident.
	 */
	    if(info[chi] & ALLOW_U) {
		nix = mtmp->mux;
		niy = mtmp->muy;
	    }
	    if (nix == u.ux && niy == u.uy) {
		mtmp->mux = u.ux;
		mtmp->muy = u.uy;
		return(0);
	    }
	    /* The monster may attack another based on 1 of 2 conditions:
	     * 1 - It may be confused.
	     * 2 - It may mistake the monster for your (displaced) image.
	     * Pets get taken care of above and shouldn't reach this code.
	     * Conflict gets handled even farther away (movemon()).
	     */
	    if((info[chi] & ALLOW_M) ||
		   (nix == mtmp->mux && niy == mtmp->muy)) {
		struct monst *mtmp2;
		int mstatus;
		mtmp2 = m_at(nix,niy);
#ifdef MONSTEED
		if (mtmp2) mtmp2 = target_rider_or_steed(mtmp, mtmp2);
#endif

		notonhead = mtmp2 && (nix != mtmp2->mx || niy != mtmp2->my);
		/* note: mstatus returns 0 if mtmp2 is nonexistent */
		mstatus = mattackm(mtmp, mtmp2);

		if (mstatus & MM_AGR_DIED)		/* aggressor died */
		    return 2;

		if ((mstatus & MM_HIT) && !(mstatus & MM_DEF_DIED)  &&
		    rn2(4) && mtmp2->movement >= NORMAL_SPEED) {
		    mtmp2->movement -= NORMAL_SPEED;
		    notonhead = 0;
		    mstatus = mattackm(mtmp2, mtmp);	/* return attack */
		    if (mstatus & MM_DEF_DIED)
			return 2;
		}
		return 3;
	    }

	    if (!m_in_out_region(mtmp,nix,niy))
	        return 3;
	    remove_monster(omx, omy);
	    place_monster(mtmp, nix, niy);
	    for(j = MTSZ-1; j > 0; j--)
		mtmp->mtrack[j] = mtmp->mtrack[j-1];
	    mtmp->mtrack[0].x = omx;
	    mtmp->mtrack[0].y = omy;
	    /* Place a segment at the old position. */
	    if (mtmp->wormno) worm_move(mtmp);
	} else {
	    if(is_unicorn(ptr) && rn2(2) && !tele_restrict(mtmp)) {
		(void) rloc(mtmp, FALSE);
		return(1);
	    }
	    if(mtmp->wormno) worm_nomove(mtmp);
	}
postmov:
	if(mmoved == 1 || mmoved == 3) {
	    boolean canseeit = cansee(mtmp->mx, mtmp->my);

	    if(mmoved == 1) {
		newsym(omx,omy);		/* update the old position */
		if (mintrap(mtmp) >= 2) {
		    if(mtmp->mx) newsym(mtmp->mx,mtmp->my);
		    return(2);	/* it died */
		}
		ptr = mtmp->data;

		/* open a door, or crash through it, if you can */
		if(IS_DOOR(levl[mtmp->mx][mtmp->my].typ)
			&& !passes_walls(ptr) /* doesn't need to open doors */
			&& !can_tunnel /* taken care of below */
		      ) {
		    struct rm *here = &levl[mtmp->mx][mtmp->my];
		    boolean btrapped = (here->doormask & D_TRAPPED);

		    if(here->doormask & (D_LOCKED|D_CLOSED) && amorphous(ptr)) {
			if (flags.verbose && canseemon(mtmp))
			    pline(E_J("%s %s under the door.",
				      "%sが扉の下から%s出てきた。"), Monnam(mtmp),
				  (mtmp->mnum == PM_FOG_CLOUD ||
				   mtmp->mnum == PM_YELLOW_LIGHT)
				  ? E_J("flows","あふれ") : E_J("oozes","にじみ"));
		    } else if(here->doormask & D_LOCKED && can_unlock) {
			if(btrapped) {
			    here->doormask = D_NODOOR;
			    newsym(mtmp->mx, mtmp->my);
			    unblock_point(mtmp->mx,mtmp->my); /* vision */
			    if(mb_trapped(mtmp)) return(2);
			} else {
			    if (flags.verbose) {
				if (canseeit)
				   You(E_J("see a door unlock and open.",
					   "扉の錠が外れ、開くのを見た。"));
				else if (flags.soundok)
				   You_hear(E_J("a door unlock and open.",
						"扉の錠が外れ、開く音を"));
			    }
			    here->doormask = D_ISOPEN;
			    /* newsym(mtmp->mx, mtmp->my); */
			    unblock_point(mtmp->mx,mtmp->my); /* vision */
			}
		    } else if (here->doormask == D_CLOSED && can_open) {
			if(btrapped) {
			    here->doormask = D_NODOOR;
			    newsym(mtmp->mx, mtmp->my);
			    unblock_point(mtmp->mx,mtmp->my); /* vision */
			    if(mb_trapped(mtmp)) return(2);
			} else {
			    if (flags.verbose) {
				if (canseeit)
				     You(E_J("see a door open.","扉が開くのを見た。"));
				else if (flags.soundok)
				     You_hear(E_J("a door open.","扉の開く音を"));
			    }
			    here->doormask = D_ISOPEN;
			    /* newsym(mtmp->mx, mtmp->my); */  /* done below */
			    unblock_point(mtmp->mx,mtmp->my); /* vision */
			}
		    } else if (here->doormask & (D_LOCKED|D_CLOSED)) {
			/* mfndpos guarantees this must be a doorbuster */
			if(btrapped) {
			    here->doormask = D_NODOOR;
			    newsym(mtmp->mx, mtmp->my);
			    unblock_point(mtmp->mx,mtmp->my); /* vision */
			    if(mb_trapped(mtmp)) return(2);
			} else {
			    if (flags.verbose) {
				if (canseeit)
				    You(E_J("see a door crash open.",
					    "扉が壊し開けられるのを見た。"));
				else if (flags.soundok)
				    You_hear(E_J("a door crash open.","扉が壊し開けられる音を"));
			    }
			    if (here->doormask & D_LOCKED && !rn2(2))
				    here->doormask = D_NODOOR;
			    else here->doormask = D_BROKEN;
			    /* newsym(mtmp->mx, mtmp->my); */ /* done below */
			    unblock_point(mtmp->mx,mtmp->my); /* vision */
			}
			/* if it's a shop door, schedule repair */
			if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
			    add_damage(mtmp->mx, mtmp->my, 0L);
		    }
		} else if (levl[mtmp->mx][mtmp->my].typ == IRONBARS) {
		    if (metallivorous(ptr)) {
			if (cansee(mtmp->mx, mtmp->my) && flags.verbose) {
#ifndef JP
			    Norep("%s eats iron bars!", Monnam(mtmp));
#else
			    Norep("%sは鉄格子を食べた！", Monnam(mtmp));
#endif /*JP*/
			} else if (flags.soundok && flags.verbose) {
			    You_hear(E_J("a crunching sound.","金属の砕ける音を"));
			}
			mtmp->meating = 50;
			levl[mtmp->mx][mtmp->my].typ = ROOM;
		    } else {
			if (flags.verbose && canseemon(mtmp)) {
#ifndef JP
			    Norep("%s %s %s the iron bars.", Monnam(mtmp),
				  /* pluralization fakes verb conjugation */
				  makeplural(locomotion(ptr, "pass")),
				  passes_walls(ptr) ? "through" : "between");
#else
			    Norep("%sは鉄格子%sを通り抜けた。", Monnam(mtmp),
				  passes_walls(ptr) ? "" : "の間");
#endif /*JP*/
			}
			if (mtmp->mnum == PM_OCHRE_JELLY ||
			    mtmp->mnum == PM_BLACK_PUDDING) {
			    if (flags.verbose && cansee(mtmp->mx, mtmp->my)) {
				Norep(E_J("%s dissolves the iron bars!",
				          "%sは鉄格子を溶かした！"), Monnam(mtmp));
			    }
			    levl[mtmp->mx][mtmp->my].typ = ROOM;
			}
		    }
		}

		/* possibly dig */
		if (can_tunnel && mdig_tunnel(mtmp))
			return(2);  /* mon died (position already updated) */

		/* set also in domove(), hack.c */
		if (u.uswallow && mtmp == u.ustuck &&
					(mtmp->mx != omx || mtmp->my != omy)) {
		    /* If the monster moved, then update */
		    u.ux0 = u.ux;
		    u.uy0 = u.uy;
		    u.ux = mtmp->mx;
		    u.uy = mtmp->my;
		    swallowed(0);
		} else
		newsym(mtmp->mx,mtmp->my);
	    }
	    if(OBJ_AT(mtmp->mx, mtmp->my) && mtmp->mcanmove) {
		/* recompute the likes tests, in case we polymorphed
		 * or if the "likegold" case got taken above */
		if (setlikes) {
		    register int pctload = (curr_mon_load(mtmp) * 100) /
			max_mon_load(mtmp);

		    /* look for gold or jewels nearby */
		    likegold = (likes_gold(ptr) && pctload < 95);
		    likegems = (likes_gems(ptr) && pctload < 85);
		    uses_items = (!mindless(ptr) && !is_animal(ptr)
				  && pctload < 75);
		    likeobjs = (likes_objs(ptr) && pctload < 75);
		    likemagic = (likes_magic(ptr) && pctload < 85);
		    likerock = (throws_rocks(ptr) && pctload < 50 &&
				!In_sokoban(&u.uz));
		    conceals = hides_under(ptr);
		}

		/* Maybe a rock mole just ate some metal object */
		if (metallivorous(ptr)) {
		    if (meatmetal(mtmp) == 2) return 2;	/* it died */
		}

		if(g_at(mtmp->mx,mtmp->my) && likegold) mpickgold(mtmp);

		/* Maybe a cube ate just about anything */
		if (mtmp->mnum == PM_GELATINOUS_CUBE) {
		    if (meatobj(mtmp) == 2) return 2;	/* it died */
		}

		if(!*in_rooms(mtmp->mx, mtmp->my, SHOPBASE) || !rn2(25)) {
		    boolean picked = FALSE;

		    if(likeobjs) picked |= mpickstuff(mtmp, practical);
		    if(likemagic) picked |= mpickstuff(mtmp, magical);
		    if(likerock) picked |= mpickstuff(mtmp, boulder_class);
		    if(likegems) picked |= mpickstuff(mtmp, gem_class);
		    if(uses_items) picked |= mpickstuff(mtmp, (char *)0);
		    if(picked) mmoved = 3;
		}

		if(mtmp->minvis) {
		    newsym(mtmp->mx, mtmp->my);
		    if (mtmp->wormno) see_wsegs(mtmp);
		}
	    }
	    /* Maybe a cube ate just about anything */
	    if (mtmp->mnum == PM_GELATINOUS_CUBE) {
		/* eat floor surface */
		if (levl[mtmp->mx][mtmp->my].typ == GRASS) levl[mtmp->mx][mtmp->my].typ = GROUND;
		if (levl[mtmp->mx][mtmp->my].typ == CARPET) levl[mtmp->mx][mtmp->my].typ = ROOM;
		if (mtmp->minvis) newsym(mtmp->mx, mtmp->my);
	    }

	    if(hides_under(ptr) || ptr->mlet == S_EEL) {
		/* Always set--or reset--mundetected if it's already hidden
		   (just in case the object it was hiding under went away);
		   usually set mundetected unless monster can't move.  */
		if (mtmp->mundetected ||
			(mtmp->mcanmove && !mtmp->msleeping && rn2(5)))
		    mtmp->mundetected = (ptr->mlet != S_EEL) ?
			OBJ_AT(mtmp->mx, mtmp->my) :
			(is_pool(mtmp->mx, mtmp->my) && !Is_waterlevel(&u.uz));
		newsym(mtmp->mx, mtmp->my);
	    }
	    if (mtmp->isshk) {
		after_shk_move(mtmp);
	    }
	}
	return(mmoved);
}

#endif /* OVL0 */
#ifdef OVL2

boolean
closed_door(x, y)
register int x, y;
{
	return((boolean)(IS_DOOR(levl[x][y].typ) &&
			(levl[x][y].doormask & (D_LOCKED | D_CLOSED))));
}

boolean
accessible(x, y)
register int x, y;
{
	return((boolean)(ACCESSIBLE(levl[x][y].typ) && !closed_door(x, y)));
}

#endif /* OVL2 */
#ifdef OVL0

/* decide where the monster thinks you are standing */
void
set_apparxy(mtmp)
register struct monst *mtmp;
{
	boolean notseen, gotu;
	register int disp, mx = mtmp->mux, my = mtmp->muy;

	/*
	 * do cheapest and/or most likely tests first
	 */

	/* pet knows your smell; grabber still has hold of you */
	if (mtmp->mtame || mtmp == u.ustuck) goto found_you;

	/* monsters which know where you are don't suddenly forget,
	   if you haven't moved away */
	if (mx == u.ux && my == u.uy) goto found_you;

	notseen = (!mtmp->mcansee || (Invis && !perceives(mtmp->data)));
	/* add cases as required.  eg. Displacement ... */
	if (notseen || Underwater) {
	    /* Xorns can smell valuable metal like gold, treat as seen */
	    if ((mtmp->mnum == PM_XORN) &&
			u.ugold
			&& !Underwater)
		disp = 0;
	    else
		disp = 1;
	} else if (Displaced) {
	    disp = couldsee(mx, my) ? 2 : 1;
	} else disp = 0;
	if (!disp) goto found_you;

	/* without something like the following, invis. and displ.
	   are too powerful */
	gotu = notseen ? !rn2(3) : Displaced ? !rn2(4) : FALSE;

#if 0		/* this never worked as intended & isn't needed anyway */
	/* If invis but not displaced, staying around gets you 'discovered' */
	gotu |= (!Displaced && u.dx == 0 && u.dy == 0);
#endif

	if (!gotu) {
	    register int try_cnt = 0;
	    do {
		if (++try_cnt > 200) goto found_you;		/* punt */
		mx = u.ux - disp + rn2(2*disp+1);
		my = u.uy - disp + rn2(2*disp+1);
	    } while (!isok(mx,my)
		  || (disp != 2 && mx == mtmp->mx && my == mtmp->my)
		  || ((mx != u.ux || my != u.uy) &&
		      !passes_walls(mtmp->data) &&
		      (!ACCESSIBLE(levl[mx][my].typ) ||
		       (closed_door(mx, my) && !can_ooze(mtmp))))
		  || !couldsee(mx, my));
	} else {
found_you:
	    mx = u.ux;
	    my = u.uy;
	}

	mtmp->mux = mx;
	mtmp->muy = my;

#ifdef MONSTEED
	if (is_mriding(mtmp)) {
	    mtmp->mlink->mux = mx;
	    mtmp->mlink->muy = my;
	}
#endif
}

boolean
can_ooze(mtmp)
struct monst *mtmp;
{
	struct obj *chain, *obj;

	if (!amorphous(mtmp->data)) return FALSE;
	if (mtmp == &youmonst) {
		if (u.ugold > 100L) return FALSE;
		chain = invent;
	} else {
		if (mtmp->mgold > 100L) return FALSE;
		chain = mtmp->minvent;
	}
	for (obj = chain; obj; obj = obj->nobj) {
		int typ = obj->otyp;

		if (obj->oclass != GEM_CLASS &&
		    !(typ >= ARROW && typ <= BOOMERANG) &&
		    !(typ >= DAGGER && typ <= CRYSKNIFE) &&
		    typ != SLING &&
		    !is_cloak(obj) && typ != FEDORA &&
		    !is_gloves(obj) && typ != LEATHER_JACKET &&
		    typ != CREDIT_CARD && !is_shirt(obj) &&
		    !(typ == CORPSE && verysmall(&mons[obj->corpsenm])) &&
		    typ != FORTUNE_COOKIE && typ != CANDY_BAR &&
		    typ != PANCAKE && typ != LEMBAS_WAFER &&
		    typ != LUMP_OF_ROYAL_JELLY &&
		    obj->oclass != AMULET_CLASS &&
		    obj->oclass != RING_CLASS &&
#ifdef WIZARD
		    obj->oclass != VENOM_CLASS &&
#endif
		    typ != SACK && typ != BAG_OF_HOLDING &&
		    typ != BAG_OF_TRICKS && !Is_candle(obj) &&
		    typ != OILSKIN_SACK && typ != LEASH &&
		    typ != STETHOSCOPE && typ != BLINDFOLD && typ != TOWEL &&
		    typ != TIN_WHISTLE && typ != MAGIC_WHISTLE &&
		    typ != MAGIC_MARKER && typ != TIN_OPENER &&
		    typ != SKELETON_KEY && typ != LOCK_PICK
		) return FALSE;
		if (Is_container(obj) && obj->cobj) return FALSE;
		    
	}
	return TRUE;
}

/* evade fire-line: TRUE if evaded */
boolean
evade_missile(mtmp, xx1, yy1, xx2, yy2)
struct monst *mtmp;
int xx1, yy1, xx2, yy2;
{
	int i, x, y;
	int idx, oka[8];
	idx = 0;
	if (mtmp->movement < NORMAL_SPEED ||
	    mtmp->msleeping || !mtmp->mcanmove ||
#ifdef STEED
	    mtmp == u.usteed ||
#endif
	    mtmp->mtrapped) return FALSE;
	for (i=0; i<8; i++) {
	    x = mtmp->mx + xdir[i];
	    y = mtmp->my + ydir[i];
	    if (!isok(x, y) ||
		(xx1 == x && yy1 == y) || (xx2 == x && yy2 == y) ||
		MON_AT(x, y) || dist2(x, y, mtmp->mux, mtmp->muy) <= 2 ||
		!ACCESSIBLE(levl[x][y].typ) || closed_door(x,y)) continue;
	    oka[idx++] = i;
	}
	if (!idx) return FALSE;
	x = mtmp->mx;
	y = mtmp->my;
	i = oka[rn2(idx)];
	mtmp->movement -= NORMAL_SPEED;
	rloc_to(mtmp, x + xdir[i], y + ydir[i]);
	/* avoid returning to the last position */
	for(i = MTSZ-1; i > 0; i--)
	    mtmp->mtrack[i] = mtmp->mtrack[i-1];
	mtmp->mtrack[0].x = x;
	mtmp->mtrack[0].y = y;
	return TRUE;
}

boolean
likes_midrange(mtmp)
struct monst *mtmp;
{
	struct obj *mw_tmp, *otmp;

	/* monster wielding bow with arrow */
	if ((mw_tmp = MON_WEP(mtmp)) && is_launcher(mw_tmp)) {
	    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
		if (ammo_and_launcher(otmp, mw_tmp)) return TRUE;
	    }
	}

	/* monster with ranged attack */
	if (can_breathe(mtmp->data) && mtmp->mspec_used <= 5) return TRUE;

	return FALSE;
}

#endif /* OVL0 */

/*monmove.c*/
