/*	SCCS Id: @(#)dokick.c	3.4	2003/12/04	*/
/* Copyright (c) Izchak Miller, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "eshk.h"

#define is_bigfoot(x)	((x)->mnum == PM_SASQUATCH)
#define martial()	(martial_bonus() || is_bigfoot(youmonst.data) || \
		(uarmf && uarmf->otyp == KICKING_BOOTS))

static NEARDATA struct rm *maploc;
static NEARDATA const char *gate_str;

extern boolean notonhead;	/* for long worms */

STATIC_DCL void FDECL(kickdmg, (struct monst *, BOOLEAN_P));
//STATIC_DCL void FDECL(kick_monster, (XCHAR_P, XCHAR_P));
STATIC_DCL int FDECL(kick_object, (XCHAR_P, XCHAR_P));
STATIC_DCL char *FDECL(kickstr, (char *));
STATIC_DCL void FDECL(otransit_msg, (struct obj *, BOOLEAN_P, long));
STATIC_DCL void FDECL(drop_to, (coord *,SCHAR_P));

static NEARDATA struct obj *kickobj;

static const char kick_passes_thru[] = E_J("kick passes harmlessly through",
					   "蹴りは手ごたえなく通り抜けた");

STATIC_OVL void
kickdmg(mon, clumsy)
register struct monst *mon;
register boolean clumsy;
{
	register int mdx, mdy;
	register int dmg = ( ACURRSTR + ACURR(A_DEX) + ACURR(A_CON) )/ 15;
	int kick_skill = P_NONE;
	int blessed_foot_damage = 0;
	boolean trapkilled = FALSE;

	if (uarmf && uarmf->otyp == KICKING_BOOTS)
	    dmg += 5;

	/* excessive wt affects dex, so it affects dmg */
	if (clumsy) dmg /= 2;

	/* kicking a dragon or an elephant will not harm it */
	if (thick_skinned(mon->data)) dmg = 0;

	/* attacking a shade is useless */
	if (mon->mnum == PM_SHADE)
	    dmg = 0;

	if ((is_undead(mon->data) || is_demon(mon->data)) && uarmf &&
		uarmf->blessed)
	    blessed_foot_damage = 1;

	if (mon->mnum == PM_SHADE && !blessed_foot_damage) {
#ifndef JP
	    pline_The("%s.", kick_passes_thru);
#else
	    pline("%s。", kick_passes_thru);
#endif /*JP*/
	    /* doesn't exercise skill or abuse alignment or frighten pet,
	       and shades have no passive counterattack */
	    return;
	}

	if(mon->m_ap_type) seemimic(mon);

	check_caitiff(mon);

	/* squeeze some guilt feelings... */
	if(mon->mtame) {
	    abuse_dog(mon);
	    if (mon->mtame)
		monflee(mon, (dmg ? rnd(dmg) : 1), FALSE, FALSE);
	    else
		mon->mflee = 0;
	}

	if (dmg > 0) {
		/* convert potential damage to actual damage */
		dmg = rnd(dmg);
		if (martial()) {
		    if (dmg > 1) kick_skill = P_MARTIAL_ARTS;
		    dmg += rn2(ACURR(A_DEX)/3 + 1);
		}
		/* a good kick exercises your dex */
		exercise(A_DEX, TRUE);
	}
	if (blessed_foot_damage) dmg += rnd(4);
	if (uarmf) dmg += uarmf->spe;
	dmg += udaminc_bonus();	/* add ring(s) of increase damage */
	if (dmg > 0)
		mlosehp(mon, dmg);
	else
		Your(E_J("kick does no damage.",
			 "蹴りはダメージを与えられなかった。"));
	if (dmg > 0 && mon->mhp > 0 && martial()) {
	    /* hurtle */
	    if (!bigmonst(mon->data) && !rn2(3) &&
		mon->mcanmove && mon != u.ustuck && !mon->mtrapped) {
		/* see if the monster has a place to move into */
		mdx = mon->mx + u.dx;
		mdy = mon->my + u.dy;
		if(goodpos(mdx, mdy, mon, 0)) {
			pline(E_J("%s reels from the blow.",
				  "%sは衝撃でよろめいた。"), Monnam(mon));
			if (m_in_out_region(mon, mdx, mdy)) {
			    remove_monster(mon->mx, mon->my);
			    newsym(mon->mx, mon->my);
			    place_monster(mon, mdx, mdy);
			    newsym(mon->mx, mon->my);
			    set_apparxy(mon);
			    if (mintrap(mon) == 2) trapkilled = TRUE;
			}
		}
	    } else if (rn2(150) < P_SKILL(P_MARTIAL_ARTS)) {
#ifndef JP
		pline("%s loses %s balance from the blow.", Monnam(mon), mhis(mon));
#else
		pline("%sは体勢を崩した。", Monnam(mon));
#endif
		mon->movement = 0;
		mon->mstun = 1;
	    }
	}

#ifdef SHOWDMG
	if (flags.showdmg && !trapkilled && mon->mhp > 0 && dmg > 0)
		pline("(%dpt%s)", dmg, (dmg == 1 ? "" : "s"));
#endif

	(void) passive(mon, TRUE, mon->mhp > 0, AT_KICK, 0);
	if (mon->mhp <= 0 && !trapkilled) killed_showdmg(mon, dmg);

	/* may bring up a dialog, so put this after all messages */
	if (kick_skill != P_NONE)	/* exercise proficiency */
	    use_skill(kick_skill, 1);
}

