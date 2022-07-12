/*	SCCS Id: @(#)end.c	3.4	2003/03/10	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS	/* comment line for pre-compiled headers */

#include "hack.h"
#include "eshk.h"
#ifndef NO_SIGNAL
#include <signal.h>
#endif
#include "dlb.h"

	/* these probably ought to be generated by makedefs, like LAST_GEM */
#define FIRST_GEM    DILITHIUM_CRYSTAL
#define FIRST_AMULET AMULET_OF_ESP
#define LAST_AMULET  AMULET_OF_YENDOR
 
struct valuable_data { long count; int typ; };

static struct valuable_data
	gems[LAST_GEM+1 - FIRST_GEM + 1], /* 1 extra for glass */
	amulets[LAST_AMULET+1 - FIRST_AMULET];

static struct val_list { struct valuable_data *list; int size; } valuables[] = {
	{ gems,    sizeof gems / sizeof *gems },
	{ amulets, sizeof amulets / sizeof *amulets },
	{ 0, 0 }
};

#ifndef NO_SIGNAL
STATIC_PTR void FDECL(done_intr, (int));
# if defined(UNIX) || defined(VMS) || defined (__EMX__)
static void FDECL(done_hangup, (int));
# endif
#endif
STATIC_DCL void FDECL(disclose,(int,BOOLEAN_P));
STATIC_DCL void FDECL(get_valuables, (struct obj *));
STATIC_DCL void FDECL(sort_valuables, (struct valuable_data *,int));
STATIC_DCL void FDECL(artifact_score, (struct obj *,BOOLEAN_P,winid));
STATIC_DCL void FDECL(savelife, (int));
STATIC_DCL void FDECL(list_vanquished, (CHAR_P,BOOLEAN_P));
STATIC_DCL boolean FDECL(should_query_disclose_option, (int,char *));
#ifdef JP
STATIC_DCL void FDECL(remove_killer_sep, (char *));
#endif

#if defined(__BEOS__) || defined(MICRO) || defined(WIN32) || defined(OS2)
extern void FDECL(nethack_exit,(int));
#else
#define nethack_exit exit
#endif

#define done_stopprint program_state.stopprint

#ifdef AMIGA
# define NH_abort()	Abort(0)
#else
# ifdef SYSV
# define NH_abort()	(void) abort()
# else
#  ifdef WIN32
# define NH_abort()	win32_abort()
#  else
# define NH_abort()	abort()
#  endif
# endif
#endif

#ifndef JP
/*
 * The order of these needs to match the macros in hack.h.
 */
static NEARDATA const char *deaths[] = {		/* the array of death */
	"died", "choked", "poisoned", "starvation", "drowning",
	"burning", "dissolving under the heat and pressure",
	"crushed", "turned to stone", "turned into slime",
	"genocided", "panic", "trickery",
	"quit", "escaped", "ascended"
};

static NEARDATA const char *ends[] = {		/* "when you..." */
	"died", "choked", "were poisoned", "starved", "drowned",
	"burned", "dissolved in the lava",
	"were crushed", "turned to stone", "turned into slime",
	"were genocided", "panicked", "were tricked",
	"quit", "escaped", "ascended"
};
#else /*JP*/
static NEARDATA const char *ends[] = {		/* "when you..." */
	"死んだ", "窒息した", "病死した", "餓死した", "溺死した",
	"焼死した", "溶解した",
	"押し潰された", "石化した", "スライムと化した",
	"虐殺された", "パニックに陥った", "騙された",
	"ゲームを放棄した", "逃げ出した", "昇天した", "殺された"
};
#endif /*JP*/

extern const char * const killed_by_prefix[];	/* from topten.c */

/*ARGSUSED*/
void
done1(sig_unused)   /* called as signal() handler, so sent at least one arg */
int sig_unused;
{
#ifndef NO_SIGNAL
	(void) signal(SIGINT,SIG_IGN);
#endif
	if(flags.ignintr) {
#ifndef NO_SIGNAL
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
		clear_nhwindow(WIN_MESSAGE);
		curs_on_u();
		wait_synch();
		if(multi > 0) nomul(0);
	} else {
		(void)done2();
	}
}


/* "#quit" command or keyboard interrupt */
int
done2()
{
	if(yn(E_J("Really quit?","本当にゲームを放棄しますか？")) == 'n') {
#ifndef NO_SIGNAL
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
		clear_nhwindow(WIN_MESSAGE);
		curs_on_u();
		wait_synch();
		if(multi > 0) nomul(0);
		if(multi == 0) {
		    u.uinvulnerable = FALSE;	/* avoid ctrl-C bug -dlc */
		    u.usleep = 0;
		}
		return 0;
	}
#if defined(WIZARD) && (defined(UNIX) || defined(VMS) || defined(LATTICE))
	if(wizard) {
	    int c;
# ifdef VMS
	    const char *tmp = "Enter debugger?";
# else
#  ifdef LATTICE
	    const char *tmp = "Create SnapShot?";
#  else
	    const char *tmp = "Dump core?";
#  endif
# endif
	    if ((c = ynq(tmp)) == 'y') {
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
		exit_nhwindows((char *)0);
		NH_abort();
	    } else if (c == 'q') done_stopprint++;
	}
#endif
#ifndef LINT
	done(QUIT);
#endif
	return 0;
}

#ifndef NO_SIGNAL
/*ARGSUSED*/
STATIC_PTR void
done_intr(sig_unused) /* called as signal() handler, so sent at least one arg */
int sig_unused;
{
	done_stopprint++;
	(void) signal(SIGINT, SIG_IGN);
# if defined(UNIX) || defined(VMS)
	(void) signal(SIGQUIT, SIG_IGN);
# endif
	return;
}

# if defined(UNIX) || defined(VMS) || defined(__EMX__)
static void
done_hangup(sig)	/* signal() handler */
int sig;
{
	program_state.done_hup++;
	(void)signal(SIGHUP, SIG_IGN);
	done_intr(sig);
	return;
}
# endif
#endif /* NO_SIGNAL */

void
done_in_by(mtmp)
struct monst *mtmp;
{
	char buf[BUFSZ];

