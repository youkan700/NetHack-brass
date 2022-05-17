/*	SCCS Id: @(#)wizard.c	3.4	2003/02/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* wizard code - inspired by rogue code from Merlyn Leroy (digi-g!brian) */
/*	       - heavily modified to give the wiz balls.  (genat!mike)   */
/*	       - dewimped and given some maledictions. -3. */
/*	       - generalized for 3.1 (mike@bullns.on01.bull.ca) */

#include "hack.h"
#include "qtext.h"
#include "epri.h"

extern const int monstr[];

#ifdef OVLB

STATIC_DCL short FDECL(which_arti, (int));
STATIC_DCL boolean FDECL(mon_has_arti, (struct monst *,SHORT_P));
STATIC_DCL struct monst *FDECL(other_mon_has_arti, (struct monst *,SHORT_P));
STATIC_DCL struct obj *FDECL(on_ground, (SHORT_P));
STATIC_DCL boolean FDECL(you_have, (int));
STATIC_DCL long FDECL(target_on, (int,struct monst *));
STATIC_DCL long FDECL(strategy, (struct monst *));

static NEARDATA const int nasties[] = {
	PM_COCKATRICE, PM_ETTIN, PM_STALKER, PM_MINOTAUR, PM_RED_DRAGON,
	PM_BLACK_DRAGON, PM_GREEN_DRAGON, PM_OWLBEAR, PM_PURPLE_WORM,
	PM_ROCK_TROLL, PM_XAN, PM_GREMLIN, PM_UMBER_HULK, PM_VAMPIRE_LORD,
	PM_XORN, PM_ZRUTY, PM_ELF_LORD, PM_ELVENKING, PM_YELLOW_DRAGON,
	PM_LEOCROTTA, PM_BALUCHITHERIUM, PM_CARNIVOROUS_APE, PM_FIRE_GIANT,
	PM_COUATL, PM_CAPTAIN, PM_WINGED_GARGOYLE, PM_MASTER_MIND_FLAYER,
	PM_FIRE_ELEMENTAL, PM_JABBERWOCK, PM_ARCH_LICH, PM_OGRE_KING,
	PM_OLOG_HAI, PM_IRON_GOLEM, PM_OCHRE_JELLY, PM_GREEN_SLIME,
	PM_DISENCHANTER
	};

static NEARDATA const unsigned wizapp[] = {
	PM_HUMAN, PM_WATER_DEMON, PM_VAMPIRE,
	PM_RED_DRAGON, PM_TROLL, PM_UMBER_HULK,
	PM_XORN, PM_XAN, PM_COCKATRICE,
	PM_FLOATING_EYE,
	PM_GUARDIAN_NAGA,
	PM_TRAPPER
};

#endif /* OVLB */
#ifdef OVL0

/* If you've found the Amulet, make the Wizard appear after some time */
/* Also, give hints about portal locations, if amulet is worn/wielded -dlc */
void
amulet()
{
	struct monst *mtmp;
	struct trap *ttmp;
	struct obj *amu;

#if 0		/* caller takes care of this check */
	if (!u.uhave.amulet)
		return;
#endif
	if ((((amu = uamul) != 0 && amu->otyp == AMULET_OF_YENDOR) ||
	     ((amu = uwep) != 0 && amu->otyp == AMULET_OF_YENDOR))
	    && !rn2(15)) {
	    for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
		if(ttmp->ttyp == MAGIC_PORTAL) {
		    int du = distu(ttmp->tx, ttmp->ty);
		    if (du <= 9)
#ifndef JP
			pline("%s hot!", Tobjnam(amu, "feel"));
#else
			pline("%s���M���Ȃ����I", xname(amu));
#endif /*JP*/
		    else if (du <= 64)
#ifndef JP
			pline("%s very warm.", Tobjnam(amu, "feel"));
#else
			pline("%s���ƂĂ��g�����Ȃ����B", xname(amu));
#endif /*JP*/
		    else if (du <= 144)
#ifndef JP
			pline("%s warm.", Tobjnam(amu, "feel"));
#else
			pline("%s���g�����Ȃ����B", xname(amu));
#endif /*JP*/
		    /* else, the amulet feels normal */
		    break;
		}
	    }
	}

	if (!flags.no_of_wizards)
		return;
	/* find Wizard, and wake him if necessary */
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp) && mtmp->iswiz && mtmp->msleeping && !rn2(40)) {
		mtmp->msleeping = 0;
		if (distu(mtmp->mx,mtmp->my) > 2)
		    You(
    E_J("get the creepy feeling that somebody noticed your taking the Amulet.",
	"�A�������̏�����N���Ɋ��Â��ꂽ�Ƃ������C���������o�����ڂ����B")
		    );
		return;
	    }
}

