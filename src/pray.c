/*	SCCS Id: @(#)pray.c	3.4	2003/03/23	*/
/* Copyright (c) Benson I. Margulies, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "epri.h"

STATIC_PTR int NDECL(prayer_done);
STATIC_DCL struct obj *NDECL(worst_cursed_item);
STATIC_DCL int NDECL(in_trouble);
STATIC_DCL void FDECL(fix_worst_trouble,(int));
STATIC_DCL void FDECL(angrygods,(ALIGNTYP_P));
STATIC_DCL void FDECL(at_your_feet, (const char *));
#ifdef ELBERETH
STATIC_DCL void NDECL(gcrownu);
#endif	/*ELBERETH*/
STATIC_DCL void FDECL(pleased,(ALIGNTYP_P));
STATIC_DCL void FDECL(godvoice,(ALIGNTYP_P,const char*));
STATIC_DCL void FDECL(god_zaps_you,(ALIGNTYP_P));
STATIC_DCL void FDECL(fry_by_god,(ALIGNTYP_P));
STATIC_DCL void FDECL(gods_angry,(ALIGNTYP_P));
STATIC_DCL void FDECL(gods_upset,(ALIGNTYP_P));
STATIC_DCL void FDECL(consume_offering,(struct obj *));
STATIC_DCL boolean FDECL(water_prayer,(BOOLEAN_P));
STATIC_DCL boolean FDECL(blocked_boulder,(int,int));
STATIC_DCL int FDECL(getobj_filter_sacrifice, (struct obj *));
STATIC_OVL void NDECL(chromatic_dragon_ending);


/* simplify a few tests */
#define Cursed_obj(obj,typ) ((obj) && (obj)->otyp == (typ) && (obj)->cursed)

/*
 * Logic behind deities and altars and such:
 * + prayers are made to your god if not on an altar, and to the altar's god
 *   if you are on an altar
 * + If possible, your god answers all prayers, which is why bad things happen
 *   if you try to pray on another god's altar
 * + sacrifices work basically the same way, but the other god may decide to
 *   accept your allegiance, after which they are your god.  If rejected,
 *   your god takes over with your punishment.
 * + if you're in Gehennom, all messages come from Moloch
 */

/*
 *	Moloch, who dwells in Gehennom, is the "renegade" cruel god
 *	responsible for the theft of the Amulet from Marduk, the Creator.
 *	Moloch is unaligned.
 */
static const char	*Moloch = E_J("Moloch","モロク");

static const char *godvoices[] = {
    E_J("booms out","響き渡った"),
    E_J("thunders","轟いた"),
    E_J("rings out","鳴り響いた"),
    E_J("booms","響いた"),
};

static const char *bogusgods[] = {
#ifndef JP
	"Aslan", "Tash",	/* Narnia */
	"Bel-Shamharoth",	/* Discworld */
	"Deep Thought",		/* The Hitchhiker's Guide to the Galaxy */
	"The Childlike Empress",/* The Neverending Story */
	"Kadorto",		/* Wizardry */
	"Lord British",		/* Ultima */
	"Galsis",		/* XANADU */
	"Gayzac",		/* Hydlide 3 */
	"Rubiss",		/* Dragon Quest */
	"Bacterian",		/* Gradius */
	"Belser",		/* Darius */
	"Bydo",			/* R-Type */
	"the Greason System",	/* Silpheed SegaCD */
	"Sestren",		/* Panzer Dragoon Saga */
#else
	"アスラン", "タシ",
	"ベル=シャムァロス",
	"ディープ・ソート",
	"幼ごころの君",
	"カドルト",
	"ロード・ブリティッシュ",
	"ガルシス",
	"ガイザック",
	"ルビス",
	"バクテリアン",
	"ベルサー",
	"バイド",
	"グレイゾン・システム",
	"セストレン",
#endif /*JP*/
};

/* values calculated when prayer starts, and used when completed */
static aligntyp p_aligntyp;
static int p_trouble;
static int p_type; /* (-1)-3: (-1)=really naughty, 3=really good */

#define PIOUS 20
#define DEVOUT 14
#define FERVENT 9
#define STRIDENT 4

/*
 * The actual trouble priority is determined by the order of the
 * checks performed in in_trouble() rather than by these numeric
 * values, so keep that code and these values synchronized in
 * order to have the values be meaningful.
 */

#define TROUBLE_STONED			14
#define TROUBLE_SLIMED			13
#define TROUBLE_STRANGLED		12
#define TROUBLE_LAVA			11
#define TROUBLE_SICK			10
#define TROUBLE_STARVING		 9
#define TROUBLE_HIT			 8
#define TROUBLE_LYCANTHROPE		 7
#define TROUBLE_COLLAPSING		 6
#define TROUBLE_STUCK_IN_WALL		 5
#define TROUBLE_CURSED_LEVITATION	 4
#define TROUBLE_UNUSEABLE_HANDS		 3
#define TROUBLE_CURSED_BLINDFOLD	 2
#define TROUBLE_POISONED		 1

#define TROUBLE_PUNISHED	       (-1)
#define TROUBLE_FUMBLING	       (-2)
#define TROUBLE_CURSED_ITEMS	       (-3)
#define TROUBLE_SADDLE		       (-4)
#define TROUBLE_BLIND		       (-5)
#define TROUBLE_WOUNDED_LEGS	       (-6)
#define TROUBLE_HUNGRY		       (-7)
#define TROUBLE_STUNNED		       (-8)
#define TROUBLE_CONFUSED	       (-9)
#define TROUBLE_HALLUCINATION	      (-10)

/* We could force rehumanize of polyselfed people, but we can't tell
   unintentional shape changes from the other kind. Oh well.
   3.4.2: make an exception if polymorphed into a form which lacks
   hands; that's a case where the ramifications override this doubt.
 */

/* Return 0 if nothing particular seems wrong, positive numbers for
   serious trouble, and negative numbers for comparative annoyances. This
   returns the worst problem. There may be others, and the gods may fix
   more than one.

This could get as bizarre as noting surrounding opponents, (or hostile dogs),
but that's really hard.
 */

#define ugod_is_angry() (u.ualign.record < 0)
#define on_altar()	IS_ALTAR(levl[u.ux][u.uy].typ)
#define on_shrine()	((levl[u.ux][u.uy].altarmask & AM_SHRINE) != 0)
#define a_align(x,y)	((aligntyp)Amask2align(levl[x][y].altarmask & AM_MASK))

/* critically low hit points if hp <= 5 or hp <= maxhp/N for some N */
boolean
critically_low_hp(only_if_injured)
boolean only_if_injured; /* determines whether maxhp <= 5 matters */
{
    int divisor, hplim, curhp = Upolyd ? u.mh : u.uhp,
                        maxhp = Upolyd ? u.mhmax : u.uhpmax;

    if (only_if_injured && !(curhp < maxhp))
        return FALSE;
    /* if maxhp is extremely high, use lower threshold for the division test
       (golden glow cuts off at 11+5*lvl, nurse interaction at 25*lvl; this
       ought to use monster hit dice--and a smaller multiplier--rather than
       ulevel when polymorphed, but polyself doesn't maintain that) */
    hplim = 15 * u.ulevel;
    if (maxhp > hplim)
        maxhp = hplim;
    /* 7 used to be the unconditional divisor */
    switch (xlev_to_rank(u.ulevel)) { /* maps 1..30 into 0..8 */
    case 0:
    case 1:
        divisor = 5;
        break; /* explvl 1 to 5 */
    case 2:
    case 3:
        divisor = 6;
        break; /* explvl 6 to 13 */
    case 4:
    case 5:
        divisor = 7;
        break; /* explvl 14 to 21 */
    case 6:
    case 7:
        divisor = 8;
        break; /* explvl 22 to 29 */
    default:
        divisor = 9;
        break; /* explvl 30+ */
    }
    /* 5 is a magic number in TROUBLE_HIT handling below */
    return (boolean) (curhp <= 5 || curhp * divisor <= maxhp);
}

STATIC_OVL int
in_trouble()
{
	struct obj *otmp;
	int i, j, count=0;

/* Borrowed from eat.c */

#define SATIATED	0
#define NOT_HUNGRY	1
#define HUNGRY		2
#define WEAK		3
#define FAINTING	4
#define FAINTED		5
#define STARVED		6

	/*
	 * major troubles
	 */
	if(Stoned) return(TROUBLE_STONED);
	if(Slimed) return(TROUBLE_SLIMED);
	if(Strangled) return(TROUBLE_STRANGLED);
	if(u.utrap && u.utraptype == TT_LAVA) return(TROUBLE_LAVA);
	if(Sick) return(TROUBLE_SICK);
	if(u.uhs >= WEAK) return(TROUBLE_STARVING);
	if (critically_low_hp(FALSE))
	    return TROUBLE_HIT;
	if(u.ulycn >= LOW_PM) return(TROUBLE_LYCANTHROPE);
	if(near_capacity() >= EXT_ENCUMBER && AMAX(A_STR)-ABASE(A_STR) > 3)
		return(TROUBLE_COLLAPSING);

	for (i= -1; i<=1; i++) for(j= -1; j<=1; j++) {
		if (!i && !j) continue;
		if (!isok(u.ux+i, u.uy+j) || IS_ROCK(levl[u.ux+i][u.uy+j].typ)
		    || (blocked_boulder(i,j) && !throws_rocks(youmonst.data)))
			count++;
	}
	if (count == 8 && !Passes_walls)
		return(TROUBLE_STUCK_IN_WALL);

	if (Cursed_obj(uarmf, LEVITATION_BOOTS) ||
		stuck_ring(uleft, RIN_LEVITATION) ||
		stuck_ring(uright, RIN_LEVITATION))
		return(TROUBLE_CURSED_LEVITATION);
	if (nohands(youmonst.data) || !freehand()) {
	    /* for bag/box access [cf use_container()]...
	       make sure it's a case that we know how to handle;
	       otherwise "fix all troubles" would get stuck in a loop */
	    if (welded(uwep)) return TROUBLE_UNUSEABLE_HANDS;
	    if (Upolyd && nohands(youmonst.data) && (!Unchanging ||
		    ((otmp = unchanger()) != 0 && otmp->cursed)))
		return TROUBLE_UNUSEABLE_HANDS;
	}
	if(Blindfolded && ublindf->cursed) return(TROUBLE_CURSED_BLINDFOLD);
	for(i=0; i<A_MAX; i++)
	    if(ABASE(i) < AMAX(i)) return(TROUBLE_POISONED);

	/*
	 * minor troubles
	 */
	if(Punished) return(TROUBLE_PUNISHED);
	if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING) ||
		Cursed_obj(uarmf, FUMBLE_BOOTS))
	    return TROUBLE_FUMBLING;
	if (worst_cursed_item()) return TROUBLE_CURSED_ITEMS;
#ifdef STEED
	if (u.usteed) {	/* can't voluntarily dismount from a cursed saddle */
	    otmp = which_armor(u.usteed, W_SADDLE);
	    if (Cursed_obj(otmp, SADDLE)) return TROUBLE_SADDLE;
	}
#endif

	if (Blinded > 1 && haseyes(youmonst.data)) return(TROUBLE_BLIND);
	if(Wounded_legs
#ifdef STEED
		    && !u.usteed
#endif
				) return (TROUBLE_WOUNDED_LEGS);
	if(u.uhs >= HUNGRY) return(TROUBLE_HUNGRY);
	if(HStun) return (TROUBLE_STUNNED);
	if(HConfusion) return (TROUBLE_CONFUSED);
	if(Hallucination) return(TROUBLE_HALLUCINATION);
	return(0);
}

