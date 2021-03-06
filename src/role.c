/*	SCCS Id: @(#)role.c	3.4	2003/01/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"


/*** Table of all roles ***/
/* According to AD&D, HD for some classes (ex. Wizard) should be smaller
 * (4-sided for wizards).  But this is not AD&D, and using the AD&D
 * rule here produces an unplayable character.  Thus I have used a minimum
 * of an 10-sided hit die for everything.  Another AD&D change: wizards get
 * a minimum strength of 4 since without one you can't teleport or cast
 * spells. --KAA
 *
 * As the wizard has been updated (wizard patch 5 jun '96) their HD can be
 * brought closer into line with AD&D. This forces wizards to use magic more
 * and distance themselves from their attackers. --LSZ
 *
 * With the introduction of races, some hit points and energy
 * has been reallocated for each race.  The values assigned
 * to the roles has been reduced by the amount allocated to
 * humans.  --KMH
 *
 * God names use a leading underscore to flag goddesses.
 */
const struct Role roles[] = {
{	{E_J("Archeologist",	"?l?Êw??"	), 0}, {
	{E_J("Digger",		"???@??"	), 0},
	{E_J("Field Worker",	"???n??????"	), 0},
	{E_J("Investigator",	"??????"	), 0},
	{E_J("Exhumer",		"???撲????"	), 0},
	{E_J("Excavator",	"???Ք??@??"	), 0},
	{E_J("Spelunker",	"?T????"	), 0},
	{E_J("Speleologist",	"???A?w??"	), 0},
	{E_J("Collector",	"???W??"	), 0},
	{E_J("Curator",		"?????ْ?"	), 0} },
#ifndef JP
	"Quetzalcoatl", "Camaxtli", "Huhetotl", /* Central American */
	"Arc", "the College of Archeology", "the Tomb of the Toltec Kings",
#else
	"?P?c?@???R?A?g??", "?J?}?L?V?g??", "?t?t?F?e?H?g??",
	"Arc", "?l?Êw???w", "?g???e?J???Ƃ̕???",
#endif /*JP*/
	PM_ARCHEOLOGIST, NON_PM, NON_PM,
	PM_LORD_CARNARVON, PM_STUDENT, PM_MINION_OF_HUHETOTL,
	NON_PM, PM_HUMAN_MUMMY, S_SNAKE, S_MUMMY,
	ART_ORB_OF_DETECTION,
	MH_HUMAN|MH_DWARF|MH_GNOME | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   6, 14, 10, 10,  6, 14 },	/* {  7, 10, 10,  7,  7,  7 } */
	{  10, 25, 10, 20, 10, 25 },	/* { 20, 20, 20, 10, 20, 10 } */
	{  16, 19, 18, 19, 17, 19 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 11, 0,  0, 4,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	10, 5, 0, 2, 10, A_INT, SPE_MAGIC_MAPPING,   -4
},
{	{E_J("Barbarian",	"???ؐl"	), 0}, {
	{E_J("Plunderer",	"???l"		), E_J("Plunderess", 0) },
	{E_J("Pillager",	"???D??????"	), 0},
	{E_J("Bandit",		"?쓐"		), 0},
	{E_J("Brigand",		"????????"	), 0},
	{E_J("Raider",		"?P????????"	), 0},
	{E_J("Reaver",		"???D??????"	), 0},
	{E_J("Slayer",		"?E?C??????"	), 0},
	{E_J("Chieftain",	"????"		), E_J("Chieftainess","??????")},
	{E_J("Conqueror",	"??????"	), E_J("Conqueress",0)} },
#ifndef JP
	"Mitra", "Crom", "Set", /* Hyborian */
	"Bar", "the Camp of the Duali Tribe", "the Duali Oasis",
#else
	"?~?g??", "?N????", "?Z?g", /* Hyborian */
	"Bar", "?f???A?????̃L?????v", "?f???A???E?I?A?V?X",
#endif /*JP*/
	PM_BARBARIAN, NON_PM, NON_PM,
	PM_PELIAS, PM_CHIEFTAIN, PM_THOTH_AMON,
	PM_OGRE, PM_TROLL, S_OGRE, S_TROLL,
	ART_HEART_OF_AHRIMAN,
	MH_HUMAN|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  16,  4,  4, 16, 16,  4 },	/* {  16,  7,  7, 15, 16,  6 } */
	{  30,  6,  7, 20, 30,  7 },
	{ 118, 12, 16, 18, 20, 10 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0,10,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	10, 14, 0, 0,  8, A_INT, SPE_HASTE_SELF,      -4
},
{	{E_J("Caveman",		"???A?l"	), E_J("Cavewoman",0)}, {
	{E_J("Troglodyte",	"?????l"	), 0},
	{E_J("Aborigine",	"???Z??"	), 0},
	{E_J("Wanderer",	"???Q??"	), 0},
	{E_J("Vagrant",		"???Q??"	), 0},
	{E_J("Wayfarer",	"???l"		), 0},
	{E_J("Roamer",		"???Q?l"	), 0},
	{E_J("Nomad",		"?V?q??"	), 0},
	{E_J("Rover",		"???K??"	), 0},
	{E_J("Pioneer",		"??????"	), 0} },
#ifndef JP
	"Anu", "_Ishtar", "Anshar", /* Babylonian */
	"Cav", "the Caves of the Ancestors", "the Dragon's Lair",
#else
	"?A?k", "_?C?V???^??", "?A???V????", /* Babylonian */
	"Cav", "???c?̓??A", "???̐??ݏ?",
#endif /*JP*/
	PM_CAVEMAN, PM_CAVEWOMAN, PM_LITTLE_DOG,
	PM_SHAMAN_KARNOV, PM_NEANDERTHAL, PM_CHROMATIC_DRAGON,
	PM_BUGBEAR, PM_HILL_GIANT, S_HUMANOID, S_GIANT,
	ART_SCEPTRE_OF_MIGHT,
	MH_HUMAN|MH_DWARF|MH_GNOME | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  20,  6,  6,  8, 16,  4 },	/* {  10,  7,  7,  7,  8,  6 } */
	{  30,  6,  7, 20, 30,  7 },
	{ 118, 14, 16, 18, 20, 15 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 12, 0, 1,  8, A_INT, SPE_DIG,             -4
},
{	{E_J("Healer",		"??????"	), 0}, {
	{E_J("Rhizotomist",	"?򑐍̂?"	), 0}, /* rhizotomist ?????̂??߂ɐA???̍????W?߂??l */
	{E_J("Empiric",		"???w???K??"	), 0}, /* empiric ???_???????o???E???????d?񂶂??l */
	{E_J("Embalmer",	"???̐????t"	), 0},
	{E_J("Dresser",		"?O?ȏ???"	), 0}, /* dresser (Br)?O?ȏ????A???ьW */
	{E_J("Medicus ossium",	"?ڍ???"	), E_J("Medica ossium",0)}, /* medicus (Latin)????, ossium (Latin)?? */
	{E_J("Herbalist",	"?򑐊w??"	), 0},
	{E_J("Magister",	"???w???m"	), E_J("Magistra",0) }, /* magister (Latin)?搶?A?w?Z?? */
	{E_J("Physician",	"???t"		), 0}, /* physician ???Ȉ? */
	{E_J("Chirurgeon",	"?T??"		), 0} }, /* chirurgeon ?O?Ȉ? surgeon ?̌Ì? */
#ifndef JP
	"_Athena", "Hermes", "Poseidon", /* Greek */
	"Hea", "the Temple of Epidaurus", "the Temple of Coeus",
#else
	"_?A?e?i", "?w?????X", "?|?Z?C?h??", /* Greek */
	"Hea", "?G?s?_?E???X???@", "?R?C?I?X???@",
#endif /*JP*/
	PM_HEALER, NON_PM, NON_PM,
	PM_HIPPOCRATES, PM_ATTENDANT, PM_CYCLOPS,
	PM_GIANT_RAT, PM_SNAKE, S_RODENT, S_YETI,
	ART_STAFF_OF_AESCULAPIUS,
	MH_HUMAN|MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   4, 10, 14,  9,  9, 14 },	/* {  7,  7, 13,  7, 11, 16 } */
	{  10, 20, 20, 15, 20, 20 },	/* { 15, 20, 20, 15, 25,  5 } */
	{  12, 19, 19, 19, 19, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 11, 0,  0, 4,  1, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },20,	/* Energy */
	10, 3,-3, 2, 10, A_WIS, SPE_CURE_SICKNESS,   -4
},
{	{E_J("Knight",		"?R?m"		), 0}, {
	{E_J("Gallant",		"????"		), 0}, /* ???V???????E?E???ȎႢ?j?c?󂹂Ȃ??̂?page?????? */
	{E_J("Esquire",		"?]?m"		), 0}, /* ?R?m?̉??Ő??b???????? */
	{E_J("Bachelor",	"?R??"		), 0}, /* ???ʂ̋R?m ?????҂????R?m?̈ʂ𓾂????A?R?m?c?ɂ͖????? */
	{E_J("Sergeant",	"?R????"	), 0}, /* ?킩???Ȃ??̂œK???Ɂc */
	{E_J("Knight",		"?R?m"		), 0}, /* (Bachelor?̂Ƃ??ɏ??C???ꂽ?͂??Ȃ̂????c) */
	{E_J("Banneret",	"?R?m?c??"	), 0}, /* ?l?p???????????Ď????̋R?m?c?𗦂????n?ʂ̋R?m */
	{E_J("Chevalier",	"?d?R?m"	), E_J("Chevaliere",0)}, /* ?t?????X???̋R?m ?V???o???G */
	{E_J("Seignieur",	"???M?R?m"	), E_J("Dame",0)}, /* ?????̎? Sir,Dame?͋R?m?̏̍? */
	{E_J("Paladin",		"???R?m"	), 0} },
#ifndef JP
	"Lugh", "_Brigit", "Manannan Mac Lir", /* Celtic */
	"Kni", "Camelot Castle", "the Isle of Glass",
#else
	"???[?t", "_?u???W?b?g", "?}?i?i???E?}?N???[??", /* Celtic */
	"Kni", "?L???????b?g??", "?K???X?̓?",
#endif /*JP*/
	PM_KNIGHT, NON_PM, PM_PONY,
	PM_KING_ARTHUR, PM_PAGE, PM_IXOTH,
	PM_QUASIT, PM_OCHRE_JELLY, S_IMP, S_JELLY,
	ART_MAGIC_MIRROR_OF_MERLIN,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  16,  6,  8,  6, 10, 14 },	/* { 13,  7, 14,  8, 10, 17 } */
	{  25, 10, 15, 10, 20, 20 },	/* { 30, 15, 15, 10, 20, 10 } */
	{  68, 17, 19, 13, 18, 19 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },10,	/* Energy */
	10, 8,-2, 0,  9, A_WIS, SPE_TURN_UNDEAD,     -4
},
{	{E_J("Monk",		"?C?s?m"	), 0}, {
	{E_J("Candidate",	"?????@????"	), 0},
	{E_J("Novice",		"???S??"	), 0},
	{E_J("Initiate",	"??????"	), 0},
	{E_J("Student of Stones","?y?̏K????"	), 0},
	{E_J("Student of Waters","???̏K????"	), 0},
	{E_J("Student of Metals","???̏K????"	), 0},
	{E_J("Student of Winds","?؂̏K????"	), 0},
	{E_J("Student of Fire",	"?΂̏K????"	), 0},
	{E_J("Master",		"?Ƌ??F?`"	), 0} },
#ifndef JP
	"Shan Lai Ching", "Chih Sung-tzu", "Huan Ti", /* Chinese */
	"Mon", "the Monastery of Chan-Sune",
	  "the Monastery of the Earth-Lord",
#else
	"?Z??", "?ԏ??q", "????", /* Chinese */
	"Mon", "?`?????E?X?[?C???@", "?n???̏C???@",
#endif /*JP*/
	PM_MONK, NON_PM, NON_PM,
	PM_GRAND_MASTER, PM_ABBOT, PM_MASTER_KAEN,
	PM_EARTH_ELEMENTAL, PM_XORN, S_ELEMENTAL, S_XORN,
	ART_EYES_OF_THE_OVERWORLD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   6, 10, 16, 16, 14,  8 },	/* { 10,  7,  8,  8,  7,  7 } */
	{  10, 15, 25, 20, 20, 10 },	/* { 25, 10, 20, 20, 15, 10 } */
	{  10, 18, 20, 20, 20, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 12, 0,  0, 6,  1, 0 },	/* Hit points */
	{  2, 2,  0, 2,  0, 2 },10,	/* Energy */
	10, 8,-2, 2, 20, A_WIS, SPE_RESTORE_ABILITY, -4
},
/*	???[?}?E?J?g???b?N?????̊K?w?g?D
	???c > ???@?? > ???i?? > ?i?? > ?i?? > ????
 */
{	{E_J("Priest",		"?m??"		), E_J("Priestess","???m")}, {
	{E_J("Aspirant",	"?C???m"	), E_J(0,"?C????")},
	{E_J("Acolyte",		"????"		), 0}, /* ?i?Ղ̎??`?? */
	{E_J("Adept",		"????"		), 0},
	{E_J("Priest",		"?m??"		), E_J("Priestess","???m")},
	{E_J("Curate",		"????"		), 0},
	{E_J("Canon",		"????"		), E_J("Canoness","????")},
	{E_J("Lama",		"?i??"		), 0},
	{E_J("Patriarch",	"???i??"	), E_J("Matriarch",0)},
	{E_J("High Priest",	"???m??"	), E_J("High Priestess","?????c")} },
	0, 0, 0,	/* chosen randomly from among the other roles */
#ifndef JP
	"Pri", "the Great Temple", "the Temple of Nalzok",
#else
	"Pri", "?̑??Ȃ鎛?@", "?i???]?N?̎??@",
#endif /*JP*/
	PM_PRIEST, PM_PRIESTESS, NON_PM,
	PM_ARCH_PRIEST, PM_ACOLYTE, PM_NALZOK,
	PM_HUMAN_ZOMBIE, PM_WRAITH, S_ZOMBIE, S_WRAITH,
	ART_MITRE_OF_HOLINESS,
	MH_HUMAN|MH_ELF | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  10, 10, 16,  7, 10,  7 },	/* {  7,  7, 10,  7,  7,  7 } */
	{  15, 15, 25, 10, 25, 10 },	/* { 15, 10, 30, 15, 20, 10 } */
	{  18, 17, 20, 15, 18, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 12, 0,  0, 6,  1, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 2 },10,	/* Energy */
	0, 3,-2, 2, 10, A_WIS, SPE_REMOVE_CURSE,    -4
},
  /* Note:  Rogue precedes Ranger so that use of `-R' on the command line
     retains its traditional meaning. */
{	{E_J("Rogue",		"????"), 0}, {
	{E_J("Footpad",		"?ǂ?????"),     0},
	{E_J("Cutpurse",	"?X??"),    0},
	{E_J("Rogue",		"????"),       0},
	{E_J("Pilferer",	"?????D"),    0},
	{E_J("Robber",		"????"),      0},
	{E_J("Burglar",		"????"),     0},
	{E_J("Filcher",		"????????"),     0},
	{E_J("Magsman",		"???\?t"),     E_J("Magswoman", 0) },
	{E_J("Thief",		"???D?_"),       0} },
#ifndef JP
	"Issek", "Mog", "Kos", /* Nehwon */
	"Rog", "the Thieves' Guild Hall", "the Assassins' Guild Hall",
#else
	"?C?Z?N", "???O", "?R?X", /* Nehwon */
	"Rog", "?????M???h?̃A?W?g", "?ÎE?҃M???h?̃A?W?g",
#endif /*JP*/
	PM_ROGUE, NON_PM, NON_PM,
	PM_MASTER_OF_THIEVES, PM_THUG, PM_MASTER_ASSASSIN,
	PM_LEPRECHAUN, PM_GUARDIAN_NAGA, S_NYMPH, S_NAGA,
	ART_MASTER_KEY_OF_THIEVERY,
	MH_HUMAN|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  12, 12,  7, 16,  7,  6 },	/* {  7,  7,  7, 10,  7,  6 } */
	{  25, 20, 10, 25, 10, 10 },	/* { 20, 10, 10, 30, 20, 10 } */
	{  18, 17, 17, 20, 17, 16 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 10, 0,  0, 6,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 8, 0, 1,  9, A_INT, SPE_DETECT_TREASURE, -4
},
{	{E_J("Ranger","?????W???["), 0}, {
#if 0	/* OBSOLETE */
	{"Edhel",       "Elleth"},
	{"Edhel",       "Elleth"},      /* elf-maid */
	{"Ohtar",       "Ohtie"},       /* warrior */
	{"Kano",			/* commander (Q.) ['a] */
			"Kanie"},	/* educated guess, until further research- SAC */
	{"Arandur",			/* king's servant, minister (Q.) - guess */
			"Aranduriel"},	/* educated guess */
	{"Hir",         "Hiril"},       /* lord, lady (S.) ['ir] */
	{"Aredhel",     "Arwen"},       /* noble elf, maiden (S.) */
	{"Ernil",       "Elentariel"},  /* prince (S.), elf-maiden (Q.) */
	{"Elentar",     "Elentari"},	/* Star-king, -queen (Q.) */
	"Solonor Thelandira", "Aerdrie Faenya", "Lolth", /* Elven */
#endif
	{E_J("Tenderfoot",	"?V??"),    0},
	{E_J("Lookout",		"??????"),       0},
	{E_J("Trailblazer",	"?擱??"),   0},
	{E_J("Reconnoiterer",	"???@"), E_J("Reconnoiteress",0)},
	{E_J("Scout",		"?ˌ?"),         0},
	{E_J("Arbalester",	"?ˎ?"),    0},	/* One skilled at crossbows */
	{E_J("Archer",		"?|??"),        0},
	{E_J("Sharpshooter",	"???ˎ?"),  0},
	{E_J("Marksman",	"?_????"),      E_J("Markswoman",0)} },
#ifndef JP
	"Mercury", "_Venus", "Mars", /* Roman/planets */
	"Ran", "Orion's camp", "the cave of the wumpus",
#else
	"?}?[?L?????[", "_???B?[?i?X", "?}?[?Y",
	"Ran", "?I???I???̃L?????v", "?????p?X?̓??A",
#endif /*JP*/
	PM_RANGER, NON_PM, PM_LITTLE_DOG /* Orion & canis major */,
	PM_ORION, PM_HUNTER, PM_SCORPIUS,
	PM_FOREST_CENTAUR, PM_SCORPION, S_CENTAUR, S_SPIDER,
	ART_LONGBOW_OF_DIANA,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7, 12,  7, 16,  6, 12 },	/* { 13, 13, 13,  9, 13,  7 } */
	{  10, 20, 10, 30, 10, 20 },	/* { 30, 10, 10, 20, 20, 10 } */
	{  17, 19, 16, 21, 13, 19 },
	/* Init   Lower  Higher */
	{ 13, 0,  0, 6,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },12,	/* Energy */
	10, 9, 2, 1, 10, A_INT, SPE_INVISIBILITY,   -4
},
{	{E_J("Samurai",		"??"	), 0}, {
/*	{"Hatamoto",      0},*/  /* Banner Knight */
/*	{"Kuge",          0},*/  /* Noble of the Court */
/*	{"Ninja","Kunoichi"},*/  /* secret society */
	{E_J("Ronin",		"?Q?l"	), 0},  /* no allegiance */
	{E_J("Ashigaru",	"???y"	), 0},  /* footman */
	{E_J("Kumigashira",	"???g??"), 0},  /* leader of Ashigaru */
	{E_J("Bushou",		"????"	), 0},  /* general */
	{E_J("Ryoshu",		"?̎?"	), 0},  /* has a territory */
	{E_J("Joshu",		"????"	), 0},  /* heads a castle */
	{E_J("Kokushu",		"????"	), 0},  /* heads a province */
	{E_J("Daimyo",		"?喼"	), 0},  /* a samurai lord */
	{E_J("Shogun",		"???R"	), 0} },/* supreme commander, warlord */
#ifndef JP
	"_Amaterasu Omikami", "Raijin", "Susanowo", /* Japanese */
	"Sam", "the Castle of the Taro Clan", "the Shogun's Castle",
#else
	"_?V?Ƒ??_", "???_", "?{???V?j", /* Japanese */
	"Sam", "???Y?ꑰ?̏?", "???R?̏?",
#endif /*JP*/
	PM_SAMURAI, NON_PM, PM_LITTLE_DOG,
	PM_LORD_SATO, PM_ROSHI, PM_ASHIKAGA_TAKAUJI,
	PM_WOLF, PM_STALKER, S_DOG, S_ELEMENTAL,
	ART_TSURUGI_OF_MURAMASA,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  12,  5,  5, 16, 16,  6 },	/* { 10,  8,  7, 10, 17,  6 } */
	{  25, 10, 10, 20, 25, 10 },	/* { 30, 10,  8, 30, 14,  8 } */
	{  68, 15, 15, 20, 19, 15 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 13, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 10, 0, 0,  8, A_INT, SPE_CLAIRVOYANCE,    -4
},
{	{E_J("Tourist",		"?ό??q"	), 0}, {
	{E_J("Rambler",		"?U????"	), 0},
	{E_J("Sightseer",	"?ό??q"	), 0},
	{E_J("Excursionist",	"???V?q"	), 0},
	{E_J("Peregrinator",	"???K??"),E_J("Peregrinatrix",0)},
	{E_J("Traveler",	"???s??"	), 0},
	{E_J("Journeyer",	"???l"		), 0},
	{E_J("Voyager",		"?q?C??"	), 0},
	{E_J("Explorer",	"?T????"	), 0},
	{E_J("Adventurer",	"?`????"	), 0} },
#ifndef JP
	"Blind Io", "_The Lady", "Offler", /* Discworld */
	"Tou", "Ankh-Morpork", "the Thieves' Guild Hall",
#else
	"?Ӗڂ̃C?I", "_?s???_?t", "?I?t???[", /* Discworld */
	"Tou", "?A???N=?????|?[?N", "?????M???h?̃A?W?g",
#endif /*JP*/
	PM_TOURIST, NON_PM, NON_PM,
	PM_TWOFLOWER, PM_GUIDE, PM_MASTER_OF_THIEVES,
	PM_GIANT_SPIDER, PM_FOREST_CENTAUR, S_SPIDER, S_CENTAUR,
	ART_YENDORIAN_EXPRESS_CARD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   8, 12,  6,  6, 10, 15 },	/* {  7, 10,  6,  7,  7, 10 } */
	{  20, 20, 10, 10, 20, 20 },	/* { 15, 10, 10, 15, 30, 20 } */
	{  17, 19, 19, 15, 18, 19 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{  8, 0,  0, 4,  0, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	0, 5, 1, 2, 10, A_INT, SPE_CHARM_MONSTER,   -4
},
{	{E_J("Valkyrie",	"?????L???[??"	), 0}, {
	{E_J("Stripling",	"?V??"), 0},
	{E_J("Skirmisher",	"?ˌ???"), 0},
	{E_J("Fighter",		"???m"), 0},
	{E_J("Man-at-arms",	"?d????"),		E_J("Woman-at-arms",	"???̉???") },
	{E_J("Warrior",		"?퓬??"),		E_J(0,			"?퉳??") },
	{E_J("Swashbuckler",	"???m"),		E_J(0,			"????????") },
	{E_J("Hero",		"?p?Y"),		E_J("Heroine",		0)},
	{E_J("Champion",	"????"),		E_J(0,			"????") },
	{E_J("Lord",		"?M?l"),		E_J("Lady",		"?M?w?l")} },
#ifndef JP
	"Tyr", "Odin", "Loki", /* Norse */
	"Val", "the Shrine of Destiny", "the cave of Surtur",
#else
	"?e???[??", "?I?[?f?B??", "???L", /* Norse */
	"Val", "?^???̐???", "?X???g?̓??A",
#endif /*JP*/
	PM_VALKYRIE, NON_PM, NON_PM /*PM_WINTER_WOLF_CUB*/,
	PM_NORN, PM_WARRIOR, PM_LORD_SURTUR,
	PM_FIRE_ANT, PM_FIRE_GIANT, S_ANT, S_GIANT,
	ART_ORB_OF_FATE,
	MH_HUMAN|MH_DWARF | ROLE_FEMALE | ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  16,  6, 10,  6, 16,  6 },	/* { 10,  7,  7,  7, 10,  7 } */
	{  25, 10, 10, 20, 25, 10 },	/* { 30,  6,  7, 20, 30,  7 } */
	{ 118, 12, 17, 16, 20, 16 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 10,-2, 0,  9, A_WIS, SPE_CONE_OF_COLD,    -4
},
{	{E_J("Wizard",		"???@?g??"	), 0}, {
	{E_J("Evoker",		"???i?t"), 0},
	{E_J("Conjurer",	"???p?t"), 0},
	{E_J("Thaumaturge",	"???p?t"), 0},
	{E_J("Magician",	"???p?t"), 0},
	{E_J("Enchanter",	"?S?p?t"), E_J("Enchantress",0)},
	{E_J("Sorcerer",	"?????t"), E_J("Sorceress",0)},
	{E_J("Necromancer",	"?d?p?t"), 0},
	{E_J("Wizard",		"???@?g??"), 0},
	{E_J("Mage",		"?喂?p?t"), 0} },
#ifndef JP
	"Ptah", "Thoth", "Anhur", /* Egyptian */
	"Wiz", "the Lonely Tower", "the Tower of Darkness",
#else
	"?v?^?n", "?g?[?g", "?A???t??", /* Egyptian */
	"Wiz", "?B???ꂵ??", "?Í??̓?",
#endif /*JP*/
	PM_WIZARD, NON_PM, PM_KITTEN,
	PM_NEFERET_THE_GREEN, PM_APPRENTICE, PM_DARK_ONE,
	PM_VAMPIRE_BAT, PM_XORN, S_BAT, S_WRAITH,
	ART_EYE_OF_THE_AETHIOPICA,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   6, 16, 14, 12,  6,  6 },	/* {  7, 10,  7,  7,  7,  7 } */
	{  10, 25, 20, 25, 10, 10 },	/* { 10, 30, 10, 20, 20, 10 } */
	{  10, 20, 20, 19, 16, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 10, 0,  0, 2,  0, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 3 },12,	/* Energy */
	0, 1, 0, 3, 10, A_INT, SPE_MAGIC_MISSILE,   -4
},
/* Array terminator */
{{0, 0}}
};