#endif /* OVL0 */
#ifdef OVLB

int
mon_has_amulet(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == AMULET_OF_YENDOR) return(1);
	return(0);
}

int
mon_has_special(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == AMULET_OF_YENDOR ||
			is_quest_artifact(otmp) ||
			otmp->otyp == BELL_OF_OPENING ||
			otmp->otyp == CANDELABRUM_OF_INVOCATION ||
			otmp->otyp == SPE_BOOK_OF_THE_DEAD) return(1);
	return(0);
}

/*
 *	New for 3.1  Strategy / Tactics for the wiz, as well as other
 *	monsters that are "after" something (defined via mflag3).
 *
 *	The strategy section decides *what* the monster is going
 *	to attempt, the tactics section implements the decision.
 */
#define STRAT(w, x, y, typ) \
	(w | (((long)(x)<<16) & STRAT_XMASK) | (((long)(y)<<8) & STRAT_YMASK) | (long)typ)

#define M_Wants(mask)	(mtmp->data->mflags3 & (mask))

STATIC_OVL short
which_arti(mask)
	register int mask;
{
	switch(mask) {
	    case M3_WANTSAMUL:	return(AMULET_OF_YENDOR);
	    case M3_WANTSBELL:	return(BELL_OF_OPENING);
	    case M3_WANTSCAND:	return(CANDELABRUM_OF_INVOCATION);
	    case M3_WANTSBOOK:	return(SPE_BOOK_OF_THE_DEAD);
	    default:		break;	/* 0 signifies quest artifact */
	}
	return(0);
}

/*
 *	If "otyp" is zero, it triggers a check for the quest_artifact,
 *	since bell, book, candle, and amulet are all objects, not really
 *	artifacts right now.	[MRS]
 */
STATIC_OVL boolean
mon_has_arti(mtmp, otyp)
	register struct monst *mtmp;
	register short	otyp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
	    if(otyp) {
		if(otmp->otyp == otyp)
			return(1);
	    }
	     else if(is_quest_artifact(otmp)) return(1);
	}
	return(0);

}

STATIC_OVL struct monst *
other_mon_has_arti(mtmp, otyp)
	register struct monst *mtmp;
	register short	otyp;
{
	register struct monst *mtmp2;

	for(mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon)
	    /* no need for !DEADMONSTER check here since they have no inventory */
	    if(mtmp2 != mtmp)
		if(mon_has_arti(mtmp2, otyp)) return(mtmp2);

	return((struct monst *)0);
}

STATIC_OVL struct obj *
on_ground(otyp)
	register short	otyp;
{
	register struct obj *otmp;

	for (otmp = fobj; otmp; otmp = otmp->nobj)
	    if (otyp) {
		if (otmp->otyp == otyp)
		    return(otmp);
	    } else if (is_quest_artifact(otmp))
		return(otmp);
	return((struct obj *)0);
}

STATIC_OVL boolean
you_have(mask)
	register int mask;
{
	switch(mask) {
	    case M3_WANTSAMUL:	return(boolean)(u.uhave.amulet);
	    case M3_WANTSBELL:	return(boolean)(u.uhave.bell);
	    case M3_WANTSCAND:	return(boolean)(u.uhave.menorah);
	    case M3_WANTSBOOK:	return(boolean)(u.uhave.book);
	    case M3_WANTSARTI:	return(boolean)(u.uhave.questart);
	    default:		break;
	}
	return(0);
}

STATIC_OVL long
target_on(mask, mtmp)
	register int mask;
	register struct monst *mtmp;
{
	register short	otyp;
	register struct obj *otmp;
	register struct monst *mtmp2;

	if(!M_Wants(mask))	return(STRAT_NONE);

	otyp = which_arti(mask);
	if(!mon_has_arti(mtmp, otyp)) {
	    if(you_have(mask))
		return(STRAT(STRAT_PLAYER, u.ux, u.uy, mask));
	    else if((otmp = on_ground(otyp)))
		return(STRAT(STRAT_GROUND, otmp->ox, otmp->oy, mask));
	    else if((mtmp2 = other_mon_has_arti(mtmp, otyp)))
		return(STRAT(STRAT_MONSTR, mtmp2->mx, mtmp2->my, mask));
	}
	return(STRAT_NONE);
}