/* select an item for TROUBLE_CURSED_ITEMS */
STATIC_OVL struct obj *
worst_cursed_item()
{
    register struct obj *otmp;

    /* if strained or worse, check for loadstone first */
    if (near_capacity() >= HVY_ENCUMBER) {
	for (otmp = invent; otmp; otmp = otmp->nobj)
	    if (Cursed_obj(otmp, LOADSTONE)) return otmp;
    }
    /* weapon takes precedence if it is interfering
       with taking off a ring or putting on a shield */
    if (welded(uwep) && (uright || bimanual(uwep))) {	/* weapon */
	otmp = uwep;
    /* gloves come next, due to rings */
    } else if (uarmg && uarmg->cursed) {		/* gloves */
	otmp = uarmg;
    /* then shield due to two handed weapons and spells */
    } else if (uarms && uarms->cursed) {		/* shield */
	otmp = uarms;
    /* then cloak due to body armor */
    } else if (uarmc && uarmc->cursed) {		/* cloak */
	otmp = uarmc;
    } else if (uarm && uarm->cursed) {			/* suit */
	otmp = uarm;
    } else if (uarmh && uarmh->cursed) {		/* helmet */
	otmp = uarmh;
    } else if (uarmf && uarmf->cursed) {		/* boots */
	otmp = uarmf;
    } else if (uarmu && uarmu->cursed) {		/* shirt */
	otmp = uarmu;
    } else if (uamul && uamul->cursed) {		/* amulet */
	otmp = uamul;
    } else if (uleft && uleft->cursed) {		/* left ring */
	otmp = uleft;
    } else if (uright && uright->cursed) {		/* right ring */
	otmp = uright;
    } else if (ublindf && ublindf->cursed) {		/* eyewear */
	otmp = ublindf;	/* must be non-blinding lenses */
    /* if weapon wasn't handled above, do it now */
    } else if (welded(uwep)) {				/* weapon */
	otmp = uwep;
    /* active secondary weapon even though it isn't welded */
    } else if (uswapwep && uswapwep->cursed && u.twoweap) {
	otmp = uswapwep;
    /* all worn items ought to be handled by now */
    } else {
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (!otmp->cursed) continue;
	    if (otmp->otyp == LOADSTONE || confers_luck(otmp))
		break;
	}
    }
    return otmp;
}

STATIC_OVL void
fix_worst_trouble(trouble)
register int trouble;
{
	int i;
	struct obj *otmp = 0;
	const char *what = (const char *)0;
#ifndef JP
	static NEARDATA const char leftglow[] = "left ring softly glows",
				   rightglow[] = "right ring softly glows";
#else
	static NEARDATA const char leftglow[] = "左手の指輪",
				   rightglow[] = "右手の指輪";
#endif /*JP*/

	switch (trouble) {
	    case TROUBLE_STONED:
		    You_feel(E_J("more limber.","より柔らかくなったようだ。"));
		    Stoned = 0;
		    flags.botl = 1;
		    remove_delayed_killer(STONED);
//		    delayed_killer = 0;
		    break;
	    case TROUBLE_SLIMED:
		    pline_The(E_J("slime disappears.","身体についたスライムが消えた。"));
		    Slimed = 0;
		    flags.botl = 1;
		    remove_delayed_killer(SLIMED);
//		    delayed_killer = 0;
		    break;
	    case TROUBLE_STRANGLED:
		    if (uamul && uamul->otyp == AMULET_OF_STRANGULATION) {
			struct obj *otmp;
			Your(E_J("amulet vanishes!","魔除けは消滅した！"));
			otmp = uamul;
			Amulet_off();
			useup(otmp);
		    }
		    if (Strangled) {
			(void) safe_teleds(FALSE);
		    } else {
//			You("can breathe again.");
//			Strangled = 0;
			flags.botl = 1;
		    }
		    break;
	    case TROUBLE_LAVA:
		    You(E_J("are back on solid ground.",
			    "固い地面の上に戻ってきた。"));
		    /* teleport should always succeed, but if not,
		     * just untrap them.
		     */
		    if(!safe_teleds(FALSE))
			u.utrap = 0;
		    break;
	    case TROUBLE_STARVING:
		    losestr(-1);
		    /* fall into... */
	    case TROUBLE_HUNGRY:
		    Your(E_J("%s feels content.","%sは満たされたようだ。"), body_part(STOMACH));
		    init_uhunger();
		    flags.botl = 1;
		    break;
	    case TROUBLE_SICK:
		    You_feel(E_J("better.","元気になった。"));
		    make_sick(0L, (char *) 0, FALSE, SICK_ALL);
		    break;
	    case TROUBLE_HIT:
		    /* "fix all troubles" will keep trying if hero has
		       5 or less hit points, so make sure they're always
		       boosted to be more than that */
		    You_feel(E_J("much better.","とても元気になった。"));
		    if (Upolyd) {
			u.mhmax += rnd(5);
			if (u.mhmax <= 5) u.mhmax = 5+1;
			u.mh = u.mhmax;
		    }
		    if (u.uhpmax < u.ulevel * 5 + 11) {
			if (u.uhpbase <= 5)
			    u.uhpbase = 5;
			else
			    addhpmax(rnd(5));
			recalchpmax();
		    }
		    u.uhp = u.uhpmax;
		    flags.botl = 1;
		    break;
	    case TROUBLE_COLLAPSING:
		    ABASE(A_STR) = AMAX(A_STR);
		    flags.botl = 1;
		    break;
	    case TROUBLE_STUCK_IN_WALL:
		    Your(E_J("surroundings change.","周囲が変化した。"));
		    /* no control, but works on no-teleport levels */
		    (void) safe_teleds(FALSE);
		    break;
	    case TROUBLE_CURSED_LEVITATION:
		    if (Cursed_obj(uarmf, LEVITATION_BOOTS)) {
			otmp = uarmf;
		    } else if ((otmp = stuck_ring(uleft,RIN_LEVITATION)) !=0) {
			if (otmp == uleft) what = leftglow;
		    } else if ((otmp = stuck_ring(uright,RIN_LEVITATION))!=0) {
			if (otmp == uright) what = rightglow;
		    }
		    goto decurse;
	    case TROUBLE_UNUSEABLE_HANDS:
		    if (welded(uwep)) {
			otmp = uwep;
			goto decurse;
		    }
		    if (Upolyd && nohands(youmonst.data)) {
			if (!Unchanging) {
			    Your(E_J("shape becomes uncertain.",
				     "姿が不確かになりはじめた。"));
			    rehumanize();  /* "You return to {normal} form." */
			} else if ((otmp = unchanger()) != 0 && otmp->cursed) {
			    /* otmp is an amulet of unchanging */
			    goto decurse;
			}
		    }
		    if (nohands(youmonst.data) || !freehand())
			impossible("fix_worst_trouble: couldn't cure hands.");
		    break;
	    case TROUBLE_CURSED_BLINDFOLD:
		    otmp = ublindf;
		    goto decurse;
	    case TROUBLE_LYCANTHROPE:
		    you_unwere(TRUE);
		    break;
	/*
	 */
	    case TROUBLE_PUNISHED:
		    Your(E_J("chain disappears.","鎖は消え去った。"));
		    unpunish();
		    break;
	    case TROUBLE_FUMBLING:
		    if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING))
			otmp = uarmg;
		    else if (Cursed_obj(uarmf, FUMBLE_BOOTS))
			otmp = uarmf;
		    goto decurse;
		    /*NOTREACHED*/
		    break;
	    case TROUBLE_CURSED_ITEMS:
		    otmp = worst_cursed_item();
		    if (otmp == uright) what = rightglow;
		    else if (otmp == uleft) what = leftglow;
decurse:
		    if (!otmp) {
			impossible("fix_worst_trouble: nothing to uncurse.");
			return;
		    }
		    uncurse(otmp);
		    if (!Blind) {
#ifndef JP
			Your("%s %s.", what ? what :
				(const char *)aobjnam(otmp, "softly glow"),
			     hcolor(NH_AMBER));
#else
			Your("%sが%s柔らかく輝いた。",
			     what ? what : cxname(otmp),
			     j_no_ni(hcolor(NH_AMBER)));
#endif /*JP*/
			otmp->bknown = TRUE;
		    }
		    update_inventory();
		    break;
	    case TROUBLE_POISONED:
		    if (Hallucination)
			pline(E_J("There's a tiger in your tank.",
				  "あなたのタンクの中に虎がいる。"));
		    else
#ifndef JP
			You_feel("in good health again.");
#else
			Your("健康状態は回復した。");
#endif /*JP*/
		    for(i=0; i<A_MAX; i++) {
			if(ABASE(i) < AMAX(i)) {
				ABASE(i) = AMAX(i);
				flags.botl = 1;
			}
		    }
		    (void) encumber_msg();
		    break;
	    case TROUBLE_BLIND:
		{
		    int num_eyes = eyecount(youmonst.data);
		    const char *eye = body_part(EYE);

#ifndef JP
		    Your("%s feel%s better.",
			 (num_eyes == 1) ? eye : makeplural(eye),
			 (num_eyes == 1) ? "s" : "");
#else
		    Your("%sは再び見えるようになった。", eye);
#endif /*JP*/
		    u.ucreamed = 0;
		    make_blinded(0L,FALSE);
		    break;
		}
	    case TROUBLE_WOUNDED_LEGS:
		    heal_legs();
		    break;
	    case TROUBLE_STUNNED:
		    make_stunned(0L,TRUE);
		    break;
	    case TROUBLE_CONFUSED:
		    make_confused(0L,TRUE);
		    break;
	    case TROUBLE_HALLUCINATION:
		    pline (E_J("Looks like you are back in Kansas.",
				"カンザスに戻ってきたみたいだ。"));
		    (void) make_hallucinated(0L,FALSE,0L);
		    break;
#ifdef STEED
	    case TROUBLE_SADDLE:
		    otmp = which_armor(u.usteed, W_SADDLE);
		    uncurse(otmp);
		    if (!Blind) {
#ifndef JP
			pline("%s %s %s.",
			      s_suffix(upstart(y_monnam(u.usteed))),
			      aobjnam(otmp, "softly glow"),
			      hcolor(NH_AMBER));
#else
			pline("%sにつけられた%sは%s柔らかく輝いた。",
			      y_monnam(u.usteed), xname(otmp),
			      j_no_ni(hcolor(NH_AMBER)));
#endif /*JP*/
			otmp->bknown = TRUE;
		    }
		    break;
#endif
	}
}

/* "I am sometimes shocked by...  the nuns who never take a bath without
 * wearing a bathrobe all the time.  When asked why, since no man can see them,
 * they reply 'Oh, but you forget the good God'.  Apparently they conceive of
 * the Deity as a Peeping Tom, whose omnipotence enables Him to see through
 * bathroom walls, but who is foiled by bathrobes." --Bertrand Russell, 1943
 * Divine wrath, dungeon walls, and armor follow the same principle.
 */
