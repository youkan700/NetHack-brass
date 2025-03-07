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
{	{E_J("Archeologist",	"考古学者"	), 0}, {
	{E_J("Digger",		"穴掘り"	), 0},
	{E_J("Field Worker",	"実地調査員"	), 0},
	{E_J("Investigator",	"研究者"	), 0},
	{E_J("Exhumer",		"墳墓調査員"	), 0},
	{E_J("Excavator",	"遺跡発掘家"	), 0},
	{E_J("Spelunker",	"探検家"	), 0},
	{E_J("Speleologist",	"洞窟学者"	), 0},
	{E_J("Collector",	"収集家"	), 0},
	{E_J("Curator",		"博物館長"	), 0} },
#ifndef JP
	"Quetzalcoatl", "Camaxtli", "Huhetotl", /* Central American */
	"Arc", "the College of Archeology", "the Tomb of the Toltec Kings",
	"[FIXME]\0"
#else
	"ケツァルコアトル", "カマキシトリ", "フフェテォトル",
	"Arc", "考古学大学", "トルテカ王家の墳墓",
	"遺跡の探検や発掘調査に長けた職業です。\0"
	"戦闘はあまり得意ではありませんが、静かに素早く行動でき、\0"
	"仕事に必要な便利な道具をいくつか持ち込んで出発します。\0"
	"罠の発見、宝石の鑑定が得意です。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Barbarian",	"野蛮人"	), 0}, {
	{E_J("Plunderer",	"盗人"		), E_J("Plunderess", 0) },
	{E_J("Pillager",	"略奪する者"	), 0},
	{E_J("Bandit",		"野盗"		), 0},
	{E_J("Brigand",		"武装強盗"	), 0},
	{E_J("Raider",		"襲撃する者"	), 0},
	{E_J("Reaver",		"強奪する者"	), 0},
	{E_J("Slayer",		"殺戮する者"	), 0},
	{E_J("Chieftain",	"首領"		), E_J("Chieftainess","女首領")},
	{E_J("Conqueror",	"征服王"	), E_J("Conqueress",0)} },
#ifndef JP
	"Mitra", "Crom", "Set", /* Hyborian */
	"Bar", "the Camp of the Duali Tribe", "the Duali Oasis",
	"[FIXME]\0"
#else
	"ミトラ", "クロム", "セト", /* Hyborian */
	"Bar", "デュアリ族のキャンプ", "デュアリ・オアシス",
	"あらゆる武器と防具の扱いに長けた、近接戦闘の専門家です。\0"
	"立ちふさがる相手をすべて叩き潰す力を持ちますが、\0"
	"魔法は好みません。また、毒への耐性を身につけています。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Caveman",		"洞窟人"	), E_J("Cavewoman",0)}, {
	{E_J("Troglodyte",	"穴居人"	), 0},
	{E_J("Aborigine",	"原住民"	), 0},
	{E_J("Wanderer",	"放浪者"	), 0},
	{E_J("Vagrant",		"浮浪者"	), 0},
	{E_J("Wayfarer",	"旅人"		), 0},
	{E_J("Roamer",		"流浪人"	), 0},
	{E_J("Nomad",		"遊牧民"	), 0},
	{E_J("Rover",		"歴訪者"	), 0},
	{E_J("Pioneer",		"先駆者"	), 0} },
#ifndef JP
	"Anu", "_Ishtar", "Anshar", /* Babylonian */
	"Cav", "the Caves of the Ancestors", "the Dragon's Lair",
	"[FIXME]\0"
#else
	"アヌ", "_イシュタル", "アンシャル", /* Babylonian */
	"Cav", "高祖の洞窟", "竜の棲み処",
	"洞窟に住み、素朴な暮らしをしている一族です。\0"
	"力が強く、原始的な武具を使った戦闘が得意です。\0"
	"魔法はやや苦手です。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Healer",		"癒し手"	), 0}, {
	{E_J("Rhizotomist",	"薬草採り"	), 0}, /* rhizotomist 薬効のために植物の根を集める人 */
	{E_J("Empiric",		"医学実習生"	), 0}, /* empiric 理論よりも経験・実験を重んじる人 */
	{E_J("Embalmer",	"遺体整復師"	), 0},
	{E_J("Dresser",		"外科助手"	), 0}, /* dresser (Br)外科助手、包帯係 */
	{E_J("Medicus ossium",	"接骨医"	), E_J("Medica ossium",0)}, /* medicus (Latin)医者, ossium (Latin)骨 */
	{E_J("Herbalist",	"薬草学者"	), 0},
	{E_J("Magister",	"医学博士"	), E_J("Magistra",0) }, /* magister (Latin)先生、学校長 */
	{E_J("Physician",	"薬師"		), 0}, /* physician 内科医 */
	{E_J("Chirurgeon",	"典医"		), 0} }, /* chirurgeon 外科医 surgeon の古語 */