void
kick_monster(x, y)
register xchar x, y;
{
	register boolean clumsy = FALSE;
	register struct monst *mon = m_at(x, y);
	register int i, j;
	int sum;

	bhitpos.x = x;
	bhitpos.y = y;
	if (attack_checks(mon, (struct obj *)0)) return;
	setmangry(mon);

	/* Kick attacks by kicking monsters are normal attacks, not special.
	 * This is almost always worthless, since you can either take one turn
	 * and do all your kicks, or else take one turn and attack the monster
	 * normally, getting all your attacks _including_ all your kicks.
	 * If you have >1 kick attack, you get all of them.
	 */
	if (Upolyd && attacktype(youmonst.data, AT_KICK)) {
	    struct attack *uattk;
	    schar tmp = find_roll_to_hit(mon, 0);
	    int die;

	    for (i = 0; i < NATTK; i++) {
		/* first of two kicks might have provoked counterattack
		   that has incapacitated the hero (ie, floating eye) */
		if (multi < 0) break;

		uattk = &youmonst.data->mattk[i];
		/* we only care about kicking attacks here */
		if (uattk->aatyp != AT_KICK) continue;

		if (mon->mnum == PM_SHADE &&
			(!uarmf || !uarmf->blessed)) {
		    /* doesn't matter whether it would have hit or missed,
		       and shades have no passive counterattack */
#ifndef JP
		    Your("%s %s.", kick_passes_thru, mon_nam(mon));
#else
		    Your("蹴りは手ごたえなく%sを通り抜けた。", mon_nam(mon));
#endif /*JP*/
		    break;	/* skip any additional kicks */
		} else if (tmp > (die = rnd(20))) {
		    You(E_J("kick %s.","%sを蹴った。"), mon_nam(mon));
		    sum = damageum(mon, uattk);
		    (void)passive(mon, (boolean)(sum > 0), (sum != 2), AT_KICK, 0);
		    if (sum == 2)
			break;		/* Defender died */
		} else {
		    missum(mon, uattk, die - tmp);
		    (void)passive(mon, 0, 1, AT_KICK, 0);
		}
	    }
	    return;
//	} else if (uarmf && uarmf->otyp == KICKING_BOOTS) {
//	    if (find_roll_to_hit(mon, 0) > rnd(20)) {
//		You("kick %s.", mon_nam(mon));
//		kickdmg(mon, FALSE);
//	    }
//	    return;
	}


	if(Levitation && !rn2(3) && verysmall(mon->data) &&
	   !is_flying(mon)) {
		pline(E_J("Floating in the air, you miss wildly!",
			  "宙に浮いているせいで、あなたは大きく目標を外した！"));
		exercise(A_DEX, FALSE);
		(void) passive(mon, FALSE, 1, AT_KICK, 0);
		return;
	}

	i = -inv_weight();
	j = weight_cap();

	if(i < (j*3)/10) {
		if(!rn2((i < j/10) ? 2 : (i < j/5) ? 3 : 4)) {
			if(martial() && !rn2(2)) goto doit;
			Your(E_J("clumsy kick does no damage.",
				 "貧弱な蹴りではダメージを与えられなかった。"));
			(void) passive(mon, FALSE, 1, AT_KICK, 0);
			return;
		}
		if(i < j/10) clumsy = TRUE;
		else if(!rn2((i < j/5) ? 2 : 3)) clumsy = TRUE;
	}

	if(Fumbling) clumsy = TRUE;

	else if(uarm && objects[uarm->otyp].oc_bulky && ACURR(A_DEX) < rnd(25))
		clumsy = TRUE;
doit:
	You(E_J("kick %s.","%sを蹴った。"), mon_nam(mon));
	if(!rn2(clumsy ? 3 : 4) && (clumsy || !bigmonst(mon->data)) &&
	   mon->mcansee && !mon->mtrapped && !thick_skinned(mon->data) &&
	   mon->data->mlet != S_EEL && haseyes(mon->data) && mon->mcanmove &&
	   !mon->mstun && !mon->mconf && !mon->msleeping &&
	   mon->data->mmove >= 12) {
		if(!nohands(mon->data) && !rn2(martial() ? 5 : 3)) {
		    pline(E_J("%s blocks your %skick.",
			      "%sはあなたの%s蹴りを防いだ。"), Monnam(mon),
				clumsy ? E_J("clumsy ","貧弱な") : "");
		    (void) passive(mon, FALSE, 1, AT_KICK, 0);
		    return;
		} else {
		    mnexto(mon);
		    if(mon->mx != x || mon->my != y) {
			boolean cantele = !(level.flags.noteleport) &&
					   can_teleport(mon->data);
			if(glyph_is_invisible(levl[x][y].glyph)) {
			    unmap_object(x, y);
			    newsym(x, y);
			}
#ifndef JP
			pline("%s %s, %s evading your %skick.", Monnam(mon),
				(cantele ? "teleports" :
				 is_floating(mon) ? "floats" :
				 is_flying(mon) ? "swoops" :
				 (nolimbs(mon->data) || slithy(mon->data)) ?
					"slides" : "jumps"),
				clumsy ? "easily" : "nimbly",
				clumsy ? "clumsy " : "");
#else
			pline("%sは%s、あなたの%s蹴りを%sかわした。", Monnam(mon),
				(cantele ? "テレポートし" :
				 is_floating(mon) ? "浮き上がり" :
				 is_flying(mon) ? "急降下し" :
				 (nolimbs(mon->data) || slithy(mon->data)) ?
					"脇へと滑り" : "飛び跳ね"),
				clumsy ? "貧弱な" : "",
				clumsy ? "やすやすと" : "素早く");
#endif /*JP*/
			(void) passive(mon, FALSE, 1, AT_KICK, 0);
			return;
		    }
		}
	}
	kickdmg(mon, clumsy);
}

/*
 *  Return TRUE if caught (the gold taken care of), FALSE otherwise.
 *  The gold object is *not* attached to the fobj chain!
 */
boolean
ghitm(mtmp, gold)
register struct monst *mtmp;
register struct obj *gold;
{
	boolean msg_given = FALSE;

	if(!likes_gold(mtmp->data) && !mtmp->isshk && !mtmp->ispriest
			&& !is_mercenary(mtmp->data)) {
		wakeup(mtmp);
	} else if (!mtmp->mcanmove) {
		/* too light to do real damage */
		if (canseemon(mtmp)) {
#ifndef JP
		    pline_The("%s harmlessly %s %s.", xname(gold),
			      otense(gold, "hit"), mon_nam(mtmp));
#else
		    pline("%sは%sに当たったが、傷は負わせられなかった。",
			  xname(gold), mon_nam(mtmp));
#endif /*JP*/
		    msg_given = TRUE;
		}
	} else {
		mtmp->msleeping = 0;
		mtmp->meating = 0;
		if(!rn2(4)) setmangry(mtmp); /* not always pleasing */

		/* greedy monsters catch gold */
		if (cansee(mtmp->mx, mtmp->my))
		    pline(E_J("%s catches the gold.",
			      "%sは金貨を受け止めた。"), Monnam(mtmp));
		mtmp->mgold += gold->quan;

		if (mtmp->isshk) {
			struct eshk *ep = ESHK(mtmp);
			long robbed = ep->robbed;

			if (robbed) {
				robbed -= gold->quan;
				if (robbed < 0) robbed = 0;
#ifndef JP
				pline_The("amount %scovers %s recent losses.",
				      !robbed ? "" : "partially ",
				      mhis(mtmp));
#else
				pline("店主の最近の損害%sがこれで補填された。",
				      !robbed ? "" : "のいくらか");
#endif /*JP*/
				ep->robbed = robbed;
				if(!robbed)
					make_happy_shk(mtmp, FALSE, FALSE);
			} else {
				if(mtmp->mpeaceful) {
				    ep->credit += gold->quan;
				    You(E_J("have %ld %s in credit.",
					    "%ld %sのクレジットを得た。"),
					ep->credit,
					currency(ep->credit));
				} else verbalize(E_J("Thanks, scum!","ありがとうよ、クズめ！"));
			}
		} else if (mtmp->ispriest) {
			if (mtmp->mpeaceful)
			    verbalize(E_J("Thank you for your contribution.",
					  "あなたの献身に感謝します。"));
			else verbalize(E_J("Thanks, scum!","ありがとうよ、クズめ！"));
		} else if (is_mercenary(mtmp->data)) {
		    long goldreqd = 0L;

		    if (rn2(3)) {
			if (mtmp->mnum == PM_SOLDIER)
			   goldreqd = 100L;
			else if (mtmp->mnum == PM_SERGEANT)
			   goldreqd = 250L;
			else if (mtmp->mnum == PM_LIEUTENANT)
			   goldreqd = 500L;
			else if (mtmp->mnum == PM_CAPTAIN)
			   goldreqd = 750L;

			if (goldreqd) {
			   if (gold->quan > goldreqd +
				(u.ugold + u.ulevel*rn2(5))/ACURR(A_CHA))
			    mtmp->mpeaceful = TRUE;
			}
		     }
		     if (mtmp->mpeaceful)
			    verbalize(E_J("That should do.  Now beat it!",
					  "まあ、いいだろう。さっさと失せろ！"));
		     else verbalize(E_J("That's not enough, coward!",
					"これでは足りんな、臆病者め！"));
		}

		dealloc_obj(gold);
		return TRUE;
	}

	if (!msg_given) miss(xname(gold), mtmp);
	return FALSE;
}

/* container is kicked, dropped, thrown or otherwise impacted by player.
 * Assumes container is on floor.  Checks contents for possible damage. */
void
container_impact_dmg(obj)
struct obj *obj;
{
	struct monst *shkp;
	struct obj *otmp, *otmp2;
	long loss = 0L;
	boolean costly, insider;
	xchar x = obj->ox, y = obj->oy;

	/* only consider normal containers */
	if (!Is_container(obj) || Is_mbag(obj)) return;