STATIC_OVL void
god_zaps_you(resp_god)
aligntyp resp_god;
{
	if (u.uswallow) {
	    pline(E_J("Suddenly a bolt of lightning comes down at you from the heavens!",
		      "突然、天から一筋の稲妻があなためがけて放たれた！"));
	    pline(E_J("It strikes %s!","雷は%sに落ちた！"), mon_nam(u.ustuck));
	    if (!resists_elec(u.ustuck)) {
		pline(E_J("%s fries to a crisp!",
			  "%sは黒こげとなった！"), Monnam(u.ustuck));
		/* Yup, you get experience.  It takes guts to successfully
		 * pull off this trick on your god, anyway.
		 */
		xkilled(u.ustuck, 0);
	    } else
		pline(E_J("%s seems unaffected.",
			  "%sは影響を受けないようだ。"), Monnam(u.ustuck));
	} else {
	    pline(E_J("Suddenly, a bolt of lightning strikes you!",
		      "突如、一筋の稲妻があなたを撃ちぬいた！"));
	    if (Reflecting) {
		shieldeff(u.ux, u.uy);
		if (Blind)
		    pline(E_J("For some reason you're unaffected.",
			      "なぜか、あなたは影響を受けなかった。"));
		else
		    (void) ureflects(E_J("%s reflects from your %s.",
					 "%sはあなたの%sで反射された。"),
				     E_J("It","稲妻"));
		damage_resistant_obj(REFLECTING, d(5,10));
	    } else if (is_full_resist(SHOCK_RES)) {
		shieldeff(u.ux, u.uy);
		pline(E_J("It seems not to affect you.",
			  "あなたは影響を受けないようだ。"));
	    } else
		fry_by_god(resp_god);
	}

	pline(E_J("%s is not deterred...",
		  "%sは諦めていない…。"), align_gname(resp_god));
	if (u.uswallow) {
	    pline(E_J("A wide-angle disintegration beam aimed at you hits %s!",
		      "あなたを狙った広角度の分解光線が%sに命中した！"),
			mon_nam(u.ustuck));
	    if (!resists_disint(u.ustuck)) {
		pline(E_J("%s fries to a crisp!",
			  "%sは粉砕し尽くされた！"), Monnam(u.ustuck));
		xkilled(u.ustuck, 2); /* no corpse */
	    } else
		pline(E_J("%s seems unaffected.",
			  "%sは影響を受けないようだ。"), Monnam(u.ustuck));
	} else {
	    pline(E_J("A wide-angle disintegration beam hits you!",
		      "広角度の分解光線があなたに命中した！"));

	    /* disintegrate shield and body armor before disintegrating
	     * the impudent mortal, like black dragon breath -3.
	     */
	    if (!is_full_resist(DISINT_RES)) {
		if (uarms && !(EReflecting & W_ARMS) &&
			    !(EDisint_resistance & W_ARMS))
		    (void) destroy_arm(uarms);
		if (uarmc && !(EReflecting & W_ARMC) &&
			    !(EDisint_resistance & W_ARMC))
		    (void) destroy_arm(uarmc);
		if (uarm && !(EReflecting & W_ARM) &&
			    !(EDisint_resistance & W_ARM) && !uarmc)
		    (void) destroy_arm(uarm);
		if (uarmu && !uarm && !uarmc) (void) destroy_arm(uarmu);
	    }
	    if (!Disint_resistance)
		fry_by_god(resp_god);
	    else {
		You(E_J("bask in its %s glow for a minute...",
			"%s光芒を浴びながらしばし佇んだ…。"), NH_BLACK);
		godvoice(resp_god, E_J("I believe it not!","信じられぬ！"));
	    }
	    if (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)) {
		/* one more try for high altars */
		verbalize(E_J("Thou cannot escape my wrath, mortal!",
			      "\"我が怒りからは逃れられぬぞ、定命の者よ！"));
		summon_minion(resp_god, FALSE);
		summon_minion(resp_god, FALSE);
		summon_minion(resp_god, FALSE);
#ifndef JP
		verbalize("Destroy %s, my servants!", uhim());
#else
		verbalize("\"我が下僕ども、奴を滅せよ！");
#endif /*JP*/
	    }
	}
}

STATIC_OVL void
fry_by_god(resp_god)
aligntyp resp_god;
{
	char killerbuf[64];

	You(E_J("fry to a crisp.","消し炭と化した。"));
	killer_format = KILLED_BY;
	Sprintf(killerbuf, E_J("the wrath of %s","%sの怒りに触れ"), align_gname(resp_god));
	killer = killerbuf;
	done(DIED);
}

STATIC_OVL void
angrygods(resp_god)
aligntyp resp_god;
{
	register int	maxanger;

	if(Inhell) resp_god = A_NONE;
	u.ublessed = 0;

	/* changed from tmp = u.ugangr + abs (u.uluck) -- rph */
	/* added test for alignment diff -dlc */
	if(resp_god != u.ualign.type)
	    maxanger =  u.ualign.record/2 + (Luck > 0 ? -Luck/3 : -Luck);
	else
	    maxanger =  3*u.ugangr +
		((Luck > 0 || u.ualign.record >= STRIDENT) ? -Luck/3 : -Luck);
	if (maxanger < 1) maxanger = 1; /* possible if bad align & good luck */
	else if (maxanger > 15) maxanger = 15;	/* be reasonable */

	switch (rn2(maxanger)) {
	    case 0:
	    case 1:	You_feel(E_J("that %s is %s.","%s%sような気がした。"),
			    align_gname(resp_god),
			    Hallucination ? E_J("bummed","がヘコんでる") :
					    E_J("displeased","の不興を買っている"));
			break;
	    case 2:
	    case 3:
			godvoice(resp_god,(char *)0);
#ifndef JP
			pline("\"Thou %s, %s.\"",
			    (ugod_is_angry() && resp_god == u.ualign.type)
				? "hast strayed from the path" :
						"art arrogant",
			      youmonst.data->mlet == S_HUMAN ? "mortal" : "creature");
			verbalize("Thou must relearn thy lessons!");
#else
			pline("“汝%s、%sよ。”",
			    (ugod_is_angry() && resp_god == u.ualign.type)
				? "は道を外れている" : "傲慢なり",
			      youmonst.data->mlet == S_HUMAN ? "定命の者" : "生き物");
			verbalize("\"汝、我が教えを学び直さねばならぬ！");
#endif /*JP*/
			(void) adjattrib(A_WIS, -1, FALSE);
			losexp((char *)0);
			break;
	    case 6:	if (!Punished) {
			    gods_angry(resp_god);
			    punish((struct obj *)0);
			    break;
			} /* else fall thru */
	    case 4:
	    case 5:	gods_angry(resp_god);
			if (!Blind && !Antimagic)
#ifndef JP
			    pline("%s glow surrounds you.",
				  An(hcolor(NH_BLACK)));
#else
			    pline("%s輝きがあなたを包んだ。",
				  hcolor(NH_BLACK));
#endif /*JP*/
			rndcurse();
			break;
	    case 7:
	    case 8:	godvoice(resp_god,(char *)0);
			verbalize(E_J("Thou durst %s me?","\"我を%sるか？"),
				  (on_altar() &&
				   (a_align(u.ux,u.uy) != resp_god)) ?
				  E_J("scorn","愚弄す") :
				  E_J("call upon","呼びつけ"));
#ifndef JP
			pline("\"Then die, %s!\"",
			      youmonst.data->mlet == S_HUMAN ? "mortal" : "creature");
#else
			pline("“ならば死ね、%s！”",
			      youmonst.data->mlet == S_HUMAN ? "人間" : "生物");
#endif /*JP*/
			summon_minion(resp_god, FALSE);
			break;

	    default:	gods_angry(resp_god);
			god_zaps_you(resp_god);
			break;
	}
	u.ublesscnt = rnz(300);
	return;
}

/* helper to print "str appears at your feet", or appropriate */
static void
at_your_feet(str)
	const char *str;
{
	if (Blind) str = Something;
	if (u.uswallow) {
	    /* barrier between you and the floor */
#ifndef JP
	    pline("%s %s into %s %s.", str, vtense(str, "drop"),
		  s_suffix(mon_nam(u.ustuck)), mbodypart(u.ustuck, STOMACH));
#else
	    pline("%sが%sの%sの中に転がってきた。", str,
		  mon_nam(u.ustuck), mbodypart(u.ustuck, STOMACH));
#endif /*JP*/
	} else {
#ifndef JP
	    pline("%s %s %s your %s!", str,
		  Blind ? "lands" : vtense(str, "appear"),
		  Levitation ? "beneath" : "at",
		  makeplural(body_part(FOOT)));
#else
	    pline("%sがあなたの%sに%s！", str,
		  Levitation ? "下方" : "足元",
		  Blind ? "落とされた" : "現れた");
#endif /*JP*/
	}
}