	You(E_J("die...","死にました…。"));
	mark_synch();	/* flush buffered screen output */
	setup_killername(mtmp, buf);
#ifdef JP
	Strcat(buf, "に");
#endif /*JP*/

	if (multi) Strcat(buf, E_J(", while helpless","\tなすすべも\tなく\t"));

	killer = buf;
	if (mtmp->data->mlet == S_WRAITH)
		u.ugrave_arise = PM_WRAITH;
	else if (mtmp->data->mlet == S_MUMMY && urace.mummynum != NON_PM)
		u.ugrave_arise = urace.mummynum;
	else if (mtmp->data->mlet == S_VAMPIRE && Race_if(PM_HUMAN))
		u.ugrave_arise = PM_VAMPIRE;
	else if (mtmp->mnum == PM_GHOUL)
		u.ugrave_arise = PM_GHOUL;
	if (u.ugrave_arise >= LOW_PM &&
				(mvitals[u.ugrave_arise].mvflags & G_GENOD))
		u.ugrave_arise = NON_PM;
	if (touch_petrifies(mtmp->data))
#ifndef JP
		done(STONING);
	else
		done(DIED);
#else
	{
	    Strcat(buf, "石化させられた");
	    killer_format = NO_KILLER_PREFIX;
	    done(STONING);
	} else {
	    /*JP
	      DIED の場合は通常 "死んだ" が補われるが、
	      怪物による場合は "殺された" を補う。
	    */
	    killer_format = KILLED_SUFFIX;
	    done(DIED);
	}
#endif /*JP*/
	return;
}

void setup_killername(mtmp, buf)
struct monst *mtmp;
char *buf;
{
	boolean distorted = (boolean)(Hallucination && canspotmon(mtmp));

	buf[0] = '\0';
	killer_format = KILLED_BY_AN;
#ifndef JP
	/* "killed by the high priest of Crom" is okay, "killed by the high
	   priest" alone isn't */
	if ((mtmp->data->geno & G_UNIQ) != 0 && !(mtmp->mnum == PM_HIGH_PRIEST && !mtmp->ispriest)) {
	    if (!type_is_pname(mtmp->data))
		Strcat(buf, "the ");
	    killer_format = KILLED_BY;
	}
	/* _the_ <invisible> <distorted> ghost of Dudley */
	if (mtmp->mnum == PM_GHOST && mtmp->has_name) {
		Strcat(buf, "the ");
		killer_format = KILLED_BY;
	}
#endif /*JP*/
	if (mtmp->minvis)
		Strcat(buf, E_J("invisible ","透明な"));
	if (distorted)
		Strcat(buf, E_J("hallucinogen-distorted ","幻覚で歪んだ"));

	if(mtmp->mnum == PM_GHOST) {
#ifndef JP
		Strcat(buf, "ghost");
		if (mtmp->has_name) Sprintf(eos(buf), " of %s", NAME(mtmp));
#else
		if (mtmp->has_name) Sprintf(eos(buf), "%sの", NAME(mtmp));
		Strcat(buf, "幽霊");
#endif /*JP*/
	} else if(mtmp->isshk) {
#ifndef JP
		Sprintf(eos(buf), "%s %s, the shopkeeper",
			(mtmp->female ? "Ms." : "Mr."), shkname(mtmp));
#else
		Sprintf(eos(buf), "%sという名の%s店主",
			shkname(mtmp), (mtmp->female ? "女" : ""));
#endif /*JP*/
		killer_format = KILLED_BY;
	} else if (mtmp->ispriest || mtmp->isminion) {
		/* m_monnam() suppresses "the" prefix plus "invisible", and
		   it overrides the effect of Hallucination on priestname() */
		killer = m_monnam(mtmp);
		Strcat(buf, killer);
	} else if (is_animobj(mtmp->data)) {
		Sprintf(eos(buf), E_J("dancing %s", "宙を舞う%s"), xname(mtmp->mw));
	} else {
#ifndef JP
		Strcat(buf, mtmp->data->mname);
		if (mtmp->has_name)
		    Sprintf(eos(buf), " called %s", NAME(mtmp));
#else
		if (mtmp->has_name)
		    Sprintf(eos(buf), "%sという名の", NAME(mtmp));
		if (mtmp->cham == CHAM_DOPPELGANGER && is_mplayer(mtmp->data) &&
		    mtmp->data == &mons[u.umonster]) {
		    Strcat(buf, "あなた");
		} else
		if (mtmp->mnum == PM_MASTER_MIND_FLAYER)
		    Strcat(buf, "マスター\tマインドフレア");
		else if (mtmp->mnum == PM_GIANT_BEETLE)
		    Strcat(buf, "ジャイアント\tビートル");
		else
		    Strcat(buf, JMONNAM(monsndx(mtmp->data)));
		if (mtmp->cham != CHAM_ORDINARY) {
		    Sprintf(eos(buf), "に化けた%s", JMONNAM(get_true_pm(mtmp)));
		}
#endif /*JP*/
	}
}

#if defined(WIN32)
#define NOTIFY_NETHACK_BUGS
#endif

/*VARARGS1*/
void
panic VA_DECL(const char *, str)
	VA_START(str);
	VA_INIT(str, char *);

	if (program_state.panicking++)
	    NH_abort();	/* avoid loops - this should never happen*/

	if (iflags.window_inited) {
	    raw_print("\r\nOops...");
	    wait_synch();	/* make sure all pending output gets flushed */
	    exit_nhwindows((char *)0);
	    iflags.window_inited = 0; /* they're gone; force raw_print()ing */
	}

#ifndef JP
	raw_print(program_state.gameover ?
		  "Postgame wrapup disrupted." :
		  !program_state.something_worth_saving ?
		  "Program initialization has failed." :
		  "Suddenly, the dungeon collapses.");
#else
	raw_print(program_state.gameover ?
		  "ゲームの終了処理が崩壊した。" :
		  !program_state.something_worth_saving ?
		  "プログラムの初期化に失敗した。" :
		  "突然、迷宮が崩壊した。");
