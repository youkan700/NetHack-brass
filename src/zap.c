/*	SCCS Id: @(#)zap.c	3.4	2003/08/24	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Disintegration rays have special treatment; corpses are never left.
 * But the routine which calculates the damage is separate from the routine
 * which kills the monster.  The damage routine returns this cookie to
 * indicate that the monster should be disintegrated.
 */
#define MAGIC_COOKIE 1000

static boolean obj_zapped;
static int poly_zapped;

extern boolean notonhead;	/* for long worms */

/* kludge to use mondied instead of killed */
extern boolean m_using;

STATIC_DCL void FDECL(costly_cancel, (struct obj *));
STATIC_DCL void FDECL(polyuse, (struct obj*, int, int));
STATIC_DCL void FDECL(create_polymon, (struct obj *, int));
STATIC_DCL boolean FDECL(zap_updown, (struct obj *));
STATIC_DCL void FDECL(revive_egg, (struct obj *));
#ifdef STEED
STATIC_DCL boolean FDECL(zap_steed, (struct obj *));
#endif

STATIC_DCL int FDECL(zap_hit, (int,int));
STATIC_DCL void FDECL(backfire, (struct obj *));
STATIC_DCL int FDECL(spell_hit_bonus, (int));
STATIC_DCL int FDECL(zap_to_glyph2, (int,int,int,int,int,int,int));
STATIC_DCL void FDECL(buzzcore, (struct zapinfo *, int *, boolean *, boolean));
STATIC_DCL int FDECL(zapinfo2otyp, (struct zapinfo *));


#define is_hero_spell(ztmp)	((ztmp)->byyou && (ztmp)->aatyp == AT_MAGC)

STATIC_VAR const char are_blinded_by_the_flash[] = E_J("are blinded by the flash!",
						       "閃光で目がくらんだ！");

const char * const flash_types[] = {	/* also used in buzzmu(mcastu.c) */
	E_J("magic missile",		"魔法の矢"),	/* Wands must be 0-9 */
	E_J("bolt of fire",		"炎の奔流"),
	E_J("bolt of cold",		"冷気の奔流"),
	E_J("sleep ray",		"眠りの光線"),
	E_J("bolt of lightning",	"稲妻"),
	E_J("bolt of poison gas",       "毒霧の奔流"),
	E_J("stream of acid",           "酸の激流"),
	E_J("disintegration ray",	"分解光線"),
	E_J("paralysis ray",            "麻痺の光線"),
	E_J("death ray",		"死の光線"),

	E_J("magic missile",		"魔法の矢"),	/* Spell equivalents must be 10-19 */
	E_J("fireball",			"ファイアボール"),
	E_J("cone of cold",		"渦巻く凍気"),
	E_J("sleep ray",		"眠りの光線"),
	E_J("bolt of lightning",	"稲妻"),	/* There is no spell, used for retribution */
	E_J("blast of poison",          "毒霧の奔流"),
	E_J("stream of acid",           "酸の激流"),
	E_J("disintegration ray",	"分解光線"),
	E_J("paralysis ray",            "麻痺の光線"),
	E_J("finger of death",		"死の指"),

	E_J("blast of missiles",	"魔力のブレス"),	/* Dragon breath equivalents 20-29*/
	E_J("blast of fire",		"炎のブレス"),
	E_J("blast of frost",		"凍気のブレス"),
	E_J("blast of sleep gas",	"催眠ガスのブレス"),
	E_J("blast of lightning",	"雷撃のブレス"),
	E_J("blast of poison gas",	"毒ガスのブレス"),
	E_J("blast of acid",		"酸のブレス"),
	E_J("blast of disintegration",	"分解のブレス"),
	E_J("blast of paralysis gas",	"麻痺のブレス"),
	E_J("blast of death",		"死のブレス"),

	E_J("magical blast",		"魔力の爆発"),		/* Explosion equivalents 30-39*/
	E_J("fireball",			"ファイアボール"),
	E_J("ball of cold",		"冷気の塊"),
	E_J("sleep field",              "眠りの魔力"),
	E_J("ball of lightning",	"雷撃の球体"),
	E_J("poison gas cloud",		"毒ガスの雲"),
	E_J("splash of acid",		"酸の飛沫"),
	E_J("disintegration field",	"分解空間"),
	E_J("paralysis field",          "麻痺の魔力"),
	E_J("death field",		"死の暴風")
};

const char * const flash_simple_name[] = {	/* only for wands and spells */
#ifndef JP
	/* Wands 0-9 */
	"missile", "bolt", "bolt", "ray", "lightning", "bolt", "stream", "ray", "ray", "ray",
	/* Spells 10-19 */
	"missile", "fireball", "blizzard", "ray", "lightning", "blast", "stream", "ray", "ray", "ray"
#else
	/* Wands 0-9 */
	"矢", "炎", "冷気", "光線", "稲妻", "毒", "酸", "光線", "光線", "光線",
	/* Spells 10-19 */
	"矢", "ファイアボール", "凍気", "光線", "稲妻", "毒霧", "酸", "分解光線", "光線", "死の指"
#endif /*JP*/
};

void
setup_zapinfo(ztmp, aatyp, adtyp, damn, damd, fltxt, fstxt, mon)
struct zapinfo *ztmp;
uchar aatyp, adtyp, damn, damd;
const char *fltxt, *fstxt;
struct monst *mon; /* zapper */
{
	ztmp->aatyp = aatyp;
	ztmp->adtyp = adtyp;
	ztmp->damn = damn;
	ztmp->damd = damd;
	ztmp->fltxt = fltxt;
	ztmp->fstxt = fstxt;
	ztmp->zapper = mon;
	ztmp->byyou = (mon == &youmonst);
	ztmp->stdkiller = (!fltxt && !fstxt);
	ztmp->beam_type = adtyp - 1;
	ztmp->oclass = '\0';
	if (aatyp == AT_BREA) {
	    /* breath */
	    if (!fstxt) ztmp->fstxt = E_J("blast","ブレス");
	    if (!fltxt) {
		if (adtyp >= AD_MAGM && adtyp <= AD_DETH)
		    ztmp->fltxt = flash_types[adtyp - 1 + 20];
		else
		    impossible("setup_zapinfo: unknown breath?");
	    }
	} else if (aatyp == AT_MAGC) {
	    /* spell */
	    if (!fstxt) {
		if (adtyp >= AD_MAGM && adtyp <= AD_DETH)
		    ztmp->fstxt = flash_simple_name[adtyp - 1 + 10];
		else
		    impossible("setup_zapinfo: unknown spell?");
	    }
	    if (!fltxt) {
		if (adtyp >= AD_MAGM && adtyp <= AD_DETH)
		    ztmp->fltxt = flash_types[adtyp - 1 + 10];
		else
		    impossible("setup_zapinfo: unknown spell?");
	    }
	} else if (aatyp == AT_EXPL) {
	    /* explosion */
	    if (!fstxt) {
		ztmp->fstxt = E_J("explosion","爆発");
	    }
	    if (!fltxt) {
		if (adtyp >= AD_MAGM && adtyp <= AD_DETH)
		    ztmp->fltxt = flash_types[adtyp - 1 + 30];
		else
		    impossible("setup_zapinfo: unknown spell?");
	    }
	} else {
	    /* wand */
	    if (!fstxt) {
		if (adtyp >= AD_MAGM && adtyp <= AD_DETH)
		    ztmp->fstxt = flash_simple_name[adtyp - 1];
		else
		    impossible("setup_zapinfo: unknown wand?");
	    }
	    if (!fltxt) {
		if (adtyp >= AD_MAGM && adtyp <= AD_DETH)
		    ztmp->fltxt = flash_types[adtyp - 1];
		else
		    impossible("setup_zapinfo: unknown wand?");
	    }
	}
}

void
setup_zapobj(ztmp, otmp, mon)
struct zapinfo *ztmp;
struct obj *otmp;
struct monst *mon;
{
	int otyp = otmp->otyp;
	int nd = 6;
	uchar aatyp = AT_NONE;
	uchar adtyp = 0;
	const char *fltxt = 0, *fstxt = 0; /* use default names */
	switch (otmp->oclass) {
	    case WAND_CLASS:
		if (otyp >= WAN_MAGIC_MISSILE && otyp <= WAN_LIGHTNING) {
		    adtyp = (uchar)(otyp - WAN_MAGIC_MISSILE + 1);
		    nd = (otyp == WAN_MAGIC_MISSILE) ? 2 : 6;
		} else switch (otyp) {
		    case WAN_DEATH:	adtyp = AD_DETH; break;
		    default:
			break;
		}
		break;
	    case TOOL_CLASS:
		if (otyp == FROST_HORN || otyp == FIRE_HORN) {
		    adtyp = (otyp == FROST_HORN) ? AD_COLD : AD_FIRE;
		    nd = rn1(6,6);
		}
		break;
	    case SCROLL_CLASS:
		if (otyp == SCR_FIRE) {
		    aatyp = AT_EXPL;
		    adtyp = AD_FIRE;
		    fltxt = "炎の柱";
		}
		break;
	    case POTION_CLASS:
		if (otyp == POT_OIL) {
		    aatyp = AT_EXPL;
		    adtyp = AD_FIRE;
		    fltxt = "燃える油";
		}
		break;
	    default:
		break;
	}
	if (!adtyp) {
	    impossible("setup_zapobj: unknown obj?");
	    adtyp = AD_MAGM;
	}
	setup_zapinfo(ztmp, aatyp, adtyp, nd, 6, fltxt, fstxt, mon);
	ztmp->oclass = otmp->oclass;
}

