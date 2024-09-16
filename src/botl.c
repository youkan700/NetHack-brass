/*	SCCS Id: @(#)botl.c	3.4	1996/07/15	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const char *hu_stat[];	/* defined in eat.c */

const char * const enc_stat[] = {
	"",
	E_J("Burdened", "重荷"),
	E_J("Stressed", "圧迫"),
	E_J("Strained",	"重量過多"),
	E_J("Overtaxed","重量限界"),
	E_J("Overloaded","重量超過")
};

STATIC_DCL void NDECL(bot1);
STATIC_DCL void NDECL(bot2);

#ifdef HPMON
extern void FDECL(term_start_color, (int));
extern void NDECL(term_end_color);
STATIC_DCL int FDECL(attrib_color, (int));
STATIC_DCL char *FDECL(maybe_flush, (int, char *, char *));
STATIC_DCL char *FDECL(maybe_putstr, (int *, char *, char *));
STATIC_DCL void FDECL(colored_putstr, (int , char *));
#endif /*HPMON*/

/* MAXCO must hold longest uncompressed status line, and must be larger
 * than COLNO
 *
 * longest practical second status line at the moment is
 *	Astral Plane $:12345 HP:700(700) Pw:111(111) AC:-127 Xp:30/123456789
 *	T:123456 Satiated Conf FoodPois Ill Blind Stun Hallu Overloaded
 * -- or somewhat over 130 characters
 */
#if COLNO <= 140
#define MAXCO 160
#else
#define MAXCO (COLNO+20)
#endif

static int mrank_sz = 0; /* loaded by max_rank_sz (from u_init) */

STATIC_DCL const char *NDECL(rank);


/* convert experience level (1..30) to rank index (0..8) */
int
xlev_to_rank(xlev)
int xlev;
{
	// xlev | 1-2 | 3-5 | 6-9 | 10-13 | 14-17 | 18-21 | 22-25 | 26-29 | 30 |
	// rank | 0   | 1   | 2   | 3     | 4     | 5     | 6     | 7     | 8  |
	return (xlev <= 2) ? 0 : (xlev <= 30) ? ((xlev + 2) / 4) : 8;
}

#if 0	/* not currently needed */
/* convert rank index (0..8) to experience level (1..30) */
int
rank_to_xlev(rank)
int rank;
{
	return (rank <= 0) ? 1 : (rank <= 8) ? ((rank * 4) - 2) : 30;
}
#endif

const char *
rank_of(lev, monnum, female)
	int lev;
	short monnum;
	boolean female;
{
	register struct Role *role;
	register int i;


	/* Find the role */
	for (role = (struct Role *) roles; role->name.m; role++)
	    if (monnum == role->malenum || monnum == role->femalenum)
	    	break;
	if (!role->name.m)
	    role = &urole;

	/* Find the rank */
	for (i = xlev_to_rank((int)lev); i >= 0; i--) {
	    if (female && role->rank[i].f) return (role->rank[i].f);
	    if (role->rank[i].m) return (role->rank[i].m);
	}

	/* Try the role name, instead */
	if (female && role->name.f) return (role->name.f);
	else if (role->name.m) return (role->name.m);
	return (E_J("Player","プレイヤー"));
}


STATIC_OVL const char *
rank()
{
	return(rank_of(u.ulevel, Role_switch, flags.female));
}

int
title_to_mon(str, rank_indx, title_length)
const char *str;
int *rank_indx, *title_length;
{
	register int i, j;


	/* Loop through each of the roles */
	for (i = 0; roles[i].name.m; i++)
	    for (j = 0; j < 9; j++) {
	    	if (roles[i].rank[j].m && !strncmpi(str,
	    			roles[i].rank[j].m, strlen(roles[i].rank[j].m))) {
	    	    if (rank_indx) *rank_indx = j;
	    	    if (title_length) *title_length = strlen(roles[i].rank[j].m);
	    	    return roles[i].malenum;
	    	}
	    	if (roles[i].rank[j].f && !strncmpi(str,
	    			roles[i].rank[j].f, strlen(roles[i].rank[j].f))) {
	    	    if (rank_indx) *rank_indx = j;
	    	    if (title_length) *title_length = strlen(roles[i].rank[j].f);
	    	    return ((roles[i].femalenum != NON_PM) ?
	    	    		roles[i].femalenum : roles[i].malenum);
	    	}
	    }
	return NON_PM;
}


