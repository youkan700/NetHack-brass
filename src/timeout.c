/*	SCCS Id: @(#)timeout.c	3.4	2002/12/17	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"	/* for checking save modes */

STATIC_DCL void NDECL(stoned_dialogue);
STATIC_DCL void NDECL(vomiting_dialogue);
STATIC_DCL void NDECL(choke_dialogue);
STATIC_DCL void NDECL(slime_dialogue);
STATIC_DCL void NDECL(levitation_dialogue);
STATIC_DCL void NDECL(slip_or_trip);
STATIC_DCL void FDECL(see_lamp_flicker, (struct obj *, const char *));
STATIC_DCL void FDECL(lantern_message, (struct obj *));
STATIC_DCL void FDECL(cleanup_burn, (genericptr_t,long));

#ifdef OVLB

boolean
drownbymon()
{
	return (u.uswallow && u.ustuck &&
		u.ustuck->mnum == PM_WATER_ELEMENTAL);
}
void
reset_drownbymon()
{
	end_suffocation(SUFFO_WATER);
}

void
start_suffocation(reason)
long reason;
{
	if (!Strangled) Strangled = 6;
	StrangledBy |= reason;
}

void
end_suffocation(reason)
long reason;
{
	long cured = StrangledBy & reason;
	StrangledBy &= (~reason);
	if (cured & SUFFO_NECK) {
	    /* amulet of strangulation */
	    if (StrangledBy & SUFFO_WATER) {
		pline(E_J("Now you can drink water...",
			  "あなたは水を飲めるようになった…。"));
	    } else if (StrangledBy & SUFFO_NOAIR)
		pline(E_J("Unfortunately still you get no air.",
			  "不幸なことに、あなたはまだ空気を得られない。"));
	    else
		You(E_J("can breathe more easily!",
			"楽に息ができるようになった！"));
	} else if (cured & SUFFO_WATER) {
	    /* water elemental */
	    if (StrangledBy & SUFFO_NECK)
		You(E_J("are still strangled.",
			"まだ首を絞められている。"));
	    else if (StrangledBy & SUFFO_NOAIR)
		You(E_J("still get no air!",
			"まだ空気を得られない！"));
	    else if (Amphibious && !breathless(youmonst.data))
		You(E_J("can breathe now.",
			"呼吸できるようになった。"));
	    else if (Breathless)
		You_feel(E_J("no longer choked.",
			     "もう首を絞められていない。"));
	    else
		pline(E_J("Pheew!  That was close.",
			  "ふーっ！ 危なかった。"));
	} else if (cured & SUFFO_NOAIR) {
	    /* in vacuum or buried */
	    if (!StrangledBy)
		You(E_J("can breathe now!","息ができるようになった！"));
	}
	if (!StrangledBy) Strangled = 0;
}


/* He is being petrified - dialogue by inmet!tower */
static NEARDATA const char * const stoned_texts[] = {
	E_J("You are slowing down.",		"あなたは動きが鈍くなりはじめた。"),	/* 5 */
	E_J("Your limbs are stiffening.",	"あなたの手足は固まりつつある。"),	/* 4 */
	E_J("Your limbs have turned to stone.",	"あなたの手足は石と化した。"),		/* 3 */
	E_J("You have turned to stone.",	"あなたは石になった。"),		/* 2 */
	E_J("You are a statue.",		"あなたは石像だ。")			/* 1 */
};

STATIC_OVL void
stoned_dialogue()
{
	register long i = (Stoned & TIMEOUT);

	if (i > 0L && i <= SIZE(stoned_texts))
		pline(stoned_texts[SIZE(stoned_texts) - i]);
	if (i == 5L)
		HFast = 0L;
	if (i == 3L)
		nomul(-3);
	exercise(A_DEX, FALSE);
}

/* He is getting sicker and sicker prior to vomiting */
static NEARDATA const char * const vomiting_texts[] = {
	E_J("are feeling mildly nauseated.",	"軽い吐き気を催した。"),			/* 14 */
	E_J("feel slightly confused.",		"やや混乱してきた。"),				/* 11 */
	E_J("can't seem to think straight.",	"まともに考えることができなくなった。"),	/* 8 */
	E_J("feel incredibly sick.",		"ひどく気分が悪くなった。"),			/* 5 */
	E_J("suddenly vomit!",			"突然嘔吐した！")				/* 2 */
};

STATIC_OVL void
vomiting_dialogue()
{
	register long i = (Vomiting & TIMEOUT) / 3L;

	if ((((Vomiting & TIMEOUT) % 3L) == 2) && (i >= 0)
	    && (i < SIZE(vomiting_texts)))
		You(vomiting_texts[SIZE(vomiting_texts) - i - 1]);

	switch ((int) i) {
	case 0:
		vomit();
		morehungry(20);
		break;
	case 2:
		make_stunned(HStun + d(2,4), FALSE);
		/* fall through */
	case 3:
		make_confused(HConfusion + d(2,4), FALSE);
		break;
	}
	exercise(A_CON, FALSE);
}

static NEARDATA const char * const choke_texts[] = {
	E_J("You find it hard to breathe.",	"あなたは息をすることが困難になった。"),
	E_J("You're gasping for air.",		"あなたは空気を求めてあえいだ。"),
	E_J("You can no longer breathe.",	"あなたはもはや息ができない。"),
	E_J("You're turning %s.",		"あなたの顔は%sなった。"),
	E_J("You suffocate.",			"あなたは窒息した。"),
};

static NEARDATA const char * const choke_texts2[] = {
	E_J("Your %s is becoming constricted.",			"あなたの%sが絞め上げられてゆく。"),
	E_J("Your blood is having trouble reaching your brain.","あなたの血が脳に回らなくなってきた。"),
	E_J("The pressure on your %s increases.",		"あなたの%sを絞め上げる力が増した。"),
	E_J("Your consciousness is fading.",			"あなたは意識が遠のいてゆく。"),
	E_J("You suffocate.",					"あなたは窒息した。"),
};

STATIC_OVL void
choke_dialogue()
{
	register long i = (Strangled & TIMEOUT);

	if(i > 0 && i <= SIZE(choke_texts)) {
	    if ((Breathless || !rn2(50)) && (StrangledBy & SUFFO_NECK))
		pline(choke_texts2[SIZE(choke_texts2) - i], body_part(NECK));
	    else {
		const char *str = choke_texts[SIZE(choke_texts)-i];

		if (index(str, '%'))
#ifndef JP
		    pline(str, hcolor(NH_BLUE));
#else
		    pline(str, j_no_ni(hcolor(NH_BLUE)));
#endif /*JP*/
		else
		    pline(str);
	    }
	}
	exercise(A_STR, FALSE);
}

static NEARDATA const char * const slime_texts[] = {
	E_J("You are turning a little %s.",	"あなたの身体が%sなってきた。"),	/* 5 */
	E_J("Your limbs are getting oozy.",	"あなたの手足が溶けはじめた。"),	/* 4 */
	E_J("Your skin begins to peel away.",	"あなたの肌が剥け落ちはじめた。"),	/* 3 */
	E_J("You are turning into %s.",		"あなたは%sと化しつつある。"),		/* 2 */
	E_J("You have become %s.",		"あなたは%sになった。")			/* 1 */
};

STATIC_OVL void
slime_dialogue()
{
	register long i = (Slimed & TIMEOUT) / 2L;

	if (((Slimed & TIMEOUT) % 2L) && i >= 0L
		&& i < SIZE(slime_texts)) {
	    const char *str = slime_texts[SIZE(slime_texts) - i - 1L];

	    if (index(str, '%')) {
		if (i == 4L) {	/* "you are turning green" */
		    if (!Blind)	/* [what if you're already green?] */
#ifndef JP
			pline(str, hcolor(NH_GREEN));
#else
			pline(str, j_no_ni(hcolor(NH_GREEN)));
#endif /*JP*/
		} else
#ifndef JP
		    pline(str, an(Hallucination ? rndmonnam() : "green slime"));
#else
		    pline(str, Hallucination ? rndmonnam() : "グリーンスライム");
#endif /*JP*/
	    } else
		pline(str);
	}
	if (i == 3L) {	/* limbs becoming oozy */
	    HFast = 0L;	/* lose intrinsic speed */
	    stop_occupation();
	    if (multi > 0) nomul(0);
	}
	exercise(A_DEX, FALSE);
}