/* Routines for IMMEDIATE wands and spells. */
/* bhitm: monster mtmp was hit by the effect of wand or spell otmp */
int
bhitm(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;
{
	boolean wake = TRUE;	/* Most 'zaps' should wake monster */
	boolean reveal_invis = FALSE;
	boolean dbldam = Role_if(PM_KNIGHT) && u.uhave.questart;
	int dmg, otyp = otmp->otyp;
	const char *zap_type_text = E_J("spell","呪文");
	struct obj *obj;
	boolean disguised_mimic = (mtmp->data->mlet == S_MIMIC &&
				   mtmp->m_ap_type != M_AP_NOTHING);

	if (u.uswallow && mtmp == u.ustuck)
	    reveal_invis = FALSE;

	switch(otyp) {
	case WAN_STRIKING:
		zap_type_text = E_J("wand","杖");
		/* fall through */
	case SPE_FORCE_BOLT:
		reveal_invis = TRUE;
		if (resists_magm(mtmp)) {	/* match effect on player */
			shieldeff(mtmp->mx, mtmp->my);
			break;	/* skip makeknown */
		} else if (u.uswallow || rnd(20) < 10 + find_mac(mtmp)) {
			dmg = d(2,12);
			if(dbldam) dmg *= 2;
			if (otyp == SPE_FORCE_BOLT)
			    dmg += spell_damage_bonus();
			hit(zap_type_text, mtmp, exclam(dmg));
			(void) resist(mtmp, otmp->oclass, dmg, TELL);
		} else miss(zap_type_text, mtmp);
		makeknown(otyp);
		break;
	case WAN_SLOW_MONSTER:
	case SPE_SLOW_MONSTER:
		if (!resist(mtmp, otmp->oclass, 0, NOTELL)) {
			mon_adjust_speed(mtmp, -1, otmp);
			m_dowear(mtmp, FALSE); /* might want speed boots */
			if (u.uswallow && (mtmp == u.ustuck) &&
			    is_whirly(mtmp->data)) {
				You(E_J("disrupt %s!","%sの流れを乱した！"), mon_nam(mtmp));
				pline(E_J("A huge hole opens up...","巨大な空隙が現れた…。"));
				expels(mtmp, mtmp->data, TRUE);
			}
		}
		break;
	case WAN_SPEED_MONSTER:
		if (!resist(mtmp, otmp->oclass, 0, NOTELL)) {
			mon_adjust_speed(mtmp, 1, otmp);
			m_dowear(mtmp, FALSE); /* might want speed boots */
		}
		break;
	case WAN_UNDEAD_TURNING:
	case SPE_TURN_UNDEAD:
		wake = FALSE;
		if (unturn_dead(mtmp)) wake = TRUE;
		if (is_undead(mtmp->data)) {
			reveal_invis = TRUE;
			wake = TRUE;
			dmg = rnd(8);
			if(dbldam) dmg *= 2;
			if (otyp == SPE_TURN_UNDEAD)
				dmg += spell_damage_bonus();
			flags.bypasses = TRUE;	/* for make_corpse() */
			if (!resist(mtmp, otmp->oclass, dmg, NOTELL)) {
			    if (mtmp->mhp > 0) monflee(mtmp, 0, FALSE, TRUE);
			}
		}
		break;
	case WAN_POLYMORPH:
	case SPE_POLYMORPH:
	case POT_POLYMORPH:
		if (resists_magm(mtmp)) {
		    /* magic resistance protects from polymorph traps, so make
		       it guard against involuntary polymorph attacks too... */
		    shieldeff(mtmp->mx, mtmp->my);
		} else if (!resist(mtmp, otmp->oclass, 0, NOTELL)) {
		    /* natural shapechangers aren't affected by system shock
		       (unless protection from shapechangers is interfering
		       with their metabolism...) */
		    if (mtmp->cham == CHAM_ORDINARY && !rn2(25)) {
			if (canseemon(mtmp)) {
			    pline(E_J("%s shudders!","%sは激しく震えた！"), Monnam(mtmp));
			    makeknown(otyp);
			}
			/* dropped inventory shouldn't be hit by this zap */
			for (obj = mtmp->minvent; obj; obj = obj->nobj)
			    bypass_obj(obj);
			/* flags.bypasses = TRUE; ## for make_corpse() */
			/* no corpse after system shock */
			xkilled(mtmp, 3);
		    } else if (newcham(mtmp, (struct permonst *)0,
				       (otyp != POT_POLYMORPH), FALSE)) {
			if (!Hallucination && canspotmon(mtmp))
			    makeknown(otyp);
		    }
		}
		break;
	case WAN_CANCELLATION:
	case SPE_CANCELLATION:
		(void) cancel_monst(mtmp, otmp, TRUE, TRUE, FALSE);
		break;
	case WAN_TELEPORTATION:
	case SPE_TELEPORT_AWAY:
		reveal_invis = !u_teleport_mon(mtmp, TRUE);
		break;
	case WAN_MAKE_INVISIBLE:
	    {
		int oldinvis = mtmp->minvis;
		char nambuf[BUFSZ];

		/* format monster's name before altering its visibility */
		Strcpy(nambuf, Monnam(mtmp));
		mon_set_minvis(mtmp);
		if (!oldinvis && knowninvisible(mtmp)) {
		    pline(E_J("%s turns transparent!","%sの体は透き通った！"), nambuf);
		    makeknown(otyp);
		}
		break;
	    }
	case WAN_NOTHING:
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	case WAN_MAINTENANCE:
		wake = FALSE;
		break;
	case WAN_PROBING:
		wake = FALSE;
		reveal_invis = TRUE;
		probe_monster(mtmp);
		makeknown(otyp);
		break;
	case WAN_OPENING:
	case SPE_KNOCK:
		wake = FALSE;	/* don't want immediate counterattack */
		if (u.uswallow && mtmp == u.ustuck) {
			if (is_animal(mtmp->data)) {
				if (Blind) You_feel(E_J("a sudden rush of air!",
							"急に突風が吹いた！"));
				else pline(E_J("%s opens its mouth!",
						"%sは口を開けた！"), Monnam(mtmp));
			}
			expels(mtmp, mtmp->data, TRUE);
#ifdef STEED
		} else if (!!(obj = which_armor(mtmp, W_SADDLE))) {
			mtmp->misc_worn_check &= ~obj->owornmask;
			update_mon_intrinsics(mtmp, obj, FALSE, FALSE);
			obj->owornmask = 0L;
			obj_extract_self(obj);
			place_object(obj, mtmp->mx, mtmp->my);
			/* call stackobj() if we ever drop anything that can merge */
			newsym(mtmp->mx, mtmp->my);
#endif
		}
		break;
	case SPE_HEALING:
	case SPE_EXTRA_HEALING:
		reveal_invis = TRUE;
	    if (mtmp->mnum != PM_PESTILENCE) {
		wake = FALSE;		/* wakeup() makes the target angry */
		mlosehp(mtmp, -d( 6+(Role_if(PM_HEALER) ? 1 : 0)+
				  ((flags.female && uarmh && uarmh->otyp == NURSE_CAP) ? 1 : 0)+
				  ((flags.female && uarm  && uarm->otyp  == NURSE_UNIFORM) ? 1 : 0),
				otyp == SPE_EXTRA_HEALING ? 8 : 4));
		if (mtmp->mblinded) {
		    mtmp->mblinded = 0;
		    mtmp->mcansee = 1;
		}
		if (canseemon(mtmp)) {
		    if (disguised_mimic) {
			if (mtmp->m_ap_type == M_AP_OBJECT &&
			    mtmp->mappearance == STRANGE_OBJECT) {
			    /* it can do better now */
			    set_mimic_sym(mtmp);
			    newsym(mtmp->mx, mtmp->my);
			} else
			    mimic_hit_msg(mtmp, otyp);
		    } else pline(E_J("%s looks%s better.","%sは%s元気になったようだ。"), Monnam(mtmp),
				 otyp == SPE_EXTRA_HEALING ? E_J(" much","とても") : "" );
		}
		if (mtmp->mtame || mtmp->mpeaceful) {
		    adjalign(Role_if(PM_HEALER) ? 1 : sgn(u.ualign.type));
		}
	    } else {	/* Pestilence */
		/* Pestilence will always resist; damage is half of 3d{4,8} */
		(void) resist(mtmp, otmp->oclass,
			      d(3, otyp == SPE_EXTRA_HEALING ? 8 : 4), TELL);
	    }
		break;
	case WAN_LIGHT:	/* (broken wand) */
		if (flash_hits_mon(mtmp, otmp)) {
		    makeknown(WAN_LIGHT);
		    reveal_invis = TRUE;
		}
		break;
	case WAN_SLEEP:	/* (broken wand) */
		/* [wakeup() doesn't rouse victims of temporary sleep,
		    so it's okay to leave `wake' set to TRUE here] */
		reveal_invis = TRUE;
		if (sleep_monst(mtmp, d(1 + otmp->spe, 12), WAND_CLASS))
		    slept_monst(mtmp);
		if (!Blind) makeknown(WAN_SLEEP);
		break;
	case SPE_STONE_TO_FLESH:
		if (monsndx(mtmp->data) == PM_STONE_GOLEM) {
		    char *name = Monnam(mtmp);
		    /* turn into flesh golem */
		    if (newcham(mtmp, &mons[PM_FLESH_GOLEM], FALSE, FALSE)) {
			if (canseemon(mtmp))
			    pline(E_J("%s turns to flesh!","%sは肉に変わった！"), name);
		    } else {
			if (canseemon(mtmp))
			    pline(E_J("%s looks rather fleshy for a moment.",
				      "%sは一瞬非常に肉感的に見えた。"),
				  name);
		    }
		} else
		    wake = FALSE;
		break;
	case SPE_DRAIN_LIFE:
		dmg = rnd(8);
		if(dbldam) dmg *= 2;
		if (otyp == SPE_DRAIN_LIFE)
			dmg += spell_damage_bonus();
		if (resists_drli(mtmp))
		    shieldeff(mtmp->mx, mtmp->my);
		else if (!resist(mtmp, otmp->oclass, dmg, NOTELL) &&
				mtmp->mhp > 0) {
		    mtmp->mhp -= dmg;
		    mtmp->mhpmax -= dmg;
		    if (mtmp->mhp <= 0 || mtmp->mhpmax <= 0 || mtmp->m_lev < 1)
			xkilled(mtmp, 1);
		    else {
			mtmp->m_lev--;
			if (canseemon(mtmp))
			    pline(E_J("%s suddenly seems weaker!",
				      "%sは突然弱々しくなった！"), Monnam(mtmp));
		    }
		}
		break;
	default:
		impossible("What an interesting effect (%d)", otyp);
		break;
	}
	if(wake) {
	    if(mtmp->mhp > 0) {
		wakeup(mtmp);
		m_respond(mtmp);
		if(mtmp->isshk && !*u.ushops) hot_pursuit(mtmp);
	    } else if(mtmp->m_ap_type)
		seemimic(mtmp); /* might unblock if mimicing a boulder/door */
	}
	/* note: bhitpos won't be set if swallowed, but that's okay since
	 * reveal_invis will be false.  We can't use mtmp->mx, my since it
	 * might be an invisible worm hit on the tail.
	 */
	if (reveal_invis) {
	    if (mtmp->mhp > 0 && cansee(bhitpos.x, bhitpos.y) &&
							!canspotmons(mtmp))
		map_invisible(bhitpos.x, bhitpos.y);
	}
	return 0;
}

void
probe_monster(mtmp)
struct monst *mtmp;
{
	struct obj *otmp;

	mstatusline(mtmp);
	if (notonhead) return;	/* don't show minvent for long worm tail */

	if (mtmp->minvent || mtmp->mgold) {
	    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		otmp->dknown = 1;	/* treat as "seen" */
	    (void) display_minventory(mtmp, MINV_ALL, (char *)0);
	} else {
	    pline(E_J("%s is not carrying anything.",
		      "%sは何も所持していない。"), noit_Monnam(mtmp));
	}
}


/*
 * Return the object's physical location.  This only makes sense for
 * objects that are currently on the level (i.e. migrating objects
 * are nowhere).  By default, only things that can be seen (in hero's
 * inventory, monster's inventory, or on the ground) are reported.
 * By adding BURIED_TOO and/or CONTAINED_TOO flags, you can also get
 * the location of buried and contained objects.  Note that if an
 * object is carried by a monster, its reported position may change
 * from turn to turn.  This function returns FALSE if the position
 * is not available or subject to the constraints above.
 */
boolean
get_obj_location(obj, xp, yp, locflags)
struct obj *obj;
xchar *xp, *yp;
int locflags;
{
	switch (obj->where) {
	    case OBJ_INVENT:
		*xp = u.ux;
		*yp = u.uy;
		return TRUE;
	    case OBJ_FLOOR:
		*xp = obj->ox;
		*yp = obj->oy;
		return TRUE;
	    case OBJ_MINVENT:
		if (obj->ocarry->mx) {
		    *xp = obj->ocarry->mx;
		    *yp = obj->ocarry->my;
		    return TRUE;
		}
		break;	/* !mx => migrating monster */
	    case OBJ_BURIED:
		if (locflags & BURIED_TOO) {
		    *xp = obj->ox;
		    *yp = obj->oy;
		    return TRUE;
		}
		break;
	    case OBJ_CONTAINED:
		if (locflags & CONTAINED_TOO)
		    return get_obj_location(obj->ocontainer, xp, yp, locflags);
		break;
	}
	*xp = *yp = 0;
	return FALSE;
}

boolean
get_mon_location(mon, xp, yp, locflags)
struct monst *mon;
xchar *xp, *yp;
int locflags;	/* non-zero means get location even if monster is buried */
{
	if (mon == &youmonst) {
	    *xp = u.ux;
	    *yp = u.uy;
	    return TRUE;
	} else if (mon->mx > 0 && (!mon->mburied || locflags)) {
	    *xp = mon->mx;
	    *yp = mon->my;
	    return TRUE;
	} else {	/* migrating or buried */
	    *xp = *yp = 0;
	    return FALSE;
	}
}

/* used by revive() and animate_statue() */
struct monst *
montraits(obj,cc)
struct obj *obj;
coord *cc;
{
	struct monst *mtmp = (struct monst *)0;
	struct monst *mtmp2 = (struct monst *)0;

	mtmp2 = get_mtraits(obj);
	obj->omon = 0;
	if (mtmp2) {
#if 0
		/* save_mtraits() validated mtmp2->mnum */
		mtmp2->data = &mons[mtmp2->mnum];
		if (mtmp2->mhpmax <= 0 && !is_rider(mtmp2->data))
			return (struct monst *)0;
		mtmp = makemon(mtmp2->data,
				cc->x, cc->y, NO_MINVENT|MM_NOWAIT|MM_NOCOUNTBIRTH);
		if (!mtmp) return mtmp;
		/* copy the necessary info from old monster */
		mtmp->m_lev     = mtmp2->m_lev;
		mtmp->malign    = mtmp2->malign;
		mtmp->mtame     = mtmp2->mtame;
		mtmp->mpeaceful = mtmp2->mpeaceful;
		mtmp->female    = mtmp2->female;
		mtmp->minvis    = mtmp2->minvis;
		mtmp->perminvis = mtmp2->perminvis;
		mtmp->invis_blkd= mtmp2->invis_blkd;
		mtmp->mspeed    = mtmp2->mspeed;
		mtmp->permspeed = mtmp2->permspeed;
		mtmp->isshk     = mtmp2->isshk;
		mtmp->isminion  = mtmp2->isminion;
		mtmp->isgd      = mtmp2->isgd;
		mtmp->ispriest  = mtmp2->ispriest;
		mtmp->iswiz     = mtmp2->iswiz;
		mtmp->maligntyp = mtmp2->maligntyp;
		mtmp->has_name  = mtmp2->has_name;
		/* heal the monster */
		if (!is_rider(mtmp2->data) || mtmp->mhpmax < mtmp2->mhpmax)
			mtmp->mhpmax = mtmp2->mhpmax;
		mtmp->mhp = mtmp->mhpmax;
		/* Get these ones from mtmp */
		/* monster ID is available if the monster died in the current
		   game, but should be zero if the corpse was in a bones level
		   (we cleared it when loading bones) */
		if (mtmp2->m_id)
		    mtmp->m_id = mtmp2->m_id;
		/* most cancelled monsters return to normal,
		   but some need to stay cancelled */
		if (dmgtype(mtmp2->data, AD_SEDU)
#ifdef SEDUCE
		    || dmgtype(mtmp2->data, AD_SSEX)
#endif
		    ) mtmp->mcan = mtmp2->mcan;
		mtmp->mextra = mtmp2->mextra;
		mtmp2->mextra = 0;
		refresh_xdat_mon(mtmp);
		monfree(mtmp2);
#else
		mtmp = mtmp2;
		/* heal the monster */
		mtmp->mhp = mtmp->mhpmax;
		mtmp->mx = mtmp->my = 0; /* not on the map */
		mtmp->mtraitskept = 0;
		mtmp->nmon = fmon;
		fmon = mtmp;
		rloc_to(mtmp, cc->x, cc->y);
		refresh_xdat_mon(mtmp);
#endif
	}
	return mtmp;
}

/*
 * get_container_location() returns the following information
 * about the outermost container:
 * loc argument gets set to: 
 *	OBJ_INVENT	if in hero's inventory; return 0.
 *	OBJ_FLOOR	if on the floor; return 0.
 *	OBJ_BURIED	if buried; return 0.
 *	OBJ_MINVENT	if in monster's inventory; return monster.
 * container_nesting is updated with the nesting depth of the containers
 * if applicable.
 */
struct monst *
get_container_location(obj, loc, container_nesting)
struct obj *obj;
int *loc;
int *container_nesting;
{
	if (!obj || !loc)
		return 0;

	if (container_nesting) *container_nesting = 0;
	while (obj && obj->where == OBJ_CONTAINED) {
		if (container_nesting) *container_nesting += 1;
		obj = obj->ocontainer;
	}
	if (obj) {
	    *loc = obj->where;	/* outermost container's location */
	    if (obj->where == OBJ_MINVENT) return obj->ocarry;
	}
	return (struct monst *)0;
}

/*
 * Attempt to revive the given corpse, return the revived monster if
 * successful.  Note: this does NOT use up the corpse if it fails.
 */
struct monst *
revive(obj)
register struct obj *obj;
{
	register struct monst *mtmp = (struct monst *)0;
	struct obj *container = (struct obj *)0;
	int container_nesting = 0;
	schar savetame = 0;
	boolean recorporealization = FALSE;
	boolean in_container = FALSE;
	if(obj->otyp == CORPSE) {
		int montype = obj->corpsenm;
		xchar x, y;

		if (obj->where == OBJ_CONTAINED) {
			/* deal with corpses in [possibly nested] containers */
			struct monst *carrier;
			int holder = 0;

			container = obj->ocontainer;
			carrier = get_container_location(container, &holder,
							&container_nesting);
			switch(holder) {
			    case OBJ_MINVENT:
				x = carrier->mx; y = carrier->my;
				in_container = TRUE;
				break;
			    case OBJ_INVENT:
				x = u.ux; y = u.uy;
				in_container = TRUE;
				break;
			    case OBJ_FLOOR:
				if (!get_obj_location(obj, &x, &y, CONTAINED_TOO))
					return (struct monst *) 0;
				in_container = TRUE;
				break;
			    default:
			    	return (struct monst *)0;
			}
		} else {
			/* only for invent, minvent, or floor */
			if (!get_obj_location(obj, &x, &y, 0))
			    return (struct monst *) 0;
		}
		if (in_container) {
			/* Rules for revival from containers:
			   - the container cannot be locked
			   - the container cannot be heavily nested (>2 is arbitrary)
			   - the container cannot be a statue or bag of holding
			     (except in very rare cases for the latter)
			*/
			if (!x || !y || container->olocked || container_nesting > 2 ||
			    container->otyp == STATUE ||
			    (container->otyp == BAG_OF_HOLDING && rn2(40)))
				return (struct monst *)0;
		}

		if (MON_AT(x,y) || (x == u.ux && y == u.uy)) {
		    coord new_xy;

		    if (enexto(&new_xy, x, y, &mons[montype]))
			x = new_xy.x,  y = new_xy.y;
		}

		if(cant_create(&montype, TRUE)) {
			/* make a zombie or worm instead */
			mtmp = makemon(&mons[montype], x, y,
				       NO_MINVENT|MM_NOWAIT);
			if (mtmp) {
				mtmp->mhp = mtmp->mhpmax = 100;
				mon_adjust_speed(mtmp, 2, (struct obj *)0); /* MFAST */
			}
		} else {
		    if (obj->omon) {
			    coord xy;
			    xy.x = x; xy.y = y;
		    	    mtmp = montraits(obj, &xy);
		    	    if (mtmp && mtmp->mtame && !mtmp->isminion)
				wary_dog(mtmp, TRUE);
		    } else
 		            mtmp = makemon(&mons[montype], x, y,
				       NO_MINVENT|MM_NOWAIT|MM_NOCOUNTBIRTH);
		    if (mtmp) {
			unsigned *tmpid;
			tmpid = (unsigned *)get_xdat_obj(obj, XDAT_M_ID);
			if (tmpid) {
			    unsigned m_id;
			    struct monst *ghost;
			    m_id = *tmpid;
			    ghost = find_mid(m_id, FM_FMON);
		    	    if (ghost && ghost->mnum == PM_GHOST) {
		    		    int x2, y2;
		    		    x2 = ghost->mx; y2 = ghost->my;
		    		    if (ghost->mtame)
		    		    	savetame = ghost->mtame;
		    		    if (canseemon(ghost))
		    		  	pline(E_J("%s is suddenly drawn into its former body!",
						  "突然、%sは元の身体に引き戻された！"),
						Monnam(ghost));
				    mondead(ghost);
				    recorporealization = TRUE;
				    newsym(x2, y2);
			    }
			    /* don't mess with obj->oxlth here */
			}
			/* Monster retains its name */
			if (obj->has_name)
			    mtmp = christen_monst(mtmp, ONAME(obj));
			/* flag the quest leader as alive. */
			if (mtmp->data->msound == MS_LEADER || mtmp->m_id ==
				quest_status.leader_m_id)
			    quest_status.leader_is_dead = FALSE;
		    }
		}
		if (mtmp) {
			if (obj->oclass == FOOD_CLASS && obj->oeaten)
				mtmp->mhp = eaten_stat(mtmp->mhp, obj);
			/* track that this monster was revived at least once */
			mtmp->mrevived = 1;

			if (recorporealization) {
				/* If mtmp is revivification of former tame ghost*/
				if (savetame) {
				    struct monst *mtmp2 = tamedog(mtmp, (struct obj *)0);
				    if (mtmp2) {
					mtmp2->mtame = savetame;
					mtmp = mtmp2;
				    }
				}
				/* was ghost, now alive, it's all very confusing */
				mtmp->mconf = 1;
			}

			switch (obj->where) {
			    case OBJ_INVENT:
				useup(obj);
				break;
			    case OBJ_FLOOR:
				/* in case MON_AT+enexto for invisible mon */
				x = obj->ox,  y = obj->oy;
				/* not useupf(), which charges */
				if (obj->quan > 1L)
				    obj = splitobj(obj, 1L);
				delobj(obj);
				newsym(x, y);
				break;
			    case OBJ_MINVENT:
				m_useup(obj->ocarry, obj);
				break;
			    case OBJ_CONTAINED:
				obj_extract_self(obj);
				obfree(obj, (struct obj *) 0);
				break;
			    default:
				panic("revive");
			}
		}
	}
	return mtmp;
}

STATIC_OVL void
revive_egg(obj)
struct obj *obj;
{
	/*
	 * Note: generic eggs with corpsenm set to NON_PM will never hatch.
	 */
	if (obj->otyp != EGG) return;
	if (obj->corpsenm != NON_PM && !dead_species(obj->corpsenm, TRUE))
	    attach_egg_hatch_timeout(obj);
}

/* try to revive all corpses and eggs carried by `mon' */
int
unturn_dead(mon)
struct monst *mon;
{
	struct obj *otmp, *otmp2;
	struct monst *mtmp2;
	char owner[BUFSZ], corpse[BUFSZ];
	boolean youseeit;
	int once = 0, res = 0;

	youseeit = (mon == &youmonst) ? TRUE : canseemon(mon);
	otmp2 = (mon == &youmonst) ? invent : mon->minvent;

	while ((otmp = otmp2) != 0) {
	    otmp2 = otmp->nobj;
	    if (otmp->otyp == EGG)
		revive_egg(otmp);
	    if (otmp->otyp != CORPSE) continue;
	    /* save the name; the object is liable to go away */
	    if (youseeit) Strcpy(corpse, corpse_xname(otmp, TRUE));

	    /* for a merged group, only one is revived; should this be fixed? */
	    if ((mtmp2 = revive(otmp)) != 0) {
		++res;
		if (youseeit) {
		    if (!once++) Strcpy(owner,
					(mon == &youmonst) ? E_J("Your","あなたの") :
					s_suffix(Monnam(mon)));
		    pline(E_J("%s %s suddenly comes alive!",
			      "%s%sが突然動き出した！"), owner, corpse);
		} else if (canseemon(mtmp2))
		    pline(E_J("%s suddenly appears!",
			      "%sが突然現れた！"), Amonnam(mtmp2));
	    }
	}
	return res;
}

static const char charged_objs[] = { WAND_CLASS, WEAPON_CLASS, ARMOR_CLASS, 0 };

STATIC_OVL void
costly_cancel(obj)
register struct obj *obj;
{
	char objroom;
	struct monst *shkp = (struct monst *)0;

	if (obj->no_charge) return;

	switch (obj->where) {
	case OBJ_INVENT:
		if (obj->unpaid) {
		    shkp = shop_keeper(*u.ushops);
		    if (!shkp) return;
		    Norep(E_J("You cancel an unpaid object, you pay for it!",
			      "売り物を無力化しやがって！ 買ってもらうよ！"));
		    bill_dummy_object(obj);
		}
		break;
	case OBJ_FLOOR:
		if (!costly_spot(obj->ox, obj->oy)) return;
		objroom = inside_shop(obj->ox, obj->oy);
		shkp = shop_keeper(objroom);
		if (!shkp || !inhishop(shkp)) return;
		if (costly_spot(u.ux, u.uy) && objroom == *u.ushops) {
		    Norep(E_J("You cancel it, you pay for it!",
			      "無力化したんだから、買ってもらうよ！"));
		    bill_dummy_object(obj);
		} else
		    (void) stolen_value(obj, obj->ox, obj->oy, FALSE, FALSE);
		break;
	}
}

/* cancel obj, possibly carried by you or a monster */
void
cancel_item(obj)
register struct obj *obj;
{
	boolean	u_ring = (obj == uleft) || (obj == uright);
	register boolean holy = (obj->otyp == POT_WATER && obj->blessed);

	switch(obj->otyp) {
		case RIN_GAIN_STRENGTH:
		case RIN_GAIN_CONSTITUTION:
		case RIN_ADORNMENT:
		case RIN_INCREASE_ACCURACY:
		case RIN_INCREASE_DAMAGE:
		case RIN_PROTECTION:
		case HELM_OF_BRILLIANCE:
		case CLOAK_OF_PROTECTION:
		case ROBE_OF_PROTECTION:
			adj_abon(obj, -obj->spe);
			break;
	}
	if (objects[obj->otyp].oc_magic
	    || (obj->spe && (obj->oclass == ARMOR_CLASS ||
			     obj->oclass == WEAPON_CLASS || is_weptool(obj)))
	    || obj->otyp == POT_ACID || obj->otyp == POT_SICKNESS) {
	    if (obj->spe != ((obj->oclass == WAND_CLASS) ? -1 : 0) &&
	       obj->otyp != WAN_CANCELLATION &&
		 /* can't cancel cancellation */
		 obj->otyp != MAGIC_LAMP &&
		 obj->otyp != CANDELABRUM_OF_INVOCATION) {
		costly_cancel(obj);
		obj->spe = (obj->oclass == WAND_CLASS) ? -1 : 0;
	    }
	    switch (obj->oclass) {
	      case SCROLL_CLASS:
		costly_cancel(obj);
		set_otyp(obj, SCR_BLANK_PAPER);
		obj->spe = 0;
		break;
	      case SPBOOK_CLASS:
		if (obj->otyp != SPE_CANCELLATION &&
			obj->otyp != SPE_BOOK_OF_THE_DEAD) {
		    costly_cancel(obj);
		    set_otyp(obj, SPE_BLANK_PAPER);
		}
		break;
	      case POTION_CLASS:
		costly_cancel(obj);
		if (obj->otyp == POT_SICKNESS ||
		    obj->otyp == POT_SEE_INVISIBLE) {
	    /* sickness is "biologically contaminated" fruit juice; cancel it
	     * and it just becomes fruit juice... whereas see invisible
	     * tastes like "enchanted" fruit juice, it similarly cancels.
	     */
		    set_otyp(obj, POT_FRUIT_JUICE);
		} else {
	            set_otyp(obj, POT_WATER);
		    obj->odiluted = 0; /* same as any other water */
		}
		break;
	    }
	}
	if (holy) costly_cancel(obj);
	unbless(obj);
	uncurse(obj);
	return;
}

/* Remove a positive enchantment or charge from obj,
 * possibly carried by you or a monster
 */
boolean
drain_item(obj)
register struct obj *obj;
{
	boolean u_ring;

	/* Is this a charged/enchanted object? */
	if (!obj || (!objects[obj->otyp].oc_charged &&
			obj->oclass != WEAPON_CLASS &&
			obj->oclass != ARMOR_CLASS && !is_weptool(obj)) ||
			obj->spe <= 0)
	    return (FALSE);
	if (obj_resists(obj, 10, 90))
	    return (FALSE);

	/* Charge for the cost of the object */
	costly_cancel(obj);	/* The term "cancel" is okay for now */

	/* Drain the object and any implied effects */
	obj->spe--;
	u_ring = (obj == uleft) || (obj == uright);
	switch(obj->otyp) {
	case RIN_GAIN_STRENGTH:
	case RIN_GAIN_CONSTITUTION:
	case RIN_ADORNMENT:
	case RIN_INCREASE_ACCURACY:
	case RIN_INCREASE_DAMAGE:
	case RIN_PROTECTION:
	    if ((obj->owornmask & W_RING) && u_ring) {
		adj_abon(obj, -1);
	    }
	    break;
	case HELM_OF_BRILLIANCE:
	    if ((obj->owornmask & W_ARMH) && (obj == uarmh)) {
		adj_abon(obj, -1);
	    }
	    break;
	case GAUNTLETS_OF_DEXTERITY:
	    if ((obj->owornmask & W_ARMG) && (obj == uarmg)) {
		adj_abon(obj, -1);
	    }
	    break;
	case CLOAK_OF_PROTECTION:
	    if ((obj->owornmask & W_ARMC) && (obj == uarmc)) {
		adj_abon(obj, -1);
	    }
	    break;
	case ROBE_OF_PROTECTION:
	    if ((obj->owornmask & W_ARM) && (obj == uarm)) {
		adj_abon(obj, -1);
	    }
	    break;
	}
	if (carried(obj)) update_inventory();
	return (TRUE);
}


boolean
obj_resists(obj, ochance, achance)
struct obj *obj;
int ochance, achance;	/* percent chance for ordinary objects, artifacts */
{
	if (obj->otyp == AMULET_OF_YENDOR ||
	    obj->otyp == SPE_BOOK_OF_THE_DEAD ||
	    obj->otyp == CANDELABRUM_OF_INVOCATION ||
	    obj->otyp == BELL_OF_OPENING ||
	    obj->otyp == RIN_PORTAL ||
	    (obj->otyp == CORPSE && is_rider(&mons[obj->corpsenm]))) {
		return TRUE;
	} else {
		int chance = rn2(100);

		return((boolean)(chance < (obj->oartifact ? achance : ochance)));
	}
}

boolean
obj_shudders(obj)
struct obj *obj;
{
	int	zap_odds;

	if (obj->oclass == WAND_CLASS)
		zap_odds = 3;	/* half-life = 2 zaps */
	else if (obj->cursed)
		zap_odds = 3;	/* half-life = 2 zaps */
	else if (obj->blessed)
		zap_odds = 12;	/* half-life = 8 zaps */
	else
		zap_odds = 8;	/* half-life = 6 zaps */

	/* adjust for "large" quantities of identical things */
	if(obj->quan > 4L) zap_odds /= 2;

	return((boolean)(! rn2(zap_odds)));
}

/* Use up at least minwt number of things made of material mat.
 * There's also a chance that other stuff will be used up.  Finally,
 * there's a random factor here to keep from always using the stuff
 * at the top of the pile.
 */
STATIC_OVL void
polyuse(objhdr, mat, minwt)
    struct obj *objhdr;
    int mat, minwt;
{
    register struct obj *otmp, *otmp2;

    for(otmp = objhdr; minwt > 0 && otmp; otmp = otmp2) {
	otmp2 = otmp->nexthere;
	if (otmp == uball || otmp == uchain) continue;
	if (obj_resists(otmp, 0, 0)) continue;	/* preserve unique objects */
#ifdef MAIL
	if (otmp->otyp == SCR_MAIL) continue;
#endif

	if (((int) get_material(otmp) == mat) ==
		(rn2(minwt + 1) != 0)) {
	    /* appropriately add damage to bill */
	    if (costly_spot(otmp->ox, otmp->oy)) {
		if (*u.ushops)
			addtobill(otmp, FALSE, FALSE, FALSE);
		else
			(void)stolen_value(otmp,
					   otmp->ox, otmp->oy, FALSE, FALSE);
	    }
	    if (otmp->quan < LARGEST_INT)
		minwt -= (int)otmp->quan;
	    else
		minwt = 0;
	    delobj(otmp);
	}
    }
}

/*
 * Polymorph some of the stuff in this pile into a monster, preferably
 * a golem of the kind okind.
 */
STATIC_OVL void
create_polymon(obj, okind)
    struct obj *obj;
    int okind;
{
	struct permonst *mdat = (struct permonst *)0;
	struct monst *mtmp;
	const char *material;
	int pm_index;

	/* no golems if you zap only one object -- not enough stuff */
	if(!obj || (!obj->nexthere && obj->quan == 1L)) return;

	/* some of these choices are arbitrary */
	switch(okind) {
	case IRON:
	case METAL:
	case MITHRIL:
	    pm_index = PM_IRON_GOLEM;
	    material = E_J("metal ","金物");
	    break;
	case COPPER:
	case SILVER:
	case PLATINUM:
	case GEMSTONE:
	case MINERAL:
	    pm_index = rn2(2) ? PM_STONE_GOLEM : PM_CLAY_GOLEM;
	    material = E_J("lithic ","鉱物質");
	    break;
	case 0:
	case FLESH:
	    /* there is no flesh type, but all food is type 0, so we use it */
	    pm_index = PM_FLESH_GOLEM;
	    material = E_J("organic ","有機物");
	    break;
	case WOOD:
	    pm_index = PM_WOOD_GOLEM;
	    material = E_J("wood ","木製品");
	    break;
	case LEATHER:
	    pm_index = PM_LEATHER_GOLEM;
	    material = E_J("leather ","革製品");
	    break;
	case CLOTH:
	    pm_index = PM_ROPE_GOLEM;
	    material = E_J("cloth ","繊維製品");
	    break;
	case BONE:
	    pm_index = PM_SKELETON;     /* nearest thing to "bone golem" */
	    material = E_J("bony ","骨片");
	    break;
	case GOLD:
	    pm_index = PM_GOLD_GOLEM;
	    material = E_J("gold ","金の品物");
	    break;
	case GLASS:
	    pm_index = PM_GLASS_GOLEM;
	    material = E_J("glassy ","ガラス質");
	    break;
	case PAPER:
	    pm_index = PM_PAPER_GOLEM;
	    material = E_J("paper ","紙製品");
	    break;
	default:
	    /* if all else fails... */
	    pm_index = PM_STRAW_GOLEM;
	    material = E_J("","雑多なごみ");
	    break;
	}

	if (!(mvitals[pm_index].mvflags & G_GENOD))
		mdat = &mons[pm_index];

	mtmp = makemon(mdat, obj->ox, obj->oy, NO_MM_FLAGS);
	polyuse(obj, okind, (int)mons[pm_index].cwt);

	if(mtmp && cansee(mtmp->mx, mtmp->my)) {
	    pline(E_J("Some %sobjects meld, and %s arises from the pile!",
		      "いくつかの%sが融合し、品物の山から%sが出現した！"),
		  material, a_monnam(mtmp));
	}
}

/* Assumes obj is on the floor. */
void
do_osshock(obj)
struct obj *obj;
{
	long i;

#ifdef MAIL
	if (obj->otyp == SCR_MAIL) return;
#endif
	obj_zapped = TRUE;

	if(poly_zapped < 0) {
	    /* some may metamorphosize */
	    for (i = obj->quan; i; i--)
		if (! rn2(Luck + 45)) {
		    poly_zapped = get_material(obj);
		    break;
		}
	}

	/* if quan > 1 then some will survive intact */
	if (obj->quan > 1L) {
	    if (obj->quan > LARGEST_INT)
		obj = splitobj(obj, (long)rnd(30000));
	    else
		obj = splitobj(obj, (long)rnd((int)obj->quan - 1));
	}

	/* appropriately add damage to bill */
	if (costly_spot(obj->ox, obj->oy)) {
		if (*u.ushops)
			addtobill(obj, FALSE, FALSE, FALSE);
		else
			(void)stolen_value(obj,
					   obj->ox, obj->oy, FALSE, FALSE);
	}

	/* zap the object */
	delobj(obj);
}

/*
 * Polymorph the object to the given object ID.  If the ID is STRANGE_OBJECT
 * then pick random object from the source's class (this is the standard
 * "polymorph" case).  If ID is set to a specific object, inhibit fusing
 * n objects into 1.  This could have been added as a flag, but currently
 * it is tied to not being the standard polymorph case. The new polymorphed
 * object replaces obj in its link chains.  Return value is a pointer to
 * the new object.
 *
 * This should be safe to call for an object anywhere.
 */
struct obj *
poly_obj(obj, id)
	struct obj *obj;
	int id;
{
	struct obj *otmp;
	xchar ox, oy;
	boolean can_merge = (id == STRANGE_OBJECT);
	int obj_location = obj->where;

	if (obj->otyp == BOULDER && In_sokoban(&u.uz))
	    change_luck(-1);	/* Sokoban guilt */
	if (id == STRANGE_OBJECT) { /* preserve symbol */
	    int try_limit = 3;
	    /* Try up to 3 times to make the magic-or-not status of
	       the new item be the same as it was for the old one. */
	    otmp = (struct obj *)0;
	    do {
		if (otmp) delobj(otmp);
		otmp = mkobj(obj->oclass, FALSE);
	    } while (--try_limit > 0 &&
		  objects[obj->otyp].oc_magic != objects[otmp->otyp].oc_magic);
	} else {
	    /* literally replace obj with this new thing */
	    otmp = mksobj(id, FALSE, FALSE);
	/* Actually more things use corpsenm but they polymorph differently */
#define USES_CORPSENM(typ) ((typ)==CORPSE || (typ)==STATUE || (typ)==FIGURINE)
	    if (USES_CORPSENM(obj->otyp) && USES_CORPSENM(id))
		otmp->corpsenm = obj->corpsenm;
#undef USES_CORPSENM
	}

	/* since sokoprize and boxlock shares the same flag,
	   so avoid boxes for the new object */
	if (Is_sokoprize(obj) && Is_box(otmp)) {
	    set_otyp(otmp, SACK);
	    otmp->otrapped = FALSE;
	}
	otmp->sokoprize = obj->sokoprize;

	/* preserve quantity */
	otmp->quan = obj->quan;
	/* preserve the shopkeepers (lack of) interest */
	otmp->no_charge = obj->no_charge;
	/* preserve inventory letter if in inventory */
	if (obj_location == OBJ_INVENT)
	    otmp->invlet = obj->invlet;
#ifdef MAIL
	/* You can't send yourself 100 mail messages and then
	 * polymorph them into useful scrolls
	 */
	if (obj->otyp == SCR_MAIL) {
		set_otyp(otmp, SCR_MAIL);
		otmp->spe = 1;
	}
#endif

	/* avoid abusing eggs laid by you */
	if (obj->otyp == EGG && obj->spe) {
		int mnum, tryct = 100;

		/* first, turn into a generic egg */
		if (otmp->otyp == EGG)
		    kill_egg(otmp);
		else {
		    set_otyp(otmp, EGG);
		    otmp->owt = weight(otmp);
		}
		otmp->corpsenm = NON_PM;
		otmp->spe = 0;

		/* now change it into something layed by the hero */
		while (tryct--) {
		    mnum = can_be_hatched(random_monster());
		    if (mnum != NON_PM && !dead_species(mnum, TRUE)) {
			otmp->spe = 1;	/* layed by hero */
			otmp->corpsenm = mnum;
			attach_egg_hatch_timeout(otmp);
			break;
		    }
		}
	}

	/* keep special fields (including charges on wands) */
	if (index(charged_objs, otmp->oclass)) otmp->spe = obj->spe;
	otmp->recharged = obj->recharged;

	otmp->cursed = obj->cursed;
	otmp->blessed = obj->blessed;
	otmp->oeroded = obj->oeroded;
	otmp->oeroded2 = obj->oeroded2;
	if (!is_flammable(otmp) && !is_rustprone(otmp)) otmp->oeroded = 0;
	if (!is_corrodeable(otmp) && !is_rottable(otmp)) otmp->oeroded2 = 0;
	if (is_damageable(otmp))
	    otmp->oerodeproof = obj->oerodeproof;

	/* Keep chest/box traps and poisoned ammo if we may */
	if (obj->otrapped && Is_box(otmp)) otmp->otrapped = TRUE;

	if (obj->opoisoned && is_poisonable(otmp))
		otmp->opoisoned = TRUE;

	if (id == STRANGE_OBJECT && obj->otyp == CORPSE) {
	/* turn crocodile corpses into shoes */
	    if (obj->corpsenm == PM_CROCODILE) {
		set_otyp(otmp, LOW_BOOTS);
		otmp->oclass = ARMOR_CLASS;
		otmp->spe = 0;
		otmp->oeroded = 0;
		otmp->oerodeproof = TRUE;
		otmp->quan = 1L;
		otmp->cursed = FALSE;
	    }
	}

	/* no box contents --KAA */
	if (Has_contents(otmp)) delete_contents(otmp);

	/* 'n' merged objects may be fused into 1 object */
	if (otmp->quan > 1L && (!objects[otmp->otyp].oc_merge ||
				(can_merge && otmp->quan > (long)rn2(1000))))
	    otmp->quan = 1L;

	switch (otmp->oclass) {

	case TOOL_CLASS:
	    if (otmp->otyp == MAGIC_LAMP) {
		set_otyp(otmp, OIL_LAMP);
		otmp->age = 1500L;	/* "best" oil lamp possible */
	    } else if (otmp->otyp == MAGIC_MARKER) {
		otmp->recharged = 1;	/* degraded quality */
	    }
	    /* don't care about the recharge count of other tools */
	    break;

	case WAND_CLASS:
	    while (otmp->otyp == WAN_WISHING || otmp->otyp == WAN_POLYMORPH)
		set_otyp(otmp, rnd_class(WAN_LIGHT, WAN_LIGHTNING));
	    /* altering the object tends to degrade its quality
	       (analogous to spellbook `read count' handling) */
	    if ((int)otmp->recharged < rn2(7))	/* recharge_limit */
		otmp->recharged++;
	    break;

	case POTION_CLASS:
	    while (otmp->otyp == POT_POLYMORPH)
		set_otyp(otmp, rnd_class(POT_GAIN_ABILITY, POT_WATER));
	    break;

	case SPBOOK_CLASS:
	    while (otmp->otyp == SPE_POLYMORPH)
		set_otyp(otmp, rnd_class(SPE_DIG, SPE_BLANK_PAPER));
	    /* reduce spellbook abuse */
	    otmp->spestudied = obj->spestudied + 1;
	    break;

	case GEM_CLASS:
	    if (otmp->quan > (long) rnd(4) &&
		    get_material(obj) == MINERAL &&
		    get_material(otmp) != MINERAL) {
		set_otyp(otmp, ROCK);	/* transmutation backfired */
		otmp->quan /= 2L;	/* some material has been lost */
	    }
	    break;

	case FOOD_CLASS:
	    if (otmp->otyp == CORPSE) {
		otmp->age = monstermoves; /* if stone-to-freshed, make it fresh */
		start_corpse_timeout(otmp);
	    }
	    break;

	}

	/* update the weight */
	otmp->owt = weight(otmp);

	/* for now, take off worn items being polymorphed */
	if (obj_location == OBJ_INVENT) {
	    if (id == STRANGE_OBJECT)
		remove_worn_item(obj, TRUE);
	    else {
		/* This is called only for stone to flesh.  It's a lot simpler
		 * than it otherwise might be.  We don't need to check for
		 * special effects when putting them on (no meat objects have
		 * any) and only three worn masks are possible.
		 */
		otmp->owornmask = obj->owornmask;
		remove_worn_item(obj, TRUE);
		setworn(otmp, otmp->owornmask);
		if (otmp->owornmask & LEFT_RING)
		    uleft = otmp;
		if (otmp->owornmask & RIGHT_RING)
		    uright = otmp;
		if (otmp->owornmask & W_WEP)
		    uwep = otmp;
		if (otmp->owornmask & W_SWAPWEP)
		    uswapwep = otmp;
		if (otmp->owornmask & W_QUIVER)
		    uquiver = otmp;
		goto no_unwear;
	    }
	}

	/* preserve the mask in case being used by something else */
	otmp->owornmask = obj->owornmask;
no_unwear:

	/* ** we are now done adjusting the object ** */


	/* swap otmp for obj */
	replace_object(obj, otmp);
	if (obj_location == OBJ_INVENT) {
	    /*
	     * We may need to do extra adjustments for the hero if we're
	     * messing with the hero's inventory.  The following calls are
	     * equivalent to calling freeinv on obj and addinv on otmp,
	     * while doing an in-place swap of the actual objects.
	     */
	    freeinv_core(obj);
	    addinv_core1(otmp);
	    addinv_core2(otmp);
	}

	/* check if vision is changed */
	if (obj_location == OBJ_FLOOR && obj->otyp == BOULDER &&
	    otmp->otyp != BOULDER) {
	    /* boulder -> statue */
	    struct obj *boulder;
	    boulder = sobj_at(BOULDER, obj->ox, obj->oy);
	    if (boulder) {
		/* avoid placing a statue over the boulder */
		obj_extract_self(boulder);
		place_object(boulder, obj->ox, obj->oy);
	    } else
		/* Now no boulder here */
		unblock_point(obj->ox, obj->oy);
	}
	if (obj_location == OBJ_FLOOR && obj->otyp != BOULDER &&
	    otmp->otyp == BOULDER) /* statue -> boulder */
		block_point(obj->ox, obj->oy);

	if ((!carried(otmp) || obj->unpaid) &&
		get_obj_location(otmp, &ox, &oy, BURIED_TOO|CONTAINED_TOO) &&
		costly_spot(ox, oy)) {
	    register struct monst *shkp =
		shop_keeper(*in_rooms(ox, oy, SHOPBASE));

	    if ((!obj->no_charge ||
		 (Has_contents(obj) &&
		    (contained_cost(obj, shkp, 0L, FALSE, FALSE) != 0L)))
	       && inhishop(shkp)) {
		if(shkp->mpeaceful) {
		    if(*u.ushops && *in_rooms(u.ux, u.uy, 0) ==
			    *in_rooms(shkp->mx, shkp->my, 0) &&
			    !costly_spot(u.ux, u.uy))
			make_angry_shk(shkp, ox, oy);
		    else {
			pline(E_J("%s gets angry!","%sは怒った！"), Monnam(shkp));
			hot_pursuit(shkp);
		    }
		} else Norep(E_J("%s is furious!","%sは激怒している！"), Monnam(shkp));
	    }
	}
	delobj(obj);
	return otmp;
}

/*
 * Object obj was hit by the effect of the wand/spell otmp.  Return
 * non-zero if the wand/spell had any effect.
 */
int
bhito(obj, otmp)
struct obj *obj, *otmp;
{
	int res = 1;	/* affected object by default */
	xchar refresh_x, refresh_y;

	if (obj->bypass) {
		/* The bypass bit is currently only used as follows:
		 *
		 * POLYMORPH - When a monster being polymorphed drops something
		 *             from its inventory as a result of the change.
		 *             If the items fall to the floor, they are not
		 *             subject to direct subsequent polymorphing
		 *             themselves on that same zap. This makes it
		 *             consistent with items that remain in the
		 *             monster's inventory. They are not polymorphed
		 *             either.
		 * UNDEAD_TURNING - When an undead creature gets killed via
		 *	       undead turning, prevent its corpse from being
		 *	       immediately revived by the same effect.
		 *
		 * The bypass bit on all objects is reset each turn, whenever
		 * flags.bypasses is set.
		 *
		 * We check the obj->bypass bit above AND flags.bypasses
		 * as a safeguard against any stray occurrence left in an obj
		 * struct someplace, although that should never happen.
		 */
		if (flags.bypasses)
			return 0;
		else {
#ifdef DEBUG
			pline("%s for a moment.", Tobjnam(obj, "pulsate"));
#endif
			obj->bypass = 0;
		}
	}

	/*
	 * Some parts of this function expect the object to be on the floor
	 * obj->{ox,oy} to be valid.  The exception to this (so far) is
	 * for the STONE_TO_FLESH spell.
	 */
	if (!(obj->where == OBJ_FLOOR || otmp->otyp == SPE_STONE_TO_FLESH))
	    impossible("bhito: obj is not floor or Stone To Flesh spell");

	if (obj == uball) {
		res = 0;
	} else if (obj == uchain) {
		if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK) {
		    unpunish();
		    makeknown(otmp->otyp);
		} else
		    res = 0;
	} else
	switch(otmp->otyp) {
	case WAN_POLYMORPH:
	case SPE_POLYMORPH:
		if (obj->otyp == WAN_POLYMORPH ||
			obj->otyp == SPE_POLYMORPH ||
			obj->otyp == POT_POLYMORPH ||
			obj_resists(obj, 5, 95)) {
		    res = 0;
		    break;
		}
		/* KMH, conduct */
		u.uconduct.polypiles++;
		/* any saved lock context will be dangerously obsolete */
		if (Is_box(obj)) (void) boxlock(obj, otmp);

		if (obj_shudders(obj)) {
		    if (cansee(obj->ox, obj->oy))
			makeknown(otmp->otyp);
		    do_osshock(obj);
		    break;
		}
		obj = poly_obj(obj, STRANGE_OBJECT);
		newsym(obj->ox,obj->oy);
		break;
	case WAN_PROBING:
		res = !obj->dknown;
		/* target object has now been "seen (up close)" */
		obj->dknown = 1;
		if (Is_container(obj) || obj->otyp == STATUE) {
		    if (!obj->cobj)
#ifndef JP
			pline("%s empty.", Tobjnam(obj, "are"));
#else
			pline("%sは空だ。", xname(obj));
#endif /*JP*/
		    else {
			struct obj *o;
			/* view contents (not recursively) */
			for (o = obj->cobj; o; o = o->nobj)
			    o->dknown = 1;	/* "seen", even if blind */
			(void) display_cinventory(obj);
		    }
		    res = 1;
		}
		if (res) makeknown(WAN_PROBING);
		break;
	case WAN_STRIKING:
	case SPE_FORCE_BOLT:
		if (obj->otyp == BOULDER)
			fracture_rock(obj);
		else if (obj->otyp == STATUE)
			(void) break_statue(obj);
		else {
			if (!flags.mon_moving)
			    (void)hero_breaks(obj, obj->ox, obj->oy, FALSE);
			else
			    (void)breaks(obj, obj->ox, obj->oy);
			res = 0;
		}
		/* BUG[?]: shouldn't this depend upon you seeing it happen? */
		makeknown(otmp->otyp);
		break;
	case WAN_CANCELLATION:
	case SPE_CANCELLATION:
		cancel_item(obj);
#ifdef TEXTCOLOR
		newsym(obj->ox,obj->oy);	/* might change color */
#endif
		break;
	case SPE_DRAIN_LIFE:
		(void) drain_item(obj);
		break;
	case WAN_TELEPORTATION:
	case SPE_TELEPORT_AWAY:
		rloco(obj);
		break;
	case WAN_MAKE_INVISIBLE:
		break;
	case WAN_UNDEAD_TURNING:
	case SPE_TURN_UNDEAD:
		if (obj->otyp == EGG)
			revive_egg(obj);
		else
			res = !!revive(obj);
		break;
	case WAN_OPENING:
	case SPE_KNOCK:
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
		if(Is_box(obj))
			res = boxlock(obj, otmp);
		else
			res = 0;
		if (res /* && otmp->oclass == WAND_CLASS */)
			makeknown(otmp->otyp);
		break;
	case WAN_SLOW_MONSTER:		/* no effect on objects */
	case SPE_SLOW_MONSTER:
	case WAN_SPEED_MONSTER:
	case WAN_NOTHING:
	case SPE_HEALING:
	case SPE_EXTRA_HEALING:
		res = 0;
		break;
	case SPE_STONE_TO_FLESH:
		refresh_x = obj->ox; refresh_y = obj->oy;
		if (get_material(obj) != MINERAL &&
			get_material(obj) != GEMSTONE) {
		    res = 0;
		    break;
		}
		/* add more if stone objects are added.. */
		switch (objects[obj->otyp].oc_class) {
		    case ROCK_CLASS:	/* boulders and statues */
			if (obj->otyp == BOULDER) {
			    obj = poly_obj(obj, HUGE_CHUNK_OF_MEAT);
			    goto smell;
			} else if (obj->otyp == STATUE) {
			    xchar oox, ooy;

			    (void) get_obj_location(obj, &oox, &ooy, 0);
			    refresh_x = oox; refresh_y = ooy;
			    if (vegetarian(&mons[obj->corpsenm])) {
				/* Don't animate monsters that aren't flesh */
				obj = poly_obj(obj, MEATBALL);
			    	goto smell;
			    }
			    if (!animate_statue(obj, oox, ooy,
						ANIMATE_SPELL, (int *)0)) {
				struct obj *item;
makecorpse:			if (mons[obj->corpsenm].geno &
							(G_NOCORPSE|G_UNIQ)) {
				    res = 0;
				    break;
				}
				/* Unlikely to get here since genociding
				 * monsters also sets the G_NOCORPSE flag.
				 * Drop the contents, poly_obj looses them.
				 */
				while ((item = obj->cobj) != 0) {
				    obj_extract_self(item);
				    place_object(item, oox, ooy);
				}
				obj = poly_obj(obj, CORPSE);
				break;
			    }
			} else { /* new rock class object... */
			    /* impossible? */
			    res = 0;
			}
			break;
		    case TOOL_CLASS:	/* figurine */
		    {
			struct monst *mon;
			xchar oox, ooy;

			if (obj->otyp != FIGURINE) {
			    res = 0;
			    break;
			}
			if (vegetarian(&mons[obj->corpsenm])) {
			    /* Don't animate monsters that aren't flesh */
			    obj = poly_obj(obj, MEATBALL);
			    goto smell;
			}
			(void) get_obj_location(obj, &oox, &ooy, 0);
			refresh_x = oox; refresh_y = ooy;
			mon = makemon(&mons[obj->corpsenm],
				      oox, ooy, NO_MM_FLAGS);
			if (mon) {
			    delobj(obj);
			    if (cansee(mon->mx, mon->my))
				pline_The(E_J("figurine animates!","人形は動き出した！"));
			    break;
			}
			goto makecorpse;
		    }
		    /* maybe add weird things to become? */
		    case RING_CLASS:	/* some of the rings are stone */
			obj = poly_obj(obj, MEAT_RING);
			goto smell;
		    case WAND_CLASS:	/* marble wand */
			obj = poly_obj(obj, MEAT_STICK);
			goto smell;
		    case GEM_CLASS:	/* rocks & gems */
			obj = poly_obj(obj, MEATBALL);
smell:
			if (herbivorous(youmonst.data) &&
			    (!carnivorous(youmonst.data) ||
			     Role_if(PM_MONK) || !u.uconduct.unvegetarian))
			    Norep(E_J("You smell the odor of meat.",
				      "あなたは肉の匂いを嗅いだ。"));
			else
			    Norep(E_J("You smell a delicious smell.",
				      "美味しそうな匂いがする。"));
			break;
		    case WEAPON_CLASS:	/* crysknife */
		    	/* fall through */
		    default:
			res = 0;
			break;
		}
		newsym(refresh_x, refresh_y);
		break;
	default:
		impossible("What an interesting effect (%d)", otmp->otyp);
		break;
	}
	return res;
}

