/*	SCCS Id: @(#)do_wear.c	3.4	2003/11/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define MAX_LEV_POWER 300
#define Perm_Mask (W_ART|W_ARTI|W_ARMF)
#define Perm_Lev  ((ELevAtWill & Perm_Mask) && !BLevAtWill)

STATIC_DCL void FDECL(recharge_ring_levitation, (struct obj *));

void
floating_above(what)
const char *what;
{
    You(E_J("are floating high above the %s.",
	    "%sのはるか上に浮いている。"), what);
}

boolean
can_reach_floor()
{
	struct trap *ttmp = t_at(u.ux, u.uy);
	return (boolean)(!u.uswallow &&
#ifdef STEED
			/* Restricted/unskilled riders can't reach the floor */
			!(u.usteed && P_SKILL(P_RIDING) < P_BASIC) &&
#endif
			/* You are in a pit, not standing around it */
			!(ttmp && !u.utrap && ttmp->tseen &&
			  (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT)) &&
			/* You are not levitating (except for air, water levels) */
			 (!Levitation ||
			  Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)));
}

void
float_up()
{
	if(u.utrap) {
		if(u.utraptype == TT_PIT) {
			u.utrap = 0;
			You(E_J("float up, out of the pit!",
				"浮き上がり、落とし穴から抜け出た！"));
			vision_full_recalc = 1;	/* vision limits change */
			fill_pit(u.ux, u.uy);
		} else if (u.utraptype == TT_INFLOOR) {
#ifndef JP
			Your("body pulls upward, but your %s are still stuck.",
			     makeplural(body_part(LEG)));
#else
			Your("身体は上に引っぱられたが、まだ%sが挟まれている。",
			     body_part(LEG));
#endif /*JP*/
		} else if (u.utraptype == TT_SWAMP) {
			You(E_J("float up, out of the swamp.",
				"浮き上がり、沼から抜け出た。"));
			u.utrap = 0;
			u.utraptype = 0;
		} else {
			You(E_J("float up, only your %s is still stuck.",
				"浮き上がったが、片%sがまだ捕らえられている。"),
				body_part(LEG));
		}
	}
	else if(Is_waterlevel(&u.uz))
		pline(E_J("It feels as though you've lost some weight.",
			  "あなたは体重が少し軽くなったかのような感覚をおぼえた。"));
	else if(u.uinwater)
		spoteffects(TRUE);
	else if(u.uswallow)
		You(is_animal(u.ustuck->data) ?
			E_J("float away from the %s.","%sから浮き上がった。")  :
			E_J("spiral up into %s.","ぐるぐる回りながら%sの中心へと上がっていった。"),
		    is_animal(u.ustuck->data) ?
			surface(u.ux, u.uy) :
			mon_nam(u.ustuck));
	else if (Hallucination)
		pline(E_J("Up, up, and awaaaay!  You're walking on air!",
			  "上へ、うえへ、うええぇぇぇぇ！ あなたは空中を歩いている！"));
	else if(Is_airlevel(&u.uz))
		You(E_J("gain control over your movements.",
			"自由に動けるようになった。"));
	else
		You(E_J("start to float in the air!",
			"空中に浮き始めた！"));
#ifdef STEED
	if (u.usteed && !is_floater(u.usteed->data) &&
						!is_flyer(u.usteed->data)) {
/* Disallow levitating with steed */
//	    if (Lev_at_will)
//	    	pline(E_J("%s magically floats up!",
//			  "%sは魔法の力で宙に浮き上がった！"), Monnam(u.usteed));
//	    else {
	    	You(E_J("cannot stay on %s.",
			"%sの上に乗っていられない。"), mon_nam(u.usteed));
	    	dismount_steed(DISMOUNT_GENERIC);
//	    }
	}
#endif
	return;
}