#ifndef JP
	"_Athena", "Hermes", "Poseidon", /* Greek */
	"Hea", "the Temple of Epidaurus", "the Temple of Coeus",
	"[FIXME]\0"
#else
	"_アテナ", "ヘルメス", "ポセイドン", /* Greek */
	"Hea", "エピダウロス寺院", "コイオス寺院",
	"病気や怪我を癒す技術を習得している職業です。\0"
	"戦闘はあまり得意ではありませんが、生き延びるのに必要な\0"
	"知識を持ち、さまざまな医療道具と薬を携えています。\0"
	"職業柄、はじめから毒への耐性を身につけています。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Knight",		"騎士"		), 0}, {
	{E_J("Gallant",		"小姓"		), 0}, /* 礼儀正しい・勇敢な若い男…訳せないのでpage扱いで */
	{E_J("Esquire",		"従士"		), 0}, /* 騎士の下で世話をする者 */
	{E_J("Bachelor",	"騎兵"		), 0}, /* 下位の騎士 統治者から騎士の位を得たが、騎士団には未所属 */
	{E_J("Sergeant",	"騎兵長"	), 0}, /* わからないので適当に… */
	{E_J("Knight",		"騎士"		), 0}, /* (Bachelorのときに叙任されたはずなのだが…) */
	{E_J("Banneret",	"騎士団長"	), 0}, /* 四角い旗を持って自分の騎士団を率いる地位の騎士 */
	{E_J("Chevalier",	"重騎士"	), E_J("Chevaliere",0)}, /* フランス語の騎士 シュバリエ */
	{E_J("Seignieur",	"殊勲騎士"	), E_J("Dame",0)}, /* 封建領主 Sir,Dameは騎士の称号 */
	{E_J("Paladin",		"聖騎士"	), 0} },
#ifndef JP
	"Lugh", "_Brigit", "Manannan Mac Lir", /* Celtic */
	"Kni", "Camelot Castle", "the Isle of Glass",
	"[FIXME]\0"
#else
	"ルーフ", "_ブリジット", "マナナン・マクリール", /* Celtic */
	"Kni", "キャメロット城", "ガラスの島",
	"独特な騎士道精神を重んじる戦士です。武器の扱いに長け、\0"
	"重装備での戦闘が得意です。騎乗しての戦闘も得意で、\0"
	"馬上槍を構えての突撃は高い威力を誇ります。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Monk",		"修行僧"	), 0}, {
	{E_J("Candidate",	"門を叩く者"	), 0},
	{E_J("Novice",		"初心者"	), 0},
	{E_J("Initiate",	"入門者"	), 0},
	{E_J("Student of Stones","土の習い手"	), 0},
	{E_J("Student of Waters","水の習い手"	), 0},
	{E_J("Student of Metals","金の習い手"	), 0},
	{E_J("Student of Winds","木の習い手"	), 0},
	{E_J("Student of Fire",	"火の習い手"	), 0},
	{E_J("Master",		"免許皆伝"	), 0} },
#ifndef JP
	"Shan Lai Ching", "Chih Sung-tzu", "Huan Ti", /* Chinese */
	"Mon", "the Monastery of Chan-Sune",
	  "the Monastery of the Earth-Lord",
	"[FIXME]\0"
#else
	"禹帝", "赤松子", "黄帝", /* Chinese */
	"Mon", "チャン・スー修道院", "地王の修道院",
	"独自の戒律を守り、自然の中で修行をしている修道士です。\0"
	"徒手空拳での戦いを最も得意とします。重い鎧は苦手ですが、\0"
	"軽装であっても重戦士に引けを取らない戦闘力を発揮します。\0"
	"戒律により、肉食を避けています。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Medium",		"巫女"		), 0}, {
	{E_J("Pure Girl",	"無垢な少女"	), 0},
	{E_J("Jinme Girl",	"御神馬の少女"	), 0},
	{E_J("Mist Maiden",	"朝霧の巫女"	), 0},
	{E_J("Moon Maiden",	"夕月の巫女"	), 0},
	{E_J("Princess Maiden",	"姫巫女"	), 0},
	{E_J("Hanuman Maiden",	"山神の巫女"	), 0},
	{E_J("Ryujin Maiden",	"龍神の巫女"	), 0},
	{E_J("Kamiko",		"神子"		), 0},
	{E_J("Eternal Maiden",	"常乙女"	), 0} },