STATIC_OVL long
strategy(mtmp)
	register struct monst *mtmp;
{
	long strat, dstrat;

	if (!is_covetous(mtmp->data) ||
		/* perhaps a shopkeeper has been polymorphed into a master
		   lich; we don't want it teleporting to the stairs to heal
		   because that will leave its shop untended */
		(mtmp->isshk && inhishop(mtmp)))
	    return STRAT_NONE;

	switch((mtmp->mhp*3)/mtmp->mhpmax) {	/* 0-3 */

	   default:
	    case 0:	/* panic time - mtmp is almost snuffed */
			return(STRAT_HEAL);

	    case 1:	/* the wiz is less cautious */
			if(mtmp->data != &mons[PM_WIZARD_OF_YENDOR])
			    return(STRAT_HEAL);
			/* else fall through */

	    case 2:	dstrat = STRAT_HEAL;
			break;

	    case 3:	dstrat = STRAT_NONE;
			break;
	}

	if(u.uevent.ufound_amulet/*flags.made_amulet*/)
	    if((strat = target_on(M3_WANTSAMUL, mtmp)) != STRAT_NONE)
		return(strat);

	if(u.uevent.invoked) {		/* priorities change once gate opened */

	    if((strat = target_on(M3_WANTSARTI, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSBOOK, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSBELL, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSCAND, mtmp)) != STRAT_NONE)
		return(strat);
	} else {

	    if((strat = target_on(M3_WANTSBOOK, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSBELL, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSCAND, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSARTI, mtmp)) != STRAT_NONE)
		return(strat);
	}
	return(dstrat);
}

int
tactics(mtmp)
	struct monst *mtmp;
{
	long strat = strategy(mtmp);

	/* only monsters who have teleport control can jump to you */
	if (!can_teleport(mtmp->data) || !control_teleport(mtmp->data)) {
	    /* if he arrived to the goal, allow him to pick up obj */
	    if ((strat & STRAT_STRATMASK) != STRAT_GROUND ||
		mtmp->mx != STRAT_GOALX(strat) ||
		mtmp->my != STRAT_GOALY(strat)) return(0);
	}

	switch (strat) {
	    case STRAT_HEAL:	/* hide and recover */
		/* if wounded, hole up on or near the stairs (to block them) */
		/* unless, of course, there are no stairs (e.g. endlevel) */
		mtmp->mavenge = 1; /* covetous monsters attack while fleeing */
		if (In_W_tower(mtmp->mx, mtmp->my, &u.uz) ||
			(mtmp->iswiz && !xupstair && !mon_has_amulet(mtmp))) {
		    if (!rn2(3 + mtmp->mhp/10)) (void) rloc(mtmp, FALSE);
		} else if (xupstair &&
			 (mtmp->mx != xupstair || mtmp->my != yupstair)) {
		    (void) mnearto(mtmp, xupstair, yupstair, TRUE);
		}
		/* if you're not around, cast healing spells */
		if (distu(mtmp->mx,mtmp->my) > (BOLT_LIM * BOLT_LIM))
		    if(mtmp->mhp <= mtmp->mhpmax - 8) {
			mlosehp(mtmp, -rnd(8));
			return(1);
		    }
		/* fall through :-) */

	    case STRAT_NONE:	/* harrass */
		if (!rn2(!mtmp->mflee ? 5 : 33)) mnexto(mtmp);
		return(0);

	    default:		/* kill, maim, pillage! */
	    {
		long  where = (strat & STRAT_STRATMASK);
		xchar tx = STRAT_GOALX(strat),
		      ty = STRAT_GOALY(strat);
		int   targ = strat & STRAT_GOAL;
		struct obj *otmp;

		if(!targ) { /* simply wants you to close */
		    return(0);
		}
		if((u.ux == tx && u.uy == ty) || where == STRAT_PLAYER) {
		    /* player is standing on it (or has it) */
		    mnexto(mtmp);
		    return(0);
		}
		if(where == STRAT_GROUND) {
		    if(!MON_AT(tx, ty) || (mtmp->mx == tx && mtmp->my == ty)) {
			/* teleport to it and pick it up */
			rloc_to(mtmp, tx, ty);	/* clean old pos */

			if ((otmp = on_ground(which_arti(targ))) != 0) {
			    if (cansee(mtmp->mx, mtmp->my))
				pline(E_J("%s picks up %s.",
					  "%s��%s���E�����B"),
				    Monnam(mtmp),
				    (distu(mtmp->mx, mtmp->my) <= 5) ?
				     doname(otmp) : distant_name(otmp, doname));
			    obj_extract_self(otmp);
			    (void) mpickobj(mtmp, otmp);
			    return(1);
			} else return(0);
		    } else {
			/* a monster is standing on it - cause some trouble */
			if (!rn2(5)) mnexto(mtmp);
			return(0);
		    }
	        } else { /* a monster has it - 'port beside it. */
		    (void) mnearto(mtmp, tx, ty, FALSE);
		    return(0);
		}
	    }
	}
	/*NOTREACHED*/
	return(0);
}