int
float_down(hmask, emask)
long hmask, emask;     /* might cancel timeout */
{
	register struct trap *trap = (struct trap *)0;
	d_level current_dungeon_level;
	boolean no_msg = FALSE;

	HLevitation &= ~hmask;
	ELevitation &= ~emask;
	HLevAtWill  &= ~hmask;
	if(Levitation) return(0); /* maybe another ring/potion/boots */
	flags.botl = 1;
	if(u.uswallow) {
	    You(E_J("float down, but you are still %s.",
		    "下へと降りていったが、いまだ%s。"),
		is_animal(u.ustuck->data) ? E_J("swallowed","飲み込まれている") :
					    E_J("engulfed","巻き込まれている"));
	    return(1);
	}

	if (Punished && !carried(uball) &&
	    (is_pool(uball->ox, uball->oy) ||
	     ((trap = t_at(uball->ox, uball->oy)) &&
	      ((trap->ttyp == PIT) || (trap->ttyp == SPIKED_PIT) ||
	       (trap->ttyp == TRAPDOOR) || (trap->ttyp == HOLE))))) {
			u.ux0 = u.ux;
			u.uy0 = u.uy;
			u.ux = uball->ox;
			u.uy = uball->oy;
			movobj(uchain, uball->ox, uball->oy);
			newsym(u.ux0, u.uy0);
			vision_full_recalc = 1;	/* in case the hero moved. */
	}
	/* check for falling into pool - added by GAN 10/20/86 */
	if(!Flying) {
		if (!u.uswallow && u.ustuck) {
			if (sticks(youmonst.data))
				You(E_J("aren't able to maintain your hold on %s.",
					"%sを捕まえていられなくなった。"),
					mon_nam(u.ustuck));
			else
				pline(E_J("Startled, %s can no longer hold you!",
					  "%sは驚き、あなたを捕まえていられなくなった！"),
					mon_nam(u.ustuck));
			u.ustuck = 0;
		}
		/* kludge alert:
		 * drown() and lava_effects() print various messages almost
		 * every time they're called which conflict with the "fall
		 * into" message below.  Thus, we want to avoid printing
		 * confusing, duplicate or out-of-order messages.
		 * Use knowledge of the two routines as a hack -- this
		 * should really be handled differently -dlc
		 */
		if(is_pool(u.ux,u.uy) && !Wwalking && !Swimming && !u.uinwater)
			no_msg = drown() || Levitation; /* maybe levitated again in drown() */

		if(is_lava(u.ux,u.uy)) {
			(void) lava_effects();
			no_msg = TRUE;
		}
		if(is_swamp(u.ux,u.uy) && !Wwalking && !Swimming) {
			(void) swamp_effects();
			no_msg = TRUE;
		}
	}
	if (!trap) {
	    trap = t_at(u.ux,u.uy);
	    if(Is_airlevel(&u.uz))
		You(E_J("begin to tumble in place.","その場で回りはじめた。"));
	    else if (Is_waterlevel(&u.uz) && !no_msg)
		You_feel(E_J("heavier.","重さが増したようだ。"));
	    /* u.uinwater msgs already in spoteffects()/drown() */
	    else if (!u.uinwater && !no_msg) {
#ifdef STEED
		if (!(emask & W_SADDLE))
#endif
		{
		    boolean sokoban_trap = (In_sokoban(&u.uz) && trap);
		    if (Hallucination)
#ifndef JP
			pline("Bummer!  You've %s.",
			      is_pool(u.ux,u.uy) ?
			      "splashed down" : sokoban_trap ? "crashed" :
			      "hit the ground");
#else
			pline("サイテー！ あなたは%sた。",
			      is_pool(u.ux,u.uy) ?
			      "水柱を上げて落っこち" : sokoban_trap ? "衝突し" :
			      "地面に激突し");
#endif /*JP*/
		    else {
			if (sokoban_trap || BLevAtWill) {
			    if (sokoban_trap) {
				/* Justification elsewhere for Sokoban traps
				 * is based on air currents. This is
				 * consistent with that.
				 * The unexpected additional force of the
				 * air currents once leviation
				 * ceases knocks you off your feet.
				 */
				You(E_J("fall over.","倒れ伏した。"));
				losehp(rnd(2), E_J("dangerous winds","危険な気流で"), KILLED_BY);
			    } else {
				pline(E_J("Suddenly, you feel the gravitiy returns to normal!",
					  "突然、あなたを支えていた浮力がなくなった！"));
				You(E_J("fall to the %s.","%sに墜落した。"), surface(u.ux, u.uy));
				losehp(rnd(6), E_J("falling","高所から落下して"), KILLED_BY);
			    }
#ifdef STEED
			    if (u.usteed) dismount_steed(DISMOUNT_FELL);
#endif
			    selftouch(E_J("As you fall, you","落下中、あなたは"));
			} else {
			    if (is_pool(u.ux,u.uy) || is_swamp(u.ux,u.uy))
				You(E_J("gently land on water.","静かに着水した。"));
			    else
				You(E_J("float gently to the %s.","そっと%sに降り立った。"),
				    surface(u.ux, u.uy));
			}
		    }
		}
	    }
	}

	/* can't rely on u.uz0 for detecting trap door-induced level change;
	   it gets changed to reflect the new level before we can check it */
	assign_level(&current_dungeon_level, &u.uz);

	if(trap)
		switch(trap->ttyp) {
		case STATUE_TRAP:
			break;
		case HOLE:
		case TRAPDOOR:
			if(!Can_fall_thru(&u.uz) || u.ustuck)
				break;
			/* fall into next case */
		default:
			if (!u.utrap) /* not already in the trap */
				dotrap(trap, 0);
	}

	if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !u.uswallow &&
		/* falling through trap door calls goto_level,
		   and goto_level does its own pickup() call */
		on_level(&u.uz, &current_dungeon_level))
	    (void) pickup(1);
	return 1;
}

