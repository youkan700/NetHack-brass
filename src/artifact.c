/*	SCCS Id: @(#)artifact.c 3.4	2003/08/11	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"
#ifdef OVLB
#include "artilist.h"
#else
STATIC_DCL struct artifact artilist[];
#endif
/*
 * Note:  both artilist[] and artiexist[] have a dummy element #0,
 *	  so loops over them should normally start at #1.  The primary
 *	  exception is the save & restore code, which doesn't care about
 *	  the contents, just the total size.
 */

extern boolean notonhead;	/* for long worms */

#define get_artifact(o) \
		(((o)&&(o)->oartifact) ? &artilist[(int) (o)->oartifact] : 0)

STATIC_DCL int FDECL(spec_applies, (const struct artifact *,struct monst *));
STATIC_DCL int FDECL(arti_invoke, (struct obj*));
STATIC_DCL boolean FDECL(Mb_hit, (struct monst *magr,struct monst *mdef,
				  struct obj *,int *,int,BOOLEAN_P,char *));

#ifndef OVLB
STATIC_DCL int spec_dbon_applies;
STATIC_DCL xchar artidisco[NROFARTIFACTS];
#else	/* OVLB */
/* coordinate effects from spec_dbon() with messages in artifact_hit() */
STATIC_OVL int spec_dbon_applies = 0;

/* flags including which artifacts have already been created */
static boolean artiexist[1+NROFARTIFACTS+1];
/* and a discovery list for them (no dummy first entry here) */
STATIC_OVL xchar artidisco[NROFARTIFACTS];

STATIC_DCL void NDECL(hack_artifacts);
STATIC_DCL boolean FDECL(attacks, (int,struct obj *));
STATIC_DCL int FDECL(getobj_filter_invoke, (struct obj *));

/* handle some special cases; must be called after u_init() */
STATIC_OVL void
hack_artifacts()
{
	struct artifact *art;
	int alignmnt = aligns[flags.initalign].value;

	/* Fix up the alignments of "gift" artifacts */
	for (art = artilist+1; art->otyp; art++) {
	    /* artifacts for your role */
	    if (art->role == Role_switch) {
		art->alignment = alignmnt;
	    /* artifacts not for you */
	    } else if (art->alignment == A_CROSSALIGNED) {
		art->alignment = ((alignmnt+1) & 1); /* {-1,0,1} to {0,1,0} */
	    }
	}

	/* Excalibur can be used by any lawful character, not just knights */
	if (!Role_if(PM_KNIGHT))
	    artilist[ART_EXCALIBUR].role = NON_PM;

	/* Fix up the quest artifact */
	if (urole.questarti) {
	    artilist[urole.questarti].alignment = alignmnt;
	    artilist[urole.questarti].role = Role_switch;
	}
	return;
}

/* zero out the artifact existence list */
void
init_artifacts()
{
	(void) memset((genericptr_t) artiexist, 0, sizeof artiexist);
	(void) memset((genericptr_t) artidisco, 0, sizeof artidisco);
	hack_artifacts();
}

void
save_artifacts(fd)
int fd;
{
	bwrite(fd, (genericptr_t) artiexist, sizeof artiexist);
	bwrite(fd, (genericptr_t) artidisco, sizeof artidisco);
}

void
restore_artifacts(fd)
int fd;
{
	mread(fd, (genericptr_t) artiexist, sizeof artiexist);
	mread(fd, (genericptr_t) artidisco, sizeof artidisco);
	hack_artifacts();	/* redo non-saved special cases */
}

const char *
artiname(artinum)
int artinum;
{
	if (artinum <= 0 || artinum > NROFARTIFACTS) return("");
	return(E_J(artilist[artinum].name,
		   jartifact_names[artinum]));
}

/*
   Make an artifact.  If a specific alignment is specified, then an object of
   the appropriate alignment is created from scratch, or 0 is returned if
   none is available.  (If at least one aligned artifact has already been
   given, then unaligned ones also become eligible for this.)
   If no alignment is given, then 'otmp' is converted
   into an artifact of matching type, or returned as-is if that's not possible.
   For the 2nd case, caller should use ``obj = mk_artifact(obj, A_NONE);''
   for the 1st, ``obj = mk_artifact((struct obj *)0, some_alignment);''.
 */
struct obj *
mk_artifact(otmp, alignment)
struct obj *otmp;	/* existing object; ignored if alignment specified */
aligntyp alignment;	/* target alignment, or A_NONE */
{
	const struct artifact *a;
	int n, m, p;
	int maxprob = 0;
	int maxskill;
	boolean by_align = (alignment != A_NONE);
	short o_typ = (by_align || !otmp) ? 0 : otmp->otyp;
	boolean unique = !by_align && otmp && objects[o_typ].oc_unique;
	short eligible[NROFARTIFACTS];
	short prob[NROFARTIFACTS];

	/* gather eligible artifacts */
	for (n = 0, a = artilist+1, m = 1; a->otyp; a++, m++)
	    if ((!by_align ? a->otyp == o_typ :
		    (a->alignment == alignment ||
			(a->alignment == A_NONE && u.ugifts > 0))) &&
		(!(a->spfx & SPFX_NOGEN) || unique) && !artiexist[m]) {
		p = 100;
		if (by_align) {
		    /* skip enemies' equipment */
		    if (a->race != NON_PM && race_hostile(&mons[a->race])) continue;
		    /* skip artifact gauntlets if you are not a monk */
		    if (a->role == PM_MONK && !Role_if(PM_MONK)) continue;
		    /* class artifact weapon... 'a' points to the desired one */
		    if (Role_if(a->role)) p=10000;
		    /* unfamiliar weapons are hard to get */
		    else if (objects[a->otyp].oc_class == WEAPON_CLASS ||
			a->otyp == PICK_AXE || a->otyp == GRAPPLING_HOOK ||
			a->otyp == UNICORN_HORN) {
			maxskill = P_MAX_SKILL(objects[a->otyp].oc_skill);
			p = (maxskill < P_MINIMUM) ? P_MINIMUM : maxskill;
		    }
		    /* supress Sting/Orcrist */
//		    if (!(a->spfx & SPFX_RESTR)) p = 5;
		}
		prob[n] = p;
		eligible[n++] = m;
		maxprob += p;
	    }

	if (n) {		/* found at least one candidate */
	    for (n = 0, p = rnd(maxprob); (p -= prob[n]) > 0; n++);
	    m = eligible[n];	/* [0..n-1] */
	    a = &artilist[m];

	    /* make an appropriate object if necessary, then christen it */
	    if (by_align) otmp = mksobj((int)a->otyp, TRUE, FALSE);
//	    otmp = oname(otmp, E_J(a->name, jartifact_names[m]));
	    otmp->oartifact = m;
	    change_material(otmp, a->material);
	    artiexist[m] = TRUE;
	    if (m == ART_GRIMTOOTH) otmp->opoisoned = 1;
	} else {
	    /* nothing appropriate could be found; return the original object */
	    if (by_align) otmp = 0;	/* (there was no original object) */
	}
	return otmp;
}

/*
 * Make a given object to the specified artifact.
 * If otmp==0, create an appropriate object.
 */
struct obj *
create_artifact(otmp, artinum)
struct obj *otmp;
int artinum;
{
	if (!otmp)
	    otmp = mksobj((int)artilist[artinum].otyp, TRUE, FALSE);
	otmp->oartifact = artinum;
	otmp->age = 0;
	change_material(otmp, artilist[artinum].material);
	artiexist[artinum] = TRUE;
	if (otmp->oartifact) {
	    /* can't dual-wield with artifact as secondary weapon */
	    if (otmp == uswapwep) untwoweapon();
	    /* activate warning if you've just named your weapon "Sting" */
	    if (otmp == uwep) set_artifact_intrinsic(otmp, TRUE, W_WEP);
	}
	return otmp;
}

/*
 * Returns the full name (with articles and correct capitalization) of an
 * artifact named "name" if one exists, or NULL, it not.
 * The given name must be rather close to the real name for it to match.
 * The object type of the artifact is returned in otyp if the return value
 * is non-NULL.
 */
const char*
artifact_name(name, otyp, anum)
const char *name;
short *otyp;
int *anum;
{
    register const struct artifact *a;
    register const char *aname;
    int i;

    if(!strncmpi(name, "the ", 4)) name += 4;

    for (a = artilist+1, i = 1; a->otyp; a++, i++) {
	aname = a->name;
	if(!strncmpi(aname, "the ", 4)) aname += 4;
	if(!strcmpi(name, aname)) {
	    *otyp = a->otyp;
	    if (anum) *anum = i;
	    return E_J(a->name, jartifact_names[i]);
	}
#ifdef JP
	if(!strcmpi(name, jartifact_names[i])) {
	    *otyp = a->otyp;
	    if (anum) *anum = i;
	    return jartifact_names[i];
	}
#endif /*JP*/
    }

    return (char *)0;
}

