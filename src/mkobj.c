/*	SCCS Id: @(#)mkobj.c	3.4	2002/10/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "prop.h"
#include "eportal.h"

STATIC_DCL void FDECL(mkbox_cnts,(struct obj *));
STATIC_DCL void FDECL(obj_timer_checks,(struct obj *, XCHAR_P, XCHAR_P, int));
STATIC_DCL void FDECL(container_weight, (struct obj *));
STATIC_DCL void FDECL(save_mtraits, (struct obj *, struct monst *));
#ifdef WIZARD
STATIC_DCL const char *FDECL(where_name, (int));
STATIC_DCL void FDECL(check_contained, (struct obj *,const char *));
#endif
STATIC_DCL const struct probmat *FDECL(get_obj_material_table, (struct obj *));
STATIC_DCL void FDECL(adjust_weight_by_material, (struct obj *obj));
STATIC_DCL void FDECL(init_portal_ring, (struct obj *));

extern struct obj *thrownobj;		/* defined in dothrow.c */

/*#define DEBUG_EFFECTS*/	/* show some messages for debugging */

struct icp {
    int  iprob;		/* probability of an item type */
    char iclass;	/* item class */
};


const struct icp mkobjprobs[] = {
{100, WEAPON_CLASS},
{100, ARMOR_CLASS},
{200, FOOD_CLASS},
{ 80, TOOL_CLASS},
{ 80, GEM_CLASS},
{160, POTION_CLASS},
{160, SCROLL_CLASS},
{ 40, SPBOOK_CLASS},
{ 40, WAND_CLASS},
{ 30, RING_CLASS},
{ 10, AMULET_CLASS}
};

const struct icp boxiprobs[] = {
{180, GEM_CLASS},
{150, FOOD_CLASS},
{180, POTION_CLASS},
{180, SCROLL_CLASS},
{120, SPBOOK_CLASS},
{ 70, COIN_CLASS},
{ 60, WAND_CLASS},
{ 50, RING_CLASS},
{ 10, AMULET_CLASS}
};

#ifdef REINCARNATION
const struct icp rogueprobs[] = {
{120, WEAPON_CLASS},
{120, ARMOR_CLASS},
{220, FOOD_CLASS},
{220, POTION_CLASS},
{220, SCROLL_CLASS},
{ 50, WAND_CLASS},
{ 50, RING_CLASS}
};
#endif

const struct icp hellprobs[] = {
{260, GEM_CLASS},
{200, WEAPON_CLASS},
{200, ARMOR_CLASS},
{120, TOOL_CLASS},
{ 10, POTION_CLASS},
{ 10, SCROLL_CLASS},
{ 80, WAND_CLASS},
{ 80, RING_CLASS},
{ 40, AMULET_CLASS}
};

struct obj *
mkobj_at(let, x, y, artif)
char let;
int x, y;
boolean artif;
{
	struct obj *otmp;

	otmp = mkobj(let, artif);
	place_object(otmp, x, y);
	return(otmp);
}

struct obj *
mksobj_at(otyp, x, y, init, artif)
int otyp, x, y;
boolean init, artif;
{
	struct obj *otmp;

	otmp = mksobj(otyp, init, artif);
	place_object(otmp, x, y);
	return(otmp);
}

struct obj *
mkobj(oclass, artif)
char oclass;
boolean artif;
{
	int tprob, i, prob = rnd(1000);

	if(oclass == RANDOM_CLASS) {
		const struct icp *iprobs =
#ifdef REINCARNATION
				    (Is_rogue_level(&u.uz)) ?
				    (const struct icp *)rogueprobs :
#endif
				    Inhell ? (const struct icp *)hellprobs :
				    (const struct icp *)mkobjprobs;

		for(tprob = rnd(1000);
		    (tprob -= iprobs->iprob) > 0;
		    iprobs++);
		oclass = iprobs->iclass;
	}

	i = bases[(int)oclass];
	while((prob -= objects[i].oc_prob) > 0) i++;

	if(objects[i].oc_class != oclass || !OBJ_NAME(objects[i]))
		panic("probtype error, oclass=%d i=%d", (int) oclass, i);

	return(mksobj(i, TRUE, artif));
}

struct obj *
mkshopobj_at(atype, x, y)
int atype, x, y;
{
	struct obj *otmp;
	int tprob, i;
	if (atype < 0)
	    otmp = mksobj_at(-atype, x, y, TRUE, TRUE);
	else {
	    if (atype == RANDOM_CLASS) {
		atype = GEM_CLASS;
		for (i=50; i && atype == GEM_CLASS; i--) {
			const struct icp *iprobs =
			    Inhell ? (const struct icp *)hellprobs :
			    (const struct icp *)mkobjprobs;
			for(tprob = rnd(1000);
			    (tprob -= iprobs->iprob) > 0;
			    iprobs++);
			atype = iprobs->iclass;
		}
	    }
	    otmp = mkobj_at(atype, x, y, TRUE);
	}
	return otmp;
}

STATIC_OVL void
mkbox_cnts(box)
struct obj *box;
{
	register int n;
	register struct obj *otmp;

	box->cobj = (struct obj *) 0;

	switch (box->otyp) {
	case ICE_BOX:		n = 20; break;
	case CHEST:		n = 5; break;
	case LARGE_BOX:		n = 3; break;
	case SACK:
	case OILSKIN_SACK:
				/* initial inventory: sack starts out empty */
				if (moves <= 1 && !in_mklev) { n = 0; break; }
				/*else FALLTHRU*/
	case BAG_OF_HOLDING:	n = 1; break;
	default:		n = 0; break;
	}

	for (n = rn2(n+1); n > 0; n--) {
	    if (box->otyp == ICE_BOX) {
		if (!(otmp = mksobj(CORPSE, TRUE, TRUE))) continue;
		/* Note: setting age to 0 is correct.  Age has a different
		 * from usual meaning for objects stored in ice boxes. -KAA
		 */
		otmp->age = 0L;
		if (otmp->timed) {
		    (void) stop_timer(ROT_CORPSE, (genericptr_t)otmp);
		    (void) stop_timer(REVIVE_MON, (genericptr_t)otmp);
		}
	    } else {
		register int tprob;
		const struct icp *iprobs = boxiprobs;

		for (tprob = rnd(1000); (tprob -= iprobs->iprob) > 0; iprobs++)
		    ;
		if (!(otmp = mkobj(iprobs->iclass, TRUE))) continue;

		/* handle a couple of special cases */
		if (otmp->oclass == COIN_CLASS) {
		    /* 2.5 x level's usual amount; weight adjusted below */
		    otmp->quan = (long)(rnd(level_difficulty()+2) * rnd(75));
		    otmp->owt = weight(otmp);
		} else while (otmp->otyp == ROCK) {
		    set_otyp(otmp, rnd_class(DILITHIUM_CRYSTAL, LOADSTONE));
		    if (otmp->quan > 2L) otmp->quan = 1L;
		    otmp->owt = weight(otmp);
		}
		if (box->otyp == BAG_OF_HOLDING) {
		    if (Is_mbag(otmp)) {
			set_otyp(otmp, SACK);
			otmp->spe = 0;
			otmp->owt = weight(otmp);
		    } else while (otmp->otyp == WAN_CANCELLATION)
			    set_otyp(otmp, rnd_class(WAN_LIGHT, WAN_LIGHTNING));
		}
	    }
	    (void) add_to_container(box, otmp);
	}
}

int
rndmonnum()	/* select a random, common monster type */
{
	register struct permonst *ptr;
	register int	i;

	/* Plan A: get a level-appropriate common monster */
	ptr = rndmonst();
	if (ptr) return(monsndx(ptr));

	/* Plan B: get any common monster */
	do {
	    i = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
	    ptr = &mons[i];
	} while((ptr->geno & G_NOGEN) || (!Inhell && (ptr->geno & G_HELL)));

	return(i);
}

/*
 * Split obj so that it gets size gets reduced by num. The quantity num is
 * put in the object structure delivered by this call.  The returned object
 * has its wornmask cleared and is positioned just following the original
 * in the nobj chain (and nexthere chain when on the floor).
 */
