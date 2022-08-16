/*      SCCS Id: @(#)tech.c    3.2     98/Oct/30        */
/*      Original Code by Warren Cheung (Basis: spell.c, attrib.c) */
/*      Copyright (c) M. Stephenson 1988                          */
/* NetHack may be freely redistributed.  See license for details. */

/* All of the techs from cmd.c are ported here */

#include "hack.h"
#include "eshk.h"

/* #define DEBUG */		/* turn on for diagnostics */

static boolean FDECL(gettech, (int *));
static boolean tech_known(short);
static void aborttech(short);
static boolean FDECL(dotechmenu, (int, int *));
static boolean FDECL(dotechhelp, (int *));
static int FDECL(get_tech_no,(int));
static int FDECL(techeffects, (int));
static void FDECL(hurtmon, (struct monst *,int));
STATIC_PTR int NDECL(draw_energy);
static const struct innate_tech * NDECL(role_tech);
static const struct innate_tech * NDECL(race_tech);

static int FDECL(getobj_filter_surgery, (struct obj *));
static int NDECL(surgery);
static int NDECL(dopurify);
static int NDECL(throw_ofuda);

static int FDECL(getobj_filter_dwarvish_repair, (struct obj *));

static NEARDATA schar delay;            /* moves left for tinker/energy draw */

/* 
 * Do try to keep the names <= 25 chars long, or else the
 * menu will look bad :B  WAC
 */

static const struct innate_tech 
	/* Roles */
	arc_tech[] = { {   1, T_RESEARCH, 1},
		       {   0, 0, 0} },
	bar_tech[] = { {   1, T_BERSERK, 1},
		       {   0, 0, 0} },
	cav_tech[] = { {   0, 0, 0} },
	hea_tech[] = { {   1, T_SURGERY, 1},
		       {   0, 0, 0} },
	kni_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_HEAL_HANDS, 1},
		       {   0, 0, 0} },
	mon_tech[] = { {   0, 0, 0} },
	med_tech[] = { {   1, T_OFUDA, 1},
		       {   3, T_PURIFY, 1},
		       {  10, T_HAMAYA, 1},
		       {   0, 0, 0} },
	pri_tech[] = { {   1, T_TURN_UNDEAD, 1},
		       {   1, T_BLESSING, 1},
		       {   0, 0, 0} },
	ran_tech[] = { {   1, T_FLURRY, 1},
		       {   0, 0, 0} },
	rog_tech[] = { {   1, T_CRIT_STRIKE, 1},
		       {  15, T_CUTTHROAT, 1},
		       {   0, 0, 0} },
	sam_tech[] = { {   1, T_KIII, 1},
		       {   0, 0, 0} },
	tou_tech[] = { /* Put Tech here */
		       {   0, 0, 0} },
	val_tech[] = { {   1, T_PRACTICE, 1},
		       {   0, 0, 0} },
	wiz_tech[] = { {   0, 0, 0} },

	/* Races */
	dwa_tech[] = { {   1, T_MAINTENANCE, 1},
		       {   0, 0, 0} },
	elf_tech[] = { /* Put Tech here */
		       {   0, 0, 0} },
	gno_tech[] = { {   0, 0, 0} },
	hob_tech[] = { {   0, 0, 0} };
	/* Orc */

/* Local Macros 
 * these give you direct access to the player's list of techs.  
 * Are you sure you don't want to use tech_inuse,  which is the
 * extern function for checking whether a fcn is inuse
 */

#define techt_inuse(tech)       tech_list[tech].t_inuse
#define techtout(tech)        tech_list[tech].t_tout
#define techlev(tech)         (u.ulevel - tech_list[tech].t_lev)
#define techid(tech)          tech_list[tech].t_id
#ifndef JP
#define techname(tech)        (tech_prop[techid(tech)].name)
#else
#define techname(tech)        (jtech_names[techid(tech)])
#endif /*JP*/
#define techlet(tech)  \
        ((char)((tech < 26) ? ('a' + tech) : ('A' + tech - 26)))


/* Whether you know the tech */
boolean
tech_known(tech)
	short tech;
{
	int i;
	for (i = 0; i < MAXTECH; i++) {
		if (techid(i) == tech) 
		     return TRUE;
	}
	return FALSE;
}

/* Called to prematurely stop a technique */
void
aborttech(tech)
	short tech;
{
	int i;

	i = get_tech_no(tech);
	if (tech_list[i].t_inuse) {
	    switch (tech_list[i].t_id) {
//		case T_RAGE:
//		    u.uhpmax -= tech_list[i].t_inuse - 1;
//		    if (u.uhpmax < 1)
//			u.uhpmax = 0;
//		    u.uhp -= tech_list[i].t_inuse - 1;
//		    if (u.uhp < 1)
//			u.uhp = 1;
//		    break;
//		case T_POWER_SURGE:
//		    u.uenmax -= tech_list[i].t_inuse - 1;
//		    if (u.uenmax < 1)
//			u.uenmax = 0;
//		    u.uen -= tech_list[i].t_inuse - 1;
//		    if (u.uen < 0)
//			u.uen = 0;
//		    break;
		default:
		    break;
	    }
	    tech_list[i].t_inuse = 0;
	}
}

/* Called to teach a new tech.  Level is starting tech level */
void
learntech(tech, mask, tlevel)
	short tech;
	long mask;
	int tlevel;
{
	int i;
	const struct innate_tech *tp;

	i = get_tech_no(tech);
	if (tlevel > 0) {
	    if (i < 0) {
		i = get_tech_no(NO_TECH);
		if (i < 0) {
		    impossible("No room for new technique?");
		    return;
		}
	    }
	    tlevel = u.ulevel ? u.ulevel - tlevel : 0;
	    if (tech_list[i].t_id == NO_TECH) {
		tech_list[i].t_id = tech;
		tech_list[i].t_lev = tlevel;
		tech_list[i].t_inuse = 0; /* not in use */
		tech_list[i].t_intrinsic = 0;
	    }
	    else if (tech_list[i].t_intrinsic & mask) {
		impossible("Tech already known.");
		return;
	    }
	    if (mask == FROMOUTSIDE) {
		tech_list[i].t_intrinsic &= ~OUTSIDE_LEVEL;
		tech_list[i].t_intrinsic |= tlevel & OUTSIDE_LEVEL;
	    }
	    if (tlevel < tech_list[i].t_lev)
		tech_list[i].t_lev = tlevel;
	    tech_list[i].t_intrinsic |= mask;
	    tech_list[i].t_tout = 0; /* Can use immediately*/
	}
	else if (tlevel < 0) {
	    if (i < 0 || !(tech_list[i].t_intrinsic & mask)) {
		impossible("Tech not known.");
		return;
	    }
	    tech_list[i].t_intrinsic &= ~mask;
	    if (!(tech_list[i].t_intrinsic & INTRINSIC)) {
		if (tech_list[i].t_inuse)
		    aborttech(tech);
		tech_list[i].t_id = NO_TECH;
		return;
	    }
	    /* Re-calculate lowest t_lev */
	    if (tech_list[i].t_intrinsic & FROMOUTSIDE)
		tlevel = tech_list[i].t_intrinsic & OUTSIDE_LEVEL;
	    if (tech_list[i].t_intrinsic & FROMEXPER) {
		for(tp = role_tech(); tp->tech_id; tp++)
		    if (tp->tech_id == tech)
			break;
		if (!tp->tech_id)
		    impossible("No inate technique for role?");
		else if (tlevel < 0 || tp->ulevel - tp->tech_lev < tlevel)
		    tlevel = tp->ulevel - tp->tech_lev;
	    }
	    if (tech_list[i].t_intrinsic & FROMRACE) {
		for(tp = race_tech(); tp->tech_id; tp++)
		    if (tp->tech_id == tech)
			break;
		if (!tp->tech_id)
		    impossible("No inate technique for race?");
		else if (tlevel < 0 || tp->ulevel - tp->tech_lev < tlevel)
		    tlevel = tp->ulevel - tp->tech_lev;
	    }
	    tech_list[i].t_lev = tlevel;
	}
	else
	    impossible("Invalid Tech Level!");
}

