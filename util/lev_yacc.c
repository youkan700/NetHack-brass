#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "lev_comp.y"
/*	SCCS Id: @(#)lev_yacc.c	3.4	2000/01/17	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the Level Compiler code
 * It may handle special mazes & special room-levels
 */

/* In case we're using bison in AIX.  This definition must be
 * placed before any other C-language construct in the file
 * excluding comments and preprocessor directives (thanks IBM
 * for this wonderful feature...).
 *
 * Note: some cpps barf on this 'undefined control' (#pragma).
 * Addition of the leading space seems to prevent barfage for now,
 * and AIX will still see the directive.
 */
#ifdef _AIX
 #pragma alloca		/* keep leading space! */
#endif

#include "hack.h"
#include "sp_lev.h"

#define MAX_REGISTERS	10
#define ERR		(-1)
/* many types of things are put in chars for transference to NetHack.
 * since some systems will use signed chars, limit everybody to the
 * same number for portability.
 */
#define MAX_OF_TYPE	128

#define New(type)		\
	(type *) memset((genericptr_t)alloc(sizeof(type)), 0, sizeof(type))
#define NewTab(type, size)	(type **) alloc(sizeof(type *) * size)
#define Free(ptr)		free((genericptr_t)ptr)

extern void FDECL(yyerror, (const char *));
extern void FDECL(yywarning, (const char *));
extern int NDECL(yylex);
int NDECL(yyparse);

extern int FDECL(get_floor_type, (CHAR_P));
extern int FDECL(get_room_type, (char *));
extern int FDECL(get_trap_type, (char *));
extern int FDECL(get_monster_id, (char *,CHAR_P));
extern int FDECL(get_object_id, (char *,CHAR_P));
extern boolean FDECL(check_monster_char, (CHAR_P));
extern boolean FDECL(check_object_char, (CHAR_P));
extern char FDECL(what_map_char, (CHAR_P));
extern void FDECL(scan_map, (char *));
extern void NDECL(wallify_map);
extern boolean NDECL(check_subrooms);
extern void FDECL(check_coord, (int,int,const char *));
extern void NDECL(store_part);
extern void NDECL(store_room);
extern boolean FDECL(write_level_file, (char *,splev *,specialmaze *));
extern void FDECL(free_rooms, (splev *));

static struct reg {
	int x1, y1;
	int x2, y2;
}		current_region;

static struct coord {
	int x;
	int y;
}		current_coord, current_align;

static struct size {
	int height;
	int width;
}		current_size;

char tmpmessage[256];
digpos *tmppass[32];
char *tmpmap[ROWNO];

digpos *tmpdig[MAX_OF_TYPE];
region *tmpreg[MAX_OF_TYPE];
lev_region *tmplreg[MAX_OF_TYPE];
door *tmpdoor[MAX_OF_TYPE];
drawbridge *tmpdb[MAX_OF_TYPE];
walk *tmpwalk[MAX_OF_TYPE];

room_door *tmprdoor[MAX_OF_TYPE];
trap *tmptrap[MAX_OF_TYPE];
monster *tmpmonst[MAX_OF_TYPE];
object *tmpobj[MAX_OF_TYPE];
altar *tmpaltar[MAX_OF_TYPE];
lad *tmplad[MAX_OF_TYPE];
stair *tmpstair[MAX_OF_TYPE];
gold *tmpgold[MAX_OF_TYPE];
engraving *tmpengraving[MAX_OF_TYPE];
fountain *tmpfountain[MAX_OF_TYPE];
sink *tmpsink[MAX_OF_TYPE];
pool *tmppool[MAX_OF_TYPE];

mazepart *tmppart[10];
room *tmproom[MAXNROFROOMS*2];
corridor *tmpcor[MAX_OF_TYPE];

static specialmaze maze;
static splev special_lev;
static lev_init init_lev;

static char olist[MAX_REGISTERS], mlist[MAX_REGISTERS];
static struct coord plist[MAX_REGISTERS];

int n_olist = 0, n_mlist = 0, n_plist = 0;

unsigned int nlreg = 0, nreg = 0, ndoor = 0, ntrap = 0, nmons = 0, nobj = 0;
unsigned int ndb = 0, nwalk = 0, npart = 0, ndig = 0, nlad = 0, nstair = 0;
unsigned int naltar = 0, ncorridor = 0, nrooms = 0, ngold = 0, nengraving = 0;
unsigned int nfountain = 0, npool = 0, nsink = 0, npass = 0;

static int lev_flags = 0;

unsigned int max_x_map, max_y_map;

static xchar in_room;

extern int fatal_error;
extern int want_warnings;
extern const char *fname;