/* returns nonzero if something was hit */
int
bhitpile(obj,fhito,tx,ty)
    struct obj *obj;
    int FDECL((*fhito), (OBJ_P,OBJ_P));
    int tx, ty;
{
    int hitanything = 0;
    register struct obj *otmp, *next_obj;

    if (obj->otyp == SPE_FORCE_BOLT || obj->otyp == WAN_STRIKING) {
	struct trap *t = t_at(tx, ty);

	/* We can't settle for the default calling sequence of
	   bhito(otmp) -> break_statue(otmp) -> activate_statue_trap(ox,oy)
	   because that last call might end up operating on our `next_obj'
	   (below), rather than on the current object, if it happens to
	   encounter a statue which mustn't become animated. */
	if (t && t->ttyp == STATUE_TRAP &&
	    activate_statue_trap(t, tx, ty, TRUE) && obj->otyp == WAN_STRIKING)
	    makeknown(obj->otyp);
    }

    poly_zapped = -1;
    for(otmp = level.objects[tx][ty]; otmp; otmp = next_obj) {
	/* Fix for polymorph bug, Tim Wright */
	next_obj = otmp->nexthere;
	hitanything += (*fhito)(otmp, obj);
#ifdef NOPOLYPILE
	if (obj->otyp == WAN_POLYMORPH || obj->otyp == SPE_POLYMORPH) break;
#endif /*NOPOLYPILE*/
    }
    if(poly_zapped >= 0)
	create_polymon(level.objects[tx][ty], poly_zapped);

    return hitanything;
}

/*
 * zappable - returns 1 if zap is available, 0 otherwise.
 *	      it removes a charge from the wand if zappable.
 * added by GAN 11/03/86
 */
int
zappable(wand)
register struct obj *wand;
{
	if(wand->spe < 0 || (wand->spe == 0 && rn2(10)))
		return 0;
	if(wand->spe == 0 && wand->otyp != WAN_NOTHING) {
		You(E_J("wrest one last charge from the worn-out wand.",
			"使い切られた杖から最後の魔力をしぼり出した。"));
		force_more();
	}
	wand->spe--;
	return 1;
}

/*
 * zapnodir - zaps a NODIR wand/spell.
 * added by GAN 11/03/86
 */
void
zapnodir(obj)
register struct obj *obj;
{
	boolean known = FALSE;

	switch(obj->otyp) {
		case WAN_LIGHT:
		case SPE_LIGHT:
			litroom(TRUE,obj);
			if (!Blind) known = TRUE;
			break;
		case WAN_SECRET_DOOR_DETECTION:
		case SPE_DETECT_UNSEEN:
			if(!findit()) return;
			if (!Blind) known = TRUE;
			break;
		case WAN_CREATE_MONSTER:
			known = create_critters(rn2(23) ? 1 : rn1(7,2),
					(struct permonst *)0);
			break;
		case WAN_WISHING:
			known = TRUE;
			if(Luck + rn2(5) < 0) {
				pline(E_J("Unfortunately, nothing happens.",
					  "不幸なことに、何も起きなかった。"));
				break;
			}
			makewish();
			break;
		case WAN_ENLIGHTENMENT:
			known = TRUE;
			You_feel(E_J("self-knowledgeable...",
				     "自分をより良く理解した…。"));
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(FALSE);
			pline_The(E_J("feeling subsides.","直感は消えていった。"));
			exercise(A_WIS, TRUE);
			break;
		case WAN_MAINTENANCE: {
			struct obj *otmp;
			const char *spword = 0;
			boolean fsav; /* supress "Never mind" in getobj() */
			int i = 0;
#ifdef JP
			static const struct getobj_words rprw = { 0, 0, "修理する", "修理し" };
#endif /*JP*/
			known = TRUE;
			if (!objects[obj->otyp].oc_name_known)
			    pline(E_J("This is a wand of maintenance.","これは修復の杖だ。"));
			fsav = flags.verbose;
			flags.verbose = 0;
			otmp = getobj(all_count, E_J("repair",&rprw), getobj_filter_repair);
			flags.verbose = fsav;
			if (!otmp) {
			    if (!Blind)
#ifndef JP
				pline("%s glows and fades.", The(xname(obj)));
#else
				pline("%sは一瞬輝いたが、光は消えた。", xname(obj));
#endif /*JP*/
			    break;
			}
			if (!is_damageable(otmp) ||
			    (!otmp->oeroded && !otmp->oeroded2) ||
			    otmp->oclass == FOOD_CLASS || otmp->oclass == POTION_CLASS ||
			    otmp->oclass == SCROLL_CLASS || otmp->oclass == SPBOOK_CLASS ||
			    otmp->oclass == GEM_CLASS || otmp->oclass == ROCK_CLASS ||
			    otmp->oclass == AMULET_CLASS) {
			    pline(nothing_happens);
			    break;
			}
			otmp->oeroded = otmp->oeroded2 = 0;
			if (otmp->otyp == DENTED_POT) spword = E_J("dented","くぼんでいる");
			else if (objects[otmp->otyp].oc_descr_idx == TATTERED_CAPE)
			    spword = E_J("tattered","ぼろぼろな");
#ifndef JP
			Your("%s %s as good as new!",
				 xname(otmp),
				 otense(otmp, Blind ? "feel" : "look"));
			if (spword) pline("(Though it is still %s...)", spword);
#else
			Your("%sは新品同様になった%s！", xname(otmp), Blind ? "ようだ" : "");
			if (spword) pline("(%sのは相変わらずだが…)", spword);
#endif /*JP*/
		}
	}
	if (known && !objects[obj->otyp].oc_name_known) {
		makeknown(obj->otyp);
		more_experienced(0,10);
	}
}

STATIC_OVL void
backfire(otmp)
struct obj *otmp;
{
	otmp->in_use = TRUE;	/* in case losehp() is fatal */
#ifndef JP
	pline("%s suddenly explodes!", The(xname(otmp)));
#else
	pline("突然、%sが爆発した！", xname(otmp));
#endif /*JP*/
	losehp(d(otmp->spe+2,6), E_J("exploding wand","杖が爆発して"), KILLED_BY_AN);
	useup(otmp);
}

static NEARDATA const char zap_syms[] = { WAND_CLASS, 0 };

int
dozap()
{
	register struct obj *obj;
	int	damage;
#ifdef JP
	static const struct getobj_words zapw = { "どの杖", 0, "振る", "振り" };
#endif /*JP*/

	if(check_capacity((char *)0)) return(0);
	obj = getobj(zap_syms, E_J("zap",&zapw), 0);
	if(!obj) return(0);

	check_unpaid(obj);

	/* zappable addition done by GAN 11/03/86 */
	if(!zappable(obj)) pline(nothing_happens);
	else if(obj->cursed && !rn2(100)) {
		backfire(obj);	/* the wand blows up in your face! */
		exercise(A_STR, FALSE);
		return(1);
	} else if(!(objects[obj->otyp].oc_dir == NODIR) && !/*getdir((char *)0)*/
		  getdir_or_pos(0, GETPOS_MONTGT, (char *)0, E_J("zap at","目標"))) {
		if (!Blind)
#ifndef JP
		    pline("%s glows and fades.", The(xname(obj)));
#else
		    pline("%sは輝き、元に戻った。", xname(obj));
#endif /*JP*/
		/* make him pay for knowing !NODIR */
	} else if(!u.dx && !u.dy && !u.dz && !(objects[obj->otyp].oc_dir == NODIR)) {
	    if ((damage = zapyourself(obj, TRUE)) != 0) {
#ifndef JP
		char buf[BUFSZ];
		Sprintf(buf, "zapped %sself with a wand", uhim());
#endif /*JP*/
		losehp(damage, E_J(buf,"杖を自分めがけて撃った"), NO_KILLER_PREFIX);
	    }
	} else {

		/*	Are we having fun yet?
		 * weffects -> buzz(obj->otyp) -> zhitm (temple priest) ->
		 * attack -> hitum -> known_hitum -> ghod_hitsu ->
		 * buzz(AD_ELEC) -> destroy_item(WAND_CLASS) ->
		 * useup -> obfree -> dealloc_obj -> free(obj)
		 */
		current_wand = obj;
		weffects(obj);
		obj = current_wand;
		current_wand = 0;
	}
	if (obj && obj->spe < 0) {
#ifndef JP
	    pline("%s to dust.", Tobjnam(obj, "turn"));
#else
	    pline("%sは塵になった。", xname(obj));
#endif /*JP*/
	    useup(obj);
	}
	update_inventory();	/* maybe used a charge */
	return(1);
}