void
burn_away_slime()
{
	if (Slimed) {
	    pline_The(E_J("slime that covers you is burned away!",
			  "あなたを覆っていたスライムが焼け落ちた！"));
	    Slimed = 0L;
	    flags.botl = 1;
	}
	return;
}

STATIC_OVL void
levitation_dialogue()
{
	long i = (HLevitation & TIMEOUT);

	if (i == 7L && !ELevitation && (i == HLevitation) && !BLevAtWill &&
	    !can_transit_levitation()) {
		You_feel(E_J("gravity starts increasing.",
			     "浮力が弱まってきたのを感じた。"));
		stop_occupation();
	}
}


#endif /* OVLB */
#ifdef OVL0

void
nh_timeout()
{
	register struct prop *upp;
	int sleeptime;
	int m_idx;
	int baseluck = (flags.moonphase == FULL_MOON) ? 1 : 0;

	if (flags.friday13) baseluck -= 1;

	if (u.uluck != baseluck &&
		moves % (u.uhave.amulet || u.ugangr ? 300 : 600) == 0) {
	/* Cursed luckstones stop bad luck from timing out; blessed luckstones
	 * stop good luck from timing out; normal luckstones stop both;
	 * neither is stopped if you don't have a luckstone.
	 * Luck is based at 0 usually, +1 if a full moon and -1 on Friday 13th
	 */
	    register int time_luck = stone_luck(FALSE);
	    boolean nostone = !carrying(LUCKSTONE) && !stone_luck(TRUE);

	    if(u.uluck > baseluck && (nostone || time_luck < 0))
		u.uluck--;
	    else if(u.uluck < baseluck && (nostone || time_luck > 0))
		u.uluck++;
	}

	/* WAC -- check for timeout of specials */
	tech_timeout();

	if(u.uinvulnerable) return; /* things past this point could kill you */
	if(Stoned) stoned_dialogue();
	if(Slimed) slime_dialogue();
	if(Vomiting) vomiting_dialogue();
	if(Strangled) choke_dialogue();
	if(Levitation) levitation_dialogue();
	if(u.mtimedone && !--u.mtimedone) {
		if (Unchanging)
			u.mtimedone = rnd(100*youmonst.data->mlevel + 1);
		else
			rehumanize();
	}
	if(u.ucreamed) u.ucreamed--;

	/* Dissipate spell-based protection. */
	if (u.usptime) {
	    if (--u.usptime == 0 && u.uspellprot) {
		u.usptime = u.uspmtime;
		u.uspellprot--;
		find_ac();
		if (!Blind)
#ifndef JP
		    Norep("The %s haze around you %s.", hcolor(NH_GOLDEN),
			  u.uspellprot ? "becomes less dense" : "disappears");
#else
		    Norep("あなたを包んでい%s%sもやが%sった。", 
			  u.uspellprot ? "る" : "た", hcolor(NH_GOLDEN),
			  u.uspellprot ? "薄くな" : "消え去");
#endif /*JP*/
	    }
	}
	/* magic torch */
	if (u.uspellit) {
	    int age = --u.uspellit;
	    if (age == 0) {
		extinguish_torch();
		if (!Blind) pline_The(E_J("magical light around you goes away.",
					  "あなたを囲んでいた魔法の灯りが消えた。"));
	    }
	    if (age == 50) {
		change_light_source_range(LS_MONSTER, &youmonst, 2);
		if (!Blind) Your(E_J("magic torch is getting dim.",
				     "魔法の灯りはやや暗くなった。"));
	    }
	}

#ifdef STEED
	if (u.ugallop) {
	    if (--u.ugallop == 0L && u.usteed)
	    	pline(E_J("%s stops galloping.",
			  "%sは速駆けをやめた。"), Monnam(u.usteed));
	}
#endif

	for(upp = u.uprops; upp < u.uprops+SIZE(u.uprops); upp++)
	    if((upp->intrinsic & TIMEOUT) && !(--upp->intrinsic & TIMEOUT)) {
		switch(upp - u.uprops){
		case STONED:
			if (get_delayed_killer(STONED, 0) && !killer) {
				killer = get_delayed_killer(STONED, &killer_format);
				delayed_killer = 0;
			}
			if (!killer) {
				/* leaving killer_format would make it
				   "petrified by petrification" */
				killer_format = NO_KILLER_PREFIX;
				killer = E_J("killed by petrification","石化した");
			}
			done(STONING);
			break;
		case SLIMED:
			if (get_delayed_killer(SLIMED, 0) && !killer) {
				killer = get_delayed_killer(SLIMED, &killer_format);
				delayed_killer = 0;
			}
			if (!killer) {
				killer_format = NO_KILLER_PREFIX;
				killer = E_J("turned into green slime",
					     "グリーンスライムと化した");
			}
			done(TURNED_SLIME);
			break;
		case VOMITING:
			make_vomiting(0L, TRUE);
			break;
		case SICK:
			You(E_J("die from your illness.","病気で死んだ。"));
			killer_format = KILLED_BY_AN;
			killer = u.usick_cause;
			if ((m_idx = name_to_mon(killer)) >= LOW_PM) {
#ifndef JP
			    if (type_is_pname(&mons[m_idx])) {
				killer_format = KILLED_BY;
			    } else if (mons[m_idx].geno & G_UNIQ) {
				killer = the(killer);
				Strcpy(u.usick_cause, killer);
				killer_format = KILLED_BY;
			    }
#else
			    Strcat(u.usick_cause, "に\tうつされた病気で死んだ");
			    killer_format = NO_KILLER_PREFIX;
#endif /*JP*/
			}
			u.usick_type = 0;
			done(POISONING);
			break;
		case FAST:
#ifndef JP
			You_feel("yourself slowing down%s.",
						Fast ? " a bit" : "");
#else
			You("自分の動きが%s遅くなったのを感じた。",
						Fast ? "少し" : "");
#endif /*JP*/
			if (Rocketskate) {
			    if (!(HFumbling & ~(FROMOUTSIDE|TIMEOUT)))
				HFumbling &= ~(FROMOUTSIDE|TIMEOUT);
			}
			u.uspdbon2 = 0;
			break;
		case CONFUSION:
			HConfusion = 1; /* So make_confused works properly */
			make_confused(0L, TRUE);
			stop_occupation();
			break;
		case STUNNED:
			HStun = 1;
			make_stunned(0L, TRUE);
			stop_occupation();
			break;
		case BLINDED:
			Blinded = 1;
			make_blinded(0L, TRUE);
			stop_occupation();
			break;
		case INVIS:
			newsym(u.ux,u.uy);
			if (!Invis && !BInvis && !Blind) {
#ifndef JP
			    You(!See_invisible ?
				    "are no longer invisible." :
				    "can no longer see through yourself.");
#else
			    Your(!See_invisible ?
				    "姿が現れた。" : "姿は透明性を失った。");
#endif /*JP*/
			    stop_occupation();
			}
			break;
		case SEE_INVIS:
			set_mimic_blocking(); /* do special mimic handling */
			see_monsters();		/* make invis mons appear */
			newsym(u.ux,u.uy);	/* make self appear */
			stop_occupation();
			break;
		case WOUNDED_LEGS:
			heal_legs();
			stop_occupation();
			break;
		case HALLUC:
			HHallucination = 1;
			(void) make_hallucinated(0L, TRUE, 0L);
			stop_occupation();
			break;
		case SLEEPING:
			if (unconscious() || Sleep_resistance)
				HSleeping += rnd(100);
			else if (Sleeping) {
				You(E_J("fall asleep.","眠りに落ちた。"));
				sleeptime = rnd(20);
				fall_asleep(-sleeptime, TRUE);
				HSleeping += sleeptime + rnd(100);
			}
			break;
		case LEVATWILL:
			if (Lev_at_will || !Levitation) break;
			/* fallthru */
		case LEVITATION:
			if (!Levitation && can_transit_levitation()) {
				/* potion to ring, ring to ring */
				start_ring_levitation(0);
				break;
			} else
				stop_ring_levitation(0);
			if (float_down(TIMEOUT, 0L)) stop_occupation();
			break;
		case STRANGLED:
			if (drownbymon()) {
#ifndef JP
				killer_format = KILLED_BY_AN;
				killer = u.ustuck->data->mname;
#else
				killer_format = KILLED_SUFFIX;
				Sprintf(killer_buf, "%sに", JMONNAM(monsndx(u.ustuck->data)));
				killer = killer_buf;
#endif /*JP*/
			} else {
#ifndef JP
				killer_format = KILLED_BY;
				killer = (u.uburied) ? "suffocation" : "strangulation";
#else
				killer_format = KILLED_BY;
				killer = (u.uburied) ? "窒息して" : "首を絞められて";
#endif /*JP*/
			}
			done(DIED);
			break;
		case FUMBLING:
			/* call this only when a move took place.  */
			/* otherwise handle fumbling msgs locally. */
			if (u.umoved && !Levitation) {
			    slip_or_trip();
			    nomul(-2);
			    nomovemsg = "";
			    /* The more you are carrying the more likely you
			     * are to make noise when you fumble.  Adjustments
			     * to this number must be thoroughly play tested.
			     */
			    if ((inv_weight() > -500)) {
				You(E_J("make a lot of noise!",
					"うるさく音を立ててしまった！"));
				wake_nearby();
			    }
			}
			/* from outside means slippery ice; don't reset
			   counter if that's the only fumble reason */
			if (!Rocketskate)
			    HFumbling &= ~FROMOUTSIDE;
			if (Fumbling)
			    HFumbling += rnd(20);
			break;
		case DETECT_MONSTERS:
			see_monsters();
			break;
		case WWALKING:
			if ((is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy) || is_swamp(u.ux,u.uy)) &&
			    !Wwalking && !Levitation && !Flying && !is_clinger(youmonst.data)) {
				spoteffects(TRUE);
			}
			break;
		case KNOW_ENCHANTMENT:
			if (!Know_enchantment)
			    You(E_J("are no longer able to know enchantment of weapons and armor.",
				    "武具に関する鑑定眼を失った。"));
			break;
		}
	}

	run_timers();
}