#endif /*JP*/
#if defined(WIZARD) && !defined(MICRO)
# if defined(NOTIFY_NETHACK_BUGS)
	if (!wizard)
	    raw_printf(E_J("Report the following error to \"%s\".",
			   "エラーを %s に報告してください。"),
			"http://www11.cds.ne.jp/~youkan/nethack/"/*"nethack-bugs@nethack.org"*/);
	else if (program_state.something_worth_saving)
	    raw_print(E_J("\nError save file being written.\n",
			  "\nエラーセーブファイルが出力されました。\n"));
# else
	if (!wizard)
	    raw_printf(E_J("Report error to \"%s\"%s.",
			   "エラーを %s に報告してください。%s"),
#  ifdef WIZARD_NAME	/*(KR1ED)*/
			WIZARD_NAME,
#  else
			WIZARD,
#  endif
			!program_state.something_worth_saving ? "" :
			E_J(" and it may be possible to rebuild.",
			    "再ビルドが必要かもしれません。"));
# endif
	if (program_state.something_worth_saving) {
	    set_error_savefile();
	    (void) dosave0();
	}
#endif
	{
	    char buf[BUFSZ];
	    Vsprintf(buf,str,VA_ARGS);
	    raw_print(buf);
	    paniclog("panic", buf);
	}
//#ifdef WIN32
//	interject(INTERJECT_PANIC);
//#endif
#if defined(WIZARD) && (defined(UNIX) || defined(VMS) || defined(LATTICE) || defined(WIN32))
	if (wizard)
	    NH_abort();	/* generate core dump */
#endif
	VA_END();
	done(PANICKED);
}

STATIC_OVL boolean
should_query_disclose_option(category, defquery)
int category;
char *defquery;
{
    int idx;
    char *dop = index(disclosure_options, category);

    if (dop && defquery) {
	idx = dop - disclosure_options;
	if (idx < 0 || idx > (NUM_DISCLOSURE_OPTIONS - 1)) {
	    impossible(
		   "should_query_disclose_option: bad disclosure index %d %c",
		       idx, category);
	    *defquery = DISCLOSE_PROMPT_DEFAULT_YES;
	    return TRUE;
	}
	if (flags.end_disclose[idx] == DISCLOSE_YES_WITHOUT_PROMPT) {
	    *defquery = 'y';
	    return FALSE;
	} else if (flags.end_disclose[idx] == DISCLOSE_NO_WITHOUT_PROMPT) {
	    *defquery = 'n';
	    return FALSE;
	} else if (flags.end_disclose[idx] == DISCLOSE_PROMPT_DEFAULT_YES) {
	    *defquery = 'y';
	    return TRUE;
	} else if (flags.end_disclose[idx] == DISCLOSE_PROMPT_DEFAULT_NO) {
	    *defquery = 'n';
	    return TRUE;
	}
    }
    if (defquery)
	impossible("should_query_disclose_option: bad category %c", category);
    else
	impossible("should_query_disclose_option: null defquery");
    return TRUE;
}

STATIC_OVL void
disclose(how,taken)
int how;
boolean taken;
{
	char	c = 0, defquery;
	char	qbuf[QBUFSZ];
	boolean ask;

	if (invent) {
	    if(taken)
#ifndef JP
		Sprintf(qbuf,"Do you want to see what you had when you %s?",
			(how == QUIT) ? "quit" : "died");
#else
		Sprintf(qbuf,"%s時点で何を持っていたか見ますか？",
			(how == QUIT) ? "放棄した" : "死んだ");
#endif /*JP*/
	    else
		Strcpy(qbuf,E_J("Do you want your possessions identified?",
				"持ち物を識別しますか？"));

	    ask = should_query_disclose_option('i', &defquery);
	    if (!done_stopprint) {
		c = ask ? yn_function(qbuf, ynqchars, defquery) : defquery;
		if (c == 'y') {
			struct obj *obj;

			for (obj = invent; obj; obj = obj->nobj) {
			    makeknown(obj->otyp);
			    obj->known = obj->bknown = obj->dknown = obj->rknown = 1;
			}
			(void) display_inventory((char *)0, TRUE);
			container_contents(invent, TRUE, TRUE);
		}
		if (c == 'q')  done_stopprint++;
	    }
	}

	ask = should_query_disclose_option('a', &defquery);
	if (!done_stopprint) {
	    c = ask ? yn_function(E_J("Do you want to see your attributes?",
				      "属性を見ますか？"),
				  ynqchars, defquery) : defquery;
	    if (c == 'y')
		enlightenment(how >= PANICKED ? 1 : 2); /* final */
	    if (c == 'q') done_stopprint++;
	}

	ask = should_query_disclose_option('v', &defquery);
	if (!done_stopprint)
	    list_vanquished(defquery, ask);

	ask = should_query_disclose_option('g', &defquery);
	if (!done_stopprint)
	    list_genocided(defquery, ask);

	ask = should_query_disclose_option('c', &defquery);
	if (!done_stopprint) {
	    c = ask ? yn_function(E_J("Do you want to see your conduct?",
				      "守った戒律について確認しますか？"),
				  ynqchars, defquery) : defquery;
	    if (c == 'y')
		show_conduct(how >= PANICKED ? 1 : 2);
	    if (c == 'q') done_stopprint++;
	}
}

/* try to get the player back in a viable state after being killed */
STATIC_OVL void
savelife(how)
int how;
{
	u.uswldtim = 0;
	u.uhp = u.uhpmax;
	if (u.uhunger < 500) {
	    u.uhunger = 500;
	    newuhs(FALSE);
	}
	/* cure impending doom of sickness hero won't have time to fix */
	if ((Sick & TIMEOUT) == 1) {
	    u.usick_type = 0;
	    Sick = 0;
	}
	if (how == CHOKING) init_uhunger();
	nomovemsg = E_J("You survived that attempt on your life.",
			"あなたは生命の危機を生き延びた。");
	flags.move = 0;
	if(multi > 0) multi = 0; else multi = -1;
	if(u.utrap && u.utraptype == TT_LAVA) u.utrap = 0;
	flags.botl = 1;
	u.ugrave_arise = NON_PM;
	HUnchanging = 0L;
	curs_on_u();
}

/*
 * Get valuables from the given list.  Revised code: the list always remains
 * intact.
 */