boolean
exist_artifact(artinum)
int artinum;
{
	if (artinum <= 0 || artinum > NROFARTIFACTS) return FALSE;
	return artiexist[artinum];
}

void
artifact_exists(otmp, name, mod)
register struct obj *otmp;
register const char *name;
register boolean mod;
{
	register const struct artifact *a;

	if (otmp && *name)
	    for (a = artilist+1; a->otyp; a++)
		if (a->otyp == otmp->otyp && (
		    !strcmp(a->name, name)
#ifdef JP
		    || !strcmp(jartifact_names[(a - artilist)], name)
#endif /*JP*/
		   )) {
		    register int m = a - artilist;
		    otmp->oartifact = (char)(mod ? m : 0);
		    otmp->age = 0;
		    change_material(otmp, a->material);
		    if(otmp->otyp == RIN_INCREASE_DAMAGE)
			otmp->spe = 0;
		    artiexist[m] = mod;
		    break;
		}
	return;
}

int
nartifact_exist()
{
    int a = 0;
    int n = SIZE(artiexist);

    while(n > 1)
	if(artiexist[--n]) a++;

    return a;
}
#endif /* OVLB */
#ifdef OVL0

boolean
spec_ability(otmp, abil)
struct obj *otmp;
unsigned long abil;
{
	const struct artifact *arti = get_artifact(otmp);

	return((boolean)(arti && (arti->spfx & abil)));
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
confers_luck(obj)
struct obj *obj;
{
    /* might as well check for this too */
    if (obj->otyp == LUCKSTONE) return TRUE;

    return (obj->oartifact && spec_ability(obj, SPFX_LUCK));
}

/* used to check whether a monster is getting reflection from an artifact */
boolean
arti_reflects(obj)
struct obj *obj;
{
    const struct artifact *arti = get_artifact(obj);

    if (arti) {      
	/* while being worn */
	if ((obj->owornmask & ~W_ART) && (arti->spfx & SPFX_REFLECT))
	    return TRUE;
	/* just being carried */
	if (arti->cspfx & SPFX_REFLECT) return TRUE;
    }
    return FALSE;
}

#endif /* OVL0 */
#ifdef OVLB

//boolean
//restrict_name(otmp, name)  /* returns 1 if name is restricted for otmp->otyp */
//register struct obj *otmp;
//register const char *name;
//{
//	register const struct artifact *a;
//	register const char *aname;
//
//	if (!*name) return FALSE;
//	if (!strncmpi(name, "the ", 4)) name += 4;
//
//		/* Since almost every artifact is SPFX_RESTR, it doesn't cost
//		   us much to do the string comparison before the spfx check.
//		   Bug fix:  don't name multiple elven daggers "Sting".
//		 */
//	for (a = artilist+1; a->otyp; a++) {
//	    if (a->otyp != otmp->otyp) continue;
//	    aname = a->name;
//	    if (!strncmpi(aname, "the ", 4)) aname += 4;
//	    if (!strcmp(aname, name)
//#ifdef JP
//		|| !strcmp(jartifact_names[(a - artilist)], name)
//#endif /*JP*/
//	       ) return ((boolean)((a->spfx & (SPFX_NOGEN|SPFX_RESTR)) != 0 ||
//			otmp->quan > 1L));
//	}
//
//	return FALSE;
//}

int
get_artifact_adtyp(otmp)
struct obj *otmp;
{
	const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return (weap->attk.adtyp);
	return AD_PHYS;
}

STATIC_OVL boolean
attacks(adtyp, otmp)
register int adtyp;
register struct obj *otmp;
{
	register const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return((boolean)(weap->attk.adtyp == adtyp));
	return FALSE;
}

boolean
defends(adtyp, otmp)
register int adtyp;
register struct obj *otmp;
{
	register const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return((boolean)(weap->defn.adtyp == adtyp));
	return FALSE;
}

/* used for monsters */
boolean
protects(adtyp, otmp)
int adtyp;
struct obj *otmp;
{
	register const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return (boolean)(weap->cary.adtyp == adtyp);
	return FALSE;
}

/*
 * a potential artifact has just been worn/wielded/picked-up or
 * unworn/unwielded/dropped.  Pickup/drop only set/reset the W_ART mask.
 */
void
set_artifact_intrinsic(otmp,on,wp_mask)
register struct obj *otmp;
boolean on;
long wp_mask;
{
	long *mask = 0;
	register const struct artifact *oart = get_artifact(otmp);
	uchar dtyp;
	long spfx;

	if (!oart) return;

	/* effects from the defn field */
	dtyp = (wp_mask != W_ART) ? oart->defn.adtyp : oart->cary.adtyp;

	if (dtyp == AD_FIRE)
	    mask = &EFire_resistance;
	else if (dtyp == AD_COLD)
	    mask = &ECold_resistance;
	else if (dtyp == AD_ELEC)
	    mask = &EShock_resistance;
	else if (dtyp == AD_MAGM)
	    mask = &EAntimagic;
	else if (dtyp == AD_DISN)
	    mask = &EDisint_resistance;
	else if (dtyp == AD_DRST)
	    mask = &EPoison_resistance;

	if (mask && wp_mask == W_ART && !on) {
	    /* find out if some other artifact also confers this intrinsic */
	    /* if so, leave the mask alone */
	    register struct obj* obj;
	    for(obj = invent; obj; obj = obj->nobj)
		if(obj != otmp && obj->oartifact) {
		    register const struct artifact *art = get_artifact(obj);
		    if(art->cary.adtyp == dtyp) {
			mask = (long *) 0;
			break;
		    }
		}
	}
	if (mask) {
	    if (on) *mask |= wp_mask;
	    else *mask &= ~wp_mask;
	}

	/* intrinsics from the spfx field; there could be more than one */
	spfx = (wp_mask != W_ART) ? oart->spfx : oart->cspfx;
	if(spfx && wp_mask == W_ART && !on) {
	    /* don't change any spfx also conferred by other artifacts */
	    register struct obj* obj;
	    for(obj = invent; obj; obj = obj->nobj)
		if(obj != otmp && obj->oartifact) {
		    register const struct artifact *art = get_artifact(obj);
		    spfx &= ~art->cspfx;
		}
	}

	if (spfx & SPFX_SEARCH) {
	    if(on) ESearching |= wp_mask;
	    else ESearching &= ~wp_mask;
	}
	if (spfx & SPFX_HALRES) {
	    /* make_hallucinated must (re)set the mask itself to get
	     * the display right */
	    /* restoring needed because this is the only artifact intrinsic
	     * that can print a message--need to guard against being printed
	     * when restoring a game
	     */
	    (void) make_hallucinated((long)!on, restoring ? FALSE : TRUE, wp_mask);
	}
	if (spfx & SPFX_ESP) {
	    if(on) ETelepat |= wp_mask;
	    else ETelepat &= ~wp_mask;
	    see_monsters();
	}
	if (spfx & SPFX_STLTH) {
	    if (on) EStealth |= wp_mask;
	    else EStealth &= ~wp_mask;
	}
	if (spfx & SPFX_REGEN) {
	    if (on) ERegeneration |= wp_mask;
	    else ERegeneration &= ~wp_mask;
	}
	if (spfx & SPFX_TCTRL) {
	    if (on) ETeleport_control |= wp_mask;
	    else ETeleport_control &= ~wp_mask;
	}
	if (spfx & SPFX_WARN) {
	    if (spec_m2(otmp)) {
	    	if (on) {
			EWarn_of_mon |= wp_mask;
			flags.warntype |= spec_m2(otmp);
	    	} else {
			EWarn_of_mon &= ~wp_mask;
	    		flags.warntype &= ~spec_m2(otmp);
		}
		see_monsters();
	    } else {
		if (on) EWarning |= wp_mask;
	    	else EWarning &= ~wp_mask;
	    }
	}
	if (spfx & SPFX_EREGEN) {
	    if (on) EEnergy_regeneration |= wp_mask;
	    else EEnergy_regeneration &= ~wp_mask;
	}
	if (spfx & SPFX_HSPDAM) {
	    if (on) EHalf_spell_damage |= wp_mask;
	    else EHalf_spell_damage &= ~wp_mask;
	}
	if (spfx & SPFX_HPHDAM) {
	    if (on) EHalf_physical_damage |= wp_mask;
	    else EHalf_physical_damage &= ~wp_mask;
	}
	if (spfx & SPFX_XRAY) {
	    /* this assumes that no one else is using xray_range */
	    if (on) u.xray_range = 3;
	    else u.xray_range = -1;
	    vision_full_recalc = 1;
	}
	if ((spfx & SPFX_REFLECT) && (wp_mask & W_WEP)) {
	    if (on) EReflecting |= wp_mask;
	    else EReflecting &= ~wp_mask;
	}
	if (spfx & SPFX_LWILL) {
	    if (on) {
		long oldprop = ELevAtWill;
		ELevAtWill |= wp_mask;
		levitation_on(oldprop, TRUE);
	    } else {
		ELevAtWill &= ~wp_mask;
		levitation_off(0);
	    }
	}

	if(wp_mask == W_ART && !on && oart->inv_prop) {
	    /* might have to turn off invoked power too */
	    if (oart->inv_prop <= LAST_PROP &&
		(u.uprops[oart->inv_prop].extrinsic & W_ARTI))
		(void) arti_invoke(otmp);
	}
}

/*
 * creature (usually player) tries to touch (pick up or wield) an artifact obj.
 * Returns 0 if the object refuses to be touched.
 * This routine does not change any object chains.
 * Ignores such things as gauntlets, assuming the artifact is not
 * fooled by such trappings.
 */
int
touch_artifact(obj,mon,byhand)
    struct obj *obj;
    struct monst *mon;
    boolean byhand;
{
    register const struct artifact *oart = get_artifact(obj);
    boolean badclass, badalign, self_willed, yours;
    boolean prot;

    if(!oart) return 1;

    yours = (mon == &youmonst);
    prot = yours && byhand && uarmg && (get_material(uarmg) == MITHRIL);

    /* all quest artifacts are self-willed; it this ever changes, `badclass'
       will have to be extended to explicitly include quest artifacts */
    self_willed = ((oart->spfx & SPFX_INTEL) != 0);
    if (yours) {
	badclass = self_willed &&
		   ((oart->role != NON_PM && !Role_if(oart->role)) ||
		    (oart->race != NON_PM && !Race_if(oart->race)));
	badalign = /*(oart->spfx & SPFX_RESTR) &&*/ oart->alignment != A_NONE &&
		   (oart->alignment != u.ualign.type || u.ualign.record < 0);
    } else if (!is_covetous(mon->data) && !is_mplayer(mon->data)) {
	badclass = self_willed &&
		   oart->role != NON_PM && oart != &artilist[ART_EXCALIBUR];
	badalign = /*(oart->spfx & SPFX_RESTR) &&*/ oart->alignment != A_NONE &&
		   (oart->alignment != sgn(mon->data->maligntyp));
    } else {    /* an M3_WANTSxxx monster or a fake player */
	/* special monsters trying to take the Amulet, invocation tools or
	   quest item can touch anything except for `spec_applies' artifacts */
	badclass = badalign = FALSE;
    }
    /* weapons which attack specific categories of monsters are
       bad for them even if their alignments happen to match */
    if (!badalign && (oart->spfx & SPFX_DBONUS) != 0) {
	struct artifact tmp;

	tmp = *oart;
	tmp.spfx &= SPFX_DBONUS;
	badalign = !!spec_applies(&tmp, mon);
    }

    if (((badclass || badalign) && self_willed) ||
       (badalign && (!yours || !rn2(4))))  {
	int dmg;
	char buf[BUFSZ];

	if (!yours) return 0;
#ifndef JP
	You("are blasted by %s power!", s_suffix(the(xname(obj))));
#else
	You("%sの力を浴びた！", xname(obj));
#endif /*JP*/
	if (prot) {
#ifndef JP
	    Your("mithril gauntlet%s you from its raging energies!",
		 bimanual(obj) ? "s protect" : " protects");;
#else
	    pline("ミスリルのガントレットが、%sの強力なエネルギーからあなたの身を守った！", xname(obj));
#endif /*JP*/
	} else {
	    dmg = d((Antimagic ? 2 : 4), (self_willed ? 10 : 4));
	    if (Antimagic) damage_resistant_obj(ANTIMAGIC, rnd(dmg));
#ifndef JP
	    sprintf(buf, "touching %s", oart->name);
#else
	    Sprintf(buf, "%sに触って", jartifact_names[obj->oartifact]);
#endif /*JP*/
	    losehp(dmg, buf, KILLED_BY);
	    exercise(A_WIS, FALSE);
	}
    }

    /* can pick it up unless you're totally non-synch'd with the artifact */
    if (badclass && badalign && self_willed && !prot) {
#ifndef JP
	if (yours) pline("%s your grasp!", Tobjnam(obj, "evade"));
#else
	if (yours) pline("%sはあなたの手を逃れた！", xname(obj));
#endif /*JP*/
	return 0;
    }

    return 1;
}

#endif /* OVLB */
#ifdef OVL1

/* decide whether an artifact's special attacks apply against mtmp */
STATIC_OVL int
spec_applies(weap, mtmp)
register const struct artifact *weap;
struct monst *mtmp;
{
	struct permonst *ptr;
	boolean yours;

	if(!(weap->spfx & (SPFX_DBONUS | SPFX_ATTK)))
	    return(weap->attk.adtyp == AD_PHYS);

	yours = (mtmp == &youmonst);
	ptr = mtmp->data;

	if (weap->spfx & SPFX_DMONS) {
	    return (ptr->mnum == (int)weap->mtype);
	} else if (weap->spfx & SPFX_DCLAS) {
	    return (weap->mtype == (unsigned long)ptr->mlet);
	} else if (weap->spfx & SPFX_DFLAG1) {
	    return ((ptr->mflags1 & weap->mtype) != 0L);
	} else if (weap->spfx & SPFX_DFLAG2) {
	    return ((ptr->mflags2 & weap->mtype) || (yours &&
			((!Upolyd && (urace.selfmask & weap->mtype)) ||
			 ((weap->mtype & M2_WERE) && u.ulycn >= LOW_PM))));
	} else if (weap->spfx & SPFX_DALIGN) {
	    return yours ? (u.ualign.type != weap->alignment) :
			   (ptr->maligntyp == A_NONE ||
				sgn(ptr->maligntyp) != weap->alignment);
	} else if (weap->spfx & SPFX_ATTK) {
	    struct obj *defending_weapon = (yours ? uwep : MON_WEP(mtmp));

	    if (defending_weapon && defending_weapon->oartifact &&
		    defends((int)weap->attk.adtyp, defending_weapon))
		return FALSE;
	    switch(weap->attk.adtyp) {
		case AD_FIRE:
			return !(yours ? Fire_resistance : resists_fire(mtmp));
		case AD_COLD:
			return !(yours ? Cold_resistance : resists_cold(mtmp));
		case AD_ELEC:
			return !(yours ? Shock_resistance : resists_elec(mtmp));
		case AD_MAGM:
		case AD_STUN:
			return !(yours ? Antimagic : (rn2(100) < ptr->mr));
		case AD_DRST:
			return !(yours ? Poison_resistance : resists_poison(mtmp));
		case AD_DRLI:
			return !(yours ? Drain_resistance : resists_drli(mtmp));
		case AD_STON:
			return !(yours ? Stone_resistance : resists_ston(mtmp));
		case AD_PHYS: /* specially handled */
			if (weap == &artilist[ART_SKULLCRUSHER]) {
			    return (has_head(ptr) && !unsolid(ptr) && !amorphous(ptr));
			} else return FALSE;
		default:	impossible("Weird weapon special attack.");
	    }
	}
	return(0);
}

/* return the M2 flags of monster that an artifact's special attacks apply against */
long
spec_m2(otmp)
struct obj *otmp;
{
	register const struct artifact *artifact = get_artifact(otmp);
	if (artifact)
		return artifact->mtype;
	return 0L;
}

int
is_bane(otmp, mtmp)
struct obj *otmp;
struct monst *mtmp;
{
	struct permonst *ptr = mtmp->data;
	const struct artifact *artifact = get_artifact(otmp);

	if (artifact) {
	    if (artifact->spfx & SPFX_DMONS) {
		return (ptr->mnum == (int)artifact->mtype);
	    } else if (artifact->spfx & SPFX_DCLAS) {
		return (artifact->mtype == (unsigned long)ptr->mlet);
	    } else if (artifact->spfx & SPFX_DFLAG1) {
		return ((ptr->mflags1 & artifact->mtype) != 0L);
	    } else if (artifact->spfx & SPFX_DFLAG2) {
		return (ptr->mflags2 & artifact->mtype);
	    }
	}
	return 0;
}

/* special attack bonus */
int
spec_abon(otmp, mon)
struct obj *otmp;
struct monst *mon;
{
	register const struct artifact *weap = get_artifact(otmp);

	/* no need for an extra check for `NO_ATTK' because this will
	   always return 0 for any artifact which has that attribute */

	if (weap && weap->attk.damn && spec_applies(weap, mon))
	    return rnd((int)weap->attk.damn);
	return 0;
}

/* special damage bonus */
int
spec_dbon(otmp, mon, tmp)
struct obj *otmp;
struct monst *mon;
int tmp;
{
	const struct artifact *weap = get_artifact(otmp);
	int dbon = 0;

	if (!weap || (weap->attk.adtyp == AD_PHYS && /* check for `NO_ATTK' */
			weap->attk.damn == 0 && weap->attk.damd == 0)) {
	    spec_dbon_applies = FALSE;
	    return 0;
	} else
	    spec_dbon_applies = spec_applies(weap, mon);

	if (spec_dbon_applies) {
	    int dd = weap->attk.damd;
	    if (dd > ADMG_MAX)
		switch (dd) {
		    case ADMG_DOUBLE:
			dbon += max(tmp,1);
			break;
		    default:
			break;
		}
	    else if (dd) dbon += rnd(dd);
	}
	/*    return weap->attk.damd ? rnd((int)weap->attk.damd) : max(tmp,1);*/
	return dbon;
}

/* add identified artifact to discoveries list */
void
discover_artifact(m)
xchar m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's not present, so add it */
    for (i = 0; i < NROFARTIFACTS; i++)
	if (artidisco[i] == 0 || artidisco[i] == m) {
	    artidisco[i] = m;
	    return;
	}
    /* there is one slot per artifact, so we should never reach the
       end without either finding the artifact or an empty slot... */
    impossible("couldn't discover artifact (%d)", (int)m);
}