void
max_rank_sz()
{
	register int i, r, maxr = 0;
	for (i = 0; i < 9; i++) {
	    if (urole.rank[i].m && (r = strlen(urole.rank[i].m)) > maxr) maxr = r;
	    if (urole.rank[i].f && (r = strlen(urole.rank[i].f)) > maxr) maxr = r;
	}
	mrank_sz = maxr;
	return;
}


#ifdef SCORE_ON_BOTL
long
botl_score()
{
    int deepest = deepest_lev_reached(FALSE);

    long ugold = u.ugold + hidden_gold();

    if ((ugold -= u.ugold0) < 0L) ugold = 0L;
    return ugold + u.urexp + (long)(50 * (deepest - 1))
			  + (long)(deepest > 30 ? 10000 :
				   deepest > 20 ? 1000*(deepest - 20) : 0);
}
#endif

#ifdef HPMON
int
attrib_color(a)
int a;
{
	if (ACURR(a) == AMAX(a)) return NO_COLOR;
	return (ACURR(a) > AMAX(a)) ? CLR_WHITE : CLR_YELLOW;
}

char *maybe_flush(color, head, tail)
int color;
char *head;
char *tail;
{
	if (color != NO_COLOR) {
	    putstr(WIN_STATUS, 0, head);
	    *head = 0;
            return head;
	}
        return tail;
}
char *maybe_putstr(pcolor, head, tail)
int *pcolor;
char *head;
char *tail;
{
	if (*pcolor != NO_COLOR) {
	    term_start_color(*pcolor);
	    putstr(WIN_STATUS, 0, head);
	    term_end_color();
	    *pcolor = NO_COLOR;
	    *head = 0;
            return head;
	}
        return tail;
}
void colored_putstr(color, str)
int color;
char *str;
{
	term_start_color(color);
	putstr(WIN_STATUS, 0, str);
	term_end_color();
}
#endif /*HPMON*/

STATIC_OVL void
bot1()
{
	char newbot1[MAXCO];
	register char *nb;
	register int i,j;
	int c = NO_COLOR;
	static const char *atrnam[5] = { "Dx",  "Co",  "In",  "Wi",  "Ch"  };
	static const int   atridx[5] = { A_DEX, A_CON, A_INT, A_WIS, A_CHA };

	curs(WIN_STATUS, 1, 0);

        /* Player name and rank */
	Strcpy(newbot1, plname);
	if('a' <= newbot1[0] && newbot1[0] <= 'z') newbot1[0] += 'A'-'a';
	newbot1[10] = 0;
	Sprintf(nb = eos(newbot1),E_J(" the "," "));

	if (Upolyd) {
		char mbot[BUFSZ];
		int k = 0;

		Strcpy(mbot, E_J(mons[u.umonnum].mname, JMONNAM(u.umonnum)));
		while(mbot[k] != 0) {
		    if ((k == 0 || (k > 0 && mbot[k-1] == ' ')) &&
					'a' <= mbot[k] && mbot[k] <= 'z')
			mbot[k] += 'A' - 'a';
		    k++;
		}
		Sprintf(nb = eos(nb), mbot);
	} else
		Sprintf(nb = eos(nb), rank());

	Sprintf(nb = eos(nb),"  ");
	i = mrank_sz + 15;
	j = (nb + 2) - newbot1; /* aka strlen(newbot1) but less computation */
	if((i - j) > 0)
		Sprintf(nb = eos(nb),"%*s", i-j, " ");	/* pad with spaces */

        /* Strength */
        Sprintf(nb = eos(nb),"St:");
#ifdef HPMON
	c = attrib_color(A_STR);
        nb = maybe_flush(c, newbot1, nb);
#endif /*HPMON*/
	if (ACURR(A_STR) > 18) {
		if (ACURR(A_STR) > STR18(100))
		    Sprintf(nb = eos(nb),"%2d ",ACURR(A_STR)-100);
		else if (ACURR(A_STR) < STR18(100))
		    Sprintf(nb = eos(nb), "18/%02d ",ACURR(A_STR)-18);
		else
		    Sprintf(nb = eos(nb),"18/** ");
	} else
		Sprintf(nb = eos(nb), "%-1d ",ACURR(A_STR));
#ifdef HPMON
        nb = maybe_putstr(&c, newbot1, nb);

        /* Dex, Con, Int, Wiz, Cha */
	for (i = 0; i < 5; i++) {
	    Sprintf(nb = eos(nb), "%s:", atrnam[i]);
	    j = atridx[i];
	    c = attrib_color(j);
            nb = maybe_flush(c, newbot1, nb);
	    Sprintf(nb = eos(nb), "%-1d ", ACURR(j));
            nb = maybe_putstr(&c, newbot1, nb);
	}
#else /*HPMON*/
	Sprintf(nb = eos(nb),
		"Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
		ACURR(A_DEX), ACURR(A_CON), ACURR(A_INT), ACURR(A_WIS), ACURR(A_CHA));
#endif /*HPMON*/
#ifndef JP
	Sprintf(nb = eos(nb), (u.ualign.type == A_CHAOTIC) ? "  Chaotic" :
			(u.ualign.type == A_NEUTRAL) ? "  Neutral" : "  Lawful");
#else
	Sprintf(nb = eos(nb), (u.ualign.type == A_CHAOTIC) ? "  混沌" :
			(u.ualign.type == A_NEUTRAL) ? "  中立" : "  秩序");
#endif /*JP*/
#ifdef SCORE_ON_BOTL
	if (flags.showscore)
	    Sprintf(nb = eos(nb), " S:%ld", botl_score());
#endif
	putstr(WIN_STATUS, 0, newbot1);
}

