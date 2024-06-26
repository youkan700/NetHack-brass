/*	SCCS Id: @(#)decl.c	3.2	2001/12/10	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

int NDECL((*afternmv));
int NDECL((*occupation));

/* from xxxmain.c */
const char *hname = 0;		/* name of the game (argv[0] of main) */
int hackpid = 0;		/* current process id */
#if defined(UNIX) || defined(VMS)
int locknum = 0;		/* max num of simultaneous users */
#endif
#ifdef DEF_PAGER
char *catmore = 0;		/* default pager */
#endif

NEARDATA int bases[MAXOCLASSES] = DUMMY;

NEARDATA int multi = 0;
#if 0
NEARDATA int warnlevel = 0;		/* used by movemon and dochugw */
#endif
NEARDATA int nroom = 0;
NEARDATA int nsubroom = 0;
NEARDATA int occtime = 0;

int x_maze_max, y_maze_max;	/* initialized in main, used in mkmaze.c */
int otg_temp;			/* used by object_to_glyph() [otg] */

#ifdef REDO
NEARDATA int in_doagain = 0;
#endif

/*
 *	The following structure will be initialized at startup time with
 *	the level numbers of some "important" things in the game.
 */
struct dgn_topology dungeon_topology = {DUMMY};

#include "quest.h"
struct q_score	quest_status = DUMMY;

NEARDATA int smeq[MAXNROFROOMS+1] = DUMMY;
NEARDATA int doorindex = 0;

NEARDATA char *save_cm = 0;
NEARDATA int killer_format = 0;
const char *killer = 0;
const char *delayed_killer = 0;
struct dkiller_info delayed_killers[MAX_DKILLER];
char killer_buf[BUFSZ] = DUMMY;
char dkiller_buf[BUFSZ] = DUMMY;
#ifdef JP
char killer_buf_with_separator[BUFSZ] = DUMMY;
#endif /*JP*/
const char *nomovemsg = 0;
const char nul[40] = DUMMY;			/* contains zeros */
NEARDATA char plname[PL_NSIZ] = DUMMY;		/* player name */
NEARDATA char pl_character[PL_CSIZ] = DUMMY;
NEARDATA char pl_race = '\0';

NEARDATA char pl_fruit[PL_FSIZ] = DUMMY;
NEARDATA int current_fruit = 0;
NEARDATA struct fruit *ffruit = (struct fruit *)0;
NEARDATA char pl_weapon[PL_PSIZ] = DUMMY;

NEARDATA char tune[6] = DUMMY;

const char *occtxt = DUMMY;
const char quitchars[] = " \r\n\033";
const char vowels[] = "aeiouAEIOU";
const char ynchars[] = "yn";
const char ynqchars[] = "ynq";
const char ynaqchars[] = "ynaq";
const char ynNaqchars[] = "yn#aq";
NEARDATA long yn_number = 0L;

const char disclosure_options[] = "iavgc";

#if defined(MICRO) || defined(WIN32)
char hackdir[PATHLEN];		/* where rumors, help, record are */
# ifdef MICRO
char levels[PATHLEN];		/* where levels are */
# endif
#endif /* MICRO || WIN32 */


#ifdef MFLOPPY
char permbones[PATHLEN];	/* where permanent copy of bones go */
int ramdisk = FALSE;		/* whether to copy bones to levels or not */
int saveprompt = TRUE;
const char *alllevels = "levels.*";
const char *allbones = "bones*.*";
#endif

struct linfo level_info[MAXLINFO];

NEARDATA struct sinfo program_state;

/* 'rogue'-like direction commands (cmd.c) */
const char sdir[] = "hykulnjb><";
const char ndir[] = "47896321><";	/* number pad mode */
const schar xdir[10] = { -1,-1, 0, 1, 1, 1, 0,-1, 0, 0 };
const schar ydir[10] = {  0,-1,-1,-1, 0, 1, 1, 1, 0, 0 };
const schar zdir[10] = {  0, 0, 0, 0, 0, 0, 0, 0, 1,-1 };

NEARDATA schar tbx = 0, tby = 0;	/* mthrowu: target */

/* for xname handling of multiple shot missile volleys:
   number of shots, index of current one, validity check, shoot vs throw */
NEARDATA struct multishot m_shot = { 0, 0, STRANGE_OBJECT, FALSE };

NEARDATA struct dig_info digging;

NEARDATA dungeon dungeons[MAXDUNGEON];	/* ini'ed by init_dungeon() */
NEARDATA s_level *sp_levchn;
NEARDATA stairway upstair = { 0, 0 }, dnstair = { 0, 0 };
NEARDATA stairway upladder = { 0, 0 }, dnladder = { 0, 0 };
NEARDATA stairway sstairs = { 0, 0 };
NEARDATA dest_area updest = { 0, 0, 0, 0, 0, 0, 0, 0 };
NEARDATA dest_area dndest = { 0, 0, 0, 0, 0, 0, 0, 0 };
NEARDATA coord inv_pos = { 0, 0 };