/* The player's role, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Role urole =
{	{"Undefined", 0}, { {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	"L", "N", "C", "Xxx", "home", "locate",
	NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM,
	NON_PM, NON_PM, 0, 0, 0, 0,
	/* Str Int Wis Dex Con Cha */
	{   7,  7,  7,  7,  7,  7 },
	{  20, 15, 15, 20, 20, 10 },
	{ 118, 18, 18, 18, 18, 18 },	/* max */
	/* Init   Lower  Higher */
	{ 10, 0,  0, 8,  1, 0 },	/* Hit points */
	{  2, 0,  0, 2,  0, 3 },14,	/* Energy */
	0, 10, 0, 0,  4, A_INT, 0, -3
};



/* Table of all races */
const struct Race races[] = {
{
#ifndef JP
	"human", "human", "humanity", "Hum",
	{"man", "woman"},
#else
	"?l??", "?l?Ԃ?", "?l??", "Hum",
	{"?j", "??"},
#endif /*JP*/
	PM_HUMAN, NON_PM, PM_HUMAN_MUMMY, PM_HUMAN_ZOMBIE,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	MH_HUMAN, 0, MH_GNOME|MH_ORC,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   2,  0,  0,  0,  0,  0 },	/* STR18(100), 18, 18, 18, 18, 18 */
	/* Init   Lower  Higher */
	{  2, 0,  0, 2,  1, 0 },	/* Hit points */
	{  1, 0,  2, 0,  2, 0 }		/* Energy */
},{
#ifndef JP
	"elf", "elven", "elvenkind", "Elf",
	{0, 0},
#else
	"?G???t", "?G???t??", "?G???t??", "Elf",
	{0, 0},
#endif /*JP*/
	PM_ELF, NON_PM, PM_ELF_MUMMY, PM_ELF_ZOMBIE,
	MH_ELF | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ELF, MH_ELF, MH_ORC,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   0,  2,  2,  0, -2,  1 },	/* 18, 20, 20, 18, 16, 19 */
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  1, 0 },	/* Hit points */
	{  2, 0,  3, 0,  3, 0 }		/* Energy */
},{
#ifndef JP
	"dwarf", "dwarven", "dwarvenkind", "Dwa",
	{0, 0},
#else
	"?h???[?t", "?h???[?t??", "?h???[?t??", "Dwa",
	{0, 0},
#endif /*JP*/
	PM_DWARF, NON_PM, PM_DWARF_MUMMY, PM_DWARF_ZOMBIE,
	MH_DWARF | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	MH_DWARF, MH_DWARF|MH_GNOME, MH_ORC,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   2, -2, -2,  2,  2, -2 },	/* STR18(100), 16, 16, 20, 20, 16 */
	/* Init   Lower  Higher */
	{  4, 0,  0, 3,  2, 0 },	/* Hit points */
	{  0, 0,  0, 0,  0, 0 }		/* Energy */
},{
#ifndef JP
	"gnome", "gnomish", "gnomehood", "Gno",
	{0, 0},
#else
	"?m?[??", "?m?[????", "?m?[????", "Gno",
	{0, 0},
#endif /*JP*/
	PM_GNOME, NON_PM, PM_GNOME_MUMMY, PM_GNOME_ZOMBIE,
	MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	MH_GNOME, MH_DWARF|MH_GNOME, MH_HUMAN,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   1,  1,  0,  0,  0,  0 },	/* STR18(50),19, 18, 18, 18, 18 */
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  2, 0,  2, 0,  2, 0 }		/* Energy */
},{
#ifndef JP
	"orc", "orcish", "orcdom", "Orc",
	{0, 0},
#else
	"?I?[?N", "?I?[?N??", "?I?[?N??", "Orc",
	{0, 0},
#endif /*JP*/
	PM_ORC, NON_PM, PM_ORC_MUMMY, PM_ORC_ZOMBIE,
	MH_ORC | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ORC, 0, MH_HUMAN|MH_ELF|MH_DWARF,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   2, -2, -2,  0,  0, -3 },	/* STR18(100),16, 16, 18, 18, 15 */
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  1, 0,  1, 0,  1, 0 }		/* Energy */
},
/* Array terminator */
{ 0, 0, 0, 0 }};