	costly = ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) &&
		  costly_spot(x, y));
	insider = (*u.ushops && inside_shop(u.ux, u.uy) &&
		   *in_rooms(x, y, SHOPBASE) == *u.ushops);

	for (otmp = obj->cobj; otmp; otmp = otmp2) {
	    const char *result = (char *)0;

	    otmp2 = otmp->nobj;
	    if (get_material(otmp) == GLASS &&
		otmp->oclass != GEM_CLASS && !obj_resists(otmp, 33, 100)) {
		result = E_J("shatter","割れた");
	    } else if (otmp->otyp == EGG && !rn2(3)) {
		result = E_J("cracking","潰れた");
	    }
	    if (result) {
		if (otmp->otyp == MIRROR) change_luck(-2);

		/* eggs laid by you.  penalty is -1 per egg, max 5,
		 * but it's always exactly 1 that breaks */
		if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
		    change_luck(-1);
#ifndef JP
		You_hear("a muffled %s.", result);
#else
		pline("中で何かが%s音がした。", result);
#endif /*JP*/
		if (costly)
		    loss += stolen_value(otmp, x, y,
					 (boolean)shkp->mpeaceful, TRUE);
		if (otmp->quan > 1L)
		    useup(otmp);
		else {
		    obj_extract_self(otmp);
		    obfree(otmp, (struct obj *) 0);
		}
	    }
	}
	if (costly && loss) {
	    if (!insider) {
		You(E_J("caused %ld %s worth of damage!",
			"%ld %s分の損害を引き起こした！"), loss, currency(loss));
		make_angry_shk(shkp, x, y);
	    } else {
		You(E_J("owe %s %ld %s for objects destroyed.",
			"%sに%ld %s分の損害賠償責任を負った。"),
		    mon_nam(shkp), loss, currency(loss));
	    }
	}
}

STATIC_OVL int
kick_object(x, y)
xchar x, y;
{
	int range;
	register struct monst *mon, *shkp;
	struct trap *trap;
	char bhitroom;
	boolean costly, isgold, slide = FALSE;

	/* if a pile, the "top" object gets kicked */
	kickobj = level.objects[x][y];

	/* kickobj should always be set due to conditions of call */
	if(!kickobj || kickobj->otyp == BOULDER
			|| kickobj == uball || kickobj == uchain)
		return(0);

	if ((trap = t_at(x,y)) != 0 &&
			(((trap->ttyp == PIT ||
			   trap->ttyp == SPIKED_PIT) && !Passes_walls) ||
			 trap->ttyp == WEB)) {
		if (!trap->tseen) find_trap(trap);
#ifndef JP
		You_cant("kick %s that's in a %s!", something,
			 Hallucination ? "tizzy" :
			 (trap->ttyp == WEB) ? "web" : "pit");
#else
		You("%sっているものを蹴ることはできない！",
			 Hallucination ? "テンパ" :
			 (trap->ttyp == WEB) ? "蜘蛛の巣にかか" : "落とし穴にはま");
#endif /*JP*/
		return 1;
	}

	if(Fumbling && !rn2(3)) {
		Your(E_J("clumsy kick missed.","貧弱な蹴りは命中しなかった。"));
		return(1);
	}

	if(kickobj->otyp == CORPSE && touch_petrifies(&mons[kickobj->corpsenm])
			&& !Stone_resistance && !uarmf) {
	    char kbuf[BUFSZ];

#ifndef JP
	    You("kick the %s with your bare %s.",
		corpse_xname(kickobj, TRUE), makeplural(body_part(FOOT)));
#else
	    You("%sを素%sで蹴ってしまった。",
		corpse_xname(kickobj, TRUE), body_part(FOOT));
#endif /*JP*/
	    if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
		You(E_J("turn to stone...","石になった…。"));
		killer_format = KILLED_BY;
		/* KMH -- otmp should be kickobj */
#ifndef JP
		Sprintf(kbuf, "kicking %s without boots",
			an(corpse_xname(kickobj, TRUE)));
#else
		Sprintf(kbuf, "ブーツなしで%sを蹴って",
			corpse_xname(kickobj, TRUE));
#endif /*JP*/
		killer = kbuf;
		done(STONING);
	    }
	}

	/* range < 2 means the object will not move.	*/
	/* maybe dexterity should also figure here.     */
	range = (int)((ACURRSTR)/2 - kickobj->owt/40);

	if(martial()) range += rnd(3);

	if (is_pool(x, y)) {
	    /* you're in the water too; significantly reduce range */
	    range = range / 3 + 1;	/* {1,2}=>1, {3,4,5}=>2, {6,7,8}=>3 */
	} else {
	    if (is_ice(x, y)) range += rnd(3),  slide = TRUE;
	    if (kickobj->greased) range += rnd(3),  slide = TRUE;
	}

	/* Mjollnir is magically too heavy to kick */
	if(kickobj->oartifact == ART_MJOLLNIR) range = 1;

	/* see if the object has a place to move into */
	if(!ZAP_POS(levl[x+u.dx][y+u.dy].typ) || closed_door(x+u.dx, y+u.dy))
		range = 1;

	costly = ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) &&
				    costly_spot(x, y));
	isgold = (kickobj->oclass == COIN_CLASS);

	if (IS_ROCK(levl[x][y].typ) || closed_door(x, y)) {
	    if ((!martial() && rn2(20) > ACURR(A_DEX)) ||
		    IS_ROCK(levl[u.ux][u.uy].typ) || closed_door(u.ux, u.uy)) {
		if (Blind)
		    pline(E_J("It doesn't come loose.",
			      "何かは引っかかったままだ。"));
		else
#ifndef JP
		    pline("%s %sn't come loose.",
			  The(distant_name(kickobj, xname)),
			  otense(kickobj, "do"));
#else
		    pline("%sは引っかかったままだ。",
			  distant_name(kickobj, xname));
#endif /*JP*/
		return (!rn2(3) || martial());
	    }
	    if (Blind)
		pline(E_J("It comes loose.","何かが取れるようになった。"));
	    else
#ifndef JP
		pline("%s %s loose.",
		      The(distant_name(kickobj, xname)),
		      otense(kickobj, "come"));
#else
		pline("%sが取れるようになった。",
		      distant_name(kickobj, xname));
#endif /*JP*/
	    obj_extract_self(kickobj);
	    newsym(x, y);
	    if (costly && (!costly_spot(u.ux, u.uy) ||
		    !index(u.urooms, *in_rooms(x, y, SHOPBASE))))
		addtobill(kickobj, FALSE, FALSE, FALSE);
	    if (!flooreffects(kickobj, u.ux, u.uy, E_J("fall","落ちた"))) {
		place_object(kickobj, u.ux, u.uy);
		stackobj(kickobj);
		newsym(u.ux, u.uy);
	    }
	    return 1;
	}

	/* a box gets a chance of breaking open here */
	if(Is_box(kickobj)) {
		boolean otrp = kickobj->otrapped;

		if(range < 2) pline(E_J("THUD!","ガン！"));

		container_impact_dmg(kickobj);

		if (kickobj->olocked) {
		    if (!rn2(5) || (martial() && !rn2(2))) {
			You(E_J("break open the lock!",
				"錠を壊した！"));
			kickobj->olocked = 0;
			kickobj->obroken = 1;
			if (otrp) (void) chest_trap(kickobj, LEG, FALSE);
			return(1);
		    }
		} else {
		    if (!rn2(3) || (martial() && !rn2(2))) {
			pline_The(E_J("lid slams open, then falls shut.",
				      "蓋が勢いよく開き、そして閉まった。"));
			if (otrp) (void) chest_trap(kickobj, LEG, FALSE);
			return(1);
		    }
		}
		if(range < 2) return(1);
		/* else let it fall through to the next cases... */
	}

	/* fragile objects should not be kicked */
	if (hero_breaks(kickobj, kickobj->ox, kickobj->oy, FALSE)) return 1;

	/* too heavy to move.  range is calculated as potential distance from
	 * player, so range == 2 means the object may move up to one square
	 * from its current position
	 */
	if(range < 2 || (isgold && kickobj->quan > 300L)) {
	    if(!Is_box(kickobj)) pline(E_J("Thump!","ドスッ！"));
	    return(!rn2(3) || martial());
	}

	if (kickobj->quan > 1L && !isgold) kickobj = splitobj(kickobj, 1L);

	if (slide && !Blind)
#ifndef JP
	    pline("Whee!  %s %s across the %s.", Doname2(kickobj),
		  otense(kickobj, "slide"), surface(x,y));