STATIC_OVL void
get_valuables(list)
struct obj *list;	/* inventory or container contents */
{
    register struct obj *obj;
    register int i;

    /* find amulets and gems, ignoring all artifacts */
    for (obj = list; obj; obj = obj->nobj)
	if (Has_contents(obj)) {
	    get_valuables(obj->cobj);
	} else if (obj->oartifact) {
	    continue;
	} else if (obj->oclass == AMULET_CLASS) {
	    i = obj->otyp - FIRST_AMULET;
	    if (!amulets[i].count) {
		amulets[i].count = obj->quan;
		amulets[i].typ = obj->otyp;
	    } else amulets[i].count += obj->quan; /* always adds one */
	} else if (obj->oclass == GEM_CLASS && obj->otyp < LUCKSTONE) {
	    i = min(obj->otyp, LAST_GEM + 1) - FIRST_GEM;
	    if (!gems[i].count) {
		gems[i].count = obj->quan;
		gems[i].typ = obj->otyp;
	    } else gems[i].count += obj->quan;
	}
    return;
}

/*
 *  Sort collected valuables, most frequent to least.  We could just
 *  as easily use qsort, but we don't care about efficiency here.
 */
STATIC_OVL void
sort_valuables(list, size)
struct valuable_data list[];
int size;		/* max value is less than 20 */
{
    register int i, j;
    struct valuable_data ltmp;

    /* move greater quantities to the front of the list */
    for (i = 1; i < size; i++) {
	if (list[i].count == 0) continue;	/* empty slot */
	ltmp = list[i]; /* structure copy */
	for (j = i; j > 0; --j)
	    if (list[j-1].count >= ltmp.count) break;
	    else {
		list[j] = list[j-1];
	    }
	list[j] = ltmp;
    }
    return;
}

/* called twice; first to calculate total, then to list relevant items */
STATIC_OVL void
artifact_score(list, counting, endwin)
struct obj *list;
boolean counting;	/* true => add up points; false => display them */
winid endwin;
{
    char pbuf[BUFSZ];
    struct obj *otmp;
    long value, points;
    short dummy;	/* object type returned by artifact_name() */

    for (otmp = list; otmp; otmp = otmp->nobj) {
	if (otmp->oartifact ||
			otmp->otyp == BELL_OF_OPENING ||
			otmp->otyp == SPE_BOOK_OF_THE_DEAD ||
			otmp->otyp == CANDELABRUM_OF_INVOCATION) {
	    value = arti_cost(otmp);	/* zorkmid value */
	    points = value * 5 / 2;	/* score value */
	    if (counting) {
		u.urexp += points;
	    } else {
		makeknown(otmp->otyp);
		otmp->known = otmp->dknown = otmp->bknown = otmp->rknown = 1;
		/* assumes artifacts don't have quan > 1 */
#ifndef JP
		Sprintf(pbuf, "%s%s (worth %ld %s and %ld points)",
			the_unique_obj(otmp) ? "The " : "",
			otmp->oartifact ? artifact_name(xname(otmp), &dummy, 0) :
				OBJ_NAME(objects[otmp->otyp]),
			value, currency(value), points);
#else
		Sprintf(pbuf, "%s (%ld %s, %ldポイントの価値)",
			otmp->oartifact ? artifact_name(xname(otmp), &dummy, 0) :
				JOBJ_NAME(objects[otmp->otyp]),
			value, currency(value), points);
#endif /*JP*/
		putstr(endwin, 0, pbuf);
	    }
	}
	if (Has_contents(otmp))
	    artifact_score(otmp->cobj, counting, endwin);
    }
}

#ifdef JP
STATIC_OVL void
remove_killer_sep(buf)
char *buf;
{
	char c, *psrc, *pdst;
	psrc = pdst = buf;
	while (*psrc) {
		c = *psrc++;
		if (c != '\t') *pdst++ = c;
	}
	*pdst = 0;
}
#endif

void
set_delayed_killer(cause, format, str)
int cause;
int format;
const char *str;
{
	int i;
	for (i=0; i<MAX_DKILLER; i++)
	    if (delayed_killers[i].killer == cause) goto set_dk;
	for (i=0; i<MAX_DKILLER; i++)
	    if (!delayed_killers[i].killer) break;
	if (i >= MAX_DKILLER) {
	    impossible("set_delayed_killer: No empty slot in delayed_killers[]");
	    return;
	}
set_dk:
	delayed_killers[i].killer = cause;
	delayed_killers[i].killer_format = format;
	Strcpy(delayed_killers[i].delayed_killer, str);
}

char *
get_delayed_killer(cause, format)
int cause;
int *format;
{
	int i;
	for (i=0; i<MAX_DKILLER; i++) {
	    if (delayed_killers[i].killer == cause) {
		if (format) *format = delayed_killers[i].killer_format;
		return delayed_killers[i].delayed_killer;
	    }
	}
	return (char *)0;
}

void
remove_delayed_killer(cause)
int cause;
{
	int i;
	if (!cause) {
	    /* all */
	    for (i=0; i<MAX_DKILLER; i++) {
		delayed_killers[i].killer = 0;
		delayed_killers[i].killer_format = 0;
	    }
	    return;
	}
	for (i=0; i<MAX_DKILLER; i++) {
	    if (delayed_killers[i].killer == cause) {
		delayed_killers[i].killer = 0;
		delayed_killers[i].killer_format = 0;
		return;
	    }
	}
	impossible("remove_delayed_killer: No cause found?");
}