/* The player's race, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Race urace =
{	"something", "undefined", "something", "Xxx",
	{0, 0},
	NON_PM, NON_PM, NON_PM, NON_PM,
	0, 0, 0, 0,
	/*    Str     Int Wis Dex Con Cha */
	{      3,      3,  3,  3,  3,  3 },
	{ STR18(100), 18, 18, 18, 18, 18 },
	/* Init   Lower  Higher */
	{  2, 0,  0, 2,  1, 0 },	/* Hit points */
	{  1, 0,  2, 0,  2, 0 }		/* Energy */
};


/* Table of all genders */
const struct Gender genders[] = {
#ifndef JP
	{"male",	"he",	"him",	"his",	"Mal",	ROLE_MALE},
	{"female",	"she",	"her",	"her",	"Fem",	ROLE_FEMALE},
	{"neuter",	"it",	"it",	"its",	"Ntr",	ROLE_NEUTER}
#else
	{"?j??",	"??",	"??",	"?ނ?",	"Mal",	ROLE_MALE},
	{"????",	"?ޏ?",	"?ޏ?","?ޏ???","Fem",	ROLE_FEMALE},
	{"????",	"????",	"????",	"????",	"Ntr",	ROLE_NEUTER}
#endif /*JP*/
};


/* Table of all alignments */
const struct Align aligns[] = {
#ifndef JP
	{"law",		"lawful",	"Law",	ROLE_LAWFUL,	A_LAWFUL},
	{"balance",	"neutral",	"Neu",	ROLE_NEUTRAL,	A_NEUTRAL},
	{"chaos",	"chaotic",	"Cha",	ROLE_CHAOTIC,	A_CHAOTIC},
	{"evil",	"unaligned",	"Una",	0,		A_NONE}
#else
	{"?@",		"????",		"Law",	ROLE_LAWFUL,	A_LAWFUL},
	{"?ύt",	"????",		"Neu",	ROLE_NEUTRAL,	A_NEUTRAL},
	{"????",	"????",		"Cha",	ROLE_CHAOTIC,	A_CHAOTIC},
	{"?׈?",	"???S",		"Una",	0,		A_NONE}
#endif /*JP*/
};