#else
	    pline("イェー！ %sは%sの上を滑っていった。",
		  doname(kickobj), surface(x,y));
#endif /*JP*/

	obj_extract_self(kickobj);
	(void) snuff_candle(kickobj);
	newsym(x, y);
	mon = bhit(u.dx, u.dy, range, KICKED_WEAPON,
		   (int FDECL((*),(MONST_P,OBJ_P)))0,
		   (int FDECL((*),(OBJ_P,OBJ_P)))0,
		   kickobj);

	if(mon) {
	    if (mon->isshk &&
		    kickobj->where == OBJ_MINVENT && kickobj->ocarry == mon)
		return 1;	/* alert shk caught it */
	    notonhead = (mon->mx != bhitpos.x || mon->my != bhitpos.y);
	    if (isgold ? ghitm(mon, kickobj) :	/* caught? */
		    thitmonst(mon, kickobj))	/* hit && used up? */
		return(1);
	}

	/* the object might have fallen down a hole */
	if (kickobj->where == OBJ_MIGRATING) {
	    if (costly) {
		if(isgold)
		    costly_gold(x, y, kickobj->quan);
		else (void)stolen_value(kickobj, x, y,
					(boolean)shkp->mpeaceful, FALSE);
	    }
	    return 1;
	}

	bhitroom = *in_rooms(bhitpos.x, bhitpos.y, SHOPBASE);
	if (costly && (!costly_spot(bhitpos.x, bhitpos.y) ||
			*in_rooms(x, y, SHOPBASE) != bhitroom)) {
	    if(isgold)
		costly_gold(x, y, kickobj->quan);
	    else (void)stolen_value(kickobj, x, y,
				    (boolean)shkp->mpeaceful, FALSE);
	}

	if(flooreffects(kickobj,bhitpos.x,bhitpos.y,E_J("fall","落ちた"))) return(1);
	place_object(kickobj, bhitpos.x, bhitpos.y);
	stackobj(kickobj);
	newsym(kickobj->ox, kickobj->oy);
	return(1);
}

STATIC_OVL char *
kickstr(buf)
char *buf;
{
	const char *what;

	if (kickobj) what = distant_name(kickobj,doname);
	else what = placenam(maploc->typ);
#ifndef JP
	return strcat(strcpy(buf, "kicking "), what);
#else
	return strcat(strcpy(buf, what), "を蹴って");
#endif /*JP*/
}