NEARDATA boolean in_mklev = FALSE;
NEARDATA boolean stoned = FALSE;	/* done to monsters hit by 'c' */
NEARDATA boolean unweapon = FALSE;
NEARDATA boolean mrg_to_wielded = FALSE;
			 /* weapon picked is merged with wielded one */
NEARDATA struct obj *current_wand = 0;	/* wand currently zapped/applied */

NEARDATA boolean in_steed_dismounting = FALSE;

/*NEARDATA coord bhitpos = DUMMY;*/
NEARDATA struct bresenham bhitpos = DUMMY;
NEARDATA uchar bhitflag;
NEARDATA coord doors[DOORMAX] = {DUMMY};

NEARDATA struct mkroom rooms[(MAXNROFROOMS+1)*2] = {DUMMY};
NEARDATA struct mkroom* subrooms = &rooms[MAXNROFROOMS+1];
struct mkroom *upstairs_room, *dnstairs_room, *sstairs_room;

dlevel_t level;		/* level map */
struct trap *ftrap = (struct trap *)0;
NEARDATA struct monst youmonst = DUMMY;
NEARDATA struct flag flags = DUMMY;
NEARDATA struct instance_flags iflags = DUMMY;
NEARDATA struct you u = DUMMY;

NEARDATA struct obj *invent = (struct obj *)0,
	*uwep = (struct obj *)0, *uarm = (struct obj *)0,
	*uswapwep = (struct obj *)0,
	*uquiver = (struct obj *)0, /* quiver */
	*uarmu = (struct obj *)0, /* under-wear, so to speak */
	*uskin = (struct obj *)0, /* dragon armor, if a dragon */
	*uarmc = (struct obj *)0, *uarmh = (struct obj *)0,
	*uarms = (struct obj *)0, *uarmg = (struct obj *)0,
	*uarmf = (struct obj *)0, *uamul = (struct obj *)0,
	*uright = (struct obj *)0,
	*uleft = (struct obj *)0,
	*ublindf = (struct obj *)0,
	*uchain = (struct obj *)0,
	*uball = (struct obj *)0;

#ifdef TEXTCOLOR
/*
 *  This must be the same order as used for buzz() in zap.c.
 */
const int zapcolors[NUM_ZAP] = {
    HI_ZAP,		/* 0 - missile */
    CLR_ORANGE,		/* 1 - fire */
    CLR_WHITE,		/* 2 - frost */
    HI_ZAP,		/* 3 - sleep */
    CLR_WHITE,		/* 4 - lightning */
    CLR_YELLOW,		/* 5 - poison gas */
    CLR_GREEN,		/* 6 - acid */
    CLR_BLACK,		/* 7 - death */
    CLR_MAGENTA,	/* 8 - paralysis */
    CLR_BLACK,		/* 9 - death */
};
#endif /* text color */

const int shield_static[SHIELD_COUNT] = {
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,	/* 7 per row */
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
};

NEARDATA struct spell spl_book[MAXSPELL + 1] = {DUMMY};

NEARDATA struct tech tech_list[MAXTECH + 1] = {DUMMY};

NEARDATA long moves = 1L, monstermoves = 1L;
	 /* These diverge when player is Fast */
NEARDATA long wailmsg = 0L;

/* objects that are moving to another dungeon level */
NEARDATA struct obj *migrating_objs = (struct obj *)0;
/* objects not yet paid for */
NEARDATA struct obj *billobjs = (struct obj *)0;

/* used to zero all elements of a struct obj */
NEARDATA struct obj zeroobj = DUMMY;

/* originally from dog.c */
NEARDATA char dogname[PL_PSIZ] = DUMMY;
NEARDATA char catname[PL_PSIZ] = DUMMY;
NEARDATA char horsename[PL_PSIZ] = DUMMY;
char preferred_pet;	/* '\0', 'c', 'd', 'n' (none) */
/* monsters that went down/up together with @ */
NEARDATA struct monst *mydogs = (struct monst *)0;
/* monsters that are moving to another dungeon level */
NEARDATA struct monst *migrating_mons = (struct monst *)0;
struct monst *monactive = (struct monst *)0;

NEARDATA struct mvitals mvitals[NUMMONS];
/* used to zero all elements of a struct monst */
NEARDATA struct monst zeromonst = DUMMY;

NEARDATA struct c_color_names c_color_names = {
#ifndef JP
	"black", "amber", "golden",
	"light blue", "red", "green",
	"silver", "blue", "purple",
	"white"
#else
	"黒い", "琥珀色の", "金色の",
	"明るい青色の", "赤い", "緑色の",
	"銀色の", "青い", "紫色の",
	"白い"
#endif /*JP*/
};