struct obj *
splitobj(obj, num)
struct obj *obj;
long num;
{
	struct obj *otmp;

	if (obj->cobj || num <= 0L || obj->quan <= num)
	    panic("splitobj");	/* can't split containers */
	otmp = newobj();
	*otmp = *obj;		/* copies whole structure */
	otmp->o_id = flags.ident++;
	if (!otmp->o_id) otmp->o_id = flags.ident++;	/* ident overflowed */
	otmp->timed = 0;	/* not timed, yet */
	otmp->lamplit = 0;	/* ditto */
	otmp->owornmask = 0L;	/* new object isn't worn */
	obj->quan -= num;
	obj->owt = weight(obj);
	otmp->quan = num;
	otmp->owt = weight(otmp);	/* -= obj->owt ? */
	obj->nobj = otmp;
	otmp->oextra = 0;
	otmp->has_name = 0;
	otmp->omon = 0;
	/* Only set nexthere when on the floor, nexthere is also used */
	/* as a back pointer to the container object when contained. */
	if (obj->where == OBJ_FLOOR)
	    obj->nexthere = otmp;
	copy_xdat_obj(otmp, obj);
	if (obj->unpaid) splitbill(obj,otmp);
	if (obj->timed) obj_split_timers(obj, otmp);
	if (obj_sheds_light(obj)) obj_split_light_source(obj, otmp);
	return otmp;
}

struct obj *
duplicateobj(obj)
struct obj *obj;
{
	struct obj *otmp;
	struct obj *otmp2, *odup, **olnk;

	otmp = newobj();
	*otmp = *obj;		/* copies whole structure */
	otmp->where = OBJ_FREE;
	otmp->o_id = flags.ident++;
	if (!otmp->o_id) otmp->o_id = flags.ident++;	/* ident overflowed */
	otmp->timed = 0;	/* not timed, yet */
	otmp->lamplit = 0;	/* ditto */
	otmp->owornmask = 0L;	/* new object isn't worn */
	otmp->nobj = 0;
	otmp->oartifact = 0;	/* cannot copy artifact */
	otmp->unpaid = 0;
	otmp->oextra = 0;
	otmp->omon   = 0;
	/* Only set nexthere when on the floor, nexthere is also used */
	/* as a back pointer to the container object when contained. */
	if (obj->where == OBJ_FLOOR)
	    obj->nexthere = otmp;
	copy_xdat_obj(otmp, obj);
	/* copy contents */
	if (obj->cobj)
	    for (otmp2 = obj->cobj, olnk = &(otmp->cobj); otmp2; otmp2 = otmp2->nobj) {
		odup = duplicateobj(otmp2);
		odup->nobj = 0;
		odup->where = OBJ_CONTAINED;
		odup->ocontainer = otmp;
		*olnk = odup;
		olnk = &(odup->nobj);
	    }
	return otmp;
}

/*
 * Insert otmp right after obj in whatever chain(s) it is on.  Then extract
 * obj from the chain(s).  This function does a literal swap.  It is up to
 * the caller to provide a valid context for the swap.  When done, obj will
 * still exist, but not on any chain.
 *
 * Note:  Don't use use obj_extract_self() -- we are doing an in-place swap,
 * not actually moving something.
 */
void
replace_object(obj, otmp)
struct obj *obj;
struct obj *otmp;
{
    otmp->where = obj->where;
    switch (obj->where) {
    case OBJ_FREE:
	/* do nothing */
	break;
    case OBJ_INVENT:
	otmp->nobj = obj->nobj;
	obj->nobj = otmp;
	extract_nobj(obj, &invent);
	break;
    case OBJ_CONTAINED:
	otmp->nobj = obj->nobj;
	otmp->ocontainer = obj->ocontainer;
	obj->nobj = otmp;
	extract_nobj(obj, &obj->ocontainer->cobj);
	break;
    case OBJ_MINVENT:
	otmp->nobj = obj->nobj;
	otmp->ocarry =  obj->ocarry;
	obj->nobj = otmp;
	extract_nobj(obj, &obj->ocarry->minvent);
	break;
    case OBJ_FLOOR:
	otmp->nobj = obj->nobj;
	otmp->nexthere = obj->nexthere;
	otmp->ox = obj->ox;
	otmp->oy = obj->oy;
	obj->nobj = otmp;
	obj->nexthere = otmp;
	extract_nobj(obj, &fobj);
	extract_nexthere(obj, &level.objects[obj->ox][obj->oy]);
	break;
    default:
	panic("replace_object: obj position");
	break;
    }
}

/*
 * Create a dummy duplicate to put on shop bill.  The duplicate exists
 * only in the billobjs chain.  This function is used when a shop object
 * is being altered, and a copy of the original is needed for billing
 * purposes.  For example, when eating, where an interruption will yield
 * an object which is different from what it started out as; the "I x"
 * command needs to display the original object.
 *
 * The caller is responsible for checking otmp->unpaid and
 * costly_spot(u.ux, u.uy).  This function will make otmp no charge.
 *
 * Note that check_unpaid_usage() should be used instead for partial
 * usage of an object.
 */
void
bill_dummy_object(otmp)
register struct obj *otmp;
{
	register struct obj *dummy;

	if (otmp->unpaid)
	    subfrombill(otmp, shop_keeper(*u.ushops));
	dummy = newobj();
	*dummy = *otmp;
	dummy->where = OBJ_FREE;
	dummy->o_id = flags.ident++;
	if (!dummy->o_id) dummy->o_id = flags.ident++;	/* ident overflowed */
	dummy->timed = 0;
	dummy->oextra = 0;
	dummy->omon = 0;
	copy_xdat_obj(dummy, otmp);
	if (Is_candle(dummy)) dummy->lamplit = 0;
	addtobill(dummy, FALSE, TRUE, TRUE);
	otmp->no_charge = 1;
	otmp->unpaid = 0;
	return;
}


static const char dknowns[] = {
		WAND_CLASS, RING_CLASS, POTION_CLASS, SCROLL_CLASS,
		GEM_CLASS, SPBOOK_CLASS, WEAPON_CLASS, TOOL_CLASS, 0
};