int
zapyourself(obj, ordinary)
struct obj *obj;
boolean ordinary;
{
	int	damage = 0;
	char buf[BUFSZ];

	switch(obj->otyp) {
		case WAN_STRIKING:
		    makeknown(WAN_STRIKING);
		case SPE_FORCE_BOLT:
/* Striking/force bolt is now physical attack */
//		    if(Antimagic) {
//			shieldeff(u.ux, u.uy);
//			pline("Boing!");
//		    } else {
			if (ordinary) {
			    You(E_J("bash yourself!","自分を打ち据えた！"));
			    damage = d(2,12);
			} else
			    damage = d(1 + obj->spe,6);
			exercise(A_STR, FALSE);
//		    }
		    break;

		case WAN_LIGHTNING:
		    makeknown(WAN_LIGHTNING);
		    if (Shock_resistance && is_full_resist(SHOCK_RES)) {
			shieldeff(u.ux, u.uy);
			You(E_J("zap yourself, but seem unharmed.",
				"自分を撃ったが、傷つかなかったようだ。"));
			ugolemeffects(AD_ELEC, d(12,6));
			break;
		    } else {
			You(E_J("shock yourself!",
				"自分に電撃をくらわせた！"));
			damage = d((Shock_resistance) ? 3: 12, 6);
			exercise(A_CON, FALSE);
		    }
		    destroy_item(WAND_CLASS, AD_ELEC);
		    destroy_item(RING_CLASS, AD_ELEC);
		    if (!resists_blnd(&youmonst)) {
			    You(are_blinded_by_the_flash);
			    make_blinded((long)rnd(100),FALSE);
			    if (!Blind) Your(vision_clears);
		    }
		    break;

		case SPE_FIREBALL: {
		    struct zapinfo zi;
		    You(E_J("explode a fireball on top of yourself!",
			    "自分の目の前でファイアボールを炸裂させた！"));
		    setup_zapinfo(&zi, AT_EXPL, AD_FIRE, 1, 1, 0, 0, &youmonst);
		    zi.oclass = 0;
		    explode(u.ux, u.uy, &zi, d(6,6), EXPL_FIERY);
		    break;
		}
		case WAN_FIRE:
		    makeknown(WAN_FIRE);
		case FIRE_HORN:
		    if (Fire_resistance && is_full_resist(FIRE_RES)) {
			shieldeff(u.ux, u.uy);
			E_J(You_feel("rather warm."), pline("少々暑すぎるようだ。"));
			ugolemeffects(AD_FIRE, d(12,6));
		    } else {
			pline(E_J("You've set yourself afire!",
				  "あなたは自分を火だるまにした！"));
			damage = d((Fire_resistance) ? 3 : 12, 6);
		    }
		    burn_away_slime();
		    (void) burnarmor(&youmonst);
		    destroy_item(SCROLL_CLASS, AD_FIRE);
		    destroy_item(POTION_CLASS, AD_FIRE);
		    destroy_item(SPBOOK_CLASS, AD_FIRE);
		    destroy_item(TOOL_CLASS, AD_FIRE);
		    break;

		case WAN_COLD:
		    makeknown(WAN_COLD);
		case SPE_CONE_OF_COLD:
		case FROST_HORN:
		    if (Cold_resistance && is_full_resist(COLD_RES)) {
			shieldeff(u.ux, u.uy);
			E_J(You_feel("a little chill."), pline("少々涼しすぎるようだ。"));
			ugolemeffects(AD_COLD, d(12,6));
		    } else {
			You(E_J("imitate a popsicle!","アイスキャンデーの真似をした！"));
			damage = d((Cold_resistance) ? 3 : 12, 6);
		    }
		    destroy_item(POTION_CLASS, AD_COLD);
		    break;

		case WAN_MAGIC_MISSILE:
		    makeknown(WAN_MAGIC_MISSILE);
		case SPE_MAGIC_MISSILE:
		    if(Antimagic) {
			shieldeff(u.ux, u.uy);
			pline_The(E_J("missiles bounce!","矢ははね返された！"));
			damage_resistant_obj(ANTIMAGIC, 1);
		    } else {
			damage = d(4,6);
			pline(E_J("Idiot!  You've shot yourself!",
				  "この間抜け！ あなたは自分を撃った！"));
		    }
		    break;

		case WAN_POLYMORPH:
		    if (!Unchanging)
		    	makeknown(WAN_POLYMORPH);
		case SPE_POLYMORPH:
		    if (!Unchanging)
		    	polyself(FALSE);
		    break;

		case WAN_CANCELLATION:
		case SPE_CANCELLATION:
		    (void) cancel_monst(&youmonst, obj, TRUE, FALSE, TRUE);
		    break;

		case SPE_DRAIN_LIFE:
			if (!Drain_resistance) {
				losexp(E_J("life drainage","生命力を奪われて"));
				makeknown(obj->otyp);
			}
			damage = 0;	/* No additional damage */
			break;

		case WAN_MAKE_INVISIBLE: {
		    /* have to test before changing HInvis but must change
		     * HInvis before doing newsym().
		     */
		    int msg = !Invis && !Blind && !BInvis;

		    if (BInvis && uarmc->otyp == MUMMY_WRAPPING) {
			/* A mummy wrapping absorbs it and protects you */
#ifndef JP
		        You_feel("rather itchy under your %s.", xname(uarmc));
#else
		        Your("巻いている%sの下がむずがゆくなった。", xname(uarmc));
#endif /*JP*/
		        break;
		    }
/* never get permanent invisibity by wand - youkan */
//		    if (ordinary || !rn2(10)) {	/* permanent */
//			HInvis |= FROMOUTSIDE;
//		    } else {			/* temporary */
		    	incr_itimeout(&HInvis, d(obj->spe, 250));
//		    }
		    if (msg) {
			makeknown(WAN_MAKE_INVISIBLE);
			newsym(u.ux, u.uy);
			self_invis_message();
		    }
		    break;
		}

		case WAN_SPEED_MONSTER:
		    if (Very_fast) {
#ifndef JP
			Your("%s get new energy.",
				makeplural(body_part(LEG)));
#else
			Your("%sは新たな活力を得た。", body_part(LEG));
#endif /*JP*/
			break;
		    }
		    You(E_J("are suddenly moving %sfaster.",
			    "突然%s素早く動けるようになった。"),
				(Fast || BFast) ? "" : E_J("much ","とても"));
		    u.uspdbon2 = 2;
		    incr_itimeout(&HFast, rn1(10, 10));
/* wand of speed monster no longer convey intrinsic speed */
//		    if (!(HFast & INTRINSIC)) {
//			if (!Fast)
//			    You("speed up.");
//			else
//			    Your("quickness feels more natural.");
//			makeknown(WAN_SPEED_MONSTER);
//			exercise(A_DEX, TRUE);
//		    }
//		    HFast |= FROMOUTSIDE;
		    break;

		case WAN_SLEEP:
		    makeknown(WAN_SLEEP);
		case SPE_SLEEP:
		    if(Sleep_resistance) {
			shieldeff(u.ux, u.uy);
			You(E_J("don't feel sleepy!","眠気を感じない！"));
		    } else {
			pline_The(E_J("sleep ray hits you!","眠りの光線があなたに命中した！"));
			fall_asleep(-rnd(50), TRUE);
		    }
		    break;

		case WAN_SLOW_MONSTER:
		case SPE_SLOW_MONSTER:
		    if(HFast & (TIMEOUT | INTRINSIC)) {
			u_slow_down();
			makeknown(obj->otyp);
		    }
		    break;

		case WAN_TELEPORTATION:
		case SPE_TELEPORT_AWAY:
		    tele();
		    break;

		case WAN_DEATH:
		case SPE_FINGER_OF_DEATH:
		    if (vs_death_factor(youmonst.data)) {
			pline((obj->otyp == WAN_DEATH) ?
			  E_J("The wand shoots an apparently harmless beam at you.",
			      "杖から明らかに無害な光線が放たれ、あなたを照らした。")
			  : E_J("You seem no deader than before.",
				"あなたはこれ以上死なないようだ。"));
			break;
		    }
#ifndef JP
		    Sprintf(buf, "shot %sself with a death ray", uhim());
#endif /*JP*/
		    killer = E_J(buf, "死の光線を自分めがけて撃った");
		    killer_format = NO_KILLER_PREFIX;
		    You(E_J("irradiate yourself with pure energy!",
			    "自らを純粋な死のエネルギーに曝した！"));
		    You(E_J("die.","死んだ。"));
		    makeknown(obj->otyp);
			/* They might survive with an amulet of life saving */
		    done(DIED);
		    break;
		case WAN_UNDEAD_TURNING:
		    makeknown(WAN_UNDEAD_TURNING);
		case SPE_TURN_UNDEAD:
		    (void) unturn_dead(&youmonst);
		    if (is_undead(youmonst.data)) {
			You_feel(E_J("frightened and %sstunned.",
				     "%sおびえ、よろめいた。"),
			     Stunned ? E_J("even more ","より一層") : "");
			make_stunned(HStun + rnd(30), FALSE);
		    } else
			You(E_J("shudder in dread.","恐怖におののいた。"));
		    break;
		case SPE_HEALING:
		case SPE_EXTRA_HEALING: {
		    int oldhp;
		    oldhp = u.uhp;
		    healup(d(6, obj->otyp == SPE_EXTRA_HEALING ? 8 : 4),
			   0, FALSE, (obj->otyp == SPE_EXTRA_HEALING));
		    You_feel(E_J("%sbetter.","%s元気になった。"),
			obj->otyp == SPE_EXTRA_HEALING ? E_J("much ","とても") : "");
		    if (u.uhp > oldhp) morehungry(u.uhp - oldhp);
		    break;
		}
		case WAN_LIGHT:	/* (broken wand) */
		 /* assert( !ordinary ); */
		    damage = d(obj->spe, 25);
		case EXPENSIVE_CAMERA:
		    damage += rnd(25);
		    if (!resists_blnd(&youmonst)) {
			You(are_blinded_by_the_flash);
			make_blinded((long)damage, FALSE);
			makeknown(obj->otyp);
			if (!Blind) Your(vision_clears);
		    }
		    damage = 0;	/* reset */
		    break;
		case WAN_OPENING:
		    if (Punished) makeknown(WAN_OPENING);
		case SPE_KNOCK:
		    if (Punished) Your(E_J("chain quivers for a moment.",
					   "鎖が一瞬震えた。"));
		    break;
		case WAN_DIGGING:
		case SPE_DIG:
		case SPE_DETECT_UNSEEN:
		case WAN_NOTHING:
		case WAN_LOCKING:
		case SPE_WIZARD_LOCK:
		case WAN_MAINTENANCE:
		    break;
		case WAN_PROBING:
		    for (obj = invent; obj; obj = obj->nobj)
			obj->dknown = 1;
		    /* note: `obj' reused; doesn't point at wand anymore */
		    makeknown(WAN_PROBING);
		    ustatusline();
		    break;
		case SPE_STONE_TO_FLESH:
		    {
		    struct obj *otemp, *onext;
		    boolean didmerge;

		    if (u.umonnum == PM_STONE_GOLEM)
			(void) polymon(PM_FLESH_GOLEM);
		    if (Stoned) fix_petrification();	/* saved! */
		    /* but at a cost.. */
		    for (otemp = invent; otemp; otemp = onext) {
			onext = otemp->nobj;
			(void) bhito(otemp, obj);
			}
		    /*
		     * It is possible that we can now merge some inventory.
		     * Do a higly paranoid merge.  Restart from the beginning
		     * until no merges.
		     */
		    do {
			didmerge = FALSE;
			for (otemp = invent; !didmerge && otemp; otemp = otemp->nobj)
			    for (onext = otemp->nobj; onext; onext = onext->nobj)
			    	if (merged(&otemp, &onext)) {
			    		didmerge = TRUE;
			    		break;
			    		}
		    } while (didmerge);
		    }
		    break;
		default: impossible("object %d used?",obj->otyp);
		    break;
	}
	return(damage);
}

#ifdef STEED
/* you've zapped a wand downwards while riding
 * Return TRUE if the steed was hit by the wand.
 * Return FALSE if the steed was not hit by the wand.
 */
STATIC_OVL boolean
zap_steed(obj)
struct obj *obj;	/* wand or spell */
{
	int steedhit = FALSE;
	
	switch (obj->otyp) {

	   /*
	    * Wands that are allowed to hit the steed
	    * Carefully test the results of any that are
	    * moved here from the bottom section.
	    */
		case WAN_PROBING:
		    probe_monster(u.usteed);
		    makeknown(WAN_PROBING);
		    steedhit = TRUE;
		    break;
		case WAN_TELEPORTATION:
		case SPE_TELEPORT_AWAY:
		    /* you go together */
		    tele();
		    if(Teleport_control || !couldsee(u.ux0, u.uy0) ||
			(distu(u.ux0, u.uy0) >= 16))
				makeknown(obj->otyp);
		    steedhit = TRUE;
		    break;

		/* Default processing via bhitm() for these */
		case SPE_CURE_SICKNESS:
		case WAN_MAKE_INVISIBLE:
		case WAN_CANCELLATION:
		case SPE_CANCELLATION:
		case WAN_POLYMORPH:
		case SPE_POLYMORPH:
		case WAN_STRIKING:
		case SPE_FORCE_BOLT:
		case WAN_SLOW_MONSTER:
		case SPE_SLOW_MONSTER:
		case WAN_SPEED_MONSTER:
		case SPE_HEALING:
		case SPE_EXTRA_HEALING:
		case SPE_DRAIN_LIFE:
		case WAN_OPENING:
		case SPE_KNOCK:
		    (void) bhitm(u.usteed, obj);
		    steedhit = TRUE;
		    break;

		default:
		    steedhit = FALSE;
		    break;
	}
	return steedhit;
}
#endif


/*
 * cancel a monster (possibly the hero).  inventory is cancelled only
 * if the monster is zapping itself directly, since otherwise the
 * effect is too strong.  currently non-hero monsters do not zap
 * themselves with cancellation.
 */
boolean
cancel_monst(mdef, obj, youattack, allow_cancel_kill, self_cancel)
register struct monst	*mdef;
register struct obj	*obj;
boolean			youattack, allow_cancel_kill, self_cancel;
{
	boolean	youdefend = (mdef == &youmonst);
	static const char writing_vanishes[] =
				E_J("Some writing vanishes from %s head!",
				    "%s頭に書かれた文字が消えた！");
	static const char your[] = E_J("your","あなたの");	/* should be extern */

	if (youdefend ? (!youattack && Antimagic)
		      : resist(mdef, obj->oclass, 0, NOTELL))
		return FALSE;	/* resisted cancellation */

	if (self_cancel) {	/* 1st cancel inventory */
	    struct obj *otmp;

	    for (otmp = (youdefend ? invent : mdef->minvent);
			    otmp; otmp = otmp->nobj)
		cancel_item(otmp);
	    if (youdefend) {
		flags.botl = 1;	/* potential AC change */
		find_ac();
	    }
	}

	/* now handle special cases */
	if (youdefend) {
	    if (Upolyd) {
		if ((u.umonnum == PM_CLAY_GOLEM) && !Blind)
		    pline(writing_vanishes, your);

		if (Unchanging)
		    Your(E_J("amulet grows hot for a moment, then cools.",
			     "魔除けが一瞬熱くなり、そして元に戻った。"));
		else
		    rehumanize();
	    }
	} else {
	    mdef->mcan = TRUE;

	    if (is_were(mdef->data) && mdef->data->mlet != S_HUMAN)
		were_change(mdef);

	    if (mdef->mnum == PM_CLAY_GOLEM) {
		if (canseemon(mdef))
		    pline(writing_vanishes, s_suffix(mon_nam(mdef)));

		if (allow_cancel_kill) {
		    if (youattack)
			killed(mdef);
		    else
			monkilled(mdef, "", AD_SPEL);
		}
	    }
	}
	return TRUE;
}

/* you've zapped an immediate type wand up or down */
STATIC_OVL boolean
zap_updown(obj)
struct obj *obj;	/* wand or spell */
{
	boolean striking = FALSE, disclose = FALSE;
	int x, y, xx, yy, ptmp;
	struct obj *otmp;
	struct engr *e;
	struct trap *ttmp;
	struct zapinfo zi;
	char buf[BUFSZ];

	/* some wands have special effects other than normal bhitpile */
	/* drawbridge might change <u.ux,u.uy> */
	x = xx = u.ux;	/* <x,y> is zap location */
	y = yy = u.uy;	/* <xx,yy> is drawbridge (portcullis) position */
	ttmp = t_at(x, y); /* trap if there is one */

	switch (obj->otyp) {
	case WAN_PROBING:
	    ptmp = 0;
	    if (u.dz < 0) {
		You(E_J("probe towards the %s.","%sの方向を探査した。"), ceiling(x,y));
	    } else {
		ptmp += bhitpile(obj, bhito, x, y);
		You(E_J("probe beneath the %s.","%sの下を探査した。"), surface(x,y));
		if (covers_objects(x,y))
		    ptmp += display_underwater(x, y, TRUE);
		else
		    ptmp += display_binventory(x, y, TRUE);
	    }
	    if (!ptmp) Your(E_J("probe reveals nothing.","探査には何も引っかからなかった。"));
	    return TRUE;	/* we've done our own bhitpile */
	case WAN_OPENING:
	case SPE_KNOCK:
	    /* up or down, but at closed portcullis only */
	    if (is_db_wall(x,y) && find_drawbridge(&xx, &yy)) {
		open_drawbridge(xx, yy);
		disclose = TRUE;
	    } else if (u.dz > 0 && (x == xdnstair && y == ydnstair) &&
			/* can't use the stairs down to quest level 2 until
			   leader "unlocks" them; give feedback if you try */
			on_level(&u.uz, &qstart_level) && !ok_to_quest()) {
		pline_The(E_J("stairs seem to ripple momentarily.",
			      "一瞬、階段に波紋が走ったように見えた。"));
		disclose = TRUE;
	    }
	    break;
	case WAN_STRIKING:
	case SPE_FORCE_BOLT:
	    striking = TRUE;
	    /*FALLTHRU*/
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	    /* down at open bridge or up or down at open portcullis */
	    if ((levl[x][y].typ == DRAWBRIDGE_DOWN) ? (u.dz > 0) :
			(is_drawbridge_wall(x,y) && !is_db_wall(x,y)) &&
		    find_drawbridge(&xx, &yy)) {
		if (!striking)
		    close_drawbridge(xx, yy);
		else
		    destroy_drawbridge(xx, yy);
		disclose = TRUE;
	    } else if (striking && u.dz < 0 && rn2(3) &&
			!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) &&
			!Underwater && !Is_qstart(&u.uz)) {
		/* similar to zap_dig() */
		pline(E_J("A rock is dislodged from the %s and falls on your %s.",
			  "岩のかけらが%sから外れ、あなたの%sめがけて落ちてきた。"),
		      ceiling(x, y), body_part(HEAD));
		losehp(rnd((uarmh && is_metallic(uarmh)) ? 2 : 6),
		       E_J("falling rock","落石で"), KILLED_BY_AN);
		if ((otmp = mksobj_at(ROCK, x, y, FALSE, FALSE)) != 0) {
		    (void)xname(otmp);	/* set dknown, maybe bknown */
		    stackobj(otmp);
		}
		newsym(x, y);
	    } else if (!striking && ttmp && ttmp->ttyp == TRAPDOOR && u.dz > 0) {
		if (!Blind) {
			if (ttmp->tseen) {
				pline(E_J("A trap door beneath you closes up then vanishes.",
					  "あなたの下にある落とし扉が閉じ、消えた。"));
				disclose = TRUE;
			} else {
#ifndef JP
				You("see a swirl of %s beneath you.",
					is_ice(x,y) ? "frost" : "dust");
#else
				Your("足下で%sが渦巻いた。",
					is_ice(x,y) ? "霜" : "ほこり");
#endif /*JP*/
			}
		} else {
			You_hear(E_J("a twang followed by a thud.",
				     "糸が張り、続いて勢いよく何かがぶつかる音を"));
		}
		deltrap(ttmp);
		ttmp = (struct trap *)0;
		newsym(x, y);
	    }
	    break;
	case SPE_STONE_TO_FLESH:
	    if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) ||
		     Underwater || (Is_qstart(&u.uz) && u.dz < 0)) {
		pline(nothing_happens);
	    } else if (u.dz < 0) {	/* we should do more... */
		pline(E_J("Blood drips on your %s.",
			  "血があなたの%sに滴ってきた。"), body_part(FACE));
	    } else if (u.dz > 0 && !OBJ_AT(u.ux, u.uy)) {
		/*
		Print this message only if there wasn't an engraving
		affected here.  If water or ice, act like waterlevel case.
		*/
		e = engr_at(u.ux, u.uy);
		if (!(e && e->engr_type == ENGRAVE)) {
		    if (is_pool(u.ux, u.uy) || is_ice(u.ux, u.uy))
			pline(nothing_happens);
		    else
#ifndef JP
			pline("Blood %ss %s your %s.",
			      is_lava(u.ux, u.uy) ? "boil" : "pool",
			      Levitation ? "beneath" : "at",
			      makeplural(body_part(FOOT)));
#else
			Your("%s%s下%sた。",
			      body_part(FOOT), Levitation ? "の" : "",
			      is_lava(u.ux, u.uy) ? "で血が煮えたぎっ" : "血溜まりができ");
#endif /*JP*/
		}
	    }
	    break;
	case WAN_FIRE:
	case SPE_FIREBALL:
	    setup_zapinfo(&zi, obj->otyp == WAN_FIRE ? AT_NONE : AT_MAGC, AD_FIRE,
			       1, 6, (const char *)0, (const char *)0, &youmonst);
	    zap_over_floor(x, y, &zi, 0);
	    spoteffects(FALSE);
	    break;
	case WAN_COLD:
	case SPE_CONE_OF_COLD:
	    setup_zapinfo(&zi, obj->otyp == WAN_COLD ? AT_NONE : AT_MAGC, AD_COLD,
			       1, 6, (const char *)0, (const char *)0, &youmonst);
	    zap_over_floor(x, y, &zi, 0);
	    spoteffects(FALSE);
	    break;
	default:
	    break;
	}

	if (u.dz > 0) {
	    /* zapping downward */
	    (void) bhitpile(obj, bhito, x, y);

	    /* subset of engraving effects; none sets `disclose' */
	    if ((e = engr_at(x, y)) != 0 && e->engr_type != HEADSTONE) {
		switch (obj->otyp) {
		case WAN_POLYMORPH:
		case SPE_POLYMORPH:
		    del_engr(e);
		    make_engr_at(x, y, random_engraving(buf), moves, (xchar)0);
		    break;
		case WAN_CANCELLATION:
		case SPE_CANCELLATION:
		case WAN_MAKE_INVISIBLE:
		    del_engr(e);
		    break;
		case WAN_TELEPORTATION:
		case SPE_TELEPORT_AWAY:
		    rloc_engr(e);
		    break;
		case SPE_STONE_TO_FLESH:
		    if (e->engr_type == ENGRAVE) {
			/* only affects things in stone */
			pline_The(Hallucination ?
			    E_J("floor runs like butter!","床がバターのように溶けた！") :
			    E_J("edges on the floor get smoother.","床の角部分が滑らかになった。"));
			wipe_engr_at(x, y, d(2,4));
			}
		    break;
		case WAN_STRIKING:
		case SPE_FORCE_BOLT:
		    wipe_engr_at(x, y, d(2,4));
		    break;
		default:
		    break;
		}
	    }
	}

	return disclose;
}


/* called for various wand and spell effects - M. Stephenson */
void
weffects(obj)
register struct	obj	*obj;
{
	int otyp = obj->otyp;
	boolean disclose = FALSE, was_unkn = !objects[otyp].oc_name_known;
	struct zapinfo zi;

	exercise(A_WIS, TRUE);
#ifdef STEED
	if (u.usteed && (objects[otyp].oc_dir != NODIR) &&
	    !u.dx && !u.dy && (u.dz > 0) && zap_steed(obj)) {
		disclose = TRUE;
	} else
#endif
	if (otyp == WAN_NOTHING && obj->corpsenm != 0) {
	    setup_zapinfo(&zi, AT_NONE, obj->corpsenm, (obj->corpsenm == AD_PLYS) ? 1 : 6, 6,
			       (const char *)0, (const char *)0, /* use default names */
			       &youmonst);
	    buzz(&zi, u.ux, u.uy, u.dx, u.dy);
	    if(obj->spe == 0) {
		obj->corpsenm = 0; /* revert to normal wand of nothing */
		update_inventory();
	    }
	} else if (objects[otyp].oc_dir == IMMEDIATE) {
	    obj_zapped = FALSE;

	    if (u.uswallow) {
		(void) bhitm(u.ustuck, obj);
		/* [how about `bhitpile(u.ustuck->minvent)' effect?] */
	    } else if (u.dz) {
		disclose = zap_updown(obj);
	    } else {
		(void) bhit(u.dx,u.dy, rn1(8,6),ZAPPED_WAND, bhitm,bhito, obj);
	    }
	    /* give a clue if obj_zapped */
	    if (obj_zapped)
		You_feel(E_J("shuddering vibrations.","激しい振動を感じた。"));

	} else if (objects[otyp].oc_dir == NODIR) {
	    zapnodir(obj);

	} else {
	    /* neither immediate nor directionless */

	    if (otyp == WAN_DIGGING || otyp == SPE_DIG)
		zap_dig();
	    else if (otyp == SPE_FIREBALL) {
		(void) bhit(u.dx, u.dy, rn1(7,7), THROWN_SPELL + AD_FIRE - 1,
			    (int FDECL((*),(MONST_P,OBJ_P)))0,
			    (int FDECL((*),(OBJ_P,OBJ_P)))0,
			    (struct obj *)0);
		setup_zapinfo(&zi, AT_MAGC, AD_FIRE, u.ulevel / 2 + 1, 6, 0, 0, &youmonst);
		explode(bhitpos.x, bhitpos.y, &zi, d(12,6), EXPL_FIERY);
	    } else if (otyp >= SPE_MAGIC_MISSILE && otyp <= SPE_FINGER_OF_DEATH) {
		uchar adtyp = (uchar)(otyp - SPE_MAGIC_MISSILE + 1);
		if (otyp == SPE_FINGER_OF_DEATH) adtyp = AD_DETH;
		setup_zapinfo(&zi, AT_MAGC, adtyp, u.ulevel / 2 + 1, 6,
				   (const char *)0, (const char *)0, /* use default names */
				   &youmonst);
		buzz(&zi, u.ux, u.uy, u.dx, u.dy);
	    } else if (objects[otyp].oc_dir == RAY) {
		setup_zapobj(&zi, obj, &youmonst);
		buzz(&zi, u.ux, u.uy, u.dx, u.dy);
	    } else
		impossible("weffects: unexpected spell or wand");
	    disclose = TRUE;
	}
	if (disclose && was_unkn) {
	    makeknown(otyp);
	    more_experienced(0,10);
	}
	return;
}