/*
 * Return TRUE if a tech was picked, with the tech index in the return
 * parameter.  Otherwise return FALSE.
 */
static boolean
gettech(tech_no)
        int *tech_no;
{
        int i, ntechs, idx;
	char ilet, lets[BUFSZ], qbuf[QBUFSZ];
	boolean selected;

	for (ntechs = i = 0; i < MAXTECH; i++)
	    if (techid(i) != NO_TECH) ntechs++;
	if (ntechs == 0)  {
            You(E_J("don't know any techniques right now.",
		    "特殊技能をひとつも習得していない。"));
	    return FALSE;
	}
	if (flags.menu_style == MENU_TRADITIONAL) {
            if (ntechs == 1)  Strcpy(lets, "a");
            else if (ntechs < 27)  Sprintf(lets, "a-%c", 'a' + ntechs - 1);
            else if (ntechs == 27)  Sprintf(lets, "a-z A");
            else Sprintf(lets, "a-z A-%c", 'A' + ntechs - 27);

	    for(;;)  {
                Sprintf(qbuf, "Perform which technique? [%s ?]", lets);
		if ((ilet = yn_function(qbuf, (char *)0, '\0')) == '?')
		    break;

		if (index(quitchars, ilet))
		    return FALSE;

		if (letter(ilet) && ilet != '@') {
		    /* in a-zA-Z, convert back to an index */
		    if (lowc(ilet) == ilet)     /* lower case */
			idx = ilet - 'a';
		    else
			idx = ilet - 'A' + 26;

                    if (idx < ntechs)
			for(i = 0; i < MAXTECH; i++)
			    if (techid(i) != NO_TECH) {
				if (idx-- == 0) {
				    *tech_no = i;
				    return TRUE;
				}
			    }
		}
                You("don't know that technique.");
	    }
	}
	
	do {
	    selected = dotechmenu(PICK_ONE, tech_no);
	    if (selected && *tech_no == MAXTECH) {
		/* do tech help */
		if (dotechhelp(tech_no)) {
		    dotechdesc(techid(*tech_no));
		    selected = FALSE;
		}
	    }
	} while (selected && *tech_no == MAXTECH);
        return selected;
}

static boolean
dotechmenu(how, tech_no)
	int how;
        int *tech_no;
{
	winid tmpwin;
	int i, n, len, longest, techs_useable, tlevel;
	boolean readytouse;
	char buf[BUFSZ], let = 'a';
	char statstr[BUFSZ];
	const char *prefix;
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;         /* zero out all bits */

	techs_useable = 0;

	longest = 22;
	if (!iflags.menu_tab_sep) {
#ifndef JP                  //0123456789012345678901234567890123456789
	    Sprintf(buf, "    Name                   Level   Status");
#else
	    Sprintf(buf, "    名称                   レベル  状態  ");
#endif
	} else
#ifndef JP
	    Sprintf(buf, "Name\tLevel\tStatus");
#else
	    Sprintf(buf, "名称\tレベル\t状態");
#endif

	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);

	for (i = 0; i < MAXTECH; i++) {
	    if (techid(i) == NO_TECH)
		continue;
	    tlevel = techlev(i);
	    readytouse = FALSE;

	    if (tech_inuse(techid(i))) {
		Sprintf(statstr, E_J("Active", "使用中"));
	    } else if (tlevel <= 0) {
		Sprintf(statstr, E_J("Beyond recall","忘却"));
	    } else {
		if (tech_props[techid(i)].cost1 < 0) {
		    Sprintf(statstr, "%s (Pw: %d)",
			    tech_props[techid(i)].cost2 <= u.uen ? E_J("Prepared","使用可能") : E_J("Not Ready","使用不可"),
			    tech_props[techid(i)].cost2);
		    readytouse = (tech_props[techid(i)].cost2 <= u.uen && tlevel > 0);
		} else {
		    Sprintf(statstr,
			!techtout(i) ? E_J("Prepared","使用可能") : 
			techtout(i) > 100 ? E_J("Not Ready","しばらく使用不可") : E_J("Soon","もうじき使用可能"));
		    readytouse = (!techtout(i) && tlevel > 0);
		}
	    }

	    if (readytouse) {
		/* Ready to use */
		techs_useable++;
		prefix = "";
		any.a_int = i + 1;
	    } else {
		prefix = "    ";
		any.a_int = 0;
	    }

#ifdef WIZARD
	    if (wizard) 
		if (!iflags.menu_tab_sep)			
		    Sprintf(buf, "%s%-*s %2d%c%c%c   %s(%i)",
			    prefix, longest, techname(i), tlevel,
			    tech_list[i].t_intrinsic & FROMEXPER ? 'X' : ' ',
			    tech_list[i].t_intrinsic & FROMRACE ? 'R' : ' ',
			    tech_list[i].t_intrinsic & FROMOUTSIDE ? 'O' : ' ',
			    statstr,
			    techtout(i));
		else
		    Sprintf(buf, "%s%s\t%2d%c%c%c\t%s(%i)",
			    prefix, techname(i), tlevel,
			    tech_list[i].t_intrinsic & FROMEXPER ? 'X' : ' ',
			    tech_list[i].t_intrinsic & FROMRACE ? 'R' : ' ',
			    tech_list[i].t_intrinsic & FROMOUTSIDE ? 'O' : ' ',
			    statstr,
			    techtout(i));
	    else
#endif
	    if (!iflags.menu_tab_sep)
		Sprintf(buf, "%s%-*s %5d   %s",
			prefix, longest, techname(i), tlevel, statstr);
	    else
		Sprintf(buf, "%s%s\t%5d\t%s",
			prefix, techname(i), tlevel, statstr);

	    add_menu(tmpwin, NO_GLYPH, &any,
		    techtout(i) ? 0 : let, 0, ATR_NONE, buf, MENU_UNSELECTED);
	    if (let++ == 'z') let = 'A';
	}

	any.a_int = 0;
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	any.a_int = MAXTECH + 1;
	add_menu(tmpwin, NO_GLYPH, &any, '?', 0, ATR_NONE,
		 E_J("Show help of a technique",
		     "各特殊技能の解説を見る"), MENU_UNSELECTED);

	end_menu(tmpwin, techs_useable ? E_J("Choose a technique", "特殊技能の選択") :
					 E_J("Currently known techniques", "習得した特殊技能"));

	n = select_menu(tmpwin, PICK_ONE, &selected);
	destroy_nhwindow(tmpwin);
	if (n > 0) {
	    *tech_no = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    return TRUE;
	}
	return FALSE;
}

static boolean
dotechhelp(tech_no)
        int *tech_no;
{
	winid tmpwin;
	int i, n;
	char buf[BUFSZ], let = 'a';
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;         /* zero out all bits */

	if (!iflags.menu_tab_sep) {
	    Sprintf(buf, E_J("    Name", "    名称"));
	} else
	    Sprintf(buf, E_J("Name", "名称"));

	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);

	for (i = 0; i < MAXTECH; i++) {
	    if (techid(i) == NO_TECH)
		continue;

	    any.a_int = i + 1;
	    add_menu(tmpwin, NO_GLYPH, &any, let, 0, ATR_NONE, techname(i), MENU_UNSELECTED);
	    if (let++ == 'z') let = 'A';
	}

	end_menu(tmpwin,E_J("Choose a technique", "特殊技能の説明"));

	n = select_menu(tmpwin, PICK_ONE, &selected);
	destroy_nhwindow(tmpwin);
	if (n > 0) {
	    *tech_no = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    return TRUE;
	}
	return FALSE;
}