#endif /* OVL0 */
#ifdef OVL1

void
fall_asleep(how_long, wakeup_msg)
int how_long;
boolean wakeup_msg;
{
	stop_occupation();
	nomul(how_long);
	/* generally don't notice sounds while sleeping */
	if (wakeup_msg && multi == how_long) {
	    /* caller can follow with a direct call to Hear_again() if
	       there's a need to override this when wakeup_msg is true */
	    flags.soundok = 0;
	    afternmv = Hear_again;	/* this won't give any messages */
	}
	/* early wakeup from combat won't be possible until next monster turn */
	u.usleep = monstermoves;
	nomovemsg = wakeup_msg ? E_J("You wake up.","あなたは目を覚ました。") :
			You_can_move_again;
}

/* Attach an egg hatch timeout to the given egg. */
void
attach_egg_hatch_timeout(egg)
struct obj *egg;
{
	int i;

	/* stop previous timer, if any */
	(void) stop_timer(HATCH_EGG, (genericptr_t) egg);

	/*
	 * Decide if and when to hatch the egg.  The old hatch_it() code tried
	 * once a turn from age 151 to 200 (inclusive), hatching if it rolled
	 * a number x, 1<=x<=age, where x>150.  This yields a chance of
	 * hatching > 99.9993%.  Mimic that here.
	 */
	for (i = (MAX_EGG_HATCH_TIME-50)+1; i <= MAX_EGG_HATCH_TIME; i++)
	    if (rnd(i) > 150) {
		/* egg will hatch */
		(void) start_timer((long)i, TIMER_OBJECT,
						HATCH_EGG, (genericptr_t)egg);
		break;
	    }
}

/* prevent an egg from ever hatching */
void
kill_egg(egg)
struct obj *egg;
{
	/* stop previous timer, if any */
	(void) stop_timer(HATCH_EGG, (genericptr_t) egg);
}

/* timer callback routine: hatch the given egg */
void
hatch_egg(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *egg;
	struct monst *mon, *mon2;
	coord cc;
	xchar x, y;
	boolean yours, silent, knows_egg = FALSE;
	boolean cansee_hatchspot = FALSE;
	int i, mnum, hatchcount = 0;

	egg = (struct obj *) arg;
	/* sterilized while waiting */
	if (egg->corpsenm == NON_PM) return;

	mon = mon2 = (struct monst *)0;
	mnum = big_to_little(egg->corpsenm);
	/* The identity of one's father is learned, not innate */
	yours = (egg->spe || (!flags.female && carried(egg) && !rn2(2)));
	silent = (timeout != monstermoves);	/* hatched while away */

	/* only can hatch when in INVENT, FLOOR, MINVENT */
	if (get_obj_location(egg, &x, &y, 0)) {
	    hatchcount = rnd((int)egg->quan);
	    cansee_hatchspot = cansee(x, y) && !silent;
	    if (!(mons[mnum].geno & G_UNIQ) &&
		   !(mvitals[mnum].mvflags & (G_GENOD | G_EXTINCT))) {
		for (i = hatchcount; i > 0; i--) {
		    if (!enexto(&cc, x, y, &mons[mnum]) ||
			 !(mon = makemon(&mons[mnum], cc.x, cc.y, NO_MINVENT)))
			break;
		    /* tame if your own egg hatches while you're on the
		       same dungeon level, or any dragon egg which hatches
		       while it's in your inventory */
		    if ((yours && !silent) ||
			(carried(egg) && mon->data->mlet == S_DRAGON)) {
			if ((mon2 = tamedog(mon, (struct obj *)0)) != 0) {
			    mon = mon2;
			    if (carried(egg) && mon->data->mlet != S_DRAGON)
				mon->mtame = 20;
			}
		    }
		    if (mvitals[mnum].mvflags & G_EXTINCT)
			break;	/* just made last one */
		    mon2 = mon;	/* in case makemon() fails on 2nd egg */
		}
		if (!mon) mon = mon2;
		hatchcount -= i;
		egg->quan -= (long)hatchcount;
	    }
	}
#if 0
	/*
	 * We could possibly hatch while migrating, but the code isn't
	 * set up for it...
	 */
	else if (obj->where == OBJ_MIGRATING) {
	    /*
	    We can do several things.  The first ones that come to
	    mind are:

	    + Create the hatched monster then place it on the migrating
	      mons list.  This is tough because all makemon() is made
	      to place the monster as well.    Makemon() also doesn't
	      lend itself well to splitting off a "not yet placed"
	      subroutine.

	    + Mark the egg as hatched, then place the monster when we
	      place the migrating objects.

	    + Or just kill any egg which gets sent to another level.
	      Falling is the usual reason such transportation occurs.
	    */
	    cansee_hatchspot = FALSE;
	    mon = ???
	    }
#endif

	if (mon) {
	    char monnambuf[BUFSZ], carriedby[BUFSZ];
	    boolean siblings = (hatchcount > 1), redraw = FALSE;

	    if (cansee_hatchspot) {
		Sprintf(monnambuf, "%s%s",
			siblings ? E_J("some ","何匹かの") : "",
			E_J((siblings ? makeplural(m_monnam(mon)) : an(m_monnam(mon))),
			    m_monnam(mon)));
		/* we don't learn the egg type here because learning
		   an egg type requires either seeing the egg hatch
		   or being familiar with the egg already,
		   as well as being able to see the resulting
		   monster, checked below
		*/
	    }
	    switch (egg->where) {
		case OBJ_INVENT:
		    knows_egg = TRUE; /* true even if you are blind */
		    if (!cansee_hatchspot)
			You_feel(E_J("%s %s from your pack!","%sが背負い袋から%s出るのを感じた！"),
			    something, locomotion(mon->data, E_J("drop","転げ")));
		    else
#ifndef JP
			You("see %s %s out of your pack!",
			    monnambuf, locomotion(mon->data, "drop"));
#else
			pline("%sがあなたの背負い袋から%s出た！",
			    monnambuf, locomotion(mon->data, E_J("drop","転げ")));
#endif /*JP*/
		    if (yours) {
#ifndef JP
			pline("%s cries sound like \"%s%s\"",
			    siblings ? "Their" : "Its",
			    flags.female ? "mommy" : "daddy",
			    egg->spe ? "." : "?");
#else
			pline("その鳴き声は「%s%s」と言っているように聞こえた。",
			    flags.female ? "ママ" : "パパ",
			    egg->spe ? "" : "？");
#endif /*JP*/
		    } else if (mon->data->mlet == S_DRAGON) {
			verbalize(E_J("Gleep!","ピギャア！"));		/* Mything eggs :-) */
		    }
		    break;

		case OBJ_FLOOR:
		    if (cansee_hatchspot) {
			knows_egg = TRUE;
			You(E_J("see %s hatch.","%sが孵るのを見た。"), monnambuf);
			redraw = TRUE;	/* update egg's map location */
		    }
		    break;

		case OBJ_MINVENT:
		    if (cansee_hatchspot) {
			/* egg carring monster might be invisible */
#ifndef JP
			if (canseemon(egg->ocarry)) {
			    Sprintf(carriedby, "%s pack",
				     s_suffix(a_monnam(egg->ocarry)));
			    knows_egg = TRUE;
			}
			else if (is_pool(mon->mx, mon->my))
			    Strcpy(carriedby, "empty water");
			else
			    Strcpy(carriedby, "thin air");
			You("see %s %s out of %s!", monnambuf,
			    locomotion(mon->data, "drop"), carriedby);
#else
			if (canseemon(egg->ocarry)) {
			    Sprintf(carriedby, "%sの荷物",
				     mon_nam(egg->ocarry));
			    knows_egg = TRUE;
			}
			else if (is_pool(mon->mx, mon->my))
			    Strcpy(carriedby, "何もない水中");
			else
			    Strcpy(carriedby, "何もない空中");
			pline("%sが%sから%s出た！", monnambuf,
			      carriedby, locomotion(mon->data, "転げ"));
#endif /*JP*/
		    }
		    break;
#if 0
		case OBJ_MIGRATING:
		    break;
#endif
		default:
		    impossible("egg hatched where? (%d)", (int)egg->where);
		    break;
	    }

	    if (cansee_hatchspot && knows_egg)
		learn_egg_type(mnum);

	    if (egg->quan > 0) {
		/* still some eggs left */
		attach_egg_hatch_timeout(egg);
		if (egg->timed) {
		    /* replace ordinary egg timeout with a short one */
		    (void) stop_timer(HATCH_EGG, (genericptr_t)egg);
		    (void) start_timer((long)rnd(12), TIMER_OBJECT,
					HATCH_EGG, (genericptr_t)egg);
		}
	    } else if (carried(egg)) {
		useup(egg);
	    } else {
		/* free egg here because we use it above */
		obj_extract_self(egg);
		obfree(egg, (struct obj *)0);
	    }
	    if (redraw) newsym(x, y);
	}
}