void
aggravate()
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp)) {
		mtmp->msleeping = 0;
		if(!mtmp->mcanmove && !rn2(5)) {
			mtmp->mfrozen = 0;
			mtmp->mcanmove = 1;
		}
	    }
}

void
clonewiz()
{
	register struct monst *mtmp2;

	if ((mtmp2 = makemon(&mons[PM_WIZARD_OF_YENDOR],
				u.ux, u.uy, NO_MM_FLAGS)) != 0) {
	    mtmp2->msleeping = mtmp2->mtame = mtmp2->mpeaceful = 0;
	    if (!u.uhave.amulet && rn2(2)) {  /* give clone a fake */
		(void) add_to_minv(mtmp2, mksobj(FAKE_AMULET_OF_YENDOR,
					TRUE, FALSE));
	    }
	    mtmp2->m_ap_type = M_AP_MONSTER;
	    mtmp2->mappearance = wizapp[rn2(SIZE(wizapp))];
	    newsym(mtmp2->mx,mtmp2->my);
	}
}

/* also used by newcham() */
int
pick_nasty()
{
    /* To do?  Possibly should filter for appropriate forms when
       in the elemental planes or surrounded by water or lava. */
    return nasties[rn2(SIZE(nasties))];
}

/* create some nasty monsters, aligned or neutral with the caster */
/* a null caster defaults to a chaotic caster (e.g. the wizard) */
int
nasty(mcast)
	struct monst *mcast;
{
    register struct monst	*mtmp;
    register int	i, j, tmp;
    int castalign = (mcast ? mcast->data->maligntyp : -1);
    coord bypos;
    int count=0;