int
dokick()
{
	int x, y;
	int avrg_attrib;
	register struct monst *mtmp;
	boolean no_kick = FALSE;
	char buf[BUFSZ];

	if (nolimbs(youmonst.data) || slithy(youmonst.data)) {
		You(E_J("have no legs to kick with.",
			"蹴るための脚を持っていない。"));
		no_kick = TRUE;
	} else if (verysmall(youmonst.data)) {
		You(E_J("are too small to do any kicking.",
			"何かを蹴るには身体が小さすぎる。"));
		no_kick = TRUE;
#ifdef STEED
	} else if (u.usteed) {
		if (yn_function(E_J("Kick your steed?",
				    "乗騎を蹴りますか？"), ynchars, 'y') == 'y') {
		    You(E_J("kick %s.","%sを蹴った。"), mon_nam(u.usteed));
		    kick_steed();
		    return 1;
		} else {
		    return 0;
		}
#endif
	} else if (Wounded_legs) {
		/* note: jump() has similar code */
		long wl = (EWounded_legs & BOTH_SIDES);
		const char *bp = body_part(LEG);

#ifndef JP
		if (wl == BOTH_SIDES) bp = makeplural(bp);
		Your("%s%s %s in no shape for kicking.",
		     (wl == LEFT_SIDE) ? "left " :
			(wl == RIGHT_SIDE) ? "right " : "",
		     bp, (wl == BOTH_SIDES) ? "are" : "is");
#else
		Your("%s%sは蹴りを放てるような状態ではない。",
			(wl == LEFT_SIDE) ? "左" :
			(wl == RIGHT_SIDE) ? "右" : "両", bp);
#endif /*JP*/
		no_kick = TRUE;
	} else if (near_capacity() > SLT_ENCUMBER) {
		Your(E_J("load is too heavy to balance yourself for a kick.",
			 "荷物は重すぎて、蹴りを放つとバランスが取れない。"));
		no_kick = TRUE;
	} else if (youmonst.data->mlet == S_LIZARD) {
		Your(E_J("legs cannot kick effectively.",
			 "脚では効果的な蹴りは放てない。"));
		no_kick = TRUE;
	} else if (u.uinwater && !rn2(2)) {
		Your(E_J("slow motion kick doesn't hit anything.",
			 "のろい蹴りでは何にも命中しない。"));
		no_kick = TRUE;
	} else if (u.utrap) {
		switch (u.utraptype) {
		    case TT_PIT:
			pline(E_J("There's not enough room to kick down here.",
				  "この狭い穴の中では、蹴るための空間が取れない。"));
			break;
		    case TT_WEB:
		    case TT_BEARTRAP:
			You_cant(E_J("move your %s!","%sを動かせない！"), body_part(LEG));
			break;
		    default:
			break;
		}
		no_kick = TRUE;
	}

	if (no_kick) {
		/* ignore direction typed before player notices kick failed */
		force_more();
		return 0;
	}

	if(!getdir((char *)0)) return(0);
	if(!u.dx && !u.dy) return(0);

	x = u.ux + u.dx;
	y = u.uy + u.dy;

	/* KMH -- Kicking boots always succeed */
	if (uarmf && uarmf->otyp == KICKING_BOOTS)
	    avrg_attrib = 99;
	else
	    avrg_attrib = (ACURRSTR+ACURR(A_DEX)+ACURR(A_CON))/3;

	if(u.uswallow) {
		switch(rn2(3)) {
		case 0:  You_cant(E_J("move your %s!","%sを動かせない！"), body_part(LEG));
			 break;
		case 1:  if (is_animal(u.ustuck->data)) {
				pline(E_J("%s burps loudly.",
					  "%sは大きくゲップをした。"), Monnam(u.ustuck));
				break;
			 }
		default: Your(E_J("feeble kick has no effect.",
				  "弱々しい蹴りはなんの効果もなかった。")); break;
		}
		return(1);
	}
	if (Levitation) {
		int xx, yy;

		xx = u.ux - u.dx;
		yy = u.uy - u.dy;
		/* doors can be opened while levitating, so they must be
		 * reachable for bracing purposes
		 * Possible extension: allow bracing against stuff on the side?
		 */
		if (isok(xx,yy) && !IS_ROCK(levl[xx][yy].typ) &&
			!IS_DOOR(levl[xx][yy].typ) &&
			(!Is_airlevel(&u.uz) || !OBJ_AT(xx,yy))) {
		    You(E_J("have nothing to brace yourself against.",
			    "蹴りの軸足を支えることができない。"));
		    return(0);
		}
	}

	wake_nearby();
	u_wipe_engr(2);

	if (!isok(x, y)) goto dumb;

	maploc = &levl[x][y];

	/* The next five tests should stay in    */
	/* their present order: monsters, pools, */
	/* objects, non-doors, doors.		 */

	if(MON_AT(x, y)) {
		struct permonst *mdat;

		mtmp = m_at(x, y);
		mdat = mtmp->data;
		if (!mtmp->mpeaceful || !canspotmon(mtmp))
		    flags.forcefight = TRUE; /* attack even if invisible */
		kick_monster(x, y);
		flags.forcefight = FALSE;
		/* see comment in attack_checks() */
		if (!DEADMONSTER(mtmp) &&
		    !canspotmons(mtmp) &&
		    /* check x and y; a monster that evades your kick by
		       jumping to an unseen square doesn't leave an I behind */
		    mtmp->mx == x && mtmp->my == y &&
		    !glyph_is_invisible(levl[x][y].glyph) &&
		    !(u.uswallow && mtmp == u.ustuck))
			map_invisible(x, y);
		if((Is_airlevel(&u.uz) || Levitation) && flags.move) {
		    int range;

		    range = ((int)youmonst.data->cwt + (weight_cap() + inv_weight()));
		    if (range < 1) range = 1; /* divide by zero avoidance */
		    range = (3*(int)mdat->cwt) / range;

		    if(range < 1) range = 1;
		    hurtle(-u.dx, -u.dy, range, TRUE);
		}
		return(1);
	}
	if (glyph_is_invisible(levl[x][y].glyph)) {
		unmap_object(x, y);
		newsym(x, y);
	}
	if (is_pool(x, y) ^ !!u.uinwater) {
		/* objects normally can't be removed from water by kicking */
		You(E_J("splash some water around.","水をはね散らかした。"));
		return 1;
	}

	kickobj = (struct obj *)0;
	if (OBJ_AT(x, y) &&
	    (!Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
	     || sobj_at(BOULDER,x,y))) {
		if(kick_object(x, y)) {
		    if(Is_airlevel(&u.uz))
			hurtle(-u.dx, -u.dy, 1, TRUE); /* assume it's light */
		    return(1);
		}
		goto ouch;
	}

	if(!IS_DOOR(maploc->typ)) {
		if(maploc->typ == SDOOR) {
		    if(!Levitation && rn2(30) < avrg_attrib) {
			cvt_sdoor_to_door(maploc);	/* ->typ = DOOR */
#ifndef JP
			pline("Crash!  %s a secret door!",
			      /* don't "kick open" when it's locked
				 unless it also happens to be trapped */
			(maploc->doormask & (D_LOCKED|D_TRAPPED)) == D_LOCKED ?
			      "Your kick uncovers" : "You kick open");
#else
			pline((maploc->doormask & (D_LOCKED|D_TRAPPED)) == D_LOCKED ?
			      "あなたの蹴りで隠し扉があらわになった！" :
			      "あなたは隠し扉を蹴り開けた！");
#endif /*JP*/
			exercise(A_DEX, TRUE);
			if(maploc->doormask & D_TRAPPED) {
			    maploc->doormask = D_NODOOR;
			    b_trapped(E_J("door","扉"), FOOT);
			} else if (maploc->doormask != D_NODOOR &&
				   !(maploc->doormask & D_LOCKED))
			    maploc->doormask = D_ISOPEN;
			if (Blind)
			    feel_location(x,y);	/* we know it's gone */
			else
			    newsym(x,y);
			if (maploc->doormask == D_ISOPEN ||
			    maploc->doormask == D_NODOOR)
			    unblock_point(x,y);	/* vision */
			return(1);
		    } else goto ouch;
		}
		if(maploc->typ == SCORR) {
		    if(!Levitation && rn2(30) < avrg_attrib) {
			pline(E_J("Crash!  You kick open a secret passage!",
				  "あなたは隠し通路を蹴り開けた！"));
			exercise(A_DEX, TRUE);
			maploc->typ = CORR;
			if (Blind)
			    feel_location(x,y);	/* we know it's gone */
			else
			    newsym(x,y);
			unblock_point(x,y);	/* vision */
			return(1);
		    } else goto ouch;
		}
		if(IS_THRONE(maploc->typ)) {
		    register int i;
		    if(Levitation) goto dumb;
		    if((Luck < 0 || maploc->doormask) && !rn2(3)) {
			maploc->typ = ROOM;
			maploc->doormask = 0; /* don't leave loose ends.. */
			(void) mkgold((long)rnd(200), x, y);
			if (Blind)
			    pline(E_J("CRASH!  You destroy it.",
				      "すさまじい音を立てて、あなたは何かを壊した。"));
			else {
			    pline(E_J("CRASH!  You destroy the throne.",
				      "すさまじい音を立てて、あなたは玉座を壊した。"));
			    newsym(x, y);
			}
			exercise(A_DEX, TRUE);
			return(1);
		    } else if(Luck > 0 && !rn2(3) && !maploc->looted) {
			(void) mkgold((long) rn1(201, 300), x, y);
			i = Luck + 1;
			if(i > 6) i = 6;
			while(i--)
			    (void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL,
					LUCKSTONE-1), x, y, FALSE, TRUE);
			if (Blind)
#ifndef JP
			    You("kick %s loose!", something);
#else
			    Your("蹴りで何かが外れた！");
#endif /*JP*/
			else {
#ifndef JP
			    You("kick loose some ornamental coins and gems!");
#else
			    Your("蹴りでいくつかの貴金属や宝飾品が外れた！");
#endif /*JP*/
			    newsym(x, y);
			}
			/* prevent endless milking */
			maploc->looted = T_LOOTED;
			return(1);
		    } else if (!rn2(4)) {
			if(dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz)/*-1*/) { /* ctown */
			    fall_through(FALSE);
			    return(1);
			} else goto ouch;
		    }
		    goto ouch;
		}
		if(IS_ALTAR(maploc->typ)) {
		    if(Levitation) goto dumb;
		    You(E_J("kick %s.","%sを蹴った。"),(Blind ? something : E_J("the altar","祭壇")));
		    if(!rn2(3)) goto ouch;
		    altar_wrath(x, y);
		    exercise(A_DEX, TRUE);
		    return(1);
		}
		if(IS_FOUNTAIN(maploc->typ)) {
		    if(Levitation) goto dumb;
		    You(E_J("kick %s.","%sを蹴った。"),(Blind ? something : E_J("the fountain","泉")));
		    if(!rn2(3)) goto ouch;
		    /* make metal boots rust */
		    if(uarmf && rn2(3))
			if (!rust_dmg(uarmf, E_J("metal boots","金属製の靴"), 1, FALSE, &youmonst)) {
				Your(E_J("boots get wet.","靴が濡れた。"));
				/* could cause short-lived fumbling here */
			}
		    exercise(A_DEX, TRUE);
		    return(1);
		}
		if(IS_GRAVE(maploc->typ) || maploc->typ == IRONBARS)
		    goto ouch;
		if(IS_TREE(maploc->typ)) {
		    struct obj *treefruit;
		    /* nothing, fruit or trouble? 75:23.5:1.5% */
		    if (rn2(3)) {
			if ( !rn2(6) && !(mvitals[PM_KILLER_BEE].mvflags & G_GONE) )
			    You_hear(E_J("a low buzzing.","低い羽音のうなりを")); /* a warning */
			goto ouch;
		    }
		    if (rn2(15) && !(maploc->looted & TREE_LOOTED) &&
			  (treefruit = rnd_treefruit_at(x, y))) {
			long nfruit = 8L-rnl(7), nfall;
			short frtype = treefruit->otyp;
			treefruit->quan = nfruit;
			if (is_plural(treefruit))
			    pline(E_J("Some %s fall from the tree!",
				      "木から%sがいくつか落ちてきた！"), xname(treefruit));
			else
#ifndef JP
			    pline("%s falls from the tree!", An(xname(treefruit)));
#else
			    pline("木から%sが落ちてきた！", xname(treefruit));
#endif /*JP*/
			nfall = scatter(x,y,2,MAY_HIT,treefruit);
			if (nfall != nfruit) {
			    /* scatter left some in the tree, but treefruit
			     * may not refer to the correct object */
			    treefruit = mksobj(frtype, TRUE, FALSE);
			    treefruit->quan = nfruit-nfall;
#ifndef JP
			    pline("%ld %s got caught in the branches.",
				nfruit-nfall, xname(treefruit));
#else
			    pline("%ld%sの%sが枝に引っかかった。",
				nfruit-nfall, jjosushi(treefruit), xname(treefruit));
#endif /*JP*/
			    dealloc_obj(treefruit);
			}
			exercise(A_DEX, TRUE);
			exercise(A_WIS, TRUE);	/* discovered a new food source! */
			newsym(x, y);
			maploc->looted |= TREE_LOOTED;
			return(1);
		    } else if (!(maploc->looted & TREE_SWARM)) {
		    	int cnt = rnl(4) + 2;
			int made = 0;
		    	coord mm;
		    	mm.x = x; mm.y = y;
			while (cnt--) {
			    if (enexto(&mm, mm.x, mm.y, &mons[PM_KILLER_BEE])
				&& makemon(&mons[PM_KILLER_BEE],
					       mm.x, mm.y, MM_ANGRY))
				made++;
			}
			if ( made )
			    pline(E_J("You've attracted the tree's former occupants!",
				      "あなたはこの木の先住者を呼び寄せてしまった！"));
			else
			    You(E_J("smell stale honey.","古びた蜂蜜の臭いを嗅いだ。"));
			maploc->looted |= TREE_SWARM;
			return(1);
		    }
		    goto ouch;
		}
		if(IS_DEADTREE(maploc->typ)) {
		    if(Levitation) goto dumb;
		    You(E_J("kick %s.","%sを蹴った。"),(Blind ? something : E_J("the dead tree","枯れ木")));
		    switch (rn2(4)) {
			case 0:	goto ouch;
			case 1:	pline(E_J("The tree totters...","木は揺れ動いた…。"));
				break;
			case 2:	pline(E_J("Some branches swing...","枝が不気味に揺れた…。"));
				break;
			case 3:	if (!may_dig(x,y)) goto ouch;
				pline(E_J("The dead tree falls down.","枯れ木は倒壊した。"));
				maploc->typ = ROOM;
				if (Blind)
				    feel_location(x,y);	/* we know it's gone */
				else
				    newsym(x,y);
				unblock_point(x,y);	/* vision */
				break;
		    }
		    return(1);
		}