/* Learn to recognize eggs of the given type. */
void
learn_egg_type(mnum)
int mnum;
{
	/* baby monsters hatch from grown-up eggs */
	mnum = little_to_big(mnum);
	mvitals[mnum].mvflags |= MV_KNOWS_EGG;
	/* we might have just learned about other eggs being carried */
	update_inventory();
}

/* Attach a fig_transform timeout to the given figurine. */
void
attach_fig_transform_timeout(figurine)
struct obj *figurine;
{
	int i;

	/* stop previous timer, if any */
	(void) stop_timer(FIG_TRANSFORM, (genericptr_t) figurine);

	/*
	 * Decide when to transform the figurine.
	 */
	i = rnd(9000) + 200;
	/* figurine will transform */
	(void) start_timer((long)i, TIMER_OBJECT,
				FIG_TRANSFORM, (genericptr_t)figurine);
}

/* give a fumble message */
STATIC_OVL void
slip_or_trip()
{
	struct obj *otmp = vobj_at(u.ux, u.uy);
	const char *what, *pronoun;
	char buf[BUFSZ];
	boolean on_foot = TRUE;
#ifdef STEED
	if (u.usteed) on_foot = FALSE;
#endif

	if (otmp && on_foot && !u.uinwater && is_pool(u.ux, u.uy)) otmp = 0;

  	if (otmp && on_foot) {		/* trip over something in particular */
	    /*
		If there is only one item, it will have just been named
		during the move, so refer to by via pronoun; otherwise,
		if the top item has been or can be seen, refer to it by
		name; if not, look for rocks to trip over; trip over
		anonymous "something" if there aren't any rocks.
	     */
#ifndef JP
	    pronoun = otmp->quan == 1L ? "it" : Hallucination ? "they" : "them";
	    what = !otmp->nexthere ? pronoun :
		  (otmp->dknown || !Blind) ? doname(otmp) :
		  ((otmp = sobj_at(ROCK, u.ux, u.uy)) == 0 ? something :
		  (otmp->quan == 1L ? "a rock" : "some rocks"));
	    if (Hallucination) {
		what = strcpy(buf, what);
		buf[0] = highc(buf[0]);
		pline("Egads!  %s bite%s your %s!",
			what, (!otmp || otmp->quan == 1L) ? "s" : "",
			body_part(FOOT));
	    } else {
		You("trip over %s.", what);
	    }
#else
	    what = !otmp->nexthere ? something :
		  (otmp->dknown || !Blind) ? doname(otmp) :
		  ((otmp = sobj_at(ROCK, u.ux, u.uy)) == 0 ? something :
		  "石ころ");
	    if (Hallucination) {
		pline("ギャッ！ %sがあなたの%sに噛みついた！",
			what, body_part(FOOT));
	    } else {
		You("%sにつまずいた。", what);
	    }
#endif /*JP*/
	} else if (rn2(3) && is_ice(u.ux, u.uy)) {
	    pline(E_J("%s %s%s on the ice.","%sは氷の上で%sった。"),
#ifdef STEED
		u.usteed ? upstart(x_monnam(u.usteed,
				u.usteed->has_name ? ARTICLE_NONE : ARTICLE_THE,
				(char *)0, SUPPRESS_SADDLE, FALSE)) :
#endif
#ifndef JP
		"You", rn2(2) ? "slip" : "slide", on_foot ? "" : "s");
#else
		"あなた", rn2(2) ? "転びそうにな" : "滑");
#endif /*JP*/
	} else {
	    if (on_foot) {
		switch (rn2(4)) {
		  case 1:
#ifndef JP
			You("trip over your own %s.", Hallucination ?
				"elbow" : makeplural(body_part(FOOT)));
#else
			You("自分の%sにつまずいた。", Hallucination ?
				"ひじ" : body_part(FOOT));
#endif /*JP*/
			break;
		  case 2:
#ifndef JP
			You("slip %s.", Hallucination ?
				"on a banana peel" : "and nearly fall");
#else
			You("%sった。", Hallucination ?
				"バナナの皮で滑" : "滑って転びそうにな");
#endif /*JP*/
			break;
		  case 3:
			You(E_J("flounder.","バランスを取るのに苦労している。"));
			break;
		  default:
			You(E_J("stumble.","よろめいた。"));
			break;
		}
	    }
#ifdef STEED
	    else {
		switch (rn2(4)) {
		  case 1:
#ifndef JP
			Your("%s slip out of the stirrups.", makeplural(body_part(FOOT)));
#else
			Your("%sがあぶみから外れた。", body_part(FOOT));
#endif /*JP*/
			break;
		  case 2:
			You(E_J("let go of the reins.","手綱を放してしまった。"));
			break;
		  case 3:
			You(E_J("bang into the saddle-horn.","鞍の上でつんのめった。"));
			break;
		  default:
			You(E_J("slide to one side of the saddle.","鞍の片側に滑っていった。"));
			break;
		}
		dismount_steed(DISMOUNT_FELL);
	    }
#endif
	}
}

/* Print a lamp flicker message with tailer. */
STATIC_OVL void
see_lamp_flicker(obj, tailer)
struct obj *obj;
const char *tailer;
{
	switch (obj->where) {
	    case OBJ_INVENT:
	    case OBJ_MINVENT:
#ifndef JP
		pline("%s flickers%s.", Yname2(obj), tailer);
#else
		pline("%sが%s揺らめいた。", yname(obj), tailer);
#endif /*JP*/
		break;
	    case OBJ_FLOOR:
#ifndef JP
		You("see %s flicker%s.", an(xname(obj)), tailer);
#else
		You("%sが%s揺らめくのを見た。", xname(obj), tailer);
#endif /*JP*/
		break;
	}
}