/*
 * Generate the to damage bonus for a spell. Based on the hero's intelligence
 */
int
spell_damage_bonus()
{
    int tmp, intell = ACURR(A_INT);

    /* Punish low intellegence before low level else low intellegence
       gets punished only when high level */
    if (intell < 10)
	tmp = -3;
    else if (u.ulevel < 5)
	tmp = 0;
    else if (intell < 14)
	tmp = 0;
    else if (intell <= 18)
	tmp = 1;
    else		/* helm of brilliance */
	tmp = 2;

    return tmp;
}

/*
 * Generate the to hit bonus for a spell.  Based on the hero's skill in
 * spell class and dexterity.
 */
STATIC_OVL int
spell_hit_bonus(skill)
int skill;
{
    int hit_bon = 0;
    int dex = ACURR(A_DEX);

    schar hbon[11] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% */
		-4, -2, -2, -1, -1,  0,  0, +1, +2, +2, +3
    };
    hit_bon = hbon[P_SKILL(spell_skilltype(skill))/10];

    if (dex < 4)
	hit_bon -= 3;
    else if (dex < 6)
	hit_bon -= 2;
    else if (dex < 8)
	hit_bon -= 1;
    else if (dex < 14)
	hit_bon -= 0;		/* Will change when print stuff below removed */
    else
	hit_bon += dex - 14; /* Even increment for dextrous heroes (see weapon.c abon) */

    return hit_bon;
}

const char *
exclam(force)
register int force;
{
	/* force == 0 occurs e.g. with sleep ray */
	/* note that large force is usual with wands so that !! would
		require information about hand/weapon/wand */
	return (const char *)((force < 0) ? "?" : (force <= 4) ? E_J(".","。") : E_J("!","！"));
}

void
hit(str,mtmp,force)
register const char *str;
register struct monst *mtmp;
register const char *force;		/* usually either "." or "!" */
{
	if((!cansee(bhitpos.x,bhitpos.y) && !canspotmon(mtmp) &&
	     !(u.uswallow && mtmp == u.ustuck))
	   || !flags.verbose)
#ifndef JP
	    pline("%s %s it.", The(str), vtense(str, "hit"));
	else pline("%s %s %s%s", The(str), vtense(str, "hit"),
		   mon_nam(mtmp), force);
#else
	    pline("%sが何かに命中した。", str);
	else pline("%sが%sに命中した%s", str, mon_nam(mtmp), force);
#endif /*JP*/
}

void
miss(str,mtmp)
register const char *str;
register struct monst *mtmp;
{
#ifndef JP
	pline("%s %s %s.", The(str), vtense(str, "miss"),
	      ((cansee(bhitpos.x,bhitpos.y) || canspotmon(mtmp))
	       && flags.verbose) ?
	      mon_nam(mtmp) : "it");
#else
	pline("%sが%sをかすめた。", str,
	      ((cansee(bhitpos.x,bhitpos.y) || canspotmon(mtmp))
	       && flags.verbose) ?
	      mon_nam(mtmp) : "何か");
#endif /*JP*/
}

/*
 *	Bresenham line algorithm
 */
int bresenham_init(bp, x1, y1, x2, y2)
struct bresenham *bp;
int x1, y1, x2, y2;
{
	int dx, dy;
	bp->x = x1;
	bp->y = y1;
	dx = x2 - x1;
	dy = y2 - y1;
	bp->vx = sgn(dx);
	bp->vy = sgn(dy);
	if (dx < 0) dx = -dx;
	if (dy < 0) dy = -dy;
	if (dx == 0 && dy == 0) return 0;	/* start pos == end pos */
	if (dx > dy) {
		bp->e = dx;
		bp->delta = dy * 2;
		bp->threshold = dx * 2;
		bp->linetyp = 0;
		bp->count = dx;
	} else {
		bp->e = dy;
		bp->delta = dx * 2;
		bp->threshold = dy * 2;
		bp->linetyp = 1;
		bp->count = dy;
	}
	if (dx == 0 || dy == 0 || dx == dy) {	/*  */
		bp->linetyp = 2;
		return 1;
	}
	return 1;
}

int bresenham_step(bp)
struct bresenham *bp;
{
	switch (bp->linetyp) {
	    case 0: /* width > height */
		bp->x += bp->vx;
		bp->e += bp->delta;
		if (bp->e >= bp->threshold) {
			bp->y += bp->vy;
			bp->e -= bp->threshold;
		}
		break;
	    case 1: /* width < height */
		bp->y += bp->vy;
		bp->e += bp->delta;
		if (bp->e >= bp->threshold) {
			bp->x += bp->vx;
			bp->e -= bp->threshold;
		}
		break;
	    case 2: /* special */
		bp->x += bp->vx;
		bp->y += bp->vy;
		break;
	}
	bp->count--;
	return (bp->count > 0);
}

void bresenham_back(bp)
struct bresenham *bp;
{
	switch (bp->linetyp) {
	    case 0: /* width > height */
		bp->x -= bp->vx;
		bp->e -= bp->delta;
		if (bp->e < 0) {
			bp->y -= bp->vy;
			bp->e += bp->threshold;
		}
		break;
	    case 1: /* width < height */
		bp->y -= bp->vy;
		bp->e -= bp->delta;
		if (bp->e < 0) {
			bp->x -= bp->vx;
			bp->e += bp->threshold;
		}
		break;
	    case 2: /* special */
		bp->x -= bp->vx;
		bp->y -= bp->vy;
		break;
	}
	bp->count++;
}

/*
 *  Called for the following distance effects:
 *	when a weapon is thrown (weapon == THROWN_WEAPON)
 *	when an object is kicked (KICKED_WEAPON)
 *	when an IMMEDIATE wand is zapped (ZAPPED_WAND)
 *	when a light beam is flashed (FLASHED_LIGHT)
 *	when a mirror is applied (INVIS_BEAM)
 *  A thrown/kicked object falls down at the end of its range or when a monster
 *  is hit.  The variable 'bhitpos' is set to the final position of the weapon
 *  thrown/zapped.  The ray of a wand may affect (by calling a provided
 *  function) several objects and monsters on its path.  The return value
 *  is the monster hit (weapon != ZAPPED_WAND), or a null monster pointer.
 *
 *  Check !u.uswallow before calling bhit().
 *  This function reveals the absence of a remembered invisible monster in
 *  necessary cases (throwing or kicking weapons).  The presence of a real
 *  one is revealed for a weapon, but if not a weapon is left up to fhitm().
 */
struct monst *
bhit(ddx,ddy,range,weapon,fhitm,fhito,obj)
register int ddx,ddy,range;		/* direction and range */
int weapon;				/* see values in hack.h */
int FDECL((*fhitm), (MONST_P, OBJ_P)),	/* fns called when mon/obj hit */
    FDECL((*fhito), (OBJ_P, OBJ_P));
struct obj *obj;			/* object tossed/used */
{
	register boolean shopdoor = FALSE;
	int sx, sy;

	if (weapon == KICKED_WEAPON) {
	    /* object starts one square in front of player */
	    sx = u.ux + ddx;
	    sy = u.uy + ddy;
	    range--;
	} else {
	    sx = u.ux;
	    sy = u.uy;
	}
	return bhitcore(sx,sy, ddx, ddy, range, weapon, fhitm, fhito, obj, TRUE);
}

struct monst *
bhitcore(startx,starty, ddx,ddy, range, weapon, fhitm,fhito, obj, youshoot)
int startx, starty;			/* starting position */
int ddx,ddy,range;			/* direction and range */
int weapon;				/* see values in hack.h */
int FDECL((*fhitm), (MONST_P, OBJ_P)),	/* fns called when mon/obj hit (return value: ) */
    FDECL((*fhito), (OBJ_P, OBJ_P));	/* 	return value -- 0:keep going 1:stop */
struct obj *obj;			/* object tossed/used */
boolean youshoot;			/* TRUE:you shot it */
{
	struct monst *mtmp;
	uchar typ;
	boolean shopdoor = FALSE, point_blank = TRUE;
	int x, y;
	int lx, ly;
	int dbx, dby;
	int glyphtype;

	bresenham_init(&bhitpos, startx, starty, startx+ddx, starty+ddy);

	glyphtype = (weapon & BHIT_GLYPHMASK);
	if (glyphtype == BHIT_GLYPH_FLASH) {
	    tmp_at(DISP_BEAM, cmap_to_glyph(S_flashbeam));
	} else if (glyphtype == BHIT_GLYPH_OBJ) {
	    tmp_at(DISP_FLASH, obj_to_glyph(obj));
	} else if (glyphtype >= BHIT_GLYPH_FIREBALL) {
	    tmp_at(DISP_FLASH, fireball_to_glyph(glyphtype-BHIT_GLYPH_FIREBALL));
	}

	while((range > 0) &&
	      (range*range > dist2(startx, starty, bhitpos.x, bhitpos.y))) {

	    lx = bhitpos.x;
	    ly = bhitpos.y;

	    bresenham_step(&bhitpos);
	    x = bhitpos.x;
	    y = bhitpos.y;

	    if(!isok(x, y)) {
		bresenham_back(&bhitpos);
		break;
	    }

	    if(obj && is_pick(obj) && inside_shop(x, y) &&
					   (mtmp = shkcatch(obj, x, y))) {
		tmp_at(DISP_END, 0);
		return(mtmp);
	    }

	    typ = levl[x][y].typ;

	    /* iron bars will block anything big enough */
	    if ((weapon & BHIT_OBJTHROWN) /* physical flying or sliding object */
	    		 && typ == IRONBARS &&
		    hits_bars(&obj, lx, ly,
			      point_blank ? 0 : !rn2(5), 1)) {
		/* caveat: obj might now be null... */
		bresenham_back(&bhitpos);
		break;
	    }

	    /* drawbridge handling */
	    dbx = x;
	    dby = y;
	    if ((weapon & BHIT_ZAPPEDWAND) && find_drawbridge(&dbx,&dby)) {
		switch (obj->otyp) {
		    case WAN_OPENING:
		    case SPE_KNOCK:
			if (is_db_wall(x,y)) {
			    if (cansee(dbx,dby) || cansee(x,y))
				makeknown(obj->otyp);
			    open_drawbridge(dbx,dby);
			}
			break;
		    case WAN_LOCKING:
		    case SPE_WIZARD_LOCK:
			if ((cansee(dbx,dby) || cansee(x,y))
			    && levl[dbx][dby].typ == DRAWBRIDGE_DOWN)
			    makeknown(obj->otyp);
			close_drawbridge(dbx,dby);
			break;
		    case WAN_STRIKING:
		    case SPE_FORCE_BOLT:
			if (typ != DRAWBRIDGE_UP)
			    destroy_drawbridge(dbx,dby);
			makeknown(obj->otyp);
			break;
		    default:
			break;
		}
	    }

	    if (!youshoot && x==u.ux && y==u.uy)
		 mtmp = &youmonst;
	    else mtmp = m_at(x, y);
	    if (mtmp && is_unicorn(mtmp->data) && !level.flags.noteleport &&
		(!(weapon & BHIT_OBJTHROWN) || obj->oclass != GEM_CLASS) &&
		!mtmp->msleeping && mtmp->mcanmove) {
		struct bresenham bb;
		bb = bhitpos;
		bresenham_step(&bb);
		if (evade_missile(mtmp, lx, ly, bb.x, bb.y)) mtmp = (struct monst *)0;
	    }
	    if (mtmp) {
#ifdef MONSTEED
		mtmp = mrider_or_msteed(mtmp, !rn2(3));
#endif
		/* hit a monster */
		notonhead = (x != mtmp->mx || y != mtmp->my);
		if (!((weapon & BHIT_PASSINVIS) && mtmp->minvis)) {
		    if (!fhitm) {
			/* no hitmon callback -- return the first monster hit */
			if ((weapon & BHIT_SENSEINVIS) &&
			    cansee(x, y) && !canspotmons(mtmp))
				map_invisible(x, y);
			if((weapon & BHIT_GLYPHMASK) != BHIT_GLYPH_NONE)
				tmp_at(DISP_END, 0);
			return(mtmp);
		    } else {
			/* hitmon callback -- call fhitm for every monster to hit */
			if ((*fhitm)(mtmp, obj)) break; /* stopped: exit the loop */
			range -= 3;			/* keep going */
		    }
		}

	    } else {
		/* did not hit a monster */
		if ((weapon & BHIT_ZAPPEDWAND) && obj->otyp == WAN_PROBING &&
		    glyph_is_invisible(levl[x][y].glyph)) {
		    unmap_object(x, y);
		    newsym(x, y);
		}
	    }
	    if(fhito) {
		if(bhitpile(obj,fhito,x,y))
		    range--;
#ifdef NOPOLYPILE
		if ((weapon & BHIT_ZAPPEDWAND) &&
		    (obj->otyp == WAN_POLYMORPH || obj->otyp == SPE_POLYMORPH))
		    range = 0;
#endif /*NOPOLYPILE*/
	    } else {
		/* hero tries to kick dropped gold out of the shop */
		if(weapon == KICKED_WEAPON &&
		      ((obj->oclass == COIN_CLASS &&
			 OBJ_AT(x, y)) ||
			    ship_object(obj, x, y,
					costly_spot(x, y)))) {
			tmp_at(DISP_END, 0);
			return (struct monst *)0;
		}
	    }

	    /* door handling(open/close/crash etc.) */
	    if((weapon & BHIT_ZAPPEDWAND) && (IS_DOOR(typ) || typ == SDOOR)) {
		switch (obj->otyp) {
		case WAN_OPENING:
		case WAN_LOCKING:
		case WAN_STRIKING:
		case SPE_KNOCK:
		case SPE_WIZARD_LOCK:
		case SPE_FORCE_BOLT:
		    if (doorlock(obj, x, y)) {
			if (cansee(x, y) ||
			    (obj->otyp == WAN_STRIKING))
			    makeknown(obj->otyp);
			if (levl[x][y].doormask == D_BROKEN
			    && *in_rooms(x, y, SHOPBASE)) {
			    shopdoor = TRUE;
			    add_damage(x, y, youshoot ? 400L : 0L);
			}
		    }
		    break;
		default:
		    break;
		}
	    }

	    /* blocked by door */
	    if(!ZAP_POS(typ) || closed_door(x, y)) {
		bresenham_back(&bhitpos);
		break;
	    }
	    /* 'I' present but no monster: erase */
	    /* do this before the tmp_at() */
	    if (!(weapon & BHIT_PASSINVIS) &&
		glyph_is_invisible(levl[x][y].glyph) &&
		cansee(x, y)) {
		unmap_object(x, y);
		newsym(x, y);
	    }
	    /* show the symbol flying */
	    if((weapon & BHIT_GLYPHMASK) != BHIT_GLYPH_NONE) {
		tmp_at(x, y);
		delay_output();
	    }
	    /* kicked objects fall in pools */
	    if((weapon & BHIT_FALLTOPOOL) &&
	       (is_pool(x, y) || is_lava(x, y)))
		    break;
#ifdef SINKS
	    /* physical objects fall onto sink */
	    if(IS_SINK(typ) && (weapon & BHIT_STOPATSINK))
		break;
#endif
	    /* physical objects hits a large statue */
	    if (weapon & BHIT_OBJTHROWN) {
		struct obj *otmp = level.objects[x][y];
		while (otmp) {
		    if (otmp->otyp == STATUE &&
			rn2(5) <= mons[otmp->corpsenm].msize) {
			if (cansee(x, y)) {
#ifndef JP
			    pline("%s hits %s.",
				  The(distant_name(obj, xname)), an(xname(otmp)));
#else
			    pline("%sは%sに命中した。",
				  distant_name(obj, xname), xname(otmp));
#endif /*JP*/
			    range = 0;
			    break;
			}
		    }
		    otmp = otmp->nexthere;
		}
	    }

	    /* limit range of ball so hero won't make an invalid move */
	    if ((weapon & BHIT_OBJTHROWN) && range > 0 &&
		obj->otyp == HEAVY_IRON_BALL) {
		struct obj *bobj;
		struct trap *t;
		if ((bobj = sobj_at(BOULDER, x, y)) != 0) {
		    if (cansee(x,y))
#ifndef JP
			pline("%s hits %s.",
			      The(distant_name(obj, xname)), an(xname(bobj)));
#else
			pline("%sが%sに命中した。",
			      distant_name(obj, xname), xname(bobj));
#endif /*JP*/
		    range = 0;
		} else if (obj == uball) {
		    if (!test_move(lx, ly, x - lx, y - ly, TEST_MOVE)) {
			/* nb: it didn't hit anything directly */
			if (cansee(x,y))
#ifndef JP
			    pline("%s jerks to an abrupt halt.",
				  The(distant_name(obj, xname))); /* lame */
#else
			    pline("%sは唐突に引き戻され、止まった。",
				  distant_name(obj, xname)); /* lame */
#endif /*JP*/
			range = 0;
		    } else if (In_sokoban(&u.uz) && (t = t_at(x, y)) != 0 &&
			       (t->ttyp == PIT || t->ttyp == SPIKED_PIT ||
				t->ttyp == HOLE || t->ttyp == TRAPDOOR)) {
			/* hero falls into the trap, so ball stops */
			range = 0;
		    }
		}
	    }

	    /* thrown/kicked missile has moved away from its starting spot */
	    point_blank = FALSE;	/* affects passing through iron bars */

	}

	if ((weapon & BHIT_GLYPHMASK) != BHIT_GLYPH_NONE) tmp_at(DISP_END, 0);

	if(shopdoor && youshoot)
	    pay_for_damage(E_J("destroy","扉を壊した"), FALSE);

	return (struct monst *)0;
}

/* boomerang: return 0->nohit &youmonst->youhit mtmp->objgone */
struct monst *
boomhit(dx, dy, boomerang)
int dx, dy;
struct obj *boomerang;
{
	register int i, ct;
	int boom = S_boomleft;	/* showsym[] index  */
	struct monst *mtmp;
	boolean obj_gone;

	bhitpos.x = u.ux;
	bhitpos.y = u.uy;

	for (i = 0; i < 8; i++) if (xdir[i] == dx && ydir[i] == dy) break;
	tmp_at(DISP_FLASH, cmap_to_glyph(boom));
	for (ct = 0; ct < 10; ct++) {
		if(i == 8) i = 0;
		boom = (boom == S_boomleft) ? S_boomright : S_boomleft;
		tmp_at(DISP_CHANGE, cmap_to_glyph(boom));/* change glyph */
		dx = xdir[i];
		dy = ydir[i];
		bhitpos.x += dx;
		bhitpos.y += dy;
		if(MON_AT(bhitpos.x, bhitpos.y)) {
			mtmp = m_at(bhitpos.x,bhitpos.y);
			m_respond(mtmp);
			notonhead = (bhitpos.x != mtmp->mx || bhitpos.y != mtmp->my);
			obj_gone = thitmonst(mtmp, boomerang);
			if (obj_gone || bhitflag) {
				tmp_at(DISP_END, 0);
				return (obj_gone) ? mtmp : (struct monst *)0;
			}
		}
		if(!ZAP_POS(levl[bhitpos.x][bhitpos.y].typ) ||
		   closed_door(bhitpos.x, bhitpos.y)) {
			bhitpos.x -= dx;
			bhitpos.y -= dy;
			break;
		}
		if(bhitpos.x == u.ux && bhitpos.y == u.uy) { /* ct == 9 */
			if(Fumbling || rn2(20) >= ACURR(A_DEX)) {
				/* we hit ourselves */
#ifdef JP
				bullet_killername(&youmonst, boomerang, 0);
#endif /*JP*/
				(void) thitu(10, rnd(10), (struct obj *)0,
					E_J("boomerang","ブーメラン"));
				break;
			} else {	/* we catch it */
				tmp_at(DISP_END, 0);
				You(E_J("skillfully catch the boomerang.",
					"ブーメランを巧みに受け止めた。"));
				return(&youmonst);
			}
		}
		tmp_at(bhitpos.x, bhitpos.y);
		delay_output();
		if(ct % 5 != 0) i++;
#ifdef SINKS
		if(IS_SINK(levl[bhitpos.x][bhitpos.y].typ))
			break;	/* boomerang falls on sink */
#endif
	}
	tmp_at(DISP_END, 0);	/* do not leave last symbol */
	return (struct monst *)0;
}

int
zhitm(mon, ztmp, ootmp)			/* returns damage to mon */
register struct monst *mon;
struct zapinfo *ztmp;
struct obj **ootmp;	/* to return worn armor for caller to disintegrate */
{
	register int tmp = 0;
	int nd = ztmp->damn;
	int dd = ztmp->damd;
	boolean sho_shieldeff = FALSE;
	boolean nosavingthrow = FALSE;
	boolean spellcaster = is_hero_spell(ztmp); /* maybe get a bonus! */