#ifdef SINKS
		if(IS_SINK(maploc->typ)) {
		    int gend = poly_gender();
		    short washerndx = (gend == 1 || (gend == 2 && rn2(2))) ?
					PM_INCUBUS : PM_SUCCUBUS;

		    if(Levitation) goto dumb;
		    if(rn2(5)) {
			if(flags.soundok)
			    pline(E_J("Klunk!  The pipes vibrate noisily.",
				      "配管が金属音をやかましく響かせた。"));
			else pline(E_J("Klunk!","ガン！"));
			exercise(A_DEX, TRUE);
			return(1);
		    } else if(!(maploc->looted & S_LPUDDING) && !rn2(3) &&
			  !(mvitals[PM_BLACK_PUDDING].mvflags & G_GONE)) {
			if (Blind)
			    You_hear(E_J("a gushing sound.",
					 "何かが噴き出す音を"));
			else
			    pline(E_J("A %s ooze gushes up from the drain!",
				      "%s汚泥が排水口から噴き出した！"),
					 hcolor(NH_BLACK));
			(void) makemon(&mons[PM_BLACK_PUDDING],
					 x, y, NO_MM_FLAGS);
			exercise(A_DEX, TRUE);
			newsym(x,y);
			maploc->looted |= S_LPUDDING;
			return(1);
		    } else if(!(maploc->looted & S_LDWASHER) && !rn2(3) &&
			      !(mvitals[washerndx].mvflags & G_GONE)) {
			/* can't resist... */
			pline(E_J("%s returns!","%sが戻ってきた！"),
				(Blind ? Something : E_J("The dish washer","皿洗い")));
			if (makemon(&mons[washerndx], x, y, NO_MM_FLAGS))
			    newsym(x,y);
			maploc->looted |= S_LDWASHER;
			exercise(A_DEX, TRUE);
			return(1);
		    } else if(!rn2(3)) {
#ifndef JP
			pline("Flupp!  %s.", (Blind ?
				      "You hear a sloshing sound" :
				      "Muddy waste pops up from the drain"));
#else
			pline(Blind ? "あなたは水がごぼごぼと流れ出る音を聞いた。" :
				      "配管に詰まっていたヘドロが排水口から飛び出した。");
#endif /*JP*/
			if(!(maploc->looted & S_LRING)) { /* once per sink */
			    if (!Blind)
				You(E_J("see a ring shining in its midst.",
					"汚泥の中で指輪が光るのを見つけた。"));
			    (void) mkobj_at(RING_CLASS, x, y, TRUE);
			    newsym(x, y);
			    exercise(A_DEX, TRUE);
			    exercise(A_WIS, TRUE);	/* a discovery! */
			    maploc->looted |= S_LRING;
			}
			return(1);
		    }
		    goto ouch;
		}
#endif
		if (maploc->typ == STAIRS || maploc->typ == LADDER ||
						    IS_STWALL(maploc->typ)) {
		    if(!IS_STWALL(maploc->typ) && maploc->ladder == LA_DOWN)
			goto dumb;
ouch:
		    pline(E_J("Ouch!  That hurts!","痛っ！ 怪我した！"));
		    exercise(A_DEX, FALSE);
		    exercise(A_STR, FALSE);
		    if (Blind) feel_location(x,y); /* we know we hit it */
		    if (is_drawbridge_wall(x,y) >= 0) {
			pline_The(E_J("drawbridge is unaffected.",
				      "跳ね橋はびくともしない。"));
			/* update maploc to refer to the drawbridge */
			(void) find_drawbridge(&x,&y);
			maploc = &levl[x][y];
		    }
		    if(!rn2(3)) set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
		    losehp(rnd(ACURR(A_CON) > 15 ? 3 : 5), kickstr(buf),
			KILLED_BY);
		    if(Is_airlevel(&u.uz) || Levitation)
			hurtle(-u.dx, -u.dy, rn1(2,4), TRUE); /* assume it's heavy */
		    return(1);
		}
		goto dumb;
	}

	if(maploc->doormask == D_ISOPEN ||
	   maploc->doormask == D_BROKEN ||
	   maploc->doormask == D_NODOOR) {
dumb:
		exercise(A_DEX, FALSE);
		if (martial() || ACURR(A_DEX) >= 16 || rn2(3)) {
			You(E_J("kick at empty space.",
				"何もない空間を蹴った。"));
			if (Blind) feel_location(x,y);
		} else {
			pline(E_J("Dumb move!  You strain a muscle.",
				  "ばかな行為だ！ あなたは脚を痛めた。"));
			exercise(A_STR, FALSE);
			set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
		}
		if ((Is_airlevel(&u.uz) || Levitation) && rn2(2)) {
		    hurtle(-u.dx, -u.dy, 1, TRUE);
		    return 1;		/* you moved, so use up a turn */
		}
		return(0);
	}

	/* not enough leverage to kick open doors while levitating */
	if(Levitation) goto ouch;

	exercise(A_DEX, TRUE);
	/* door is known to be CLOSED or LOCKED */
	if(rnl(35) < avrg_attrib + (!martial() ? 0 : ACURR(A_DEX))) {
		boolean shopdoor = *in_rooms(x, y, SHOPBASE) ? TRUE : FALSE;
		/* break the door */
		if(maploc->doormask & D_TRAPPED) {
		    if (flags.verbose) You(E_J("kick the door.","扉を蹴った。"));
		    exercise(A_STR, FALSE);
		    maploc->doormask = D_NODOOR;
		    b_trapped(E_J("door","扉"), FOOT);
		} else if(ACURR(A_STR) > 18 && !rn2(5) && !shopdoor) {
		    pline(E_J("As you kick the door, it shatters to pieces!",
			      "あなたが扉を蹴ると、扉は粉々に砕けた！"));
		    exercise(A_STR, TRUE);
		    maploc->doormask = D_NODOOR;
		} else {
		    pline(E_J("As you kick the door, it crashes open!",
			      "あなたが扉を蹴ると、扉は壊れて開いた！"));
		    exercise(A_STR, TRUE);
		    maploc->doormask = D_BROKEN;
		}
		if (Blind)
		    feel_location(x,y);		/* we know we broke it */
		else
		    newsym(x,y);
		unblock_point(x,y);		/* vision */
		if (shopdoor) {
		    add_damage(x, y, 400L);
		    pay_for_damage(E_J("break","扉を壊した"), FALSE);
		}
		if (in_town(x, y))
		  for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if((mtmp->mnum == PM_WATCHMAN ||
			mtmp->mnum == PM_WATCH_CAPTAIN) &&
			couldsee(mtmp->mx, mtmp->my) &&
			mtmp->mpeaceful) {
			if (canspotmon(mtmp))
			    pline(E_J("%s yells:","%sは叫んだ:"), Amonnam(mtmp));
			else
			    You_hear(E_J("someone yell:","誰かが叫ぶのを"));
			verbalize(E_J("Halt, thief!  You're under arrest!",
				      "動くな、泥棒！ お前を逮捕する！"));
			(void) angry_guards(FALSE);
			break;
		    }
		  }
	} else {
	    if (Blind) feel_location(x,y);	/* we know we hit it */
	    exercise(A_STR, TRUE);
	    pline(E_J("WHAMMM!!!","ドガッ!!!"));
	    if (in_town(x, y))
		for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if ((mtmp->mnum == PM_WATCHMAN ||
			 mtmp->mnum == PM_WATCH_CAPTAIN) &&
			    mtmp->mpeaceful && couldsee(mtmp->mx, mtmp->my)) {
			if (canspotmon(mtmp))
			    pline(E_J("%s yells:","%sは叫んだ:"), Amonnam(mtmp));
			else
			    You_hear(E_J("someone yell:","誰かが叫ぶのを"));
			if(levl[x][y].looted & D_WARNED) {
			    verbalize(E_J("Halt, vandal!  You're under arrest!",
					  "器物損壊の現行犯でお前を逮捕する！"));
			    (void) angry_guards(FALSE);
			} else {
			    verbalize(E_J("Hey, stop damaging that door!",
					  "おい、扉を壊すのをやめろ！"));
			    levl[x][y].looted |= D_WARNED;
			}
			break;
		    }
		}
	}
	return(1);
}