/* recharge lev-power from idle time */
STATIC_OVL void
recharge_ring_levitation(otmp)
struct obj *otmp;
{
	long t;
	if (otmp->otyp != RIN_LEVITATION || otmp->spe != 0) return;
	t = (monstermoves - otmp->age) / 4;
	if (otmp->odamaged >= t)
	    otmp->odamaged -= t;
	else
	    otmp->odamaged = 0;
	otmp->age = monstermoves;
}

int
start_ring_levitation(obj)
struct obj *obj;
{
	struct obj *otmp;
	long t;
	if (obj && obj->otyp == RIN_LEVITATION &&
	    obj->spe == 0 && /*obj->age < monstermoves &&*/
	    obj->odamaged < MAX_LEV_POWER) {
	    otmp = obj;
	} else if (uright && uright->otyp == RIN_LEVITATION &&
		   uright->spe == 0 &&/* uright->age < monstermoves &&*/
		   uright->odamaged < MAX_LEV_POWER) {
	    otmp = uright;
	} else if (uleft && uleft->otyp == RIN_LEVITATION &&
		   uleft->spe == 0 &&/* uleft->age < monstermoves &&*/
		   uleft->odamaged < MAX_LEV_POWER) {
	    otmp = uleft;
	} else return 0;
	recharge_ring_levitation(otmp);
	if (otmp->odamaged >= MAX_LEV_POWER) return 0;
	t = !otmp->cursed ? MAX_LEV_POWER - otmp->odamaged : rn2(140);
	if (t < 10) t += 10;
	otmp->age = monstermoves;
	otmp->spe = -1;
	incr_itimeout(&HLevitation, t);
	return 1;
}

int
stop_ring_levitation(obj)
struct obj *obj;
{
	struct obj *otmp;
	if (obj && obj->otyp == RIN_LEVITATION &&
	    obj->spe == -1) {
	    otmp = obj;
	} else if (uright && uright->otyp == RIN_LEVITATION &&
		   uright->spe == -1) {
	    otmp = uright;
	} else if (uleft && uleft->otyp == RIN_LEVITATION &&
		   uleft->spe == -1) {
	    otmp = uleft;
	} else return 0;
	otmp->odamaged += monstermoves - otmp->age;
	if (otmp->odamaged > MAX_LEV_POWER-10)
	    otmp->odamaged += rn1(25, 25);
	otmp->age = monstermoves;
	otmp->spe = 0;
	HLevitation &= ~TIMEOUT;
	return 1;
}