	*ootmp = (struct obj *)0;
	switch(ztmp->adtyp) {
	case AD_MAGM:
		if (resists_magm(mon)) {
		    sho_shieldeff = TRUE;
		    break;
		}
		tmp = d(nd,dd);
		if (spellcaster)
		    tmp += spell_damage_bonus();
#ifdef WIZ_PATCH_DEBUG
		if (spellcaster)
		    pline("Damage = %d + %d", tmp-spell_damage_bonus(),
			spell_damage_bonus());
#endif
		break;
	case AD_FIRE:
		if (resists_fire(mon)) {
		    sho_shieldeff = TRUE;
		    break;
		}
		tmp = d(nd,dd);
		if (resists_cold(mon)) tmp += 7;
		if (spellcaster)
		    tmp += spell_damage_bonus();
#ifdef WIZ_PATCH_DEBUG
		if (spellcaster)
		    pline("Damage = %d + %d",tmp-spell_damage_bonus(),
			spell_damage_bonus());
#endif
		if (burnarmor(mon)) {
		    if (!rn2(3)) (void)destroy_mitem(mon, POTION_CLASS, AD_FIRE);
		    if (!rn2(3)) (void)destroy_mitem(mon, SCROLL_CLASS, AD_FIRE);
		    if (!rn2(5)) (void)destroy_mitem(mon, SPBOOK_CLASS, AD_FIRE);
		}
		break;
	case AD_COLD:
		if (resists_cold(mon)) {
		    sho_shieldeff = TRUE;
		    break;
		}
		tmp = d(nd,dd);
		if (resists_fire(mon)) tmp += d(nd, (dd+1)/2);
		if (spellcaster)
		    tmp += spell_damage_bonus();
#ifdef WIZ_PATCH_DEBUG
		if (spellcaster)
		    pline("Damage = %d + %d", tmp-spell_damage_bonus(),
			spell_damage_bonus());
#endif
		if (!rn2(3)) (void)destroy_mitem(mon, POTION_CLASS, AD_COLD);
		break;
	case AD_SLEE:
		tmp = 0;
		(void)sleep_monst(mon, d(nd, 25), ztmp->oclass);
		break;
	case AD_PLYS:
		tmp = 0;
		if (resists_paraly(mon)) {
		    sho_shieldeff = TRUE;
		    break;
		}
		mon->mcanmove = 0;
		mon->mfrozen = d(nd, dd);
		break;
	case AD_DETH:	/* death */
		if(mon->mnum == PM_DEATH) {
		    mon->mhpmax += mon->mhpmax/2;
		    if (mon->mhpmax >= MAGIC_COOKIE)
			mon->mhpmax = MAGIC_COOKIE - 1;
		    mon->mhp = mon->mhpmax;
		    tmp = 0;
		    break;
		}
		if (vs_death_factor(mon->data) ||
		    resists_drli(mon) ||
		    resists_magm(mon)) {	/* similar to player */
		    sho_shieldeff = TRUE;
		    break;
		}
		nosavingthrow = TRUE; /* so they don't get saving throws */
		tmp = mon->mhp+1;
		break;
	case AD_DISN: {	/* disintegration */
		struct obj *otmp2;

		if (resists_disint(mon)) {
		    sho_shieldeff = TRUE;
		} else if (mon->misc_worn_check & W_ARMS) {
		    /* destroy shield; victim survives */
		    *ootmp = which_armor(mon, W_ARMS);
		} else if (mon->misc_worn_check & W_ARM) {
		    /* destroy body armor, also cloak if present */
		    *ootmp = which_armor(mon, W_ARM);
		    if ((otmp2 = which_armor(mon, W_ARMC)) != 0)
			m_useup(mon, otmp2);
		} else {
		    /* no body armor, victim dies; destroy cloak
		       and shirt now in case target gets life-saved */
		    tmp = MAGIC_COOKIE;
		    if ((otmp2 = which_armor(mon, W_ARMC)) != 0)
			m_useup(mon, otmp2);
		    if ((otmp2 = which_armor(mon, W_ARMU)) != 0)
			m_useup(mon, otmp2);
		    nosavingthrow = TRUE;	/* no saving throw wanted */
		}
		break;	/* not ordinary damage */
	}
	case AD_ELEC:
		if (resists_elec(mon)) {
		    sho_shieldeff = TRUE;
		    tmp = 0;
		    /* can still blind the monster */
		} else
		    tmp = d(nd,dd);
		if (spellcaster)
		    tmp += spell_damage_bonus();
#ifdef WIZ_PATCH_DEBUG
		if (spellcaster)
		    pline("Damage = %d + %d", tmp-spell_damage_bonus(),
			spell_damage_bonus());
#endif
		if (!resists_blnd(mon) &&
				!(ztmp->byyou && u.uswallow && mon == u.ustuck)) {
			register unsigned rnd_tmp = rnd(50);
			mon->mcansee = 0;
			if((mon->mblinded + rnd_tmp) > 127)
				mon->mblinded = 127;
			else mon->mblinded += rnd_tmp;
		}
		if (!rn2(3)) (void)destroy_mitem(mon, WAND_CLASS, AD_ELEC);
		/* not actually possible yet */
		if (!rn2(3)) (void)destroy_mitem(mon, RING_CLASS, AD_ELEC);
		break;
	case AD_DRST:
		if (resists_poison(mon)) {
		    sho_shieldeff = TRUE;
		    break;
		}
		tmp = d(nd,dd);
		break;
	case AD_ACID:
		if (resists_acid(mon)) {
		    sho_shieldeff = TRUE;
		    break;
		}
		tmp = d(nd,dd);
		if (!rn2(6)) erode_obj(MON_WEP(mon), TRUE, TRUE);
		if (!rn2(6)) erode_armor(mon, TRUE);
		break;
	}
	if (sho_shieldeff) shieldeff(mon->mx, mon->my);
	if (is_hero_spell(ztmp) && (Role_if(PM_KNIGHT) && u.uhave.questart))
	    tmp *= 2;
	if (tmp > 0 && ztmp->byyou && !nosavingthrow && resist(mon, ztmp->oclass, 0, NOTELL))
	    tmp /= 2;
	if (tmp < 0) tmp = 0;		/* don't allow negative damage */
#ifdef WIZ_PATCH_DEBUG
	pline("zapped monster hp = %d (= %d - %d)", mon->mhp-tmp,mon->mhp,tmp);
#endif
	mlosehp(mon, tmp);
	return(tmp);
}

void
zhitu(ztmp, fltxt, sx, sy)
struct zapinfo *ztmp;
const char *fltxt;
xchar sx, sy;
{
	int nd = ztmp->damn;
	int dd = ztmp->damd;
	int dam = 0;
	boolean not_effected = FALSE;
	char kbuf[BUFSZ], *kstr;
	int ksuf;

	/* default killer name */
	kstr = (char *)fltxt;
	ksuf = E_J(KILLED_BY_AN,KILLED_SUFFIX);
#ifdef JP
	if (ztmp->stdkiller && ztmp->zapper) {
	    if (ztmp->aatyp == AT_BREA) {
		if (ztmp->byyou) {
		    Sprintf(kbuf, "自分の吐いた%sで", flash_types[19+ztmp->adtyp]);
		    kstr = kbuf;
		    ksuf = KILLED_BY_AN; /* 死んだ */
		} else {
		    /* breathed by a monster */
		    setup_killername(ztmp->zapper, kbuf);
		    Sprintf(eos(kbuf), "に%sで", flash_types[19+ztmp->adtyp]);
		    kstr = kbuf;
		}
	    } else if (ztmp->aatyp == AT_MAGC) {
		if (ztmp->byyou) {
		    Sprintf(kbuf, "自分の撃った%sで", flash_types[19+ztmp->adtyp]);
		    kstr = kbuf;
		    ksuf = KILLED_BY_AN; /* 死んだ */
		} else {
		    /* breathed by a monster */
		    setup_killername(ztmp->zapper, kbuf);
		    Sprintf(eos(kbuf), "の撃った%sで", flash_types[19+ztmp->adtyp]);
		    kstr = kbuf;
		}
	    } else {
		int otyp;
		otyp = zapinfo2otyp(ztmp);
		if (ztmp->byyou) {
		    Sprintf(kbuf, "自分の撃った");
		    ksuf = KILLED_BY_AN; /* 死んだ */
		} else {
		    setup_killername(ztmp->zapper, kbuf);
		    Sprintf(eos(kbuf), "に");
		}
		Sprintf(eos(kbuf), "%s%sで", JOBJ_NAME(objects[otyp]),
			ztmp->oclass == WAND_CLASS ? "杖" : "");
		kstr = kbuf;
	    }
	} else {
	    Sprintf(kbuf, "%sで", fltxt);
	}
#endif /*JP*/

	if (!nd) nd = 1;
	if (!dd) dd = 6;
	switch (ztmp->adtyp) {
	case AD_MAGM:
	    if (Antimagic) {
		shieldeff(sx, sy);
		pline_The(E_J("missiles bounce off!","矢は跳ね返された！"));
		damage_resistant_obj(ANTIMAGIC, 1);
	    } else {
		dam = d(nd, dd);
		exercise(A_STR, FALSE);
	    }
	    break;
	case AD_FIRE:
	    dam = d(nd, dd);
	    if (Fire_resistance) {
		if (not_effected = is_full_resist(FIRE_RES)) {
		    shieldeff(sx, sy);
		    You(E_J("don't feel hot!","熱さを感じない！"));
		    dam = 0;
		} else {
		    You(E_J("endure the heat.","熱に耐えている。"));
		    dam = (dam+3) / 4;
		}
		ugolemeffects(AD_FIRE, d(nd, dd));
	    }
	    burn_away_slime();
	    if (!not_effected && burnarmor(&youmonst)) {	/* "body hit" */
		if (!rn2(3)) destroy_item(POTION_CLASS, AD_FIRE);
		if (!rn2(3)) destroy_item(SCROLL_CLASS, AD_FIRE);
		if (!rn2(5)) destroy_item(SPBOOK_CLASS, AD_FIRE);
		if (!rn2(3)) destroy_item(TOOL_CLASS, AD_FIRE);
	    }
	    break;
	case AD_COLD:
	    dam = d(nd, dd);
	    if (Cold_resistance) {
		if (not_effected = is_full_resist(COLD_RES)) {
		    shieldeff(sx, sy);
		    You(E_J("don't feel cold.","寒さを感じない。"));
		    dam = 0;
		} else {
		    You(E_J("feel slightly cold.","寒さに耐えている。"));
		    dam = (dam+3) / 4;
		}
		ugolemeffects(AD_COLD, d(nd, dd));
	    }
	    if (!not_effected && !rn2(3))
		destroy_item(POTION_CLASS, AD_COLD);
	    break;
	case AD_SLEE:
	    if (Sleep_resistance) {
		shieldeff(u.ux, u.uy);
		You(E_J("don't feel sleepy.","眠くならない。"));
	    } else {
		fall_asleep(-d(nd,25), TRUE); /* sleep ray */
	    }
	    break;
	case AD_PLYS:
	    if (Free_action) {
		shieldeff(u.ux, u.uy);
		You(E_J("stiffen momentarily.","一瞬動けなくなった。"));
	    } else {
		You(E_J("are frozen!","麻痺した！"));
		nomul(-d(nd, 4));
		nomovemsg = You_can_move_again;
		exercise(A_DEX, FALSE);
	    }
	    break;
	case AD_DETH:
	case AD_DISN:
	    if (ztmp->adtyp == AD_DISN) {
		if (is_full_resist(DISINT_RES)) {
		    shieldeff(sx, sy);
		    You(E_J("aren't affected.","影響を受けない。"));
		    break;
		} else if (uarms) {
		    /* destroy shield; other possessions are safe */
		    (void) destroy_arm(uarms);
		    break;
		} else if (uarm) {
		    /* destroy suit; if present, cloak goes too */
		    if (uarmc) (void) destroy_arm(uarmc);
		    (void) destroy_arm(uarm);
		    break;
		}
		/* no shield or suit, you're dead; wipe out cloak
		   and/or shirt in case of life-saving or bones */
		if (uarmc) (void) destroy_arm(uarmc);
		if (uarmu) (void) destroy_arm(uarmu);
		if (Disint_resistance) {
		    You(E_J("are not disintegrated.","分解されなかった。"));
		    break;
		}
	    } else if (vs_death_factor(youmonst.data)) {
		shieldeff(sx, sy);
		You(E_J("seem unaffected.","影響を受けないようだ。"));
		break;
	    } else if (Antimagic || Drain_resistance) {
		shieldeff(sx, sy);
		You(E_J("aren't affected.","影響を受けない。"));
		if (Drain_resistance) break;
		damage_resistant_obj(ANTIMAGIC, rnd(3));
		break;
	    }
#ifndef JP
	    killer_format = E_J(KILLED_BY_AN,KILLED_SUFFIX);
	    killer = fltxt;
#else
	    killer_format = ksuf;
	    killer = kstr;
#endif
	    /* when killed by disintegration breath, don't leave corpse */
	    u.ugrave_arise = (ztmp->adtyp == AD_DISN) ? -3 : NON_PM;
	    done(DIED);
	    return; /* lifesaved */
	case AD_ELEC:
	    dam = d(nd, dd);
	    if (Shock_resistance) {
		if (not_effected = is_full_resist(SHOCK_RES)) {
		    shieldeff(sx, sy);
		    You(E_J("aren't affected.","影響を受けない。"));
		    dam = 0;
		} else {
		    You(E_J("nearly resist the shock.","電撃に耐えている。"));
		    dam = (dam+3) / 4;
		}
		ugolemeffects(AD_ELEC, d(nd, dd));
	    } else {
		exercise(A_CON, FALSE);
	    }
	    if (!not_effected) {
		if (!rn2(3)) destroy_item(WAND_CLASS, AD_ELEC);
		if (!rn2(3)) destroy_item(RING_CLASS, AD_ELEC);
	    }
	    break;
	case AD_DRST:
	    poisoned("blast", A_DEX, E_J("poisoned blast","毒のブレスで殺された"), 15);
	    if (!is_full_resist(POISON_RES)) {
		if (!rn2(3)) hurtarmor(AD_DCAY);
	    }
	    break;
	case AD_ACID:
	    if (Acid_resistance) {
		dam = 0;
	    } else {
		pline_The(E_J("acid burns!","身体が灼ける！"));
		dam = d(nd, dd);
		exercise(A_STR, FALSE);
	    }
	    if (!is_full_resist(ACID_RES)) {
		/* using two weapons at once makes both of them more vulnerable */
		if (!rn2(u.twoweap ? 3 : 6)) erode_obj(uwep, TRUE, TRUE);
		if (u.twoweap && !rn2(3)) erode_obj(uswapwep, TRUE, TRUE);
		if (!rn2(6)) erode_armor(&youmonst, TRUE);
	    }
	    break;
	}

	if (Half_spell_damage && dam && !ztmp->byyou && ztmp->aatyp == AT_MAGC)
	    dam = (dam + 1) / 2;
	losehp(dam, kstr, ksuf);
	return;
}


/*
 * burn scrolls and spellbooks on floor at position x,y
 * return the number of scrolls and spellbooks burned
 */
int
burn_floor_paper(x, y, give_feedback, u_caused)
int x, y;
boolean give_feedback;	/* caller needs to decide about visibility checks */
boolean u_caused;
{
	struct obj *obj, *obj2;
	long i, scrquan, delquan;
	char buf1[BUFSZ], buf2[BUFSZ];
	int cnt = 0;

	for (obj = level.objects[x][y]; obj; obj = obj2) {
	    obj2 = obj->nexthere;
	    if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS ||
		obj->otyp == SHEAF_OF_STRAW) {
		if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL ||
			obj_resists(obj, 2, 100))
		    continue;
		scrquan = obj->quan;	/* number present */
		delquan = 0;		/* number to destroy */
		for (i = scrquan; i > 0; i--)
		    if (!rn2(3)) delquan++;
		if (delquan) {
		    /* save name before potential delobj() */
		    if (give_feedback) {
#ifndef JP
			obj->quan = 1;
			Strcpy(buf1, (x == u.ux && y == u.uy) ?
				xname(obj) : distant_name(obj, xname));
			obj->quan = 2;
		    	Strcpy(buf2, (x == u.ux && y == u.uy) ?
				xname(obj) : distant_name(obj, xname));
			obj->quan = scrquan;
#else
			Strcpy(buf1, (x == u.ux && y == u.uy) ?
				xname(obj) : distant_name(obj, xname));
		    	Strcpy(buf2, jjosushi(obj));
#endif /*JP*/
		    }
		    /* useupf(), which charges, only if hero caused damage */
		    if (u_caused) useupf(obj, delquan);
		    else if (delquan < scrquan) obj->quan -= delquan;
		    else delobj(obj);
		    cnt += delquan;
		    if (give_feedback) {
			if (delquan > 1)
#ifndef JP
			    pline("%ld %s burn.", delquan, buf1, buf2);
#else
			    pline("%ld%sの%sが燃えた。", delquan, buf2);
#endif /*JP*/
			else
#ifndef JP
			    pline("%s burns.", An(buf1));
#else
			    pline("%sが燃えた。", buf1);
#endif /*JP*/
		    }
		}
	    } else if (obj->otyp == STATUE && get_material(obj) == LIQUID) {
		struct trap *trap;
		if ((trap = t_at(x,y)) && trap->ttyp == STATUE_TRAP) {
		    activate_statue_trap(trap, x, y, TRUE);
		    continue;
		}
		if (give_feedback) {
#ifndef JP
		    pline("%s thaws.", An(xname(obj)));
#else
		    pline("%sは解けた。", xname(obj));
#endif /*JP*/
		}
		if (can_be_frozen(&mons[obj->corpsenm]))
		    (void) mkcorpstat(CORPSE, (struct monst *) 0,
				      &mons[obj->corpsenm], x, y, TRUE);
		if (u_caused) useupf(obj, 1);
		else delobj(obj);
	    }
	}
	return cnt;
}

/* will zap/spell/breath attack score a hit against armor class `ac'? */
STATIC_OVL int
zap_hit(ac, type)
int ac;
int type;	/* either hero cast spell type or 0 */
{
    int chance = rn2(20);
    int spell_bonus = type ? spell_hit_bonus(type) : 0;

    /* small chance for naked target to avoid being hit */
    if (!chance) return rnd(10) < ac+spell_bonus;

    /* very high armor protection does not achieve invulnerability */
    ac = AC_VALUE(ac);

    return (3 - chance) < ac+spell_bonus;
}

/* 012
   3 4
   567 */
STATIC_OVL int
zap_to_glyph2(x1,y1,x2,y2,x3,y3,beam_type)
int x1,y1,x2,y2,x3,y3,beam_type;
{
    static const char z2g[64] = {
	S_lslant,   S_lslant,   S_rbeam_tc, S_lslant,   S_hbeamu,   S_rbeam_ml, S_vbeaml,   S_lslant,	/* 0 */
	S_lslant,   S_vbeam,    S_rslant,   S_rbeam_tl, S_rbeam_tr, S_vbeaml,   S_vbeam,    S_vbeamr,	/* 1 */
	S_rbeam_tc, S_rslant,   S_rslant,   S_hbeamu,   S_rslant,   S_rslant,   S_vbeamr,   S_rbeam_mr,	/* 2 */
	S_lslant,   S_rbeam_tl, S_hbeamu,   S_hbeam,    S_hbeam,    S_rslant,   S_rbeam_bl, S_hbeamd,	/* 3 */
	S_hbeamu,   S_rbeam_tr, S_rslant,   S_hbeam,    S_hbeam,    S_hbeamd,   S_rbeam_br, S_lslant,	/* 4 */
	S_rbeam_ml, S_vbeaml,   S_rslant,   S_rslant,   S_hbeamd,   S_rslant,   S_rslant,   S_rbeam_bc,	/* 5 */
	S_vbeaml,   S_vbeam,    S_vbeamr,   S_rbeam_bl, S_rbeam_br, S_rslant,   S_vbeam,    S_lslant,	/* 6 */
	S_lslant,   S_vbeamr,   S_rbeam_mr, S_hbeamd,   S_lslant,   S_rbeam_bc, S_lslant,   S_lslant	/* 7 */
    };
	int d1, d2;
	d1 = 4 + (sgn(y1-y2)*3) + sgn(x1-x2);
	if (d1 >= 4) d1--;
	d2 = 4 + (sgn(y3-y2)*3) + sgn(x3-x2);
	if (d2 >= 4) d2--;
	return (beam_type << 4) + (z2g[(d1 << 3) + d2] - S_vbeam) +
		GLYPH_ZAP_OFF + GLYPH_TYPE_OTHERS;
}

/* called with dx = dy = 0 with vertical bolts */
void
buzz(ztmp,sx,sy,dx,dy)
struct zapinfo *ztmp;
xchar sx,sy;
int dx,dy;
{
    int range;
    struct bresenham save_bhitpos;
    boolean shopdamage = FALSE;
    register const char *fltxt;
    struct obj *otmp;

    fltxt = ztmp->fltxt;
    if(u.uswallow) {
	register int tmp;

	if(!ztmp->byyou) return;
	tmp = zhitm(u.ustuck, ztmp, &otmp);
	if(!u.ustuck)	u.uswallow = 0;
	else	pline(E_J("%s rips into %s%s", "%sが%sを貫いた%s"),
		      E_J(The(fltxt), fltxt), mon_nam(u.ustuck), exclam(tmp));
	/* Using disintegration from the inside only makes a hole... */
	if (tmp == MAGIC_COOKIE)
	    u.ustuck->mhp = 0;
	if (u.ustuck->mhp < 1)
	    killed(u.ustuck);
	return;
    }
    if(!ztmp->byyou) newsym(u.ux,u.uy);
    range = rn1(7,7);
    save_bhitpos = bhitpos;
    bresenham_init(&bhitpos, sx, sy, sx+dx, sy+dy);
    range *= bhitpos.threshold;
    if(dx == 0 && dy == 0) {
	if (u.dz > 0) zap_over_floor(sx, sy, ztmp, &shopdamage);
	return;
    }

    tmp_at(DISP_BEAM, zapdir_to_glyph(dx, dy, ztmp->beam_type));

    while(range > 0) {
	buzzcore(ztmp, &range, &shopdamage, TRUE);
    }

    tmp_at(DISP_END,0);
    if (shopdamage)
	pay_for_damage(ztmp->adtyp == AD_FIRE ? E_J("burn away",   "扉を燃やした") :
		       ztmp->adtyp == AD_COLD ? E_J("shatter",	   "扉を砕いた") :
		       ztmp->adtyp == AD_DISN ? E_J("disintegrate","扉を分解した") :
						E_J("destroy",	   "扉を壊した"), FALSE);
    bhitpos = save_bhitpos;
}