#line 131 "lev_comp.y"
typedef union
{
	int	i;
	char*	map;
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} corpos;
} YYSTYPE;
#line 151 "y.tab.c"
#define CHAR 257
#define INTEGER 258
#define BOOLEAN 259
#define PERCENT 260
#define MESSAGE_ID 261
#define MAZE_ID 262
#define LEVEL_ID 263
#define LEV_INIT_ID 264
#define GEOMETRY_ID 265
#define NOMAP_ID 266
#define OBJECT_ID 267
#define COBJECT_ID 268
#define MONSTER_ID 269
#define TRAP_ID 270
#define DOOR_ID 271
#define DRAWBRIDGE_ID 272
#define MAZEWALK_ID 273
#define WALLIFY_ID 274
#define REGION_ID 275
#define FILLING 276
#define RANDOM_OBJECTS_ID 277
#define RANDOM_MONSTERS_ID 278
#define RANDOM_PLACES_ID 279
#define ALTAR_ID 280
#define LADDER_ID 281
#define STAIR_ID 282
#define NON_DIGGABLE_ID 283
#define NON_PASSWALL_ID 284
#define ROOM_ID 285
#define PORTAL_ID 286
#define TELEPRT_ID 287
#define BRANCH_ID 288
#define LEV 289
#define CHANCE_ID 290
#define CORRIDOR_ID 291
#define GOLD_ID 292
#define ENGRAVING_ID 293
#define FOUNTAIN_ID 294
#define POOL_ID 295
#define SINK_ID 296
#define NONE 297
#define RAND_CORRIDOR_ID 298
#define DOOR_STATE 299
#define LIGHT_STATE 300
#define CURSE_TYPE 301
#define ENGRAVING_TYPE 302
#define WALLSIGN_ID 303
#define ROOMSHAPE 304
#define DIRECTION 305
#define RANDOM_TYPE 306
#define O_REGISTER 307
#define M_REGISTER 308
#define P_REGISTER 309
#define A_REGISTER 310
#define ALIGNMENT 311
#define LEFT_OR_RIGHT 312
#define CENTER 313
#define TOP_OR_BOT 314
#define ALTAR_TYPE 315
#define UP_OR_DOWN 316
#define SUBROOM_ID 317
#define NAME_ID 318
#define FLAGS_ID 319
#define FLAG_TYPE 320
#define MON_ATTITUDE 321
#define MON_ALERTNESS 322
#define MON_APPEARANCE 323
#define CONTAINED 324
#define MPICKUP 325
#define MINVENT 326
#define BURIED 327
#define OBJ_RANK 328
#define STRING 329
#define MAP_ID 330
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,   37,   37,   38,   38,   39,   40,   32,   23,
   23,   23,   14,   14,   19,   19,   20,   20,   41,   41,
   46,   43,   43,   47,   47,   44,   44,   50,   50,   45,
   45,   52,   53,   53,   54,   54,   35,   51,   51,   57,
   55,   10,   10,   60,   60,   58,   58,   61,   61,   59,
   59,   56,   56,   62,   62,   62,   62,   62,   62,   62,
   62,   62,   62,   62,   62,   62,   62,   63,   64,   65,
   15,   15,   13,   13,   12,   12,   31,   11,   11,   11,
   42,   42,   77,   78,   78,   81,    1,    1,    1,    2,
    2,    2,   79,   79,   82,   82,   82,   48,   48,   49,
   49,   83,   85,   83,   80,   80,   86,   86,   86,   86,
   86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
   86,   86,   86,   86,   86,   86,   86,  101,   66,  100,
  100,  102,  102,  102,  102,  102,   67,   67,  104,  103,
   36,   36,   36,  105,  105,  105,  105,  105,  106,  106,
  106,  106,  107,  107,  108,  109,  109,  110,  110,  110,
   87,   68,   88,   94,   95,   96,   76,  111,   90,  112,
   91,  113,  115,   92,  116,   93,  114,  114,   22,   22,
   70,   71,   72,   97,   98,   89,   69,   73,   74,   75,
   25,   25,   25,   28,   28,   28,   33,   33,   34,   34,
    3,    3,    4,    4,   21,   21,   21,   21,   99,   99,
   99,    5,    5,    6,    6,    7,    7,    7,    8,    8,
  119,   29,   26,    9,   84,   24,   27,   30,   16,   16,
   17,   17,   18,   18,  118,  117,
};
short yylen[] = {                                         2,
    0,    1,    1,    2,    1,    1,    5,    7,    3,    0,
   13,   11,    1,    1,    0,    3,    3,    1,    0,    2,
    3,    0,    2,    3,    3,    0,    1,    1,    2,    1,
    1,    1,    0,    2,    5,    5,    7,    2,    2,   12,
   12,    0,    2,    5,    1,    5,    1,    5,    1,    5,
    1,    0,    2,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    3,    3,    9,
    1,    1,    1,    1,    1,    1,    5,    1,    1,    1,
    1,    2,    3,    1,    2,    5,    1,    1,    1,    1,
    1,    1,    0,    2,    3,    3,    3,    1,    3,    1,
    3,    1,    0,    4,    0,    2,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    0,   10,    0,
    2,    2,    2,    2,    2,    3,    2,    2,    0,    9,
    1,    1,    1,    1,    1,    2,    1,    2,    0,    7,
    5,    5,    1,    1,    1,    1,    1,    0,    2,    2,
    5,    6,    7,    5,    1,    5,    5,    0,    8,    0,
    8,    0,    0,    8,    0,    6,    0,    2,    1,   10,
    3,    3,    3,    3,    3,    8,    7,    5,    7,    9,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    0,    2,    4,    4,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    4,    4,    4,    4,    1,    1,    1,    1,    1,    1,
    0,    1,    1,    1,    5,    9,
};
short yydefred[] = {                                      0,
    0,    0,    0,    0,    0,    2,    0,    5,    6,    0,
    0,    0,    0,    0,    4,  228,    0,    9,    0,    0,
    0,    0,    0,    0,   16,    0,    0,    0,    0,   22,
   78,   80,   79,   77,    0,    0,    0,    0,   84,    7,
    0,   93,    0,   20,    0,   17,    0,   21,    0,   82,
    0,   85,    0,    0,    0,    0,    0,   23,   27,    0,
   52,   52,    0,   89,   87,   88,    0,    0,    0,    0,
    0,   94,    0,    0,    0,    0,   32,    8,   30,    0,
   29,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  165,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  107,
  108,  110,  117,  118,  123,  124,  125,  122,  106,  109,
  111,  112,  113,  114,  115,  116,  119,  120,  121,  126,
  127,  227,    0,   24,  226,    0,   25,  204,    0,  203,
    0,    0,   34,    0,    0,    0,    0,    0,    0,   53,
   54,   55,   56,   57,   58,   59,   60,   61,   62,   63,
   64,   65,   66,   67,    0,    0,   92,   91,   90,   86,
   95,   97,    0,   96,    0,  225,  232,    0,  137,  138,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  212,  213,    0,  211,
    0,    0,  209,  210,    0,    0,    0,    0,    0,    0,
    0,  168,    0,  179,  184,  185,  170,  172,  175,  229,
  230,    0,    0,  181,    0,   99,  101,  214,  215,    0,
    0,    0,    0,   71,   72,    0,   69,  183,  182,   68,
    0,    0,    0,    0,  195,    0,  194,  139,  196,  192,
    0,  191,    0,  193,  202,    0,  201,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  104,    0,    0,    0,    0,    0,  161,
    0,    0,  164,    0,    0,  218,    0,  216,    0,  217,
  166,    0,    0,    0,  167,    0,    0,    0,  188,  233,
  234,    0,    0,   45,    0,    0,   47,    0,    0,    0,
   36,   35,    0,   12,    0,  235,    0,    0,    0,  198,
  197,    0,  162,  221,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  173,  176,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  222,  142,  143,  141,    0,  223,
    0,  163,    0,    0,    0,  220,  219,  187,    0,    0,
    0,    0,  189,    0,    0,   49,    0,    0,    0,   51,
    0,    0,    0,   73,   74,    0,   13,   14,   11,    0,
  128,    0,    0,  186,  224,    0,  169,  171,    0,  174,
    0,    0,    0,    0,    0,    0,    0,   75,   76,    0,
    0,  145,    0,  147,    0,  144,    0,  130,    0,    0,
    0,  178,  190,   44,    0,    0,   46,    0,    0,   37,
   70,  146,  148,    0,  140,    0,    0,    0,    0,    0,
    0,   41,    0,   40,  154,  153,  155,    0,    0,    0,
  131,  236,  207,  208,    0,   48,   43,   50,    0,    0,
  133,  134,    0,  135,  132,  180,  157,  156,    0,    0,
    0,  136,    0,    0,  151,  152,    0,  159,  160,  150,
};
short yydgoto[] = {                                       3,
   67,  170,  276,  139,  219,  250,  319,  388,  320,  462,
   34,  430,  406,  409,  256,  242,  178,  332,   13,   25,
  414,  232,   21,  136,  273,  274,  133,  268,  269,  140,
    4,    5,  352,    0,  253,  379,    6,    7,    8,    9,
   28,   40,   45,   57,   78,   29,   58,  134,  137,   59,
   60,   79,   80,  143,   61,   82,   62,  339,  402,  336,
  398,  150,  151,  152,  153,  154,  155,  156,  157,  158,
  159,  160,  161,  162,  163,  164,   41,   42,   51,   71,
   43,   72,  174,  175,  213,  119,  120,  121,  122,  123,
  124,  125,  126,  127,  128,  129,  130,  131,  233,  456,
  438,  471,  179,  306,  437,  455,  468,  469,  490,  495,
  288,  290,  291,  420,  392,  292,  234,  223,  224,
};
short yysindex[] = {                                   -173,
  -19,   53,    0, -298, -298,    0, -173,    0,    0, -198,
 -198,   81, -115, -115,    0,    0,  106,    0, -169,   94,
  -98,  -98, -237,  120,    0,  -92,  108, -165,  -98,    0,
    0,    0,    0,    0, -169,  134, -147,  125,    0,    0,
 -165,    0, -145,    0, -224,    0,  -69,    0, -215,    0,
 -229,    0,  128,  131,  135,  136, -102,    0,    0, -258,
    0,    0,  154,    0,    0,    0,  156,  139,  143,  147,
 -113,    0,  -50,  -49, -277, -277,    0,    0,    0,  -82,
    0, -186, -186, -156, -271,  -50,  -49,  171,  -48,  -48,
  -48,  -48,  155,  157,  158,    0,  159,  160,  161,  162,
  163,  164,  165,  167,  168,  169,  170,  172,  173,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  185,    0,    0,  188,    0,    0,  189,    0,
  190,  177,    0,  178,  179,  180,  181,  182,  183,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  198,  199,    0,    0,    0,    0,
    0,    0,  -44,    0,    0,    0,    0,  186,    0,    0,
  187,  191, -207,    5,    5,  206,    5,    5,   27,  206,
  206,  -35,  -35,  -35, -240,    5,    5,    5,  -50,  -49,
 -184, -184,  207, -233,    5,   -8,    5,    5, -198,   -7,
  -11,  209,  211, -232, -238, -274,    0,    0,  212,    0,
  166,  214,    0,    0,  215,    3,  216,  218,  219,  226,
   12,    0,  262,    0,    0,    0,    0,    0,    0,    0,
    0,  279,  280,    0,  281,    0,    0,    0,    0,  288,
  290,   79,  310,    0,    0,  311,    0,    0,    0,    0,
  322,  338,  126,  171,    0,  296,    0,    0,    0,    0,
  297,    0,  345,    0,    0,  346,    0,    5,  138,   92,
   93,  355, -184, -170,   86,  145,  361,  363,   96,  364,
  365,  366,    5, -181,  124,  -37,  -33,  386,  -34, -207,
 -184, -184,  390,    0,  184,  406,  194, -249,    5,    0,
  360,  410,    0,  197,  417,    0,  376,    0,  424,    0,
    0,  425,  220,  -35,    0,  -35,  -35,  -35,    0,    0,
    0,  428,  430,    0,  221,  432,    0,  223,  438,  192,
    0,    0,  439,    0,  440,    0,  399, -241,  402,    0,
    0,  450,    0,    0, -207,  452, -277,  241, -268,  242,
   -4,  458,  460,    0,    0, -198, -181,  461,  -18,  462,
   11,  463, -178, -228,    0,    0,    0,    0,  464,    0,
    5,    0,  251,  466,  418,    0,    0,    0,  472,  225,
 -198,  484,    0,  498,  291,    0, -215,  504,  293,    0,
  294,  509, -235,    0,    0,  510,    0,    0,    0,    4,
    0,  512,  282,    0,    0,  299,    0,    0,  243,    0,
 -198,  519,  517,   11,  521,  520, -198,    0,    0,  522,
 -235,    0,    5,    0,    5,    0,  523,    0,  307,  524,
  528,    0,    0,    0, -271,  540,    0,  347,  540,    0,
    0,    0,    0, -265,    0,  560,  525, -225,  349,  567,
  350,    0,  569,    0,    0,    0,    0,  577,  578, -119,
    0,    0,    0,    0,  582,    0,    0,    0, -243, -230,
    0,    0, -198,    0,    0,    0,    0,    0,  580,  583,
  583,    0, -230, -267,    0,    0,  583,    0,    0,    0,
};
short yyrindex[] = {                                    628,
    0,    0,    0, -131,  115,    0,  629,    0,    0,    0,
    0,    0, -118,  401,    0,    0,    0,    0,    0,    0,
 -146,  335,    0,  353,    0,    0,    0,    0,  383,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   56,    0,    0,    0,  391,    0,    0,    0,    0,    0,
  501,    0,    0,    0,    0,    0,  123,    0,    0,  404,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    2,    0,    0,    0,    0,    0,    0,    0,    0,   58,
    0,   24,  374,    0,    0,    0,    0,    0,  574,  574,
  574,  574,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  252,    0,    0,  308,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  459,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  547,    0,    0,    0,    0,    0,    0,
    0,  603,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    9,    0,    0,  642,
    0,    0,    0,    0,    0,  195,    0,    0,  195,    0,
    0,    0,    0,    0,    0,   77,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  153,
  153,    0,    0,    0,    0,    0,  153,    0,    0,    0,
};
short yygindex[] = {                                      0,
  237,  196,    0,  -72, -283, -188,  201,    0,    0,  208,
    0,  204,    0,    0,    0,    0,   46,  272,  635,  608,
    0, -182,  633,  435,    0,    0,  441,    0,    0,  -10,
    0,    0,    0,    0,  357,    0,  651,    0,    0,    0,
   83,  632,    0,    0,    0,    0,    0,  -73,  -71,  609,
    0,    0,    0,    0,    0,  613,    0,    0,  253,    0,
    0,    0,    0,    0,    0,  605,  612,  614,  616,  617,
    0,    0,  619,  622,  623,  625,    0,    0,    0,    0,
    0,    0,  416,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  753,    0,
    0,    0,  594,    0,    0,    0,    0,  222, -447, -373,
    0,    0,    0,    0,    0,    0,  -14,  -80,    0,
};
#define YYTABLESIZE 1188
short yytable[] = {                                      17,
   18,   83,  335,  141,  226,  252,  338,  176,  149,  237,
  238,  239,  171,  251,  487,  172,  343,  240,  135,   31,
   12,  397,  428,   38,  132,  254,   55,  487,  138,  498,
  407,  275,  491,  473,  167,  465,  346,  386,   10,  383,
  466,  168,  169,  173,  173,  497,  387,   68,   69,   70,
  401,   16,   53,   54,   16,   81,  350,   31,   56,   32,
   55,  499,  488,  467,  376,  241,  231,  270,   33,  271,
  429,  382,  255,  265,  266,  488,  129,  408,  474,   16,
   89,   90,   91,   92,  144,  467,  377,   16,    1,    2,
   64,  217,   56,   98,  315,  145,   65,   66,  218,   38,
   39,  165,  166,  146,   30,  106,  107,  108,  147,  148,
   11,   44,  344,  345,   15,  248,  109,  496,   19,   19,
  330,  249,   33,  500,  331,  246,  404,  405,  247,   15,
   16,  149,   15,   15,   15,  316,  181,  182,   19,  317,
  318,  362,   10,  363,  364,  365,   10,   10,   20,   23,
   24,   26,  158,   89,   90,   91,   92,   93,   94,   95,
   96,   97,   27,   35,   36,   37,   98,   99,  100,  101,
  102,  227,  103,  104,  105,  235,  236,   47,  106,  107,
  108,   48,   49,  176,   52,   73,  316,   63,   74,  109,
  317,  318,   75,   76,   42,   77,   86,   84,  260,   85,
   87,  481,  482,  483,   88,  277,  132,  135,  142,   16,
  173,  177,  183,  212,  184,  185,  186,  187,  188,  189,
  190,  191,  192,  341,  193,  194,  195,  196,  199,  197,
  198,  200,  201,  202,  203,  204,  205,  206,  207,  208,
  209,  210,  211,  214,  215,  226,  252,  262,  216,  257,
  261,   98,  263,  230,  264,  278,  279,  280,  281,  283,
  282,  284,  285,   83,   83,  286,   83,   83,  334,  287,
  149,  149,  337,  149,  149,  149,  149,  149,  149,  149,
  149,  149,  149,  149,  384,   38,   38,  396,  149,  149,
  149,  149,  149,  149,  149,  149,  149,  351,  149,  149,
  149,  149,  149,  149,  149,  289,  149,  100,   38,  220,
  220,  149,  221,  221,   38,  230,  400,   81,   81,   31,
   31,   38,  293,  294,  295,  149,  149,  432,  433,  434,
  435,  296,  220,  297,   19,  221,  298,  378,  129,  129,
   38,  129,  129,  129,  129,  129,  129,  129,  129,  129,
  129,  129,   18,  299,  300,  393,  129,  129,  129,  129,
  129,  129,  129,  129,  129,  301,  129,  129,  129,  129,
  129,  129,  129,   39,  129,   15,   15,   15,   15,  129,
  418,  302,   19,  303,   33,   33,  305,  307,  308,  309,
   26,   15,   15,  129,  129,  311,  312,  313,  314,   15,
   10,  321,  322,   28,  323,   15,  324,  326,  327,  328,
  443,  325,   15,   33,  158,  158,  449,  158,  158,  158,
  158,  158,  158,  158,  158,  158,  158,  158,  333,  340,
  346,   15,  158,  158,  158,  158,  158,  158,  158,  158,
  158,  347,  158,  158,  158,  158,  158,  158,  158,  348,
  158,  349,  354,  355,  356,  158,   42,   42,  102,  485,
  357,   42,   42,   42,   42,   42,  358,  359,  360,  158,
  158,  366,  492,  367,   42,  369,   42,  361,  368,   42,
  370,  371,  373,  374,   42,   42,   42,   42,   42,   42,
   42,  375,   42,  381,  380,  383,  372,   42,  385,  389,
  105,  390,  103,  391,  395,  399,  403,  410,  412,  413,
  415,   42,   42,   98,   98,  416,   98,   98,   98,   98,
   98,   98,   98,   98,   98,   98,   98,  419,   98,   98,
   98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
  417,  421,   98,   98,   98,   98,  205,  424,  422,   98,
  425,  426,  427,  431,   98,  439,  441,  440,  442,  444,
  445,  447,  450,  448,  457,  472,  454,  458,   98,  100,
  100,  459,  100,  100,  100,  100,  100,  100,  100,  100,
  100,  100,  100,  461,  100,  100,  100,  100,  100,  100,
  100,  100,  100,  100,  100,  100,   19,   19,  100,  100,
  100,  100,  177,  470,  463,  100,  475,  476,  477,  478,
  100,   19,   19,   18,   18,   18,   18,   18,   18,   19,
  479,  480,  486,  493,  100,   19,  494,    1,    3,   18,
   18,  231,   19,  423,  451,   39,   39,   18,  394,   14,
  460,  206,   46,   18,   19,   19,   22,   19,   19,  272,
   18,   19,   26,   26,  267,  342,  464,   15,   39,   19,
   19,   10,   10,   10,   39,   28,   28,   19,   81,   18,
  484,   39,   50,   19,   83,  110,  446,   10,   10,  304,
   19,   26,  111,  180,  112,   10,  113,  114,   26,  115,
   39,   10,  116,  117,   28,  118,    0,    0,   10,   19,
  489,   28,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   10,    0,    0,
  102,  102,    0,  102,  102,  102,  102,  102,  102,  102,
  102,  102,  102,  102,    0,  102,  102,  102,  102,  102,
  102,  102,  102,    0,  102,  102,  102,    0,    0,    0,
  102,  102,  102,    0,    0,    0,    0,    0,    0,    0,
    0,  102,  105,  105,    0,  105,  105,  105,  105,  105,
  105,  105,  105,  105,  105,  105,    0,    0,    0,    0,
  105,  105,  105,  105,  105,    0,  105,  105,  105,    0,
    0,    0,  105,  105,  105,    0,    0,    0,    0,    0,
    0,    0,    0,  105,    0,    0,    0,    0,  205,  205,
    0,  205,  205,  205,  205,  205,  205,  205,  205,  205,
  205,  205,    0,    0,    0,    0,  205,  205,  205,  205,
  205,    0,  205,  205,  205,    0,    0,    0,  205,  205,
  205,    0,    0,    0,    0,    0,    0,    0,    0,  205,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  177,  177,    0,  177,  177,  177,
  177,  177,  177,  177,  177,  177,  177,  177,    0,    0,
    0,    0,  177,  177,  177,  177,  177,    0,  177,  177,
  177,    0,    0,    0,  177,  177,  177,    0,    0,    0,
    0,    0,    0,  206,  206,  177,  206,  206,  206,  206,
  206,  206,  206,  206,  206,  206,  206,    0,    0,    0,
    0,  206,  206,  206,  206,  206,    0,  206,  206,  206,
    0,    0,    0,  206,  206,  206,  222,  225,    0,  228,
  229,    0,    0,    0,  206,    0,    0,    0,  243,  244,
  245,    0,    0,    0,    0,    0,    0,    0,    0,  258,
  259,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  310,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  329,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  353,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  411,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  436,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  452,    0,  453,
};
short yycheck[] = {                                      10,
   11,    0,   40,   76,   40,   40,   40,   88,    0,  192,
  193,  194,   86,  202,  258,   87,  300,  258,  257,  257,
  319,   40,  258,    0,  257,  259,  285,  258,  306,  297,
  259,  306,  480,  259,  306,  301,   41,  306,   58,   44,
  306,  313,  314,   40,   40,  493,  315,  277,  278,  279,
   40,  329,  277,  278,  329,    0,  306,    0,  317,  297,
  285,  329,  306,  329,  306,  306,   40,  306,  306,  308,
  306,  355,  306,  306,  307,  306,    0,  306,  304,  329,
  267,  268,  269,  270,  271,  329,  328,  329,  262,  263,
  306,  299,  317,  280,  283,  282,  312,  313,  306,  265,
  266,  258,  259,  290,   22,  292,  293,  294,  295,  296,
   58,   29,  301,  302,    0,  300,  303,  491,  265,  266,
  302,  306,    0,  497,  306,  199,  305,  306,  200,  261,
  329,  318,  264,  265,  266,  306,   91,   92,   58,  310,
  311,  324,  261,  326,  327,  328,  265,  266,  264,   44,
  320,   58,    0,  267,  268,  269,  270,  271,  272,  273,
  274,  275,  261,   44,  257,   58,  280,  281,  282,  283,
  284,  186,  286,  287,  288,  190,  191,   44,  292,  293,
  294,  329,   58,  264,  330,   58,  306,  257,   58,  303,
  310,  311,   58,   58,    0,  298,   58,   44,  209,   44,
   58,  321,  322,  323,   58,  216,  257,  257,  291,  329,
   40,  260,   58,  258,   58,   58,   58,   58,   58,   58,
   58,   58,   58,  258,   58,   58,   58,   58,   44,   58,
   58,   44,   44,   44,   58,   58,   58,   58,   58,   58,
   58,   44,   44,   58,   58,   40,   40,  259,   58,  258,
  258,    0,   44,  289,   44,   44,   91,   44,   44,   44,
  258,   44,   44,  262,  263,   40,  265,  266,  306,  258,
  262,  263,  306,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  357,  262,  263,  306,  280,  281,
  282,  283,  284,  285,  286,  287,  288,  308,  290,  291,
  292,  293,  294,  295,  296,   44,  298,    0,  285,  306,
  306,  303,  309,  309,  291,  289,  306,  262,  263,  262,
  263,  298,   44,   44,   44,  317,  318,  324,  325,  326,
  327,   44,  306,   44,    0,  309,  258,  348,  262,  263,
  317,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,    0,   44,   44,  366,  280,  281,  282,  283,
  284,  285,  286,  287,  288,   44,  290,  291,  292,  293,
  294,  295,  296,    0,  298,  261,  262,  263,  264,  303,
  391,   44,    0,  258,  262,  263,   91,   91,   44,   44,
    0,  277,  278,  317,  318,  258,  305,  305,   44,  285,
    0,  316,  258,    0,   44,  291,   44,   44,   44,   44,
  421,  316,  298,  291,  262,  263,  427,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  275,  305,   44,
   41,  317,  280,  281,  282,  283,  284,  285,  286,  287,
  288,  258,  290,  291,  292,  293,  294,  295,  296,   44,
  298,  258,   93,   44,  258,  303,  262,  263,    0,  470,
   44,  267,  268,  269,  270,  271,   91,   44,   44,  317,
  318,   44,  483,   44,  280,   44,  282,  258,  258,  285,
  258,   44,   44,   44,  290,  291,  292,  293,  294,  295,
  296,   93,  298,   44,   93,   44,  305,  303,  258,  258,
    0,   44,   44,   44,   44,   44,   44,   44,  258,   44,
   93,  317,  318,  262,  263,   44,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,   44,  277,  278,
  279,  280,  281,  282,  283,  284,  285,  286,  287,  288,
  316,   44,  291,  292,  293,  294,    0,   44,  258,  298,
  258,  258,   44,   44,  303,   44,  258,  276,  316,   41,
   44,   41,   41,   44,  258,   41,   44,   44,  317,  262,
  263,   44,  265,  266,  267,  268,  269,  270,  271,  272,
  273,  274,  275,   44,  277,  278,  279,  280,  281,  282,
  283,  284,  285,  286,  287,  288,  262,  263,  291,  292,
  293,  294,    0,   44,  258,  298,  258,   41,  259,   41,
  303,  277,  278,  261,  262,  263,  264,  265,  266,  285,
   44,   44,   41,   44,  317,  291,   44,    0,    0,  277,
  278,   58,  298,  397,  431,  262,  263,  285,  367,    5,
  445,    0,   35,  291,  262,  263,   14,  265,  266,  215,
  298,  317,  262,  263,  214,  299,  449,    7,  285,  277,
  278,  261,  262,  263,  291,  262,  263,  285,   60,  317,
  470,  298,   41,  291,   62,   71,  424,  277,  278,  264,
  298,  291,   71,   90,   71,  285,   71,   71,  298,   71,
  317,  291,   71,   71,  291,   71,   -1,   -1,  298,  317,
  479,  298,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  317,   -1,   -1,
  262,  263,   -1,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,   -1,  277,  278,  279,  280,  281,
  282,  283,  284,   -1,  286,  287,  288,   -1,   -1,   -1,
  292,  293,  294,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  303,  262,  263,   -1,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,   -1,   -1,   -1,   -1,
  280,  281,  282,  283,  284,   -1,  286,  287,  288,   -1,
   -1,   -1,  292,  293,  294,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  303,   -1,   -1,   -1,   -1,  262,  263,
   -1,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,   -1,   -1,   -1,   -1,  280,  281,  282,  283,
  284,   -1,  286,  287,  288,   -1,   -1,   -1,  292,  293,
  294,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  303,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  262,  263,   -1,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  275,   -1,   -1,
   -1,   -1,  280,  281,  282,  283,  284,   -1,  286,  287,
  288,   -1,   -1,   -1,  292,  293,  294,   -1,   -1,   -1,
   -1,   -1,   -1,  262,  263,  303,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,   -1,   -1,   -1,
   -1,  280,  281,  282,  283,  284,   -1,  286,  287,  288,
   -1,   -1,   -1,  292,  293,  294,  184,  185,   -1,  187,
  188,   -1,   -1,   -1,  303,   -1,   -1,   -1,  196,  197,
  198,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  207,
  208,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  278,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  293,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  309,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  381,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  410,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  433,   -1,  435,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 330
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,0,"','",0,0,0,0,0,0,0,0,0,0,0,0,0,"':'",0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'['",0,"']'",0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"CHAR",
"INTEGER","BOOLEAN","PERCENT","MESSAGE_ID","MAZE_ID","LEVEL_ID","LEV_INIT_ID",
"GEOMETRY_ID","NOMAP_ID","OBJECT_ID","COBJECT_ID","MONSTER_ID","TRAP_ID",
"DOOR_ID","DRAWBRIDGE_ID","MAZEWALK_ID","WALLIFY_ID","REGION_ID","FILLING",
"RANDOM_OBJECTS_ID","RANDOM_MONSTERS_ID","RANDOM_PLACES_ID","ALTAR_ID",
"LADDER_ID","STAIR_ID","NON_DIGGABLE_ID","NON_PASSWALL_ID","ROOM_ID",
"PORTAL_ID","TELEPRT_ID","BRANCH_ID","LEV","CHANCE_ID","CORRIDOR_ID","GOLD_ID",
"ENGRAVING_ID","FOUNTAIN_ID","POOL_ID","SINK_ID","NONE","RAND_CORRIDOR_ID",
"DOOR_STATE","LIGHT_STATE","CURSE_TYPE","ENGRAVING_TYPE","WALLSIGN_ID",
"ROOMSHAPE","DIRECTION","RANDOM_TYPE","O_REGISTER","M_REGISTER","P_REGISTER",
"A_REGISTER","ALIGNMENT","LEFT_OR_RIGHT","CENTER","TOP_OR_BOT","ALTAR_TYPE",
"UP_OR_DOWN","SUBROOM_ID","NAME_ID","FLAGS_ID","FLAG_TYPE","MON_ATTITUDE",
"MON_ALERTNESS","MON_APPEARANCE","CONTAINED","MPICKUP","MINVENT","BURIED",
"OBJ_RANK","STRING","MAP_ID",
};
char *yyrule[] = {
"$accept : file",
"file :",
"file : levels",
"levels : level",
"levels : level levels",
"level : maze_level",
"level : room_level",
"maze_level : maze_def flags lev_init messages regions",
"room_level : level_def flags lev_init messages rreg_init rooms corridors_def",
"level_def : LEVEL_ID ':' string",
"lev_init :",
"lev_init : LEV_INIT_ID ':' CHAR ',' CHAR ',' BOOLEAN ',' BOOLEAN ',' light_state ',' walled",
"lev_init : LEV_INIT_ID ':' CHAR ',' CHAR ',' INTEGER ',' INTEGER ',' light_state",
"walled : BOOLEAN",
"walled : RANDOM_TYPE",
"flags :",
"flags : FLAGS_ID ':' flag_list",
"flag_list : FLAG_TYPE ',' flag_list",
"flag_list : FLAG_TYPE",
"messages :",
"messages : message messages",
"message : MESSAGE_ID ':' STRING",
"rreg_init :",
"rreg_init : rreg_init init_rreg",
"init_rreg : RANDOM_OBJECTS_ID ':' object_list",
"init_rreg : RANDOM_MONSTERS_ID ':' monster_list",
"rooms :",
"rooms : roomlist",
"roomlist : aroom",
"roomlist : aroom roomlist",
"corridors_def : random_corridors",
"corridors_def : corridors",
"random_corridors : RAND_CORRIDOR_ID",
"corridors :",
"corridors : corridors corridor",
"corridor : CORRIDOR_ID ':' corr_spec ',' corr_spec",
"corridor : CORRIDOR_ID ':' corr_spec ',' INTEGER",
"corr_spec : '(' INTEGER ',' DIRECTION ',' door_pos ')'",
"aroom : room_def room_details",
"aroom : subroom_def room_details",
"subroom_def : SUBROOM_ID ':' room_type ',' light_state ',' subroom_pos ',' room_size ',' string roomfill",
"room_def : ROOM_ID ':' room_type ',' light_state ',' room_pos ',' room_align ',' room_size roomfill",
"roomfill :",
"roomfill : ',' BOOLEAN",
"room_pos : '(' INTEGER ',' INTEGER ')'",
"room_pos : RANDOM_TYPE",
"subroom_pos : '(' INTEGER ',' INTEGER ')'",
"subroom_pos : RANDOM_TYPE",
"room_align : '(' h_justif ',' v_justif ')'",
"room_align : RANDOM_TYPE",
"room_size : '(' INTEGER ',' INTEGER ')'",
"room_size : RANDOM_TYPE",
"room_details :",
"room_details : room_details room_detail",
"room_detail : room_name",
"room_detail : room_chance",
"room_detail : room_door",
"room_detail : monster_detail",
"room_detail : object_detail",
"room_detail : trap_detail",
"room_detail : altar_detail",
"room_detail : fountain_detail",
"room_detail : sink_detail",
"room_detail : pool_detail",
"room_detail : gold_detail",
"room_detail : engraving_detail",
"room_detail : wallsign_detail",
"room_detail : stair_detail",
"room_name : NAME_ID ':' string",
"room_chance : CHANCE_ID ':' INTEGER",
"room_door : DOOR_ID ':' secret ',' door_state ',' door_wall ',' door_pos",
"secret : BOOLEAN",
"secret : RANDOM_TYPE",
"door_wall : DIRECTION",
"door_wall : RANDOM_TYPE",
"door_pos : INTEGER",
"door_pos : RANDOM_TYPE",
"maze_def : MAZE_ID ':' string ',' filling",
"filling : CHAR",
"filling : RANDOM_TYPE",
"filling : NONE",
"regions : aregion",
"regions : aregion regions",
"aregion : map_definition reg_init map_details",
"map_definition : NOMAP_ID",
"map_definition : map_geometry MAP_ID",
"map_geometry : GEOMETRY_ID ':' h_justif ',' v_justif",
"h_justif : LEFT_OR_RIGHT",
"h_justif : CENTER",
"h_justif : RANDOM_TYPE",
"v_justif : TOP_OR_BOT",
"v_justif : CENTER",
"v_justif : RANDOM_TYPE",
"reg_init :",
"reg_init : reg_init init_reg",
"init_reg : RANDOM_OBJECTS_ID ':' object_list",
"init_reg : RANDOM_PLACES_ID ':' place_list",
"init_reg : RANDOM_MONSTERS_ID ':' monster_list",
"object_list : object",
"object_list : object ',' object_list",
"monster_list : monster",
"monster_list : monster ',' monster_list",
"place_list : place",
"$$1 :",
"place_list : place $$1 ',' place_list",
"map_details :",
"map_details : map_details map_detail",
"map_detail : monster_detail",
"map_detail : object_detail",
"map_detail : door_detail",
"map_detail : trap_detail",
"map_detail : drawbridge_detail",
"map_detail : region_detail",
"map_detail : stair_region",
"map_detail : portal_region",
"map_detail : teleprt_region",
"map_detail : branch_region",
"map_detail : altar_detail",
"map_detail : fountain_detail",
"map_detail : mazewalk_detail",
"map_detail : wallify_detail",
"map_detail : ladder_detail",
"map_detail : stair_detail",
"map_detail : gold_detail",
"map_detail : engraving_detail",
"map_detail : wallsign_detail",
"map_detail : diggable_detail",
"map_detail : passwall_detail",
"$$2 :",
"monster_detail : MONSTER_ID chance ':' monster_c ',' m_name ',' coordinate $$2 monster_infos",
"monster_infos :",
"monster_infos : monster_infos monster_info",
"monster_info : ',' string",
"monster_info : ',' MON_ATTITUDE",
"monster_info : ',' MON_ALERTNESS",
"monster_info : ',' alignment",
"monster_info : ',' MON_APPEARANCE string",
"object_detail : OBJECT_ID object_desc",
"object_detail : COBJECT_ID object_desc",
"$$3 :",
"object_desc : chance ':' object_c $$3 ',' object_id ',' object_where object_infos",
"object_id : string",
"object_id : RANDOM_TYPE",
"object_id : OBJ_RANK",
"object_where : coordinate",
"object_where : CONTAINED",
"object_where : MPICKUP coordinate",
"object_where : MINVENT",
"object_where : BURIED coordinate",
"object_infos :",
"object_infos : ',' curse_state ',' monster_id ',' enchantment optional_name",
"object_infos : ',' curse_state ',' enchantment optional_name",
"object_infos : ',' monster_id ',' enchantment optional_name",
"curse_state : RANDOM_TYPE",
"curse_state : CURSE_TYPE",
"monster_id : STRING",
"enchantment : RANDOM_TYPE",
"enchantment : INTEGER",
"optional_name :",
"optional_name : ',' NONE",
"optional_name : ',' STRING",
"door_detail : DOOR_ID ':' door_state ',' coordinate",
"trap_detail : TRAP_ID chance ':' trap_name ',' coordinate",
"drawbridge_detail : DRAWBRIDGE_ID ':' coordinate ',' DIRECTION ',' door_state",
"mazewalk_detail : MAZEWALK_ID ':' coordinate ',' DIRECTION",
"wallify_detail : WALLIFY_ID",
"ladder_detail : LADDER_ID ':' coordinate ',' UP_OR_DOWN",
"stair_detail : STAIR_ID ':' coordinate ',' UP_OR_DOWN",
"$$4 :",
"stair_region : STAIR_ID ':' lev_region $$4 ',' lev_region ',' UP_OR_DOWN",
"$$5 :",
"portal_region : PORTAL_ID ':' lev_region $$5 ',' lev_region ',' string",
"$$6 :",
"$$7 :",
"teleprt_region : TELEPRT_ID ':' lev_region $$6 ',' lev_region $$7 teleprt_detail",
"$$8 :",
"branch_region : BRANCH_ID ':' lev_region $$8 ',' lev_region",
"teleprt_detail :",
"teleprt_detail : ',' UP_OR_DOWN",
"lev_region : region",
"lev_region : LEV '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'",
"fountain_detail : FOUNTAIN_ID ':' coordinate",
"sink_detail : SINK_ID ':' coordinate",
"pool_detail : POOL_ID ':' coordinate",
"diggable_detail : NON_DIGGABLE_ID ':' region",
"passwall_detail : NON_PASSWALL_ID ':' region",
"region_detail : REGION_ID ':' region ',' light_state ',' room_type prefilled",
"altar_detail : ALTAR_ID ':' coordinate ',' alignment ',' altar_type",
"gold_detail : GOLD_ID ':' amount ',' coordinate",
"engraving_detail : ENGRAVING_ID ':' coordinate ',' engraving_type ',' string",
"wallsign_detail : WALLSIGN_ID ':' coordinate ',' DIRECTION ',' engraving_type ',' string",
"monster_c : monster",
"monster_c : RANDOM_TYPE",
"monster_c : m_register",
"object_c : object",
"object_c : RANDOM_TYPE",
"object_c : o_register",
"m_name : string",
"m_name : RANDOM_TYPE",
"o_name : string",
"o_name : RANDOM_TYPE",
"trap_name : string",
"trap_name : RANDOM_TYPE",
"room_type : string",
"room_type : RANDOM_TYPE",
"prefilled :",
"prefilled : ',' FILLING",
"prefilled : ',' FILLING ',' BOOLEAN",
"prefilled : ',' FILLING ',' ROOMSHAPE",
"coordinate : coord",
"coordinate : p_register",
"coordinate : RANDOM_TYPE",
"door_state : DOOR_STATE",
"door_state : RANDOM_TYPE",
"light_state : LIGHT_STATE",
"light_state : RANDOM_TYPE",
"alignment : ALIGNMENT",
"alignment : a_register",
"alignment : RANDOM_TYPE",
"altar_type : ALTAR_TYPE",
"altar_type : RANDOM_TYPE",
"p_register : P_REGISTER '[' INTEGER ']'",
"o_register : O_REGISTER '[' INTEGER ']'",
"m_register : M_REGISTER '[' INTEGER ']'",
"a_register : A_REGISTER '[' INTEGER ']'",
"place : coord",
"monster : CHAR",
"object : CHAR",
"string : STRING",
"amount : INTEGER",
"amount : RANDOM_TYPE",
"chance :",
"chance : PERCENT",
"engraving_type : ENGRAVING_TYPE",
"engraving_type : RANDOM_TYPE",
"coord : '(' INTEGER ',' INTEGER ')'",
"region : '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 1795 "lev_comp.y"

/*lev_comp.y*/
#line 999 "y.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 7:
#line 186 "lev_comp.y"
{
			unsigned i;

			if (fatal_error > 0) {
				(void) fprintf(stderr,
				"%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				maze.flags = yyvsp[-3].i;
				(void) memcpy((genericptr_t)&(maze.init_lev),
						(genericptr_t)&(init_lev),
						sizeof(lev_init));
				maze.numpart = npart;
				maze.parts = NewTab(mazepart, npart);
				for(i=0;i<npart;i++)
				    maze.parts[i] = tmppart[i];
				if (!write_level_file(yyvsp[-4].map, (splev *)0, &maze)) {
					yyerror("Can't write output file!!");
					exit(EXIT_FAILURE);
				}
				npart = 0;
			}
			Free(yyvsp[-4].map);
		  }
break;
case 8:
#line 213 "lev_comp.y"
{
			unsigned i;

			if (fatal_error > 0) {
			    (void) fprintf(stderr,
			      "%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				special_lev.flags = (long) yyvsp[-5].i;
				(void) memcpy(
					(genericptr_t)&(special_lev.init_lev),
					(genericptr_t)&(init_lev),
					sizeof(lev_init));
				special_lev.nroom = nrooms;
				special_lev.rooms = NewTab(room, nrooms);
				for(i=0; i<nrooms; i++)
				    special_lev.rooms[i] = tmproom[i];
				special_lev.ncorr = ncorridor;
				special_lev.corrs = NewTab(corridor, ncorridor);
				for(i=0; i<ncorridor; i++)
				    special_lev.corrs[i] = tmpcor[i];
				if (check_subrooms()) {
				    if (!write_level_file(yyvsp[-6].map, &special_lev,
							  (specialmaze *)0)) {
					yyerror("Can't write output file!!");
					exit(EXIT_FAILURE);
				    }
				}
				free_rooms(&special_lev);
				nrooms = 0;
				ncorridor = 0;
			}
			Free(yyvsp[-6].map);
		  }
break;
case 9:
#line 250 "lev_comp.y"
{
			if (index(yyvsp[0].map, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if ((int) strlen(yyvsp[0].map) > 8)
			    yyerror("Level names limited to 8 characters.");
			yyval.map = yyvsp[0].map;
			special_lev.nrmonst = special_lev.nrobjects = 0;
			n_mlist = n_olist = 0;
		  }
break;
case 10:
#line 262 "lev_comp.y"
{
			/* in case we're processing multiple files,
			   explicitly clear any stale settings */
			(void) memset((genericptr_t) &init_lev, 0,
					sizeof init_lev);
			init_lev.init_present = FALSE;
			yyval.i = 0;
		  }
break;
case 11:
#line 271 "lev_comp.y"
{
			init_lev.init_present = 1;
			init_lev.fg = what_map_char((char) yyvsp[-10].i);
			if (init_lev.fg == INVALID_TYPE)
			    yyerror("Invalid foreground type.");
			init_lev.bg = what_map_char((char) yyvsp[-8].i);
			if (init_lev.bg == INVALID_TYPE)
			    yyerror("Invalid background type.");
			init_lev.smoothed = yyvsp[-6].i;
			init_lev.joined = yyvsp[-4].i;
			if (init_lev.joined &&
			    init_lev.fg != CORR && !IS_FLOOR(init_lev.fg)/*init_lev.fg != ROOM*/)
			    yyerror("Invalid foreground type for joined map.");
			init_lev.lit = yyvsp[-2].i;
			init_lev.walled = yyvsp[0].i;
			yyval.i = 1;
		  }
break;
case 12:
#line 289 "lev_comp.y"
{
			init_lev.init_present = yyvsp[-4].i;
			init_lev.arg = yyvsp[-2].i;
			init_lev.fg = what_map_char((char) yyvsp[-8].i);
			if (init_lev.fg == INVALID_TYPE)
			    yyerror("Invalid foreground type.");
			init_lev.bg = what_map_char((char) yyvsp[-6].i);
			if (init_lev.bg == INVALID_TYPE)
			    yyerror("Invalid background type.");
			init_lev.lit = yyvsp[0].i;
			init_lev.smoothed = 0;
			init_lev.joined = 0;
			init_lev.walled = 0;
			yyval.i = 1;
		  }
break;
case 15:
#line 311 "lev_comp.y"
{
			yyval.i = 0;
		  }
break;
case 16:
#line 315 "lev_comp.y"
{
			yyval.i = lev_flags;
			lev_flags = 0;	/* clear for next user */
		  }
break;
case 17:
#line 322 "lev_comp.y"
{
			lev_flags |= yyvsp[-2].i;
		  }
break;
case 18:
#line 326 "lev_comp.y"
{
			lev_flags |= yyvsp[0].i;
		  }
break;
case 21:
#line 336 "lev_comp.y"
{
			int i, j;

			i = (int) strlen(yyvsp[0].map) + 1;
			j = (int) strlen(tmpmessage);
			if (i + j > 255) {
			   yyerror("Message string too long (>256 characters)");
			} else {
			    if (j) tmpmessage[j++] = '\n';
			    (void) strncpy(tmpmessage+j, yyvsp[0].map, i - 1);
			    tmpmessage[j + i - 1] = 0;
			}
			Free(yyvsp[0].map);
		  }
break;
case 24:
#line 357 "lev_comp.y"
{
			if(special_lev.nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    special_lev.nrobjects = n_olist;
			    special_lev.robjects = (char *) alloc(n_olist);
			    (void) memcpy((genericptr_t)special_lev.robjects,
					  (genericptr_t)olist, n_olist);
			}
		  }
break;
case 25:
#line 368 "lev_comp.y"
{
			if(special_lev.nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    special_lev.nrmonst = n_mlist;
			    special_lev.rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)special_lev.rmonst,
					  (genericptr_t)mlist, n_mlist);
			  }
		  }
break;
case 26:
#line 381 "lev_comp.y"
{
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = 0;
			tmproom[nrooms]->rlit = 0;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = 0;
			tmproom[nrooms]->y = 0;
			tmproom[nrooms]->w = 2;
			tmproom[nrooms]->h = 2;
			in_room = 1;
		  }
break;
case 32:
#line 407 "lev_comp.y"
{
			tmpcor[0] = New(corridor);
			tmpcor[0]->src.room = -1;
			ncorridor = 1;
		  }
break;
case 35:
#line 419 "lev_comp.y"
{
			tmpcor[ncorridor] = New(corridor);
			tmpcor[ncorridor]->src.room = yyvsp[-2].corpos.room;
			tmpcor[ncorridor]->src.wall = yyvsp[-2].corpos.wall;
			tmpcor[ncorridor]->src.door = yyvsp[-2].corpos.door;
			tmpcor[ncorridor]->dest.room = yyvsp[0].corpos.room;
			tmpcor[ncorridor]->dest.wall = yyvsp[0].corpos.wall;
			tmpcor[ncorridor]->dest.door = yyvsp[0].corpos.door;
			ncorridor++;
			if (ncorridor >= MAX_OF_TYPE) {
				yyerror("Too many corridors in level!");
				ncorridor--;
			}
		  }
break;
case 36:
#line 434 "lev_comp.y"
{
			tmpcor[ncorridor] = New(corridor);
			tmpcor[ncorridor]->src.room = yyvsp[-2].corpos.room;
			tmpcor[ncorridor]->src.wall = yyvsp[-2].corpos.wall;
			tmpcor[ncorridor]->src.door = yyvsp[-2].corpos.door;
			tmpcor[ncorridor]->dest.room = -1;
			tmpcor[ncorridor]->dest.wall = yyvsp[0].i;
			ncorridor++;
			if (ncorridor >= MAX_OF_TYPE) {
				yyerror("Too many corridors in level!");
				ncorridor--;
			}
		  }
break;
case 37:
#line 450 "lev_comp.y"
{
			if ((unsigned) yyvsp[-5].i >= nrooms)
			    yyerror("Wrong room number!");
			yyval.corpos.room = yyvsp[-5].i;
			yyval.corpos.wall = yyvsp[-3].i;
			yyval.corpos.door = yyvsp[-1].i;
		  }
break;
case 38:
#line 460 "lev_comp.y"
{
			store_room();
		  }
break;
case 39:
#line 464 "lev_comp.y"
{
			store_room();
		  }
break;
case 40:
#line 470 "lev_comp.y"
{
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->parent = yyvsp[-1].map;
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->rtype = yyvsp[-9].i;
			tmproom[nrooms]->rlit = yyvsp[-7].i;
			tmproom[nrooms]->filled = yyvsp[0].i;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  }
break;
case 41:
#line 488 "lev_comp.y"
{
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = yyvsp[-9].i;
			tmproom[nrooms]->rlit = yyvsp[-7].i;
			tmproom[nrooms]->filled = yyvsp[0].i;
			tmproom[nrooms]->xalign = current_align.x;
			tmproom[nrooms]->yalign = current_align.y;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  }
break;
case 42:
#line 506 "lev_comp.y"
{
			yyval.i = 1;
		  }
break;
case 43:
#line 510 "lev_comp.y"
{
			yyval.i = yyvsp[0].i;
		  }
break;
case 44:
#line 516 "lev_comp.y"
{
			if ( yyvsp[-3].i < 1 || yyvsp[-3].i > 5 ||
			    yyvsp[-1].i < 1 || yyvsp[-1].i > 5 ) {
			    yyerror("Room position should be between 1 & 5!");
			} else {
			    current_coord.x = yyvsp[-3].i;
			    current_coord.y = yyvsp[-1].i;
			}
		  }
break;
case 45:
#line 526 "lev_comp.y"
{
			current_coord.x = current_coord.y = ERR;
		  }
break;
case 46:
#line 532 "lev_comp.y"
{
			if ( yyvsp[-3].i < 0 || yyvsp[-1].i < 0) {
			    yyerror("Invalid subroom position !");
			} else {
			    current_coord.x = yyvsp[-3].i;
			    current_coord.y = yyvsp[-1].i;
			}
		  }
break;
case 47:
#line 541 "lev_comp.y"
{
			current_coord.x = current_coord.y = ERR;
		  }
break;
case 48:
#line 547 "lev_comp.y"
{
			current_align.x = yyvsp[-3].i;
			current_align.y = yyvsp[-1].i;
		  }
break;
case 49:
#line 552 "lev_comp.y"
{
			current_align.x = current_align.y = ERR;
		  }
break;
case 50:
#line 558 "lev_comp.y"
{
			current_size.width = yyvsp[-3].i;
			current_size.height = yyvsp[-1].i;
		  }
break;
case 51:
#line 563 "lev_comp.y"
{
			current_size.height = current_size.width = ERR;
		  }
break;
case 68:
#line 589 "lev_comp.y"
{
			if (tmproom[nrooms]->name)
			    yyerror("This room already has a name!");
			else
			    tmproom[nrooms]->name = yyvsp[0].map;
		  }
break;
case 69:
#line 598 "lev_comp.y"
{
			if (tmproom[nrooms]->chance)
			    yyerror("This room already assigned a chance!");
			else if (tmproom[nrooms]->rtype == OROOM)
			    yyerror("Only typed rooms can have a chance!");
			else if (yyvsp[0].i < 1 || yyvsp[0].i > 99)
			    yyerror("The chance is supposed to be percentile.");
			else
			    tmproom[nrooms]->chance = yyvsp[0].i;
		   }
break;
case 70:
#line 611 "lev_comp.y"
{
			/* ERR means random here */
			if (yyvsp[-2].i == ERR && yyvsp[0].i != ERR) {
		     yyerror("If the door wall is random, so must be its pos!");
			} else {
			    tmprdoor[ndoor] = New(room_door);
			    tmprdoor[ndoor]->secret = yyvsp[-6].i;
			    tmprdoor[ndoor]->mask = yyvsp[-4].i;
			    tmprdoor[ndoor]->wall = yyvsp[-2].i;
			    tmprdoor[ndoor]->pos = yyvsp[0].i;
			    ndoor++;
			    if (ndoor >= MAX_OF_TYPE) {
				    yyerror("Too many doors in room!");
				    ndoor--;
			    }
			}
		  }
break;
case 77:
#line 643 "lev_comp.y"
{
			maze.filling = (schar) yyvsp[0].i;
			if (index(yyvsp[-2].map, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if (index(yyvsp[-2].map, '?'))
			    add_serial_no(yyvsp[-2].map);
			if ((int) strlen(yyvsp[-2].map) > 8)
			    yyerror("Level names limited to 8 characters.");
			yyval.map = yyvsp[-2].map;
			in_room = 0;
			n_plist = n_mlist = n_olist = 0;
		  }
break;
case 78:
#line 658 "lev_comp.y"
{
			yyval.i = get_floor_type((char)yyvsp[0].i);
		  }
break;
case 79:
#line 662 "lev_comp.y"
{
			yyval.i = -1;
		  }
break;
case 80:
#line 666 "lev_comp.y"
{
			yyval.i = -2;
		  }
break;
case 83:
#line 676 "lev_comp.y"
{
			store_part();
		  }
break;
case 84:
#line 682 "lev_comp.y"
{
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = 1;
			tmppart[npart]->valign = 1;
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			tmppart[npart]->xsize = 1;
			tmppart[npart]->ysize = 1;
			tmppart[npart]->map = (char **) alloc(sizeof(char *));
			tmppart[npart]->map[0] = (char *) alloc(1);
			tmppart[npart]->map[0][0] = STONE;
			max_x_map = COLNO-1;
			max_y_map = ROWNO;
		  }
break;
case 85:
#line 698 "lev_comp.y"
{
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = (yyvsp[-1].i & 15);
			tmppart[npart]->valign = (yyvsp[-1].i >> 4);
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			scan_map(yyvsp[0].map);
			Free(yyvsp[0].map);
		  }
break;
case 86:
#line 711 "lev_comp.y"
{
			yyval.i =  ((yyvsp[-2].i == -1) ? 0 : yyvsp[-2].i) +
				(((yyvsp[0].i == -1) ? 0 : yyvsp[0].i) << 4);
		  }
break;
case 95:
#line 732 "lev_comp.y"
{
			if (tmppart[npart]->nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    tmppart[npart]->robjects = (char *)alloc(n_olist);
			    (void) memcpy((genericptr_t)tmppart[npart]->robjects,
					  (genericptr_t)olist, n_olist);
			    tmppart[npart]->nrobjects = n_olist;
			}
		  }
break;
case 96:
#line 743 "lev_comp.y"
{
			if (tmppart[npart]->nloc) {
			    yyerror("Location registers already initialized!");
			} else {
			    register int i;
			    tmppart[npart]->rloc_x = (char *) alloc(n_plist);
			    tmppart[npart]->rloc_y = (char *) alloc(n_plist);
			    for(i=0;i<n_plist;i++) {
				tmppart[npart]->rloc_x[i] = plist[i].x;
				tmppart[npart]->rloc_y[i] = plist[i].y;
			    }
			    tmppart[npart]->nloc = n_plist;
			}
		  }
break;
case 97:
#line 758 "lev_comp.y"
{
			if (tmppart[npart]->nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    tmppart[npart]->rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)tmppart[npart]->rmonst,
					  (genericptr_t)mlist, n_mlist);
			    tmppart[npart]->nrmonst = n_mlist;
			}
		  }
break;
case 98:
#line 771 "lev_comp.y"
{
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = yyvsp[0].i;
			else
			    yyerror("Object list too long!");
		  }
break;
case 99:
#line 778 "lev_comp.y"
{
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = yyvsp[-2].i;
			else
			    yyerror("Object list too long!");
		  }
break;
case 100:
#line 787 "lev_comp.y"
{
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = yyvsp[0].i;
			else
			    yyerror("Monster list too long!");
		  }
break;
case 101:
#line 794 "lev_comp.y"
{
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = yyvsp[-2].i;
			else
			    yyerror("Monster list too long!");
		  }
break;
case 102:
#line 803 "lev_comp.y"
{
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
break;
case 103:
#line 810 "lev_comp.y"
{
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
break;
case 128:
#line 847 "lev_comp.y"
{
			tmpmonst[nmons] = New(monster);
			tmpmonst[nmons]->x = current_coord.x;
			tmpmonst[nmons]->y = current_coord.y;
			tmpmonst[nmons]->class = yyvsp[-4].i;
			tmpmonst[nmons]->peaceful = -1; /* no override */
			tmpmonst[nmons]->asleep = -1;
			tmpmonst[nmons]->align = - MAX_REGISTERS - 2;
			tmpmonst[nmons]->name.str = 0;
			tmpmonst[nmons]->appear = 0;
			tmpmonst[nmons]->appear_as.str = 0;
			tmpmonst[nmons]->chance = yyvsp[-6].i;
			tmpmonst[nmons]->id = NON_PM;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Monster");
			if (yyvsp[-2].map) {
			    int token = get_monster_id(yyvsp[-2].map, (char) yyvsp[-4].i);
			    if (token == ERR)
				yywarning(
			      "Invalid monster name!  Making random monster.");
			    else
				tmpmonst[nmons]->id = token;
			    Free(yyvsp[-2].map);
			}
		  }
break;
case 129:
#line 874 "lev_comp.y"
{
			if (++nmons >= MAX_OF_TYPE) {
			    yyerror("Too many monsters in room or mazepart!");
			    nmons--;
			}
		  }
break;
case 132:
#line 887 "lev_comp.y"
{
			tmpmonst[nmons]->name.str = yyvsp[0].map;
		  }
break;
case 133:
#line 891 "lev_comp.y"
{
			tmpmonst[nmons]->peaceful = yyvsp[0].i;
		  }
break;
case 134:
#line 895 "lev_comp.y"
{
			tmpmonst[nmons]->asleep = yyvsp[0].i;
		  }
break;
case 135:
#line 899 "lev_comp.y"
{
			tmpmonst[nmons]->align = yyvsp[0].i;
		  }
break;
case 136:
#line 903 "lev_comp.y"
{
			tmpmonst[nmons]->appear = yyvsp[-1].i;
			tmpmonst[nmons]->appear_as.str = yyvsp[0].map;
		  }
break;
case 137:
#line 910 "lev_comp.y"
{
		  }
break;
case 138:
#line 913 "lev_comp.y"
{
			/* 1: is contents of preceeding object with 2 */
			/* 2: is a container */
			/* 0: neither */
			tmpobj[nobj-1]->containment = 2;
		  }
break;
case 139:
#line 922 "lev_comp.y"
{
			tmpobj[nobj] = New(object);
			tmpobj[nobj]->class = yyvsp[0].i;
			tmpobj[nobj]->corpsenm = NON_PM;
			tmpobj[nobj]->curse_state = -1;
			tmpobj[nobj]->name.str = 0;
			tmpobj[nobj]->chance = yyvsp[-2].i;
			tmpobj[nobj]->material = -127;
			tmpobj[nobj]->id = -1;
		  }
break;
case 140:
#line 933 "lev_comp.y"
{
			if (++nobj >= MAX_OF_TYPE) {
			    yyerror("Too many objects in room or mazepart!");
			    nobj--;
			}
		  }
break;
case 141:
#line 942 "lev_comp.y"
{
			if (yyvsp[0].map) {
			    int token = get_object_id(yyvsp[0].map, tmpobj[nobj]->class);
			    if (token == ERR)
				yywarning(
				"Illegal object name!  Making random object.");
			     else
				tmpobj[nobj]->id = token;
			    /* material patch */
			    token = get_material_id(yyvsp[0].map);
			    if (token == ERR)
				yywarning(
				"Illegal material name!  Making random object.");
			     else
				tmpobj[nobj]->material = token;
			    Free(yyvsp[0].map);
			}
		  }
break;
case 142:
#line 961 "lev_comp.y"
{
			tmpobj[nobj]->id = -1;
		  }
break;
case 143:
#line 965 "lev_comp.y"
{
			tmpobj[nobj]->id = -1 - yyvsp[0].i;
		  }
break;
case 144:
#line 971 "lev_comp.y"
{
			tmpobj[nobj]->containment = 0;
			tmpobj[nobj]->x = current_coord.x;
			tmpobj[nobj]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Object");
		  }
break;
case 145:
#line 980 "lev_comp.y"
{
			tmpobj[nobj]->containment = 1;
			/* random coordinate, will be overridden anyway */
			tmpobj[nobj]->x = -MAX_REGISTERS-1;
			tmpobj[nobj]->y = -MAX_REGISTERS-1;
		  }
break;
case 146:
#line 987 "lev_comp.y"
{
			tmpobj[nobj]->containment = 3; /* mon picks it up during making level */
			tmpobj[nobj]->x = current_coord.x;
			tmpobj[nobj]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Object");
		  }
break;
case 147:
#line 996 "lev_comp.y"
{
			/* added into the previous monster's inventory */
			tmpobj[nobj]->containment = -nmons;
			tmpobj[nobj]->x = -MAX_REGISTERS-1;
			tmpobj[nobj]->y = -MAX_REGISTERS-1;
		  }
break;
case 148:
#line 1003 "lev_comp.y"
{
			tmpobj[nobj]->containment = 4;
			tmpobj[nobj]->x = current_coord.x;
			tmpobj[nobj]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Object");
		  }
break;
case 149:
#line 1014 "lev_comp.y"
{
			tmpobj[nobj]->spe = -127;
	/* Note below: we're trying to make as many of these optional as
	 * possible.  We clearly can't make curse_state, enchantment, and
	 * monster_id _all_ optional, since ",random" would be ambiguous.
	 * We can't even just make enchantment mandatory, since if we do that
	 * alone, ",random" requires too much lookahead to parse.
	 */
		  }
break;
case 150:
#line 1024 "lev_comp.y"
{
		  }
break;
case 151:
#line 1027 "lev_comp.y"
{
		  }
break;
case 152:
#line 1030 "lev_comp.y"
{
		  }
break;
case 153:
#line 1035 "lev_comp.y"
{
			tmpobj[nobj]->curse_state = -1;
		  }
break;
case 154:
#line 1039 "lev_comp.y"
{
			tmpobj[nobj]->curse_state = yyvsp[0].i;
		  }
break;
case 155:
#line 1045 "lev_comp.y"
{
			int token = get_monster_id(yyvsp[0].map, (char)0);
			if (token == ERR)	/* "random" */
			    tmpobj[nobj]->corpsenm = NON_PM - 1;
			else
			    tmpobj[nobj]->corpsenm = token;
			Free(yyvsp[0].map);
		  }
break;
case 156:
#line 1056 "lev_comp.y"
{
			tmpobj[nobj]->spe = -127;
		  }
break;
case 157:
#line 1060 "lev_comp.y"
{
			tmpobj[nobj]->spe = yyvsp[0].i;
		  }
break;
case 159:
#line 1067 "lev_comp.y"
{
		  }
break;
case 160:
#line 1070 "lev_comp.y"
{
			tmpobj[nobj]->name.str = yyvsp[0].map;
		  }
break;
case 161:
#line 1076 "lev_comp.y"
{
			tmpdoor[ndoor] = New(door);
			tmpdoor[ndoor]->x = current_coord.x;
			tmpdoor[ndoor]->y = current_coord.y;
			tmpdoor[ndoor]->mask = yyvsp[-2].i;
			if(current_coord.x >= 0 && current_coord.y >= 0 &&
			   tmpmap[current_coord.y][current_coord.x] != DOOR &&
			   tmpmap[current_coord.y][current_coord.x] != SDOOR)
			    yyerror("Door decl doesn't match the map");
			ndoor++;
			if (ndoor >= MAX_OF_TYPE) {
				yyerror("Too many doors in mazepart!");
				ndoor--;
			}
		  }
break;
case 162:
#line 1094 "lev_comp.y"
{
			tmptrap[ntrap] = New(trap);
			tmptrap[ntrap]->x = current_coord.x;
			tmptrap[ntrap]->y = current_coord.y;
			tmptrap[ntrap]->type = yyvsp[-2].i;
			tmptrap[ntrap]->chance = yyvsp[-4].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Trap");
			if (++ntrap >= MAX_OF_TYPE) {
				yyerror("Too many traps in room or mazepart!");
				ntrap--;
			}
		  }
break;
case 163:
#line 1111 "lev_comp.y"
{
		        int x, y, dir;

			tmpdb[ndb] = New(drawbridge);
			x = tmpdb[ndb]->x = current_coord.x;
			y = tmpdb[ndb]->y = current_coord.y;
			/* convert dir from a DIRECTION to a DB_DIR */
			dir = yyvsp[-2].i;
			switch(dir) {
			case W_NORTH: dir = DB_NORTH; y--; break;
			case W_SOUTH: dir = DB_SOUTH; y++; break;
			case W_EAST:  dir = DB_EAST;  x++; break;
			case W_WEST:  dir = DB_WEST;  x--; break;
			default:
			    yyerror("Invalid drawbridge direction");
			    break;
			}
			tmpdb[ndb]->dir = dir;
			if (current_coord.x >= 0 && current_coord.y >= 0 &&
			    !IS_WALL(tmpmap[y][x])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Wall needed for drawbridge (%02d, %02d)",
				    current_coord.x, current_coord.y);
			    yyerror(ebuf);
			}

			if ( yyvsp[0].i == D_ISOPEN )
			    tmpdb[ndb]->db_open = 1;
			else if ( yyvsp[0].i == D_CLOSED )
			    tmpdb[ndb]->db_open = 0;
			else
			    yyerror("A drawbridge can only be open or closed!");
			ndb++;
			if (ndb >= MAX_OF_TYPE) {
				yyerror("Too many drawbridges in mazepart!");
				ndb--;
			}
		   }
break;
case 164:
#line 1153 "lev_comp.y"
{
			tmpwalk[nwalk] = New(walk);
			tmpwalk[nwalk]->x = current_coord.x;
			tmpwalk[nwalk]->y = current_coord.y;
			tmpwalk[nwalk]->dir = yyvsp[0].i;
			nwalk++;
			if (nwalk >= MAX_OF_TYPE) {
				yyerror("Too many mazewalks in mazepart!");
				nwalk--;
			}
		  }
break;
case 165:
#line 1167 "lev_comp.y"
{
			wallify_map();
		  }
break;
case 166:
#line 1173 "lev_comp.y"
{
			tmplad[nlad] = New(lad);
			tmplad[nlad]->x = current_coord.x;
			tmplad[nlad]->y = current_coord.y;
			tmplad[nlad]->up = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Ladder");
			nlad++;
			if (nlad >= MAX_OF_TYPE) {
				yyerror("Too many ladders in mazepart!");
				nlad--;
			}
		  }
break;
case 167:
#line 1190 "lev_comp.y"
{
			tmpstair[nstair] = New(stair);
			tmpstair[nstair]->x = current_coord.x;
			tmpstair[nstair]->y = current_coord.y;
			tmpstair[nstair]->up = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Stairway");
			nstair++;
			if (nstair >= MAX_OF_TYPE) {
				yyerror("Too many stairs in room or mazepart!");
				nstair--;
			}
		  }
break;
case 168:
#line 1207 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 169:
#line 1216 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[-2].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			if(yyvsp[0].i)
			    tmplreg[nlreg]->rtype = LR_UPSTAIR;
			else
			    tmplreg[nlreg]->rtype = LR_DOWNSTAIR;
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 170:
#line 1236 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 171:
#line 1245 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[-2].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_PORTAL;
			tmplreg[nlreg]->rname.str = yyvsp[0].map;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 172:
#line 1262 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 173:
#line 1271 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[0].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
		  }
break;
case 174:
#line 1279 "lev_comp.y"
{
			switch(yyvsp[0].i) {
			case -1: tmplreg[nlreg]->rtype = LR_TELE; break;
			case 0: tmplreg[nlreg]->rtype = LR_DOWNTELE; break;
			case 1: tmplreg[nlreg]->rtype = LR_UPTELE; break;
			}
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 175:
#line 1295 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 176:
#line 1304 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[0].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_BRANCH;
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 177:
#line 1321 "lev_comp.y"
{
			yyval.i = -1;
		  }
break;
case 178:
#line 1325 "lev_comp.y"
{
			yyval.i = yyvsp[0].i;
		  }
break;
case 179:
#line 1331 "lev_comp.y"
{
			yyval.i = 0;
		  }
break;
case 180:
#line 1335 "lev_comp.y"
{
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if (yyvsp[-7].i <= 0 || yyvsp[-7].i >= COLNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-5].i < 0 || yyvsp[-5].i >= ROWNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-3].i <= 0 || yyvsp[-3].i >= COLNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-1].i < 0 || yyvsp[-1].i >= ROWNO)
				yyerror("Region out of level range!");
			current_region.x1 = yyvsp[-7].i;
			current_region.y1 = yyvsp[-5].i;
			current_region.x2 = yyvsp[-3].i;
			current_region.y2 = yyvsp[-1].i;
			yyval.i = 1;
		  }
break;
case 181:
#line 1355 "lev_comp.y"
{
			tmpfountain[nfountain] = New(fountain);
			tmpfountain[nfountain]->x = current_coord.x;
			tmpfountain[nfountain]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Fountain");
			nfountain++;
			if (nfountain >= MAX_OF_TYPE) {
			    yyerror("Too many fountains in room or mazepart!");
			    nfountain--;
			}
		  }
break;
case 182:
#line 1371 "lev_comp.y"
{
			tmpsink[nsink] = New(sink);
			tmpsink[nsink]->x = current_coord.x;
			tmpsink[nsink]->y = current_coord.y;
			nsink++;
			if (nsink >= MAX_OF_TYPE) {
				yyerror("Too many sinks in room!");
				nsink--;
			}
		  }
break;
case 183:
#line 1384 "lev_comp.y"
{
			tmppool[npool] = New(pool);
			tmppool[npool]->x = current_coord.x;
			tmppool[npool]->y = current_coord.y;
			npool++;
			if (npool >= MAX_OF_TYPE) {
				yyerror("Too many pools in room!");
				npool--;
			}
		  }
break;
case 184:
#line 1397 "lev_comp.y"
{
			tmpdig[ndig] = New(digpos);
			tmpdig[ndig]->x1 = current_region.x1;
			tmpdig[ndig]->y1 = current_region.y1;
			tmpdig[ndig]->x2 = current_region.x2;
			tmpdig[ndig]->y2 = current_region.y2;
			ndig++;
			if (ndig >= MAX_OF_TYPE) {
				yyerror("Too many diggables in mazepart!");
				ndig--;
			}
		  }
break;
case 185:
#line 1412 "lev_comp.y"
{
			tmppass[npass] = New(digpos);
			tmppass[npass]->x1 = current_region.x1;
			tmppass[npass]->y1 = current_region.y1;
			tmppass[npass]->x2 = current_region.x2;
			tmppass[npass]->y2 = current_region.y2;
			npass++;
			if (npass >= 32) {
				yyerror("Too many passwalls in mazepart!");
				npass--;
			}
		  }
break;
case 186:
#line 1427 "lev_comp.y"
{
			tmpreg[nreg] = New(region);
			tmpreg[nreg]->x1 = current_region.x1;
			tmpreg[nreg]->y1 = current_region.y1;
			tmpreg[nreg]->x2 = current_region.x2;
			tmpreg[nreg]->y2 = current_region.y2;
			tmpreg[nreg]->rlit = yyvsp[-3].i;
			tmpreg[nreg]->rtype = yyvsp[-1].i;
			if(yyvsp[0].i & 1) tmpreg[nreg]->rtype += MAXRTYPE+1;
			tmpreg[nreg]->rirreg = ((yyvsp[0].i >> 1/*& 2*/)/* != 0*/);
			if(current_region.x1 > current_region.x2 ||
			   current_region.y1 > current_region.y2)
			   yyerror("Region start > end!");
			if(tmpreg[nreg]->rtype == VAULT &&
			   (tmpreg[nreg]->rirreg ||
			    (tmpreg[nreg]->x2 - tmpreg[nreg]->x1 != 1) ||
			    (tmpreg[nreg]->y2 - tmpreg[nreg]->y1 != 1)))
				yyerror("Vaults must be exactly 2x2!");
			if(want_warnings && !tmpreg[nreg]->rirreg &&
			   current_region.x1 > 0 && current_region.y1 > 0 &&
			   current_region.x2 < (int)max_x_map &&
			   current_region.y2 < (int)max_y_map) {
			    /* check for walls in the room */
			    char ebuf[60];
			    register int x, y, nrock = 0;

			    for(y=current_region.y1; y<=current_region.y2; y++)
				for(x=current_region.x1;
				    x<=current_region.x2; x++)
				    if(IS_ROCK(tmpmap[y][x]) ||
				       IS_DOOR(tmpmap[y][x])) nrock++;
			    if(nrock) {
				Sprintf(ebuf,
					"Rock in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			    if (
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x2+1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x2+1])) {
				Sprintf(ebuf,
				"NonRock edge in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			} else if(tmpreg[nreg]->rirreg == 1 &&
		!IS_ROOM(tmpmap[current_region.y1][current_region.x1])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Rock in irregular room (%02d,%02d)?!",
				    current_region.x1, current_region.y1);
			    yyerror(ebuf);
			}
			nreg++;
			if (nreg >= MAX_OF_TYPE) {
				yyerror("Too many regions in mazepart!");
				nreg--;
			}
		  }
break;
case 187:
#line 1493 "lev_comp.y"
{
			tmpaltar[naltar] = New(altar);
			tmpaltar[naltar]->x = current_coord.x;
			tmpaltar[naltar]->y = current_coord.y;
			tmpaltar[naltar]->align = yyvsp[-2].i;
			tmpaltar[naltar]->shrine = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Altar");
			naltar++;
			if (naltar >= MAX_OF_TYPE) {
				yyerror("Too many altars in room or mazepart!");
				naltar--;
			}
		  }
break;
case 188:
#line 1511 "lev_comp.y"
{
			tmpgold[ngold] = New(gold);
			tmpgold[ngold]->x = current_coord.x;
			tmpgold[ngold]->y = current_coord.y;
			tmpgold[ngold]->amount = yyvsp[-2].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Gold");
			ngold++;
			if (ngold >= MAX_OF_TYPE) {
				yyerror("Too many golds in room or mazepart!");
				ngold--;
			}
		  }
break;
case 189:
#line 1528 "lev_comp.y"
{
			tmpengraving[nengraving] = New(engraving);
			tmpengraving[nengraving]->x = current_coord.x;
			tmpengraving[nengraving]->y = current_coord.y;
			tmpengraving[nengraving]->engr.str = yyvsp[0].map;
			tmpengraving[nengraving]->etype = yyvsp[-2].i;
			tmpengraving[nengraving]->onwall = 0;
			tmpengraving[nengraving]->dir = 0;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Engraving");
			nengraving++;
			if (nengraving >= MAX_OF_TYPE) {
			    yyerror("Too many engravings in room or mazepart!");
			    nengraving--;
			}
		  }
break;
case 190:
#line 1548 "lev_comp.y"
{
		  	int x, y, dir;

			tmpengraving[nengraving] = New(engraving);
			x = tmpengraving[nengraving]->x = current_coord.x;
			y = tmpengraving[nengraving]->y = current_coord.y;
			tmpengraving[nengraving]->engr.str = yyvsp[0].map;
			tmpengraving[nengraving]->etype = yyvsp[-2].i;
			tmpengraving[nengraving]->onwall = 1;
			if (!in_room) check_coord(x, y, "Wallsign");
			/* convert dir from a DIRECTION to a DB_DIR */
			dir = yyvsp[-4].i;
			switch(dir) {
			case W_NORTH: dir = SIGN_NORTH; y--; break;
			case W_SOUTH: dir = SIGN_SOUTH; y++; break;
			case W_EAST:  dir = SIGN_EAST;  x++; break;
			case W_WEST:  dir = SIGN_WEST;  x--; break;
			default:
			    yyerror("Invalid wallsign direction");
			    break;
			}
			tmpengraving[nengraving]->dir = dir;
			if (!in_room) check_coord(x, y, "Wallsign");
			nengraving++;
			if (nengraving >= MAX_OF_TYPE) {
			    yyerror("Too many engravings in room or mazepart!");
			    nengraving--;
			}
		  }
break;
case 192:
#line 1581 "lev_comp.y"
{
			yyval.i = - MAX_REGISTERS - 1;
		  }
break;
case 195:
#line 1589 "lev_comp.y"
{
			yyval.i = - MAX_REGISTERS - 1;
		  }
break;
case 198:
#line 1597 "lev_comp.y"
{
			yyval.map = (char *) 0;
		  }
break;
case 200:
#line 1604 "lev_comp.y"
{
			yyval.map = (char *) 0;
		  }
break;
case 201:
#line 1610 "lev_comp.y"
{
			int token = get_trap_type(yyvsp[0].map);
			if (token == ERR)
				yyerror("Unknown trap type!");
			yyval.i = token;
			Free(yyvsp[0].map);
		  }
break;
case 203:
#line 1621 "lev_comp.y"
{
			int token = get_room_type(yyvsp[0].map);
			if (token == ERR) {
				yywarning("Unknown room type!  Making ordinary room...");
				yyval.i = OROOM;
			} else
				yyval.i = token;
			Free(yyvsp[0].map);
		  }
break;
case 205:
#line 1634 "lev_comp.y"
{
			yyval.i = 0;
		  }
break;
case 206:
#line 1638 "lev_comp.y"
{
			yyval.i = yyvsp[0].i;
		  }
break;
case 207:
#line 1642 "lev_comp.y"
{
			yyval.i = yyvsp[-2].i + (yyvsp[0].i << 1);
		  }
break;
case 208:
#line 1646 "lev_comp.y"
{
			yyval.i = yyvsp[-2].i + (yyvsp[0].i << 1);
		  }
break;
case 211:
#line 1654 "lev_comp.y"
{
			current_coord.x = current_coord.y = -MAX_REGISTERS-1;
		  }
break;
case 218:
#line 1670 "lev_comp.y"
{
			yyval.i = - MAX_REGISTERS - 1;
		  }
break;
case 221:
#line 1680 "lev_comp.y"
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				current_coord.x = current_coord.y = - yyvsp[-1].i - 1;
		  }