#ifdef ELBERETH
STATIC_OVL void
gcrownu()
{
    struct obj *obj;
    boolean already_exists, in_hand;
    short class_gift;
    int sp_no;
#define ok_wep(o) ((o) && ((o)->oclass == WEAPON_CLASS || is_weptool(o)))

    HSee_invisible |= FROMOUTSIDE;
    HFire_resistance |= FROMOUTSIDE;
    HCold_resistance |= FROMOUTSIDE;
    HShock_resistance |= FROMOUTSIDE;
    HSleep_resistance |= FROMOUTSIDE;
    HPoison_resistance |= FROMOUTSIDE;
    godvoice(u.ualign.type, (char *)0);

    obj = ok_wep(uwep) ? uwep : 0;
    already_exists = in_hand = FALSE;	/* lint suppression */
    switch (u.ualign.type) {
    case A_LAWFUL:
	u.uevent.uhand_of_elbereth = 1;
	verbalize(E_J("I crown thee...  The Hand of Elbereth!",
		      "\"汝に栄冠を授けよう… エルベレスの御手よ！"));
	break;
    case A_NEUTRAL:
	u.uevent.uhand_of_elbereth = 2;
	in_hand = (uwep && uwep->oartifact == ART_VORPAL_BLADE);
	already_exists = exist_artifact(ART_VORPAL_BLADE);
	verbalize(E_J("Thou shalt be my Envoy of Balance!",
		      "\"汝、我が調和の使者なり！"));
	break;
    case A_CHAOTIC:
	u.uevent.uhand_of_elbereth = 3;
	in_hand = (uwep && uwep->oartifact == ART_STORMBRINGER);
	already_exists = exist_artifact(ART_STORMBRINGER);
#ifndef JP
	verbalize("Thou art chosen to %s for My Glory!",
		  already_exists && !in_hand ? "take lives" : "steal souls");
#else
	verbalize("\"汝を我が栄光のため%s者として選ばん！",
		  already_exists && !in_hand ? "命を狩る" : "魂を盗む");
#endif /*JP*/
	break;
    }

    class_gift = STRANGE_OBJECT;
    /* 3.3.[01] had this in the A_NEUTRAL case below,
       preventing chaotic wizards from receiving a spellbook */
    if (Role_if(PM_WIZARD) &&
	    (!uwep || (uwep->oartifact != ART_VORPAL_BLADE &&
		       uwep->oartifact != ART_STORMBRINGER)) &&
	    !carrying(SPE_FINGER_OF_DEATH)) {
	class_gift = SPE_FINGER_OF_DEATH;
 make_splbk:
	obj = mksobj(class_gift, TRUE, FALSE);
	bless(obj);
	obj->bknown = TRUE;
	at_your_feet(E_J("A spellbook","魔法書"));
	dropy(obj);
	u.ugifts++;
	/* when getting a new book for known spell, enhance
	   currently wielded weapon rather than the book */
	for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
	    if (spl_book[sp_no].sp_id == class_gift) {
		if (ok_wep(uwep)) obj = uwep;	/* to be blessed,&c */
		break;
	    }
    } else if (Role_if(PM_MONK) &&
	    (!uwep || !uwep->oartifact)) {
	int g1, g2, o1, o2;
	/* get artifact gauntlets */
	if (!rn2(2)) {
	    g1 = ART_FIST_OF_FURY;		o1 = GAUNTLETS;
	    g2 = ART_GAUNTLETS_OF_DEFENSE;	o2 = GAUNTLETS_OF_DEXTERITY;
	} else {
	    g1 = ART_GAUNTLETS_OF_DEFENSE;	o1 = GAUNTLETS_OF_DEXTERITY;
	    g2 = ART_FIST_OF_FURY;		o2 = GAUNTLETS;
	}
	while (o1 && exist_artifact(g1)) {
	    g1 = g2;
	    o1 = o2;
	    o2 = 0;
	}
	if (o1) {
	    obj = mksobj(o1, FALSE, FALSE);
	    create_artifact(obj, g1);
	    at_your_feet(E_J("A pair of gauntlets","篭手"));
	    obj->spe = 1;
	    dropy(obj);
	    u.ugifts++;
	    class_gift = o1;
	} else if (!carrying(SPE_RESTORE_ABILITY)) {
	    /* monks rarely wield a weapon */
	    class_gift = SPE_RESTORE_ABILITY;
	    goto make_splbk;
	}
    }

    switch (u.ualign.type) {
    case A_LAWFUL:
	if (class_gift != STRANGE_OBJECT) {
	    ;		/* already got bonus above */
	} else if (obj && obj->otyp == LONG_SWORD && !obj->oartifact) {
	    int oldmat;
	    if (!Blind)
		Your(E_J("sword shines brightly for a moment.",
			 "剣は一瞬まぶしく輝いた。"));
	    oldmat = obj->madeof;
	    create_artifact(obj, ART_EXCALIBUR);
	    if (oldmat) change_material(obj, oldmat);
	    if (obj && obj->oartifact == ART_EXCALIBUR) u.ugifts++;
	}
	/* acquire Excalibur's skill regardless of weapon or gift */
	unrestrict_weapon_skill(P_LONG_BLADE_GROUP);
	if (obj && obj->oartifact == ART_EXCALIBUR)
	    discover_artifact(ART_EXCALIBUR);
	break;
    case A_NEUTRAL:
	if (class_gift != STRANGE_OBJECT) {
	    ;		/* already got bonus above */
	} else if (in_hand) {
	    Your(E_J("%s goes snicker-snack!",
		     "%sが風を切って走った！"), xname(obj));
	    obj->dknown = TRUE;
	} else if (!already_exists) {
	    obj = mksobj(LONG_SWORD, FALSE, FALSE);
	    create_artifact(obj, ART_VORPAL_BLADE);
	    obj->spe = 1;
	    at_your_feet(E_J("A sword","剣"));
	    dropy(obj);
	    u.ugifts++;
	}
	/* acquire Vorpal Blade's skill regardless of weapon or gift */
	unrestrict_weapon_skill(P_LONG_BLADE_GROUP);
	if (obj && obj->oartifact == ART_VORPAL_BLADE)
	    discover_artifact(ART_VORPAL_BLADE);
	break;
    case A_CHAOTIC:
      {
	char swordbuf[BUFSZ];

	Sprintf(swordbuf, E_J("%s sword","%s剣"), hcolor(NH_BLACK));
	if (class_gift != STRANGE_OBJECT) {
	    ;		/* already got bonus above */
	} else if (in_hand) {
	    Your(E_J("%s hums ominously!",
		     "%sは不吉な音律のうなりを上げた！"), swordbuf);
	    obj->dknown = TRUE;
	} else if (!already_exists) {
	    obj = mksobj(RUNESWORD, FALSE, FALSE);
	    create_artifact(obj, ART_STORMBRINGER);
	    at_your_feet(E_J(An(swordbuf),swordbuf));
	    obj->spe = 1;
	    dropy(obj);
	    u.ugifts++;
	}
	/* acquire Stormbringer's skill regardless of weapon or gift */
	unrestrict_weapon_skill(P_LONG_BLADE_GROUP);
	if (obj && obj->oartifact == ART_STORMBRINGER)
	    discover_artifact(ART_STORMBRINGER);
	break;
      }
    default:
	obj = 0;	/* lint */
	break;
    }

    /* enhance weapon regardless of alignment or artifact status */
    if (ok_wep(obj)) {
	if (obj->oartifact != ART_STORMBRINGER)
	    bless(obj);
	obj->oeroded = obj->oeroded2 = 0;
	obj->oerodeproof = TRUE;
	obj->bknown = obj->rknown = TRUE;
	if (obj->spe < 1) obj->spe = 1;
	/* acquire skill in this weapon */
	unrestrict_weapon_skill(weapon_type(obj));
    } else if (class_gift == STRANGE_OBJECT) {
	/* opportunity knocked, but there was nobody home... */
	You_feel(E_J("unworthy.","分不相応な気分になった。"));
    }
    update_inventory();
    return;
}
#endif	/*ELBERETH*/

STATIC_OVL void
pleased(g_align)
	aligntyp g_align;
{
	/* don't use p_trouble, worst trouble may get fixed while praying */
	int trouble = in_trouble();	/* what's your worst difficulty? */
	int pat_on_head = 0, kick_on_butt;

#ifndef JP
	You_feel("that %s is %s.", align_gname(g_align),
	    u.ualign.record >= DEVOUT ?
	    Hallucination ? "pleased as punch" : "well-pleased" :
	    u.ualign.record >= STRIDENT ?
	    Hallucination ? "ticklish" : "pleased" :
	    Hallucination ? "full" : "satisfied");
#else
	You_feel("%sが%s感じた。", align_gname(g_align),
	    u.ualign.record >= DEVOUT ?
	    Hallucination ? "めちゃめちゃ大喜びしていると" : "とても喜んでいると" :
	    u.ualign.record >= STRIDENT ?
	    Hallucination ? "くすぐったがっていると" : "喜んでいると" :
	    Hallucination ? "満腹なのを" : "満足していると");
#endif /*JP*/

	/* not your deity */
	if (on_altar() && p_aligntyp != u.ualign.type) {
		adjalign(-1);
		return;
	} else if (u.ualign.record < 2 && trouble <= 0) adjalign(1);

	/* depending on your luck & align level, the god you prayed to will:
	   - fix your worst problem if it's major.
	   - fix all your major problems.
	   - fix your worst problem if it's minor.
	   - fix all of your problems.
	   - do you a gratuitous favor.

	   if you make it to the the last category, you roll randomly again
	   to see what they do for you.

	   If your luck is at least 0, then you are guaranteed rescued
	   from your worst major problem. */

	if (!trouble && u.ualign.record >= DEVOUT) {
	    /* if hero was in trouble, but got better, no special favor */
	    if (p_trouble == 0) pat_on_head = 1;
	} else {
	    int action = rn1(Luck + (on_altar() ? 3 + on_shrine() : 2), 1);

	    if (!on_altar()) action = min(action, 3);
	    if (u.ualign.record < STRIDENT)
		action = (u.ualign.record > 0 || !rnl(2)) ? 1 : 0;

	    switch(min(action,5)) {
	    case 5: pat_on_head = 1;
	    case 4: do fix_worst_trouble(trouble);
		    while ((trouble = in_trouble()) != 0);
		    break;

	    case 3: fix_worst_trouble(trouble);
	    case 2: while ((trouble = in_trouble()) > 0)
		    fix_worst_trouble(trouble);
		    break;

	    case 1: if (trouble > 0) fix_worst_trouble(trouble);
	    case 0: break; /* your god blows you off, too bad */
	    }
	}

    /* note: can't get pat_on_head unless all troubles have just been
       fixed or there were no troubles to begin with; hallucination
       won't be in effect so special handling for it is superfluous */
    if(pat_on_head)
	switch(rn2((Luck + 6)>>1)) {
	case 0:	break;
	case 1:
	    if (uwep && (welded(uwep) || uwep->oclass == WEAPON_CLASS ||
			 is_weptool(uwep))) {
		char repair_buf[BUFSZ];

		*repair_buf = '\0';
		if (uwep->oeroded || uwep->oeroded2)
		    Sprintf(repair_buf, " and %s now as good as new",
			    otense(uwep, "are"));

		if (uwep->oartifact == ART_STORMBRINGER) {
		    /* Stormbringer won't be blessed */
		    int hap = 0;
		    if (rnd(5) > uwep->spe) {
			uwep->spe++;
			hap = 1;
		    }
		    if (uwep->blessed) {
			uwep->blessed = 0;
			hap = 1;
		    }
		    if (hap) Your(E_J("%s blade looks more evil!",
				      "%s刃は禍々しさを増した！"), hcolor(NH_BLACK));
		} else if (uwep->cursed) {
		    uncurse(uwep);
		    uwep->bknown = TRUE;
#ifndef JP
		    if (!Blind)
			Your("%s %s%s.", aobjnam(uwep, "softly glow"),
			     hcolor(NH_AMBER), repair_buf);
		    else You_feel("the power of %s over your %s.",
			u_gname(), xname(uwep));
#else
		    if (!Blind)
			Your("%sは%s柔らかく輝%sた。", cxname(uwep),
			    j_no_ni(hcolor(NH_AMBER)),
			    repair_buf[0] ? "き、新品同様になっ" : "い");
		    else You_feel("%sの周りに%sの力を感じた。",
			    xname(uwep), u_gname());
#endif /*JP*/
		    *repair_buf = '\0';
		} else if (!uwep->blessed) {
		    bless(uwep);
		    uwep->bknown = TRUE;
#ifndef JP
		    if (!Blind)
			Your("%s with %s aura%s.",
			     aobjnam(uwep, "softly glow"),
			     an(hcolor(NH_LIGHT_BLUE)), repair_buf);
		    else You_feel("the blessing of %s over your %s.",
			u_gname(), xname(uwep));
#else
		    if (!Blind)
			Your("%sは柔らかな%s光芒に包まれ%sた。",
			     cxname(uwep),
			     hcolor(NH_LIGHT_BLUE),
			     repair_buf[0] ? "、新品同様になっ" : "");
		    else You_feel("%sの祝福が%sを包むのを感じた。",
			u_gname(), xname(uwep));
#endif /*JP*/
		    *repair_buf = '\0';
		}

		/* fix any rust/burn/rot damage, but don't protect
		   against future damage */
		if (uwep->oeroded || uwep->oeroded2) {
		    uwep->oeroded = uwep->oeroded2 = 0;
		    /* only give this message if we didn't just bless
		       or uncurse (which has already given a message) */
		    if (*repair_buf)
#ifndef JP
			Your("%s as good as new!",
			     aobjnam(uwep, Blind ? "feel" : "look"));
#else
			Your("%sに真新しさが戻ったように%sる！",
			     cxname(uwep), Blind ? "感じ" : "見え");
#endif /*JP*/
		}
		update_inventory();
	    }
	    break;
	case 3:
	    /* takes 2 hints to get the music to enter the stronghold */
	    if (!u.uevent.uopened_dbridge) {
		if (u.uevent.uheard_tune < 1) {
		    godvoice(g_align,(char *)0);
#ifndef JP
		    verbalize("Hark, %s!",
			  youmonst.data->mlet == S_HUMAN ? "mortal" : "creature");
		    verbalize(
			"To enter the castle, thou must play the right tune!");
#else
		    verbalize("\"聞け、%s！",
			  youmonst.data->mlet == S_HUMAN ? "定命の者よ" : "生き物");
		    verbalize(
			"\"城へと入るには、汝は正しき調べを奏でねばならぬ！");
#endif /*JP*/
		    u.uevent.uheard_tune++;
		    break;
		} else if (u.uevent.uheard_tune < 2) {
#ifndef JP
		    You_hear("a divine music...");
		    pline("It sounds like:  \"%s\".", tune);
#else
		    You_hear("神聖なる調べを");
		    pline("それはこのように聞こえた:“%s”", tune);
#endif /*JP*/
		    u.uevent.uheard_tune++;
		    break;
		}
	    }
	    /* Otherwise, falls into next case */
	case 2:
	    if (!Blind)
#ifndef JP
		You("are surrounded by %s glow.", an(hcolor(NH_GOLDEN)));
#else
		You("%s輝きに包まれた。", hcolor(NH_GOLDEN));
#endif /*JP*/
	    /* if any levels have been lost (and not yet regained),
	       treat this effect like blessed full healing */
	    if (u.ulevel < u.ulevelmax) {
		u.ulevelmax -= 1;	/* see potion.c */
		pluslvl(FALSE);
	    } else {
		addhpmax(5);
		if (Upolyd) u.mhmax += 5;
	    }
	    u.uhp = u.uhpmax;
	    if (Upolyd) u.mh = u.mhmax;
	    ABASE(A_STR) = AMAX(A_STR);
	    if (u.uhunger < 900) init_uhunger();
	    if (u.uluck < 0) u.uluck = 0;
	    make_blinded(0L,TRUE);
	    flags.botl = 1;
	    break;
	case 4: {
	    register struct obj *otmp;
	    int any = 0;

#ifndef JP
	    if (Blind)
		You_feel("the power of %s.", u_gname());
	    else You("are surrounded by %s aura.",
		     an(hcolor(NH_LIGHT_BLUE)));
#else
	    if (Blind)
		You("%sの力を感じた。", u_gname());
	    else You("%s光芒に包まれた。", hcolor(NH_LIGHT_BLUE));
#endif /*JP*/
	    for(otmp=invent; otmp; otmp=otmp->nobj) {
		if (otmp->cursed) {
		    uncurse(otmp);
		    if (!Blind) {
#ifndef JP
			Your("%s %s.", aobjnam(otmp, "softly glow"),
			     hcolor(NH_AMBER));
#else
			Your("%sが%s柔らかく輝いた。",
			     cxname(otmp), j_no_ni(hcolor(NH_AMBER)));
#endif /*JP*/
			otmp->bknown = TRUE;
			++any;
		    }
		}
	    }
	    if (any) update_inventory();
	    break;
	}
	case 5: {
	    const char *msg=
		E_J("\"and thus I grant thee the gift of %s!\"",
		    "「よって汝に%sの恩恵をさずけよう！」");
	    char *gift = 0;
	    if (!(HTelepat & INTRINSIC))  {
		HTelepat |= FROMOUTSIDE;
		gift = E_J("Telepathy","テレパシー");
		if (Blind) see_monsters();
	    } else if (!(HFast & INTRINSIC))  {
		HFast |= FROMOUTSIDE;
		gift = E_J("Speed","俊足");
	    } else if (!(HStealth & INTRINSIC))  {
		HStealth |= FROMOUTSIDE;
		gift = E_J("Stealth","隠密");
	    } else if (!u.ublessed || !rn2(u.ublessed+1)) {
		if (!(HProtection & INTRINSIC))
		    HProtection |= FROMOUTSIDE;
		u.ublessed++;
		gift = E_J("my protection","我が守り");
	    }
	    if (gift) {
		godvoice(u.ualign.type, E_J("Thou hast pleased me with thy progress,",
					    "汝は満足のゆく成長を示した。"));
		pline(msg, gift);
		verbalize(E_J("Use it wisely in my name!",
			      "\"我が名の下に正しく使うがよい！"));
	    }
	    break;
	}
	case 7:
	case 8:
	case 9:		/* KMH -- can occur during full moons */
#ifdef ELBERETH
	    if (u.ualign.record >= PIOUS && !u.uevent.uhand_of_elbereth) {
		gcrownu();
		break;
	    } /* else FALLTHRU */
#endif	/*ELBERETH*/
	case 6:	{
	    struct obj *otmp;
	    int sp_no, trycnt = u.ulevel + 1;

	    at_your_feet(E_J("An object","ひとつの品物"));
	    /* not yet known spells given preference over already known ones */
	    /* Also, try to grant a spell for which there is a skill slot */
	    otmp = mkobj(SPBOOK_CLASS, TRUE);
	    while (--trycnt > 0) {
		if (otmp->otyp != SPE_BLANK_PAPER) {
		    for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
			if (spl_book[sp_no].sp_id == otmp->otyp) break;
		    if (sp_no == MAXSPELL &&
			!P_RESTRICTED(spell_skilltype(otmp->otyp)))
			break;	/* usable, but not yet known */
		} else {
		    if (!objects[SPE_BLANK_PAPER].oc_name_known ||
			    carrying(MAGIC_MARKER)) break;
		}
		set_otyp(otmp, rnd_class(bases[SPBOOK_CLASS], SPE_BLANK_PAPER));
	    }
	    bless(otmp);
	    place_object(otmp, u.ux, u.uy);
	    break;
	}
	default:	impossible("Confused deity!");
	    break;
	}

	u.ublesscnt = rnz(350);
	kick_on_butt = u.uevent.udemigod ? 1 : 0;
#ifdef ELBERETH
	if (u.uevent.uhand_of_elbereth) kick_on_butt++;
#endif
	if (kick_on_butt) u.ublesscnt += kick_on_butt * rnz(1000);

	return;
}