STATIC_OVL void
drop_to(cc, loc)
coord *cc;
schar loc;
{
	/* cover all the MIGR_xxx choices generated by down_gate() */
	switch (loc) {
	 case MIGR_RANDOM:	/* trap door or hole */
		    if (Is_stronghold(&u.uz)) {
			cc->x = valley_level.dnum;
			cc->y = valley_level.dlevel;
			break;
		    } else if (In_endgame(&u.uz) || Is_botlevel(&u.uz)) {
			cc->y = cc->x = 0;
			break;
		    } /* else fall to the next cases */
	 case MIGR_STAIRS_UP:
	 case MIGR_LADDER_UP:
		    cc->x = u.uz.dnum;
		    cc->y = u.uz.dlevel + 1;
		    break;
	 case MIGR_SSTAIRS:
		    cc->x = sstairs.tolev.dnum;
		    cc->y = sstairs.tolev.dlevel;
		    break;
	 default:
	 case MIGR_NOWHERE:
		    /* y==0 means "nowhere", in which case x doesn't matter */
		    cc->y = cc->x = 0;
		    break;
	}
}

void
impact_drop(missile, x, y, dlev)
struct obj *missile;
xchar x, y, dlev;
{
	schar toloc;
	register struct obj *obj, *obj2;
	register struct monst *shkp;
	long oct, dct, price, debit, robbed;
	boolean angry, costly, isrock;
	coord cc;
	struct eshk *ep;

	if(!OBJ_AT(x, y)) return;

	toloc = down_gate(x, y);
	drop_to(&cc, toloc);
	if (!cc.y) return;

	if (dlev) {
		/* send objects next to player falling through trap door.
		 * checked in obj_delivery().
		 */
		toloc = MIGR_NEAR_PLAYER;
		cc.y = dlev;
	}

	costly = costly_spot(x, y);
	price = debit = robbed = 0L;
	angry = FALSE;
	shkp = (struct monst *) 0;
	/* if 'costly', we must keep a record of ESHK(shkp) before
	 * it undergoes changes through the calls to stolen_value.
	 * the angry bit must be reset, if needed, in this fn, since
	 * stolen_value is called under the 'silent' flag to avoid
	 * unsavory pline repetitions.
	 */
	if(costly) {
	    if ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0) {
		ep = ESHK(shkp);
		debit	= ep->debit;
		robbed	= ep->robbed;
		angry	= !shkp->mpeaceful;
	    }
	}

	isrock = (missile && missile->otyp == ROCK);
	oct = dct = 0L;
	for(obj = level.objects[x][y]; obj; obj = obj2) {
		obj2 = obj->nexthere;
		if(obj == missile) continue;
		/* number of objects in the pile */
		oct += obj->quan;
		if(obj == uball || obj == uchain) continue;
		if(Is_sokoprize(obj)) continue; /* sokoprize never falls */
		/* boulders can fall too, but rarely & never due to rocks */
		if((isrock && obj->otyp == BOULDER) ||
		   rn2(obj->otyp == BOULDER ? 30 : 3)) continue;
		obj_extract_self(obj);

		if(costly) {
		    price += stolen_value(obj, x, y,
				(costly_spot(u.ux, u.uy) &&
				 index(u.urooms, *in_rooms(x, y, SHOPBASE))),
				TRUE);
		    /* set obj->no_charge to 0 */
		    if (Has_contents(obj))
			picked_container(obj);	/* does the right thing */
		    if (obj->oclass != COIN_CLASS)
			obj->no_charge = 0;
		}

		add_to_migration(obj);
		obj->ox = cc.x;
		obj->oy = cc.y;
		obj->owornmask = (long)toloc;

		/* number of fallen objects */
		dct += obj->quan;
	}

	if (dct && cansee(x,y)) {	/* at least one object fell */
#ifndef JP
	    const char *what = (dct == 1L ? "object falls" : "objects fall");

	    if (missile)
		pline("From the impact, %sother %s.",
		      dct == oct ? "the " : dct == 1L ? "an" : "", what);
	    else if (oct == dct)
		pline("%s adjacent %s %s.",
		      dct == 1L ? "The" : "All the", what, gate_str);
	    else
		pline("%s adjacent %s %s.",
		      dct == 1L ? "One of the" : "Some of the",
		      dct == 1L ? "objects falls" : what, gate_str);
#else
	    if (missile)
		pline("衝撃で、他の品物が落下した。");
	    else if (oct == dct)
		pline("近くにあった%s品物が%s落下した。",
		      dct == 1L ? "" : "すべての", gate_str);
	    else
		pline("近くにあった品物%sが%s落下した。",
		      dct == 1L ? "" : "のうちのいくつか", gate_str);
#endif /*JP*/
	}

	if(costly && shkp && price) {
		if(ep->robbed > robbed) {
		    You(E_J("removed %ld %s worth of goods!",
			    "%ld %s相当の商品を投棄してしまった！"), price, currency(price));
		    if(cansee(shkp->mx, shkp->my)) {
			if(ep->customer[0] == 0)
			    (void) strncpy(ep->customer,
					   plname, PL_NSIZ);
			if(angry)
			    pline(E_J("%s is infuriated!",
				      "%sは激怒した！"), Monnam(shkp));
			else pline(E_J("\"%s, you are a thief!\"",
				       "「%s、この盗っ人め！」"), plname);
		    } else  You_hear(E_J("a scream, \"Thief!\"",
					 "「泥棒！」という叫びを"));
		    hot_pursuit(shkp);
		    (void) angry_guards(FALSE);
		    return;
		}
		if(ep->debit > debit) {
		    long amt = (ep->debit - debit);
		    You(E_J("owe %s %ld %s for goods lost.",
			    "%sに対し、失われた商品%ld %s相当の責を負った。"),
			Monnam(shkp),
			amt, currency(amt));
		}
	}

}

/* NOTE: ship_object assumes otmp was FREED from fobj or invent.
 * <x,y> is the point of drop.  otmp is _not_ an <x,y> resident:
 * otmp is either a kicked, dropped, or thrown object.
 */