/* used to decide whether an artifact has been fully identified */
boolean
undiscovered_artifact(m)
xchar m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's undiscovered */
    for (i = 0; i < NROFARTIFACTS; i++)
	if (artidisco[i] == m)
	    return FALSE;
	else if (artidisco[i] == 0)
	    break;
    return TRUE;
}

/* display a list of discovered artifacts; return their count */
int
disp_artifact_discoveries(tmpwin)
winid tmpwin;		/* supplied by dodiscover() */
{
    int i, m, otyp;
    char buf[BUFSZ];
#ifdef JP
    struct obj otm;
#endif /*JP*/

    for (i = 0; i < NROFARTIFACTS; i++) {
	if (artidisco[i] == 0) break;	/* empty slot implies end of list */
	if (i == 0) putstr(tmpwin, iflags.menu_headings, E_J("Artifacts","アーティファクト"));
	m = artidisco[i];
	otyp = artilist[m].otyp;
#ifndef JP
	Sprintf(buf, "  %s [%s %s%s]", artiname(m),
		align_str(artilist[m].alignment),
		(artilist[m].material ? materialnm2[artilist[m].material] : ""),
		simple_typename(otyp));
#else
	/* 素材名が正しく補われるよう、仮のオブジェクトを使う */
	otm.otyp = otyp;
	otm.oartifact = m;
	otm.madeof = artilist[m].material;
	Sprintf(buf, "  %s [%s %s%s]", artiname(m),
		align_str(artilist[m].alignment),
		material_prefix(&otm),
		simple_typename(otyp));
#endif /*JP*/
	putstr(tmpwin, 0, buf);
    }
    return i;
}