/* either blesses or curses water on the altar,
 * returns true if it found any water here.
 */
STATIC_OVL boolean
water_prayer(bless_water)
    boolean bless_water;
{
    register struct obj* otmp;
    register long changed = 0;
    boolean other = FALSE, bc_known = !(Blind || Hallucination);

    for(otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
	/* turn water into (un)holy water */
	if (otmp->otyp == POT_WATER &&
		(bless_water ? !otmp->blessed : !otmp->cursed)) {
	    otmp->blessed = bless_water;
	    otmp->cursed = !bless_water;
	    otmp->bknown = bc_known;
	    changed += otmp->quan;
	} else if(otmp->oclass == POTION_CLASS)
	    other = TRUE;
    }
    if(!Blind && changed) {
#ifndef JP
	pline("%s potion%s on the altar glow%s %s for a moment.",
	      ((other && changed > 1L) ? "Some of the" :
					(other ? "One of the" : "The")),
	      ((other || changed > 1L) ? "s" : ""), (changed > 1L ? "" : "s"),
	      (bless_water ? hcolor(NH_LIGHT_BLUE) : hcolor(NH_BLACK)));
#else
	pline("祭壇に置かれた薬壜%s、ひととき%s輝いた。",
	      ((other && changed > 1L) ? "のうちの幾つかが" :
					(other ? "が一本" : "が")),
	      j_no_ni(bless_water ? hcolor(NH_LIGHT_BLUE) : hcolor(NH_BLACK)));
#endif /*JP*/
    }
    return((boolean)(changed > 0L));
}

STATIC_OVL void
godvoice(g_align, words)
    aligntyp g_align;
    const char *words;
{
#ifndef JP
    const char *quot = "";
    if(words)
	quot = "\"";
    else
	words = "";

    pline_The("voice of %s %s: %s%s%s", align_gname(g_align),
	  godvoices[rn2(SIZE(godvoices))], quot, words, quot);
#else
    const char *oq, *cq;
    if(words) {
	oq = "“";
	cq = "”";
    } else {
	oq = "";
	cq = "";
	words = "";
    }
    pline("%sの声が%s: %s%s%s", align_gname(g_align),
	  godvoices[rn2(SIZE(godvoices))], oq, words, cq);
#endif /*JP*/
}

STATIC_OVL void
gods_angry(g_align)
    aligntyp g_align;
{
    godvoice(g_align, E_J("Thou hast angered me.","汝は我を怒らせた。"));
}

/* The g_align god is upset with you. */
STATIC_OVL void
gods_upset(g_align)
	aligntyp g_align;
{
	if(g_align == u.ualign.type) u.ugangr++;
	else if(u.ugangr) u.ugangr--;
	angrygods(g_align);
}

static NEARDATA const char sacrifice_types[] = { FOOD_CLASS, AMULET_CLASS, 0 };

STATIC_OVL void
consume_offering(otmp)
register struct obj *otmp;
{
    if (Hallucination)
	switch (rn2(3)) {
	    case 0:
		Your(E_J("sacrifice sprouts wings and a propeller and roars away!",
			"生贄から翼とプロペラがにょっきりと生え、轟音とともに飛んでいった！"));
		break;
	    case 1:
		Your(E_J("sacrifice puffs up, swelling bigger and bigger, and pops!",
			"生贄は風船のようにどんどん膨れ上がると、弾けてしまった！"));
		break;
	    case 2:
		Your(E_J("sacrifice collapses into a cloud of dancing particles and fades away!",
			"生贄は踊り狂う小片となって崩れ落ち、消え去った！"));
		break;
	}
    else if (Blind && u.ualign.type == A_LAWFUL)
	Your(E_J("sacrifice disappears!","生贄は消えた！"));
    else Your(E_J("sacrifice is consumed in a %s!",
		  "生贄は%sた！"),
	      u.ualign.type == A_LAWFUL ? E_J("flash of light","まばゆい光の中に消え去っ") :
					E_J("burst of flame","燃えさかる炎に焼き尽くされ"));
    if (carried(otmp)) useup(otmp);
    else useupf(otmp, 1L);
    exercise(A_WIS, TRUE);
}

int
getobj_filter_sacrifice(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	if (otyp == CORPSE ||
	    otyp == AMULET_OF_YENDOR ||
	    otyp == FAKE_AMULET_OF_YENDOR)
		return GETOBJ_CHOOSEIT;
	return 0;
}