/* Be careful not to call panic from here! */
void
done(how)
int how;
{
	boolean taken;
	char kilbuf[BUFSZ], pbuf[BUFSZ];
	winid endwin = WIN_ERR;
	boolean bones_ok, have_windows = iflags.window_inited;
	struct obj *corpse = (struct obj *)0;
	long umoney;

	if (how == TRICKED) {
	    if (killer) {
		paniclog("trickery", killer);
		killer = 0;
	    }
#ifdef WIZARD
	    if (wizard) {
		You("are a very tricky wizard, it seems.");
		return;
	    }
#endif
	}

	/* kilbuf: used to copy killer in case it comes from something like
	 *	xname(), which would otherwise get overwritten when we call
	 *	xname() when listing possessions
	 * pbuf: holds Sprintf'd output for raw_print and putstr
	 */
	if (how == ASCENDED || (!killer && how == GENOCIDED))
		killer_format = NO_KILLER_PREFIX;
	/* Avoid killed by "a" burning or "a" starvation */
	if (!killer && (how == STARVING || how == BURNING))
		killer_format = KILLED_BY;
	Strcpy(kilbuf, (!killer || how >= PANICKED ? E_J(deaths[how],ends[how]) : killer));
#ifdef JP
	Strcpy(killer_buf_with_separator, kilbuf);
	remove_killer_sep(kilbuf);
#endif /*JP*/
	killer = kilbuf;

	if (how < PANICKED) u.umortality++;
	if (Lifesaved && (how <= GENOCIDED)) {
		pline(E_J("But wait...","だが待て…。"));
		makeknown(AMULET_OF_LIFE_SAVING);
#ifndef JP
		Your("medallion %s!",
		      !Blind ? "begins to glow" : "feels warm");
		if (how == CHOKING) You("vomit ...");
		You_feel("much better!");
		pline_The("medallion crumbles to dust!");
#else
		Your("メダリオンが%sた！",
		      !Blind ? "輝きはじめ" : "暖かさを増し");
		if (how == CHOKING) You("嘔吐した…。");
		You("とても元気になった！");
		pline("メダリオンは砕けて塵となった！");
#endif /*JP*/
		if (uamul) useup(uamul);

		(void) adjattrib(A_CON, -1, TRUE);
		if(u.uhpmax <= 0) u.uhpmax = 10;	/* arbitrary */
		savelife(how);
		if (how == GENOCIDED)
			pline(E_J("Unfortunately you are still genocided...",
				"不幸にも、あなたは虐殺されたままだ…。"));
		else {
			killer = 0;
			killer_format = 0;
			return;
		}
	}
	if ((
#ifdef WIZARD
			wizard ||
#endif
			discover) && (how <= GENOCIDED)) {
		if(yn(E_J("Die?","死ぬ？")) == 'y') goto die;
#ifndef JP
		pline("OK, so you don't %s.",
			(how == CHOKING) ? "choke" : "die");
#else
		pline("了解、じゃああなたは%sなかった。",
			(how == CHOKING) ? "窒息し" : "死な");
#endif /*JP*/
		if(u.uhpmax <= 0) u.uhpmax = u.ulevel * 8;	/* arbitrary */
		savelife(how);
		killer = 0;
		killer_format = 0;
		return;
	}

    /*
     *	The game is now over...
     */

die:
	program_state.gameover = 1;
	/* in case of a subsequent panic(), there's no point trying to save */
	program_state.something_worth_saving = 0;
	/* render vision subsystem inoperative */
	iflags.vision_inited = 0;
	/* might have been killed while using a disposable item, so make sure
	   it's gone prior to inventory disclosure and creation of bones data */
	inven_inuse(TRUE);

	/* Sometimes you die on the first move.  Life's not fair.
	 * On those rare occasions you get hosed immediately, go out
	 * smiling... :-)  -3.
	 */
	if (moves <= 1 && how < PANICKED)	/* You die... --More-- */
#ifndef JP
	    pline("Do not pass go.  Do not collect 200 %s.", currency(200L));
#else
	    pline("注意一秒、死亡一歩。");
#endif /*JP*/

	if (have_windows) wait_synch();	/* flush screen output */
#ifndef NO_SIGNAL
	(void) signal(SIGINT, (SIG_RET_TYPE) done_intr);
# if defined(UNIX) || defined(VMS) || defined (__EMX__)
	(void) signal(SIGQUIT, (SIG_RET_TYPE) done_intr);
	(void) signal(SIGHUP, (SIG_RET_TYPE) done_hangup);
# endif
#endif /* NO_SIGNAL */

	bones_ok = (how < GENOCIDED) && can_make_bones();

	if (how == TURNED_SLIME)
	    u.ugrave_arise = PM_GREEN_SLIME;

	if (bones_ok && u.ugrave_arise < LOW_PM) {
	    /* corpse gets burnt up too */
	    if (how == BURNING)
		u.ugrave_arise = (NON_PM - 2);	/* leave no corpse */
	    else if (how == STONING)
		u.ugrave_arise = (NON_PM - 1);	/* statue instead of corpse */
	    else if (u.ugrave_arise == NON_PM &&
		     !(mvitals[u.umonnum].mvflags & G_NOCORPSE)) {
		int mnum = u.umonnum;

		if (!Upolyd) {
		    /* Base corpse on race when not poly'd since original
		     * u.umonnum is based on role, and all role monsters
		     * are human.
		     */
		    mnum = (flags.female && urace.femalenum != NON_PM) ?
			urace.femalenum : urace.malenum;
		}
		corpse = mk_named_object(CORPSE, &mons[mnum],
				       u.ux, u.uy, plname);
#ifndef JP
		Sprintf(pbuf, "%s, %s%s", plname,
			killer_format == NO_KILLER_PREFIX ? "" :
			killed_by_prefix[how],
			killer_format == KILLED_BY_AN ? an(killer) : killer);
#else
		{
		    const char *p;
		    switch(killer_format){
		      case NO_KILLER_PREFIX:
			p = "";
			break;
		      case KILLED_SUFFIX:
			p = "殺された";
			break;
		      default:
			p = killed_by_prefix[how];
		    }
		    Sprintf(pbuf, "%s%s%sの墓", killer, p, plname);
		}
#endif /*JP*/
		make_grave(u.ux, u.uy, pbuf);
	    }
	}

	if (how == QUIT) {
		killer_format = NO_KILLER_PREFIX;
		if (u.uhp < 1) {
			how = DIED;
			u.umortality++;	/* skipped above when how==QUIT */
			/* note that killer is pointing at kilbuf */
			Strcpy(kilbuf, E_J("quit while already on Charon's boat",
					"カロンの舟の上でゲームを放棄した"));
		}
	}
	if (how == ESCAPED || how == PANICKED)
		killer_format = NO_KILLER_PREFIX;

	if (how != PANICKED) {
	    /* these affect score and/or bones, but avoid them during panic */
	    taken = paybill((how == ESCAPED) ? -1 : (how != QUIT));
	    paygd();
	    clearpriests();
	} else	taken = FALSE;	/* lint; assert( !bones_ok ); */

	clearlocks();

	if (have_windows) display_nhwindow(WIN_MESSAGE, FALSE);

	if (strcmp(flags.end_disclose, "none") && how != PANICKED)
		disclose(how, taken);
	/* finish_paybill should be called after disclosure but before bones */
	if (bones_ok && taken) finish_paybill();

	/* calculate score, before creating bones [container gold] */
	{
	    long tmp;
	    int deepest = deepest_lev_reached(FALSE);

	    umoney = u.ugold;
	    tmp = u.ugold0;
	    umoney += hidden_gold();	/* accumulate gold from containers */
	    tmp = umoney - tmp;		/* net gain */

	    if (tmp < 0L)
		tmp = 0L;
	    if (how < PANICKED)
		tmp -= tmp / 10L;
	    u.urexp += tmp;
	    u.urexp += 50L * (long)(deepest - 1);
	    if (deepest > 20)
		u.urexp += 1000L * (long)((deepest > 30) ? 10 : deepest - 20);
	    if (how == ASCENDED) u.urexp *= 2L;
	}

	if (bones_ok) {
#ifdef WIZARD
	    if (!wizard || yn(E_J("Save bones?","骨ファイルを保存する？")) == 'y')
#endif
		savebones(corpse);
	    /* corpse may be invalid pointer now so
		ensure that it isn't used again */
	    corpse = (struct obj *)0;
	}

	/* update gold for the rip output, which can't use hidden_gold()
	   (containers will be gone by then if bones just got saved...) */
	u.ugold = umoney;

	/* clean up unneeded windows */
	if (have_windows) {
	    wait_synch();
	    display_nhwindow(WIN_MESSAGE, TRUE);
	    destroy_nhwindow(WIN_MAP);
	    destroy_nhwindow(WIN_STATUS);
	    destroy_nhwindow(WIN_MESSAGE);
	    WIN_MESSAGE = WIN_STATUS = WIN_MAP = WIN_ERR;

	    if(!done_stopprint || flags.tombstone)
		endwin = create_nhwindow(NHW_TEXT);

	    if (how < GENOCIDED && flags.tombstone && endwin != WIN_ERR)
		outrip(endwin, how);
	} else
	    done_stopprint = 1; /* just avoid any more output */

/* changing kilbuf really changes killer. we do it this way because
   killer is declared a (const char *)
*/
#ifndef JP
	if (u.uhave.amulet) Strcat(kilbuf, " (with the Amulet)");
	else if (how == ESCAPED) {
	    if (Is_astralevel(&u.uz))	/* offered Amulet to wrong deity */
		Strcat(kilbuf, " (in celestial disgrace)");
	    else if (carrying(FAKE_AMULET_OF_YENDOR))
		Strcat(kilbuf, " (with a fake Amulet)");
		/* don't bother counting to see whether it should be plural */
	}
#else
	if (u.uhave.amulet) {
	    Sprintf(pbuf, "魔除けを手に%s", kilbuf);
	    Strcpy(kilbuf, pbuf);
	} else if (how == ESCAPED) {
	    Strcpy(pbuf, kilbuf);
	    if (Is_astralevel(&u.uz))	/* offered Amulet to wrong deity */
		Sprintf(kilbuf, "天界で恥を晒し%s", pbuf);
	    else if (carrying(FAKE_AMULET_OF_YENDOR))
		Sprintf(kilbuf, "偽の魔除けを手に%s", pbuf);
		/* don't bother counting to see whether it should be plural */
	}
#endif /*JP*/

	if (!done_stopprint) {
#ifndef JP
	    Sprintf(pbuf, "%s %s the %s...", Goodbye(), plname,
		   how != ASCENDED ?
		      (const char *) ((flags.female && urole.name.f) ?
		         urole.name.f : urole.name.m) :
		      (const char *) (flags.female ? "Demigoddess" : "Demigod"));
#else
	    Sprintf(pbuf, "%s %sの%s…。", Goodbye(),
		   how != ASCENDED ?
		      (const char *) ((flags.female && urole.name.f) ?
		         urole.name.f : urole.name.m) :
		      (const char *) (flags.female ? "女神" : "神"),
		      plname);
#endif /*JP*/
	    putstr(endwin, 0, pbuf);
	    putstr(endwin, 0, "");
	}

	if (how == ESCAPED || how == ASCENDED) {
	    register struct monst *mtmp;
	    register struct obj *otmp;
	    register struct val_list *val;
	    register int i;

	    for (val = valuables; val->list; val++)
		for (i = 0; i < val->size; i++) {
		    val->list[i].count = 0L;
		}
	    get_valuables(invent);

	    /* add points for collected valuables */
	    for (val = valuables; val->list; val++)
		for (i = 0; i < val->size; i++)
		    if (val->list[i].count != 0L)
			u.urexp += val->list[i].count
				  * (long)objects[val->list[i].typ].oc_cost;

	    /* count the points for artifacts */
	    artifact_score(invent, TRUE, endwin);

	    keepdogs(TRUE);
	    viz_array[0][0] |= IN_SIGHT; /* need visibility for naming */
	    mtmp = mydogs;
	    if (!done_stopprint) Strcpy(pbuf, E_J("You","あなた"));
	    if (mtmp) {
		while (mtmp) {
		    if (!done_stopprint)
			Sprintf(eos(pbuf), E_J(" and %s","と%s"), mon_nam(mtmp));
		    if (mtmp->mtame)
			u.urexp += mtmp->mhp;
		    mtmp = mtmp->nmon;
		}
#ifdef JP
		if (!done_stopprint) Strcat(pbuf, "は");
#endif /*JP*/
		if (!done_stopprint) putstr(endwin, 0, pbuf);
		pbuf[0] = '\0';
	    } else {
		if (!done_stopprint) Strcat(pbuf, E_J(" ","は"));
	    }
	    if (!done_stopprint) {
#ifndef JP
		Sprintf(eos(pbuf), "%s with %ld point%s,",
			how==ASCENDED ? "went to your reward" :
					"escaped from the dungeon",
			u.urexp, plur(u.urexp));
#else
		Sprintf(eos(pbuf), "%ldポイントの得点をあげ、%s。",
			u.urexp,
			how==ASCENDED ? "昇天した" :
					"迷宮から逃げ出した");
#endif /*JP*/
		putstr(endwin, 0, pbuf);
	    }

	    if (!done_stopprint)
		artifact_score(invent, FALSE, endwin);	/* list artifacts */

	    /* list valuables here */
	    for (val = valuables; val->list; val++) {
		sort_valuables(val->list, val->size);
		for (i = 0; i < val->size && !done_stopprint; i++) {
		    int typ = val->list[i].typ;
		    long count = val->list[i].count;

		    if (count == 0L) continue;
		    if (objects[typ].oc_class != GEM_CLASS || typ <= LAST_GEM) {
			otmp = mksobj(typ, FALSE, FALSE);
			makeknown(otmp->otyp);
			otmp->known = 1;	/* for fake amulets */
			otmp->dknown = 1;	/* seen it (blindness fix) */
			otmp->has_name = 0;
			otmp->quan = count;
			Sprintf(pbuf, E_J("%8ld %s (worth %ld %s),",
					  "%8ld個の%s (%ld %sの価値),"),
				count, xname(otmp),
				count * (long)objects[typ].oc_cost, currency(2L));
			obfree(otmp, (struct obj *)0);
		    } else {
#ifndef JP
			Sprintf(pbuf,
				"%8ld worthless piece%s of colored glass,",
				count, plur(count));
#else
			Sprintf(pbuf, "%8ld個の無価値な色ガラス片", count);
#endif /*JP*/
		    }
		    putstr(endwin, 0, pbuf);
		}
	    }

	} else if (!done_stopprint) {
	    /* did not escape or ascend */
	    if (u.uz.dnum == 0 && u.uz.dlevel <= 0) {
		/* level teleported out of the dungeon; `how' is DIED,
		   due to falling or to "arriving at heaven prematurely" */
		Sprintf(pbuf, E_J("You %s beyond the confines of the dungeon",
				"迷宮の領域を越えて%s。"),
			(u.uz.dlevel < 0) ? E_J("passed away","行方知れずとなった") : ends[how]);
	    } else {
		/* more conventional demise */
		const char *where = E_J(dungeons[u.uz.dnum].dname, jdgnnam(u.uz.dnum));

		if (Is_astralevel(&u.uz)) where = E_J("The Astral Plane","天上界");
#ifndef JP
		Sprintf(pbuf, "You %s in %s", ends[how], where);
#else
		Sprintf(pbuf, "あなたは%s", where);
#endif /*JP*/
		if (!In_endgame(&u.uz) && !Is_knox(&u.uz))
		    Sprintf(eos(pbuf), E_J(" on dungeon level %d","の%d階で"),
			    In_quest(&u.uz) ? dunlev(&u.uz) : depth(&u.uz));
	    }

#ifndef JP
	    Sprintf(eos(pbuf), " with %ld point%s,",
		    u.urexp, plur(u.urexp));
#else
	    Sprintf(eos(pbuf), "%ldポイントの得点を上げ、", u.urexp);
#endif /*JP*/
	    putstr(endwin, 0, pbuf);
	}

	if (!done_stopprint) {
#ifndef JP
	    Sprintf(pbuf, "and %ld piece%s of gold, after %ld move%s.",
		    umoney, plur(umoney), moves, plur(moves));
#else
	    Sprintf(pbuf, "%ld枚の金貨を持ち、%ld歩動いた。",
		    umoney, moves);
#endif /*JP*/
	    putstr(endwin, 0, pbuf);
	}
	if (!done_stopprint) {
#ifndef JP
	    Sprintf(pbuf,
	     "You were level %d with a maximum of %d hit point%s when you %s.",
		    u.ulevel, u.uhpmax, plur(u.uhpmax), ends[how]);
#else
	    Sprintf(pbuf,
		    "%s時点で、あなたのレベルは%d、最大HPは%dであった。",
		    ends[how], u.ulevel, u.uhpmax);
#endif /*JP*/
	    putstr(endwin, 0, pbuf);
	    putstr(endwin, 0, "");
	}
	if (!done_stopprint)
	    display_nhwindow(endwin, TRUE);
	if (endwin != WIN_ERR)
	    destroy_nhwindow(endwin);

	/* "So when I die, the first thing I will see in Heaven is a
	 * score list?" */
#ifdef JP
	remove_killer_sep(killer_buf_with_separator);
#endif /*JP*/
	if (flags.toptenwin) {
	    topten(how);
	    if (have_windows)
		exit_nhwindows((char *)0);
	} else {
	    if (have_windows)
		exit_nhwindows((char *)0);
	    topten(how);
	}

	if(done_stopprint) { raw_print(""); raw_print(""); }
	terminate(EXIT_SUCCESS);
}