struct obj *
mksobj(otyp, init, artif)
int otyp;
boolean init;
boolean artif;
{
	int mndx, tryct;
	struct obj *otmp;
	char let = objects[otyp].oc_class;

	otmp = newobj();
	*otmp = zeroobj;
	otmp->age = monstermoves;
	otmp->o_id = flags.ident++;
	if (!otmp->o_id) otmp->o_id = flags.ident++;	/* ident overflowed */
	otmp->quan = 1L;
	otmp->oclass = let;
	otmp->color = objects[otyp].oc_color;
	otmp->otyp = otyp;
	otmp->where = OBJ_FREE;
	otmp->odamaged = 0;
	otmp->dknown = index(dknowns, let) ? 0 : 1;
	if (let == ARMOR_CLASS && armor_colorless_name(otmp))
/*	if ((otmp->otyp >= ELVEN_SHIELD && otmp->otyp <= ORCISH_SHIELD) ||
			otmp->otyp == SHIELD_OF_REFLECTION)*/
		otmp->dknown = 0;
	if (!objects[otmp->otyp].oc_uses_known)
		otmp->known = 1;
	if (init) switch (let) {
	case WEAPON_CLASS:
		otmp->age = 0;		/* number of times this weapon hits enemy */
		rnd_material(otmp);			/* material patch */
		if (is_multigen(otmp))
			otmp->quan = is_bullet(otmp) ? (long) rn1(30,20) : (long) rn1(6,6);
		else otmp->quan = 1L;
		if(!rn2(11)) {
			otmp->spe = rne(3);
			otmp->blessed = rn2(2);
		} else if(!rn2(10)) {
			curse(otmp);
			otmp->spe = -rne(3);
		} else	blessorcurse(otmp, 10);
		if (is_poisonable(otmp) && !rn2(30/*100*/))
			otmp->opoisoned = 1;

		if (artif && !rn2(20))
		    otmp = mk_artifact(otmp, (aligntyp)A_NONE);

		/* generate a gun with bullets */
		if (is_gun(otmp)) {
		    struct obj *otmp2;
		    int bnum = (otmp->otyp == MUSKET) ? 1 :
			       (otmp->otyp == REVOLVER) ? 6 : 0;
		    if (bnum) {
			otmp2 = mksobj(BULLET, TRUE, FALSE);
			otmp2->blessed = otmp2->cursed = 0;
			otmp2->spe = 0;
			otmp2->quan = bnum;
			otmp2->owt = weight(otmp2);
			otmp2->nobj = (struct obj *)0;
			otmp2->where = OBJ_CONTAINED;
			otmp2->ocontainer = otmp;
			otmp->cobj = otmp2;
		    }
		}
		break;
	case FOOD_CLASS:
	    otmp->oeaten = 0;
	    switch(otmp->otyp) {
	    case CORPSE:
		/* possibly overridden by mkcorpstat() */
		tryct = 50;
		do otmp->corpsenm = undead_to_corpse(rndmonnum());
		while ((mvitals[otmp->corpsenm].mvflags & G_NOCORPSE) && (--tryct > 0));
		if (tryct == 0) {
		/* perhaps rndmonnum() only wants to make G_NOCORPSE monsters on
		   this level; let's create an adventurer's corpse instead, then */
			otmp->corpsenm = PM_HUMAN;
		}
		/* timer set below */
		break;
	    case EGG:
		otmp->corpsenm = NON_PM;	/* generic egg */
		if (!rn2(3)) for (tryct = 200; tryct > 0; --tryct) {
		    mndx = can_be_hatched(rndmonnum());
		    if (mndx != NON_PM && !dead_species(mndx, TRUE)) {
			otmp->corpsenm = mndx;		/* typed egg */
			attach_egg_hatch_timeout(otmp);
			break;
		    }
		}
		break;
	    case TIN:
		otmp->corpsenm = NON_PM;	/* empty (so far) */
		if (!rn2(6))
		    otmp->spe = 1;		/* spinach */
		else for (tryct = 200; tryct > 0; --tryct) {
		    mndx = undead_to_corpse(rndmonnum());
		    if (mons[mndx].cnutrit &&
			    !(mvitals[mndx].mvflags & G_NOCORPSE)) {
			otmp->corpsenm = mndx;
			break;
		    }
		}
		blessorcurse(otmp, 10);
		break;
	    case SLIME_MOLD:
		otmp->spe = current_fruit;
		break;
	    case KELP_FROND:
		otmp->quan = (long) rnd(2);
		break;
	    }
	    if (otmp->otyp == CORPSE || otmp->otyp == MEAT_RING ||
		otmp->otyp == KELP_FROND) break;
	    /* fall into next case */

	case GEM_CLASS:
		if (otmp->otyp == LOADSTONE) curse(otmp);
		else if (otmp->otyp == ROCK) otmp->quan = (long) rn1(6,6);
		else if (otmp->otyp != LUCKSTONE && !rn2(6)) otmp->quan = 2L;
		else otmp->quan = 1L;
		break;
	case TOOL_CLASS:
	    switch(otmp->otyp) {
		case TALLOW_CANDLE:
		case WAX_CANDLE:	otmp->spe = 1;
					otmp->age = 20L * /* 400 or 200 */
					      (long)objects[otmp->otyp].oc_cost;
					otmp->lamplit = 0;
					otmp->quan = 1L +
					      (long)(rn2(2) ? rn2(7) : 0);
					blessorcurse(otmp, 5);
					break;
		case BRASS_LANTERN:
		case OIL_LAMP:		otmp->spe = 1;
					otmp->age = (long) rn1(500,1000);
					otmp->lamplit = 0;
					blessorcurse(otmp, 5);
					break;
		case MAGIC_CANDLE:	otmp->age = 1000; /* dummy */
		case MAGIC_LAMP:	otmp->spe = 1;
					otmp->lamplit = 0;
					blessorcurse(otmp, 2);
					break;
		case CHEST:
		case LARGE_BOX:		otmp->olocked = !!(rn2(5));
					otmp->otrapped = (depth(&u.uz) > rn2(20));
					if (otmp->otrapped) {
					    trap_chest(otmp);
					    if (otmp->corpsenm == BOXTRAP_EXPLODING) otmp->olocked = 1;
					}
		case ICE_BOX:
		case SACK:
		case OILSKIN_SACK:
		case BAG_OF_HOLDING:	mkbox_cnts(otmp);
					break;
#ifdef FIRSTAID
		case SCISSORS:		blessorcurse(otmp, 2);
					break;
		case BANDAGE:		otmp->quan = (long)rnd(5);
					blessorcurse(otmp, 5);
					break;
#endif
		case EXPENSIVE_CAMERA:
		case MAGIC_WHISTLE:
		case TINNING_KIT:
		case MAGIC_MARKER:	otmp->spe = rn1(70,30);
					break;
		case CAN_OF_GREASE:	otmp->spe = rnd(25);
					blessorcurse(otmp, 10);
					break;
		case CRYSTAL_BALL:	otmp->spe = rnd(5);
					blessorcurse(otmp, 2);
					break;
		case ORB_OF_DESTRUCTION: blessorcurse(otmp, 2);
					break;
		case ORB_OF_CHARGING:   otmp->spe = rn1(5,5);
					blessorcurse(otmp, 2);
					break;
		case ORB_OF_MAINTENANCE: otmp->spe = rn1(4,4);
					blessorcurse(otmp, 2);
					break;
		case HORN_OF_PLENTY:
		case BAG_OF_TRICKS:	otmp->spe = rnd(20);
					break;
		case FIGURINE:	{	int tryct2 = 0;
					do
					    otmp->corpsenm = rndmonnum();
					while(is_human(&mons[otmp->corpsenm])
						&& tryct2++ < 30);
					blessorcurse(otmp, 4);
					break;
				}
		case BELL_OF_OPENING:   otmp->spe = 3;
					break;
		case MAGIC_FLUTE:
		case MAGIC_HARP:
		case FROST_HORN:
		case FIRE_HORN:
		case DRUM_OF_EARTHQUAKE:
					otmp->spe = rn1(5,4);
					break;
#ifdef MAGIC_GLASSES
		case GLASSES_OF_PHANTASMAGORIA:
					if(rn2(10)) curse(otmp);
					break;
#endif
	    }
	    break;
	case AMULET_CLASS:
		/*if (otmp->otyp == AMULET_OF_YENDOR) flags.made_amulet = TRUE;*/
		if(rn2(10) && (otmp->otyp == AMULET_OF_STRANGULATION ||
		   otmp->otyp == AMULET_OF_CHANGE ||
		   otmp->otyp == AMULET_OF_RESTFUL_SLEEP)) {
			curse(otmp);
		} else	blessorcurse(otmp, 10);
	case VENOM_CLASS:
	case CHAIN_CLASS:
	case BALL_CLASS:
		break;
	case POTION_CLASS:
		if (otmp->otyp == POT_OIL)
		    otmp->age = MAX_OIL_IN_FLASK;	/* amount of oil */
		/* fall through */
	case SCROLL_CLASS:
#ifdef MAIL
		if (otmp->otyp != SCR_MAIL)
#endif
			blessorcurse(otmp, 4);
		break;
	case SPBOOK_CLASS:
		blessorcurse(otmp, 17);
		break;
	case ARMOR_CLASS:
		rnd_material(otmp);			/* material patch test */
		if(rn2(10) && (otmp->otyp == FUMBLE_BOOTS ||
		   otmp->otyp == LEVITATION_BOOTS ||
		   otmp->otyp == HELM_OF_OPPOSITE_ALIGNMENT ||
		   otmp->otyp == GAUNTLETS_OF_FUMBLING ||
		   otmp->otyp == ROBE_OF_WEAKNESS ||
		   !rn2(11))) {
			curse(otmp);
			otmp->spe = -rne(3);
		} else if(!rn2(10)) {
			otmp->blessed = rn2(2);
			otmp->spe = rne(3);
		} else	blessorcurse(otmp, 10);
		if (artif && !rn2(40))
		    otmp = mk_artifact(otmp, (aligntyp)A_NONE);
		/* simulate lacquered armor for samurai */
		if (Role_if(PM_SAMURAI) && otmp->otyp == SPLINT_MAIL &&
		    (moves <= 1 || In_quest(&u.uz))) {
#ifdef UNIXPC
			/* optimizer bitfield bug */
			otmp->oerodeproof = 1;
			otmp->rknown = 1;
#else
			otmp->oerodeproof = otmp->rknown = 1;
#endif
		}
		if (otmp->otyp == HAWAIIAN_SHIRT) {
			int tmpc;
			tmpc = rn2(CLR_MAX);
			if (tmpc != NO_COLOR) otmp->color = tmpc;
		} else if (otmp->otyp == T_SHIRT) {
			if (Role_if(PM_TOURIST) && Is_qstart(&u.uz)) {
			    /* underwear */
			    otmp->color = CLR_WHITE;
			    otmp->oprint = TSHIRT_PRINT_NONE;
			} else {
			    int tmpc;
			    tmpc = rn2(CLR_MAX);
			    if (tmpc != NO_COLOR) otmp->color = tmpc;
			    otmp->oprint = TSHIRT_PRINT_TEXT;
			}
		}
		break;
	case WAND_CLASS:
		if(otmp->otyp == WAN_WISHING) otmp->spe = rnd(3); else
		otmp->spe = rn1(5,
			(objects[otmp->otyp].oc_dir == NODIR) ? 11 : 4);
		blessorcurse(otmp, 17);
		otmp->recharged = 0; /* used to control recharging */
		break;
	case RING_CLASS:
		if (otmp->otyp == RIN_PORTAL) {
		    init_portal_ring(otmp);
		} else if(objects[otmp->otyp].oc_charged) {
		    blessorcurse(otmp, 3);
		    if(rn2(10)) {
			if(rn2(10) && bcsign(otmp))
			    otmp->spe = bcsign(otmp) * rne(3);
			else otmp->spe = rn2(2) ? rne(3) : -rne(3);
		    }
		    /* make useless +0 rings much less common */
		    if (otmp->spe == 0) otmp->spe = rn2(4) - rn2(3);
		    /* negative rings are usually cursed */
		    if (otmp->spe < 0 && rn2(5)) curse(otmp);
		} else if(rn2(10) && (otmp->otyp == RIN_TELEPORTATION ||
			  otmp->otyp == RIN_POLYMORPH ||
			  otmp->otyp == RIN_AGGRAVATE_MONSTER ||
			  otmp->otyp == RIN_HUNGER || !rn2(9))) {
			curse(otmp);
		}
		break;
	case ROCK_CLASS:
		switch (otmp->otyp) {
		    case STATUE:
			/* possibly overridden by mkcorpstat() */
			otmp->corpsenm = rndmonnum();
			if (!verysmall(&mons[otmp->corpsenm]) &&
				rn2(level_difficulty()/2 + 10) > 10)
			    (void) add_to_container(otmp,
						    mkobj(SPBOOK_CLASS,FALSE));
			else rnd_material(otmp);
		}
		break;
	case COIN_CLASS:
		break;	/* do nothing */
	default:
		impossible("impossible mkobj %d, sym '%c'.", otmp->otyp,
						objects[otmp->otyp].oc_class);
		return (struct obj *)0;
	}

	/* Some things must get done (timers) even if init = 0 */
	switch (otmp->otyp) {
	    case CORPSE:
		start_corpse_timeout(otmp);
		break;
	}

	/* unique objects may have an associated artifact entry */
	if (objects[otyp].oc_unique && !otmp->oartifact)
	    otmp = mk_artifact(otmp, (aligntyp)A_NONE);
	otmp->owt = weight(otmp);
	return(otmp);
}