static int
get_tech_no(tech)
int tech;
{
	int i;

	for (i = 0; i < MAXTECH; i++) {
		if (techid(i) == tech) {
			return(i);
		}
	}
	return (-1);
}

int
dotech()
{
	int tech_no;

	if (gettech(&tech_no))
	    return techeffects(tech_no);
	return 0;
}

int
getobj_filter_surgery(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	if (otyp == BANDAGE)
		return GETOBJ_CHOOSEIT;
	return 0;
}

int
surgery()
{
	int tmp = 0;

	if (Hallucination || Stunned || Confusion) {
	    You(E_J("are in no condition to perform surgery!",
		    "手術を行えるような状態ではない！"));
	    return 0;
	}

	if (Sick || Slimed) {
	    if(carrying(SCALPEL)) {
		pline(E_J("Using your scalpel, you cure your infection!",
			  "メスを使い、あなたは感染した部分を切除した！"));
		make_sick(0L, (char *) 0, TRUE, SICK_NONVOMITABLE);
		Slimed = 0L;
		if(u.uhp > 6) u.uhp -= 5;
		else          u.uhp = 1;
		flags.botl = 1;
		return rn1(500,500);
	    } else pline(E_J("If only you had a scalpel ...",
			     "メスさえあれば手術を行えるのに…。"));
	}

	if (u.uhp < u.uhpmax) {
#ifdef FIRSTAID
	    struct obj *bandage = 0;
#ifdef JP
	    static const struct getobj_words aidw = { 0, "を使って", "応急手当する", "応急手当し" };
#endif /*JP*/
	    if(carrying(BANDAGE)) {
		bandage = getobj((const char *)0, E_J("bandage",&aidw),
				 getobj_filter_surgery);
	    }
	    if (bandage) {
		if (bandage->otyp != BANDAGE) {
		    pline(E_J("That is not good for medical operation.",
			      "それは医療行為に適していない。"));
		} else if (bandage->cursed) {
#ifndef JP
		    Your("medical experience tells it's not clean.");
		    You("throw it away.");
#else
		    pline("長年の医療経験から、あなたはその包帯が清潔でないことに気づいた。");
		    You("包帯を投げ捨てた。");
#endif /*JP*/
		} else {
		    You(E_J("quickly bandage your wounds.",
			    "素早く自分の傷に包帯を巻いた。"));
		    tmp = (u.ulevel * (bcsign(bandage)+2)) + rn1(15,5);
		}
		useup(bandage);
	    }
#endif /*FIRSTAID*/
	    if (!tmp) {
		You(E_J("bandage your wounds as best you can.",
			"できるかぎりの応急処置を行った。"));
		tmp = (u.ulevel) + rn1(5,5);
	    }
	    healup(tmp, 0, FALSE, FALSE);
	    You_feel(E_J("better.","元気になった。"));
	    flags.botl = 1;
	    return rn1(1000,500);
	} else pline(E_J("Don't need healing!","あなたには治療の必要がない！"));
	return 0;
}

#ifdef JP
static const struct getobj_words blessw = { 0, 0, "祝福する", "祝福し" };
#endif

/* gettech is reworked getspell */
/* reworked class special effects code */
/* adapted from specialpower in cmd.c */
static int
techeffects(tech_no)
int tech_no;
{
	/* These variables are used in various techs */
	struct obj *obj, *otmp;
	const char *str;
	struct monst *mtmp;
	int num;
	char Your_buf[BUFSZ];
	char allowall[2];
	int i, j, t_timeout = 0;

	/* check timeout */
	if (tech_inuse(techid(tech_no))) {
	    pline(E_J("This technique is already active!",
		      "その技はすでに使用中だ！"));
	    return (0);
	}
        if (techtout(tech_no)) {
#ifndef JP
	    You("have to wait %s before using your technique again.",
		(techtout(tech_no) > 100) ?
			"for a while" : "a little longer");
#else
	    pline("再び技が使えるようになるまで、あなたは%s待たねばならない。",
		(techtout(tech_no) > 100) ?
			"しばらく" : "あと少し");
#endif /*JP*/
#ifdef WIZARD
            if (!wizard || (yn("Use technique anyways?") == 'n'))
#endif
                return(0);
        }

	/* switch to the tech and do stuff */
        switch (techid(tech_no)) {

            case T_RESEARCH: {
#ifndef JP
		const char *a = "archeological experience ";
		if (findit()) Your("%ssenses secret things.", a);
		else Your("%stells nothing special is around here.", a);
#else
		const char *a = "考古学調査の経験から、あなたは";
		if (findit()) pline("%s隠されたものに気づいた。", a);
		else pline("%s周囲に特別なものがないことを確認した。", a);
#endif /*JP*/
		break;
	    }

            case T_IDENTIFY:
		/* WAC stolen from the spellcasters...'A' can identify from
        	   historical research*/
		if(Hallucination || Stunned || Confusion) {
		    You(E_J("can't concentrate right now!",
			    "今、集中することができない！"));
		    return(0);
		} else if((ACURR(A_INT) + ACURR(A_WIS)) < rnd(60)) {
			pline(E_J("Nothing in your pack looks familiar.",
				  "荷物の中身は、見当もつかないものばかりだ。"));
                    t_timeout = rn1(500,500);
		    break;
		} else if(invent) {
			You(E_J("examine your possessions.",
				"所持品を見分した。"));
			identify_pack((int) ((techlev(tech_no) / 10) + 1));
		} else {
			/* KMH -- fixed non-compliant string */
		    You(E_J("are already quite familiar with the contents of your pack.",
			    "自分の荷物の中身をすべてよく心得ている。"));
		    break;
		}
		break;

            case T_BERSERK:
		You(E_J("fly into a beseark rage!",
			"狂戦士の怒りを解き放った！"));
		techt_inuse(tech_no) = d(2,8) +
               		(techlev(tech_no)/5) + 2;
		incr_itimeout(&HFast, techt_inuse(tech_no));
		break;

            case T_FLURRY:
                Your("%s %s become blurs as they reach for your quiver!",
			uarmg ? "gloved" : "bare",      /* Del Lamb */
			makeplural(body_part(HAND)));
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/6 + 1)) + 2;
		break;

            case T_PRACTICE:
		if(!uwep) {
		    You(E_J("are not wielding a weapon!",
			    "武器を構えていない！"));
		    break;
		} else if(uwep->known == TRUE) {
		    You(E_J("already know what that weapon's quality!",
			    "すでにその武器の品質を見極めている！"));
		    break;
		} else if(is_missile(uwep) || is_ammo(uwep)) {
		    You(E_J("can't practice with that!",
			    "その武器を試し振りできない！"));
		    break;
		} else {
		    You(E_J("study and practice with your weapon.",
			    "自分の武器を試し振りし、見極めはじめた。"));
		    if (rnd(15) <= (ACURR(A_INT) + P_SKILL(weapon_type(uwep)) / 10)) {
			You(E_J("were able to descern it's general quality.",
				"武器の一般的な品質を見極めることができた。"));
			makeknown(uwep->otyp);
			uwep->known = TRUE;
			prinv((char *)0, uwep, 0L);
		    } else 
			pline(E_J("Unfortunatly, you still don't know it's quality.",
				  "残念ながら、あなたはその品質がわからなかった。"));
		}
		break;

            case T_SURGERY:
		t_timeout = surgery();
		if (t_timeout == 0) return(0);
		break;

            case T_HEAL_HANDS:
		if (Slimed) {
		    Your(E_J("body is on fire!",
			     "体が燃え上がった！"));
		    burn_away_slime();
		} else if (Sick) {
		    You(E_J("lay your hands on the foul sickness...",
			    "穢らわしい病巣の上に手をかざした…。"));
		    make_sick(0L, (char*)0, TRUE, SICK_ALL);
		} else if (Upolyd ? u.mh < u.mhmax : u.uhp < u.uhpmax) {
		    pline(E_J("A warm glow spreads through your body!",
			      "暖かな輝きがあなたの身体中に広がった！"));
		    healup(techlev(tech_no) * 4, 0, FALSE, FALSE);
		} else {
		    pline(nothing_happens);
		    return (0);
		}
		break;

            case T_KIII:
		You(E_J("scream \"KIIIII!\"","気合を込めた叫びをあげた！"));
		aggravate();
                techt_inuse(tech_no) = rnd((int) (techlev(tech_no)/6 + 1)) + 2;
		break;