    if(!rn2(10) && Inhell) {
	msummon((struct monst *) 0);	/* summons like WoY */
	count++;
    } else {
	tmp = (u.ulevel > 3) ? u.ulevel/3 : 1; /* just in case -- rph */
	/* if we don't have a casting monster, the nasties appear around you */
	bypos.x = u.ux;
	bypos.y = u.uy;
	for(i = rnd(tmp); i > 0; --i)
	    for(j=0; j<20; j++) {
		int makeindex;

		/* Don't create more spellcasters of the monsters' level or
		 * higher--avoids chain summoners filling up the level.
		 */
		do {
		    makeindex = pick_nasty();
		} while(mcast && attacktype(&mons[makeindex], AT_MAGC) &&
			monstr[makeindex] >= monstr[mcast->mnum]);
		/* do this after picking the monster to place */
		if (mcast &&
		    !enexto(&bypos, mcast->mux, mcast->muy, &mons[makeindex]))
		    continue;
		if ((mtmp = makemon(&mons[makeindex],
				    bypos.x, bypos.y, MM_MONSTEEDOK/*NO_MM_FLAGS*/)) != 0) {
		    mtmp->msleeping = mtmp->mtame = 0;
		    setmpeaceful(mtmp, FALSE);
		} else /* GENOD? */
		    mtmp = makemon((struct permonst *)0,
					bypos.x, bypos.y, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
		if(mtmp && (mtmp->data->maligntyp == 0 ||
		            sgn(mtmp->data->maligntyp) == sgn(castalign)) ) {
		    count++;
		    break;
		}
	    }
    }
    return count;
}

/*	Let's resurrect the wizard, for some unexpected fun.	*/
void
resurrect()
{
	struct monst *mtmp, **mmtmp;
	long elapsed;
	const char *verb;

	if (!flags.no_of_wizards) {
	    /* make a new Wizard */
	    verb = E_J("kill","���E����");
	    mtmp = makemon(&mons[PM_WIZARD_OF_YENDOR], u.ux, u.uy, MM_NOWAIT);
	} else {
	    /* look for a migrating Wizard */
	    verb = E_J("elude","�̎�𓦂����");
	    mmtmp = &migrating_mons;
	    while ((mtmp = *mmtmp) != 0) {
		if (mtmp->iswiz &&
			/* if he has the Amulet, he won't bring it to you */
			!mon_has_amulet(mtmp) &&
			(elapsed = monstermoves - mtmp->mlstmv) > 0L) {
		    mon_catchup_elapsed_time(mtmp, elapsed);
		    if (elapsed >= LARGEST_INT) elapsed = LARGEST_INT - 1;
		    elapsed /= 50L;
		    if (mtmp->msleeping && rn2((int)elapsed + 1))
			mtmp->msleeping = 0;
		    if (mtmp->mfrozen == 1) /* would unfreeze on next move */
			mtmp->mfrozen = 0,  mtmp->mcanmove = 1;
		    if (mtmp->mcanmove && !mtmp->msleeping) {
			*mmtmp = mtmp->nmon;
			mon_arrive(mtmp, TRUE);
			/* note: there might be a second Wizard; if so,
			   he'll have to wait til the next resurrection */
			break;
		    }
		}
		mmtmp = &mtmp->nmon;
	    }
	}

	if (mtmp) {
		mtmp->msleeping = mtmp->mtame = 0;
		setmpeaceful(mtmp, FALSE);
		pline(E_J("A voice booms out...","���������n�����c�B"));
		verbalize(E_J("So thou thought thou couldst %s me, fool.",
			      "�]%s�Ǝv�������A�����҂߂��B"), verb);
	}

}

/*	Here, we make trouble for the poor shmuck who actually	*/
/*	managed to do in the Wizard.				*/
void
intervene()
{
	int which = Is_astralevel(&u.uz) ? rnd(4) : rn2(6);
	/* cases 0 and 5 don't apply on the Astral level */
	switch (which) {
	    case 0:
	    case 1:	You_feel(E_J("vaguely nervous.",
				     "�Ȃ�ƂȂ��s���ɂȂ����B"));
			break;
	    case 2:	if (!Blind)
			    You(E_J("notice a %s glow surrounding you.",
				    "%s�Ȃ肪���Ȃ�����芪�����̂ɋC�Â����B"),
				  hcolor(NH_BLACK));
			rndcurse();
			break;
	    case 3:	aggravate();
			break;
	    case 4:	(void)nasty((struct monst *)0);
			break;
	    case 5:	resurrect();
			break;
	}
}

void
wizdead()
{
	flags.no_of_wizards--;
	if (!u.uevent.udemigod) {
		u.uevent.udemigod = TRUE;
		u.udg_cnt = rn1(250, 50);
	}
}

const char * const random_insult[] = {
	E_J("antic",		"�����s�R��"),
	E_J("blackguard",	"�����"),
	E_J("caitiff",		"�ڗ�ȓz"),
	E_J("chucklehead",	"�\����"),
	E_J("coistrel",		"�����̂�"),
	E_J("craven",		"�ӋC�n�Ȃ�"),
	E_J("cretin",		"����"),
	E_J("cur",		"���O"),
	E_J("dastard",		"���a��"),
	E_J("demon fodder",	"�����̃G�T"),
	E_J("dimwit",		"���s"),
	E_J("dolt",		"����"),
	E_J("fool",		"�n����"),
	E_J("footpad",		"�`���s��"),
	E_J("imbecile",		"�Ԕ���"),
	E_J("knave",		"���˂Ȏ�"),
	E_J("maledict",		"�u�a�_"),
	E_J("miscreant",	"���}"),
	E_J("niddering",	"�r�r����"),
	E_J("poltroon",		"�ڋ���"),
	E_J("rattlepate",	"��������Y"),
	E_J("reprobate",	"������"),
	E_J("scapegrace",	"�낭�łȂ�"),
	E_J("varlet",		"���Y"),
	E_J("villein",		"�z��"),	/* (sic.) */
	E_J("wittol",		"�Q�����Y"),
	E_J("worm",		"�v��"),
	E_J("wretch",		"�l�łȂ�"),
};

const char * const random_malediction[] = {
	E_J("Hell shall soon claim thy remains,",	"�n���������ɂ��O�̖S�[�𐿋����悤�A"),
	E_J("I chortle at thee, thou pathetic",		"�΂킹�Ă����A���̈����"),
	E_J("Prepare to die, thou",			"���ʗp�ӂ͂������H ����"),
	E_J("Resistance is useless,",			"��R�͖��Ӗ����A"),
	E_J("Surrender or die, thou",			"�~���������A"),
	E_J("There shall be no mercy, thou",		"��Ȃǂ����ʂ��A����"),
	E_J("Thou shalt repent of thy cunning,",	"���O�͎����̃P�`�Ȍv����������邱�ƂɂȂ낤�A"),
	E_J("Thou art as a flea to me,",		"���O�Ȃǔa�ɓ������A"),
	E_J("Thou art doomed,",				"���O�͔j�ł����A"),
	E_J("Thy fate is sealed,",			"���O�̉^���͕����ꂽ�B"),
	E_J("Verily, thou shalt be one dead",		"�܂��ɂ��O�͎��l�ƂȂ낤�A")
};

#ifdef JP
const char * const random_suffix[] = {
	"", "��", "��", "�߂�"
};
#endif /*JP*/

/* Insult or intimidate the player */
void
cuss(mtmp)
register struct monst	*mtmp;
{
	if (mtmp->iswiz) {
	    if (!rn2(5))  /* typical bad guy action */
		pline(E_J("%s laughs fiendishly.",
			  "%s�͙��΂𗁂т����B"), Monnam(mtmp));
	    else
		if (u.uhave.amulet && !rn2(SIZE(random_insult)))
#ifndef JP
		    verbalize("Relinquish the amulet, %s!",
			  random_insult[rn2(SIZE(random_insult))]);
#else
		    verbalize("���������������A%s%s�I",
			  random_insult[rn2(SIZE(random_insult))],
			  random_suffix[rn2(SIZE(random_suffix))]);
#endif /*JP*/
		else if (u.uhp < 5 && !rn2(2))	/* Panic */
#ifndef JP
		    verbalize(rn2(2) ?
			  "Even now thy life force ebbs, %s!" :
			  "Savor thy breath, %s, it be thy last!",
			  random_insult[rn2(SIZE(random_insult))]);
#else
		    verbalize(rn2(2) ?
			  "�������O�̐����͂͊��オ����邼�A%s%s�I" :
			  "�Ŋ��̈ꑧ���\�����키�������A%s%s�I",
			  random_insult[rn2(SIZE(random_insult))],
			  random_suffix[rn2(SIZE(random_suffix))]);
#endif /*JP*/
		else if (mtmp->mhp < 5 && !rn2(2))	/* Parthian shot */
		    verbalize(rn2(2) ?
			      E_J("I shall return.","�]�͕K���߂��Ă���B") :
			      E_J("I'll be back.","���̎؂�͕Ԃ��B"));
		else
#ifndef JP
		    verbalize("%s %s!",
			  random_malediction[rn2(SIZE(random_malediction))],
			  random_insult[rn2(SIZE(random_insult))]);
#else
		    verbalize("%s%s%s�I",
			  random_malediction[rn2(SIZE(random_malediction))],
			  random_insult[rn2(SIZE(random_insult))],
			  random_suffix[rn2(SIZE(random_suffix))]);
#endif /*JP*/
	} else if(is_lminion(mtmp)) {
		com_pager(rn2(QTN_ANGELIC - 1 + (Hallucination ? 1 : 0)) +
			      QT_ANGELIC);
	} else {
	    if (!rn2(5))
		pline(E_J("%s casts aspersions on your ancestry.",
			  "%s�͂��Ȃ��̌��؂𒆏������B"), Monnam(mtmp));
	    else
	        com_pager(rn2(QTN_DEMONIC) + QT_DEMONIC);
	}
}

#endif /* OVLB */

/*wizard.c*/