/*
 * Start a corpse decay or revive timer.
 * This takes the age of the corpse into consideration as of 3.4.0.
 */
void
start_corpse_timeout(body)
	struct obj *body;
{
	long when; 		/* rot away when this old */
	long corpse_age;	/* age of corpse          */
	int rot_adjust;
	short action;

#define TAINT_AGE (50L)		/* age when corpses go bad */
#define TROLL_REVIVE_CHANCE 37	/* 1/37 chance for 50 turns ~ 75% chance */
#define ROT_AGE (250L)		/* age when corpses rot away */

	/* lizards and lichen don't rot or revive */
	if (body->corpsenm == PM_LIZARD || body->corpsenm == PM_LICHEN) return;

	action = ROT_CORPSE;		/* default action: rot away */
	rot_adjust = in_mklev ? 25 : 10;	/* give some variation */
	corpse_age = monstermoves - body->age;
	if (corpse_age > ROT_AGE)
		when = rot_adjust;
	else
		when = ROT_AGE - corpse_age;
	when += (long)(rnz(rot_adjust) - rot_adjust);

	if (is_rider(&mons[body->corpsenm])) {
		/*
		 * Riders always revive.  They have a 1/3 chance per turn
		 * of reviving after 12 turns.  Always revive by 500.
		 */
		action = REVIVE_MON;
		for (when = 12L; when < 500L; when++)
		    if (!rn2(3)) break;

	} else if (mons[body->corpsenm].mlet == S_TROLL && !body->norevive) {
	    long age;
	    struct monst *mtmp = get_mtraits(body);
	    if (mtmp && !mtmp->mcan) {
		for (age = 2; age <= TAINT_AGE; age++)
		    if (!rn2(TROLL_REVIVE_CHANCE)) {	/* troll revives */
			action = REVIVE_MON;
			when = age;
			break;
		    }
	    }
	}
	
	if (body->norevive) body->norevive = 0;
	(void) start_timer(when, TIMER_OBJECT, action, (genericptr_t)body);
}

void
bless(otmp)
register struct obj *otmp;
{
	otmp->cursed = 0;
	otmp->blessed = 1;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if (otmp->otyp == FIGURINE && otmp->timed)
		(void) stop_timer(FIG_TRANSFORM, (genericptr_t) otmp);
	else if (objects[otmp->otyp].oc_oprop == LEVATWILL &&
		 otmp->owornmask) {
	    long oldprop = Lev_at_will;
	    BLevAtWill &= ~otmp->owornmask;
	    levitation_on(oldprop, Lev_at_will ? TRUE : FALSE);
	}
	else if (otmp->oartifact == ART_STORMBRINGER)
#ifndef JP
		pline("%s vibrates unexpectedly...", xname(otmp));
#else
		pline("%sは恨めしげに震えた…。", xname(otmp));
#endif /*JP*/
	return;
}

void
unbless(otmp)
register struct obj *otmp;
{
	otmp->blessed = 0;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
}

void
curse(otmp)
register struct obj *otmp;
{
	otmp->blessed = 0;
	otmp->cursed = 1;
	/* welded two-handed weapon interferes with some armor removal */
	if (otmp == uwep && bimanual(uwep)) reset_remarm();
	/* rules at top of wield.c state that twoweapon cannot be done
	   with cursed alternate weapon */
	if (otmp == uswapwep && u.twoweap)
	    drop_uswapwep();
	/* some cursed items need immediate updating */
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if (otmp->otyp == FIGURINE) {
		if (otmp->corpsenm != NON_PM
	    	    && !dead_species(otmp->corpsenm,TRUE)
		    && (carried(otmp) || mcarried(otmp)))
			attach_fig_transform_timeout(otmp);
	} else if (objects[otmp->otyp].oc_oprop == LEVATWILL &&
		   otmp->owornmask) {
	    long oldprop = Lev_at_will;
	    BLevAtWill |= otmp->owornmask;
	    levitation_on(oldprop, FALSE);
	} else if (otmp->oartifact == ART_STORMBRINGER)
#ifndef JP
		pline("%s looks satisfied...", xname(otmp));
#else
		pline("%sが満足げなオーラを放ったような気がした…。", xname(otmp));
#endif /*JP*/

 	return;
}

void
uncurse(otmp)
register struct obj *otmp;
{
	otmp->cursed = 0;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if (otmp->otyp == FIGURINE && otmp->timed)
	    (void) stop_timer(FIG_TRANSFORM, (genericptr_t) otmp);
	else if (objects[otmp->otyp].oc_oprop == LEVATWILL &&
		 otmp->owornmask) {
	    long oldprop = Lev_at_will;
	    BLevAtWill &= ~otmp->owornmask;
	    levitation_on(oldprop, Lev_at_will ? TRUE : FALSE);
	}
	return;
}


void
blessorcurse(otmp, chance)
register struct obj *otmp;
register int chance;
{
	if(otmp->blessed || otmp->cursed) return;

	if(!rn2(chance)) {
	    if(!rn2(2)) {
		curse(otmp);
	    } else {
		bless(otmp);
	    }
	}
	return;
}

void
set_otyp(otmp, otyp)
struct obj *otmp;
int otyp;
{
	if(otmp) {
	    otmp->otyp = otyp;
	    otmp->color = objects[otyp].oc_color;
	}
}


int
bcsign(otmp)
register struct obj *otmp;
{
	return(!!otmp->blessed - !!otmp->cursed);
}


/*
 *  Calculate the weight of the given object.  This will recursively follow
 *  and calculate the weight of any containers.
 *
 *  Note:  It is possible to end up with an incorrect weight if some part
 *	   of the code messes with a contained object and doesn't update the
 *	   container's weight.
 */