STATIC_DCL char * FDECL(promptsep, (char *, int));
STATIC_DCL int FDECL(role_gendercount, (int));
STATIC_DCL int FDECL(race_alignmentcount, (int));

/* used by str2XXX() */
static char NEARDATA randomstr[] = "random";


boolean
validrole(rolenum)
	int rolenum;
{
	return (rolenum >= 0 && rolenum < SIZE(roles)-1);
}


int
randrole()
{
	return (rn2(SIZE(roles)-1));
}


int
str2role(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; roles[i].name.m; i++) {
	    /* Does it match the male name? */
	    if (!strncmpi(str, roles[i].name.m, len))
		return i;
	    /* Or the female name? */
	    if (roles[i].name.f && !strncmpi(str, roles[i].name.f, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, roles[i].filecode))
		return i;
	}

	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean
validrace(rolenum, racenum)
	int rolenum, racenum;
{
	/* Assumes validrole */
	return (racenum >= 0 && racenum < SIZE(races)-1 &&
		(roles[rolenum].allow & races[racenum].allow & ROLE_RACEMASK));
}


int
randrace(rolenum)
	int rolenum;
{
	int i, n = 0;

	/* Count the number of valid races */
	for (i = 0; races[i].noun; i++)
	    if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK)
	    	n++;

	/* Pick a random race */
	/* Use a factor of 100 in case of bad random number generators */
	if (n) n = rn2(n*100)/100;
	for (i = 0; races[i].noun; i++)
	    if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role has no permitted races? */
	return (rn2(SIZE(races)-1));
}