void
container_contents(list, identified, all_containers)
struct obj *list;
boolean identified, all_containers;
{
	register struct obj *box, *obj;
	char buf[BUFSZ];

	for (box = list; box; box = box->nobj) {
	    if (Is_container(box) || box->otyp == STATUE) {
		if (box->otyp == BAG_OF_TRICKS) {
		    continue;	/* wrong type of container */
		} else if (box->cobj) {
		    winid tmpwin = create_nhwindow(NHW_MENU);
#ifndef JP
		    Sprintf(buf, "Contents of %s:", the(xname(box)));
#else
		    Sprintf(buf, "%sの中身:", xname(box));
#endif /*JP*/
		    putstr(tmpwin, 0, buf);
		    putstr(tmpwin, 0, "");
		    for (obj = box->cobj; obj; obj = obj->nobj) {
			if (identified) {
			    makeknown(obj->otyp);
			    obj->known = obj->bknown =
			    obj->dknown = obj->rknown = 1;
			}
			putstr(tmpwin, 0, doname(obj));
		    }
		    display_nhwindow(tmpwin, TRUE);
		    destroy_nhwindow(tmpwin);
		    if (all_containers)
			container_contents(box->cobj, identified, TRUE);
		} else {
#ifndef JP
		    pline("%s empty.", Tobjnam(box, "are"));
#else
		    pline("%sには何も入っていない。", xname(box));
#endif /*JP*/
		    display_nhwindow(WIN_MESSAGE, FALSE);
		}
	    }
	    if (!all_containers)
		break;
	}
}