int
weight(obj)
register struct obj *obj;
{
	int wt = objects[obj->otyp].oc_weight;

	if (obj->otyp == LARGE_BOX && obj->spe == -1) /* Schroedinger's Cat */
		wt += mons[PM_HOUSECAT].cwt;
	if (Is_container(obj) || obj->otyp == STATUE || is_gun(obj)) {
		struct obj *contents;
		register int cwt = 0;

		if (obj->otyp == STATUE && obj->corpsenm >= LOW_PM) {
		    wt = (int)obj->quan *
			 ((int)mons[obj->corpsenm].cwt * 3 / 2);
		}

		for(contents=obj->cobj; contents; contents=contents->nobj)
			cwt += weight(contents);
		/*
		 *  The weight of bags of holding is calculated as the weight
		 *  of the bag plus the weight of the bag's contents modified
		 *  as follows:
		 *
		 *	Bag status	Weight of contents
		 *	----------	------------------
		 *	cursed			2x
		 *	blessed			x/4 + 1
		 *	otherwise		x/2 + 1
		 *
		 *  The macro DELTA_CWT in pickup.c also implements these
		 *  weight equations.
		 *
		 *  Note:  The above checks are performed in the given order.
		 *	   this means that if an object is both blessed and
		 *	   cursed (not supposed to happen), it will be treated
		 *	   as cursed.
		 */
		if (obj->otyp == BAG_OF_HOLDING)
		    cwt = obj->cursed ? (cwt * 2) :
					(1 + (cwt / (obj->blessed ? 4 : 2)));

		return wt + cwt;
	}
	if (!objects[obj->otyp].oc_merge && obj->madeof && obj->owt != 0)
		return((int)(obj->owt));	/* material patch */
	if (obj->otyp == CORPSE && obj->corpsenm >= LOW_PM) {
		long long_wt = obj->quan * (long) mons[obj->corpsenm].cwt;

		wt = (long_wt > LARGEST_INT) ? LARGEST_INT : (int)long_wt;
		if (obj->oeaten) wt = eaten_stat(wt, obj);
		return wt;
	} else if (obj->oclass == FOOD_CLASS && obj->oeaten) {
		return eaten_stat((int)obj->quan * wt, obj);
	} else if (obj->oclass == COIN_CLASS)
		return (int)((obj->quan + 50L) / 100L);
	else if (obj->otyp == HEAVY_IRON_BALL && obj->owt != 0)
		return((int)(obj->owt));	/* kludge for "very" heavy iron ball */
	return(wt ? wt*(int)obj->quan : ((int)obj->quan + 1)>>1);
}

static int treefruits[] = {APPLE,ORANGE,PEAR,BANANA,EUCALYPTUS_LEAF};

struct obj *
rnd_treefruit_at(x,y)
int x, y;
{
	return mksobj_at(treefruits[rn2(SIZE(treefruits))], x, y, TRUE, FALSE);
}

/***************************
	material patch
 ***************************/
struct probmat {
    int   iprob;
    uchar imat;
};

/* plate mail */
const struct probmat metal_armor_probs1[] = {
{ 570, IRON	},
{ 300, COPPER	}, /* bronze */
{ 100, GLASS	}, /* crystal */
{  30, MITHRIL	},
{   0, GOLD	},
{   0, SILVER	},
{   0, 0	} /* terminator */
};

/* club */
const struct probmat primitive_weapon_probs1[] = {
{ 850, WOOD	},
{ 100, MINERAL	},
{  50, BONE	},
{   0, 0	} /* terminator */
};

/* silver saber */
const struct probmat silver_weapon_probs0[] = {
{ 900, SILVER	},
{ 100, IRON	},
{   0, MITHRIL	},
{   0, 0	} /* terminator */
};

/* silver weapon(high prob) */
const struct probmat silver_weapon_probs1[] = {
{ 810, IRON	},
{ 180, SILVER	},
{  10, METAL	},
{   0, MITHRIL	},
{   0, 0	} /* terminator */
};

/* silver weapon(middle prob) */
const struct probmat silver_weapon_probs2[] = {
{ 899, IRON	},
{  90, SILVER	},
{  10, METAL	},
{   1, MITHRIL	},
{   0, 0	} /* terminator */
};

/* silver weapon(low prob) */
const struct probmat silver_weapon_probs3[] = {
{ 959, IRON	},
{  35, SILVER	},
{   5, METAL	},
{   1, MITHRIL	},
{   0, 0	} /* terminator */
};

/* statue */
const struct probmat statue_probs1[] = {
{ 950, MINERAL	},
{  50, COPPER	},
{   0, IRON	},
{   0, GLASS	},
{   0, GOLD	},
{   0, SILVER	},
{   0, WOOD	},
{   0, LIQUID	}, /* ice */
{   0, 0	} /* terminator */
};

/* shields (never randomly generated) */
const struct probmat special_probs1[] = {
{1000, IRON	},
{   0, COPPER	},
{   0, IRON	},
{   0, GLASS	},
{   0, GOLD	},
{   0, SILVER	},
{   0, MITHRIL	},
{   0, WOOD	},
{   0, MINERAL	},
{   0, DRAGON_HIDE },
{   0, 0	} /* terminator */
};

struct moti {
    int  typ;			/* item type */
    const struct probmat *tbl;	/* ptr to table of allowable materials */
};

const struct moti mat_obj_tbl[] = {
	{PLATE_MAIL,	metal_armor_probs1},
	{HELMET,	metal_armor_probs1},
	{CLUB,		primitive_weapon_probs1},
	{KNIFE,		silver_weapon_probs3},
	{DAGGER,	silver_weapon_probs2},
	{ARROW,		silver_weapon_probs1},
	{LONG_SWORD,	silver_weapon_probs3},
	{SHORT_SWORD,	silver_weapon_probs2},
	{MACE,		silver_weapon_probs2},
	{SABER,		silver_weapon_probs0},
	{SPEAR,		silver_weapon_probs3},
	{LANCE,		silver_weapon_probs3},
	{STATUE,	statue_probs1},
	{SHIELD,	special_probs1},
	{GAUNTLETS,	metal_armor_probs1},
	{BULLET,	silver_weapon_probs1},
	{ELVEN_DAGGER,	special_probs1},
	{ELVEN_BROADSWORD, special_probs1},
	{STRANGE_OBJECT, (void *)0 }
};

boolean
is_material_variable(obj)
struct obj *obj;
{
	if (obj->oclass != RING_CLASS &&	/* bit is used as oc_tough */
	    obj->oclass != GEM_CLASS)		/* bit is used as oc_tough */
		return (objects[obj->otyp].oc_other_material);
	return FALSE;
}

int
get_material(obj)
struct obj *obj;
{
	return (obj->madeof ? obj->madeof : objects[obj->otyp].oc_material);
}

const struct probmat *
get_obj_material_table(obj)
struct obj *obj;
{
	int i;
	for ( i=0; mat_obj_tbl[i].typ != STRANGE_OBJECT; i++ ) {
		if ( obj->otyp == mat_obj_tbl[i].typ ) return mat_obj_tbl[i].tbl;
	}
	return (const struct probmat *)0;
}

boolean
change_material(obj, material)
struct obj *obj;
int material;
{
	const struct probmat *mat;
	int i;
	if (is_material_variable(obj) || !material) {
	    if (objects[obj->otyp].oc_material == (unsigned)material ||
		!material) {
		obj->madeof = 0;			  /* natural material */
		obj->color = objects[obj->otyp].oc_color; /* natural color */
		obj->owt = weight(obj);
		return TRUE;
	    } else {
		mat = get_obj_material_table(obj);
		if (!mat) return FALSE;
		for ( i=0; mat[i].imat != 0; i++ ) {
		    if ( mat[i].imat == material ) {
			obj->madeof = material;		/* special material */
			obj->color = materialcolor[material]; /* material color */
			adjust_weight_by_material(obj);
			return TRUE;
		    }
		}
	    }
	}
	return FALSE;			/* material type of this object cannot be changed */
}

/* weight adjustment(currently it handles very few case) */
void
adjust_weight_by_material(obj)
struct obj *obj;
{
	int origwt = objects[obj->otyp].oc_weight;
	switch (obj->oclass) {
	    case ARMOR_CLASS:
		/* mithril plate mail */
		if (objects[obj->otyp].oc_material == IRON &&
		    obj->madeof == MITHRIL)
			obj->owt = origwt * 3 / 5;
		break;
	    case WEAPON_CLASS:
		/* stone club, bone club */
		if (obj->otyp == CLUB) {
		    if (obj->madeof == MINERAL) obj->owt = origwt + 10; /* slightly heavier */
		} else
		/* silver weapon */
		if (!objects[obj->otyp].oc_merge &&
		    objects[obj->otyp].oc_material == IRON &&
		    obj->madeof == SILVER)
			obj->owt = origwt * 12 / 10;
	    default:
		break;
	}
}