/* Is it possible to switch lev-power source
   if the current one exhausts? */
int
can_transit_levitation()
{
	if (uleft)  recharge_ring_levitation(uleft);
	if (uright) recharge_ring_levitation(uright);
	return (uleft && uleft->otyp == RIN_LEVITATION &&
		uleft->spe == 0 && /*uleft->age < monstermoves &&*/
		uleft->odamaged < (MAX_LEV_POWER-10)) ||
	       (uright && uright->otyp == RIN_LEVITATION &&
		uright->spe == 0 && /*uright->age < monstermoves &&*/
		uright->odamaged < (MAX_LEV_POWER-10));
}

boolean
can_levitate()
{
	return (Lev_at_will && (Perm_Lev || can_transit_levitation()));
}

/* returns 1 if successful levitation; 2 if failed due to exhausted rings;
   0 if nothing happened */
int
start_levitation()
{
	if (Levitation) {
	    You(E_J("are already levitating.","すでに浮遊している。"));
	    return 0;
	}
	if(Is_waterlevel(&u.uz)) {
	    pline(E_J("It feels as though you've lost some weight.",
		      "あなたは体重が少し軽くなったかのような感覚をおぼえた。"));
	    return 0;
	}
	flags.botl = 1;
	if (Perm_Lev) {
	    HLevitation |= I_SPECIAL;
	} else {
	    if (!can_transit_levitation()) {
		int cnt = 0;
		if (uleft  && uleft->otyp  == RIN_LEVITATION) cnt++;
		if (uright && uright->otyp == RIN_LEVITATION) cnt++;
		if (!cnt) return 0;
#ifndef JP
		Your("ring%s exhausted of levitation energy.", 
			(uleft  && uleft->otyp  == RIN_LEVITATION &&
			 uright && uright->otyp == RIN_LEVITATION) ?
			"s are both" : "is");
#else
		Your("指輪は%s浮遊の魔力を使い果たしたようだ。",
			(uleft  && uleft->otyp  == RIN_LEVITATION &&
			 uright && uright->otyp == RIN_LEVITATION) ?
			"どちらも" : "");
#endif /*JP*/
		return 2;
	    }
	    start_ring_levitation(0);
	}
	float_up();
	spoteffects(FALSE);	/* for sinks */
	return 1;
}

int
stop_levitation()
{
	/* all sources of levitation are shut off */
	stop_ring_levitation(0);
	(void) float_down(TIMEOUT|I_SPECIAL, 0L);
	return 1;
}

int
levitation_on(oldprop, atwill)
long oldprop;
boolean atwill;
{
	int mk = 0;
	if (!atwill) {
	    if (!Levitation) {
		if (ELevAtWill & Perm_Mask) {
		    incr_itimeout(&HLevitation, rn1(140,10));
		    float_up();
		    spoteffects(FALSE);	/* for sinks */
		    return 1;
		} else if (can_transit_levitation()) {
		    start_ring_levitation(0);
		    float_up();
		    spoteffects(FALSE);
		    return 1;
		} else {
		    /* no source of levitation power! */
		    return 0;
		}
	    }
	    if (oldprop || HLevAtWill) {
		You_feel(E_J("you can no longer control the levitation.",
			     "浮力を制御できなくなったような気がした。"));
		if (HLevitation & I_SPECIAL) {
		    /* if you are permanently levitating, cancel it */
		    HLevitation &= ~I_SPECIAL;
		    incr_itimeout(&HLevitation, rn1(140,10));
		}
		return 1;
	    }
	} else {
	    if (!oldprop && !HLevAtWill) {
		if (Levitation) {
		    You_feel(E_J("you can control the levitation now.",
				 "浮力を制御できるようになった気がした。"));
		    mk = 1;
		} else {
		    You_feel(E_J("gravity is slightly decreased.",
				 "浮力を感じた。"));
		    return 1;
		}
	    }
	    if (Levitation && Perm_Lev) {
		/* Permanent levitation is granted, stop counting */
		stop_ring_levitation(0);
		HLevitation &= ~TIMEOUT;
		HLevitation |= I_SPECIAL;
	    }
	}
	return mk;
}