int
str2race(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; races[i].noun; i++) {
	    /* Does it match the noun? */
	    if (!strncmpi(str, races[i].noun, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, races[i].filecode))
		return i;
	}

	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean
validgend(rolenum, racenum, gendnum)
	int rolenum, racenum, gendnum;
{
	/* Assumes validrole and validrace */
	return (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		(roles[rolenum].allow & races[racenum].allow &
		 genders[gendnum].allow & ROLE_GENDMASK));
}


int
randgend(rolenum, racenum)
	int rolenum, racenum;
{
	int i, n = 0;

	/* Count the number of valid genders */
	for (i = 0; i < ROLE_GENDERS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		genders[i].allow & ROLE_GENDMASK)
	    	n++;

	/* Pick a random gender */
	if (n) n = rn2(n);
	for (i = 0; i < ROLE_GENDERS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		genders[i].allow & ROLE_GENDMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role/race has no permitted genders? */
	return (rn2(ROLE_GENDERS));
}


int
str2gend(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_GENDERS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, genders[i].adj, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, genders[i].filecode))
		return i;
	}
	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean
validalign(rolenum, racenum, alignnum)
	int rolenum, racenum, alignnum;
{
	/* Assumes validrole and validrace */
	return (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		(roles[rolenum].allow & races[racenum].allow &
		 aligns[alignnum].allow & ROLE_ALIGNMASK));
}


int
randalign(rolenum, racenum)
	int rolenum, racenum;
{
	int i, n = 0;

	/* Count the number of valid alignments */
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK)
	    	n++;

	/* Pick a random alignment */
	if (n) n = rn2(n);
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role/race has no permitted alignments? */
	return (rn2(ROLE_ALIGNS));
}


int
str2align(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_ALIGNS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, aligns[i].adj, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, aligns[i].filecode))
		return i;
	}
	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}