boolean
rnd_material(obj)
struct obj *obj;
{
	const struct probmat *iprobs;
	int tprob, prob = rnd(1000);

	iprobs = get_obj_material_table(obj);
	if (!iprobs) return FALSE;

	for(tprob = rnd(1000);
	    (tprob -= iprobs->iprob) > 0;
	    iprobs++);
	return change_material(obj, iprobs->imat);
}

void
reset_color(obj)
struct obj *obj;
{
	int otyp = obj->otyp;

	/* Wrong color might be set for obj with shuffled appearances */
	if (is_robe(obj) ||
	    (otyp >= HELMET && otyp <= HELM_OF_TELEPATHY) ||
	    (otyp >= CLOAK_OF_PROTECTION && otyp <= CLOAK_OF_DISPLACEMENT) ||
	    obj->oclass == RING_CLASS || obj->oclass == POTION_CLASS ||
	    obj->oclass == WAND_CLASS || obj->oclass == SPBOOK_CLASS) {

	    if (!obj->madeof) {
		obj->color = objects[obj->otyp].oc_color; /* natural color */
	    } else {
		obj->color = materialcolor[obj->madeof]; /* material color */
	    }
	}
}

struct obj *
mkicefrozenmon(monnum)
int monnum;
{
	struct obj *otmp;
	int tryct;
	if (monnum == NON_PM) {
	    tryct = 50;
	    do monnum = undead_to_corpse(rndmonnum());
	    while (!can_be_frozen(&mons[monnum]) && (--tryct > 0));
	    if (tryct == 0) {
	    /* perhaps rndmonnum() only wants to make G_NOCORPSE monsters on
	       this level; let's create an adventurer's corpse instead, then */
		monnum = PM_HUMAN;
	    }
	}
	otmp = mksobj(STATUE, FALSE, FALSE);
	otmp->corpsenm = monnum;
	change_material(otmp, LIQUID); /* ice */
	otmp->owt = weight(otmp);
	return otmp;
}


struct obj *
mkgold(amount, x, y)
long amount;
int x, y;
{
    register struct obj *gold = g_at(x,y);

    if (amount <= 0L)
	amount = (long)(1 + rnd(level_difficulty()+2) * rnd(30));
    if (gold) {
	gold->quan += amount;
    } else {
	gold = mksobj_at(GOLD_PIECE, x, y, TRUE, FALSE);
	gold->quan = amount;
    }
    gold->owt = weight(gold);
    return (gold);
}


/* return TRUE if the corpse has special timing */
#define special_corpse(num)  (((num) == PM_LIZARD)		\
				|| ((num) == PM_LICHEN)		\
				|| (is_rider(&mons[num]))	\
				|| (mons[num].mlet == S_TROLL))

/*
 * OEXTRA note: Passing mtmp causes mtraits to be saved
 * even if ptr passed as well, but ptr is always used for
 * the corpse type (corpsenm). That allows the corpse type
 * to be different from the original monster,
 *	i.e.  vampire -> human corpse
 * yet still allow restoration of the original monster upon
 * resurrection.
 */
struct obj *
mkcorpstat(objtype, mtmp, ptr, x, y, init)
int objtype;	/* CORPSE or STATUE */
struct monst *mtmp;
struct permonst *ptr;
int x, y;
boolean init;
{
	register struct obj *otmp;
	if (objtype != CORPSE && objtype != STATUE)
	    impossible("making corpstat type %d", objtype);
	if (x == 0 && y == 0) {		/* special case - random placement */
		otmp = mksobj(objtype, init, FALSE);
		if (otmp) rloco(otmp);
	} else
		otmp = mksobj_at(objtype, x, y, init, FALSE);
	if (otmp) {
	    if (mtmp) {
		if (!ptr) ptr = mtmp->data;
		save_mtraits(otmp, mtmp);
	    }
	    /* use the corpse or statue produced by mksobj() as-is
	       unless `ptr' is non-null */
	    if (ptr) {
		int old_corpsenm = otmp->corpsenm;

		otmp->corpsenm = monsndx(ptr);
		otmp->owt = weight(otmp);
		if (otmp->otyp == CORPSE &&
			(special_corpse(old_corpsenm) ||
				special_corpse(otmp->corpsenm))) {
		    obj_stop_timers(otmp);
		    start_corpse_timeout(otmp);
		}
	    }
	}
	return(otmp);
}

/*
 * Attach a monster id to an object, to provide
 * a lasting association between the two.
 */
struct obj *
obj_attach_mid(obj, mid)
struct obj *obj;
unsigned mid;
{
    if (!mid || !obj) return (struct obj *)0;
    add_xdat_obj(obj, XDAT_M_ID, &mid);
    return obj;
}

void
save_mtraits(obj, mtmp)
struct obj *obj;
struct monst *mtmp;
{
	struct obj *otmp;

	/* paranoia */
	if (obj->omon) impossible("save_mtraits: omon is not empty");
	if (mtmp->minvent) impossible("save_mtraits: minvent is not empty");

	obj->omon = mtmp;
	mtmp->mtraitskept = 1; /* mark the mon is linked to an obj */
}

/* returns a pointer to a new monst structure based on
 * the one contained within the obj.
 */
struct monst *
get_mtraits(obj)
struct obj *obj;
{
	return obj->omon;
}


/* make an object named after someone listed in the scoreboard file */
struct obj *
mk_tt_object(objtype, x, y)
int objtype; /* CORPSE or STATUE */
register int x, y;
{
	register struct obj *otmp, *otmp2;
	boolean initialize_it;

	/* player statues never contain books */
	initialize_it = (objtype != STATUE);
	if ((otmp = mksobj_at(objtype, x, y, initialize_it, FALSE)) != 0) {
	    /* tt_oname will return null if the scoreboard is empty */
	    if ((otmp2 = tt_oname(otmp)) != 0) otmp = otmp2;
	}
	return(otmp);
}

/* make a new corpse or statue, uninitialized if a statue (i.e. no books) */
struct obj *
mk_named_object(objtype, ptr, x, y, nm)
int objtype;	/* CORPSE or STATUE */
struct permonst *ptr;
int x, y;
const char *nm;
{
	struct obj *otmp;

	otmp = mkcorpstat(objtype, (struct monst *)0, ptr,
				x, y, (boolean)(objtype != STATUE));
	if (nm)
		otmp = oname(otmp, nm);
	return(otmp);
}

boolean
is_flammable(otmp)
register struct obj *otmp;
{
	int otyp = otmp->otyp;
	int omat = get_material(otmp); /*objects[otyp].oc_material;*/

	if (objects[otyp].oc_oprop == FIRE_RES || otyp == WAN_FIRE)
		return FALSE;

	return((boolean)((omat <= WOOD && omat != LIQUID) || omat == PLASTIC));
}

boolean
is_rottable(otmp)
register struct obj *otmp;
{
	int omat = get_material(otmp);

	return((boolean)(otmp->oclass != TOOL_CLASS &&
			 omat <= WOOD && omat != LIQUID));
}


/*
 * These routines maintain the single-linked lists headed in level.objects[][]
 * and threaded through the nexthere fields in the object-instance structure.
 */

/* put the object at the given location */
void
place_object(otmp, x, y)
register struct obj *otmp;
int x, y;
{
    register struct obj *otmp2 = level.objects[x][y];

    if (otmp->where != OBJ_FREE)
	panic("place_object: obj not free");

    obj_no_longer_held(otmp);
    if (otmp->otyp == BOULDER) block_point(x,y);	/* vision */

    /* obj goes under boulders */
    if (otmp2 && (otmp2->otyp == BOULDER)) {
	otmp->nexthere = otmp2->nexthere;
	otmp2->nexthere = otmp;
    } else {
	otmp->nexthere = otmp2;
	level.objects[x][y] = otmp;
    }

    /* set the new object's location */
    otmp->ox = x;
    otmp->oy = y;

    otmp->where = OBJ_FLOOR;

    /* add to floor chain */
    otmp->nobj = fobj;
    fobj = otmp;
    if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
}

#define ON_ICE(a) ((a)->recharged)
#define ROT_ICE_ADJUSTMENT 2	/* rotting on ice takes 2 times as long */