#endif /* OVL1 */

#ifdef OVLB


	/*
	 * Magicbane's intrinsic magic is incompatible with normal
	 * enchantment magic.  Thus, its effects have a negative
	 * dependence on spe.  Against low mr victims, it typically
	 * does "double athame" damage, 2d4.  Occasionally, it will
	 * cast unbalancing magic which effectively averages out to
	 * 4d4 damage (3d4 against high mr victims), for spe = 0.
	 *
	 * Prior to 3.4.1, the cancel (aka purge) effect always
	 * included the scare effect too; now it's one or the other.
	 * Likewise, the stun effect won't be combined with either
	 * of those two; it will be chosen separately or possibly
	 * used as a fallback when scare or cancel fails.
	 *
	 * [Historical note: a change to artifact_hit() for 3.4.0
	 * unintentionally made all of Magicbane's special effects
	 * be blocked if the defender successfully saved against a
	 * stun attack.  As of 3.4.1, those effects can occur but
	 * will be slightly less likely than they were in 3.3.x.]
	 */
#define MB_MAX_DIEROLL		8	/* rolls above this aren't magical */
static const char * const mb_verb[2][4] = {
#ifndef JP
	{ "probe", "stun", "scare", "cancel" },
	{ "prod", "amaze", "tickle", "purge" },
#else
	{ "見極めた", "よろめかせた", "恐怖させた", "無力化した" },
	{ "つっ突いた", "びっくりさせた", "くすぐった", "追放した" },
#endif /*JP*/
};
#define MB_INDEX_PROBE		0
#define MB_INDEX_STUN		1
#define MB_INDEX_SCARE		2
#define MB_INDEX_CANCEL		3

/* called when someone is being hit by Magicbane */
STATIC_OVL boolean
Mb_hit(magr, mdef, mb, dmgptr, dieroll, vis, hittee)
struct monst *magr, *mdef;	/* attacker and defender */
struct obj *mb;			/* Magicbane */
int *dmgptr;			/* extra damage target will suffer */
int dieroll;			/* d20 that has already scored a hit */
boolean vis;			/* whether the action can be seen */
char *hittee;			/* target's name: "you" or mon_nam(mdef) */
{
    struct permonst *old_uasmon;
    const char *verb;
    boolean youattack = (magr == &youmonst),
	    youdefend = (mdef == &youmonst),
	    resisted = FALSE, do_stun, do_confuse, result;
    int attack_indx, scare_dieroll = MB_MAX_DIEROLL / 2;

    result = FALSE;		/* no message given yet */
    /* the most severe effects are less likely at higher enchantment */
    if (mb->spe >= 3)
	scare_dieroll /= (1 << (mb->spe / 3));
    /* if target successfully resisted the artifact damage bonus,
       reduce overall likelihood of the assorted special effects */
    if (!spec_dbon_applies) dieroll += 1;

    /* might stun even when attempting a more severe effect, but
       in that case it will only happen if the other effect fails;
       extra damage will apply regardless; 3.4.1: sometimes might
       just probe even when it hasn't been enchanted */
    do_stun = (max(mb->spe,0) < rn2(spec_dbon_applies ? 11 : 7));

    /* the special effects also boost physical damage; increments are
       generally cumulative, but since the stun effect is based on a
       different criterium its damage might not be included; the base
       damage is either 1d4 (athame) or 2d4 (athame+spec_dbon) depending
       on target's resistance check against AD_STUN (handled by caller)
       [note that a successful save against AD_STUN doesn't actually
       prevent the target from ending up stunned] */
    attack_indx = MB_INDEX_PROBE;
    *dmgptr += rnd(4);			/* (2..3)d4 */
    if (do_stun) {
	attack_indx = MB_INDEX_STUN;
	*dmgptr += rnd(4);		/* (3..4)d4 */
    }
    if (dieroll <= scare_dieroll) {
	attack_indx = MB_INDEX_SCARE;
	*dmgptr += rnd(4);		/* (3..5)d4 */
    }
    if (dieroll <= (scare_dieroll / 2)) {
	attack_indx = MB_INDEX_CANCEL;
	*dmgptr += rnd(4);		/* (4..6)d4 */
    }

    /* give the hit message prior to inflicting the effects */
    verb = mb_verb[!!Hallucination][attack_indx];
    if (youattack || youdefend || vis) {
	result = TRUE;
#ifndef JP
	pline_The("magic-absorbing blade %s %s!",
		  vtense((const char *)0, verb), hittee);
#else
	pline("魔力を吸い取る刃が%sを%s！", hittee, verb);
#endif /*JP*/
	/* assume probing has some sort of noticeable feedback
	   even if it is being done by one monster to another */
	if (attack_indx == MB_INDEX_PROBE && !canspotmons(mdef))
	    map_invisible(mdef->mx, mdef->my);
    }

    /* now perform special effects */
    switch (attack_indx) {
    case MB_INDEX_CANCEL:
	old_uasmon = youmonst.data;
	/* No mdef->mcan check: even a cancelled monster can be polymorphed
	 * into a golem, and the "cancel" effect acts as if some magical
	 * energy remains in spellcasting defenders to be absorbed later.
	 */
	if (!cancel_monst(mdef, mb, youattack, FALSE, FALSE)) {
	    resisted = TRUE;
	} else {
	    do_stun = FALSE;
	    if (youdefend) {
		if (youmonst.data != old_uasmon)
		    *dmgptr = 0;    /* rehumanized, so no more damage */
		if (u.uenmax > 0) {
		    You(E_J("lose magical energy!","魔力を失った！"));
		    u.uenmax--;
		    if (u.uen > 0) u.uen--;
		    flags.botl = 1;
		}
	    } else {
		if (mdef->mnum == PM_CLAY_GOLEM)
		    mdef->mhp = 1;	/* cancelled clay golems will die */
		if (youattack && attacktype(mdef->data, AT_MAGC)) {
		    You(E_J("absorb magical energy!","魔力を吸い取った！"));
		    u.uenmax++;
		    u.uen++;
		    flags.botl = 1;
		}
	    }
	}
	break;

    case MB_INDEX_SCARE:
	if (youdefend) {
	    if (Antimagic) {
		resisted = TRUE;
	    } else {
		nomul(-3);
		nomovemsg = "";
		if (magr && magr == u.ustuck && sticks(youmonst.data)) {
		    u.ustuck = (struct monst *)0;
		    You(E_J("release %s!","%sを放した！"), mon_nam(magr));
		}
	    }
	} else {
	    if (rn2(2) && resist(mdef, WEAPON_CLASS, 0, NOTELL))
		resisted = TRUE;
	    else
		monflee(mdef, 3, FALSE, (mdef->mhp > *dmgptr));
	}
	if (!resisted) do_stun = FALSE;
	break;

    case MB_INDEX_STUN:
	do_stun = TRUE;		/* (this is redundant...) */
	break;

    case MB_INDEX_PROBE:
	if (youattack && (mb->spe == 0 || !rn2(3 * abs(mb->spe)))) {
#ifndef JP
	    pline_The("%s is insightful.", verb);
#else
	    You("%sの状態を知った。", mon_nam(mdef));
#endif /*JP*/
	    /* pre-damage status */
	    probe_monster(mdef);
	}
	break;
    }
    /* stun if that was selected and a worse effect didn't occur */
    if (do_stun) {
	if (youdefend)
	    make_stunned((HStun + 3), FALSE);
	else
	    mdef->mstun = 1;
	/* avoid extra stun message below if we used mb_verb["stun"] above */
	if (attack_indx == MB_INDEX_STUN) do_stun = FALSE;
    }
    /* lastly, all this magic can be confusing... */
    do_confuse = !rn2(12);
    if (do_confuse) {
	if (youdefend)
	    make_confused(HConfusion + 4, FALSE);
	else
	    mdef->mconf = 1;
    }

    if (youattack || youdefend || vis) {
	(void) upstart(hittee);	/* capitalize */
	if (resisted) {
#ifndef JP
	    pline("%s %s!", hittee, vtense(hittee, "resist"));
#else
	    pline("%sは抵抗した！", hittee);
#endif /*JP*/
	    shieldeff(youdefend ? u.ux : mdef->mx,
		      youdefend ? u.uy : mdef->my);
	}
	if ((do_stun || do_confuse) && flags.verbose) {
	    char buf[BUFSZ];

	    buf[0] = '\0';
	    if (do_stun) Strcat(buf, E_J("stunned","よろけ"));
	    if (do_stun && do_confuse) Strcat(buf, E_J(" and ","、"));
	    if (do_confuse) Strcat(buf, E_J("confused","混乱し"));
#ifndef JP
	    pline("%s %s %s%c", hittee, vtense(hittee, "are"),
		  buf, (do_stun && do_confuse) ? '!' : '.');
#else
	    pline("%sは%sた%s", hittee, buf,
		  (do_stun && do_confuse) ? "！" : "。");
#endif /*JP*/
	}
    }

    return result;
}
  