boolean
ship_object(otmp, x, y, shop_floor_obj)
xchar  x, y;
struct obj *otmp;
boolean shop_floor_obj;
{
	schar toloc;
	xchar ox, oy;
	coord cc;
	struct obj *obj;
	struct trap *t;
	boolean nodrop, unpaid, container, impact = FALSE;
	boolean force_drop;
	long n = 0L;

	if (!otmp) return(FALSE);
	if ((toloc = down_gate(x, y)) == MIGR_NOWHERE) return(FALSE);
	drop_to(&cc, toloc);
	if (!cc.y) return(FALSE);

	/* hero throw the object to downstairs intentionally */
	force_drop = (otmp->othrown && x == u.ux && y == u.uy && u.dz > 0);

	/* objects other than attached iron ball always fall down ladder,
	   but have a chance of staying otherwise */
	nodrop = (otmp == uball) || (otmp == uchain) || Is_sokoprize(otmp) ||
		(toloc != MIGR_LADDER_UP && !force_drop && rn2(3));

	container = Has_contents(otmp);
	unpaid = (otmp->unpaid || (container && count_unpaid(otmp->cobj)));

	if(OBJ_AT(x, y)) {
	    for(obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if(obj != otmp) n += obj->quan;
	    if(n) impact = TRUE;
	}
	/* boulders never fall through trap doors, but they might knock
	   other things down before plugging the hole */
	if (otmp->otyp == BOULDER &&
		((t = t_at(x, y)) != 0) &&
		(t->ttyp == TRAPDOOR || t->ttyp == HOLE)) {
	    if (impact) impact_drop(otmp, x, y, 0);
	    return FALSE;		/* let caller finish the drop */
	}

	if (cansee(x, y))
	    otransit_msg(otmp, nodrop, n);

	if (nodrop) {
	    if (impact) impact_drop(otmp, x, y, 0);
	    return(FALSE);
	}

	if(unpaid || shop_floor_obj) {
	    if(unpaid) {
		subfrombill(otmp, shop_keeper(*u.ushops));
		(void)stolen_value(otmp, u.ux, u.uy, TRUE, FALSE);
	    } else {
		ox = otmp->ox;
		oy = otmp->oy;
		(void)stolen_value(otmp, ox, oy,
			  (costly_spot(u.ux, u.uy) &&
			      index(u.urooms, *in_rooms(ox, oy, SHOPBASE))),
			  FALSE);
	    }
	    /* set otmp->no_charge to 0 */
	    if(container)
		picked_container(otmp); /* happens to do the right thing */
	    if(otmp->oclass != COIN_CLASS)
		otmp->no_charge = 0;
	}

	if (otmp == uwep) setuwep((struct obj *)0);
	if (otmp == uquiver) setuqwep((struct obj *)0);
	if (otmp == uswapwep) setuswapwep((struct obj *)0);

	/* some things break rather than ship */
	if (breaktest(otmp)) {
	    const char *result;

	    if (get_material(otmp) == GLASS
		|| otmp->otyp == EXPENSIVE_CAMERA
		) {
		if (otmp->otyp == MIRROR)
		    change_luck(-2);
		result = E_J("crash","割れる");
	    } else {
		/* penalty for breaking eggs laid by you */
		if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
		    change_luck((schar) -min(otmp->quan, 5L));
		result = E_J("splat","潰れる");
	    }
	    You_hear(E_J("a muffled %s.","何かが%s音を"),result);
	    obj_extract_self(otmp);
	    obfree(otmp, (struct obj *) 0);
	    return TRUE;
	}

	add_to_migration(otmp);
	otmp->ox = cc.x;
	otmp->oy = cc.y;
	otmp->owornmask = (long)toloc;
	/* boulder from rolling boulder trap, no longer part of the trap */
	if (otmp->otyp == BOULDER) otmp->otrapped = 0;

	if(impact) {
	    /* the objs impacted may be in a shop other than
	     * the one in which the hero is located.  another
	     * check for a shk is made in impact_drop.  it is, e.g.,
	     * possible to kick/throw an object belonging to one
	     * shop into another shop through a gap in the wall,
	     * and cause objects belonging to the other shop to
	     * fall down a trap door--thereby getting two shopkeepers
	     * angry at the hero in one shot.
	     */
	    impact_drop(otmp, x, y, 0);
	    newsym(x,y);
	}
	return(TRUE);
}

void
obj_delivery()
{
	register struct obj *otmp, *otmp2;
	register int nx, ny;
	long where;

	for (otmp = migrating_objs; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;
	    if (otmp->ox != u.uz.dnum || otmp->oy != u.uz.dlevel) continue;

	    obj_extract_self(otmp);
	    where = otmp->owornmask;		/* destination code */
	    otmp->owornmask = 0L;

	    switch ((int)where) {
	     case MIGR_STAIRS_UP:   nx = xupstair,  ny = yupstair;
				break;
	     case MIGR_LADDER_UP:   nx = xupladder,  ny = yupladder;
				break;
	     case MIGR_SSTAIRS:	    nx = sstairs.sx,  ny = sstairs.sy;
				break;
	     case MIGR_NEAR_PLAYER: nx = u.ux,  ny = u.uy;
				break;
	     default:
	     case MIGR_RANDOM:	    nx = ny = 0;
				break;
	    }
	    if (nx > 0) {
		place_object(otmp, nx, ny);
		stackobj(otmp);
		(void)scatter(nx, ny, rnd(2), 0, otmp);
	    } else {		/* random location */
		/* set dummy coordinates because there's no
		   current position for rloco() to update */
		otmp->ox = otmp->oy = 0;
		rloco(otmp);
	    }
	}
}

STATIC_OVL void
otransit_msg(otmp, nodrop, num)
register struct obj *otmp;
register boolean nodrop;
long num;
{
	char obuf[BUFSZ];

#ifndef JP
	Sprintf(obuf, "%s%s",
		 (otmp->otyp == CORPSE &&
			type_is_pname(&mons[otmp->corpsenm])) ? "" : "The ",
		 xname(otmp));
#else
	Strcpy(obuf, xname(otmp));
#endif /*JP*/

	if(num) { /* means: other objects are impacted */
#ifndef JP
	    Sprintf(eos(obuf), " %s %s object%s",
		    otense(otmp, "hit"),
		    num == 1L ? "another" : "other",
		    num > 1L ? "s" : "");
	    if(nodrop)
		Sprintf(eos(obuf), ".");
	    else
		Sprintf(eos(obuf), " and %s %s.",
			otense(otmp, "fall"), gate_str);
#else
	    Sprintf(eos(obuf), "は他の品物に当た");
	    if(nodrop)
		Sprintf(eos(obuf), "った。");
	    else
		Sprintf(eos(obuf), "り、%s落下した。", gate_str);
#endif /*JP*/
	    pline("%s", obuf);
	} else if(!nodrop)
#ifndef JP
	    pline("%s %s %s.", obuf, otense(otmp, "fall"), gate_str);
#else
	    pline("%sは%s落下した。", obuf, gate_str);
#endif /*JP*/
}

/* migration destination for objects which fall down to next level */
schar
down_gate(x, y)
xchar x, y;
{
	struct trap *ttmp;

	gate_str = 0;
	/* this matches the player restriction in goto_level() */
	if (on_level(&u.uz, &qstart_level) && !ok_to_quest())
	    return MIGR_NOWHERE;

	if ((xdnstair == x && ydnstair == y) ||
		(sstairs.sx == x && sstairs.sy == y && !sstairs.up)) {
	    gate_str = E_J("down the stairs","階段を");
	    return (xdnstair == x && ydnstair == y) ?
		    MIGR_STAIRS_UP : MIGR_SSTAIRS;
	}
	if (xdnladder == x && ydnladder == y) {
	    gate_str = E_J("down the ladder","はしごの下へ");
	    return MIGR_LADDER_UP;
	}

	if (((ttmp = t_at(x, y)) != 0 && ttmp->tseen) &&
		(ttmp->ttyp == TRAPDOOR || ttmp->ttyp == HOLE)) {
	    gate_str = (ttmp->ttyp == TRAPDOOR) ?
		    E_J("through the trap door","落とし扉を通って") :
		    E_J("through the hole",	"穴に");
	    return MIGR_RANDOM;
	}
	return MIGR_NOWHERE;
}

/*dokick.c*/