/* is rolenum compatible with any racenum/gendnum/alignnum constraints? */
boolean
ok_role(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (rolenum >= 0 && rolenum < SIZE(roles)-1) {
	allow = roles[rolenum].allow;
	if (racenum >= 0 && racenum < SIZE(races)-1 &&
		!(allow & races[racenum].allow & ROLE_RACEMASK))
	    return FALSE;
	if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		!(allow & genders[gendnum].allow & ROLE_GENDMASK))
	    return FALSE;
	if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		!(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < SIZE(roles)-1; i++) {
	    allow = roles[i].allow;
	    if (racenum >= 0 && racenum < SIZE(races)-1 &&
		    !(allow & races[racenum].allow & ROLE_RACEMASK))
		continue;
	    if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		    !(allow & genders[gendnum].allow & ROLE_GENDMASK))
		continue;
	    if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		    !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random role subject to any racenum/gendnum/alignnum constraints */
/* If pickhow == PICK_RIGID a role is returned only if there is  */
/* a single possibility */
int
pick_role(racenum, gendnum, alignnum, pickhow)
int racenum, gendnum, alignnum, pickhow;
{
    int i;
    int roles_ok = 0;

    for (i = 0; i < SIZE(roles)-1; i++) {
	if (ok_role(i, racenum, gendnum, alignnum))
	    roles_ok++;
    }
    if (roles_ok == 0 || (roles_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    roles_ok = rn2(roles_ok);
    for (i = 0; i < SIZE(roles)-1; i++) {
	if (ok_role(i, racenum, gendnum, alignnum)) {
	    if (roles_ok == 0)
		return i;
	    else
		roles_ok--;
	}
    }
    return ROLE_NONE;
}

/* is racenum compatible with any rolenum/gendnum/alignnum constraints? */
boolean
ok_race(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (racenum >= 0 && racenum < SIZE(races)-1) {
	allow = races[racenum].allow;
	if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		!(allow & roles[rolenum].allow & ROLE_RACEMASK))
	    return FALSE;
	if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		!(allow & genders[gendnum].allow & ROLE_GENDMASK))
	    return FALSE;
	if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		!(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < SIZE(races)-1; i++) {
	    allow = races[i].allow;
	    if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		    !(allow & roles[rolenum].allow & ROLE_RACEMASK))
		continue;
	    if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		    !(allow & genders[gendnum].allow & ROLE_GENDMASK))
		continue;
	    if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		    !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random race subject to any rolenum/gendnum/alignnum constraints */
/* If pickhow == PICK_RIGID a race is returned only if there is  */
/* a single possibility */
int
pick_race(rolenum, gendnum, alignnum, pickhow)
int rolenum, gendnum, alignnum, pickhow;
{
    int i;
    int races_ok = 0;

    for (i = 0; i < SIZE(races)-1; i++) {
	if (ok_race(rolenum, i, gendnum, alignnum))
	    races_ok++;
    }
    if (races_ok == 0 || (races_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    races_ok = rn2(races_ok);
    for (i = 0; i < SIZE(races)-1; i++) {
	if (ok_race(rolenum, i, gendnum, alignnum)) {
	    if (races_ok == 0)
		return i;
	    else
		races_ok--;
	}
    }
    return ROLE_NONE;
}

/* is gendnum compatible with any rolenum/racenum/alignnum constraints? */
/* gender and alignment are not comparable (and also not constrainable) */
boolean
ok_gend(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (gendnum >= 0 && gendnum < ROLE_GENDERS) {
	allow = genders[gendnum].allow;
	if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		!(allow & roles[rolenum].allow & ROLE_GENDMASK))
	    return FALSE;
	if (racenum >= 0 && racenum < SIZE(races)-1 &&
		!(allow & races[racenum].allow & ROLE_GENDMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < ROLE_GENDERS; i++) {
	    allow = genders[i].allow;
	    if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		    !(allow & roles[rolenum].allow & ROLE_GENDMASK))
		continue;
	    if (racenum >= 0 && racenum < SIZE(races)-1 &&
		    !(allow & races[racenum].allow & ROLE_GENDMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random gender subject to any rolenum/racenum/alignnum constraints */
/* gender and alignment are not comparable (and also not constrainable) */
/* If pickhow == PICK_RIGID a gender is returned only if there is  */
/* a single possibility */
int
pick_gend(rolenum, racenum, alignnum, pickhow)
int rolenum, racenum, alignnum, pickhow;
{
    int i;
    int gends_ok = 0;

    for (i = 0; i < ROLE_GENDERS; i++) {
	if (ok_gend(rolenum, racenum, i, alignnum))
	    gends_ok++;
    }
    if (gends_ok == 0 || (gends_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    gends_ok = rn2(gends_ok);
    for (i = 0; i < ROLE_GENDERS; i++) {
	if (ok_gend(rolenum, racenum, i, alignnum)) {
	    if (gends_ok == 0)
		return i;
	    else
		gends_ok--;
	}
    }
    return ROLE_NONE;
}

/* is alignnum compatible with any rolenum/racenum/gendnum constraints? */
/* alignment and gender are not comparable (and also not constrainable) */
boolean
ok_align(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (alignnum >= 0 && alignnum < ROLE_ALIGNS) {
	allow = aligns[alignnum].allow;
	if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		!(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	if (racenum >= 0 && racenum < SIZE(races)-1 &&
		!(allow & races[racenum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < ROLE_ALIGNS; i++) {
	    allow = races[i].allow;
	    if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		    !(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
		continue;
	    if (racenum >= 0 && racenum < SIZE(races)-1 &&
		    !(allow & races[racenum].allow & ROLE_ALIGNMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random alignment subject to any rolenum/racenum/gendnum constraints */
/* alignment and gender are not comparable (and also not constrainable) */
/* If pickhow == PICK_RIGID an alignment is returned only if there is  */
/* a single possibility */
int
pick_align(rolenum, racenum, gendnum, pickhow)
int rolenum, racenum, gendnum, pickhow;
{
    int i;
    int aligns_ok = 0;

    for (i = 0; i < ROLE_ALIGNS; i++) {
	if (ok_align(rolenum, racenum, gendnum, i))
	    aligns_ok++;
    }
    if (aligns_ok == 0 || (aligns_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    aligns_ok = rn2(aligns_ok);
    for (i = 0; i < ROLE_ALIGNS; i++) {
	if (ok_align(rolenum, racenum, gendnum, i)) {
	    if (aligns_ok == 0)
		return i;
	    else
		aligns_ok--;
	}
    }
    return ROLE_NONE;
}

void
rigid_role_checks()
{
    /* Some roles are limited to a single race, alignment, or gender and
     * calling this routine prior to XXX_player_selection() will help
     * prevent an extraneous prompt that actually doesn't allow
     * you to choose anything further. Note the use of PICK_RIGID which
     * causes the pick_XX() routine to return a value only if there is one
     * single possible selection, otherwise it returns ROLE_NONE.
     *
     */
    if (flags.initrole == ROLE_RANDOM) {
	/* If the role was explicitly specified as ROLE_RANDOM
	 * via -uXXXX-@ then choose the role in here to narrow down
	 * later choices. Pick a random role in this case.
	 */
	flags.initrole = pick_role(flags.initrace, flags.initgend,
					flags.initalign, PICK_RANDOM);
	if (flags.initrole < 0)
	    flags.initrole = randrole();
    }
    if (flags.initrole != ROLE_NONE) {
	if (flags.initrace == ROLE_NONE)
	     flags.initrace = pick_race(flags.initrole, flags.initgend,
						flags.initalign, PICK_RIGID);
	if (flags.initalign == ROLE_NONE)
	     flags.initalign = pick_align(flags.initrole, flags.initrace,
						flags.initgend, PICK_RIGID);
	if (flags.initgend == ROLE_NONE)
	     flags.initgend = pick_gend(flags.initrole, flags.initrace,
						flags.initalign, PICK_RIGID);
    }
}

#define BP_ALIGN	0
#define BP_GEND		1
#define BP_RACE		2
#define BP_ROLE		3
#define NUM_BP		4

STATIC_VAR char pa[NUM_BP], post_attribs;

STATIC_OVL char *
promptsep(buf, num_post_attribs)
char *buf;
int num_post_attribs;
{
#ifndef JP
	const char *conj = "and ";
	if (num_post_attribs > 1
	    && post_attribs < num_post_attribs && post_attribs > 1)
	 	Strcat(buf, ","); 
	Strcat(buf, " ");
	--post_attribs;
	if (!post_attribs && num_post_attribs > 1) Strcat(buf, conj);
#else
	if (post_attribs-- < num_post_attribs) Strcat(buf, "?E");
#endif /*JP*/
	return buf;
}

STATIC_OVL int
role_gendercount(rolenum)
int rolenum;
{
	int gendcount = 0;
	if (validrole(rolenum)) {
		if (roles[rolenum].allow & ROLE_MALE) ++gendcount;
		if (roles[rolenum].allow & ROLE_FEMALE) ++gendcount;
		if (roles[rolenum].allow & ROLE_NEUTER) ++gendcount;
	}
	return gendcount;
}

STATIC_OVL int
race_alignmentcount(racenum)
int racenum;
{
	int aligncount = 0;
	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
		if (races[racenum].allow & ROLE_CHAOTIC) ++aligncount;
		if (races[racenum].allow & ROLE_LAWFUL) ++aligncount;
		if (races[racenum].allow & ROLE_NEUTRAL) ++aligncount;
	}
	return aligncount;
}

char *
root_plselection_prompt(suppliedbuf, buflen, rolenum, racenum, gendnum, alignnum)
char *suppliedbuf;
int buflen, rolenum, racenum, gendnum, alignnum;
{
	int k, gendercount = 0, aligncount = 0;
	char buf[BUFSZ];
	static char err_ret[] = E_J(" character's","?L?????N?^?[??");
	boolean donefirst = FALSE;

	if (!suppliedbuf || buflen < 1) return err_ret;

	/* initialize these static variables each time this is called */
	post_attribs = 0;
	for (k=0; k < NUM_BP; ++k)
		pa[k] = 0;
	buf[0] = '\0';
	*suppliedbuf = '\0';
	
	/* How many alignments are allowed for the desired race? */
	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM)
		aligncount = race_alignmentcount(racenum);

	if (alignnum != ROLE_NONE && alignnum != ROLE_RANDOM) {
		/* if race specified, and multiple choice of alignments for it */
		if ((racenum >= 0) && (aligncount > 1)) {
			if (donefirst) Strcat(buf, " ");
			Strcat(buf, aligns[alignnum].adj);
			donefirst = TRUE;
		} else {
			if (donefirst) Strcat(buf, " ");
			Strcat(buf, aligns[alignnum].adj);
			donefirst = TRUE;
		}
	} else {
		/* if alignment not specified, but race is specified
			and only one choice of alignment for that race then
			don't include it in the later list */
		if ((((racenum != ROLE_NONE && racenum != ROLE_RANDOM) &&
			ok_race(rolenum, racenum, gendnum, alignnum))
		      && (aligncount > 1))
		     || (racenum == ROLE_NONE || racenum == ROLE_RANDOM)) {
			pa[BP_ALIGN] = 1;
			post_attribs++;
		}
	}
	/* <your lawful> */

	/* How many genders are allowed for the desired role? */
	if (validrole(rolenum))
		gendercount = role_gendercount(rolenum);

	if (gendnum != ROLE_NONE  && gendnum != ROLE_RANDOM) {
		if (validrole(rolenum)) {
		     /* if role specified, and multiple choice of genders for it,
			and name of role itself does not distinguish gender */
			if ((rolenum != ROLE_NONE) && (gendercount > 1)
						&& !roles[rolenum].name.f) {
				if (donefirst) Strcat(buf, " ");
				Strcat(buf, genders[gendnum].adj);
				donefirst = TRUE;
			}
	        } else {
			if (donefirst) Strcat(buf, " ");
	        	Strcat(buf, genders[gendnum].adj);
			donefirst = TRUE;
	        }
	} else {
		/* if gender not specified, but role is specified
			and only one choice of gender then
			don't include it in the later list */
		if ((validrole(rolenum) && (gendercount > 1)) || !validrole(rolenum)) {
			pa[BP_GEND] = 1;
			post_attribs++;
		}
	}
	/* <your lawful female> */

	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
		if (validrole(rolenum) && ok_race(rolenum, racenum, gendnum, alignnum)) {
			if (donefirst) Strcat(buf, " "); 
			Strcat(buf, (rolenum == ROLE_NONE) ?
				races[racenum].noun :
				races[racenum].adj);
			donefirst = TRUE;
		} else if (!validrole(rolenum)) {
			if (donefirst) Strcat(buf, " ");
			Strcat(buf, races[racenum].noun);
			donefirst = TRUE;
		} else {
			pa[BP_RACE] = 1;
			post_attribs++;
		}
	} else {
		pa[BP_RACE] = 1;
		post_attribs++;
	}
	/* <your lawful female gnomish> || <your lawful female gnome> */

	if (validrole(rolenum)) {
		if (donefirst) Strcat(buf, " ");
		if (gendnum != ROLE_NONE) {
		    if (gendnum == 1  && roles[rolenum].name.f)
			Strcat(buf, roles[rolenum].name.f);
		    else
  			Strcat(buf, roles[rolenum].name.m);
		} else {
			if (roles[rolenum].name.f) {
				Strcat(buf, roles[rolenum].name.m);
				Strcat(buf, "/");
				Strcat(buf, roles[rolenum].name.f);
			} else 
				Strcat(buf, roles[rolenum].name.m);
		}
		donefirst = TRUE;
	} else if (rolenum == ROLE_NONE) {
		pa[BP_ROLE] = 1;
		post_attribs++;
	}
	
	if ((racenum == ROLE_NONE || racenum == ROLE_RANDOM) && !validrole(rolenum)) {
		if (donefirst) Strcat(buf, " ");
		Strcat(buf, E_J("character","?L?????N?^?["));
		donefirst = TRUE;
	}
	/* <your lawful female gnomish cavewoman> || <your lawful female gnome>
	 *    || <your lawful female character>
	 */
	if (buflen > (int) (strlen(buf) + 1)) {
		Strcpy(suppliedbuf, buf);
		return suppliedbuf;
	} else
		return err_ret;
}

char *
build_plselection_prompt(buf, buflen, rolenum, racenum, gendnum, alignnum)
char *buf;
int buflen, rolenum, racenum, gendnum, alignnum;
{
	const char *defprompt = "Shall I pick a character for you? [ynq] ";
	int num_post_attribs = 0;
	char tmpbuf[BUFSZ];
	
	if (buflen < QBUFSZ)
		return (char *)defprompt;

#ifndef JP
	Strcpy(tmpbuf, "Shall I pick ");
#else
	tmpbuf[0] = 0;
#endif /*JP*/
	if (racenum != ROLE_NONE || validrole(rolenum))
		Strcat(tmpbuf, E_J("your ","???Ȃ???"));
	else {
#ifndef JP
		Strcat(tmpbuf, "a ");
#endif /*JP*/
	}
	/* <your> */

	(void)  root_plselection_prompt(eos(tmpbuf), buflen - strlen(tmpbuf),
					rolenum, racenum, gendnum, alignnum);
	Sprintf(buf, "%s", s_suffix(tmpbuf));

	/* buf should now be:
	 * < your lawful female gnomish cavewoman's> || <your lawful female gnome's>
	 *    || <your lawful female character's>
	 *
         * Now append the post attributes to it
	 */

	num_post_attribs = post_attribs;
	if (post_attribs) {
		if (pa[BP_RACE]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("race","?푰"));
		}
		if (pa[BP_ROLE]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("role","?E??"));
		}
		if (pa[BP_GEND]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("gender","????"));
		}
		if (pa[BP_ALIGN]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("alignment","????"));
		}
	}
	Strcat(buf, E_J(" for you? [ynq] ","???K???Ɍ??߂܂????H [ynq] "));
	return buf;
}

#undef BP_ALIGN
#undef BP_GEND
#undef BP_RACE
#undef BP_ROLE
#undef NUM_BP

void
plnamesuffix()
{
	char *sptr, *eptr;
	int i;

	/* Look for tokens delimited by '-' */
	if ((eptr = index(plname, '-')) != (char *) 0)
	    *eptr++ = '\0';
	while (eptr) {
	    /* Isolate the next token */
	    sptr = eptr;
	    if ((eptr = index(sptr, '-')) != (char *)0)
		*eptr++ = '\0';

	    /* Try to match it to something */
	    if ((i = str2role(sptr)) != ROLE_NONE)
		flags.initrole = i;
	    else if ((i = str2race(sptr)) != ROLE_NONE)
		flags.initrace = i;
	    else if ((i = str2gend(sptr)) != ROLE_NONE)
		flags.initgend = i;
	    else if ((i = str2align(sptr)) != ROLE_NONE)
		flags.initalign = i;
	}
	if(!plname[0]) {
	    askname();
	    plnamesuffix();
	}

	/* commas in the plname confuse the record file, convert to spaces */
	for (sptr = plname; *sptr; sptr++) {
		if (*sptr == ',') *sptr = ' ';
	}
}


/*
 *	Special setup modifications here:
 *
 *	Unfortunately, this is going to have to be done
 *	on each newgame or restore, because you lose the permonst mods
 *	across a save/restore.  :-)
 *
 *	1 - The Rogue Leader is the Tourist Nemesis.
 *	2 - Priests start with a random alignment - convert the leader and
 *	    guardians here.
 *	3 - Elves can have one of two different leaders, but can't work it
 *	    out here because it requires hacking the level file data (see
 *	    sp_lev.c).
 *
 * This code also replaces quest_init().
 */
void
role_init()
{
	int alignmnt;
	int i,j,tmp;

	/* Strip the role letter out of the player name.
	 * This is included for backwards compatibility.
	 */
	plnamesuffix();

	/* Check for a valid role.  Try flags.initrole first. */
	if (!validrole(flags.initrole)) {
	    /* Try the player letter second */
	    if ((flags.initrole = str2role(pl_character)) < 0)
	    	/* None specified; pick a random role */
	    	flags.initrole = randrole();
	}

	/* We now have a valid role index.  Copy the role name back. */
	/* This should become OBSOLETE */
	Strcpy(pl_character, roles[flags.initrole].name.m);
	pl_character[PL_CSIZ-1] = '\0';

	/* Check for a valid race */
	if (!validrace(flags.initrole, flags.initrace))
	    flags.initrace = randrace(flags.initrole);

	/* Check for a valid gender.  If new game, check both initgend
	 * and female.  On restore, assume flags.female is correct. */
	if (flags.pantheon == -1) {	/* new game */
	    if (!validgend(flags.initrole, flags.initrace, flags.female))
		flags.female = !flags.female;
	}
	if (!validgend(flags.initrole, flags.initrace, flags.initgend))
	    /* Note that there is no way to check for an unspecified gender. */
	    flags.initgend = flags.female;

	/* Check for a valid alignment */
	if (!validalign(flags.initrole, flags.initrace, flags.initalign))
	    /* Pick a random alignment */
	    flags.initalign = randalign(flags.initrole, flags.initrace);
	alignmnt = aligns[flags.initalign].value;

	/* Initialize urole and urace */
	urole = roles[flags.initrole];
	urace = races[flags.initrace];
	for ( i=0; i<A_MAX; i++ ) {
		if ( i==A_STR ) {
		    tmp = urole.attrmax[A_STR];
		    if ( urace.attradj[i] > 0 ) {
			for ( j=0; j<urace.attradj[i]; j++ ) {
				if ( tmp < 18 ) tmp++;
				else if ( tmp < STR18(50) ) tmp = STR18(50);
				else if ( tmp < STR18(100) ) tmp = STR18(100);
				else tmp++;
			}
		    } else if ( urace.attradj[i] < 0 ) {
			for ( j=0; j<-urace.attradj[i]; j++ ) {
				if ( tmp <= 18 ) tmp--;
				else if ( tmp <= STR18(50) ) tmp = 18;
				else if ( tmp <= STR18(100) ) tmp = STR18(50);
				else tmp--;
			}
		    }
		    urole.attrmax[A_STR] = tmp;
		} else {
			urole.attrmax[i] += urace.attradj[i];
		}
	}

	/* Fix up the quest leader */
	if (urole.ldrnum != NON_PM) {
	    mons[urole.ldrnum].msound = MS_LEADER;
	    mons[urole.ldrnum].mflags2 |= (M2_PEACEFUL);
	    mons[urole.ldrnum].mflags3 |= M3_CLOSE;
	    mons[urole.ldrnum].maligntyp = alignmnt * 3;
	}

	/* Fix up the quest guardians */
	if (urole.guardnum != NON_PM) {
	    mons[urole.guardnum].mflags2 |= (M2_PEACEFUL);
	    mons[urole.guardnum].maligntyp = alignmnt * 3;
	}

	/* Fix up the quest nemesis */
	if (urole.neminum != NON_PM) {
	    mons[urole.neminum].msound = MS_NEMESIS;
	    mons[urole.neminum].mflags2 &= ~(M2_PEACEFUL);
	    mons[urole.neminum].mflags2 |= (M2_NASTY|M2_STALK|M2_HOSTILE);
	    mons[urole.neminum].mflags3 |= M3_WANTSARTI | M3_WAITFORU;
	}

	/* Fix up the god names */
	if (flags.pantheon == -1) {		/* new game */
	    flags.pantheon = flags.initrole;	/* use own gods */
	    while (!roles[flags.pantheon].lgod)	/* unless they're missing */
		flags.pantheon = randrole();
	}
	if (!urole.lgod) {
	    urole.lgod = roles[flags.pantheon].lgod;
	    urole.ngod = roles[flags.pantheon].ngod;
	    urole.cgod = roles[flags.pantheon].cgod;
	}

	/* Fix up infravision */
	if (mons[urace.malenum].mflags3 & M3_INFRAVISION) {
	    /* although an infravision intrinsic is possible, infravision
	     * is purely a property of the physical race.  This means that we
	     * must put the infravision flag in the player's current race
	     * (either that or have separate permonst entries for
	     * elven/non-elven members of each class).  The side effect is that
	     * all NPCs of that class will have (probably bogus) infravision,
	     * but since infravision has no effect for NPCs anyway we can
	     * ignore this.
	     */
	    mons[urole.malenum].mflags3 |= M3_INFRAVISION;
	    if (urole.femalenum != NON_PM)
	    	mons[urole.femalenum].mflags3 |= M3_INFRAVISION;
	}

	/* Artifacts are fixed in hack_artifacts() */

	/* Setup mnum */
	for (i=LOW_PM; i<NUMMONS; i++)
	    mons[i].mnum = i;

	/* Success! */
	return;
}

const char *
Hello(mtmp)
struct monst *mtmp;
{
	switch (Role_switch) {
#ifndef JP
	case PM_KNIGHT:
	    return ("Salutations"); /* Olde English */
	case PM_SAMURAI:
	    return (mtmp && mtmp->mnum == PM_SHOPKEEPER ?
	    		"Irasshaimase" : "Konnichi wa"); /* Japanese */
#endif /*JP*/
	case PM_TOURIST:
	    return (E_J("Aloha","?A???[?n"));       /* Hawaiian */
#ifndef JP
	case PM_VALKYRIE:
	    return (
#ifdef MAIL
	    		mtmp && mtmp->mnum == PM_MAIL_DAEMON ? "Hallo" :
#endif
	    		"Velkommen");   /* Norse */
#endif /*JP*/
	default:
	    return (E_J("Hello","?????ɂ???"));
	}
}

const char *
Goodbye()
{
	switch (Role_switch) {
	case PM_KNIGHT:
#ifndef JP
	    return ("Fare thee well");  /* Olde English */
#endif /*JP*/
	case PM_SAMURAI:
	    return (E_J("Sayonara","??????"));        /* Japanese */
	case PM_TOURIST:
	    return (E_J("Aloha","?A???[?n"));           /* Hawaiian */
	case PM_VALKYRIE:
	    return (E_J("Farvel","??????"));          /* Norse */
	default:
	    return (E_J("Goodbye","???悤?Ȃ?"));
	}
}

/* role.c */