#ifndef JP
	"_Amaterasu Omikami", "Raijin", "Susanowo", /* Japanese */
	"Med", "the Old Shrine", "the Cave of fox-spirits",
	"[FIXME]\0"
#else
	"_天照大神", "雷神", "須佐之男", /* Japanese */
	"Med", "古き神社", "妖狐の洞窟",
	"神社に仕える神職の女性です。力は弱いですが、弓が得意です。\0"
	"清めの力を持ち、矢に破魔の力をこめて放つことができます。\0"
	"お札（巻物）を投げて力を引き出す特殊能力を持っています。\0"
#endif /*JP*/
	,/*EOS*/
	PM_MEDIUM, NON_PM, NON_PM,
	PM_SHINTO_PRIEST, PM_MAIDEN, PM_NINE_TAILED_FOX,
	PM_FOX_SPIRIT, PM_HUMAN_ZOMBIE, S_DOG, S_ZOMBIE,
	ART_HOLY_DAGGER,
	MH_HUMAN | ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{   7,  7,  9, 10,  7, 16 },
	{  15, 20, 10, 25, 20, 15 },
	{  16, 18, 16, 20, 15, 20 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 13, 0,  0, 8,  1, 0 },        /* Hit points */
	{  4, 3,  0, 2,  0, 2 },10,     /* Energy */
	10, 7, -2, 2, 10, A_INT, SPE_REMOVE_CURSE,   -4
},
/*	ローマ・カトリック教会の階層組織
	教皇 > 枢機卿 > 大司教 > 司教 > 司祭 > 助祭
 */
{	{E_J("Priest",		"僧侶"		), E_J("Priestess","尼僧")}, {
	{E_J("Aspirant",	"修道士"	), E_J(0,"修道女")},
	{E_J("Acolyte",		"侍者"		), 0}, /* 司祭の手伝い */
	{E_J("Adept",		"侍祭"		), 0},
	{E_J("Priest",		"僧侶"		), E_J("Priestess","尼僧")},
	{E_J("Curate",		"助祭"		), 0},
	{E_J("Canon",		"聖者"		), E_J("Canoness","聖女")},
	{E_J("Lama",		"司教"		), 0},
	{E_J("Patriarch",	"大司教"	), E_J("Matriarch",0)},
	{E_J("High Priest",	"大僧正"	), E_J("High Priestess","女教皇")} },
	0, 0, 0,	/* chosen randomly from among the other roles */
#ifndef JP
	"Pri", "the Great Temple", "the Temple of Nalzok",
	"[FIXME]\0"
#else
	"Pri", "偉大なる寺院", "ナルゾクの寺院",
	"寺院に仕える僧侶戦士です。\0"
	"純粋な戦士には及ばないものの、重装備での戦闘をこなし、\0"
	"鈍器を扱うのが得意で、魔法も使えます。\0"
	"品物の祝福・呪いの状態を一目で見分けることができます。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Rogue",		"盗賊"), 0}, {
	{E_J("Footpad",		"追い剥ぎ"),     0},
	{E_J("Cutpurse",	"スリ"),    0},
	{E_J("Rogue",		"盗賊"),       0},
	{E_J("Pilferer",	"こそ泥"),    0},
	{E_J("Robber",		"強盗"),      0},
	{E_J("Burglar",		"空巣"),     0},
	{E_J("Filcher",		"万引き犯"),     0},
	{E_J("Magsman",		"詐欺師"),     E_J("Magswoman", 0) },
	{E_J("Thief",		"大泥棒"),       0} },
#ifndef JP
	"Issek", "Mog", "Kos", /* Nehwon */
	"Rog", "the Thieves' Guild Hall", "the Assassins' Guild Hall",
	"[FIXME]\0"
#else
	"イセク", "モグ", "コス", /* Nehwon */
	"Rog", "盗賊ギルドのアジト", "暗殺者ギルドのアジト",
	"盗みを生業としているごろつきです。\0"
	"素早く、手先が器用で、鍵や罠を外したり、\0"
	"相手から所持品を盗み取るのが得意です。\0"
	"小型の刃物を使いこなし、特殊な暗殺術を習得します。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Ranger","レンジャー"), 0}, {
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
	{E_J("Tenderfoot",	"新米"),    0},
	{E_J("Lookout",		"見張り"),       0},
	{E_J("Trailblazer",	"先導者"),   0},
	{E_J("Reconnoiterer",	"偵察"), E_J("Reconnoiteress",0)},
	{E_J("Scout",		"斥候"),         0},
	{E_J("Arbalester",	"射手"),    0},	/* One skilled at crossbows */
	{E_J("Archer",		"弓兵"),        0},
	{E_J("Sharpshooter",	"強射手"),  0},
	{E_J("Marksman",	"狙撃手"),      E_J("Markswoman",0)} },