/* should be called with either EXIT_SUCCESS or EXIT_FAILURE */
void
terminate(status)
int status;
{
#ifdef MAC
	getreturn("to exit");
#endif
	/* don't bother to try to release memory if we're in panic mode, to
	   avoid trouble in case that happens to be due to memory problems */
	if (!program_state.panicking) {
	    freedynamicdata();
	    dlb_cleanup();
	}

	nethack_exit(status);
}

STATIC_OVL void
list_vanquished(defquery, ask)
char defquery;
boolean ask;
{
    register int i, lev;
    int ntypes = 0, max_lev = 0, nkilled;
    long total_killed = 0L;
    char c;
    winid klwin;
    char buf[BUFSZ];

    /* get totals first */
    for (i = LOW_PM; i < NUMMONS; i++) {
	if (mons[i].mlet == S_WORM_TAIL) continue;
	if (mvitals[i].died) ntypes++;
	total_killed += (long)mvitals[i].died;
	if (mons[i].mlevel > max_lev) max_lev = mons[i].mlevel;
    }

    /* vanquished creatures list;
     * includes all dead monsters, not just those killed by the player
     */
    if (ntypes != 0) {
	c = ask ? yn_function(E_J("Do you want an account of creatures vanquished?",
				"倒した敵の一覧を確認しますか？"),
			      ynqchars, defquery) : defquery;
	if (c == 'q') done_stopprint++;
	if (c == 'y') {
	    klwin = create_nhwindow(NHW_MENU);
	    putstr(klwin, 0, E_J("Vanquished creatures:","打倒した敵:"));
	    putstr(klwin, 0, "");

	    /* countdown by monster "toughness" */
	    for (lev = max_lev; lev >= 0; lev--)
	      for (i = LOW_PM; i < NUMMONS; i++) {
		if (mons[i].mlet == S_WORM_TAIL) continue;
		if (mons[i].mlevel == lev && (nkilled = mvitals[i].died) > 0) {
		    if ((mons[i].geno & G_UNIQ) && i != PM_HIGH_PRIEST) {
#ifndef JP
			Sprintf(buf, "     %s%s",
				!type_is_pname(&mons[i]) ? "The " : "",
				mons[i].mname);
#else
			Sprintf(buf, "         %s", JMONNAM(i));
#endif /*JP*/
			if (nkilled > 1) {
#ifndef JP
			    switch (nkilled) {
				case 2:  Sprintf(eos(buf)," (twice)");  break;
				case 3:  Sprintf(eos(buf)," (thrice)");  break;
				default: Sprintf(eos(buf)," (%d time%s)",
						 nkilled, plur(nkilled));
					 break;
			    }
#else /*JP*/
			    Sprintf(eos(buf), " (%d回)", nkilled);
#endif /*JP*/
			}
		    } else {
			/* trolls or undead might have come back,
			   but we don't keep track of that */
#ifndef JP
			if (nkilled == 1)
			    Sprintf(buf, "     %s", an(mons[i].mname));
			else
			    Sprintf(buf, "%4d %s",
				    nkilled, makeplural(mons[i].mname));
#else
			if (nkilled == 1)
			    Sprintf(buf, "         %s", JMONNAM(i));
			else
			    Sprintf(buf, "%4d体の %s",
				    nkilled, JMONNAM(i));
#endif /*JP*/
		    }
		    putstr(klwin, 0, buf);
		}
	      }
	    /*
	     * if (Hallucination)
	     *     putstr(klwin, 0, "and a partridge in a pear tree");
	     */
	    if (ntypes > 1) {
		putstr(klwin, 0, "");
		Sprintf(buf, E_J("%ld creatures vanquished.",
				"%ld体の敵を打倒した。"), total_killed);
		putstr(klwin, 0, buf);
	    }
	    display_nhwindow(klwin, TRUE);
	    destroy_nhwindow(klwin);
	}
    }
}