break;
case 222:
#line 1689 "lev_comp.y"
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  }
break;
case 223:
#line 1698 "lev_comp.y"
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  }
break;
case 224:
#line 1707 "lev_comp.y"
{
			if ( yyvsp[-1].i >= 3 )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  }
break;
case 226:
#line 1719 "lev_comp.y"
{
			if (check_monster_char((char) yyvsp[0].i))
				yyval.i = yyvsp[0].i ;
			else {
				yyerror("Unknown monster class!");
				yyval.i = ERR;
			}
		  }
break;
case 227:
#line 1730 "lev_comp.y"
{
			char c = yyvsp[0].i;
			if (check_object_char(c))
				yyval.i = c;
			else {
				yyerror("Unknown char class!");
				yyval.i = ERR;
			}
		  }
break;
case 231:
#line 1749 "lev_comp.y"
{
			yyval.i = 100;	/* default is 100% */
		  }
break;
case 232:
#line 1753 "lev_comp.y"
{
			if (yyvsp[0].i <= 0 || yyvsp[0].i > 100)
			    yyerror("Expected percentile chance.");
			yyval.i = yyvsp[0].i;
		  }
break;
case 235:
#line 1765 "lev_comp.y"
{
			if (!in_room && !init_lev.init_present &&
			    (yyvsp[-3].i < 0 || yyvsp[-3].i > (int)max_x_map ||
			     yyvsp[-1].i < 0 || yyvsp[-1].i > (int)max_y_map))
			    yyerror("Coordinates out of map range!");
			current_coord.x = yyvsp[-3].i;
			current_coord.y = yyvsp[-1].i;
		  }
break;
case 236:
#line 1776 "lev_comp.y"
{
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if (yyvsp[-7].i < 0 || yyvsp[-7].i > (int)max_x_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-5].i < 0 || yyvsp[-5].i > (int)max_y_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-3].i < 0 || yyvsp[-3].i > (int)max_x_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-1].i < 0 || yyvsp[-1].i > (int)max_y_map)
				yyerror("Region out of map range!");
			current_region.x1 = yyvsp[-7].i;
			current_region.y1 = yyvsp[-5].i;
			current_region.x2 = yyvsp[-3].i;
			current_region.y2 = yyvsp[-1].i;
		  }
break;
#line 2703 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