/* provide the name of the current level for display by various ports */
int
describe_level(buf)
char *buf;
{
	int ret = 1;

	/* TODO:	Add in dungeon name */
	if (Is_knox(&u.uz))
		Sprintf(buf, "%s ", E_J(dungeons[u.uz.dnum].dname, jdgnnam(u.uz.dnum)));
	else if (In_quest(&u.uz))
		Sprintf(buf, E_J("Home %d ","故郷 %d "), dunlev(&u.uz));
	else if (In_endgame(&u.uz))
		Sprintf(buf,
			Is_astralevel(&u.uz) ? E_J("Astral Plane ","天上界 ") : E_J("End Game ","最終試練 "));
	else {
		/* ports with more room may expand this one */
		Sprintf(buf, E_J("Dlvl:%-2d ","地下:%-2d "), depth(&u.uz));
		ret = 0;
	}
	return ret;
}

STATIC_OVL void
bot2()
{
	char  newbot2[MAXCO];
	register char *nb;
	int hp, hpmax;
	int cap = near_capacity();
        int c;

	hp = Upolyd ? u.mh : u.uhp;
	hpmax = Upolyd ? u.mhmax : u.uhpmax;

	if(hp < 0) hp = 0;
	(void) describe_level(newbot2);
	Sprintf(nb = eos(newbot2),
#ifdef HPMON
		"%c:%-2ld HP:", oc_syms[COIN_CLASS],
		u.ugold
		);
#else /* HPMON */
		"%c:%-2ld HP:%d(%d) Pw:%d(%d) AC:%-2d", oc_syms[COIN_CLASS],
		hp, hpmax, u.uen, u.uenmax, u.uac);
#endif /* HPMON */
#ifdef HPMON
	curs(WIN_STATUS, 1, 1);
	putstr(WIN_STATUS, 0, newbot2);

	Sprintf(newbot2, "%d(%d) ", hp, hpmax);
#ifdef TEXTCOLOR
	if (iflags.use_color) {
	    int hpcolor = NO_COLOR;
	    if (critically_low_hp(FALSE)) {
		hpcolor = CLR_RED;
	    } else if(hp <= hpmax/4) {
		hpcolor = CLR_ORANGE;
	    } else if(hp <= hpmax/2) {
		hpcolor = CLR_YELLOW;
	    } else if(hp <= (hpmax*3/4)) {
		hpcolor = CLR_BRIGHT_GREEN;
	    }
            nb = maybe_putstr(&hpcolor, newbot2, nb);
	}
#endif /* TEXTCOLOR */
	Sprintf(nb = eos(newbot2), "Pw:%d(%d) AC:%-2d",
		u.uen, u.uenmax, u.uac);
#endif /* HPMON */

	if (Upolyd)
		Sprintf(nb = eos(nb), " HD:%d", mons[u.umonnum].mlevel);
#ifdef EXP_ON_BOTL
	else if(flags.showexp || flags.showrexp) {
#ifdef HPMON
	    Sprintf(nb = eos(nb), " Xp:");
            c = (u.ulevel < u.ulevelmax) ? CLR_YELLOW : NO_COLOR;
            nb = maybe_flush(c, newbot2, nb);
	    Sprintf(nb = eos(nb), "%u", u.ulevel);
            nb = maybe_putstr(&c, newbot2, nb);
#else
	    Sprintf(nb = eos(nb), " Xp:%u", u.ulevel);
#endif /* HPMON */
 	    if(flags.showexp)
		Sprintf(nb = eos(nb), "/%-1ld", u.uexp);
	    if(flags.showrexp) {
		if (u.ulevel < MAXULEV)
		    Sprintf(nb = eos(nb), "/%-1ld", newuexp(u.ulevel) - u.uexp);
	    }
	}