int
dosacrifice()
{
    register struct obj *otmp;
    int value = 0;
    int pm;
    aligntyp altaralign = a_align(u.ux,u.uy);
#ifdef JP
    static const struct getobj_words sacw = { 0, 0, "捧げる", "捧げ" };
#endif /*JP*/

    if (!on_altar() || u.uswallow) {
	You(E_J("are not standing on an altar.",
		"祭壇の上に立っていない。"));
	return 0;
    }

    if (In_endgame(&u.uz)) {
	if (!(otmp = getobj(sacrifice_types, E_J("sacrifice",&sacw), getobj_filter_sacrifice))) return 0;
    } else {
	if (!(otmp = floorfood(E_J("sacrifice",&sacw), 1))) return 0;
    }
    /*
      Was based on nutritional value and aging behavior (< 50 moves).
      Sacrificing a food ration got you max luck instantly, making the
      gods as easy to please as an angry dog!

      Now only accepts corpses, based on the game's evaluation of their
      toughness.  Human and pet sacrifice, as well as sacrificing unicorns
      of your alignment, is strongly discouraged.
     */

#define MAXVALUE 24 /* Highest corpse value (besides Wiz) */

    if (otmp->otyp == CORPSE) {
	register struct permonst *ptr = &mons[otmp->corpsenm];
	struct monst *mtmp;
	extern const int monstr[];

	/* KMH, conduct */
	u.uconduct.gnostic++;

	/* you're handling this corpse, even if it was killed upon the altar */
	feel_cockatrice(otmp, TRUE);

	if (otmp->corpsenm == PM_ACID_BLOB
		|| (monstermoves <= peek_at_iced_corpse_age(otmp) + 50)) {
	    value = monstr[otmp->corpsenm] + 1;
	    if (otmp->oeaten)
		value = eaten_stat(value, otmp);
	}

	if (your_race(ptr)) {
	    if (is_demon(youmonst.data)) {
		You(E_J("find the idea very satisfying.",
			"この思いつきに大変満足した。"));
		exercise(A_WIS, TRUE);
	    } else if (u.ualign.type != A_CHAOTIC) {
		    pline(E_J("You'll regret this infamous offense!",
			      "お前はこの悪逆な行いを後悔することとなろう！"));
		    exercise(A_WIS, FALSE);
	    }

	    if (altaralign != A_CHAOTIC && altaralign != A_NONE) {
		/* curse the lawful/neutral altar */
		pline_The(E_J("altar is stained with %s blood.",
			      "祭壇は%s血で穢された。"), urace.adj);
		if(!Is_astralevel(&u.uz))
		    levl[u.ux][u.uy].altarmask = AM_CHAOTIC;
		angry_priest();
	    } else {
		struct monst *dmon;
		const char *demonless_msg;

		/* Human sacrifice on a chaotic or unaligned altar */
		/* is equivalent to demon summoning */
		if (altaralign == A_CHAOTIC && u.ualign.type != A_CHAOTIC) {
		    pline(
#ifndef JP
		     "The blood floods the altar, which vanishes in %s cloud!",
			  an(hcolor(NH_BLACK)));
#else
		     "血が祭壇にあふれ、%s煙となって立ちのぼりはじめた！",
			  hcolor(NH_BLACK));
#endif /*JP*/
		    levl[u.ux][u.uy].typ = ROOM;
		    levl[u.ux][u.uy].altarmask = 0;
		    newsym(u.ux, u.uy);
		    angry_priest();
		    demonless_msg = E_J("cloud dissipates","煙はかき消えた");
		} else {
		    /* either you're chaotic or altar is Moloch's or both */
		    pline_The(E_J("blood covers the altar!",
				"血が祭壇をおおった！"));
		    change_luck(altaralign == A_NONE ? -2 : 2);
		    demonless_msg = E_J("blood coagulates","血は乾いて固まった");
		}
		if ((pm = dlord(altaralign)) != NON_PM &&
		    (dmon = makemon(&mons[pm], u.ux, u.uy, NO_MM_FLAGS))) {
		    You(E_J("have summoned %s!",
			    "%sを召喚した！"), a_monnam(dmon));
		    if (sgn(u.ualign.type) == sgn(dmon->data->maligntyp))
			dmon->mpeaceful = TRUE;
		    You(E_J("are terrified, and unable to move.",
			    "恐怖のあまり動けなくなった。"));
		    nomul(-3);
		} else pline_The(E_J("%s.","%s。"), demonless_msg);
	    }

	    if (u.ualign.type != A_CHAOTIC) {
		adjalign(-5);
		u.ugangr += 3;
		(void) adjattrib(A_WIS, -1, TRUE);
		if (!Inhell) angrygods(u.ualign.type);
		change_luck(-5);
	    } else adjalign(5);
	    if (carried(otmp)) useup(otmp);
	    else useupf(otmp, 1L);
	    return(1);
	} else if (((mtmp = get_mtraits(otmp)) != (struct monst *)0)
		    && mtmp->mtame) {
	    /* mtmp is a temporary pointer to a tame monster's attributes,
	     * not a real monster */
	    pline(E_J("So this is how you repay loyalty?",
		      "これが忠義に報いるやり方か？"));
	    adjalign(-3);
	    value = -1;
	    HAggravate_monster |= FROMOUTSIDE;
	} else if (is_undead(ptr)) { /* Not demons--no demon corpses */
	    if (u.ualign.type != A_CHAOTIC)
		value += 1;
	} else if (is_unicorn(ptr)) {
	    int unicalign = sgn(ptr->maligntyp);

	    /* If same as altar, always a very bad action. */
	    if (unicalign == altaralign) {
		pline(E_J("Such an action is an insult to %s!",
			  "このような行為は、%sに対する挑発だ！"),
		      (unicalign == A_CHAOTIC)
		      ? E_J("chaos","混沌") : unicalign ? E_J("law","法") : E_J("balance","調和"));
		(void) adjattrib(A_WIS, -1, TRUE);
		value = -5;
	    } else if (u.ualign.type == altaralign) {
		/* If different from altar, and altar is same as yours, */
		/* it's a very good action */
		if (u.ualign.record < ALIGNLIM)
		    You_feel(E_J("appropriately %s.",
				"正しく%sの道にいると感じた。"), align_str(u.ualign.type));
		else You_feel(E_J("you are thoroughly on the right path.",
				"自分が完全に正しい道を歩んでいると感じた。"));
		adjalign(5);
		value += 3;
	    } else
		/* If sacrificing unicorn of your alignment to altar not of */
		/* your alignment, your god gets angry and it's a conversion */
		if (unicalign == u.ualign.type) {
		    u.ualign.record = -1;
		    value = 1;
		} else value += 3;
	}
    } /* corpse */

    if (otmp->otyp == AMULET_OF_YENDOR) {
	if (!Is_astralevel(&u.uz)) {
	    if (Hallucination)
		    You_feel(E_J("homesick.","ホームシックになった。"));
	    else
		    You_feel(E_J("an urge to return to the surface.",
				"一刻も早く地上へ戻らねばならないと感じた。"));
	    return 1;
	} else {
	    /* The final Test.	Did you win? */
	    if(uamul == otmp) Amulet_off();
	    u.uevent.ascended = 1;
	    if(carried(otmp)) useup(otmp); /* well, it's gone now */
	    else useupf(otmp, 1L);
	    if ((Upolyd && youmonst.data->mnum == PM_CHROMATIC_DRAGON) ||
		(uarm && (uarm->otyp == CHROMATIC_DRAGON_SCALES ||
		          uarm->otyp == CHROMATIC_DRAGON_SCALE_MAIL)))
		chromatic_dragon_ending();
	    You(E_J("offer the Amulet of Yendor to %s...",
		    "イェンダーの魔除けを%sに捧げた…。"), a_gname());
	    if (u.ualign.type != altaralign) {
		/* And the opposing team picks you up and
		   carries you off on their shoulders */
		adjalign(-99);
#ifndef JP
		pline("%s accepts your gift, and gains dominion over %s...",
		      a_gname(), u_gname());
		pline("%s is enraged...", u_gname());
		pline("Fortunately, %s permits you to live...", a_gname());
		pline("A cloud of %s smoke surrounds you...",
		      hcolor((const char *)"orange"));
#else
		pline("%sはあなたの捧げ物を受け取り、%sの権能を手に入れた…。",
		      a_gname(), u_gname());
		pline("%sは憤激した…。", u_gname());
		pline("幸い、%sはあなたが生き延びることを許した…。", a_gname());
		pline("%s煙があなたを包んだ…。",
		      hcolor((const char *)"オレンジ色の"));
#endif /*JP*/
		done(ESCAPED);
	    } else { /* super big win */
		adjalign(10);
pline(E_J("An invisible choir sings, and you are bathed in radiance...",
	"どこからか聖なる歌声がきこえ、あなたは光につつまれた…。"));
		godvoice(altaralign, E_J("Congratulations, mortal!",
					"よくやった、定命の者よ！"));
		display_nhwindow(WIN_MESSAGE, FALSE);
verbalize(E_J("In return for thy service, I grant thee the gift of Immortality!",
	"\"汝の献身に報い、汝に不死の恩恵を授けよう！"));
		You(E_J("ascend to the status of Demigod%s...",
			"昇天し、%s神となった…。"),
		    flags.female ? E_J("dess","女") : "");
		done(ASCENDED);
	    }
	}
    } /* real Amulet */

    if (otmp->otyp == FAKE_AMULET_OF_YENDOR) {
	    if (flags.soundok)
		You_hear(E_J("a nearby thunderclap.",
			     "近くに雷鳴を"));
	    if (!otmp->known) {
#ifndef JP
		You("realize you have made a %s.",
		    Hallucination ? "boo-boo" : "mistake");
#else
		You("自分が%sことに気づいた。",
		    Hallucination ? "ヘマをした" : "過ちを犯した");
#endif /*JP*/
		otmp->known = TRUE;
		change_luck(-1);
		return 1;
	    } else {
		/* don't you dare try to fool the gods */
		change_luck(-3);
		adjalign(-1);
		u.ugangr += 3;
		value = -3;
	    }
    } /* fake Amulet */

    if (value == 0) {
	pline(nothing_happens);
	return (1);
    }

    if (altaralign != u.ualign.type &&
	(Is_astralevel(&u.uz) || Is_sanctum(&u.uz))) {
	/*
	 * REAL BAD NEWS!!! High altars cannot be converted.  Even an attempt
	 * gets the god who owns it truely pissed off.
	 */
#ifndef JP
	You_feel("the air around you grow charged...");
	pline("Suddenly, you realize that %s has noticed you...", a_gname());
	godvoice(altaralign, "So, mortal!  You dare desecrate my High Temple!");
#else
	You("周囲の空気が張りつめてゆくのを感じた…。");
	pline("突然、あなたは%sに見られていたことに気づいた…。", a_gname());
	godvoice(altaralign, "定命の者よ！ 愚かにも我が上の聖廟を穢すか！");
#endif /*JP*/
	/* Throw everything we have at the player */
	god_zaps_you(altaralign);
    } else if (value < 0) { /* I don't think the gods are gonna like this... */
	gods_upset(altaralign);
    } else {
	int saved_anger = u.ugangr;
	int saved_cnt = u.ublesscnt;
	int saved_luck = u.uluck;

	/* Sacrificing at an altar of a different alignment */
	if (u.ualign.type != altaralign) {
	    /* Is this a conversion ? */
	    /* An unaligned altar in Gehennom will always elicit rejection. */
	    if (ugod_is_angry() || (altaralign == A_NONE && Inhell)) {
		if(u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL] &&
		   altaralign != A_NONE) {
		    You(E_J("have a strong feeling that %s is angry...",
			    "%sの怒りを確信した…。"), u_gname());
		    consume_offering(otmp);
		    pline(E_J("%s accepts your allegiance.",
			      "%sはあなたの入信を受け入れた。"), a_gname());

		    /* The player wears a helm of opposite alignment? */
		    if (uarmh && uarmh->otyp == HELM_OF_OPPOSITE_ALIGNMENT)
			u.ualignbase[A_CURRENT] = altaralign;
		    else
			u.ualign.type = u.ualignbase[A_CURRENT] = altaralign;
		    u.ublessed = 0;
		    flags.botl = 1;

		    You(E_J("have a sudden sense of a new direction.",
			    "突如、新たな生きる道に目覚めた。"));
		    /* Beware, Conversion is costly */
		    change_luck(-3);
		    u.ublesscnt += 300;
		    adjalign((int)(u.ualignbase[A_ORIGINAL] * (ALIGNLIM / 2)));
		} else {
		    u.ugangr += 3;
		    adjalign(-5);
		    pline(E_J("%s rejects your sacrifice!",
			    "%sはあなたの供物を拒絶した！"), a_gname());
		    godvoice(altaralign, E_J("Suffer, infidel!","苦しめ、異端者！"));
		    change_luck(-5);
		    (void) adjattrib(A_WIS, -2, TRUE);
		    if (!Inhell) angrygods(u.ualign.type);
		}
		return(1);
	    } else {
		consume_offering(otmp);
		You(E_J("sense a conflict between %s and %s.",
			"%sと%sが争う気配を感じた。"),
		    u_gname(), a_gname());
		if (rn2(8 + u.ulevel) > 5) {
		    struct monst *pri;
#ifndef JP
		    You("feel the power of %s increase.", u_gname());
		    You("feel %s is very angry at you!", a_gname());
#else
		    You("%sの力が増すのを感じた。", u_gname());
		    You("%sの激しい怒りを感じ取った！", a_gname());
#endif /*JP*/
		    summon_minion(altaralign, FALSE);
		    if (u.ulevel > rn2(10)) summon_minion(altaralign, FALSE);
		    if (u.ulevel > rn2(20)) summon_minion(altaralign, FALSE);
		    exercise(A_WIS, TRUE);
		    change_luck(1);
		    /* Yes, this is supposed to be &=, not |= */
		    levl[u.ux][u.uy].altarmask &= AM_SHRINE;
		    /* the following accommodates stupid compilers */
		    levl[u.ux][u.uy].altarmask =
			levl[u.ux][u.uy].altarmask | (Align2amask(u.ualign.type));
		    if (!Blind)
#ifndef JP
			pline("The altar glows %s.",
			      hcolor(
			      u.ualign.type == A_LAWFUL ? NH_WHITE :
			      u.ualign.type ? NH_BLACK : (const char *)"gray"));
#else
			pline("祭壇は%s輝いた。",
			      j_no_ni(hcolor(
			      u.ualign.type == A_LAWFUL ? NH_WHITE :
			      u.ualign.type ? NH_BLACK : (const char *)"灰色の")));
#endif /*JP*/

		    if(rnl((int)u.ulevel) > 6 && u.ualign.record > 0 &&
		       rnd(u.ualign.record) > (3*ALIGNLIM)/4)
			summon_minion(altaralign, TRUE);
		    /* anger priest; test handles bones files */
		    if((pri = findpriest(temple_occupied(u.urooms))) &&
		       !p_coaligned(pri))
			angry_priest();
		} else {
		    pline(E_J("Unluckily, you feel the power of %s decrease.",
			      "不幸にも、あなたは%sの力が減じるのを感じた。"),
			  u_gname());
		    change_luck(-1);
		    exercise(A_WIS, FALSE);
		    if (rnl(u.ulevel) > 6 && u.ualign.record > 0 &&
		       rnd(u.ualign.record) > (7*ALIGNLIM)/8)
			summon_minion(altaralign, TRUE);
		}
		return(1);
	    }
	}

	consume_offering(otmp);
	/* OK, you get brownie points. */
	if(u.ugangr) {
	    u.ugangr -=
		((value * (u.ualign.type == A_CHAOTIC ? 2 : 3)) / MAXVALUE);
	    if(u.ugangr < 0) u.ugangr = 0;
	    if(u.ugangr != saved_anger) {
		if (u.ugangr) {
#ifndef JP
		    pline("%s seems %s.", u_gname(),
			  Hallucination ? "groovy" : "slightly mollified");
#else
		    pline("%sは%sようだ。", u_gname(),
			  Hallucination ? "ノリノリの" : "やや怒りを和らげた");
#endif /*JP*/

		    if ((int)u.uluck < 0) change_luck(1);
		} else {
#ifndef JP
		    pline("%s seems %s.", u_gname(), Hallucination ?
			  "cosmic (not a new fact)" : "mollified");
#else
		    pline("%sは%sようだ。", u_gname(), Hallucination ?
			  "全宇宙に連なっている(新発見ではないが)" : "怒りを和らげた");
#endif /*JP*/

		    if ((int)u.uluck < 0) u.uluck = 0;
		}
	    } else { /* not satisfied yet */
		if (Hallucination)
		    pline_The(E_J("gods seem tall.","神様はデカいなあ。"));
		else You(E_J("have a feeling of inadequacy.",
			     "供物が不足であったと感じた。"));
	    }
	} else if(ugod_is_angry()) {
	    if(value > MAXVALUE) value = MAXVALUE;
	    if(value > -u.ualign.record) value = -u.ualign.record;
	    adjalign(value);
	    You_feel(E_J("partially absolved.",
			"少しだけ赦されたのを感じた。"));
	} else if (u.ublesscnt > 0) {
	    u.ublesscnt -=
		((value * (u.ualign.type == A_CHAOTIC ? 500 : 300)) / MAXVALUE);
	    if(u.ublesscnt < 0) u.ublesscnt = 0;
	    if(u.ublesscnt != saved_cnt) {
		if (u.ublesscnt) {
		    if (Hallucination)
			You(E_J("realize that the gods are not like you and I.",
				"神様はあなたや私とは違うと気づいた。"));
		    else
			You(E_J("have a hopeful feeling.",
				"希望が見えてきたような気がした。"));
		    if ((int)u.uluck < 0) change_luck(1);
		} else {
		    if (Hallucination)
			pline(E_J("Overall, there is a smell of fried onions.",
				"そこらじゅうに、揚げた玉ねぎの匂いがした。"));
		    else
			You(E_J("have a feeling of reconciliation.",
				"受け入れられたような気がした。"));
		    if ((int)u.uluck < 0) u.uluck = 0;
		}
	    }
	} else {
	    int nartifacts = nartifact_exist();

#ifdef WIZARD
    if (wizard) pline("[Prob:1/%d, Value:%d]", 10 + (2 * u.ugifts * u.ugifts), value);
#endif
	    /* you were already in pretty good standing */
	    /* The player can gain an artifact */
	    /* The chance goes down as the number of artifacts goes up */
	    if (u.ulevel > 2 && u.uluck >= 0 &&
		!rn2(10 + (2 * u.ugifts * u.ugifts))) {
	      if (value > (d(2,5)/2)) {
		otmp = mk_artifact((struct obj *)0, a_align(u.ux,u.uy));
		if (otmp) {
		    if (otmp->spe < 0) otmp->spe = 0;
		    if (otmp->cursed) uncurse(otmp);
		    otmp->oerodeproof = TRUE;
		    dropy(otmp);
		    at_your_feet(E_J("An object","ひとつの品物"));
		    godvoice(u.ualign.type, E_J("Use my gift wisely!",
						"我が恩恵、正しく使うがよい！"));
		    u.ugifts++;
		    u.ublesscnt = rnz(300 + (50 * nartifacts));
		    exercise(A_WIS, TRUE);
		    /* make sure we can use this weapon */
		    if (ok_wep(otmp))
			unrestrict_weapon_skill(weapon_type(otmp));
		    discover_artifact(otmp->oartifact);
		    return(1);
		}
	      } else if (uwep && uwep->spe < WEP_ENCHANT_MAX &&
			 (objects[uwep->otyp].oc_class == WEAPON_CLASS ||
			  is_weptool(uwep)) &&
			 !rn2(2 << max(uwep->spe, 0))) {
		chwepon(uwep, 1);
		return(1);
	      }
	    }
	    change_luck((value * LUCKMAX) / (MAXVALUE * 2));
	    if ((int)u.uluck < 0) u.uluck = 0;
	    if (u.uluck != saved_luck) {
		if (Blind)
		    You(E_J("think %s brushed your %s.",
			    "%sが%sをくすぐったような気がした。"),something, body_part(FOOT));
		else You(Hallucination ?
#ifndef JP
		    "see crabgrass at your %s.  A funny thing in a dungeon." :
		    "glimpse a four-leaf clover at your %s.",
		    makeplural(body_part(FOOT)));
#else
		    "足元にペンペン草を見た。迷宮の中にしてはおかしな物だ。" :
		    "足元に四葉のクローバーをかいま見た。");
#endif /*JP*/
	    }
	}
    }
    return(1);
}