#ifdef STEED
	    case T_CALM_STEED:
                if (u.usteed) {
                        pline(E_J("%s gets tamer.", "%sはより懐いたようだ。"), Monnam(u.usteed));
                        tamedog(u.usteed, (struct obj *) 0);
                } else {
                        Your(E_J("technique is only effective when riding a monster.",
				 "技は騎乗中に使わないと効果がない。"));
			return (0);
		}
                break;
#endif

            case T_TURN_UNDEAD:
                return(doturn());

	    case T_CRIT_STRIKE:
		if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
		    /* Hopefully a mistake ;B */
		    You("decide against that idea.");
		    return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp) {
		    You("perform a flashy twirl!");
		    return (0);
		} else {
		    int oldhp = mtmp->mhp;
		    int tmp;

		    if (!attack(mtmp)) return(0);
		    if (!DEADMONSTER(mtmp) && mtmp->mhp < oldhp &&
			    !noncorporeal(mtmp->data) && !unsolid(mtmp->data)) {
			You("strike %s vital organs!", s_suffix(mon_nam(mtmp)));
			/* Base damage is always something, though it may be
			 * reduced to zero if the hero is hampered. However,
			 * since techlev will never be zero, stiking vital
			 * organs will always do _some_ damage.
			 */
			tmp = mtmp->mhp > 1 ? mtmp->mhp / 2 : 1;
			if (!humanoid(mtmp->data) || is_golem(mtmp->data) ||
				mtmp->data->mlet == S_CENTAUR) {
			    You("are hampered by the differences in anatomy.");
			    tmp /= 2;
			}
			tmp += techlev(tech_no);
//			t_timeout = rn1(1000, 500);
			hurtmon(mtmp, tmp);
		    }
		}
		break;

	    case T_CUTTHROAT:
		if (!is_blade(uwep)) {
		    You("need a blade to perform cutthroat!");
		    return 0;
		}
	    	if (!getdir((char *)0)) return 0;
		if (!u.dx && !u.dy) {
		    /* Hopefully a mistake ;B */
		    pline("Things may be going badly, but that's extreme.");
		    return 0;
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp) {
		    You("attack...nothing!");
		    return 0;
		} else {
		    int oldhp = mtmp->mhp;

		    if (!attack(mtmp)) return 0;
		    if (!DEADMONSTER(mtmp) && mtmp->mhp < oldhp) {
			if (!has_head(mtmp->data) || u.uswallow)
			    You_cant("perform cutthroat on %s!", mon_nam(mtmp));
			else {
			    int tmp = 0;

			    if (rn2(5) < (techlev(tech_no)/10 + 1)) {
				You("sever %s head!", s_suffix(mon_nam(mtmp)));
				tmp = mtmp->mhp;
			    } else {
				You("hurt %s badly!", s_suffix(mon_nam(mtmp)));
				tmp = mtmp->mhp / 2;
			    }
			    tmp += techlev(tech_no);
//			    t_timeout = rn1(1000,500);
			    hurtmon(mtmp, tmp);
			}
		    }
		}
		break;

	    case T_BLESSING:
		allowall[0] = ALL_CLASSES; allowall[1] = '\0';
		
		if ( !(obj = getobj(allowall, E_J("bless", &blessw), 0))) return(0);
		pline(E_J("An aura of holiness surrounds your hands!",
			  "聖なるオーラがあなたの両手を包んだ！"));
                if (!Blind) (void) Shk_Your(Your_buf, obj);
		if (obj->cursed) {
		    if (!Blind)
#ifndef JP
			pline("%s %s %s.",
			      Your_buf,
			      aobjnam(obj, "softly glow"),
			      hcolor(NH_AMBER));
#else
			pline("%s%sが%s%s輝いた。",
			      Your_buf, xname(obj),
			      Hallucination ? "" : "柔らかな",
			      j_no_ni((char *)hcolor(NH_AMBER)));
#endif /*JP*/
		    uncurse(obj);
		    obj->bknown=1;
		} else if(!obj->blessed) {
		    if (!Blind) {
#ifndef JP
			str = hcolor(NH_LIGHT_BLUE);
			pline("%s %s with a%s %s aura.",
			      Your_buf,
			      aobjnam(obj, "softly glow"),
			      index(vowels, *str) ? "n" : "", str);
#else
			pline("%s%sが%s%s柔らかな光芒につつまれた。",
			      Your_buf, xname(obj),
			      hcolor(NH_LIGHT_BLUE));
#endif /*JP*/
		    }
		    bless(obj);
		    obj->bknown=1;
		} else {
		    if (obj->bknown) {
			pline (E_J("That object is already blessed!",
				   "この品物はすでに祝福されている。"));
			return(0);
		    }
		    obj->bknown=1;
		    pline(E_J("The aura fades.","オーラは消えていった。"));
		}