/* Print a dimming message for brass lanterns. */
STATIC_OVL void
lantern_message(obj)
struct obj *obj;
{
	/* from adventure */
	switch (obj->where) {
	    case OBJ_INVENT:
		Your(E_J("lantern is getting dim.","ランタンは暗くなった。"));
		if (Hallucination)
		    pline(E_J("Batteries have not been invented yet.",
			      "電池はまだ発明されていないのに。"));
		break;
	    case OBJ_FLOOR:
		You(E_J("see a lantern getting dim.","ランタンが暗くなったのを見た。"));
		break;
	    case OBJ_MINVENT:
		pline(E_J("%s lantern is getting dim.","%sランタンは暗くなった。"),
		    s_suffix(Monnam(obj->ocarry)));
		break;
	}
}

/*
 * Timeout callback for for objects that are burning. E.g. lamps, candles.
 * See begin_burn() for meanings of obj->age and obj->spe.
 */
void
burn_object(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *obj = (struct obj *) arg;
	boolean canseeit, many, menorah, need_newsym;
	xchar x, y;
	char whose[BUFSZ];

	menorah = obj->otyp == CANDELABRUM_OF_INVOCATION;
	many = menorah ? obj->spe > 1 : obj->quan > 1L;

	/* timeout while away */
	if (timeout != monstermoves) {
	    long how_long = monstermoves - timeout;

	    if (how_long >= obj->age) {
		obj->age = 0;
		end_burn(obj, FALSE);

		if (menorah) {
		    obj->spe = 0;	/* no more candles */
		} else if (Is_candle(obj) || obj->otyp == POT_OIL) {
		    /* get rid of candles and burning oil potions */
		    obj_extract_self(obj);
		    obfree(obj, (struct obj *)0);
		    obj = (struct obj *) 0;
		}

	    } else {
		obj->age -= how_long;
		begin_burn(obj, TRUE);
	    }
	    return;
	}

	/* only interested in INVENT, FLOOR, and MINVENT */
	if (get_obj_location(obj, &x, &y, 0)) {
	    canseeit = !Blind && cansee(x, y);
	    /* set up `whose[]' to be "Your" or "Fred's" or "The goblin's" */
	    (void) Shk_Your(whose, obj);
	} else {
	    canseeit = FALSE;
	}
	need_newsym = FALSE;

	/* obj->age is the age remaining at this point.  */
	switch (obj->otyp) {
	    case POT_OIL:
		    /* this should only be called when we run out */
		    if (canseeit) {
			switch (obj->where) {
			    case OBJ_INVENT:
			    case OBJ_MINVENT:
				pline(E_J("%s potion of oil has burnt away.",
					  "%s油瓶の中身は燃え尽きた。"),
				    whose);
				break;
			    case OBJ_FLOOR:
				You(E_J("see a burning potion of oil go out.",
					"瓶の油が燃え尽きるのを見た。"));
				need_newsym = TRUE;
				break;
			}
		    }
		    end_burn(obj, FALSE);	/* turn off light source */
		    obj_extract_self(obj);
		    obfree(obj, (struct obj *)0);
		    obj = (struct obj *) 0;
		    break;

	    case BRASS_LANTERN:
	    case OIL_LAMP:
		switch((int)obj->age) {
		    case 150:
		    case 100:
		    case 50:
			if (canseeit) {
			    if (obj->otyp == BRASS_LANTERN)
				lantern_message(obj);
			    else
				see_lamp_flicker(obj,
				    obj->age == 50L ? E_J(" considerably",
							  "激しく") : "");
			}
			break;

		    case 25:
			if (canseeit) {
			    if (obj->otyp == BRASS_LANTERN)
				lantern_message(obj);
			    else {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline(E_J("%s %s seems about to go out.",
						  "%s%sは消えそうだ。"),
					    whose, xname(obj));
					break;
				    case OBJ_FLOOR:
#ifndef JP
					You("see %s about to go out.",
					    an(xname(obj)));
#else
					You("%sが消えそうになるのを見た。",
					    xname(obj));
#endif /*JP*/
					break;
				}
			    }
			}
			break;

		    case 0:
			/* even if blind you'll know if holding it */
			if (canseeit || obj->where == OBJ_INVENT) {
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    if (obj->otyp == BRASS_LANTERN)
					pline(E_J("%s lantern has run out of power.",
						  "%sランタンの電池が切れた。"),
					    whose);
				    else
					pline(E_J("%s %s has gone out.",
						  "%s%sは消えた。"),
					    whose, xname(obj));
				    break;
				case OBJ_FLOOR:
				    if (obj->otyp == BRASS_LANTERN)
					You(E_J("see a lantern run out of power.",
						"ランタンの電池が切れるのを見た。"));
				    else
#ifndef JP
					You("see %s go out.",
					    an(xname(obj)));
#else
					You("%sが消えるのを見た。",
					    xname(obj));
#endif /*JP*/
				    break;
			    }
			}
			end_burn(obj, FALSE);
			break;

		    default:
			/*
			 * Someone added fuel to the lamp while it was
			 * lit.  Just fall through and let begin burn
			 * handle the new age.
			 */
			break;
		}

		if (obj->age)
		    begin_burn(obj, TRUE);

		break;

	    case CANDELABRUM_OF_INVOCATION:
	    case TALLOW_CANDLE:
	    case WAX_CANDLE:
		switch (obj->age) {
		    case 75:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
#ifndef JP
				    pline("%s %scandle%s getting short.",
					whose,
					menorah ? "candelabrum's " : "",
					many ? "s are" : " is");
#else
				    pline("%s%sろうそくは短くなった。",
					whose,
					menorah ? "燭台の" : "");
#endif /*JP*/
				    break;
				case OBJ_FLOOR:
#ifndef JP
				    You("see %scandle%s getting short.",
					    menorah ? "a candelabrum's " :
						many ? "some " : "a ",
					    many ? "s" : "");
#else
				    You("%sろうそくが短くなるのを見た。",
					    menorah ? "燭台の" : "");
#endif /*JP*/
				    break;
			    }
			break;

		    case 15:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline(
#ifndef JP
					"%s %scandle%s flame%s flicker%s low!",
					    whose,
					    menorah ? "candelabrum's " : "",
					    many ? "s'" : "'s",
					    many ? "s" : "",
					    many ? "" : "s");
#else
					"%s%sろうそくの炎は弱くなった！",
					    whose, menorah ? "燭台の" : "");
#endif /*JP*/
				    break;
				case OBJ_FLOOR:
#ifndef JP
				    You("see %scandle%s flame%s flicker low!",
					    menorah ? "a candelabrum's " :
						many ? "some " : "a ",
					    many ? "s'" : "'s",
					    many ? "s" : "");
#else
				    You("%sろうそくの炎が弱くなるのを見た！",
					    menorah ? "燭台の" : "");
#endif /*JP*/
				    break;
			    }
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
			    if (menorah) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
#ifndef JP
					pline("%s candelabrum's flame%s.",
					    whose,
					    many ? "s die" : " dies");
#else
					pline("%s燭台の炎は消えた。", whose);
#endif /*JP*/
					break;
				    case OBJ_FLOOR:
#ifndef JP
					You("see a candelabrum's flame%s die.",
						many ? "s" : "");
#else
					You("燭台の炎が消えるのを見た。");
#endif /*JP*/
					break;
				}
			    } else {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
#ifndef JP
					pline("%s %s %s consumed!",
					    whose,
					    xname(obj),
					    many ? "are" : "is");
#else
					pline("%s%sは燃え尽きた！",
					    whose, xname(obj));
#endif /*JP*/
					break;
				    case OBJ_FLOOR:
					/*
					You see some wax candles consumed!
					You see a wax candle consumed!
					*/
#ifndef JP
					You("see %s%s consumed!",
					    many ? "some " : "",
					    many ? xname(obj):an(xname(obj)));
#else
					You("%sが燃え尽きるのを見た！",
					    xname(obj));
#endif /*JP*/
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ?
#ifndef JP
					(many ? "They shriek!" :
						"It shrieks!") :
#else
						"ろうそくは断末魔の叫びを上げた！" :
#endif /*JP*/
					Blind ? "" :
#ifndef JP
					    (many ? "Their flames die." :
						    "Its flame dies."));
#else
						    "炎は消えた。");