/* Core function for buzz(). bhitpos needs to be set */
STATIC_OVL void
buzzcore(ztmp, rangep, shopdmgp, dodelay)
struct zapinfo *ztmp;
int *rangep;
boolean *shopdmgp;
boolean dodelay;
{
	struct rm *lev;
	xchar sx,  sy;	/* current coord of the ray */
	xchar lsx, lsy;	/* previous coord of the ray */
	xchar nx,  ny;	/* next coord of the ray */
	struct bresenham bb;
	struct obj *otmp;
	struct monst *mon;
	const char *fltxt = ztmp->fltxt;
	int range = *rangep;
	boolean blocked = FALSE;

	/* quasi-distance: ((longer side) + (shorter side)/2) */
	range -= (bhitpos.threshold + (bhitpos.delta / 2));

	lsx = bhitpos.x;
	lsy = bhitpos.y;
	bresenham_step(&bhitpos);
	sx = bhitpos.x;
	sy = bhitpos.y;
	/* get next pos */
	bb = bhitpos;
	bresenham_step(&bb);
	nx = bb.x;
	ny = bb.y;
	if(isok(sx,sy) && (lev = &levl[sx][sy])->typ) {
	    mon = m_at(sx, sy);
	    if (mon && is_unicorn(mon->data) && !level.flags.noteleport &&
		!mon->msleeping && mon->mcanmove) {
		if (evade_missile(mon, lsx, lsy, nx, ny)) mon = (struct monst *)0;
	    }
	    if(cansee(sx,sy)) {
		/* reveal/unreveal invisible monsters before tmp_at() */
		if (mon && !canspotmons(mon))
		    map_invisible(sx, sy);
		else if (!mon && glyph_is_invisible(levl[sx][sy].glyph)) {
		    unmap_object(sx, sy);
		    newsym(sx, sy);
		}
		if(ZAP_POS(lev->typ) || cansee(lsx,lsy)) {
		    tmp_at(DISP_CHANGE, zap_to_glyph2(lsx, lsy, sx, sy, nx, ny, ztmp->beam_type));
		    tmp_at(sx,sy);
		}
		if (dodelay) delay_output(); /* wait a little */
	    }
#ifdef MONSTEED
	    if (mon) mon = mrider_or_msteed(mon, !rn2(3));
#endif
	} else
	    goto make_bounce;

	/* hit() and miss() need bhitpos to match the target */
	/*bhitpos.x = sx,  bhitpos.y = sy;*/
	range += zap_over_floor(sx, sy, ztmp, shopdmgp) * bhitpos.threshold;

	if (mon) {
	    if (ztmp->byyou) mon->mstrategy &= ~STRAT_WAITMASK;
#ifdef STEED
	    buzzmonst:
#endif
	    if (zap_hit(find_mac(mon), is_hero_spell(ztmp) ?
			SPE_MAGIC_MISSILE + ztmp->adtyp - 1 : 0)) {
		blocked = elem_hits_shield(mon, ztmp->adtyp, fltxt);
		if (blocked) goto skip_hit;
		if (mon_reflects(mon, (char *)0)) {
		    if(cansee(mon->mx,mon->my)) {
			hit(fltxt, mon, exclam(0));
			shieldeff(mon->mx, mon->my);
			(void) mon_reflects(mon, E_J("But it reflects from %s %s!",
						     "だが、それは%s%sで反射した！"));
		    }
		    bhitpos.vx = -bhitpos.vx;
		    bhitpos.vy = -bhitpos.vy;
		} else {
		    boolean mon_could_move = mon->mcanmove;
		    int tmp = zhitm(mon, ztmp, &otmp);

		    if (is_rider(mon->data) && ztmp->adtyp == AD_DISN) {
			if (canseemon(mon)) {
			    hit(fltxt, mon, E_J(".","。"));
			    pline(E_J("%s disintegrates.","%sは分解された。"), Monnam(mon));
			    pline(E_J("%s body reintegrates before your %s!",
				      "%s身体があなたの%sの前で再合成されてゆく！"),
				  s_suffix(Monnam(mon)),
				  E_J((eyecount(youmonst.data) == 1) ?
				  	body_part(EYE) : makeplural(body_part(EYE)),
				      body_part(EYE)));
			    pline(E_J("%s resurrects!","%sは復活した！"), Monnam(mon));
			}
			mon->mhp = mon->mhpmax;
			blocked = TRUE;
			goto skip_hit;
		    }
		    if (mon->mnum == PM_DEATH && ztmp->adtyp == AD_DETH) {
			if (canseemon(mon)) {
			    hit(fltxt, mon, E_J(".","。"));
			    pline(E_J("%s absorbs the deadly %s!",
				      "%sは致死の%sを吸収している！"),
				      Monnam(mon), ztmp->fstxt);
			    pline(E_J("It seems even stronger than before.",
				      "そして、前よりも強大になったように見える。"));
			}
			blocked = TRUE;
			goto skip_hit;
		    }

		    if (tmp == MAGIC_COOKIE) { /* disintegration */
			struct obj *otmp2, *m_amulet = mlifesaver(mon);

			if (canseemon(mon)) {
			    if (!m_amulet)
				pline(E_J("%s is disintegrated!",
					  "%sは分解された！"), Monnam(mon));
			    else
				hit(fltxt, mon, E_J("!","！"));
			}
			mon->mgold = 0L;

/* note: worn amulet of life saving must be preserved in order to operate */
#define oresist_disintegration(obj) \
		(objects[obj->otyp].oc_oprop == DISINT_RES || \
		 obj_resists(obj, 5, 50) || is_quest_artifact(obj) || \
		 obj == m_amulet)

			for (otmp = mon->minvent; otmp; otmp = otmp2) {
			    otmp2 = otmp->nobj;
			    if (!oresist_disintegration(otmp)) {
				obj_extract_self(otmp);
				obfree(otmp, (struct obj *)0);
			    }
			}

			if (!ztmp->byyou)
			    monkilled(mon, (char *)0, -AD_RBRE);
			else
			    xkilled(mon, 2);
		    } else if(mon->mhp < 1) {
			if(!ztmp->byyou)
			    monkilled(mon, fltxt, AD_RBRE);
			else
			    killed(mon);
		    } else {
			if (!otmp) {
			    /* normal non-fatal hit */
			    hit(fltxt, mon, exclam(tmp));
			} else {
			    /* some armor was destroyed; no damage done */
			    if (canseemon(mon))
				pline(E_J("%s %s is disintegrated!",
					  "%s%sは分解された！"),
				      s_suffix(Monnam(mon)),
				      distant_name(otmp, xname));
			    m_useup(mon, otmp);
			}
			if (mon_could_move && !mon->mcanmove)	/* ZT_SLEEP */
			    slept_monst(mon);
		    }
		}
		range -= 2 * bhitpos.threshold;
	    } else {
		miss(fltxt,mon);
	    }
	} else if (sx == u.ux && sy == u.uy && range >= 0) {
	    nomul(0);
#ifdef STEED
	    if (u.usteed && !rn2(3) && !mon_reflects(u.usteed, (char *)0)) {
		    mon = u.usteed;
		    goto buzzmonst;
	    } else
#endif
	    if (zap_hit((int) u.uac, 0)) {
		range -= 2 * bhitpos.threshold;
		blocked = elem_hits_shield(&youmonst, ztmp->adtyp, fltxt);
		if (blocked) goto skip_hit;
#ifndef JP
		pline("%s hits you!", The(fltxt));
#else
		pline("%sがあなたに命中した！", fltxt);
#endif /*JP*/
		if (Reflecting) {
		    int objdmg = 0;
		    if (!Blind) {
#ifndef JP
		    	(void) ureflects("But %s reflects from your %s!", "it");
#else
		    	(void) ureflects("だが、%sはあなたの%sで反射した！", ztmp->fstxt);
#endif /*JP*/
		    } else
			pline(E_J("For some reason you are not affected.",
				  "なぜか、あなたは影響を受けなかった。"));
		    /*dx = -dx;*/
		    /*dy = -dy;*/
		    bhitpos.vx = -bhitpos.vx;
		    bhitpos.vy = -bhitpos.vy;
		    shieldeff(sx, sy);
		    switch (ztmp->adtyp) {	/* reflector gets damaged */
			case AD_MAGM:
			case AD_SLEE:
			case AD_DRST:
			case AD_PLYS:
				objdmg = d(1,4);
				break;
			case AD_FIRE:
			case AD_COLD:
			case AD_ELEC:
			case AD_ACID:
				objdmg = d(2,4);
				break;
			case AD_DETH:
				objdmg = d(3,4);
				break;
			case AD_DISN:
				objdmg = rn1(10, 10);
				break;
			default:
				break;
		    }
		    damage_resistant_obj(REFLECTING, objdmg);
		} else {
		    zhitu(ztmp, fltxt, sx, sy);
		}
	    } else {
#ifndef JP
		pline("%s whizzes by you!", The(fltxt));
#else
		pline("%sがあなたをかすめた！", fltxt);
#endif /*JP*/
	    }
	    if (ztmp->adtyp == AD_ELEC && !resists_blnd(&youmonst) &&
		!is_full_resist(SHOCK_RES) && !blocked) {
		You(are_blinded_by_the_flash);
		make_blinded((long)d(ztmp->damn,50),FALSE);
		if (!Blind) Your(vision_clears);
	    }
	    stop_occupation();
	    nomul(0);
	}
skip_hit:
	if (blocked) range = 0;

	if(lev->typ == TREE && ztmp->adtyp == AD_DETH) {
	    lev->typ = DEADTREE;
	    if (cansee(sx,sy)) {
		pline(E_J("The tree withers!","木は枯れた！"));
		newsym(sx,sy);
	    }
	    range = 0;
	} else if (lev->typ == IRONBARS && ztmp->adtyp == AD_ACID) {
	    lev->typ = ROOM;
	    if (cansee(sx,sy)) {
		pline(E_J("The set of iron bars dissolves!","鉄格子は溶けた！"));
		newsym(sx,sy);
	    }
	}

	if(range > 0 && (!ZAP_POS(lev->typ) || closed_door(sx, sy))) {
	    int bounce;
	    uchar rmn;

 make_bounce:
	    bounce = 0;
	    range -= bhitpos.threshold;
	    if((range > 0) && isok(sx, sy) && cansee(sx, sy))
#ifndef JP
		pline("%s bounces!", The(fltxt));
#else
		pline("%sは跳ね返った！", fltxt);
#endif /*JP*/
	    if(!bhitpos.vx || !bhitpos.vy || !rn2(20)) {
		bhitpos.vx = -bhitpos.vx;
		bhitpos.vy = -bhitpos.vy;
	    } else {
		int tx, ty;
		/* test vertical bounce... */
		bb = bhitpos;
		bb.vy = -bb.vy;
		bresenham_step(&bb);
		tx = bb.x;
		ty = bb.y;
		if (sx != tx && sy != ty) {
		    if(isok(sx,lsy) && ZAP_POS(rmn = levl[sx][lsy].typ) &&
		       !closed_door(sx,lsy) &&
		       (IS_ROOM(rmn) || (isok(tx,ty) &&
					 ZAP_POS(levl[tx][ty].typ))))
			bounce = 1;
		} else if (sx == tx) {
		    if(isok(tx,ty) && ZAP_POS(levl[tx][ty].typ) &&
		       !closed_door(tx,ty))
		        bounce = 1;
		} else { /* sy == ty */
		    if(isok(sx,lsy) && ZAP_POS(rmn = levl[sx][lsy].typ) &&
		       !closed_door(sx,lsy) &&
		       (IS_ROOM(rmn) || (isok(tx,ty+bb.vy) &&
					 ZAP_POS(levl[tx][ty+bb.vy].typ))))
			bounce = 1+4;
		}
		/* test horizontal bounce... */
		bb = bhitpos;
		bb.vx = -bb.vx;
		bresenham_step(&bb);
		tx = bb.x;
		ty = bb.y;
		if (sx != tx && sy != ty) {
		    if(isok(lsx,sy) && ZAP_POS(rmn = levl[lsx][sy].typ) &&
		       !closed_door(lsx,sy) &&
		       (IS_ROOM(rmn) || (isok(tx,ty) &&
					 ZAP_POS(levl[tx][ty].typ))))
			if(!bounce || rn2(2))
			    bounce = 2;
		} else if (sy == ty) {
		    if(isok(tx,ty) && ZAP_POS(levl[tx][ty].typ) &&
		       !closed_door(tx,ty))
			if(!bounce || rn2(2))
			    bounce = 2;
		} else { /* sx == tx */
		    if(isok(lsx,sy) && ZAP_POS(rmn = levl[lsx][sy].typ) &&
		       !closed_door(lsx,sy) &&
		       (IS_ROOM(rmn) || (isok(tx+bb.vx,ty) &&
					 ZAP_POS(levl[tx+bb.vx][ty].typ))))
			if(!bounce || rn2(2))
			    bounce = 2+4;
		}
		if (bounce >= 4) {
		    bhitpos.e = bhitpos.threshold - 1;	/* force going diagonal */
		    bounce &= 3;
		}

		switch(bounce) {
		case 0: bhitpos.vx = -bhitpos.vx; /* fall into... */
		case 1: bhitpos.vy = -bhitpos.vy; break;
		case 2: bhitpos.vx = -bhitpos.vx; break;
		}

		if(isok(sx,sy) && cansee(sx,sy)) {
		    bb = bhitpos;
		    bresenham_step(&bb);
		    tmp_at(DISP_CHANGE, zap_to_glyph2(lsx, lsy, sx, sy, bb.x, bb.y, ztmp->beam_type));
		    tmp_at(sx,sy);
		}
	    }
	}
	*rangep = range;
}

/* Chromatic Dragon's breath */
void
buzz_chromatic(mon,dx,dy,nd)
struct monst *mon;
int dx,dy;
int nd;
{
    xchar sx, sy;
    int range[5], adtyp[10];
    int c, s;
    int i, j, tmp;
    int ddx, ddy;
    struct zapinfo zi[5];
    struct bresenham bb[5];
    struct bresenham save_bhitpos;
    boolean shopdamage = FALSE;
    boolean doloop = TRUE;
    register const char *fltxt;
    struct obj *otmp;

    if (!mon) return;

    if (mon == &youmonst) {
	sx = u.ux;
	sy = u.uy;
    } else {
	sx = mon->mx;
	sy = mon->my;
    }

    for (i=0; i<10; i++) adtyp[i] = AD_MAGM + i;
    for (i=0; i<5; i++) {
	j = rnd(9-i) + i;
	tmp = adtyp[j];  adtyp[j] = adtyp[i]; adtyp[i] = tmp;
    }
    for (i=0; i<5; i++) {
	setup_zapinfo(&zi[i], AT_BREA, adtyp[i], nd, 6, 0, 0, mon);
	range[i] = rn1(7,7);
    }
    bresenham_init(&bb[4], sx, sy, sx+dx, sy+dy);
    range[4] *= bb[4].threshold;
    for (i=0; i<4; i++) {
	if (i & 2) { c = 250; s = 433; } /* cos(60),sin(60) */
	else	   { c = 433; s = 250; } /* cos(30),sin(30) */
	if (i & 1) s = -s;
	ddx = c * dx - s * dy;
	ddy = s * dx + c * dy;
	bresenham_init(&bb[i], sx, sy, sx+ddx, sy+ddy);
	range[i] *= bb[i].threshold;
    }
    save_bhitpos = bhitpos;

    tmp_at(DISP_BEAM, zapdir_to_glyph(dx, dy, zi[0].beam_type));

    while(doloop) {
	doloop = FALSE;
	for (i=0; i<5; i++) {
	    if (range[i] > 0) {
		bhitpos = bb[i];
		buzzcore(&zi[i], &range[i], &shopdamage, FALSE);
		bb[i] = bhitpos;
		doloop = TRUE;
	    }
	}
	delay_output(); /* wait a little */
    }

    tmp_at(DISP_END,0);
    bhitpos = save_bhitpos;
}


void
melt_ice(x, y)
xchar x, y;
{
	struct rm *lev = &levl[x][y];
	struct obj *otmp;

	if (lev->typ == DRAWBRIDGE_UP) {
	    lev->drawbridgemask &= ~DB_UNDER;
	    lev->drawbridgemask |= DB_MOAT;	/* revert to DB_MOAT */
	} else {	/* lev->typ == ICE */
#ifdef STUPID
	    if (lev->icedpool == ICED_POOL) lev->typ = POOL;
	    else if (lev->icedpool == ICED_MOAT) lev->typ = MOAT;
	    else lev->typ = BOG;
#else
	    lev->typ = (lev->icedpool == ICED_POOL ? POOL :
			lev->icedpool == ICED_MOAT ? MOAT : BOG);
#endif
	    lev->icedpool = 0;
	}
	obj_ice_effects(x, y, FALSE);
	unearth_objs(x, y);
	if (Underwater) vision_recalc(1);
	newsym(x,y);
	if (cansee(x,y)) Norep(E_J("The ice crackles and melts.",
				   "氷は音を立てて砕け、解けた。"));
	if ((otmp = sobj_at(BOULDER, x, y)) != 0) {
#ifndef JP
	    if (cansee(x,y)) pline("%s settles...", An(xname(otmp)));
#else
	    if (cansee(x,y)) pline("%sは足場を作った…。", xname(otmp));
#endif /*JP*/
	    do {
		obj_extract_self(otmp);	/* boulder isn't being pushed */
		if (!boulder_hits_pool(otmp, x, y, FALSE))
		    impossible("melt_ice: no pool?");
		/* try again if there's another boulder and pool didn't fill */
	    } while ((is_pool(x,y) || is_swamp(x,y)) && (otmp = sobj_at(BOULDER, x, y)) != 0);
	    newsym(x,y);
	}
	if (x == u.ux && y == u.uy)
		spoteffects(TRUE);	/* possibly drown, notice objects */
}

/* Burn floor scrolls, evaporate pools, etc...  in a single square.  Used
 * both for normal bolts of fire, cold, etc... and for fireballs.
 * Sets shopdamage to TRUE if a shop door is destroyed, and returns the
 * amount by which range is reduced (the latter is just ignored by fireballs)
 */
int
zap_over_floor(x, y, ztmp, shopdamage)
xchar x, y;
struct zapinfo *ztmp;
boolean *shopdamage;
{
	struct monst *mon;
	struct rm *lev = &levl[x][y];
	int rangemod = 0;

	if(ztmp->adtyp == AD_FIRE) {
	    struct trap *t = t_at(x, y);

	    if (t && t->ttyp == WEB) {
		/* a burning web is too flimsy to notice if you can't see it */
		if (cansee(x,y)) Norep(E_J("A web bursts into flames!",
					   "蜘蛛の巣は炎に包まれた！"));
		(void) delfloortrap(t);
		if (cansee(x,y)) newsym(x,y);
	    }
	    if(is_ice(x, y)) {
		melt_ice(x, y);
	    } else if(is_pool(x,y) || is_swamp(x,y)) {
		const char *msgtxt = E_J("You hear hissing gas.","あなたは蒸気の噴き出す音を聞いた。");
		schar filltyp;
		int dried = 0;
		if(lev->typ != POOL && lev->typ != BOG) {	/* MOAT or DRAWBRIDGE_UP */
		    if (cansee(x,y)) msgtxt = E_J("Some water evaporates.","水面に湯気が立った。");
		} else {
		    register struct trap *ttmp;

		    rangemod -= 3;
		    if (lev->typ == BOG) {
			lev->typ = GROUND;
			filltyp = fillholetyp(x,y);
			if (ACCESSIBLE(filltyp)) {
			    dried = 1;
			} else {
			    lev->typ = BOG;
			}
		    } else {
			lev->typ = ROOM;
			filltyp = fillholetyp(x,y);
			if (ACCESSIBLE(filltyp)) {
			    ttmp = maketrap(x, y, PIT);
			    if (ttmp) ttmp->tseen = 1;
			    dried = 1;
			} else {
			    lev->typ = filltyp;
			}
		    }
		    if (cansee(x,y))
			msgtxt = (dried) ? E_J("The water evaporates.", "水は沸き立ち、蒸気となった。") :
					   E_J("Some water evaporates.","水面に湯気が立った。");
		}
		Norep(msgtxt);
		if (dried) newsym(x,y);
	    } else if(IS_FOUNTAIN(lev->typ)) {
		    if (cansee(x,y))
			pline(E_J("Steam billows from the fountain.",
				  "泉から蒸気が噴き出した。"));
		    rangemod -= 1;
		    dryup(x, y, ztmp->byyou);
	    } else if(lev->typ == GRASS) {
		    if (cansee(x,y))
			Norep(E_J("The grass catches fire and burns away!",
				  "草地に火がつき、燃え尽きた！"));
		    rangemod -= 1;
		    lev->typ = GROUND;
		    newsym(x,y);
	    }
	}
	else if(ztmp->adtyp == AD_COLD && (is_pool(x,y) || is_lava(x,y) || is_swamp(x,y))) {
		boolean lava = is_lava(x,y);
		boolean swamp = is_swamp(x,y);
		boolean moat = (!lava && !swamp && (lev->typ != POOL) &&
				(lev->typ != WATER) &&
				!Is_medusa_level(&u.uz) &&
				!Is_waterlevel(&u.uz));

		if (lev->typ == WATER) {
		    /* For now, don't let WATER freeze. */
		    if (cansee(x,y))
			pline_The(E_J("water freezes for a moment.",
				      "水は一瞬凍りついた。"));
		    else
			You_hear(E_J("a soft crackling.","氷の弱い音を"));
		    rangemod -= 1000;	/* stop */
		} else {
		    rangemod -= 3;
		    if (lev->typ == DRAWBRIDGE_UP) {
			lev->drawbridgemask &= ~DB_UNDER;  /* clear lava */
			lev->drawbridgemask |= (lava ? DB_FLOOR : DB_ICE);
		    } else {
			if (!lava)
			    lev->icedpool =
				    (lev->typ == POOL ? ICED_POOL :
				     lev->typ == MOAT ? ICED_MOAT : ICED_BOG);
			lev->typ = (lava ? ROOM : ICE);
		    }
		    bury_objs(x,y);
		    if(cansee(x,y)) {
			if(moat)
				Norep(E_J("The moat is bridged with ice!",
					  "堀は氷で覆われた！"));
			else if(lava)
				Norep(E_J("The lava cools and solidifies.",
					  "溶岩は冷えて固まった。"));
			else
				Norep(E_J("The water freezes.","水は凍った。"));
			newsym(x,y);
		    } else if(flags.soundok && !lava)
			You_hear(E_J("a crackling sound.","何かが凍る音を"));

		    if (x == u.ux && y == u.uy) {
			if (u.uinwater) {   /* not just `if (Underwater)' */
			    /* leave the no longer existent water */
			    u.uinwater = 0;
			    u.uundetected = 0;
			    docrt();
			    vision_full_recalc = 1;
			} else if (u.utrap && u.utraptype == TT_LAVA) {
			    if (Passes_walls) {
				You(E_J("pass through the now-solid rock.",
					"新たに固まった岩の中を通っている。"));
			    } else {
				u.utrap = rn1(50,20);
				u.utraptype = TT_INFLOOR;
				You(E_J("are firmly stuck in the cooling rock.",
					"冷えた岩の中にはまり込んでいて動けない。"));
			    }
			}
		    } else if ((mon = m_at(x,y)) != 0) {
			/* probably ought to do some hefty damage to any
			   non-ice creature caught in freezing water;
			   at a minimum, eels are forced out of hiding */
			if (is_swimmer(mon->data) && mon->mundetected) {
			    mon->mundetected = 0;
			    newsym(x,y);
			}
		    }
		}
		obj_ice_effects(x,y,TRUE);
	} else if(ztmp->adtyp == AD_DETH && lev->typ == GRASS) {
	    lev->typ = GROUND;
	    if (cansee(x,y)) {
		Norep(E_J("The grass withers!","草はしなびて枯れた！"));
		newsym(x,y);
	    }
	    rangemod--;
	}

	if(closed_door(x, y)) {
		int new_doormask = -1;
		const char *see_txt = 0, *sense_txt = 0, *hear_txt = 0;
		rangemod = -1000;
		switch(ztmp->adtyp) {
		case AD_FIRE:
		    new_doormask = D_NODOOR;
		    see_txt = E_J("The door is consumed in flames!",
				  "扉は炎に包まれて燃え尽きた！");
		    sense_txt = E_J("smell smoke.","煙の匂いを嗅いだ。");
		    break;
		case AD_COLD:
		    new_doormask = D_NODOOR;
		    see_txt = E_J("The door freezes and shatters!",
				  "扉は凍りつき、こなごなに砕けた！");
		    sense_txt = E_J("feel cold.","涼しくなった。");
		    break;
		case AD_DISN:
		    new_doormask = D_NODOOR;
		    see_txt = E_J("The door disintegrates!",
				  "扉は分解された！");
		    hear_txt = E_J("crashing wood.","木の砕ける音を");
		    break;
		case AD_ELEC:
		    new_doormask = D_BROKEN;
		    see_txt = E_J("The door splinters!","扉は爆裂した！");
		    hear_txt = E_J("crackling.","破砕音を");
		    break;
		default:
		    if(cansee(x,y)) {
#ifndef JP
			pline_The("door absorbs %s %s!",
			      !ztmp->byyou ? "the" : "your",
			      ztmp->fstxt);
#else
			pline("扉は%sを吸収した！",
			      ztmp->aatyp == AT_MAGC ? "呪文" : ztmp->fstxt);
#endif /*JP*/
		    } else You_feel(E_J("vibrations.","振動を感じた。"));
		    break;
		}
		if (new_doormask >= 0) {	/* door gets broken */
		    if (*in_rooms(x, y, SHOPBASE)) {
			if (ztmp->byyou) {
			    add_damage(x, y, 400L);
			    if (shopdamage) *shopdamage = TRUE;
			} else	/* caused by monster */
			    add_damage(x, y, 0L);
		    }
		    lev->doormask = new_doormask;
		    unblock_point(x, y);	/* vision */
		    if (cansee(x, y)) {
			pline(see_txt);
			newsym(x, y);
		    } else if (sense_txt) {
			You(sense_txt);
		    } else if (hear_txt) {
			if (flags.soundok) You_hear(hear_txt);
		    }
		    if (picking_at(x, y)) {
			stop_occupation();
			reset_pick();
		    }
		}
	}

	if(OBJ_AT(x, y) && ztmp->adtyp == AD_FIRE)
		if (burn_floor_paper(x, y, FALSE, ztmp->byyou) && couldsee(x, y)) {
		    newsym(x,y);
#ifndef JP
		    You("%s of smoke.",
			!Blind ? "see a puff" : "smell a whiff");
#else
		    You(!Blind ? "煙が立ち昇るのを見た。" :
				 "かすかな煙の臭いを嗅いだ。");
#endif /*JP*/
		}
	if ((mon = m_at(x,y)) != 0) {
		/* Cannot use wakeup() which also angers the monster */
		mon->msleeping = 0;
		if(mon->m_ap_type) seemimic(mon);
		if(ztmp->byyou) {
		    setmangry(mon);
		    if(mon->ispriest && *in_rooms(mon->mx, mon->my, TEMPLE))
			ghod_hitsu(mon);
		    if(mon->isshk && !*u.ushops)
			hot_pursuit(mon);
		}
	}
	return rangemod;
}


void
fracture_rock(obj)	/* fractured by pick-axe or wand of striking */
register struct obj *obj;		   /* no texts here! */
{
	/* A little Sokoban guilt... */
	if (obj->otyp == BOULDER && In_sokoban(&u.uz) && !flags.mon_moving)
	    change_luck(-1);

	trans_to_rock(obj);
	if (obj->where == OBJ_FLOOR) {
		obj_extract_self(obj);		/* move rocks back on top */
		place_object(obj, obj->ox, obj->oy);
		if(!does_block(obj->ox,obj->oy,&levl[obj->ox][obj->oy]))
	    		unblock_point(obj->ox,obj->oy);
		if(cansee(obj->ox,obj->oy))
		    newsym(obj->ox,obj->oy);
	}
}

void
trans_to_rock(obj)
register struct obj *obj;
{
	obj->otyp   = (get_material(obj) == MINERAL) ? ROCK : LAST_GEM + 1;
	obj->oclass = GEM_CLASS;
	obj->quan   = (long) rn1(60, 7);
	obj->owt    = weight(obj);
	obj->known  = FALSE;
	obj->bknown = FALSE;
	del_xdat_obj(obj, 0);		/* no names, no extra data */
}

/* handle statue hit by striking/force bolt/pick-axe */
boolean
break_statue(obj)
register struct obj *obj;
{
	/* [obj is assumed to be on floor, so no get_obj_location() needed] */
	struct trap *trap = t_at(obj->ox, obj->oy);
	struct obj *item;
	int mat = get_material(obj);

	if (mat != MINERAL && mat != GLASS) return FALSE;	/* do not shatter */

	if (trap && trap->ttyp == STATUE_TRAP &&
		activate_statue_trap(trap, obj->ox, obj->oy, TRUE))
	    return FALSE;
	/* drop any objects contained inside the statue */
	while ((item = obj->cobj) != 0) {
	    obj_extract_self(item);
	    place_object(item, obj->ox, obj->oy);
	}
	if (Role_if(PM_ARCHEOLOGIST) && !flags.mon_moving && (obj->spe & STATUE_HISTORIC)) {
	    You_feel(E_J("guilty about damaging such a historic statue.",
			 "これほどの歴史ある彫像を破壊したことに対し、罪の意識を覚えた。"));
	    adjalign(-1);
	}
	obj->spe = 0;
	fracture_rock(obj);
	return TRUE;
}

const char * const destroy_strings[] = {	/* also used in trap.c */
#ifndef JP
	"freezes and shatters", "freeze and shatter", "shattered potion",
	"boils and explodes", "boil and explode", "boiling potion",
	"catches fire and burns", "catch fire and burn", "burning scroll",
	"catches fire and burns", "catch fire and burn", "burning book",
	"turns to dust and vanishes", "turn to dust and vanish", "",
	"breaks apart and explodes", "break apart and explode", "exploding wand"
#else
	"は凍りつき、割れた", "凍った薬の破裂で",	/* 日本語は各エントリにつきポインタ2個 */
	"は沸騰し、爆発した", "沸騰した薬の爆発で",
	"に火がつき、燃えた", "巻物の延焼で",
	"に火がつき、燃えた", "呪文書の延焼で",
	"は塵と化し、消滅した", "",
	"はばらばらに弾け、爆発した", "杖の爆発で"
#endif /*JP*/
};