//		t_timeout = rn1(1000,500);
		break;

	    case T_DISARM:
	    	if (P_SKILL(weapon_type(uwep)) == P_NONE) {
	    		You("aren't wielding a proper weapon!");
	    		return(0);
	    	}
	    	if ((P_SKILL(weapon_type(uwep)) < P_SKILLED) || (Blind)) {
	    		You("aren't capable of doing this!");
	    		return(0);
	    	}
		if (u.uswallow) {
	    		pline("What do you think %s is?  A sword swallower?",
				mon_nam(u.ustuck));
	    		return(0);
		}

	    	if (!getdir((char *)0)) return(0);
		if (!u.dx && !u.dy) {
			/* Hopefully a mistake ;B */
			pline("Why don't you try wielding something else instead.");
			return(0);
		}
		mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
		if (!mtmp || !canspotmon(mtmp)) {
		    You("don't know where to aim for!");
		    return (0);
		}
	    	obj = MON_WEP(mtmp);   /* can be null */
	    	if (!obj) {
	    		You_cant("disarm an unarmed foe!");
	    		return(0);
	    	}
		/* Blindness dealt with above */
		if (!mon_visible(mtmp)
				) {
	    		You_cant("see %s weapon!", s_suffix(mon_nam(mtmp)));
	    		return(0);
		}
		num = ((rn2(techlev(tech_no) + 15)) 
			* (P_SKILL(weapon_type(uwep)) - P_SKILLED + 1)) / 10;

		You("attempt to disarm %s...",mon_nam(mtmp));
		/* WAC can't yank out cursed items */
                if (num > 0 && (!Fumbling || !rn2(10)) && !obj->cursed) {
		    int roll;
		    obj_extract_self(obj);
		    possibly_unwield(mtmp, FALSE);
		    setmnotwielded(mtmp, obj);
		    roll = rn2(num + 1);
		    if (roll > 3) roll = 3;
		    switch (roll) {
			case 2:
			    /* to floor near you */
			    You("knock %s %s to the %s!",
				s_suffix(mon_nam(mtmp)),
				xname(obj),
				surface(u.ux, u.uy));
			    if (obj->otyp == CRYSKNIFE &&
				    (!obj->oerodeproof || !rn2(10))) {
				set_otyp(obj, WORM_TOOTH);
				obj->oerodeproof = 0;
			    }
			    place_object(obj, u.ux, u.uy);
			    stackobj(obj);
			    break;
			case 3:
#if 0
			    if (!rn2(25)) {
				/* proficient at disarming, but maybe not
				   so proficient at catching weapons */
				int hitu, hitvalu;

				hitvalu = 8 + obj->spe;
				hitu = thitu(hitvalu,
					dmgval(obj, &youmonst),
					obj, xname(obj));
				if (hitu)
				    pline("%s hits you as you try to snatch it!",
					    The(xname(obj)));
				place_object(obj, u.ux, u.uy);
				stackobj(obj);
				break;
			    }
#endif /* 0 */
			    /* right into your inventory */
			    You("snatch %s %s!", s_suffix(mon_nam(mtmp)),
				    xname(obj));
			    if (obj->otyp == CORPSE &&
				    touch_petrifies(&mons[obj->corpsenm]) &&
				    !uarmg && !Stone_resistance &&
				    !(poly_when_stoned(youmonst.data) &&
					polymon(PM_STONE_GOLEM))) {
				char kbuf[BUFSZ];

				Sprintf(kbuf, "%s corpse",
					an(mons[obj->corpsenm].mname));
				pline("Snatching %s is a fatal mistake.", kbuf);
				instapetrify(kbuf);
			    }
			    obj = hold_another_object(obj, "You drop %s!",
				    doname(obj), (const char *)0);
			    break;
			default:
			    /* to floor beneath mon */
			    You("knock %s from %s grasp!", the(xname(obj)),
				    s_suffix(mon_nam(mtmp)));
			    if (obj->otyp == CRYSKNIFE &&
				    (!obj->oerodeproof || !rn2(10))) {
				set_otyp(obj, WORM_TOOTH);
				obj->oerodeproof = 0;
			    }
			    place_object(obj, mtmp->mx, mtmp->my);
			    stackobj(obj);
			    break;
		    }
		} else if (mtmp->mcanmove && !mtmp->msleeping)
		    pline("%s evades your attack.", Monnam(mtmp));
		else
		    You("fail to dislodge %s %s.", s_suffix(mon_nam(mtmp)),
			    xname(obj));
		wakeup(mtmp);
		if (!mtmp->mcanmove && !rn2(10)) {
		    mtmp->mcanmove = 1;
		    mtmp->mfrozen = 0;
		}
		break;

	    case T_MAINTENANCE: {
#ifdef JP
		static const struct getobj_words maintw = { "どの武具", "を", "手入れする", "手入れし" };
#endif /*JP*/
		if(!freehand()) {
		    You(E_J("have no free %s!",
			    "%sが空いていない！"), body_part(HAND));
		    return 0;
		}
		if (u.uswallow || Underwater) {
		    pline(E_J("This place is not good for maintenance work.",
			      "この場所は作業をするのに向いていない。"));
		    return (0);
		}
		for (otmp = invent, i = 0; otmp; otmp = otmp->nobj) {
		    if (getobj_filter_dwarvish_repair(otmp)) i++;
		}
		if (i > 0) {
		    otmp = getobj((char *)0, E_J("maintain",&maintw), getobj_filter_dwarvish_repair);
		}
		if (!otmp) {
		    Your(E_J("equipment is fully maintained.","武具は手入れが十分行き届いている。"));
		    return 0;
		}
		if (otmp->oeroded) {
		    otmp->oeroded -= min(techlev(tech_no), otmp->oeroded);
		} else if (otmp->oeroded2) {
		    otmp->oeroded2 -= min(techlev(tech_no), otmp->oeroded2);
		}
		You(E_J("maintained your %s %s.","%sを%s手入れした。"), xname(otmp),
		    (otmp->oeroded || otmp->oeroded2) ? E_J("as best", "できるだけ") :
		     E_J("perfectly", "完璧に"));
		break;
	    }

            case T_PURIFY:
		if (dopurify() == 0) return(0);
		break;

            case T_OFUDA:
                techt_inuse(tech_no) = 1;
		i = throw_ofuda();
                techt_inuse(tech_no) = 0;
		if (i == 0) return(0);
		break;

	    case T_HAMAYA:
#ifndef JP
                Your("%s are filled with the power to defeat unholy beings!",
			makeplural(body_part(HAND));
#else
                Your("両%sに破邪の力が宿った！",
			body_part(HAND));
#endif
                techt_inuse(tech_no) = xlev_to_rank(u.ulevel);
		break;

	    default:
	    	pline ("Error!  No such effect (%i)", tech_no);
		break;
        }

	if (t_timeout == 0) {
	    /* get cost of the technique */
	    if (tech_props[techid(tech_no)].cost1 > 0) {
		t_timeout = rn1(tech_props[techid(tech_no)].cost1, tech_props[techid(tech_no)].cost2);
	    } else if (tech_props[techid(tech_no)].cost1 == 0) {
		t_timeout = tech_props[techid(tech_no)].cost2;
	    } else {
		t_timeout = 0;
		if (u.uen >= tech_props[techid(tech_no)].cost2) {
		    u.uen -= tech_props[techid(tech_no)].cost2;
		}
	    }
	}
	techtout(tech_no) = (t_timeout * (100 - techlev(tech_no))/100);

	/*By default,  action should take a turn*/
	return(1);
}

static int
getobj_filter_dwarvish_repair(otmp)
struct obj *otmp;
{
	if ((otmp->oeroded || otmp->oeroded2) &&
	    (otmp->oclass == WEAPON_CLASS ||
	     otmp->oclass == ARMOR_CLASS ||
	     is_weptool(otmp)) && is_metallic(otmp))
	    return GETOBJ_CHOOSEIT;
	else
	    return 0;
}

/* Whether or not a tech is in use.
 * 0 if not in use, turns left if in use. Tech is done when techinuse == 1
 */
int
tech_inuse(tech_id)
int tech_id;
{
        int i;

        if (tech_id < 1 || tech_id > MAXTECH) {
                impossible ("invalid tech: %d", tech_id);
                return(0);
        }
        for (i = 0; i < MAXTECH; i++) {
                if (techid(i) == tech_id) {
                        return (techt_inuse(i));
                }
        }
	return (0);
}

void
tech_timeout()
{
	int i;
	
        for (i = 0; i < MAXTECH; i++) {
	    if (techid(i) == NO_TECH)
		continue;
	    if (techt_inuse(i)) {
	    	/* Check if technique is done */
	        if (!(--techt_inuse(i)))
	        switch (techid(i)) {
		    case T_BERSERK:
#ifndef JP
			The("red haze in your mind clears.");
#else
			Your("心を覆っていた赤いもやが晴れた。");
#endif
			break;
		    case T_KIII:
			You(E_J("calm down.","落ち着いた。"));
			break;
		    case T_FLURRY:
			You(E_J("relax.","緊張が解けた。"));
			break;
		    case T_HAMAYA:
#ifndef JP
			The("power on your %s fades away.", body_part(HAND));
#else
			Your("%sから破邪の力が消えた。", body_part(HAND));
#endif
			break;
	            default:
	            	break;
	        } else switch (techid(i)) {
	        /* During the technique */
//		    case T_RAGE:
//			/* Bleed but don't kill */
//			if (u.uhpmax > 1) u.uhpmax--;
//			if (u.uhp > 1) u.uhp--;
//			break;
//		    case T_POWER_SURGE:
//			/* Bleed off power.  Can go to zero as 0 power is not fatal */
//			if (u.uenmax > 1) u.uenmax--;
//			if (u.uen > 0) u.uen--;
//			break;
	            default:
	            	break;
	        }
	    } 

	    if (techtout(i) > 0) techtout(i)--;
        }
}