#endif /*JP*/
			    }
			}
			end_burn(obj, FALSE);

			if (menorah) {
			    obj->spe = 0;
			} else {
			    obj_extract_self(obj);
			    obfree(obj, (struct obj *)0);
			    obj = (struct obj *) 0;
			}
			break;

		    default:
			/*
			 * Someone added fuel (candles) to the menorah while
			 * it was lit.  Just fall through and let begin burn
			 * handle the new age.
			 */
			break;
		}

		if (obj && obj->age)
		    begin_burn(obj, TRUE);

		break;

	    default:
		impossible("burn_object: unexpeced obj %s", xname(obj));
		break;
	}
	if (need_newsym) newsym(x, y);
}

/*
 * Start a burn timeout on the given object. If not "already lit" then
 * create a light source for the vision system.  There had better not
 * be a burn already running on the object.
 *
 * Magic lamps stay lit as long as there's a genie inside, so don't start
 * a timer.
 *
 * Burn rules:
 *	potions of oil, lamps & candles:
 *		age = # of turns of fuel left
 *		spe = <unused>
 *
 *	magic lamps:
 *		age = <unused>
 *		spe = 0 not lightable, 1 lightable forever
 *
 *	candelabrum:
 *		age = # of turns of fuel left
 *		spe = # of candles
 *
 * Once the burn begins, the age will be set to the amount of fuel
 * remaining _once_the_burn_finishes_.  If the burn is terminated
 * early then fuel is added back.
 *
 * This use of age differs from the use of age for corpses and eggs.
 * For the latter items, age is when the object was created, so we
 * know when it becomes "bad".
 *
 * This is a "silent" routine - it should not print anything out.
 */
void
begin_burn(obj, already_lit)
	struct obj *obj;
	boolean already_lit;
{
	int radius = 3;
	long turns = 0;
	boolean do_timer = TRUE;

	if (obj->age == 0 && obj->otyp != MAGIC_LAMP && !artifact_light(obj))
	    return;

	switch (obj->otyp) {
	    case MAGIC_CANDLE:
		/*radius = candle_light_range(obj);*/
		/* fall through */
	    case MAGIC_LAMP:
		obj->lamplit = 1;
		do_timer = FALSE;
		break;

	    case POT_OIL:
		turns = obj->age;
		radius = 1;	/* very dim light */
		break;

	    case BRASS_LANTERN:
	    case OIL_LAMP:
		/* magic times are 150, 100, 50, 25, and 0 */
		if (obj->age > 150L)
		    turns = obj->age - 150L;
		else if (obj->age > 100L)
		    turns = obj->age - 100L;
		else if (obj->age > 50L)
		    turns = obj->age - 50L;
		else if (obj->age > 25L) {
		    turns = obj->age - 25L;
		    radius = 2;
		    if (already_lit)
			change_light_source_range(LS_OBJECT, obj, 2);
		} else {
		    turns = obj->age;
		    radius = 2;
		}
		break;

	    case CANDELABRUM_OF_INVOCATION:
		if (obj->mcandles7) {
		    obj->lamplit = 1;
		    do_timer = FALSE;
		    radius = candle_light_range(obj) + 1;
		    break;
		}
		/* fall through */
	    case TALLOW_CANDLE:
	    case WAX_CANDLE:
		/* magic times are 75, 15, and 0 */
		if (obj->age > 75L)
		    turns = obj->age - 75L;
		else if (obj->age > 15L)
		    turns = obj->age - 15L;
		else
		    turns = obj->age;
		radius = candle_light_range(obj);
		break;

	    default:
                /* [ALI] Support artifact light sources */
                if (artifact_light(obj)) {
		    obj->lamplit = 1;
		    do_timer = FALSE;
		    /*radius = 2;*/
		} else {
		    impossible("begin burn: unexpected %s", xname(obj));
		    turns = obj->age;
		}
		break;
	}

	if (do_timer) {
	    if (start_timer(turns, TIMER_OBJECT,
					BURN_OBJECT, (genericptr_t)obj)) {
		obj->lamplit = 1;
		obj->age -= turns;
		if (carried(obj) && !already_lit)
		    update_inventory();
	    } else {
		obj->lamplit = 0;
	    }
	} else {
	    if (carried(obj) && !already_lit)
		update_inventory();
	}

	if (obj->lamplit && !already_lit) {
	    xchar x, y;

	    if (get_obj_location(obj, &x, &y, CONTAINED_TOO|BURIED_TOO))
		new_light_source(x, y, radius, LS_OBJECT, (genericptr_t) obj);
	    else
		impossible("begin_burn: can't get obj position");
	}
}

/*
 * Stop a burn timeout on the given object if timer attached.  Darken
 * light source.
 */
void
end_burn(obj, timer_attached)
	struct obj *obj;
	boolean timer_attached;
{
	if (!obj->lamplit) {
	    impossible("end_burn: obj %s not lit", xname(obj));
	    return;
	}

	if (obj->otyp == MAGIC_LAMP || obj->otyp == MAGIC_CANDLE || artifact_light(obj) ||
	    (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->mcandles7))
	    timer_attached = FALSE;

	if (!timer_attached) {
	    /* [DS] Cleanup explicitly, since timer cleanup won't happen */
	    del_light_source(LS_OBJECT, (genericptr_t)obj);
	    obj->lamplit = 0;
	    if (obj->where == OBJ_INVENT)
		update_inventory();
	} else if (!stop_timer(BURN_OBJECT, (genericptr_t) obj))
	    impossible("end_burn: obj %s not timed!", xname(obj));
}

#endif /* OVL1 */
#ifdef OVL0

/*
 * Cleanup a burning object if timer stopped.
 */
static void
cleanup_burn(arg, expire_time)
    genericptr_t arg;
    long expire_time;
{
    struct obj *obj = (struct obj *)arg;
    if (!obj->lamplit) {
	impossible("cleanup_burn: obj %s not lit", xname(obj));
	return;
    }

    del_light_source(LS_OBJECT, arg);

    /* restore unused time */
    obj->age += expire_time - monstermoves;

    obj->lamplit = 0;

    if (obj->where == OBJ_INVENT)
	update_inventory();
}

#endif /* OVL0 */
#ifdef OVL1

void
do_storms()
{
    int nstrike;
    register int x, y;
    int dirx, diry;
    int count;
    struct zapinfo zi;

    /* no lightning if not the air level or too often, even then */
    if(!Is_airlevel(&u.uz) || rn2(8))
	return;

    /* the number of strikes is 8-log2(nstrike) */
    for(nstrike = rnd(64); nstrike <= 64; nstrike *= 2) {
	count = 0;
	do {
	    x = rnd(COLNO-1);
	    y = rn2(ROWNO);
	} while (++count < 100 && levl[x][y].typ != CLOUD);

	if(count < 100) {
	    dirx = rn2(9) - 4;
	    diry = rn2(9) - 4;
	    if(dirx != 0 || diry != 0)
		setup_zapinfo(&zi, AT_MAGC, AD_ELEC, 8, 6,
				   (const char *)0, (const char *)0, /* use default names */
				   (struct monst *)0);
		buzz(&zi, x, y, dirx, diry);
	}
    }

    if(levl[u.ux][u.uy].typ == CLOUD) {
	/* inside a cloud during a thunder storm is deafening */
	pline(E_J("Kaboom!!!  Boom!!  Boom!!",
		  "耳をつんざく雷鳴が轟いた！"));
	if(!u.uinvulnerable) {
	    stop_occupation();
	    nomul(-3);
	}
    } else
	You_hear(E_J("a rumbling noise.","遠雷を"));
}
#endif /* OVL1 */