/* number of monster species which have been genocided */
int
num_genocides()
{
    int i, n = 0;

    for (i = LOW_PM; i < NUMMONS; ++i)
	if (mvitals[i].mvflags & G_GENOD) ++n;

    return n;
}

void
list_genocided(defquery, ask)
char defquery;
boolean ask;
{
    register int i;
    int ngenocided;
    char c;
    winid klwin;
    char buf[BUFSZ];

    ngenocided = num_genocides();

    /* genocided species list */
    if (ngenocided != 0) {
	c = ask ? yn_function(E_J("Do you want a list of species genocided?",
				"虐殺した種族の一覧を確認しますか？"),
			      ynqchars, defquery) : defquery;
	if (c == 'q') done_stopprint++;
	if (c == 'y') {
	    klwin = create_nhwindow(NHW_MENU);
	    putstr(klwin, 0, E_J("Genocided species:","虐殺した種族:"));
	    putstr(klwin, 0, "");

	    for (i = LOW_PM; i < NUMMONS; i++)
		if (mvitals[i].mvflags & G_GENOD) {
#ifndef JP
		    if ((mons[i].geno & G_UNIQ) && i != PM_HIGH_PRIEST)
			Sprintf(buf, "%s%s",
				!type_is_pname(&mons[i]) ? "" : "the ",
				mons[i].mname);
		    else
			Strcpy(buf, makeplural(mons[i].mname));
#else
		    Strcpy(buf, JMONNAM(i));
#endif /*JP*/
		    putstr(klwin, 0, buf);
		}

	    putstr(klwin, 0, "");
	    Sprintf(buf, E_J("%d species genocided.",
			     "%d種の怪物を虐殺した。"), ngenocided);
	    putstr(klwin, 0, buf);

	    display_nhwindow(klwin, TRUE);
	    destroy_nhwindow(klwin);
	}
    }
}

/*end.c*/