void
docalm()
{
	int i, tech, n = 0;

	for (i = 0; i < MAXTECH; i++) {
	    tech = techid(i);
	    if (tech != NO_TECH && techt_inuse(i)) {
		aborttech(tech);
		n++;
	    }
	}
	if (n)
	    You("calm down.");
}

static void
hurtmon(mtmp, tmp)
struct monst *mtmp;
int tmp;
{
	mtmp->mhp -= tmp;
	if (mtmp->mhp < 1) killed (mtmp);
#ifdef SHOW_DMG
	else showdmg(tmp);
#endif
}

static const struct 	innate_tech *
role_tech()
{
	switch (Role_switch) {
		case PM_ARCHEOLOGIST:	return (arc_tech);
		case PM_BARBARIAN:	return (bar_tech);
		case PM_CAVEMAN:	return (cav_tech);
		case PM_HEALER:		return (hea_tech);
		case PM_KNIGHT:		return (kni_tech);
		case PM_MONK: 		return (mon_tech);
		case PM_MEDIUM:		return (med_tech);
		case PM_PRIEST:		return (pri_tech);
		case PM_RANGER:		return (ran_tech);
		case PM_ROGUE:		return (rog_tech);
		case PM_SAMURAI:	return (sam_tech);
		case PM_TOURIST:	return (tou_tech);
		case PM_VALKYRIE:	return (val_tech);
		case PM_WIZARD:		return (wiz_tech);
		default: 		return ((struct innate_tech *) 0);
	}
}

static const struct     innate_tech *
race_tech()
{
	switch (Race_switch) {
		case PM_DWARF:		return (dwa_tech);
		case PM_ELF:		return (elf_tech);
		case PM_GNOME:		return (gno_tech);
		case PM_HOBBIT:		return (hob_tech);
		default: 		return ((struct innate_tech *) 0);
	}
}

void
adjtech(oldlevel,newlevel)
int oldlevel, newlevel;
{
	const struct   innate_tech  
		*tech = role_tech(), *rtech = race_tech();
	long mask = FROMEXPER;

	while (tech || rtech) {
	    /* Have we finished with the tech lists? */
	    if (!tech || !tech->tech_id) {
	    	/* Try the race intrinsics */
	    	if (!rtech || !rtech->tech_id) break;
	    	tech = rtech;
	    	rtech = (struct innate_tech *) 0;
		mask = FROMRACE;
	    }
		
	    for(; tech->tech_id; tech++)
		if(oldlevel < tech->ulevel && newlevel >= tech->ulevel) {
		    if (tech->ulevel != 1 && !tech_known(tech->tech_id))
#ifndef JP
			You("learn how to perform %s!",
			  tech_props[tech->tech_id].name);
#else
			You("「%s」の技を習得した！",
			  jtech_names[tech->tech_id]);
#endif /*JP*/
		    learntech(tech->tech_id, mask, tech->tech_lev);
		} else if (oldlevel >= tech->ulevel && newlevel < tech->ulevel
		    && tech->ulevel != 1) {
		    learntech(tech->tech_id, mask, -1);
		    if (!tech_known(tech->tech_id))
#ifndef JP
			You("lose the ability to perform %s!",
			  tech_props[tech->tech_id].name);
#else
			You("「%s」の技を忘れてしまった！",
			  jtech_names[tech->tech_id]);
#endif /*JP*/
		}
	}
}

/*WAC  draw energy from surrounding objects */
STATIC_PTR int
draw_energy()
{
	int powbonus = 1;
	if (delay) {    /* not if (delay++), so at end delay == 0 */
		delay++;
		confdir();
		if(isok(u.ux + u.dx, u.uy + u.dy)) {
			switch((&levl[u.ux + u.dx][u.uy + u.dy])->typ) {
			    case ALTAR: /* Divine power */
			    	powbonus =  (u.uenmax > 28 ? u.uenmax / 4
			    			: 7);
				break;
			    case THRONE: /* Regal == pseudo divine */
			    	powbonus =  (u.uenmax > 36 ? u.uenmax / 6
			    			: 6);			    		 	
				break;
			    case CLOUD: /* Air */
			    case TREE: /* Earth */
			    case LAVAPOOL: /* Fire */
			    case ICE: /* Water - most ordered form */
			    	powbonus = 5;
				break;
			    case AIR:
			    case MOAT: /* Doesn't freeze */
			    case WATER:
			    	powbonus = 4;
				break;
			    case POOL: /* Can dry up */
			    	powbonus = 3;
				break;
			    case FOUNTAIN:
			    	powbonus = 2;
				break;
			    case SINK:  /* Cleansing water */
			    	if (!rn2(3)) powbonus = 2;
				break;
			    case GRAVE:
			    	powbonus = -4;
				break;
			    default:
				break;
			}
		}
		u.uen += powbonus;
		if (u.uen > u.uenmax) {
			delay = 0;
			u.uen = u.uenmax;
		}
		if (u.uen < 1) u.uen = 0;
		flags.botl = 1;
		return(1); /* still busy */
	}
	You("finish drawing energy from your surroundings.");
	return(0);
}

static int
dopurify()
{
	boolean confused = (Confusion != 0);
	struct obj *obj;
	struct obj pseudo_scrl;

	if (!Role_if(PM_MEDIUM)) {
	    You("みそぎの作法を知らない。");
	    return(0);
	}

	if (!IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
	    pline("みそぎをするには泉が必要だ。");
	    return(0);
	} else
	    if (Levitation && !Lev_at_will && !u.uinwater) {
	    You_cant(E_J("reach the fountain below.","泉に届かない。"));
	    return(0);
	}

	u.uconduct.gnostic++;

	if (Inhell) {
	    pline(E_J("Since you are in Gehennom, %s won't help you.",
		      "ゲヘナの中では、%sの力はあなたに届かない。"),
		  u_gname());
	    aggravate();
	    return(0);
	}

	if ((u.ualign.type != A_CHAOTIC &&
	    (is_demon(youmonst.data) || is_undead(youmonst.data))) ||
	     u.ugangr > 6 /* "Die, mortal!" */) {
	    pline(E_J("Somehow, %s seems to ignore you.",
		      "なぜか、%sはあなたを無視したようだ。"), u_gname());
	    aggravate();
	    exercise(A_WIS, FALSE);
	    return(0);
	}

	if(confused) {
	    if (Hallucination) {
		You("水底のルルイエを見てしまった！ なんてこった！");
		return(0);
	    } else {
		You("準備体操をせずに泳ごうとした！");
		return(0);
	    }
	} else {
	    if (Hallucination) {
		You("%sを洗おうとしたが、%sを忘れてきた！",
		    body_part(HAIR),
		    has_head(youmonst.data) ? "シャンプー" : body_part(HEAD));
		return(0);
	    } else {
		You("%sに祈りをささげ、澄んだ水で身を洗い清めた…。", u_gname());
	    }
	}

	if (u.ulevel < 3) { 
	    pline("あなたはまだ修行が足りないようだ。"); 
	    return(0); 
	}

	if (u.uluck < 0)
	    goto disturbed;

	pseudo_scrl = zeroobj;
	if (levl[u.ux][u.uy].blessedftn || !rn2(6)) {
	    You_feel(E_J("like someone is helping you!",
			 "誰かに助けられているような気がした！"));
	    exercise(A_WIS, TRUE);
	    levl[u.ux][u.uy].blessedftn = 0;
	    /* remove curses in your inventory */
	    pseudo_scrl.blessed = 1;
	    remove_curse(&pseudo_scrl, FALSE);
	    /* cure lycanthropy */
	    if (u.ulycn >= LOW_PM) you_unwere(TRUE);
	    /* cure sickness */
	    use_unicorn_horn((struct obj *)0);
	    nomul(-rnd((MAXULEV+6) - u.ulevel));
	} else if (rn2(3)) {
	    You_feel(E_J("like someone is helping you.",
			 "誰かに助けられているような気がした。"));
	    exercise(A_WIS, TRUE);
	    /* remove curses of worn items */
	    remove_curse(&pseudo_scrl, FALSE);
	    /* cure lycanthropy  */
	    if (u.ulycn >= LOW_PM) you_unwere(TRUE);
	    /* consume the fountain */
	    if (rn2(3)) dryup(u.ux, u.uy, TRUE);
	    nomul(-rnd((MAXULEV+6) - u.ulevel));
	} else if (!rn2(10)) {
disturbed:
	    You_feel(E_J("someone is watching you!  You get dressed in a hurry.",
			 "何者かの視線を感じ、慌てて服を着た！"));
	    aggravate();
	    exercise(A_WIS, FALSE);
	} else {
	    pline("何も起こらなかった。");
	    nomul(-5);
	}

	return (1);
}