#ifdef OVL0
/* ------------------------------------------------------------------------- */
/*
 * Generic Timeout Functions.
 *
 * Interface:
 *
 * General:
 *	boolean start_timer(long timeout,short kind,short func_index,
 *							genericptr_t arg)
 *		Start a timer of kind 'kind' that will expire at time
 *		monstermoves+'timeout'.  Call the function at 'func_index'
 *		in the timeout table using argument 'arg'.  Return TRUE if
 *		a timer was started.  This places the timer on a list ordered
 *		"sooner" to "later".  If an object, increment the object's
 *		timer count.
 *
 *	long stop_timer(short func_index, genericptr_t arg)
 *		Stop a timer specified by the (func_index, arg) pair.  This
 *		assumes that such a pair is unique.  Return the time the
 *		timer would have gone off.  If no timer is found, return 0.
 *		If an object, decrement the object's timer count.
 *
 *	void run_timers(void)
 *		Call timers that have timed out.
 *
 *
 * Save/Restore:
 *	void save_timers(int fd, int mode, int range)
 *		Save all timers of range 'range'.  Range is either global
 *		or local.  Global timers follow game play, local timers
 *		are saved with a level.  Object and monster timers are
 *		saved using their respective id's instead of pointers.
 *
 *	void restore_timers(int fd, int range, boolean ghostly, long adjust)
 *		Restore timers of range 'range'.  If from a ghost pile,
 *		adjust the timeout by 'adjust'.  The object and monster
 *		ids are not restored until later.
 *
 *	void relink_timers(boolean ghostly)
 *		Relink all object and monster timers that had been saved
 *		using their object's or monster's id number.
 *
 * Object Specific:
 *	void obj_move_timers(struct obj *src, struct obj *dest)
 *		Reassign all timers from src to dest.
 *
 *	void obj_split_timers(struct obj *src, struct obj *dest)
 *		Duplicate all timers assigned to src and attach them to dest.
 *
 *	void obj_stop_timers(struct obj *obj)
 *		Stop all timers attached to obj.
 */

#ifdef WIZARD
STATIC_DCL const char *FDECL(kind_name, (SHORT_P));
STATIC_DCL void FDECL(print_queue, (winid, timer_element *));
#endif
STATIC_DCL void FDECL(insert_timer, (timer_element *));
STATIC_DCL timer_element *FDECL(remove_timer, (timer_element **, SHORT_P,
								genericptr_t));
STATIC_DCL void FDECL(write_timer, (int, timer_element *));
STATIC_DCL boolean FDECL(mon_is_local, (struct monst *));
STATIC_DCL boolean FDECL(timer_is_local, (timer_element *));
STATIC_DCL int FDECL(maybe_write_timer, (int, int, BOOLEAN_P));

/* ordered timer list */
static timer_element *timer_base;		/* "active" */
static unsigned long timer_id = 1;

/* If defined, then include names when printing out the timer queue */
#define VERBOSE_TIMER

typedef struct {
    timeout_proc f, cleanup;
#ifdef VERBOSE_TIMER
    const char *name;
# define TTAB(a, b, c) {a,b,c}
#else
# define TTAB(a, b, c) {a,b}
#endif
} ttable;

/* table of timeout functions */
static const ttable timeout_funcs[NUM_TIME_FUNCS] = {
    TTAB(rot_organic,	(timeout_proc)0,	"rot_organic"),
    TTAB(rot_corpse,	(timeout_proc)0,	"rot_corpse"),
    TTAB(revive_mon,	(timeout_proc)0,	"revive_mon"),
    TTAB(burn_object,	cleanup_burn,		"burn_object"),
    TTAB(hatch_egg,	(timeout_proc)0,	"hatch_egg"),
    TTAB(fig_transform,	(timeout_proc)0,	"fig_transform"),
};
#undef TTAB


#if defined(WIZARD)

STATIC_OVL const char *
kind_name(kind)
    short kind;
{
    switch (kind) {
	case TIMER_LEVEL: return "level";
	case TIMER_GLOBAL: return "global";
	case TIMER_OBJECT: return "object";
	case TIMER_MONSTER: return "monster";
    }
    return "unknown";
}

STATIC_OVL void
print_queue(win, base)
    winid win;
    timer_element *base;
{
    timer_element *curr;
    char buf[BUFSZ], arg_address[20];

    if (!base) {
	putstr(win, 0, "<empty>");
    } else {
	putstr(win, 0, "timeout  id   kind   call");
	for (curr = base; curr; curr = curr->next) {
#ifdef VERBOSE_TIMER
	    Sprintf(buf, " %4ld   %4ld  %-6s %s(%s)",
		curr->timeout, curr->tid, kind_name(curr->kind),
		timeout_funcs[curr->func_index].name,
		fmt_ptr((genericptr_t)curr->arg, arg_address));
#else
	    Sprintf(buf, " %4ld   %4ld  %-6s #%d(%s)",
		curr->timeout, curr->tid, kind_name(curr->kind),
		curr->func_index,
		fmt_ptr((genericptr_t)curr->arg, arg_address));
#endif
	    putstr(win, 0, buf);
	}
    }
}

int
wiz_timeout_queue()
{
    winid win;
    char buf[BUFSZ];

    win = create_nhwindow(NHW_MENU);	/* corner text window */
    if (win == WIN_ERR) return 0;

    Sprintf(buf, "Current time = %ld.", monstermoves);
    putstr(win, 0, buf);
    putstr(win, 0, "");
    putstr(win, 0, "Active timeout queue:");
    putstr(win, 0, "");
    print_queue(win, timer_base);

    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);

    return 0;
}

void
timer_sanity_check()
{
    timer_element *curr;
    char obj_address[20];

    /* this should be much more complete */
    for (curr = timer_base; curr; curr = curr->next)
	if (curr->kind == TIMER_OBJECT) {
	    struct obj *obj = (struct obj *) curr->arg;
	    if (obj->timed == 0) {
		pline("timer sanity: untimed obj %s, timer %ld",
		      fmt_ptr((genericptr_t)obj, obj_address), curr->tid);
	    }
	}
}

#endif /* WIZARD */


/*
 * Pick off timeout elements from the global queue and call their functions.
 * Do this until their time is less than or equal to the move count.
 */
void
run_timers()
{
    timer_element *curr;

    /*
     * Always use the first element.  Elements may be added or deleted at
     * any time.  The list is ordered, we are done when the first element
     * is in the future.
     */
    while (timer_base && timer_base->timeout <= monstermoves) {
	curr = timer_base;
	timer_base = curr->next;

	if (curr->kind == TIMER_OBJECT) ((struct obj *)(curr->arg))->timed--;
	(*timeout_funcs[curr->func_index].f)(curr->arg, curr->timeout);
	free((genericptr_t) curr);
    }
}


/*
 * Start a timer.  Return TRUE if successful.
 */
boolean
start_timer(when, kind, func_index, arg)
long when;
short kind;
short func_index;
genericptr_t arg;
{
    timer_element *gnu;

    if (func_index < 0 || func_index >= NUM_TIME_FUNCS)
	panic("start_timer");

    gnu = (timer_element *) alloc(sizeof(timer_element));
    gnu->next = 0;
    gnu->tid = timer_id++;
    gnu->timeout = monstermoves + when;
    gnu->kind = kind;
    gnu->needs_fixup = 0;
    gnu->func_index = func_index;
    gnu->arg = arg;
    insert_timer(gnu);

    if (kind == TIMER_OBJECT)	/* increment object's timed count */
	((struct obj *)arg)->timed++;

    /* should check for duplicates and fail if any */
    return TRUE;
}


/*
 * Remove the timer from the current list and free it up.  Return the time
 * it would have gone off, 0 if not found.
 */
long
stop_timer(func_index, arg)
short func_index;
genericptr_t arg;
{
    timer_element *doomed;
    long timeout;

    doomed = remove_timer(&timer_base, func_index, arg);

    if (doomed) {
	timeout = doomed->timeout;
	if (doomed->kind == TIMER_OBJECT)
	    ((struct obj *)arg)->timed--;
	if (timeout_funcs[doomed->func_index].cleanup)
	    (*timeout_funcs[doomed->func_index].cleanup)(arg, timeout);
	free((genericptr_t) doomed);
	return timeout;
    }
    return 0;
}


/*
 * Move all object timers from src to dest, leaving src untimed.
 */
void
obj_move_timers(src, dest)
    struct obj *src, *dest;
{
    int count;
    timer_element *curr;

    for (count = 0, curr = timer_base; curr; curr = curr->next)
	if (curr->kind == TIMER_OBJECT && curr->arg == (genericptr_t)src) {
	    curr->arg = (genericptr_t) dest;
	    dest->timed++;
	    count++;
	}
    if (count != src->timed)
	panic("obj_move_timers");
    src->timed = 0;
}


/*
 * Find all object timers and duplicate them for the new object "dest".
 */
void
obj_split_timers(src, dest)
    struct obj *src, *dest;
{
    timer_element *curr, *next_timer=0;

    for (curr = timer_base; curr; curr = next_timer) {
	next_timer = curr->next;	/* things may be inserted */
	if (curr->kind == TIMER_OBJECT && curr->arg == (genericptr_t)src) {
	    (void) start_timer(curr->timeout-monstermoves, TIMER_OBJECT,
					curr->func_index, (genericptr_t)dest);
	}
    }
}