#endif
	else
		Sprintf(nb = eos(nb), " Exp:%u", u.ulevel);
	if(flags.showweight)
	    Sprintf(nb = eos(nb), " Wt:%ld/%ld", inv_weight()+weight_cap(), weight_cap() );
	if(flags.time)
	    Sprintf(nb = eos(nb), " T:%ld", moves);
	if(strcmp(hu_stat[u.uhs], "        ")) {
#ifdef HPMON
		if (u.uhs > 2) {
		    putstr(WIN_STATUS, 0, newbot2);
		    Sprintf(newbot2, " %s", hu_stat[u.uhs]);
                    c = (u.uhs == 3 ? CLR_YELLOW :
			 u.uhs == 4 ? CLR_ORANGE : CLR_RED);
                    nb = maybe_putstr(&c, newbot2, nb);
		} else
		    Sprintf(nb = eos(nb), " %s", hu_stat[u.uhs]);
#else
		Sprintf(nb = eos(nb), " ");
		Strcat(newbot2, hu_stat[u.uhs]);
#endif /*HPMON*/
	}
#ifdef HPMON
	putstr(WIN_STATUS, 0, newbot2);
        newbot2[0] = 0;
        nb = newbot2;
	if(Confusion)	   colored_putstr(CLR_YELLOW, E_J(" Conf"," 混乱"));
	if(Sick) {
		if (u.usick_type & SICK_VOMITABLE)
			   colored_putstr(CLR_BRIGHT_MAGENTA, E_J(" FoodPois"," 食中毒"));
		if (u.usick_type & SICK_NONVOMITABLE)
			   colored_putstr(CLR_BRIGHT_MAGENTA, E_J(" Ill"," 病気"));
	}
	if(Blind)	   colored_putstr(Blinded ? CLR_YELLOW : CLR_GRAY, E_J(" Blind"," 盲目"));
	if(Stunned)	   colored_putstr(CLR_YELLOW, E_J(" Stun"," よろめき"));
	if(Hallucination)  colored_putstr(CLR_YELLOW, E_J(" Hallu"," 幻覚"));
	if(Stoned)         colored_putstr(CLR_BRIGHT_MAGENTA, E_J(" Stone"," 石化"));
	if(Slimed)         colored_putstr(CLR_BRIGHT_MAGENTA, E_J(" Slime"," スライム化"));
	if(Strangled)      colored_putstr(CLR_BRIGHT_MAGENTA, E_J(" Choke"," 窒息"));
	if(cap > UNENCUMBERED)
		Sprintf(nb = eos(nb), " %s", enc_stat[cap]);
	if(Levitation) {
	        long i = (HLevitation & TIMEOUT);
                c = (i <= 7L && !ELevitation && (i == HLevitation) && !BLevAtWill &&
                     !can_transit_levitation()) ? CLR_YELLOW : NO_COLOR;
                nb = maybe_flush(c, newbot2, nb);
                Sprintf(nb = eos(nb), E_J(" Levitate"," 浮遊"));
                nb = maybe_putstr(&c, newbot2, nb);
        }
#else
	if(Confusion)	   Sprintf(nb = eos(nb), E_J(" Conf"," 混乱"));
	if(Sick) {
		if (u.usick_type & SICK_VOMITABLE)
			   Sprintf(nb = eos(nb), E_J(" FoodPois"," 食中毒"));
		if (u.usick_type & SICK_NONVOMITABLE)
			   Sprintf(nb = eos(nb), E_J(" Ill"," 病気"));
	}
	if(Blind)	   Sprintf(nb = eos(nb), E_J(" Blind"," 盲目"));
	if(Stunned)	   Sprintf(nb = eos(nb), E_J(" Stun"," よろめき"));
	if(Hallucination)  Sprintf(nb = eos(nb), E_J(" Hallu"," 幻覚"));
	if(Stoned)         Sprintf(nb = eos(nb), E_J(" Slime"," 石化"));
	if(Slimed)         Sprintf(nb = eos(nb), E_J(" Slime"," スライム化"));
	if(Strangled)      Sprintf(nb = eos(nb), E_J(" Choke"," 窒息"));
	if(cap > UNENCUMBERED)
		Sprintf(nb = eos(nb), " %s", enc_stat[cap]);
	if(Levitation)     Sprintf(nb = eos(nb), E_J(" Levitate"," 浮遊"));
#endif /*HPMON*/
#ifndef HPMON
	curs(WIN_STATUS, 1, 1);