void
destroy_item(osym, dmgtyp)
register int osym, dmgtyp;
{
	register struct obj *obj, *obj2;
	register int dmg, xresist, skip;
	register long i, cnt, quan;
	register int dindx;
	const char *mult;

	if (dmgtyp == AD_FIRE && is_full_resist(FIRE_RES)) return;
	if (dmgtyp == AD_COLD && is_full_resist(COLD_RES)) return;
	if (dmgtyp == AD_ELEC && is_full_resist(SHOCK_RES)) return;

	for(obj = invent; obj; obj = obj2) {
	    obj2 = obj->nobj;
	    if(obj->oclass != osym) continue; /* test only objs of type osym */
	    if(obj->oartifact) continue; /* don't destroy artifacts */
	    if(obj->in_use && obj->quan == 1) continue; /* not available */
	    xresist = skip = 0;
#ifdef GCC_WARN
	    dmg = dindx = 0;
	    quan = 0L;
#endif
	    switch(dmgtyp) {
		case AD_COLD:
		    if(osym == POTION_CLASS && obj->otyp != POT_OIL) {
			quan = obj->quan;
			dindx = 0;
			dmg = rnd(4);
		    } else skip++;
		    break;
		case AD_FIRE:
		    xresist = (Fire_resistance && obj->oclass != POTION_CLASS);

		    if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL)
			skip++;
		    if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
			skip++;
			if (!Blind)
#ifndef JP
			    pline("%s glows a strange %s, but remains intact.",
				The(xname(obj)), hcolor("dark red"));
#else
			    pline("%sは奇妙な%s輝いたが、全く傷つかなかった。",
				xname(obj), j_no_ni(hcolor("赤黒い色の")));
#endif /*JP*/
		    }
		    quan = obj->quan;
		    switch(osym) {
			case POTION_CLASS:
			    dindx = 1;
			    dmg = rnd(6);
			    break;
			case SCROLL_CLASS:
			    dindx = 2;
			    dmg = 1;
			    break;
			case SPBOOK_CLASS:
			    dindx = 3;
			    dmg = 1;
			    break;
			case TOOL_CLASS:
			case CHAIN_CLASS:
			    if (get_material(obj) == CLOTH ||
				get_material(obj) == VEGGY) {
				dindx = 3;
				dmg = 0;
				if (Is_container(obj) || !objects[obj->otyp].oc_merge) {
				    char buf[BUFSZ];
				    Sprintf(buf, xname(obj));
				    rust_dmg(obj, buf, 0, FALSE, &youmonst);
				    skip++;
				}
			    } else skip++;
			    break;
			default:
			    skip++;
			    break;
		    }
		    break;
		case AD_ELEC:
		    xresist = (Shock_resistance && obj->oclass != RING_CLASS);
		    quan = obj->quan;
		    switch(osym) {
			case RING_CLASS:
			    if(obj->otyp == RIN_SHOCK_RESISTANCE)
				    { skip++; break; }
			    dindx = 4;
			    dmg = 0;
			    break;
			case WAND_CLASS:
			    if(obj->otyp == WAN_LIGHTNING) { skip++; break; }
#if 0
			    if (obj == current_wand) { skip++; break; }
#endif
			    dindx = 5;
			    dmg = rnd(10);
			    break;
			default:
			    skip++;
			    break;
		    }
		    break;
		default:
		    skip++;
		    break;
	    }
	    if(!skip) {
		if (obj->in_use) --quan; /* one will be used up elsewhere */
		for(i = cnt = 0L; i < quan; i++)
		    if(!rn2(3)) cnt++;

		if(!cnt) continue;
#ifndef JP
		if(cnt == quan)	mult = "Your";
		else	mult = (cnt == 1L) ? "One of your" : "Some of your";
		pline("%s %s %s!", mult, xname(obj),
			(cnt > 1L) ? destroy_strings[dindx*3 + 1]
				  : destroy_strings[dindx*3]);
#else
		{ char buf[BUFSZ];
		Strcpy(buf, destroy_strings[dindx*2]);
		if(cnt == quan)	mult = "";
		else {
			mult = (cnt == 1L) ? "の一つ" : "のいくつか";
			if (dindx != 2 && dindx != 3)
				strncpy(buf, "が", 2);
		}
		pline("あなたの%s%s%s！", xname(obj), mult, buf);
		}
#endif /*JP*/
		if(osym == POTION_CLASS && dmgtyp != AD_COLD) {
		    if (!breathless(youmonst.data) || haseyes(youmonst.data))
		    	potionbreathe(obj);
		}
//		if (obj->owornmask) {
//		    if (obj->owornmask & W_RING) /* ring being worn */
//			Ring_gone(obj);
//		    else
//			setnotworn(obj);
//		}
		if (obj == current_wand) current_wand = 0;	/* destroyed */
		
		if (Is_container(obj)) {
		    destroy_container(obj, &youmonst);
		}
		for (i = 0; i < cnt; i++)
		    useup(obj);
		if(dmg) {
		    if(xresist)	You(E_J("aren't hurt!","傷つかない！"));
		    else {
			const char *how = destroy_strings[E_J(dindx * 3 + 2, dindx * 2 + 1)];
#ifndef JP
			boolean one = (cnt == 1L);

			losehp(dmg, one ? how : (const char *)makeplural(how),
			       one ? KILLED_BY_AN : KILLED_BY);
#else
			losehp(dmg, how, KILLED_BY);
#endif /*JP*/
			exercise(A_STR, FALSE);
		    }
		}
	    }
	}
	return;
}

void
destroy_container(obj, victim)
struct obj *obj;
struct monst *victim;
{
	xchar x, y;
	if (victim == &youmonst) {
	    x = u.ux;
	    y = u.uy;
	} else if (victim != 0) {
	    x = victim->mx;
	    x = victim->my;
	}
	if (obj->otyp == BAG_OF_TRICKS) {
	    if (obj->spe > 0 && create_critters(obj->spe, (struct permonst *)0)) {
		pline(E_J("Something drops out from the burning bag!",
			  "焼けた鞄から怪物があふれ出た！"));
	    }
	} else if (Is_container(obj) && Has_contents(obj)) {
	    struct obj *otmp;
	    long ccnt;
	    schar typ = levl[x][y].typ;
	    for (ccnt = 0, otmp = obj->cobj; otmp; otmp = otmp->nobj)
		ccnt += otmp->quan;
#ifndef JP
	    if (ccnt) pline_The("content%s of %s fall%s %sto the %s!",
				(ccnt > 1) ? "s" : "", the(xname(obj)),
				(ccnt == 1) ? "s" : "",
				(IS_SOFT(typ) || IS_FOUNTAIN(typ) ||
				 typ == LAVAPOOL) ? "in" : "",
				surface(x, y));
#else
	    if (ccnt) pline("%sの中身が%s%sに落ちた！",
				xname(obj), surface(x, y),
				(IS_SOFT(typ) || IS_FOUNTAIN(typ) ||
				 typ == LAVAPOOL) ? "の中" : "");
#endif /*JP*/
	    while (obj->cobj) {
		otmp = obj->cobj;
		obj_extract_self(otmp);
		place_object(otmp, x, y);
		stackobj(otmp);
	    }
	}
}

void
destroy_items(dmgtyp)
register int dmgtyp;
{
	switch (dmgtyp) {
	    case AD_FIRE:
		destroy_item(SCROLL_CLASS, AD_FIRE);
		destroy_item(POTION_CLASS, AD_FIRE);
		if (rn2(5) < 3) destroy_item(SPBOOK_CLASS, AD_FIRE);
		destroy_item(TOOL_CLASS, AD_FIRE);
		break;
	    case AD_COLD:
		destroy_item(POTION_CLASS, AD_COLD);
		break;
	    case AD_ELEC:
		destroy_item(WAND_CLASS, AD_ELEC);
		destroy_item(RING_CLASS, AD_ELEC);
		break;
	    default:
		break;
	}
}

int
destroy_mitem(mtmp, osym, dmgtyp)
struct monst *mtmp;
int osym, dmgtyp;
{
	struct obj *obj, *obj2;
	int skip, tmp = 0;
	long i, cnt, quan;
	int dindx;
	boolean vis;

	if (mtmp == &youmonst) {	/* this simplifies artifact_hit() */
	    destroy_item(osym, dmgtyp);
	    return 0;	/* arbitrary; value doesn't matter to artifact_hit() */
	}

	vis = canseemon(mtmp);
	for(obj = mtmp->minvent; obj; obj = obj2) {
	    obj2 = obj->nobj;
	    if(obj->oclass != osym) continue; /* test only objs of type osym */
	    skip = 0;
	    quan = 0L;
	    dindx = 0;

	    switch(dmgtyp) {
		case AD_COLD:
		    if(osym == POTION_CLASS && obj->otyp != POT_OIL) {
			quan = obj->quan;
			dindx = 0;
			tmp++;
		    } else skip++;
		    break;
		case AD_FIRE:
		    if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL)
			skip++;
		    if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
			skip++;
			if (vis)
#ifndef JP
			    pline("%s glows a strange %s, but remains intact.",
				The(distant_name(obj, xname)),
				hcolor("dark red"));
#else
			    pline("%sは奇妙な%s輝いたが、全く傷つかなかった。",
				xname(obj), j_no_ni(hcolor("赤黒い色の")));
#endif /*JP*/
		    }
		    quan = obj->quan;
		    switch(osym) {
			case POTION_CLASS:
			    dindx = 1;
			    tmp++;
			    break;
			case SCROLL_CLASS:
			    dindx = 2;
			    tmp++;
			    break;
			case SPBOOK_CLASS:
			    dindx = 3;
			    tmp++;
			    break;
			default:
			    skip++;
			    break;
		    }
		    break;
		case AD_ELEC:
		    quan = obj->quan;
		    switch(osym) {
			case RING_CLASS:
			    if(obj->otyp == RIN_SHOCK_RESISTANCE)
				    { skip++; break; }
			    dindx = 4;
			    break;
			case WAND_CLASS:
			    if(obj->otyp == WAN_LIGHTNING) { skip++; break; }
			    dindx = 5;
			    tmp++;
			    break;
			default:
			    skip++;
			    break;
		    }
		    break;
		default:
		    skip++;
		    break;
	    }
	    if(!skip) {
		for(i = cnt = 0L; i < quan; i++)
		    if(!rn2(3)) cnt++;

		if(!cnt) continue;
#ifndef JP
		if (vis) pline("%s %s %s!",
			s_suffix(Monnam(mtmp)), xname(obj),
			(cnt > 1L) ? destroy_strings[dindx*3 + 1]
				  : destroy_strings[dindx*3]);
#else
		if (vis) pline("%sの%s%s！",
			mon_nam(mtmp), xname(obj), destroy_strings[dindx*2]);
#endif /*JP*/
		for(i = 0; i < cnt; i++) m_useup(mtmp, obj);
	    }
	}
	return(tmp);
}


int
resist(mtmp, oclass, damage, tell)
struct monst *mtmp;
char oclass;
int damage, tell;
{
	int resisted;
	int alev, dlev;

	/* attack level */
	switch (oclass) {
	    case WAND_CLASS:	alev = 12;	 break;
	    case TOOL_CLASS:	alev = 10;	 break;	/* instrument */
	    case WEAPON_CLASS:	alev = 10;	 break;	/* artifact */
	    case SCROLL_CLASS:	alev =  9;	 break;
	    case POTION_CLASS:	alev =  6;	 break;
	    case RING_CLASS:	alev =  5;	 break;
	    default:		alev = u.ulevel; break;	/* spell */
	}
	/* defense level */
	dlev = (int)mtmp->m_lev;
	if (dlev > 50) dlev = 50;
	else if (dlev < 1) dlev = is_mplayer(mtmp->data) ? u.ulevel : 1;

	resisted = rn2(100 + alev - dlev) < mtmp->data->mr;
	if (resisted) {
	    if (tell) {
		shieldeff(mtmp->mx, mtmp->my);
		pline(E_J("%s resists!","%sは抵抗した！"), Monnam(mtmp));
	    }
	    damage = (damage + 1) / 2;
	}

	if (damage) {
	    mlosehp(mtmp, damage);
	    if (mtmp->mhp < 1) {
		if(m_using) monkilled(mtmp, "", AD_RBRE);
		else killed(mtmp);
	    }
	}
	return(resisted);
}

void
makewish()
{
	char buf[BUFSZ];
	struct obj *otmp, nothing;
	int tries = 0;

	nothing = zeroobj;  /* lint suppression; only its address matters */
	if (flags.verbose) You(E_J("may wish for an object.",
				   "望みのものを手に入れられる。"));
retry:
	getlin(E_J("For what do you wish?","何を願いますか？"), buf);
	if(buf[0] == '\033') buf[0] = 0;
	/*
	 *  Note: if they wished for and got a non-object successfully,
	 *  otmp == &zeroobj.  That includes gold, or an artifact that
	 *  has been denied.  Wishing for "nothing" requires a separate
	 *  value to remain distinct.
	 */
	otmp = readobjnam(buf, &nothing, TRUE);
	if (!otmp) {
	    pline(E_J("Nothing fitting that description exists in the game.",
		      "その記述に合致するような品物はゲーム中に存在しません。"));
	    if (++tries < 5) goto retry;
	    pline(thats_enough_tries);
	    otmp = readobjnam((char *)0, (struct obj *)0, TRUE);
	    if (!otmp) return;	/* for safety; should never happen */
	} else if (otmp == &nothing) {
	    /* explicitly wished for "nothing", presumeably attempting
	       to retain wishless conduct */
	    return;
	}

	/* KMH, conduct */
	u.uconduct.wishes++;

	if (otmp != &zeroobj) {
	    /* The(aobjnam()) is safe since otmp is unidentified -dlc */
#ifndef JP
	    (void) hold_another_object(otmp, u.uswallow ?
				       "Oops!  %s out of your reach!" :
				       (Is_airlevel(&u.uz) ||
					Is_waterlevel(&u.uz) ||
					levl[u.ux][u.uy].typ < IRONBARS ||
					levl[u.ux][u.uy].typ >= ICE) ?
				       "Oops!  %s away from you!" :
				       "Oops!  %s to the floor!",
				       The(aobjnam(otmp,
					     Is_airlevel(&u.uz) || u.uinwater ?
						   "slip" : "drop")),
				       (const char *)0);
#else
	    (void) hold_another_object(otmp, u.uswallow ?
				"しまった！ %sは手の届かない所へ行ってしまった！" :
				  (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) ||
				   levl[u.ux][u.uy].typ < IRONBARS ||
				   levl[u.ux][u.uy].typ >= ICE) ?
				    "しまった！ %sをつかみそこなった！" :
				    "しまった！ %sをとり落とした！",
				xname(otmp), (const char *)0);
#endif /*JP*/
	    u.ublesscnt += rn1(100,50);  /* the gods take notice */
	}
}


struct resiobj {
	long	mask;
	struct obj **objp;
	uchar	objclass;
	uchar	objsubc;
};

const struct resiobj resiobjtbl[] = {
	/* reversed order */
	{ W_ARMF,    &uarmf,	ARMOR_CLASS,  ARM_BOOTS  },
	{ W_ARMG,    &uarmg,	ARMOR_CLASS,  ARM_GLOVES },
	{ W_ARMH,    &uarmh,	ARMOR_CLASS,  ARM_HELM   },
	{ W_ARM,     &uarm,	ARMOR_CLASS,  ARM_SUIT   },
//	{ W_SWAPWEP, &uswapwep,	WEAPON_CLASS, 2          },
	{ W_WEP,     &uwep,	WEAPON_CLASS, 1          },
	{ W_ARMC,    &uarmc,	ARMOR_CLASS,  ARM_CLOAK  },
	{ W_ARMS,    &uarms,	ARMOR_CLASS,  ARM_SHIELD },
	{ W_RINGR,   &uright,	RING_CLASS,   2          },
	{ W_RINGL,   &uleft,	RING_CLASS,   1          },
	{ W_AMUL,    &uamul,	AMULET_CLASS, 0          },
	{ 0, 0, 0, 0 }
};

void
damage_resistant_obj(prop, damval)
uchar prop;
int damval;
{
	long e = u.uprops[prop].extrinsic;
	int oc1 = 0, oc2 = 0;
	int oldtmp, newtmp = 0;
	const struct resiobj *tbl;
	struct obj *otmp;

	if (e & (W_ART|W_ARTI)) return;	/* artifacts */
	if ((e & W_ARM) && uarm && uarm->otyp == CHROMATIC_DRAGON_SCALE_MAIL) return;

	for ( tbl = resiobjtbl; tbl->mask; tbl++ ) {
		if ( e & tbl->mask ) {
			if (!*(tbl->objp)) {
			    impossible("damage to null object?");
			    return;
			}
			/* artifacts do not get damaged */
			if ((*(tbl->objp))->oartifact) return;
			oc1 = tbl->objclass;
			oc2 = tbl->objsubc;
		}
	}
	if ( !oc1 ) return;	/* there is no object to get damaged */
	if ( oc1 == WEAPON_CLASS && oc2 == 2 && !u.twoweap ) return;	/* do not count unwield weapon */

	switch (oc1) {
	    case AMULET_CLASS:
		if (prop != ANTIMAGIC && prop != REFLECTING ) break;
		if (uamul) {
			if (uamul->oartifact) break;
			oldtmp = uamul->odamaged;
			newtmp = oldtmp + damval;
			if ( newtmp>100 ) {
				if (Blind) You_feel(E_J("your medallion breaks in pieces!",
							"メダリオンがこなごなに砕け散るのを感じた！"));
				else Your(E_J("medallion glows brightly, and breaks in pieces!",
					      "メダリオンは激しく輝くと、こなごなに砕け散った！"));
				otmp = uamul;
				Amulet_off();
				useup(otmp);
				break;
			} else if ( /*oldtmp<90 &&*/ newtmp>=90 )
				if (!Blind) Your(E_J("medallion gives a faint glimmer of light!",
						     "メダリオンが不規則に明滅した！"));
				else Your(E_J("medallion vibrates violently!",
					      "メダリオンが激しく振動した！"));
			else if ( oldtmp<75 && newtmp>=75 )
				Your(E_J("medallion vibrates unexpectedly.",
					 "メダリオンが不意に震えた。"));
			uamul->odamaged = newtmp;
		}
		break;
	    case ARMOR_CLASS:
		switch (oc2) {
		    case ARM_CLOAK:
			if (uarmc) {
				if (uarmc->oartifact) break;
				oldtmp = uarmc->odamaged;
				newtmp = oldtmp + damval;
				if ( newtmp>100 ) {
					if (Blind)
					    You_feel(E_J("your %s is gone away!",
							 "自分の%sが無くなったのを感じた！"),
							cloak_simple_name(uarmc));
					else
					    Your(E_J("%s crumbles and turns to dust!",
						     "%sはずたずたに千切れ、塵と化した！"),
						 cloak_simple_name(uarmc));
					otmp = uarmc;
	       				(void) Cloak_off();
					useup(otmp);
					break;
				} else if ( /*oldtmp<90 &&*/ newtmp>=90 )
					Your(E_J("%s is about to come apart!",
						 "%sはばらばらになりそうだ！"), cloak_simple_name(uarmc));
				else if ( oldtmp<75 && newtmp>=75 )
					Your(E_J("%s becomes slightly shabby.",
						 "%sはかなり擦り切れている。"), cloak_simple_name(uarmc));
				uarmc->odamaged = newtmp;
			}
			break;
		    case ARM_SHIELD:
			if (uarms && uarms->oartifact) break;
			if (uarms && prop == REFLECTING) {	/* shield of reflection */
				oldtmp = uarms->odamaged;
				newtmp = oldtmp + damval;
				if ( newtmp>100 ) {
					if (!Blind) Your(E_J("shield is completely tarnished.",
							     "盾は完全に曇ってしまった。"));
					else You_feel(E_J("your shield is somewhat helpless.",
							  "自分の盾がなんとなく頼りなくなったのを感じた。"));
					otmp = uarms;
					Shield_off();
					otmp->prevotyp = otmp->otyp;
					set_otyp(otmp, SHIELD);
					otmp->odamaged = 0;
					change_material(otmp, SILVER);	/* mere silver shield */
					setworn(otmp, W_ARMS);
					break;
				} else if ( /*oldtmp<90 &&*/ newtmp>=90 ) {
				    if (!Blind) Your(E_J("shield looks much duller!",
							 "盾は輝きを失いかけている！"));
				    else Your(E_J("shield shivers!","盾は震えた！"));
				} else if ( oldtmp<75 && newtmp>=75 ) {
				    if (!Blind) Your(E_J("shield looks slightly dull.",
							 "盾の輝きが鈍ってきたようだ。"));
				    else Your(E_J("shield shivers slightly.",
						  "盾がかすかに震えた。"));
				}
				uarms->odamaged = newtmp;
			}
			break;
		    case ARM_SUIT:
			if (uarm && uarm->oartifact) break;
			if (prop != ANTIMAGIC && prop != REFLECTING ) break;
			if (uarm && Is_dragon_armor(uarm)) {	/* gray or silver dragon */
				char buf[BUFSZ];
				int isdm = Is_dragon_mail(uarm);
				oldtmp = uarm->odamaged;
				newtmp = oldtmp + damval;
#ifndef JP
				Sprintf(buf, "dragon scale%s", isdm ? " mail" : "s");
#else
				Sprintf(buf, "ドラゴンの鱗%s", isdm ? "鎧" : "");
#endif /*JP*/
				if ( newtmp>100 ) {
				    if (isdm) {
					Your(E_J("%s seems to lose its power.",
						 "%sは力を失ったようだ。"), buf);
					otmp = uarm;
					Armor_off();
					otmp->prevotyp = otmp->otyp;
					set_otyp(otmp, PLAIN_DRAGON_SCALE_MAIL);	/* mere scale mail */
					otmp->odamaged = 0;
					setworn(otmp, W_ARM);
				    } else {
					Your(E_J("%s crumble and fall apart!",
						 "%sはばらばらになり、あたりに散らばった！"), buf);
					otmp = uarm;
					Armor_off();
					useup(otmp);
				    }
				    break;
				} else if ( /*oldtmp<90 &&*/ newtmp>=90 )
#ifndef JP
				    Your("%s shiver%s!", buf, isdm ? "s" : "");
#else
				    Your("%sは震えた！", buf);
#endif /*JP*/
				else if ( oldtmp<75 && newtmp>=75 )
#ifndef JP
				    Your("%s shiver%s slightly.", buf, isdm ? "s" : "");
#else
				    Your("%sはかすかに震えた。", buf);
#endif /*JP*/
				uarm->odamaged = newtmp;
			}
			break;
		    default:
			break;
		}
		break;
	    default:
		/*impossible("Damage to strange object???")*/;
	}
	if (wizard) pline("[%d%]", newtmp);
	return;
}

boolean 
does_obj_worn_out(otmp)
struct obj *otmp;
{
	if (otmp->oartifact) return FALSE;
	if (otmp->otyp == AMULET_OF_REFLECTION ||
	    otmp->otyp == SHIELD_OF_REFLECTION ||
	    otmp->otyp == CLOAK_OF_MAGIC_RESISTANCE ||
	    otmp->otyp == FRILLED_APRON ||
	    otmp->otyp == SILVER_DRAGON_SCALE_MAIL ||
	    otmp->otyp == SILVER_DRAGON_SCALES ||
	    otmp->otyp == GRAY_DRAGON_SCALE_MAIL ||
	    otmp->otyp == GRAY_DRAGON_SCALES ||
	    otmp->otyp == PLAIN_DRAGON_SCALE_MAIL)
		return TRUE;
	if (otmp->otyp == SHIELD && get_material(otmp) == SILVER)
		return TRUE;
	return FALSE;
}

int
zapinfo2otyp(ztmp)
struct zapinfo *ztmp;
{
	int otyp = STRANGE_OBJECT;
	switch (ztmp->oclass) {
	    case WAND_CLASS:
		if (ztmp->adtyp >= AD_MAGM && ztmp->adtyp <= AD_ELEC) {
		    otyp = (int)(ztmp->adtyp - AD_MAGM) + WAN_MAGIC_MISSILE;
		} else {
		    if (ztmp->adtyp == AD_DETH) otyp = WAN_DEATH;
		}
		break;
	    case TOOL_CLASS:
		switch (ztmp->adtyp) {
		    case AD_FIRE: otyp = FIRE_HORN; break;
		    case AD_COLD: otyp = FROST_HORN; break;
		    default: break;
		}
		break;
	    case SCROLL_CLASS:
		if (ztmp->adtyp == AD_FIRE) otyp = SCR_FIRE;
		break;
	    case POTION_CLASS:
		if (ztmp->adtyp == AD_FIRE) otyp = POT_OIL;
		break;
	    default:
		break;
	}
	return otyp;
}

/*zap.c*/