#ifndef JP
	"Mercury", "_Venus", "Mars", /* Roman/planets */
	"Ran", "Orion's camp", "the cave of the wumpus",
	"[FIXME]\0"
#else
	"マーキュリー", "_ヴィーナス", "マーズ",
	"Ran", "オリオンのキャンプ", "ワンパスの洞窟",
	"山野での活動に長けた職業で、きわめて強力な射手です。\0"
	"弓の扱いがとても上手く、遠距離から敵を倒します。\0"
	"敵に気づかれずに素早く行動するすべを身につけています。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Samurai",		"侍"	), 0}, {
/*	{"Hatamoto",      0},*/  /* Banner Knight */
/*	{"Kuge",          0},*/  /* Noble of the Court */
/*	{"Ninja","Kunoichi"},*/  /* secret society */
	{E_J("Ronin",		"浪人"	), 0},  /* no allegiance */
	{E_J("Ashigaru",	"足軽"	), 0},  /* footman */
	{E_J("Kumigashira",	"侍組頭"), 0},  /* leader of Ashigaru */
	{E_J("Bushou",		"武将"	), 0},  /* general */
	{E_J("Ryoshu",		"領主"	), 0},  /* has a territory */
	{E_J("Joshu",		"城主"	), 0},  /* heads a castle */
	{E_J("Kokushu",		"国主"	), 0},  /* heads a province */
	{E_J("Daimyo",		"大名"	), 0},  /* a samurai lord */
	{E_J("Shogun",		"将軍"	), 0} },/* supreme commander, warlord */
#ifndef JP
	"_Amaterasu Omikami", "Raijin", "Susanowo", /* Japanese */
	"Sam", "the Castle of the Taro Clan", "the Shogun's Castle",
	"[FIXME]\0"
#else
	"_天照大神", "雷神", "須佐之男", /* Japanese */
	"Sam", "太郎一族の城", "将軍の城",
	"東方の辺境から来た、風変わりな戦士です。\0"
	"異国の強力な武具を装備しており、恐るべき戦闘力で\0"
	"立ちはだかる者すべてを斬り払います。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Tourist",		"観光客"	), 0}, {
	{E_J("Rambler",		"散歩者"	), 0},
	{E_J("Sightseer",	"観光客"	), 0},
	{E_J("Excursionist",	"周遊客"	), 0},
	{E_J("Peregrinator",	"歴訪者"),E_J("Peregrinatrix",0)},
	{E_J("Traveler",	"旅行者"	), 0},
	{E_J("Journeyer",	"旅人"		), 0},
	{E_J("Voyager",		"航海者"	), 0},
	{E_J("Explorer",	"探検家"	), 0},
	{E_J("Adventurer",	"冒険者"	), 0} },
#ifndef JP
	"Blind Io", "_The Lady", "Offler", /* Discworld */
	"Tou", "Ankh-Morpork", "the Thieves' Guild Hall",
	"[FIXME]\0"
#else
	"盲目のイオ", "_《女神》", "オフラー", /* Discworld */
	"Tou", "アンク=モルポーク", "盗賊ギルドのアジト",
	"運命の大迷宮に観光に来た旅行者です。\0"
	"地図とカメラを手に、お土産を買うためのお金と、\0"
	"たくさんの食べ物をカバンに詰め、迷宮を見物します。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Valkyrie",	"ワルキューレ"	), 0}, {
	{E_J("Stripling",	"新兵"), 0},
	{E_J("Skirmisher",	"斥候兵"), 0},
	{E_J("Fighter",		"戦士"), 0},
	{E_J("Man-at-arms",	"重装兵"),		E_J("Woman-at-arms",	"剣の乙女") },
	{E_J("Warrior",		"戦闘兵"),		E_J(0,			"戦乙女") },
	{E_J("Swashbuckler",	"剣士"),		E_J(0,			"盾持つ乙女") },
	{E_J("Hero",		"英雄"),		E_J("Heroine",		0)},
	{E_J("Champion",	"強者"),		E_J(0,			"女傑") },
	{E_J("Lord",		"貴人"),		E_J("Lady",		"貴婦人")} },