/* Function used when someone attacks someone else with an artifact
 * weapon.  Only adds the special (artifact) damage, and returns a 1 if it
 * did something special (in which case the caller won't print the normal
 * hit message).  This should be called once upon every artifact attack;
 * dmgval() no longer takes artifact bonuses into account.  Possible
 * extension: change the killer so that when an orc kills you with
 * Stormbringer it's "killed by Stormbringer" instead of "killed by an orc".
 */
boolean
artifact_hit(magr, mdef, otmp, dmgptr, dieroll)
struct monst *magr, *mdef;
struct obj *otmp;
int *dmgptr;
int dieroll; /* needed for Magicbane and vorpal blades */
{
	boolean youattack = (magr == &youmonst);
	boolean youdefend = (mdef == &youmonst);
	boolean vis = (!youattack && magr && cansee(magr->mx, magr->my))
	    || (!youdefend && cansee(mdef->mx, mdef->my))
	    || (youattack && u.uswallow && mdef == u.ustuck && !Blind);
	boolean realizes_damage;
	const char *wepdesc;
	static const char you[] = E_J("you","あなた");
	char hittee[BUFSZ];

	Strcpy(hittee, youdefend ? you : mon_nam(mdef));

	/* The following takes care of most of the damage, but not all--
	 * the exception being for level draining, which is specially
	 * handled.  Messages are done in this function, however.
	 */
	*dmgptr += spec_dbon(otmp, mdef, *dmgptr);

	if (youattack && youdefend) {
	    impossible("attacking yourself with weapon?");
	    return FALSE;
	}

	realizes_damage = (youdefend || vis || 
			   /* feel the effect even if not seen */
			   (youattack && mdef == u.ustuck));

	/* the four basic attacks: fire, cold, shock and missiles */
	if (attacks(AD_FIRE, otmp)) {
	    if (realizes_damage)
#ifndef JP
		pline_The("fiery %s %s %s%c",
			is_blade(otmp) ? "blade" : "weapon",
			!spec_dbon_applies ? "hits" :
			(mdef->mnum == PM_WATER_ELEMENTAL) ?
			"vaporizes part of" : "burns",
			hittee, !spec_dbon_applies ? '.' : '!');
#else
		pline("炎の%sが%s%s%s",
			is_blade(otmp) ? "刃" : "武器",
			hittee,
			!spec_dbon_applies ? "に命中した" :
			(mdef->mnum == PM_WATER_ELEMENTAL) ?
			"の一部を蒸発させた" : "を焼いた",
			!spec_dbon_applies ? "。" : "！");
#endif /*JP*/
	    if (!rn2(4)) (void) destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
	    if (!rn2(4)) (void) destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
	    if (!rn2(7)) (void) destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
	    if (youdefend && Slimed) burn_away_slime();
	    return realizes_damage;
	}
	if (attacks(AD_COLD, otmp)) {
	    if (realizes_damage)
#ifndef JP
		pline_The("ice-cold %s %s %s%c",
			is_blade(otmp) ? "blade" : "weapon",
			!spec_dbon_applies ? "hits" : "freezes",
			hittee, !spec_dbon_applies ? '.' : '!');
#else
		pline("氷結の%sが%s%s%s",
			is_blade(otmp) ? "刃" : "武器",
			hittee,
			!spec_dbon_applies ? "に命中した" : "を凍らせた",
			!spec_dbon_applies ? "。" : "！");
#endif /*JP*/
	    if (!rn2(4)) (void) destroy_mitem(mdef, POTION_CLASS, AD_COLD);
	    return realizes_damage;
	}
	if (attacks(AD_ELEC, otmp)) {
	    if (realizes_damage)
#ifndef JP
		pline_The("massive hammer hits%s %s%c",
			  !spec_dbon_applies ? "" : "!  Lightning strikes",
			  hittee, !spec_dbon_applies ? '.' : '!');
#else
		pline("巨大な槌が%sに命中し%s", hittee,
			!spec_dbon_applies ? "た。" : "、電撃をくらわせた！");
#endif /*JP*/
	    if (!rn2(5)) (void) destroy_mitem(mdef, RING_CLASS, AD_ELEC);
	    if (!rn2(5)) (void) destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
	    return realizes_damage;
	}
	if (attacks(AD_MAGM, otmp)) {
	    if (realizes_damage)
#ifndef JP
		pline_The("imaginary widget hits%s %s%c",
			  !spec_dbon_applies ? "" :
				"!  A hail of magic missiles strikes",
			  hittee, !spec_dbon_applies ? '.' : '!');
#else
		pline("架空の仕掛けが%sに命中し%s", hittee,
			!spec_dbon_applies ? "た。" : "、魔法の矢が降りそそいだ！");
#endif /*JP*/
	    return realizes_damage;
	}

	if (attacks(AD_STUN, otmp) && dieroll <= MB_MAX_DIEROLL) {
	    /* Magicbane's special attacks (possibly modifies hittee[]) */
	    return Mb_hit(magr, mdef, otmp, dmgptr, dieroll, vis, hittee);
	}

	if (!spec_dbon_applies) {
	    /* since damage bonus didn't apply, nothing more to do;  
	       no further attacks have side-effects on inventory */
	    return FALSE;
	}

	/* We really want "on a natural 20" but Nethack does it in */
	/* reverse from AD&D. */
	if (spec_ability(otmp, SPFX_BEHEAD)) {
	    if (otmp->oartifact == ART_TSURUGI_OF_MURAMASA &&
	        (tech_inuse(T_KIII) ||  dieroll == 1)) {
		wepdesc = E_J("The razor-sharp blade","鋭利な刃");
		/* not really beheading, but so close, why add another SPFX */
		if (youattack && u.uswallow && mdef == u.ustuck) {
		    You(E_J("slice %s wide open!","%sを中から切り開いた！"), mon_nam(mdef));
		    *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
		    return TRUE;
		}
		if (!youdefend) {
			/* allow normal cutworm() call to add extra damage */
			if(notonhead)
			    return FALSE;

			if (bigmonst(mdef->data)) {
				if (youattack)
					You(E_J("slice deeply into %s!",
						"%sに深く斬り込んだ！"),
						mon_nam(mdef));
				else if (vis)
					pline(E_J("%s cuts deeply into %s!",
						  "%sは%sに深く斬り込んだ！"),
					      Monnam(magr), hittee);
				*dmgptr *= 2;
				return TRUE;
			}
			*dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
			pline(E_J("%s cuts %s in half!",
				  "%sが%sを真っ二つにした！"), wepdesc, mon_nam(mdef));
			otmp->dknown = TRUE;
			return TRUE;
		} else {
			if (bigmonst(youmonst.data)) {
				pline(E_J("%s cuts deeply into you!",
					  "%sはあなたに深く斬り込んだ！"),
				      magr ? Monnam(magr) : wepdesc);
				*dmgptr *= 2;
				return TRUE;
			}

			/* Players with negative AC's take less damage instead
			 * of just not getting hit.  We must add a large enough
			 * value to the damage so that this reduction in
			 * damage does not prevent death.
			 */
			*dmgptr = 2 * (Upolyd ? u.mh : u.uhp) + FATAL_DAMAGE_MODIFIER;
			pline(E_J("%s cuts you in half!",
				  "%sがあなたを真っ二つにした！"), wepdesc);
			otmp->dknown = TRUE;
			return TRUE;
		}
	    } else if (otmp->oartifact == ART_VORPAL_BLADE &&
			(dieroll < 3 || mdef->mnum == PM_JABBERWOCK)) {
		static const char * const behead_msg[2] = {
		     E_J("%s beheads %s!",	"%sが%sの首をはねた！"),
		     E_J("%s decapitates %s!",	"%sが%sの首を斬り飛ばした！")
		};

		if (youattack && u.uswallow && mdef == u.ustuck)
			return FALSE;
		wepdesc = E_J(artilist[ART_VORPAL_BLADE].name,
			      jartifact_names[ART_VORPAL_BLADE]);
		if (!youdefend) {
			if (!has_head(mdef->data) || notonhead || u.uswallow) {
				if (youattack)
					pline(E_J("Somehow, you miss %s wildly.",
						  "なぜか、あなたの一撃は%sを大きく外れた。"),
						mon_nam(mdef));
				else if (vis)
					pline(E_J("Somehow, %s misses wildly.",
						  "なぜか、%sの一撃は大きく外れた。"),
						mon_nam(magr));
				*dmgptr = 0;
				return ((boolean)(youattack || vis));
			}
			if (noncorporeal(mdef->data) || amorphous(mdef->data)) {
				pline(E_J("%s slices through %s %s.",
					  "%sが%s%sを空しく通り抜けた。"), wepdesc,
				      s_suffix(mon_nam(mdef)),
				      mbodypart(mdef,NECK));
				return TRUE;
			}
			*dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
			pline(behead_msg[rn2(SIZE(behead_msg))],
			      wepdesc, mon_nam(mdef));
			otmp->dknown = TRUE;
			return TRUE;
		} else {
			if (!has_head(youmonst.data)) {
				pline(E_J("Somehow, %s misses you wildly.",
					  "なぜか、%sの一撃はあなたを大きく外れた。"),
				      magr ? mon_nam(magr) : wepdesc);
				*dmgptr = 0;
				return TRUE;
			}
			if (noncorporeal(youmonst.data) || amorphous(youmonst.data)) {
				pline(E_J("%s slices through your %s.",
					  "%sがあなたの%sを空しく通り抜けた。"),
				      wepdesc, body_part(NECK));
				return TRUE;
			}
			*dmgptr = 2 * (Upolyd ? u.mh : u.uhp)
				  + FATAL_DAMAGE_MODIFIER;
			pline(behead_msg[rn2(SIZE(behead_msg))],
			      wepdesc, E_J("you","あなた"));
			otmp->dknown = TRUE;
			/* Should amulets fall off? */
			return TRUE;
		}
	    }
	}
	if (attacks(AD_DRLI, otmp)) {
		boolean is_sb = (otmp->oartifact == ART_STORMBRINGER);
		if (is_sb && otmp->blessed) {
			if (!rn2(50)) {
				pline_The(E_J("blade glows %s!",
					      "%s刃が禍々しいオーラを放った！"),
				      hcolor(NH_BLACK));
				otmp->blessed = 0;
			} else
				return FALSE;
		}
		if (!youdefend) {
			if (vis) {
			    if(is_sb)
				pline_The(E_J("%s blade draws the life from %s!",
					      "%s刃が%sの生命力を吸い取った！"),
				      hcolor(NH_BLACK),
				      mon_nam(mdef));
			    else
#ifndef JP
				pline("%s draws the life from %s!",
				      The(distant_name(otmp, xname)),
				      mon_nam(mdef));
#else
				pline("%sが%sの生命力を吸い取った！",
				      distant_name(otmp, xname),
				      mon_nam(mdef));
#endif /*JP*/
			}
			if (mdef->m_lev == 0) {
			    *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
			} else {
			    int drain = rnd(m_hd(mdef)/*8*/);
			    if (is_sb && otmp->cursed) drain += rnd(12);
			    *dmgptr += drain;
			    mdef->mhpmax -= drain;
			    mdef->m_lev--;
			    drain /= 2;
			    if (drain) healup(drain, 0, FALSE, FALSE);
			}
			return vis;
		} else { /* youdefend */
			int oldhpmax = u.uhpmax;

			if (Blind)
				You_feel(E_J("an %s drain your life!","%sがあなたの生命力を奪うのを感じた！"),
				    is_sb ?
				    E_J("unholy blade","不浄な刃") : E_J("object","物体"));
			else if (is_sb)
				pline_The(E_J("%s blade drains your life!","%s刃があなたの生命力を奪った！"),
				      hcolor(NH_BLACK));
			else
				pline(E_J("%s drains your life!","%sがあなたの生命力を奪った！"),
				      E_J(The(distant_name(otmp, xname)), distant_name(otmp, xname)));
			losexp(E_J("life drainage","生命力を奪われて"));
			if (magr && magr->mhp < magr->mhpmax) {
			    mlosehp(magr, -((oldhpmax - u.uhpmax)/2));
			}
			return TRUE;
		}
	}
	/* Trollsbane cancels trolls ... do not allow them to revive */
	if (otmp->oartifact == ART_TROLLSBANE /* && spec_dbon_applies */) {
	    if (!youdefend) mdef->mcan = TRUE;
	}

	return FALSE;
}