static const char ofudas[] = { SCROLL_CLASS, 0 };
#ifdef JP
static const struct getobj_words ofudaw = { "どの巻物", "を", "投げる", "投げ" };
#endif /*JP*/

static int
throw_ofuda()
{
	struct obj *scroll;
	struct monst *mtmp;

	/* check if you are ready to throw an ofuda */
	if (Upolyd) {
#ifndef JP
	    You_cant("use your special power in your current form.");
#else
	    pline("変身中は、あなた本来の特別な力を使うことができない。");
#endif /*JP*/
	    return (0);
	}
	if(check_capacity((char *)0)) return(0);

	/* choose an ofuda to throw */
	scroll = getobj(ofudas, E_J("throw",&ofudaw), 0);
	if (!scroll) return(0);

	throw_obj(scroll, 0);
	return (1);
}

void
ofuda_hit(mon, obj)
struct monst *mon;
struct obj *obj;
{
	struct obj *otmp, *otmp2;
	struct monst *mtmp;
	int ct;
	boolean wake = TRUE;	/* wake monster */
	boolean nohap = TRUE;
	boolean ismimic = FALSE;
	char tmpbuf[BUFSZ];
	char monbuf[BUFSZ];

	if (u.uswallow && mon == u.ustuck) {
	    pline_The(E_J("scroll hits %s %s!","巻物は%sの%sに命中した！"),
		      s_suffix(mon_nam(mon)), mbodypart(mon, STOMACH));
	} else {
	    if (mon->data->mlet == S_MIMIC &&
		mon->m_ap_type != M_AP_NOTHING) {
		ismimic = TRUE;
		(void) lookat(mon->mx, mon->my, tmpbuf, monbuf);
	    }
	    pline_The(E_J("scroll hits %s!","巻物は%sに命中した！"),
		      ismimic ? tmpbuf : mon_nam(mon));
	}

