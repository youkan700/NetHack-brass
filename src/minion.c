/*	SCCS Id: @(#)minion.c	3.4	2003/01/09	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "epri.h"

void
msummon(mon)		/* mon summons a monster */
struct monst *mon;
{
	register struct permonst *ptr;
	register int dtype = NON_PM, cnt = 0;
	aligntyp atyp;
	struct monst *mtmp;

	if (mon) {
	    ptr = mon->data;

	    if (uwep && uwep->oartifact == ART_DEMONBANE && is_demon(ptr)) {
		if (canseemon(mon))
		    pline(E_J("%s looks puzzled for a moment.",
			      "%sは少し困惑したようだ。"), Monnam(mon));
		return;
	    }

	    atyp = getmaligntyp(mon);
	} else {
	    ptr = &mons[PM_WIZARD_OF_YENDOR];
	    atyp = (ptr->maligntyp==A_NONE) ? A_NONE : sgn(ptr->maligntyp);
	}

	if (is_dprince(ptr) || (ptr->mnum == PM_WIZARD_OF_YENDOR)) {
	    dtype = (!rn2(20)) ? dprince(atyp) :
				 (!rn2(4)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_dlord(ptr)) {
	    dtype = (!rn2(50)) ? dprince(atyp) :
				 (!rn2(20)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_ndemon(ptr)) {
	    dtype = (!rn2(20)) ? dlord(atyp) :
				 (!rn2(6)) ? ndemon(atyp) : monsndx(ptr);
	    cnt = 1;
	} else if (is_lminion(mon)) {
	    dtype = (is_lord(ptr) && !rn2(20)) ? llord() :
		     (is_lord(ptr) || !rn2(6)) ? lminion() : monsndx(ptr);
	    cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
	} else if (ptr->mnum == PM_ANGEL) {
	    /* non-lawful angels can also summon */
	    if (!rn2(6)) {
		switch (atyp) { /* see summon_minion */
		case A_NEUTRAL:
		    dtype = PM_AIR_ELEMENTAL + rn2(4);
		    break;
		case A_CHAOTIC:
		case A_NONE:
		    dtype = ndemon(atyp);
		    break;
		}
	    } else {
		dtype = PM_ANGEL;
	    }
	    cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
	}

	if (dtype == NON_PM) return;

	/* sanity checks */
	if (cnt > 1 && (mons[dtype].geno & G_UNIQ)) cnt = 1;
	/*
	 * If this daemon is unique and being re-summoned (the only way we
	 * could get this far with an extinct dtype), try another.
	 */
	if (mvitals[dtype].mvflags & G_GONE) {
	    dtype = ndemon(atyp);
	    if (dtype == NON_PM) return;
	}

	while (cnt > 0) {
	    mtmp = makemon(&mons[dtype], u.ux, u.uy, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
	    if (mtmp && (dtype == PM_ANGEL)) {
		/* alignment should match the summoner */
		setmaligntyp(mtmp, atyp);
	    }
	    cnt--;
	}
}

void
summon_minion(alignment, talk)
aligntyp alignment;
boolean talk;
{
    register struct monst *mon;
    int mnum;

    switch ((int)alignment) {
	case A_LAWFUL:
	    mnum = lminion();
	    break;
	case A_NEUTRAL:
	    mnum = PM_AIR_ELEMENTAL + rn2(4);
	    break;
	case A_CHAOTIC:
	case A_NONE:
	    mnum = ndemon(alignment);
	    break;
	default:
	    impossible("unaligned player?");
	    mnum = ndemon(A_NONE);
	    break;
    }
    if (mnum == NON_PM) {
	mon = 0;
    } else {
	struct permonst *pm = &mons[mnum];
	mon = makemon(pm, u.ux, u.uy, NO_MM_FLAGS);
	if (mon) {
	    mon->isminion = TRUE;
	    setmaligntyp(mon, alignment);
	}
    }
    if (mon) {
	if (talk) {
	    pline_The(E_J("voice of %s booms:","%sの声が轟いた:"), align_gname(alignment));
	    verbalize(E_J("Thou shalt pay for thy indiscretion!",
			  "\"汝、己が不法の報いを受けよう！"));
	    if (!Blind)
		pline(E_J("%s appears before you.","%sがあなたの前に現れた。"), Amonnam(mon));
	}
	mon->mpeaceful = FALSE;
	/* don't call set_malign(); player was naughty */
    }
}

#define Athome	(Inhell && !mtmp->cham)

int
demon_talk(mtmp)		/* returns 1 if it won't attack. */
register struct monst *mtmp;
{
	long cash, demand, offer;
	boolean sawmon;

	if (uwep && uwep->oartifact == ART_EXCALIBUR) {
	    pline(E_J("%s looks very angry.","%sは怒りに燃えているようだ。"), Amonnam(mtmp));
	    mtmp->mpeaceful = mtmp->mtame = 0;
	    set_malign(mtmp);
	    newsym(mtmp->mx, mtmp->my);
	    return 0;
	}

	/* Slight advantage given. */
	mtmp->mtrybribe = 0;
	if (is_dprince(mtmp->data) && mtmp->minvis) {
	    sawmon = canspotmons(mtmp);
	    mtmp->minvis = mtmp->perminvis = 0;
	    if (!Blind && !sawmon) pline(E_J("%s appears before you.","%sがあなたの前に現れた。"), Amonnam(mtmp));
	    newsym(mtmp->mx,mtmp->my);
	}
	if (youmonst.data->mlet == S_DEMON) {	/* Won't blackmail their own. */
	    pline(E_J("%s says, \"Good hunting, %s.\"","%sは言った:「よい狩を、わが%sよ。」"),
		  Amonnam(mtmp), flags.female ? E_J("Sister","妹") : E_J("Brother","兄弟"));
	    setmpeaceful(mtmp, TRUE);
	    if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
	    return(1);
	}
	cash = u.ugold;
	demand = (cash * (rnd(80) + 20 * Athome)) /
	    (100 * (1 + (sgn(u.ualign.type) == sgn(mtmp->data->maligntyp))));

	if (!demand) {		/* you have no gold */
	    setmpeaceful(mtmp, FALSE);
	    return 0;
	} else {
	    /* make sure that the demand is unmeetable if the monster
	       has the Amulet, preventing monster from being satisified
	       and removed from the game (along with said Amulet...) */
	    if (mon_has_amulet(mtmp))
		demand = cash + (long)rn1(1000,40);

	    pline(E_J("%s demands %ld %s for safe passage.",
		      "%sは通行料として%ld %sを要求した。"),
		  Amonnam(mtmp), demand, currency(demand));

	    if ((offer = bribe(mtmp)) >= demand) {
		pline(E_J("%s vanishes, laughing about cowardly mortals.",
			  "%sは臆病な定命の者をあざ笑いながら、姿を消した。"),
		      Amonnam(mtmp));
	    } else if (offer > 0L && (long)rnd(40) > (demand - offer)) {
		pline(E_J("%s scowls at you menacingly, then vanishes.",
			  "%sはあなたを恐ろしげに睨みつけると、姿を消した。"),
		      Amonnam(mtmp));
	    } else {
		pline(E_J("%s gets angry...","%sは怒った…。"), Amonnam(mtmp));
		setmpeaceful(mtmp, FALSE);
		return 0;
	    }
	}
	mongone(mtmp);
	return(1);
}

long
bribe(mtmp)
struct monst *mtmp;
{
	char buf[BUFSZ];
	long offer;

	getlin(E_J("How much will you offer?","いくら渡しますか？"), buf);
	if (sscanf(buf, "%ld", &offer) != 1) offer = 0L;

	/*Michael Paddon -- fix for negative offer to monster*/
	/*JAR880815 - */
	if (offer < 0L) {
		You(E_J("try to shortchange %s, but fumble.",
			"%sにイカサマを仕掛けようとしたが、失敗した。"),
			mon_nam(mtmp));
		return 0L;
	} else if (offer == 0L) {
		You(E_J("refuse.","拒否した。"));
		return 0L;
	} else if (offer >= u.ugold) {
		You(E_J("give %s all your gold.",
			"%sに手持ちの金をすべて渡した。"), mon_nam(mtmp));
		offer = u.ugold;
	} else {
		You(E_J("give %s %ld %s.", "%sに%ld %sを渡した。"),
		    mon_nam(mtmp), offer, currency(offer));
	}
	u.ugold -= offer;
	mtmp->mgold += offer;
	flags.botl = 1;
	return(offer);
}

int
dprince(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_DEMOGORGON + 1 - PM_ORCUS, PM_ORCUS);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(dlord(atyp));	/* approximate */
}

int
dlord(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_YEENOGHU + 1 - PM_JUIBLEX, PM_JUIBLEX);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(ndemon(atyp));	/* approximate */
}

/* create lawful (good) lord */
int
llord()
{
	if (!(mvitals[PM_ARCHON].mvflags & G_GONE))
		return(PM_ARCHON);

	return(lminion());	/* approximate */
}

int
lminion()
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_ANGEL,0);
	    if (ptr && !is_lord(ptr))
		return(monsndx(ptr));
	}

	return NON_PM;
}

int
ndemon(atyp)
aligntyp atyp;
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_DEMON, 0);
	    if (ptr && is_ndemon(ptr) &&
		    (atyp == A_NONE || sgn(ptr->maligntyp) == sgn(atyp)))
		return(monsndx(ptr));
	}

	return NON_PM;
}

void
setmaligntyp(mon, mal)
struct monst *mon;
schar mal;
{
	if (mal == A_NONE) {
	    mon->maligntyp = 3;
	} else switch (sgn(mal)) {
	    case A_CHAOTIC:	mon->maligntyp = 0; break;
	    case A_NEUTRAL:	mon->maligntyp = 1; break;
	    case A_LAWFUL:	mon->maligntyp = 2; break;
	}
}

aligntyp
getmaligntyp(mon)
struct monst *mon;
{
	static const aligntyp atbl[4] = {
	    A_CHAOTIC, A_NEUTRAL, A_LAWFUL, A_NONE
	};
	return atbl[mon->maligntyp];
}

/*minion.c*/