#endif /* HPMON */
	putstr(WIN_STATUS, 0, newbot2);
//	draw_hpbar(WIN_MAP, u.ux, u.uy, hp, hpmax); //test//
}

void
bot()
{
#ifdef WIN32_GRAPHICS2
	display_nhwindow(WIN_STATUS, 1); /* suppress flashing */
#endif /*WIN32_GRAPHICS2*/
	bot1();
	bot2();
#ifdef WIN32_GRAPHICS2
	display_nhwindow(WIN_STATUS, 0);
#endif /*WIN32_GRAPHICS2*/
	flags.botl = flags.botlx = 0;
}

/*----------------------
	HP Bar
  ----------------------*/
#if 0
#define MAX_HPBARS 8
static struct {
	unsigned mid;		/* monster id */
	long rectime;		/* recorded time */
	int lasthp;		/* after scaled */
	int next;		/* next index */
} hpbars[MAX_HPBARS];

void init_hpbar()
{
	int i;
	for (i=0; i<MAX_HPBARS; i++) {
	    hpbars[i].next = i+1;
	}
}

int get_hpbar(mtmp)
struct monst *mtmp;
{
	int i;
	/* search if mtmp is already in the list */
	for (i=1; i<MAX_HPBARS; i++) {
	    if (!hpbars[i].mid) continue;
	    if (hpbars[i].mid == mtmp->m_id) return i;
	}
	/* not found... */
	if (!mtmp->mtame) {
	} else {
	}
}

void entry_hpbar(mtmp, index)
struct monst *mtmp;
int index;
{
	hpbars[i].mid = mtmp ? mtmp->m_id : 0;
	hpbars[i].rectime = 0L;
}

void remove_hpbar(mtmp)
struct monst *mtmp;
{
	int i;
	for (i=0; i<MAX_HPBARS; i++)
	    if (hpbars[i].mid == mtmp->m_id) break;
	if (i == MAX_HPBARS) return; /* not found? */
	for (; i<MAX_HPBARS-1; i++) hpbars[i] = hpbars[i+1];
	entry_hpbar(0, i);
}

void win32y_draw_hpbar(winid win, int index, int glyph, int hpnew, int hpold);
void
hpbar(mtmp, dmg)
struct monst *mtmp;
int dmg;
{
	int hpnew, hpold, hpmax;
	int green_x, red_x;
	long oldest, index;
	int i;
	if (mtmp == &youmonst) { i=0; goto found; }
	/* search if mtmp is already registered,
	   or there is an empty slot */
	for (i=1; i<MAX_HPBARS; i++) {
	    if (hpbars[i].mid == mtmp->m_id) goto found;
	    if (!hpbars[i].mid) {
		entry_hpbar(mtmp, i);
		goto found;
	    }
	}
	/* all slots are used... remove least-recently used slot */
	for (i=1, oldest = moves+1; i<MAX_HPBARS; i++) {
	    if (hpbars[i].mon->mtame) continue;
	    if (hpbars[i].rectime < oldest) {
		oldest = hpbars[i].rectime;
		index = i;
	    }
	}
	remove_hpbar(hpbars[index].mon);
	i = MAX_HPBARS - 1; /* last slot */
	hpbars[i].mon = mtmp;
	hpbars[i].rectime = 0L;
found:
//pline("{%d}",i);
	if (mtmp == &youmonst) {
	    if (Upolyd) {
		hpnew = u.mh;
		hpmax = u.mhmax;
	    } else {
		hpnew = u.uhp;
		hpmax = u.uhpmax;
	    }
	} else {
	    hpnew = mtmp->mhp;
	    hpmax = mtmp->mhpmax;
	}
	hpold = hpnew;
	if (hpbars[i].rectime == moves) {
	    red_x = hpbars[i].lasthp;
	} else {
	    red_x = (hpold << 10) / hpmax;
	    hpbars[i].lasthp = red_x;
	    hpbars[i].rectime = moves;
	}
	hpnew -= dmg;
	if (hpnew < 0) {
	    hpnew = 0;
	    hpbars[i].rectime = 0L;
	} else if (hpnew > hpmax) hpnew = hpmax;
	green_x = (hpnew << 10) / hpmax;
	if (red_x < green_x) red_x = green_x;
/*pline("{%d,%d,%d}",green_x,red_x,moves);*/
	win32y_draw_hpbar(WIN_HPBAR, i, i ? mon_to_glyph(mtmp) : hero_glyph, green_x, red_x);
	return;
}
#endif


/*botl.c*/