	switch(obj->otyp) {

	case SCR_DESTROY_ARMOR:
	    otmp = some_armor(mon);
	    if (otmp && !u.uswallow) {
		if (canseemon(mon)) {
		    pline(E_J("%s %s turns to dust!",
			      "%s%sは塵と化した！"),
			  s_suffix(Monnam(mon)),
			  distant_name(otmp, xname));
		    makeknown(SCR_DESTROY_ARMOR);
		}
		m_useup(mon, otmp);
		nohap = FALSE;
	    }
	    break;

	case SCR_CONFUSE_MONSTER:
	    if (!mon->mconf) {
		mon->mconf = 1;
		if (!mon->mstun && mon->mcanmove && !mon->msleeping &&
			canseemon(mon)) {
		    pline(E_J("%s appears confused.",
			      "%sは混乱したようだ。"), Monnam(mon));
		    makeknown(SCR_CONFUSE_MONSTER);
		}
		nohap = FALSE;
	    }
	    break;

	case SCR_SCARE_MONSTER: {
	    int wasflee = mon->mflee;
	    monflee(mon, 0, FALSE, TRUE);
	    if (!wasflee && mon->mflee) {
		makeknown(SCR_SCARE_MONSTER);
		nohap = FALSE;
	    }
	    break;
	}

	case SCR_REMOVE_CURSE:
	    if (mon->data->mlet == S_MUMMY ||
		mon->data->mlet == S_ZOMBIE) {
		pline(E_J("The curse on %s is removed and it returns to a corpse.",
			  "%sにかけられていた呪いが祓われ、死体に戻った。"),
		      mon_nam(mon));
		killed(mtmp);
		makeknown(SCR_REMOVE_CURSE);
		nohap = FALSE;
	    }
	    break;

	case SCR_CREATE_MONSTER:
	    if (!u.uswallow) {
		mtmp = make_familiar((struct obj *)0, mon->mx, mon->my, TRUE);
		if (mtmp) {
		    pline(E_J("%s appears to help you!",
			      "%sがあなたを助けに現れた！"), Monnam(mtmp));
		    makeknown(SCR_CREATE_MONSTER);
		    nohap = FALSE;
		}
	    }
	    break;

	case SCR_GOLD_DETECTION:
	    nohap = FALSE;
	    if (mon->mnum == PM_GOLD_GOLEM) {
		pline(E_J("Wow! It is all gold!","わぁ！ 全身が金だ！"));
		if (mon->minvis) {
		    mon->minvis = 0;
		    newsym(mon->mx,mon->my);
		}
		break;
	    }
	    ct = 0;
	    for (otmp = mon->minvent; otmp; otmp = otmp->nobj) {
		if (isgoldobj(otmp)) {
		    ct++;
		    otmp2 = otmp;
		}
	    }
	    if (ct == 0) {
		if (mon->mgold)
		    pline(E_J("%s has %ld %s.","%sは%ld %sの金を持っている。"),
			  Monnam(mon), mon->mgold, currency(mon->mgold));
		else
		    pline(E_J("%s does not have any gold.",
			      "%sは金を持っていない。"), Monnam(mon));
	    } else if (ct == 1 && mon->mgold == 0) {
		pline(E_J("%s has %s.","%sは%sを持っている。"),
		      Monnam(mon), doname(otmp2));
	    } else {
		(void) display_minventory(mon, MINV_ALL|MINV_NOLET|MINV_GOLDOBJ, 0);
	    }
	    makeknown(SCR_GOLD_DETECTION);
	    break;

	case SCR_FOOD_DETECTION:
	    nohap = FALSE;
	    ct = 0;
	    for (otmp = mon->minvent; otmp; otmp = otmp->nobj) {
		if (is_edible(otmp)) {
		    ct++;
		    otmp2 = otmp;
		}
	    }
	    if (ct == 0) {
		pline(E_J("%s does not have any food.",
			  "%sは食べ物を何も持っていない。"), Monnam(mon));
	    } else if (ct == 1) {
		pline(E_J("%s has %s.","%sは%sを持っている。"),
		      Monnam(mon), doname(otmp2));
	    } else {
		(void) display_minventory(mon, MINV_ALL|MINV_NOLET|MINV_FOODOBJ, 0);
	    }
	    makeknown(SCR_FOOD_DETECTION);
	    break;

	case SCR_TAMING: {
	    int waspeaceful = mon->mpeaceful;
	    int wastame     = mon->mtame;
	    maybe_tame(mon, obj);
	    if (waspeaceful != mon->mpeaceful ||
		wastame != mon->mtame) {
		makeknown(SCR_TAMING);
		nohap = FALSE;
	    }
	    if (mon->mpeaceful || mon->mtame)
		wake = FALSE;
	    break;
	}

	case SCR_FIRE: {
	    struct zapinfo zi;
	    setup_zapobj(&zi, obj, TRUE);
	    explode(mon->mx, mon->my, &zi, (2*(rn1(3, 3) + 2 * bcsign(obj)) + 1), EXPL_FIERY);
	    makeknown(SCR_FIRE);
	    nohap = FALSE;
	}

	case SCR_EARTH:
	    use_scr_earth(mon, obj, mon->mx, mon->my, TRUE, FALSE);
	    makeknown(SCR_EARTH);
	    nohap = FALSE;
	    break;

	case SCR_STINKING_CLOUD:
	    (void) create_stinking_cloud(mon->mx, mon->my, 3+bcsign(obj),
					 8+4*bcsign(obj), TRUE);
	    makeknown(SCR_STINKING_CLOUD);
	    nohap = FALSE;
	    break;

	case SCR_LIGHT:
	    if (flash_hits_mon(mon, obj)) {
		nohap = FALSE;
		makeknown(SCR_LIGHT);
	    }
	    break;

	case SCR_TELEPORTATION:
	    u_teleport_mon(mon, TRUE);
	    makeknown(SCR_TELEPORTATION);
	    nohap = FALSE;
	    break;

	case SCR_IDENTIFY: {
	    long save_invis;
	    int mcham;
	    if (!cansee(mon->mx, mon->my)) break;
	    save_invis = HSee_invisible;
	    HSee_invisible |= 1;
	    if (ismimic) {
		seemimic(mon);
		pline_The(E_J("%s is actually a %s!","%sは%sの擬態だった！"),
			  tmpbuf, mon_nam(mon));
	    }
	    mcham = mon->cham;
	    if (mcham != CHAM_ORDINARY) {
		strcpy(monbuf, mon_nam(mon));
		mon->cham = CHAM_ORDINARY;
		(void) newcham(mon, &mons[cham_to_pm[mcham]],
			       FALSE, FALSE);
		mon->mcan = 1;
		pline_The(E_J("%s is actually a %s!","%sの正体は%sだった！"),
			  monbuf, mon_nam(mon));
	    }
	    probe_monster(mon);
	    nohap = FALSE;
	    makeknown(SCR_IDENTIFY);
	    HSee_invisible = save_invis;
	    break;
	}

	case SCR_GENOCIDE: {
	    struct zapinfo zi;
	    int oldhp = mon->mhp;
	    setup_zapinfo(&zi, AT_MAGC, AD_DETH, 1, 1,
			  E_J("lethal power","致死の魔力"),
			  E_J("power","魔力"), TRUE);
	    zhitm(mon, &zi, &otmp);
	    if (mon->mhp < oldhp) {
		xkilled(mon, 1);
		makeknown(SCR_GENOCIDE);
	    } else {
		if (mon->mnum == PM_DEATH) {
		    if (canseemon(mon)) {
			pline(E_J("%s absorbs the deadly power!",
				  "%sは致死の魔力を吸収している！"),
				  Monnam(mon));
			pline(E_J("It seems even stronger than before.",
				  "そして、前よりも強大になったように見える。"));
		    }
		} else {
		    pline(E_J("%s seems not affected.",
			      "%sには効果がないようだ。"),
			      Monnam(mon));
		}
	    }
	    nohap = FALSE;
	    break;
	}

	case SCR_AMNESIA:
	    if (mindless(mon->data)) break;
	    mon->mtrapseen = 0L;
	    mon->mflee = 0;
	    if (mon->mtame) {
		pline(E_J("%s seems to forget about you...",
			  "%sはあなたのことを忘れてしまったようだ…。"),
		      Monnam(mon));
		mon->mtame = 0;
		newsym(mon->mx, mon->my);
		if (mon->mleashed) {
		    pline(E_J("%s gets angry at being leashed!",
			      "%sは綱で繋がれていることに気づき、怒った！"),
			  Monnam(mon));
		    m_unleash(mon, TRUE);
		} else
		    wake = 0;
	    } else if (!mon->mpeaceful && always_peaceful(mon->data)) {
		if (mon->isshk) {
		    struct eshk *eshkp = ESHK(mon);
		    eshkp->visitct = 0;
		    eshkp->customer[0] = 0;
		    if (inhishop(mon)) {
			verbalize(E_J("What have I been doing?",
				      "今まで何をしていたんだっけ？"));
		    } else {
			verbalize(E_J("Where am I? What am I doing here?",
				      "ここはどこだ？ 私はここで何をしてるんだ？"));
		    }
		    make_happy_shk(mon, FALSE, TRUE);
		} else {
		    verbalize(E_J("What have I been doing?",
				  "今まで何をしていたんだっけ？"));
		    setmpeaceful(mon, FALSE);
		}
		wake = 0;
	    } else {
		if (!mon->mconf) {
		    mon->mconf = 1;
		    if (!mon->mstun && mon->mcanmove && !mon->msleeping &&
			    canseemon(mon))
			pline(E_J("%s appears confused.",
				  "%sは混乱したようだ。"), Monnam(mon));
		}
	    }
	    nohap = FALSE;
	    break;

	case SCR_CHARGING:
	    if (mon->mspec_used) {
		mon->mspec_used = 0;
		nohap = 0;
		if (canseemon(mon))
		    pline(E_J("%s seems to regain its power.",
			      "%sは力を取り戻したようだ。"), Monnam(mon));
	    }
	    break;

	case SCR_PUNISHMENT:
	    if (mon->misc_worn_check & W_ARMH) break;
	    if (!has_head(mon->data)) break;
	    otmp = mksobj(DUNCE_CAP, TRUE, FALSE);
	    otmp->etherial = 1;
	    curse(otmp);
	    (void) mpickobj(mon, otmp);
	    mon->misc_worn_check |= W_ARMH;
	    otmp->owornmask |= W_ARMH;
	    update_mon_intrinsics(mon, otmp, TRUE, TRUE);
	    nohap = 0;
	    if (canseemon(mon))
#ifndef JP
		pline("%s appears on %s %s!",
		      An(xname(otmp)), s_suffix(mon_nam(mtmp)), body_part(HEAD));
#else
		pline("%sの%s上に%sが現れた！",
		      Monnam(mon), body_part(HEAD), xname(otmp));
#endif
	    break;

	case SCR_ENCHANT_ARMOR:
	case SCR_ENCHANT_WEAPON:
	case SCR_MAGIC_MAPPING:
	default:
	    break;
	}

	if(wake) {
	    if(mon->mhp > 0) {
		wakeup(mon);
		m_respond(mon);
//		if(mon->isshk && !*u.ushops) hot_pursuit(mon); /* done in thitmonst() */
	    } else if(mon->m_ap_type)
		seemimic(mon); /* might unblock if mimicing a boulder/door */
	}

	if (nohap) pline(nothing_happens);
}

#ifdef DEBUG
void
wiz_debug_cmd() /* in this case, allow controlled loss of techniques */
{
	int tech_no, id, n = 0;
	long mask;
	if (gettech(&tech_no)) {
		id = techid(tech_no);
		if (id == NO_TECH) {
		    impossible("Unknown technique ([%d])?", tech_no);
		    return;
		}
		mask = tech_list[tech_no].t_intrinsic;
		if (mask & FROMOUTSIDE) n++;
		if (mask & FROMRACE) n++;
		if (mask & FROMEXPER) n++;
		if (!n) {
		    impossible("No intrinsic masks set (0x%lX).", mask);
		    return;
		}
		n = rn2(n);
		if (mask & FROMOUTSIDE && !n--) mask = FROMOUTSIDE;
		if (mask & FROMRACE && !n--) mask = FROMRACE;
		if (mask & FROMEXPER && !n--) mask = FROMEXPER;
		learntech(id, mask, -1);
		if (!tech_known(id))
		    You("lose the ability to perform %s.", tech_names[id]);
	}
}
#endif /* DEBUG */