/* If ice was affecting any objects correct that now
 * Also used for starting ice effects too. [zap.c]
 */
void
obj_ice_effects(x, y, do_buried)
int x, y;
boolean do_buried;
{
	struct obj *otmp;

	for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere) {
		if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
	}
	if (do_buried) {
	    for (otmp = level.buriedobjlist; otmp; otmp = otmp->nobj) {
 		if (otmp->ox == x && otmp->oy == y) {
			if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
		}
	    }
	}
}

/*
 * Returns an obj->age for a corpse object on ice, that would be the
 * actual obj->age if the corpse had just been lifted from the ice.
 * This is useful when just using obj->age in a check or calculation because
 * rot timers pertaining to the object don't have to be stopped and
 * restarted etc.
 */
long
peek_at_iced_corpse_age(otmp)
struct obj *otmp;
{
    long age, retval = otmp->age;
    
    if (otmp->otyp == CORPSE && ON_ICE(otmp)) {
	/* Adjust the age; must be same as obj_timer_checks() for off ice*/
	age = monstermoves - otmp->age;
	retval = otmp->age + (age / ROT_ICE_ADJUSTMENT);
#ifdef DEBUG_EFFECTS
	pline_The("%s age has ice modifications:otmp->age = %ld, returning %ld.",
		s_suffix(doname(otmp)),otmp->age, retval);
	pline("Effective age of corpse: %ld.",
		monstermoves - retval);
#endif
    }
    return retval;
}

STATIC_OVL void
obj_timer_checks(otmp, x, y, force)
struct obj *otmp;
xchar x, y;
int force;	/* 0 = no force so do checks, <0 = force off, >0 force on */
{
    long tleft = 0L;
    short action = ROT_CORPSE;
    boolean restart_timer = FALSE;
    boolean on_floor = (otmp->where == OBJ_FLOOR);
    boolean buried = (otmp->where == OBJ_BURIED);

    /* Check for corpses just placed on or in ice */
    if (otmp->otyp == CORPSE && (on_floor || buried) && is_ice(x,y)) {
	tleft = stop_timer(action, (genericptr_t)otmp);
	if (tleft == 0L) {
		action = REVIVE_MON;
		tleft = stop_timer(action, (genericptr_t)otmp);
	} 
	if (tleft != 0L) {
	    long age;
	    
	    tleft = tleft - monstermoves;
	    /* mark the corpse as being on ice */
	    ON_ICE(otmp) = 1;
#ifdef DEBUG_EFFECTS
	    pline("%s is now on ice at %d,%d.", The(xname(otmp)),x,y);
#endif
	    /* Adjust the time remaining */
	    tleft *= ROT_ICE_ADJUSTMENT;
	    restart_timer = TRUE;
	    /* Adjust the age; must be same as in obj_ice_age() */
	    age = monstermoves - otmp->age;
	    otmp->age = monstermoves - (age * ROT_ICE_ADJUSTMENT);
	}
    }
    /* Check for corpses coming off ice */
    else if ((force < 0) ||
	     (otmp->otyp == CORPSE && ON_ICE(otmp) &&
	     ((on_floor && !is_ice(x,y)) || !on_floor))) {
	tleft = stop_timer(action, (genericptr_t)otmp);
	if (tleft == 0L) {
		action = REVIVE_MON;
		tleft = stop_timer(action, (genericptr_t)otmp);
	}
	if (tleft != 0L) {
		long age;

		tleft = tleft - monstermoves;
		ON_ICE(otmp) = 0;
#ifdef DEBUG_EFFECTS
	    	pline("%s is no longer on ice at %d,%d.", The(xname(otmp)),x,y);
#endif
		/* Adjust the remaining time */
		tleft /= ROT_ICE_ADJUSTMENT;
		restart_timer = TRUE;
		/* Adjust the age */
		age = monstermoves - otmp->age;
		otmp->age = otmp->age + (age / ROT_ICE_ADJUSTMENT);
	}
    }
    /* now re-start the timer with the appropriate modifications */ 
    if (restart_timer)
	(void) start_timer(tleft, TIMER_OBJECT, action, (genericptr_t)otmp);
}

#undef ON_ICE
#undef ROT_ICE_ADJUSTMENT

void
remove_object(otmp)
register struct obj *otmp;
{
    xchar x = otmp->ox;
    xchar y = otmp->oy;

    if (otmp->where != OBJ_FLOOR)
	panic("remove_object: obj not on floor");
    extract_nexthere(otmp, &level.objects[x][y]);
    extract_nobj(otmp, &fobj);
    if (otmp->otyp == BOULDER && !sobj_at(BOULDER,x,y))
	unblock_point(x,y); /* vision */
    if (otmp->timed) obj_timer_checks(otmp,x,y,0);
}

/* throw away all of a monster's inventory */
void
discard_minvent(mtmp)
struct monst *mtmp;
{
    struct obj *otmp;

    while ((otmp = mtmp->minvent) != 0) {
	obj_extract_self(otmp);
	obfree(otmp, (struct obj *)0);	/* dealloc_obj() isn't sufficient */
    }
}

/*
 * Free obj from whatever list it is on in preperation of deleting it or
 * moving it elsewhere.  This will perform all high-level consequences
 * involved with removing the item.  E.g. if the object is in the hero's
 * inventory and confers heat resistance, the hero will lose it.
 *
 * Object positions:
 *	OBJ_FREE	not on any list
 *	OBJ_FLOOR	fobj, level.locations[][] chains (use remove_object)
 *	OBJ_CONTAINED	cobj chain of container object
 *	OBJ_INVENT	hero's invent chain (use freeinv)
 *	OBJ_MINVENT	monster's invent chain
 *	OBJ_MIGRATING	migrating chain
 *	OBJ_BURIED	level.buriedobjs chain
 *	OBJ_ONBILL	on billobjs chain
 */
void
obj_extract_self(obj)
    struct obj *obj;
{
    switch (obj->where) {
	case OBJ_FREE:
	    break;
	case OBJ_FLOOR:
	    remove_object(obj);
	    break;
	case OBJ_CONTAINED:
	    extract_nobj(obj, &obj->ocontainer->cobj);
	    container_weight(obj->ocontainer);
	    break;
	case OBJ_INVENT:
	    freeinv(obj);
	    break;
	case OBJ_MINVENT:
	    extract_nobj(obj, &obj->ocarry->minvent);
	    break;
	case OBJ_MIGRATING:
	    extract_nobj(obj, &migrating_objs);
	    break;
	case OBJ_BURIED:
	    extract_nobj(obj, &level.buriedobjlist);
	    break;
	case OBJ_ONBILL:
	    extract_nobj(obj, &billobjs);
	    break;
	default:
	    panic("obj_extract_self");
	    break;
    }
}


/* Extract the given object from the chain, following nobj chain. */
void
extract_nobj(obj, head_ptr)
    struct obj *obj, **head_ptr;
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nobj) {
	if (curr == obj) {
	    if (prev)
		prev->nobj = curr->nobj;
	    else
		*head_ptr = curr->nobj;
	    break;
	}
    }
    if (!curr) panic("extract_nobj: object lost");
    obj->where = OBJ_FREE;
}


/*
 * Extract the given object from the chain, following nexthere chain.
 *
 * This does not set obj->where, this function is expected to be called
 * in tandem with extract_nobj, which does set it.
 */
void
extract_nexthere(obj, head_ptr)
    struct obj *obj, **head_ptr;
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nexthere) {
	if (curr == obj) {
	    if (prev)
		prev->nexthere = curr->nexthere;
	    else
		*head_ptr = curr->nexthere;
	    break;
	}
    }
    if (!curr) panic("extract_nexthere: object lost");
}


/*
 * Add obj to mon's inventory.  If obj is able to merge with something already
 * in the inventory, then the passed obj is deleted and 1 is returned.
 * Otherwise 0 is returned.
 */
int
add_to_minv(mon, obj)
    struct monst *mon;
    struct obj *obj;
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
	panic("add_to_minv: obj not free");

    /* merge if possible */
    for (otmp = mon->minvent; otmp; otmp = otmp->nobj)
	if (merged(&otmp, &obj))
	    return 1;	/* obj merged and then free'd */
    /* else insert; don't bother forcing it to end of chain */
    obj->where = OBJ_MINVENT;
    obj->ocarry = mon;
    obj->nobj = mon->minvent;
#ifdef PICKUP_THROWN
    obj->othrown = 0;