#ifndef JP
	"Tyr", "Odin", "Loki", /* Norse */
	"Val", "the Shrine of Destiny", "the cave of Surtur",
	"[FIXME]\0"
#else
	"テュール", "オーディン", "ロキ", /* Norse */
	"Val", "運命の聖堂", "スルトの洞窟",
	"北方からやってきた女性戦士です。\0"
	"力が強く、近接戦闘を得意とします。魔法は苦手です。\0"
	"盾の扱いに特に秀でており、敵の攻撃を寄せ付けません。\0"
#endif /*JP*/
	,/*EOS*/
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
{	{E_J("Wizard",		"魔法使い"	), 0}, {
	{E_J("Evoker",		"手品師"), 0},
	{E_J("Conjurer",	"奇術師"), 0},
	{E_J("Thaumaturge",	"呪術師"), 0},
	{E_J("Magician",	"魔術師"), 0},
	{E_J("Enchanter",	"心術師"), E_J("Enchantress",0)},
	{E_J("Sorcerer",	"魔道師"), E_J("Sorceress",0)},
	{E_J("Necromancer",	"妖術師"), 0},
	{E_J("Wizard",		"魔法使い"), 0},
	{E_J("Mage",		"大魔術師"), 0} },
#ifndef JP
	"Ptah", "Thoth", "Anhur", /* Egyptian */
	"Wiz", "the Lonely Tower", "the Tower of Darkness",
	"[FIXME]\0"
#else
	"プタハ", "トート", "アンフル", /* Egyptian */
	"Wiz", "隠されし塔", "暗黒の塔",
	"魔法のエキスパートです。\0"
	"力は弱く、武器を使った戦闘はあまり得意ではありませんが、\0"
	"高い知性であらゆる魔法を使いこなします。\0"
#endif /*JP*/
	,/*EOS*/
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
	"L", "N", "C", "Xxx", "home", "locate", "abstract\0",
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
	"人間", "人間の", "人類", "Hum",
	{"男", "女"},
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
	"エルフ", "エルフの", "エルフ族", "Elf",
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
	"ドワーフ", "ドワーフの", "ドワーフ族", "Dwa",
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
	"ノーム", "ノームの", "ノーム族", "Gno",
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
	"オーク", "オークの", "オーク族", "Orc",
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
	{"男性",	"彼",	"彼",	"彼の",	"Mal",	ROLE_MALE},
	{"女性",	"彼女",	"彼女","彼女の","Fem",	ROLE_FEMALE},
	{"中性",	"それ",	"それ",	"その",	"Ntr",	ROLE_NEUTER}
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
	{"法",		"秩序",		"Law",	ROLE_LAWFUL,	A_LAWFUL},
	{"均衡",	"中立",		"Neu",	ROLE_NEUTRAL,	A_NEUTRAL},
	{"混沌",	"混沌",		"Cha",	ROLE_CHAOTIC,	A_CHAOTIC},
	{"邪悪",	"無心",		"Una",	0,		A_NONE}
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
	if (post_attribs-- < num_post_attribs) Strcat(buf, "・");
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
	static char err_ret[] = E_J(" character's","キャラクターの");
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
		Strcat(buf, E_J("character","キャラクター"));
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
		Strcat(tmpbuf, E_J("your ","あなたの"));
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
			Strcat(buf, E_J("race","種族"));
		}
		if (pa[BP_ROLE]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("role","職業"));
		}
		if (pa[BP_GEND]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("gender","性別"));
		}
		if (pa[BP_ALIGN]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("alignment","属性"));
		}
	}
	Strcat(buf, E_J(" for you? [ynq] ","を適当に決めますか？ [ynq] "));
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
	    return (E_J("Aloha","アローハ"));       /* Hawaiian */
#ifndef JP
	case PM_VALKYRIE:
	    return (
#ifdef MAIL
	    		mtmp && mtmp->mnum == PM_MAIL_DAEMON ? "Hallo" :
#endif
	    		"Velkommen");   /* Norse */
#endif /*JP*/
	default:
	    return (E_J("Hello","こんにちは"));
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
	    return (E_J("Sayonara","さらば"));        /* Japanese */
	case PM_TOURIST:
	    return (E_J("Aloha","アローハ"));           /* Hawaiian */
	case PM_VALKYRIE:
	    return (E_J("Farvel","さらば"));          /* Norse */
	default:
	    return (E_J("Goodbye","さようなら"));
	}
}

/* role.c */