static NEARDATA const char invoke_types[] = { ALL_CLASSES, 0 };
		/* #invoke: an "ugly check" filters out most objects */
int
getobj_filter_invoke(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	if (otmp->oartifact ||
	    objects[otyp].oc_unique ||
	    (otyp == FAKE_AMULET_OF_YENDOR && !otmp->known) ||
	    otyp == CRYSTAL_BALL ||	/* #invoke synonym for apply */
	   /* note: presenting the possibility of invoking non-artifact
	      mirrors and/or lamps is a simply a cruel deception... */
	    otyp == MIRROR || otyp == MAGIC_LAMP ||
	    (otyp == OIL_LAMP &&	/* don't list known oil lamp */
	     (otmp->dknown && objects[OIL_LAMP].oc_name_known)))
		return GETOBJ_CHOOSEIT;
	return 0;
}

int
doinvoke()
{
    register struct obj *obj;
#ifdef JP
    static const struct getobj_words invkw = { "どの品物", "の", "魔力を発動する", "魔力を発動し" };
#endif /*JP*/

    obj = getobj(invoke_types, E_J("invoke",&invkw), getobj_filter_invoke);
    if(!obj) return 0;
    if (obj->oartifact && !touch_artifact(obj, &youmonst, TRUE)) return 1;
    return arti_invoke(obj);
}