const char *c_obj_colors[] = {
	E_J("black",		"黒い"		),	/* CLR_BLACK */
	E_J("red",		"赤い"		),	/* CLR_RED */
	E_J("green",		"緑色の"	),	/* CLR_GREEN */
	E_J("brown",		"茶色い"	),	/* CLR_BROWN */
	E_J("blue",		"青い"		),	/* CLR_BLUE */
	E_J("magenta",		"紫色の"	),	/* CLR_MAGENTA */
	E_J("cyan",		"水色の"	),	/* CLR_CYAN */
	E_J("gray",		"灰色の"	),	/* CLR_GRAY */
	E_J("transparent",	"透明な"	),	/* no_color */
	E_J("orange",		"オレンジ色の"	),	/* CLR_ORANGE */
	E_J("bright green",	"明るい緑色の"	),	/* CLR_BRIGHT_GREEN */
	E_J("yellow",		"黄色い"	),	/* CLR_YELLOW */
	E_J("bright blue",	"明るい青色の"	),	/* CLR_BRIGHT_BLUE */
	E_J("bright magenta",	"明るい紫色の"	),	/* CLR_BRIGHT_MAGENTA */
	E_J("bright cyan",	"明るい水色の"	),	/* CLR_BRIGHT_CYAN */
	E_J("white",		"白い"		),	/* CLR_WHITE */
};

#ifndef JP
struct c_common_strings c_common_strings = {
	"Nothing happens.",		"That's enough tries!",
	"That is a silly thing to %s.",	"shudder for a moment.",
	"something", "Something", "You can move again.", "Never mind.",
	"vision quickly clears.", {"the", "your"}
};
#else
struct c_common_strings c_common_strings = {
	"何も起こらなかった。",	"もう諦めろ！",
	"それを%sなんて、ばかげている。", "一瞬身震いした。",
	"何か", "何か", "あなたは再び動けるようになった。", "お気になさらず。",
	"視界はすぐにはっきりした。", {"", ""}
};
#endif /*JP*/

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
const char *materialnm[] = {
	"mysterious", "liquid", "wax", "organic", "flesh",
	"paper", "cloth", "leather", "wooden", "bone", "dragonhide",
	"iron", "metal", "copper", "silver", "gold", "platinum", "mithril",
	"plastic", "glass", "gemstone", "stone"
};
const char *materialnm2[] = {
	"mysterious ", "ice ", "wax ", "organic ", "flesh ",
	"paper ", "cloth ", "leather ", "wooden ", "bone ", "dragonhide ",
	"iron ", "mystic ", "bronze ", "silver ", "golden ", "platinum ", "mithril ",
	"plastic ", "crystal ", "gemstone ", "stone "
};
/* used if object is made of non-standard materials */
const uchar materialcolor[] = {
	HI_OBJ, CLR_BRIGHT_CYAN, CLR_WHITE, HI_ORGANIC, HI_ORGANIC,
	HI_PAPER, HI_CLOTH, HI_LEATHER, HI_WOOD, CLR_WHITE, HI_LEATHER,
	HI_METAL, HI_METAL, HI_COPPER, HI_SILVER, HI_GOLD, HI_SILVER, HI_SILVER,
	CLR_WHITE, CLR_WHITE, CLR_BRIGHT_MAGENTA, HI_MINERAL
};
/* material's specific gravity */
const short materialwt[] = {
	 100,  100,   90,  100,  100,
	 100,  100,  100,   50,  200,  100,
	 785,  785,  890, 1050, 1930, 2140,  450,
	 100,  265,  300,  300
};

/* convert chameleon index to monster index */
const short cham_to_pm[4] = {
	NON_PM,		/* placeholder for CHAM_ORDINARY */
	PM_CHAMELEON,
	PM_DOPPELGANGER,
	PM_SANDESTIN
};

/* Vision */
NEARDATA boolean vision_full_recalc = 0;
NEARDATA char	 **viz_array = 0;/* used in cansee() and couldsee() macros */

/* Global windowing data, defined here for multi-window-system support */
NEARDATA winid WIN_MESSAGE = WIN_ERR, WIN_STATUS = WIN_ERR;
NEARDATA winid WIN_MAP = WIN_ERR, WIN_INVEN = WIN_ERR;
NEARDATA winid WIN_HPBAR = WIN_ERR;
char toplines[TBUFSZ];
/* Windowing stuff that's really tty oriented, but present for all ports */
struct tc_gbl_data tc_gbl_data = { 0,0, 0,0 };	/* AS,AE, LI,CO */

char *fqn_prefix[PREFIX_COUNT] = { (char *)0, (char *)0, (char *)0, (char *)0,
				(char *)0, (char *)0, (char *)0, (char *)0, (char *)0 };

#ifdef PREFIXES_IN_USE
char *fqn_prefix_names[PREFIX_COUNT] = { "hackdir", "leveldir", "savedir",
					"bonesdir", "datadir", "scoredir",
					"lockdir", "configdir", "troubledir" };
#endif

/* getobj */
const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
struct obj *getobj_override;

/* dummy routine used to force linkage */
void
decl_init()
{
    return;
}

/*decl.c*/