/* determine prayer results in advance; also used for enlightenment */
boolean
can_pray(praying)
boolean praying;	/* false means no messages should be given */
{
    int alignment;

    p_aligntyp = on_altar() ? a_align(u.ux,u.uy) : u.ualign.type;
    p_trouble = in_trouble();

    if (is_demon(youmonst.data) && (p_aligntyp != A_CHAOTIC)) {
	if (praying)
#ifndef JP
	    pline_The("very idea of praying to a %s god is repugnant to you.",
		  p_aligntyp ? "lawful" : "neutral");
#else
	    pline("%s神に祈るなど、あなたにとっては唾棄すべき考えだ。",
		  p_aligntyp ? "秩序の" : "中立の");
#endif /*JP*/
	return FALSE;
    }

    if (praying)
	You(E_J("begin praying to %s.",
		"%sに祈り始めた。"), align_gname(p_aligntyp));

    if (u.ualign.type && u.ualign.type == -p_aligntyp)
	alignment = -u.ualign.record;		/* Opposite alignment altar */
    else if (u.ualign.type != p_aligntyp)
	alignment = u.ualign.record / 2;	/* Different alignment altar */
    else alignment = u.ualign.record;

    if ((p_trouble > 0) ? (u.ublesscnt > 200) : /* big trouble */
	(p_trouble < 0) ? (u.ublesscnt > 100) : /* minor difficulties */
	(u.ublesscnt > 0))			/* not in trouble */
	p_type = 0;		/* too soon... */
    else if ((int)Luck < 0 || u.ugangr || alignment < 0)
	p_type = 1;		/* too naughty... */
    else /* alignment >= 0 */ {
	if(on_altar() && u.ualign.type != p_aligntyp)
	    p_type = 2;
	else
	    p_type = 3;
    }

    if (is_undead(youmonst.data) && !Inhell &&
	(p_aligntyp == A_LAWFUL || (p_aligntyp == A_NEUTRAL && !rn2(10))))
	p_type = -1;
    /* Note:  when !praying, the random factor for neutrals makes the
       return value a non-deterministic approximation for enlightenment.
       This case should be uncommon enough to live with... */

    return !praying ? (boolean)(p_type == 3 && !Inhell) : TRUE;
}

int
dopray()
{
    /* Confirm accidental slips of Alt-P */
    if (flags.prayconfirm)
	if (yn(E_J("Are you sure you want to pray?",
		   "本当に祈りますか？")) == 'n')
	    return 0;

    u.uconduct.gnostic++;
    /* Praying implies that the hero is conscious and since we have
       no deafness attribute this implies that all verbalized messages
       can be heard.  So, in case the player has used the 'O' command
       to toggle this accessible flag off, force it to be on. */
    flags.soundok = 1;

    /* set up p_type and p_alignment */
    if (!can_pray(TRUE)) return 0;

#ifdef WIZARD
    if (wizard && p_type >= 0) {
	if (yn("Force the gods to be pleased?") == 'y') {
	    u.ublesscnt = 0;
	    if (u.uluck < 0) u.uluck = 0;
	    if (u.ualign.record <= 0) u.ualign.record = 1;
	    u.ugangr = 0;
	    if(p_type < 2) p_type = 3;
	}
    }
#endif
    nomul(-3);
    nomovemsg = E_J("You finish your prayer.","あなたは祈りを終えた。");
    afternmv = prayer_done;

    if(p_type == 3 && !Inhell) {
	/* if you've been true to your god you can't die while you pray */
	if (!Blind)
	    You(E_J("are surrounded by a shimmering light.",
		    "ゆらめく光芒に包まれた。"));
	u.uinvulnerable = TRUE;
    }

    return(1);
}