STATIC_OVL int
arti_invoke(obj)
    register struct obj *obj;
{
    register const struct artifact *oart = get_artifact(obj);

    if(!oart || !oart->inv_prop) {
	if(obj->otyp == CRYSTAL_BALL)
	    use_crystal_ball(obj);
	else if (obj->otyp == RIN_LEVITATION ||
		 obj->otyp == LEVITATION_BOOTS ||
		 (oart && (oart->cspfx & SPFX_LWILL)))
	    return invoke_levitation(obj);
	else
	    pline(nothing_happens);
	return 1;
    }

    if(oart->inv_prop > LAST_PROP) {
	/* It's a special power, not "just" a property */
	if(obj->age > monstermoves) {
#ifdef WIZARD
	  if (!wizard || (yn("Force the invocation to succeed?") != 'y')) {
#endif /*WIZARD*/
	    /* the artifact is tired :-) */
#ifndef JP
	    You_feel("that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
#else
	    pline("%sはあなたを無視しているようだ。", xname(obj));
#endif /*JP*/
	    /* and just got more so; patience is essential... */
	    obj->age += (long) d(3,10);
	    return 1;
#ifdef WIZARD
	  }
#endif /*WIZARD*/
	}
	obj->age = monstermoves + rnz(100);

	switch(oart->inv_prop) {
	case TAMING: {
	    struct obj pseudo;

	    pseudo = zeroobj;	/* neither cursed nor blessed */
	    pseudo.otyp = SCR_TAMING;
	    (void) seffects(&pseudo);
	    break;
	  }
	case HEALING: {
	    int healamt = (u.uhpmax + 1 - u.uhp) / 2;
	    long creamed = (long)u.ucreamed;

	    if (Upolyd) healamt = (u.mhmax + 1 - u.mh) / 2;
	    if (healamt || Sick || Slimed || Blinded > creamed)
		You_feel(E_J("better.","回復した。"));
	    else
		goto nothing_special;
	    if (healamt > 0) {
		if (Upolyd) u.mh += healamt;
		else u.uhp += healamt;
	    }
	    if(Sick) make_sick(0L,(char *)0,FALSE,SICK_ALL);
	    if(Slimed) Slimed = 0L;
	    if (Blinded > creamed) make_blinded(creamed, FALSE);
	    flags.botl = 1;
	    break;
	  }
	case ENERGY_BOOST: {
	    int epboost = (u.uenmax + 1 - u.uen) / 2;
	    if (epboost > 120) epboost = 120;		/* arbitrary */
	    else if (epboost < 12) epboost = u.uenmax - u.uen;
	    if(epboost) {
		You_feel(E_J("re-energized.","魔力に満たされた。"));
		u.uen += epboost;
		flags.botl = 1;
	    } else
		goto nothing_special;
	    break;
	  }
	case UNTRAP: {
	    if(!untrap(TRUE)) {
		obj->age = 0; /* don't charge for changing their mind */
		return 0;
	    }
	    break;
	  }
	case CHARGE_OBJ: {
	    struct obj *otmp = getchargableobj();
	    boolean b_effect;

	    if (!otmp) {
		obj->age = 0;
		return 0;
	    }
	    b_effect = obj->blessed &&
		(Role_switch == oart->role || !oart->role);
	    recharge(otmp, b_effect ? 1 : obj->cursed ? -1 : 0);
	    update_inventory();
	    break;
	  }
	case LEV_TELE:
	    level_tele();
	    break;
	case CREATE_PORTAL: {
	    int i, num_ok_dungeons, last_ok_dungeon = 0;
	    d_level newlev;
	    extern int n_dgns; /* from dungeon.c */
	    winid tmpwin = create_nhwindow(NHW_MENU);
	    anything any;

	    any.a_void = 0;	/* set all bits to zero */
	    start_menu(tmpwin);
	    /* use index+1 (cant use 0) as identifier */
	    for (i = num_ok_dungeons = 0; i < n_dgns; i++) {
		if (!dungeons[i].dunlev_ureached) continue;
		any.a_int = i+1;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 E_J(dungeons[i].dname, jdgnnam(i)), MENU_UNSELECTED);
		num_ok_dungeons++;
		last_ok_dungeon = i;
	    }
	    end_menu(tmpwin, E_J("Open a portal to which dungeon?",
				 "どの場所への門を開きますか？"));
	    if (num_ok_dungeons > 1) {
		/* more than one entry; display menu for choices */
		menu_item *selected;
		int n;

		n = select_menu(tmpwin, PICK_ONE, &selected);
		if (n <= 0) {
		    destroy_nhwindow(tmpwin);
		    goto nothing_special;
		}
		i = selected[0].item.a_int - 1;
		free((genericptr_t)selected);
	    } else
		i = last_ok_dungeon;	/* also first & only OK dungeon */
	    destroy_nhwindow(tmpwin);

	    /*
	     * i is now index into dungeon structure for the new dungeon.
	     * Find the closest level in the given dungeon, open
	     * a use-once portal to that dungeon and go there.
	     * The closest level is either the entry or dunlev_ureached.
	     */
	    newlev.dnum = i;
	    if(dungeons[i].depth_start >= depth(&u.uz))
		newlev.dlevel = dungeons[i].entry_lev;
	    else
		newlev.dlevel = dungeons[i].dunlev_ureached;
	    if(u.uhave.amulet || In_endgame(&u.uz) || In_endgame(&newlev) ||
	       newlev.dnum == u.uz.dnum) {
		You_feel(E_J("very disoriented for a moment.",
			     "一瞬自分がどこにいるのか全くわからなくなった。"));
	    } else {
		if(!Blind) You(E_J("are surrounded by a shimmering sphere!",
				   "きらめく光の球につつまれた！"));
		else You_feel(E_J("weightless for a moment.","一瞬身体が浮かび上がるのを感じた。"));
		goto_level(&newlev, FALSE, FALSE, FALSE);
	    }
	    break;
	  }
	case ENLIGHTENING:
	    enlightenment(0);
	    break;
	case CREATE_AMMO: {
	    const int ammotype[4] = { ARROW, CROSSBOW_BOLT, FLINT, BULLET };
	    struct obj *otmp;
	    int atyp;

	    if (obj->oartifact == ART_WEREBANE) {
		int cnt;
		struct obj *oblt, *lblt;
		/* create bullets and fill the cylinder */
		cnt = 6;
		lblt = 0;
		for (oblt = obj->cobj; oblt; oblt = oblt->nobj) {
		    cnt -= oblt->quan;
		    lblt = oblt;
		}
		if (cnt <= 0) goto nothing_special;
		otmp = mksobj(BULLET, TRUE, FALSE);
		if (!otmp) goto nothing_special;
		otmp->blessed = obj->blessed;
		otmp->cursed = obj->cursed;
		otmp->bknown = obj->bknown;
		if ((obj->blessed && otmp->spe < 0) ||
		    (obj->cursed  && otmp->spe > 0)) otmp->spe = 0;
		change_material(otmp, SILVER);
		otmp->quan = cnt;
		otmp->owt = weight(otmp);
#ifndef JP
		pline("%s in the cylinder of %s.",
		      aobjnam(otmp, "magically appear"), the(xname(obj)));
#else
		pline("%sが%sの弾倉に装填された。",
		      doname(otmp), xname(obj));
#endif /*JP*/
		/* merge if possible */
		if (!obj->cobj || !merged(&lblt, &otmp)) {
		    otmp->nobj = (struct obj *)0;
		    otmp->where = OBJ_CONTAINED;
		    otmp->ocontainer = obj;
		    if (lblt) lblt->nobj = otmp;
		    else obj->cobj = otmp;
		}
		obj->owt = weight(obj);
		obj->age += (long) rn1(100,50) * cnt / 6;
		break;
	    }

	    atyp = ammotype[objects[obj->otyp].oc_wprop & WP_SUBTYPE];
	    if (atyp == ARROW) {
		switch (obj->otyp) {
		    case YUMI:		atyp = YA;		break;
		    case ELVEN_BOW:	atyp = ELVEN_ARROW;	break;
		    case ORCISH_BOW:	atyp = ORCISH_ARROW;	break;
		    default:		break;
		}
	    }
	    otmp = mksobj(atyp, TRUE, FALSE);
	    if (!otmp) goto nothing_special;
	    otmp->blessed = obj->blessed;
	    otmp->cursed = obj->cursed;
	    otmp->bknown = obj->bknown;
	    if (obj->blessed) {
		if (otmp->spe < 0) otmp->spe = 0;
		otmp->quan += rnd(10);
	    } else if (obj->cursed) {
		if (otmp->spe > 0) otmp->spe = 0;
	    } else
		otmp->quan += rnd(5);
	    if (oart->alignment == A_LAWFUL)
		otmp->opoisoned = 0;
	    otmp->owt = weight(otmp);
#ifndef JP
	    otmp = hold_another_object(otmp, "Suddenly %s out.",
				       aobjnam(otmp, "fall"), (const char *)0);
#else
	    otmp = hold_another_object(otmp, "突然、%sが足元に落ちた。",
				       xname(otmp), (const char *)0);
#endif /*JP*/
	    break;
	  }
	case EMIT_LIGHT: {
	    struct obj *otmp = mksobj(SCR_LIGHT, FALSE, FALSE);
	    struct monst *mtmp, *mtmp2;
	    int range = obj->blessed ? 9*9 : 5*5;
	    /* emit a light */
	    litroom(TRUE,obj);
	    if (u.uswallow) break;
	    /* turn undead in the light area */
	    vision_recalc(0);	/* litroom() sets vision_full_recalc=1 */
	    for(mtmp = fmon; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (!DEADMONSTER(mtmp) &&
		    (is_undead(mtmp->data) || is_demon(mtmp->data)) &&
		    !mtmp->mpeaceful &&
		    (distu(mtmp->mx,mtmp->my) <= range) &&
		    cansee(mtmp->mx,mtmp->my)) {
		    mtmp->msleeping = 0;
		    if (!resist(mtmp, '\0', 0, TELL)) {
			int xlev = 14 + obj->spe;
			switch (mtmp->data->mlet) {
			    /* this is intentional, lichs are tougher
			       than zombies. */
			case S_LICH:    xlev -= 2;  /*FALLTHRU*/
			case S_GHOST:   xlev -= 2;  /*FALLTHRU*/
			case S_VAMPIRE: xlev -= 2;  /*FALLTHRU*/
			case S_WRAITH:  xlev -= 2;  /*FALLTHRU*/
			case S_MUMMY:   xlev -= 2;  /*FALLTHRU*/
			case S_ZOMBIE:
			    if (rnd(20) < xlev && !resist(mtmp, '\0', 0, NOTELL)) {
				killed(mtmp);
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
	    break;
	  }
	case POISON_BLADE: {
#ifndef JP
	    pline("%s glistens.", The(xname(obj)));
#else
	    pline("%sの刃がぬらりと光った。", xname(obj));
#endif /*JP*/
	    if (!obj->opoisoned) {
		obj->opoisoned = 1;
		update_inventory();
	    }
	    break;
	  }
	}
    } else {
	long eprop = (u.uprops[oart->inv_prop].extrinsic ^= W_ARTI),
	     iprop = u.uprops[oart->inv_prop].intrinsic;
	boolean on = (eprop & W_ARTI) != 0; /* true if invoked prop just set */

	if(on && obj->age > monstermoves) {
	    /* the artifact is tired :-) */
	    u.uprops[oart->inv_prop].extrinsic ^= W_ARTI;
#ifndef JP
	    You_feel("that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
#else
	    pline("%sはあなたを無視しているようだ。", xname(obj));
#endif /*JP*/
	    /* can't just keep repeatedly trying */
	    obj->age += (long) d(3,10);
	    return 1;
	} else if(!on) {
	    /* when turning off property, determine downtime */
	    /* arbitrary for now until we can tune this -dlc */
	    obj->age = monstermoves + rnz(100);
	}

	if ((eprop & ~W_ARTI) || iprop) {
nothing_special:
	    /* you had the property from some other source too */
	    if (carried(obj))
		You_feel(E_J("a surge of power, but nothing seems to happen.",
			     "力の高まりを感じたが、何も起こらなかったようだ。"));
	    return 1;
	}
	switch(oart->inv_prop) {
	case CONFLICT:
	    if(on) You_feel(E_J("like a rabble-rouser.","暴徒を扇動しているような気がした。"));
	    else You_feel(E_J("the tension decrease around you.",
			      "周囲の張りつめた空気が緩むのを感じた。"));
	    break;
//	case LEVITATION:
//	    if(on) {
//		float_up();
//		spoteffects(FALSE);
//	    } else (void) float_down(I_SPECIAL|TIMEOUT, W_ARTI);
//	    break;
	case INVIS:
	    if (BInvis || Blind) goto nothing_special;
	    newsym(u.ux, u.uy);
	    if (on)
		Your(E_J("body takes on a %s transparency...",
			 "身体は%s透明になっていった…。"),
		     Hallucination ? E_J("normal","ありきたりな") : E_J("strange","不思議に"));
	    else
		Your(E_J("body seems to unfade...","姿は再び現れた…。"));
	    break;
	}
    }

    return 1;
}


/* WAC return TRUE if artifact is always lit */
boolean
artifact_light(obj)
    struct obj *obj;
{
    return (get_artifact(obj) && obj->oartifact == ART_SUNSWORD);
}

/* KMH -- Talking artifacts are finally implemented */
void
arti_speak(obj)
    struct obj *obj;
{
	register const struct artifact *oart = get_artifact(obj);
	const char *line;
	char buf[BUFSZ];


	/* Is this a speaking artifact? */
	if (!oart || !(oart->spfx & SPFX_SPEAK))
		return;

	line = getrumor(bcsign(obj), buf, TRUE);
	if (!*line)
		line = E_J("NetHack rumors file closed for renovation.",
			   "NetHack噂ファイルは改装中のためお休みです。");
#ifndef JP
	pline("%s:", Tobjnam(obj, "whisper"));
#else
	pline("%sはささやいた:", xname(obj));
#endif /*JP*/
	verbalize("%s", line);
	return;
}

boolean
artifact_has_invprop(otmp, inv_prop)
struct obj *otmp;
uchar inv_prop;
{
	const struct artifact *arti = get_artifact(otmp);

	return((boolean)(arti && (arti->inv_prop == inv_prop)));
}

/* Return the price sold to the hero of a given artifact or unique item */
long
arti_cost(otmp)
struct obj *otmp;
{
	struct artifact *oart;

	if (!otmp->oartifact)
	    return ((long)objects[otmp->otyp].oc_cost);

	oart = get_artifact(otmp);
	if (oart->cost)
	    return (oart->cost);
	else
	    return (100L * (long)objects[otmp->otyp].oc_cost);
}

#endif /* OVLB */

void
know_enchantment_artifact_worker(weapon, win)
struct obj *weapon;
winid win;
{
	char buf[BUFSZ];
	const struct artifact *artifact = get_artifact(weapon);

	if (artifact) {

	    /* NO_ATTK ? */
	    if (artifact->attk.adtyp == AD_PHYS &&
		artifact->attk.aatyp == 0 &&
		artifact->attk.damn == 0 && artifact->attk.damd == 0) return;

	    putstr(win, 0, "");
	    if (artifact == &artilist[ART_SKULLCRUSHER]) {
		Sprintf(buf, E_J("To foes have skulls", "頭蓋骨を持つ敵に対し"));
	    } else if (artifact->spfx & SPFX_DCLAS) {
		switch (artifact->mtype) {
		    case S_DRAGON:
			Sprintf(buf, E_J("To dragons", "ドラゴン族に対し"));
			break;
		    case S_OGRE:
			Sprintf(buf, E_J("To ogres", "オーガ族に対し"));
			break;
		    case S_TROLL:
			Sprintf(buf, E_J("To trolls", "トロル族に対し"));
			break;
		    default:
			Sprintf(buf, "??? Unknown artifact");
			break;
		}
	    } else if (artifact->spfx & SPFX_DFLAG2) {
		switch (artifact->mtype) {
		    case M2_ORC:
			Sprintf(buf, E_J("To orcs", "オーク族に対し"));
			break;
		    case M2_UNDEAD:
			Sprintf(buf, E_J("To ogres", "アンデッドに対し"));
			break;
		    case M2_DEMON:
			Sprintf(buf, E_J("To devils and demons", "悪魔に対し"));
			break;
		    case M2_WERE:
			Sprintf(buf, E_J("To lycanthropes", "獣人に対し"));
			break;
		    case M2_GIANT:
			Sprintf(buf, E_J("To giants", "巨人に対し"));
			break;
		    default:
			Sprintf(buf, "??? Unknown artifact");
			break;
		}
	    } else if (artifact->spfx & SPFX_DALIGN) {
		Sprintf(buf, E_J("To foes except %s ones", "%s属性でない敵に対し"), align_str(artifact->alignment));
	    } else if (artifact->spfx & SPFX_ATTK) {
		switch(artifact->attk.adtyp) {
		    case AD_FIRE:
			Sprintf(buf, E_J("To foes not fire resistant", "炎が効く敵に対し"));
			break;
		    case AD_COLD:
			Sprintf(buf, E_J("To foes not cold resistant", "冷気が効く敵に対し"));
			break;
		    case AD_ELEC:
			Sprintf(buf, E_J("To foes not shock resistant", "電撃が効く敵に対し"));
			break;
		    case AD_DRST:
			Sprintf(buf, E_J("To foes not poison resistant", "毒が効く敵に対し"));
			break;
		    case AD_STUN:
			Sprintf(buf, E_J("To foes not magic resistant", "魔法攻撃が効く敵に対し"));
			break;
		    case AD_DRLI:
			Sprintf(buf, E_J("To foes not drain resistant", "生命力吸収が効く敵に対し"));
			break;
		    case AD_PHYS:
			Sprintf(buf, E_J("To all foes", "あらゆる敵に対し"));
			break;
		    default:
			Sprintf(buf, "??? Unknown artifact");
			break;
		}
	    } else {
		Sprintf(buf, E_J("To all foes", "あらゆる敵に対し"));
	    }
	    putstr(win, 0, buf);
	    if (artifact->attk.damn>0) {
		Sprintf(buf, E_J("  Hit bonus: +1d%d", "　命中ボーナス: +1d%d"), artifact->attk.damn);
		putstr(win, 0, buf);
	    }
	    if (artifact->attk.damd == ADMG_DOUBLE) {
		Sprintf(buf, E_J("  Double damage", "　ダメージ2倍"));
		putstr(win, 0, buf);
	    } else if (artifact->attk.damd > 0) {
		Sprintf(buf, E_J("  Damage bonus: +1d%d", "　追加ダメージ: +1d%d"), artifact->attk.damd);
		putstr(win, 0, buf);
	    }
	    if (artifact == &artilist[ART_ORCRIST] ||
	        artifact == &artilist[ART_STING]) {
		putstr(win, 0, "");
		putstr(win, 0, E_J("+1d4 damage bonus to all foes", "あらゆる敵に対し +1d4の追加ダメージ"));
	    }

	}
}

/*artifact.c*/