#endif
    mon->minvent = obj;
    return 0;	/* obj on mon's inventory chain */
}

/*
 * Add obj to container, make sure obj is "free".  Returns (merged) obj.
 * The input obj may be deleted in the process.
 */
struct obj *
add_to_container(container, obj)
    struct obj *container, *obj;
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
	panic("add_to_container: obj not free");
    if (container->where != OBJ_INVENT && container->where != OBJ_MINVENT)
	obj_no_longer_held(obj);

    /* merge if possible */
    for (otmp = container->cobj; otmp; otmp = otmp->nobj)
	if (merged(&otmp, &obj)) return (otmp);

    obj->where = OBJ_CONTAINED;
    obj->ocontainer = container;
    obj->nobj = container->cobj;
    container->cobj = obj;
    return (obj);
}

void
add_to_migration(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("add_to_migration: obj not free");

    obj->where = OBJ_MIGRATING;
    obj->nobj = migrating_objs;
    migrating_objs = obj;
}

void
add_to_buried(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("add_to_buried: obj not free");

    obj->where = OBJ_BURIED;
    obj->nobj = level.buriedobjlist;
    level.buriedobjlist = obj;
}

/* Recalculate the weight of this container and all of _its_ containers. */
STATIC_OVL void
container_weight(container)
    struct obj *container;
{
    container->owt = weight(container);
    if (container->where == OBJ_CONTAINED)
	container_weight(container->ocontainer);
/*
    else if (container->where == OBJ_INVENT)
	recalculate load delay here ???
*/
}

/*
 * Deallocate the object.  _All_ objects should be run through here for
 * them to be deallocated.
 */
void
dealloc_obj(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("dealloc_obj: obj not free");

    /* free up any timers attached to the object */
    if (obj->timed)
	obj_stop_timers(obj);

    /*
     * Free up any light sources attached to the object.
     *
     * We may want to just call del_light_source() without any
     * checks (requires a code change there).  Otherwise this
     * list must track all objects that can have a light source
     * attached to it (and also requires lamplit to be set).
     */
    if (obj_sheds_light(obj))
	del_light_source(LS_OBJECT, (genericptr_t) obj);

    if (obj == thrownobj) thrownobj = (struct obj*)0;

    free((genericptr_t) obj);
}

#ifdef WIZARD
/* Check all object lists for consistency. */
void
obj_sanity_check()
{
    int x, y;
    struct obj *obj;
    struct monst *mon;
    const char *mesg;
    char obj_address[20], mon_address[20];  /* room for formatted pointers */

    mesg = "fobj sanity";
    for (obj = fobj; obj; obj = obj->nobj) {
	if (obj->where != OBJ_FLOOR) {
	    pline("%s obj %s %s@(%d,%d): %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where),
		obj->ox, obj->oy, doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "location sanity";
    for (x = 0; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
	    for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if (obj->where != OBJ_FLOOR) {
		    pline("%s obj %s %s@(%d,%d): %s\n", mesg,
			fmt_ptr((genericptr_t)obj, obj_address),
			where_name(obj->where),
			obj->ox, obj->oy, doname(obj));
		}

    mesg = "invent sanity";
    for (obj = invent; obj; obj = obj->nobj) {
	if (obj->where != OBJ_INVENT) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "migrating sanity";
    for (obj = migrating_objs; obj; obj = obj->nobj) {
	if (obj->where != OBJ_MIGRATING) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "buried sanity";
    for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
	if (obj->where != OBJ_BURIED) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "bill sanity";
    for (obj = billobjs; obj; obj = obj->nobj) {
	if (obj->where != OBJ_ONBILL) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	/* shouldn't be a full container on the bill */
	if (obj->cobj) {
	    pline("%s obj %s contains %s! %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		something, doname(obj));
	}
    }

    mesg = "minvent sanity";
    for (mon = fmon; mon; mon = mon->nmon)
	for (obj = mon->minvent; obj; obj = obj->nobj) {
	    if (obj->where != OBJ_MINVENT) {
		pline("%s obj %s %s: %s\n", mesg,
			fmt_ptr((genericptr_t)obj, obj_address),
			where_name(obj->where), doname(obj));
	    }
	    if (obj->ocarry != mon) {
		pline("%s obj %s (%s) not held by mon %s (%s)\n", mesg,
			fmt_ptr((genericptr_t)obj, obj_address),
			doname(obj),
			fmt_ptr((genericptr_t)mon, mon_address),
			mon_nam(mon));
	    }
	    check_contained(obj, mesg);
	}
}

/* This must stay consistent with the defines in obj.h. */
static const char *obj_state_names[NOBJ_STATES] = {
	"free",		"floor",	"contained",	"invent",
	"minvent",	"migrating",	"buried",	"onbill"
};

STATIC_OVL const char *
where_name(where)
    int where;
{
    return (where<0 || where>=NOBJ_STATES) ? "unknown" : obj_state_names[where];
}

/* obj sanity check: check objs contained by container */
STATIC_OVL void
check_contained(container, mesg)
    struct obj *container;
    const char *mesg;
{
    struct obj *obj;
    char obj1_address[20], obj2_address[20];

    for (obj = container->cobj; obj; obj = obj->nobj) {
	if (obj->where != OBJ_CONTAINED)
	    pline("contained %s obj %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj1_address),
		where_name(obj->where));
	else if (obj->ocontainer != container)
	    pline("%s obj %s not in container %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj1_address),
		fmt_ptr((genericptr_t)container, obj2_address));
    }
}
#endif /* WIZARD */

struct obj *
mk_ranked_obj_at(let, x, y, rank, artif)
char let;
int x, y;
int rank;
boolean artif;
{
	struct obj *otmp;
	int otyp;
	int trycnt;

	otmp = (struct obj *)0;
	otyp = 0;
	trycnt = 10;
	if (let == ARMOR_CLASS) {
	    trycnt = 5;
	}
	for (; trycnt; trycnt--) {
	    if (otmp) obfree(otmp, (struct obj *)0);
	    otmp = mkobj(let, artif);
	    if (otmp->oartifact) break;
	    if (objects[otmp->otyp].oc_rank >= rank) break;
	}

	/* try some compensation... */
	if (!trycnt) {
	    switch (otmp->oclass) {
		case WEAPON_CLASS:
		    /* try making it of silver */
		    if (!change_material(otmp, SILVER)) {
			otmp->spe += 2 + rnd(3);
		    }
		    if (otmp->spe < 0) otmp->spe = 0;
		    break;
		case ARMOR_CLASS:
		    /* try making it of silver */
		    if (!change_material(otmp, MITHRIL)) {
			otmp->spe += 2 + rnd(3);
		    }
		    if (otmp->spe < 0) otmp->spe = 0;
		    break;
		case GEM_CLASS:
		    if (get_material(otmp) == GLASS) {
			otyp = otmp->otyp - 9;
		    } else if (otmp->otyp == ROCK) {
			otyp = FLINT;
		    }
		    break;
		case FOOD_CLASS:
		    if (otmp->otyp == TRIPE_RATION) {
			otyp = rn2(3) ? LEMBAS_WAFER : TIN;
		    } else if (otmp->otyp == EGG) {
			otyp = LUMP_OF_ROYAL_JELLY;
		    }
		    break;
		default:
		    break;
	    }
	    if (otyp) {
		obfree(otmp, (struct obj *)0);
		otmp = mksobj(otyp, TRUE, artif);
	    }
	}
	if ((otmp->oclass == WEAPON_CLASS || otmp->oclass == ARMOR_CLASS) &&
	    otmp->spe < 0) otmp->spe = 0;
	if (otmp->cursed) uncurse(otmp);

	place_object(otmp, x, y);
	return(otmp);
}

void
init_portal_ring(otmp)
struct obj *otmp;
{
	struct eportal eptmp;
	int i;

	eptmp.num_slots = 3;
	for (i=0; i<MAX_EPORTAL_SLOT; i++) {
	    eptmp.dests[i].dlev.dnum   = -1;	/* empty slot */
	    eptmp.dests[i].dlev.dlevel = 0;
	    eptmp.dests[i].dx = 0;
	    eptmp.dests[i].dy = 0;
	}
	eptmp.dests[0].dlev = stronghold_level;
	eptmp.dests[0].dx = 41;
	eptmp.dests[0].dy = 11;

	add_xdat_obj(otmp, XDAT_PORTAL, &eptmp);
}


/*mkobj.c*/