STATIC_PTR int
prayer_done()		/* M. Stephenson (1.0.3b) */
{
    aligntyp alignment = p_aligntyp;

    u.uinvulnerable = FALSE;
    if(p_type == -1) {
	godvoice(alignment,
		 alignment == A_LAWFUL ?
		E_J("Vile creature, thou durst call upon me?",
		    "下劣な存在め、汝が我を呼びつけしか？") :
		E_J("Walk no more, perversion of nature!",
		    "倒れよ、摂理に反する存在め！"));
	You_feel(E_J("like you are falling apart.",
		     "まるで身体がばらばらになるかのように感じた。"));
	/* KMH -- Gods have mastery over unchanging */
	rehumanize();
	losehp(rnd(20), E_J("residual undead turning effect",
			    "死人祓いの効果の余波で"), KILLED_BY_AN);
	exercise(A_CON, FALSE);
	return(1);
    }
    if (Inhell) {
	pline(E_J("Since you are in Gehennom, %s won't help you.",
		"ゲヘナの中では、%sの力はあなたに届かない。"),
	      align_gname(alignment));
	/* haltingly aligned is least likely to anger */
	if (u.ualign.record <= 0 || rnl(u.ualign.record))
	    angrygods(u.ualign.type);
	return(0);
    }

    if (p_type == 0) {
	if(on_altar() && u.ualign.type != alignment)
	    (void) water_prayer(FALSE);
	u.ublesscnt += rnz(250);
	change_luck(-3);
	gods_upset(u.ualign.type);
    } else if(p_type == 1) {
	if(on_altar() && u.ualign.type != alignment)
	    (void) water_prayer(FALSE);
	angrygods(u.ualign.type);	/* naughty */
    } else if(p_type == 2) {
	if(water_prayer(FALSE)) {
	    /* attempted water prayer on a non-coaligned altar */
	    u.ublesscnt += rnz(250);
	    change_luck(-3);
	    gods_upset(u.ualign.type);
	} else pleased(alignment);
    } else {
	/* coaligned */
	if(on_altar())
	    (void) water_prayer(TRUE);
	pleased(alignment); /* nice */
    }
    return(1);
}

int
doturn()
{	/* Knights & Priest(esse)s only please */

	struct monst *mtmp, *mtmp2;
	int once, range, xlev;

	if (!Role_if(PM_PRIEST) && !Role_if(PM_KNIGHT)) {
		/* Try to use turn undead spell. */
		if (objects[SPE_TURN_UNDEAD].oc_name_known) {
		    register int sp_no;
		    for (sp_no = 0; sp_no < MAXSPELL &&
			 spl_book[sp_no].sp_id != NO_SPELL &&
			 spl_book[sp_no].sp_id != SPE_TURN_UNDEAD; sp_no++);

		    if (sp_no < MAXSPELL &&
			spl_book[sp_no].sp_id == SPE_TURN_UNDEAD)
			    return spelleffects(sp_no, TRUE);
		}

		You(E_J("don't know how to turn undead!",
			"どうすれば死人を祓えるのか分からない！"));
		return(0);
	}
	u.uconduct.gnostic++;

	if ((u.ualign.type != A_CHAOTIC &&
		    (is_demon(youmonst.data) || is_undead(youmonst.data))) ||
				u.ugangr > 6 /* "Die, mortal!" */) {

		pline(E_J("For some reason, %s seems to ignore you.",
			"何らかの理由で、%sはあなたを無視しているようだ。"), u_gname());
		aggravate();
		exercise(A_WIS, FALSE);
		return(0);
	}

	if (Inhell) {
	    pline(E_J("Since you are in Gehennom, %s won't help you.",
		      "ゲヘナの中では、%sの助力はあなたに届かない。"), u_gname());
	    aggravate();
	    return(0);
	}
	pline(E_J("Calling upon %s, you chant an arcane formula.",
		  "%sへの祈りを込め、あなたは神秘の術式を唱えた。"), u_gname());
	exercise(A_WIS, TRUE);

	/* note: does not perform unturn_dead() on victims' inventories */
	range = BOLT_LIM + (u.ulevel / 5);	/* 5 to 11 */
	range *= range;
	once = 0;
	for(mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;

	    if (DEADMONSTER(mtmp)) continue;
	    if (!cansee(mtmp->mx,mtmp->my) ||
		distu(mtmp->mx,mtmp->my) > range) continue;

	    if (!mtmp->mpeaceful && (is_undead(mtmp->data) ||
		   (is_demon(mtmp->data) && (u.ulevel > (MAXULEV/2))))) {

		    mtmp->msleeping = 0;
		    if (Confusion) {
			if (!once++)
			    pline(E_J("Unfortunately, your voice falters.",
			      "残念ながら、あなたは詠唱をとちってしまった。"));
			mtmp->mflee = 0;
			mtmp->mfrozen = 0;
			mtmp->mcanmove = 1;
		    } else if (!resist(mtmp, '\0', 0, TELL)) {
			xlev = 6;
			switch (mtmp->data->mlet) {
			    /* this is intentional, lichs are tougher
			       than zombies. */
			case S_LICH:    xlev += 2;  /*FALLTHRU*/
			case S_GHOST:   xlev += 2;  /*FALLTHRU*/
			case S_VAMPIRE: xlev += 2;  /*FALLTHRU*/
			case S_WRAITH:  xlev += 2;  /*FALLTHRU*/
			case S_MUMMY:   xlev += 2;  /*FALLTHRU*/
			case S_ZOMBIE:
			    if (u.ulevel >= xlev &&
				    !resist(mtmp, '\0', 0, NOTELL)) {
				if (u.ualign.type == A_CHAOTIC) {
				    setmpeaceful(mtmp, TRUE);
				} else { /* damn them */
				    killed(mtmp);
				}
				break;
			    } /* else flee */
			    /*FALLTHRU*/
			default:
			    monflee(mtmp, 0, FALSE, TRUE);
			    break;
			}
		    }
	    }
	}
	nomul(-5);
	return(1);
}

const char *
a_gname()
{
    return(a_gname_at(u.ux, u.uy));
}

const char *
a_gname_at(x,y)     /* returns the name of an altar's deity */
xchar x, y;
{
    if(!IS_ALTAR(levl[x][y].typ)) return((char *)0);

    return align_gname(a_align(x,y));
}

const char *
u_gname()  /* returns the name of the player's deity */
{
    return align_gname(u.ualign.type);
}

const char *
align_gname(alignment)
aligntyp alignment;
{
    const char *gnam;

    switch (alignment) {
     case A_NONE:	gnam = Moloch; break;
     case A_LAWFUL:	gnam = urole.lgod; break;
     case A_NEUTRAL:	gnam = urole.ngod; break;
     case A_CHAOTIC:	gnam = urole.cgod; break;
     default:		impossible("unknown alignment.");
			gnam = "someone"; break;
    }
    if (*gnam == '_') ++gnam;
    return gnam;
}

/* hallucination handling for priest/minion names: select a random god
   if character is hallucinating */
const char *
halu_gname(alignment)
aligntyp alignment;
{
    const char *gnam;
    int which;

    if (!Hallucination) return align_gname(alignment);

    if (rn2(36/* rolenum x 3 */ + SIZE(bogusgods)) < SIZE(bogusgods))
	return bogusgods[rn2(SIZE(bogusgods))];

    which = randrole();
    switch (rn2(3)) {
     case 0:	gnam = roles[which].lgod; break;
     case 1:	gnam = roles[which].ngod; break;
     case 2:	gnam = roles[which].cgod; break;
     default:	gnam = 0; break;		/* lint suppression */
    }
    if (!gnam) gnam = Moloch;
    if (*gnam == '_') ++gnam;
    return gnam;
}

/* deity's title */
const char *
align_gtitle(alignment)
aligntyp alignment;
{
    const char *gnam, *result = E_J("god","神");

    switch (alignment) {
     case A_LAWFUL:	gnam = urole.lgod; break;
     case A_NEUTRAL:	gnam = urole.ngod; break;
     case A_CHAOTIC:	gnam = urole.cgod; break;
     default:		gnam = 0; break;
    }
    if (gnam && *gnam == '_') result = E_J("goddess","女神");
    return result;
}

void
altar_wrath(x, y)
register int x, y;
{
    aligntyp altaralign = a_align(x,y);

    if(!strcmp(align_gname(altaralign), u_gname())) {
	godvoice(altaralign, E_J("How darest thou desecrate my altar!",
		"汝、我が祭壇を汚すか！"));
	(void) adjattrib(A_WIS, -1, FALSE);
    } else {
	pline(E_J("A voice (could it be %s?) whispers:",
		  "ささやく声(おそらく%sの？)が聞こえた:"),
	      align_gname(altaralign));
	verbalize(E_J("Thou shalt pay, infidel!","報いを受けよ、異端者め！"));
	change_luck(-1);
    }
}

/* assumes isok() at one space away, but not necessarily at two */
STATIC_OVL boolean
blocked_boulder(dx,dy)
int dx,dy;
{
    register struct obj *otmp;
    long count = 0L;

    for(otmp = level.objects[u.ux+dx][u.uy+dy]; otmp; otmp = otmp->nexthere) {
	if(otmp->otyp == BOULDER)
	    count += otmp->quan;
    }

    switch(count) {
	case 0: return FALSE; /* no boulders--not blocked */
	case 1: break; /* possibly blocked depending on if it's pushable */
	default: return TRUE; /* >1 boulder--blocked after they push the top
	    one; don't force them to push it first to find out */
    }

    if (!isok(u.ux+2*dx, u.uy+2*dy))
	return TRUE;
    if (IS_ROCK(levl[u.ux+2*dx][u.uy+2*dy].typ))
	return TRUE;
    if (sobj_at(BOULDER, u.ux+2*dx, u.uy+2*dy))
	return TRUE;

    return FALSE;
}

STATIC_OVL void
chromatic_dragon_ending()
{
	char buf[BUFSZ];
	adjalign(-99);
#ifndef JP
	You("stopped offering the Amulet of Yendor to %s.", a_gname());
	pline("Why I have to give away the Amulet?");
	pline("This Amulet should belong to... ME!");
	if (!Upolyd) polyself(FALSE);
	You_feel("loud laughter of the Mother of all Dragons, and rage of %s.", u_gname());
	pline("But everything seems to fly away fast.");
	Your("conscious awareness fades out...",
	Sprintf(buf, "sacrificed %sself to Chromatic Dragon", uhim());
#else
	You("イェンダーの魔除けを%sに捧げようとして、不意に動きを止めた。", a_gname());
	pline("なぜ自分がこの素晴らしい魔除けを手放さねばならない？");
	pline("この魔除けは、……わらわにこそふさわしい！");
	if (!Upolyd) polyself(FALSE);
	You("竜の大母の哄笑と、%sの激しい怒りをないまぜに感じたが、全ては急速に遠ざかっていった。",
	    a_gname());
	Your("意識は遠のき、魂は消滅した…。");
	Sprintf(buf, "自らの肉体をクロマティック・ドラゴンに捧げた");
#endif /*JP*/
	killer = buf;
	killer_format = NO_KILLER_PREFIX;
	Lifesaved = 0L; /* not allowed */
	done(DIED);
}

/*pray.c*/