int
levitation_off(obj)
struct obj *obj;
{
	int mk = 0;
	if (!ELevAtWill && !HLevAtWill && !Levitation) {
	    You_feel(E_J("the gravity returns to normal.",
			 "浮力を感じなくなった。"));
	    return 1;
	}
	if ((HLevitation & I_SPECIAL) && !Perm_Lev) {
	    /* If you are permanently levitating but lost
	       its source, switch to limited one (if possible) */
	    HLevitation &= ~I_SPECIAL;
	    if (!(HLevitation & TIMEOUT) &&
		can_transit_levitation())
		start_ring_levitation(0);
	} else if (HLevitation & TIMEOUT) {
	    /* You are levitating by a ring or a potion.
	       Stop it if by a ring */
	    stop_ring_levitation(obj);
	} else {
	    /* You are not levitating */
	    return 0;
	}
	/* levitation power is lost? */
	if (!Levitation) {
	    (void) float_down(TIMEOUT|I_SPECIAL, 0L);
	    mk = 1;
	}

	return mk;
}

int
invoke_levitation(obj)
struct obj *obj;
{
	struct obj *otmp;
	if (!Lev_at_will) {
	    You_cant(E_J("control levitation at will.",
			 "自分の意思で浮遊を制御することはできない。"));
	    return 0;
	}
	if (!obj->owornmask && !obj->oartifact) {
	    E_J(You("must put it on to invoke its power."),
		pline("身につけていなければ、浮遊の魔力を引き出すことはできない。"));
		return 0;
	}
	if (Perm_Lev) {
	    /* permanent levitation source: just toggle levitation  */
	    if (obj->otyp == LEVITATION_BOOTS || obj->oartifact) {
		if (Levitation) return stop_levitation();
		return start_levitation();
	    }
	    /* invoke ring when you have perm lev src */
	    if (obj->otyp == RIN_LEVITATION) {
		if (Levitation)
		    You(E_J("are not levitating by the ring's power.",
			    "浮遊の魔力を指輪から得ていない。"));
		else
		    You(E_J("don't need to consume the ring's power to levitate.",
			    "指輪の魔力を消費せずとも浮遊することができる。"));
		return 0;
	    }
	    impossible("invoke_levitation: strange lev-source?");
	    return 0;
	}
	if (obj->otyp == RIN_LEVITATION) {
	    if (Levitation) {
		if (obj->spe == -1) return stop_levitation();
		otmp = (obj == uleft ) ? uright :
		       (obj == uright) ? uleft : 0;
		if (!otmp || otmp->otyp != RIN_LEVITATION || !otmp->spe) {
		    /* levitating by potion or spell */
		    You(E_J("are not levitating by the ring's power.",
			    "浮遊の魔力を指輪から得ていない。"));
		    return 0;
		}
		if (can_transit_levitation()) {
		    stop_ring_levitation(otmp);
		    start_ring_levitation(obj);
		    You(E_J("adjust your balance in the air.",
			    "空中でバランスを取り直した。"));
		    return 1;
		}
	    } else {
		recharge_ring_levitation(obj);
		if (obj->odamaged < (MAX_LEV_POWER-10)) {
		    unsigned odam_sav = 0xFFFF;
		    otmp = (obj == uleft ) ? uright :
			   (obj == uright) ? uleft : 0;
		    if (otmp && otmp->otyp == RIN_LEVITATION) {
			/* temporalily disable the other ring */
			odam_sav = otmp->odamaged;
			otmp->odamaged = MAX_LEV_POWER;
		    }
		    start_levitation();
		    if (odam_sav != 0xFFFF) otmp->odamaged = odam_sav;
		    return 1;
		} else {
		    Your(E_J("ring is exhausted of levitation energy.",
			     "指輪は浮遊の魔力を使い果たしたようだ。"));
		    return 0;
		}
	    }
	}
	impossible("invoke_levitation: strange lev-source?");
	return 0;
}