/*
 * Stop all timers attached to this object.  We can get away with this because
 * all object pointers are unique.
 */
void
obj_stop_timers(obj)
    struct obj *obj;
{
    timer_element *curr, *prev, *next_timer=0;

    for (prev = 0, curr = timer_base; curr; curr = next_timer) {
	next_timer = curr->next;
	if (curr->kind == TIMER_OBJECT && curr->arg == (genericptr_t)obj) {
	    if (prev)
		prev->next = curr->next;
	    else
		timer_base = curr->next;
	    if (timeout_funcs[curr->func_index].cleanup)
		(*timeout_funcs[curr->func_index].cleanup)(curr->arg,
			curr->timeout);
	    free((genericptr_t) curr);
	} else {
	    prev = curr;
	}
    }
    obj->timed = 0;
}


/* Insert timer into the global queue */
STATIC_OVL void
insert_timer(gnu)
    timer_element *gnu;
{
    timer_element *curr, *prev;

    for (prev = 0, curr = timer_base; curr; prev = curr, curr = curr->next)
	if (curr->timeout >= gnu->timeout) break;

    gnu->next = curr;
    if (prev)
	prev->next = gnu;
    else
	timer_base = gnu;
}


STATIC_OVL timer_element *
remove_timer(base, func_index, arg)
timer_element **base;
short func_index;
genericptr_t arg;
{
    timer_element *prev, *curr;

    for (prev = 0, curr = *base; curr; prev = curr, curr = curr->next)
	if (curr->func_index == func_index && curr->arg == arg) break;

    if (curr) {
	if (prev)
	    prev->next = curr->next;
	else
	    *base = curr->next;
    }

    return curr;
}


STATIC_OVL void
write_timer(fd, timer)
    int fd;
    timer_element *timer;
{
    genericptr_t arg_save;

    switch (timer->kind) {
	case TIMER_GLOBAL:
	case TIMER_LEVEL:
	    /* assume no pointers in arg */
	    bwrite(fd, (genericptr_t) timer, sizeof(timer_element));
	    break;

	case TIMER_OBJECT:
	    if (timer->needs_fixup)
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
	    else {
		/* replace object pointer with id */
		arg_save = timer->arg;
		timer->arg = (genericptr_t)((struct obj *)timer->arg)->o_id;
		timer->needs_fixup = 1;
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
		timer->arg = arg_save;
		timer->needs_fixup = 0;
	    }
	    break;

	case TIMER_MONSTER:
	    if (timer->needs_fixup)
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
	    else {
		/* replace monster pointer with id */
		arg_save = timer->arg;
		timer->arg = (genericptr_t)((struct monst *)timer->arg)->m_id;
		timer->needs_fixup = 1;
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
		timer->arg = arg_save;
		timer->needs_fixup = 0;
	    }
	    break;

	default:
	    panic("write_timer");
	    break;
    }
}


/*
 * Return TRUE if the object will stay on the level when the level is
 * saved.
 */
boolean
obj_is_local(obj)
    struct obj *obj;
{
    switch (obj->where) {
	case OBJ_INVENT:
	case OBJ_MIGRATING:	return FALSE;
	case OBJ_FLOOR:
	case OBJ_BURIED:	return TRUE;
	case OBJ_CONTAINED:	return obj_is_local(obj->ocontainer);
	case OBJ_MINVENT:	return mon_is_local(obj->ocarry);
    }
    panic("obj_is_local");
    return FALSE;
}


/*
 * Return TRUE if the given monster will stay on the level when the
 * level is saved.
 */
STATIC_OVL boolean
mon_is_local(mon)
struct monst *mon;
{
    struct monst *curr;

    for (curr = migrating_mons; curr; curr = curr->nmon)
	if (curr == mon) return FALSE;
    /* `mydogs' is used during level changes, never saved and restored */
    for (curr = mydogs; curr; curr = curr->nmon)
	if (curr == mon) return FALSE;
    return TRUE;
}


/*
 * Return TRUE if the timer is attached to something that will stay on the
 * level when the level is saved.
 */
STATIC_OVL boolean
timer_is_local(timer)
    timer_element *timer;
{
    switch (timer->kind) {
	case TIMER_LEVEL:	return TRUE;
	case TIMER_GLOBAL:	return FALSE;
	case TIMER_OBJECT:	return obj_is_local((struct obj *)timer->arg);
	case TIMER_MONSTER:	return mon_is_local((struct monst *)timer->arg);
    }
    panic("timer_is_local");
    return FALSE;
}


/*
 * Part of the save routine.  Count up the number of timers that would
 * be written.  If write_it is true, actually write the timer.
 */
STATIC_OVL int
maybe_write_timer(fd, range, write_it)
    int fd, range;
    boolean write_it;
{
    int count = 0;
    timer_element *curr;

    for (curr = timer_base; curr; curr = curr->next) {
	if (range == RANGE_GLOBAL) {
	    /* global timers */

	    if (!timer_is_local(curr)) {
		count++;
		if (write_it) write_timer(fd, curr);
	    }

	} else {
	    /* local timers */

	    if (timer_is_local(curr)) {
		count++;
		if (write_it) write_timer(fd, curr);
	    }

	}
    }

    return count;
}


/*
 * Save part of the timer list.  The parameter 'range' specifies either
 * global or level timers to save.  The timer ID is saved with the global
 * timers.
 *
 * Global range:
 *		+ timeouts that follow the hero (global)
 *		+ timeouts that follow obj & monst that are migrating
 *
 * Level range:
 *		+ timeouts that are level specific (e.g. storms)
 *		+ timeouts that stay with the level (obj & monst)
 */
void
save_timers(fd, mode, range)
    int fd, mode, range;
{
    timer_element *curr, *prev, *next_timer=0;
    int count;

    if (perform_bwrite(mode)) {
	if (range == RANGE_GLOBAL)
	    bwrite(fd, (genericptr_t) &timer_id, sizeof(timer_id));

	count = maybe_write_timer(fd, range, FALSE);
	bwrite(fd, (genericptr_t) &count, sizeof count);
	(void) maybe_write_timer(fd, range, TRUE);
    }

    if (release_data(mode)) {
	for (prev = 0, curr = timer_base; curr; curr = next_timer) {
	    next_timer = curr->next;	/* in case curr is removed */

	    if ( !(!!(range == RANGE_LEVEL) ^ !!timer_is_local(curr)) ) {
		if (prev)
		    prev->next = curr->next;
		else
		    timer_base = curr->next;
		free((genericptr_t) curr);
		/* prev stays the same */
	    } else {
		prev = curr;
	    }
	}
    }
}


/*
 * Pull in the structures from disk, but don't recalculate the object and
 * monster pointers.
 */
void
restore_timers(fd, range, ghostly, adjust)
    int fd, range;
    boolean ghostly;	/* restoring from a ghost level */
    long adjust;	/* how much to adjust timeout */
{
    int count;
    timer_element *curr;

    if (range == RANGE_GLOBAL)
	mread(fd, (genericptr_t) &timer_id, sizeof timer_id);

    /* restore elements */
    mread(fd, (genericptr_t) &count, sizeof count);
    while (count-- > 0) {
	curr = (timer_element *) alloc(sizeof(timer_element));
	mread(fd, (genericptr_t) curr, sizeof(timer_element));
	if (ghostly)
	    curr->timeout += adjust;
	insert_timer(curr);
    }
}


/* reset all timers that are marked for reseting */
void
relink_timers(ghostly)
    boolean ghostly;
{
    timer_element *curr;
    unsigned nid;

    for (curr = timer_base; curr; curr = curr->next) {
	if (curr->needs_fixup) {
	    if (curr->kind == TIMER_OBJECT) {
		if (ghostly) {
		    if (!lookup_id_mapping((unsigned)curr->arg, &nid))
			panic("relink_timers 1");
		} else
		    nid = (unsigned) curr->arg;
		curr->arg = (genericptr_t) find_oid(nid);
		if (!curr->arg) panic("cant find o_id %d", nid);
		curr->needs_fixup = 0;
	    } else if (curr->kind == TIMER_MONSTER) {
		panic("relink_timers: no monster timer implemented");
	    } else
		panic("relink_timers 2");
	}
    }
}

#endif /* OVL0 */

/*timeout.c*/
