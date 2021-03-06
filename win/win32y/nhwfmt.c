#include "nhw.h"

/* ================================================
	Pre-defined menu formatting
   ================================================ */
/* ------------------------------------------------
	Option
   ------------------------------------------------ */
NHWMenuFormat mf_opt_1[] = {
	{ -1, -1, 0, 0 }, /* column for "a - " */
	{  0, 14, 0, 1 },
	{ 14,  0, 0, 2 }
};
NHWMenuFormat mf_opt_2[] = {
	{ -1, -1, 0, 0 },
	{  0, 14, 0, 1 },
	{ 14,  0, 0, 2 }
};
NHWMenuFormat mf_opt_3[] = {
	{ -1, -1, 0, 0 },
	{  0, 16, 0, 1 },
	{ 16,  0, 0, 2 }
};
int stateOptions, widthOptions;
int isOptions(BYTE *menu) {
	NHWMenuItem *mip;
	stateOptions = 0;
	mip = NHWFirstMenuItem(menu);
	return (mip != NULL && !strcmp(mip->str, E_J("Set what options?",
					"どのオプションを設定しますか？")));
}
NHWMenuFormat *formatOptions(NHWMenuItem *mip) {
	char *p;
	p = mip->str;
	switch (stateOptions) {
	    case 0:
		if (!mip->id.a_void) {
		    if (E_J(!strncmp(p, "Compounds", 9),
			    !strncmp(p, "引数を持つ", 10))) {
			stateOptions = 1;
			widthOptions = 0;
			return NULL;
		    }
		    if (strncmp(p, "    ", 4)) return NULL;
		    p += 4; /* skip "    " */
		}
		if (strlen(p) <= 14) return NULL;
		if (p[14] != '[') return NULL;
		return mf_opt_1;
	    case 1:
		if (widthOptions == 0) {
		    char *p0;
		    p0 = index(p, '[');
		    if (p0 == NULL) return NULL;
		    widthOptions = (p0-p);
		    if (!strncmp(p, "    ", 4)) widthOptions -= 4;
		    mf_opt_2[1].oend   = widthOptions;
		    mf_opt_2[2].ostart = widthOptions;
		    mf_opt_3[1].oend   = widthOptions;
		    mf_opt_3[2].ostart = widthOptions;
		    return mf_opt_2;
		}
		if (!mip->id.a_void) {
		    if (E_J(!strncmp(p, "Variable", 8),
			    !strncmp(p, "ファイルの", 10))) {
			stateOptions = 2;
			return NULL;
		    }
		    if (strncmp(p, "    ", 4)) return NULL;
		    p += 4; /* skip "    " */
		}
		if (strlen(p) <= widthOptions) return NULL;
		if (p[widthOptions] != '[') return NULL;
		return mf_opt_2;
	    case 2:
		if (!mip->id.a_void) {
		    if (strncmp(p, "    ", 4)) return NULL;
		    p += 4; /* skip "    " */
		}
		if (strlen(p) <= widthOptions) return NULL;
		if (p[widthOptions] != '[') return NULL;
		return mf_opt_3;
	    default:
		break;
	}
	return NULL;
}

/* ------------------------------------------------
	Skill
   ------------------------------------------------ */
NHWMenuFormat mf_skill_a[] = {
	{ -1, -1, 0, 0 }, /* column for "a - " */
	{  2, 14, 0, 1 }, /* skill name */
	{ 14, 19, MF_ALIGNRIGHT, 2 }, /* current skill */
	{ 19,  0, MF_ALIGNRIGHT, 3 }  /* skill gain */
};
int widthSkills;
int isSkills(BYTE *menu) {
	NHWMenuItem *mip;
	widthSkills = 0;
	mip = NHWFirstMenuItem(menu);
#ifndef JP
	return (mip != NULL &&
		(!strncmp(mip->str, "Pick a skill to", 15) ||
		 !strncmp(mip->str, "Current skills:", 15)));
#else
	return (mip != NULL &&
		(!strncmp(mip->str, "上達させたい技能", 16) ||
		 !strncmp(mip->str, "現在の技能", 10)));
#endif /*JP*/
}
NHWMenuFormat *formatSkills(NHWMenuItem *mip) {
	char *p;
	int f;
	p = mip->str;
	f = !strncmp(p, "   ", 3);
	if (widthSkills == 0) {
	    char *p0;
	    p0 = index(p, '%');
	    if (p0 == NULL) return NULL;
	    widthSkills = (p0-p)-4;
	    if (f) widthSkills -= 4;
	    mf_skill_a[1].oend   = widthSkills;
	    mf_skill_a[2].ostart = widthSkills;
	    mf_skill_a[2].oend   = widthSkills + 5;
	    mf_skill_a[3].ostart = widthSkills + 5;
	    return mf_skill_a;
	}
	if (p[0] != ' ') return NULL;
	return mf_skill_a;
}

/* ------------------------------------------------
	Spell
   ------------------------------------------------ */
NHWMenuFormat mf_spell_h[] = {
	{ -1, -1, 0, 0 }, /* column for "a - " */
	{  0, 21, 0, 1 }, /* Name */
	{ 21, 28, 0, 2 }, /* Level */
	{ 28, 41, 0, 3 }, /* Category */
	{ 41,  0, 0, 4 }  /* Fail */
};
NHWMenuFormat mf_spell_b[] = {
	{ -1, -1, 0		, 0 }, /* column for "a - " */
	{  0, 21, 0		, 1 }, /* Name */
	{ 21, 26, MF_ALIGNCENTER, 2 }, /* Level */
	{ 28, 41, 0		, 3 }, /* Category */
	{ 41,  0, MF_ALIGNRIGHT , 4 }  /* Fail */
};
int isSpells(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
#ifndef JP
	return (mip != NULL &&
		(!strncmp(mip->str, "Currently known spells", 22) ||
		 !strncmp(mip->str, "Choose which spell", 18) ||
		 !strncmp(mip->str, "Reordering spells", 17)));
#else
	return (mip != NULL &&
		(!strncmp(mip->str, "現在憶えている呪文", 18) ||
		 !strncmp(mip->str, "唱える呪文の選択", 16) ||
		 !strncmp(mip->str, "呪文の並べ替え", 14)));
#endif /*JP*/
}
NHWMenuFormat *formatSpells(NHWMenuItem *mip) {
	if (mip->accelerator) return mf_spell_b;
	if (!strncmp(mip->str, E_J("    Name","    名称"), 8)) return mf_spell_h;
	return NULL;
}

/* ------------------------------------------------
	Technique
   ------------------------------------------------ */
NHWMenuFormat mf_tech_h[] = {
	{ -1, -1, 0, 0 }, /* column for "a - " */
	{  0, 23, 0, 1 }, /* Name */
	{ 23, 31, 0, 2 }, /* Level */
	{ 31,  0, 0, 3 }  /* Status */
};
NHWMenuFormat mf_tech_b[] = {
	{ -1, -1, 0		, 0 }, /* column for "a - " */
	{  0, 23, 0		, 1 }, /* Name */
	{ 23, 31, MF_ALIGNCENTER, 2 }, /* Level */
	{ 31,  0, 0             , 3 }  /* Status */
};
int isTech(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
#ifndef JP
	return (mip != NULL &&
		(!strncmp(mip->str, "Choose a technique", 18) ||
		 !strncmp(mip->str, "Currently known techniques", 26)));
#else
	return (mip != NULL &&
		(!strncmp(mip->str, "特殊技能の選択", 14) ||
		 !strncmp(mip->str, "習得した特殊技能", 16)));
#endif /*JP*/
}
NHWMenuFormat *formatTech(NHWMenuItem *mip) {
	if (mip->accelerator) return (mip->accelerator == '?') ? NULL : mf_tech_b;
	if (!strncmp(mip->str, E_J("    Name","    名称"), 8)) return mf_tech_h;
	if (!strncmp(mip->str, "    ", 4)) return mf_tech_b;
	return NULL;
}

/* ------------------------------------------------
	Base attributes
   ------------------------------------------------ */
NHWMenuFormat mf_battr[] = {
	{  0, 15, 0, 0 },
	{ 15, 16, 0, 1 },
	{ 17,  0, 0, 2 }
};
NHWMenuFormat mf_battr2[] = {
	{  0, 15, 0, 0 },
	{ 15, 16, 0, 1 },
	{ 17, 34, 0, 2 },
	{ 34,  0, 0, 3 }
};
int isBAttr(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
#ifndef JP
	return (mip != NULL && !strncmp(mip->str, "Base Attr", 9));
#else
	return (mip != NULL && !strncmp(mip->str, "基本属性", 8));
#endif /*JP*/
}
NHWMenuFormat *formatBAttr(NHWMenuItem *mip) {
	char *p;
	p = mip->str;
	if (strlen(p) >= 16 && p[15] == ':') {
	    if (index(p, '(') != NULL) return mf_battr2;
	    return mf_battr;
	}
	return NULL;
}

/* ------------------------------------------------
	Unpaid Items
   ------------------------------------------------ */
NHWMenuFormat mf_unpaid[] = {
	{  0,  1, MF_ALIGNCENTER, 0 }, /* 'a' */
	{  2,  3, MF_ALIGNCENTER, 1 }, /* '-' */
	{  4, 49, 0,		  2 }, /* item name */
	{ 50, 56, MF_ALIGNRIGHT,  3 }, /* cost */
	{ 57,  0, 0,		  4 }, /* 'zorkmids' */
};
int isUnpaid(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
#ifndef JP
	return (mip != NULL && !strncmp(mip->str, "Unpaid ", 7));
#else
	return (mip != NULL &&
		(!strncmp(mip->str, "未払いの", 8) ||
		 !strncmp(mip->str, "支払い前", 8)));
#endif /*JP*/
}
NHWMenuFormat *formatUnpaid(NHWMenuItem *mip) {
	char *p;
	p = mip->str;
	if (strlen(p) >= 4 && p[1] == ' ' &&
	    p[2] == '-' && p[3] == ' ') return mf_unpaid;
	return NULL;
}

/* ------------------------------------------------
	Vanquished creatures
   ------------------------------------------------ */
NHWMenuFormat mf_vanq[] = {
#ifndef JP
	{  0,  4, MF_ALIGNRIGHT, 0 }, /* num */
	{  5,  0, 0,		 1 }, /* creature name */
#else
	{  0,  8, MF_ALIGNRIGHT, 0 }, /* num */
	{  9,  0, 0,		 1 }, /* creature name */
#endif /*JP*/
};
isVanq(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
	return (mip != NULL && !strncmp(mip->str, E_J("Vanquished","打倒した敵"), 10));
}
NHWMenuFormat *formatVanqC(NHWMenuItem *mip) {
	char c, *p;
	int l;
	p = mip->str;
	l = strlen(p);
#ifndef JP
	if (l < 6) return NULL;
	c = p[l-1];
	if (c == '.' || c == ':') return NULL;
#else
	if (l < 10) return NULL;
	c = p[8];
	if (c != ' ') return NULL;
#endif /*JP*/
	return mf_vanq;
}

/* ------------------------------------------------
	wizcommand: lightsources
   ------------------------------------------------ */
#ifdef WIZARD
NHWMenuFormat mf_lights_h[] = {
	{  0,  8, 0, 0 },
	{  9, 14, 0, 1 },
	{ 15, 21, 0, 2 },
	{ 22, 26, 0, 3 },
	{ 28,  0, 0, 4 }
};
NHWMenuFormat mf_lights_b[] = {
	{  2,  7, MF_ALIGNRIGHT	, 0 },
	{ 10, 12, MF_ALIGNCENTER, 1 },
	{ 15, 21, 0		, 2 },
	{ 23, 26, 0		, 3 },
	{ 28,  0, 0		, 4 }
};
int isLightS(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
	if (!wizard) return 0;
	return (mip != NULL && !strncmp(mip->str, "Mobile light", 12));
}
NHWMenuFormat *formatLightS(NHWMenuItem *mip) {
	char *p;
	p = mip->str;
	if (!strncmp(p, "  ", 2))
	    return mf_lights_b;
	if (!strncmp(p, "location", 8) ||
	    !strncmp(p, "--------", 8))
	    return mf_lights_h;
	return NULL;
}
#endif

/* ------------------------------------------------
	wizcommand: lightsources
   ------------------------------------------------ */
#ifdef WIZARD
NHWMenuFormat mf_timeout_h[] = {
	{  0,  7, 0, 0 },
	{  9, 11, 0, 1 },
	{ 14, 18, 0, 2 },
	{ 21,  0, 0, 3 }
};
NHWMenuFormat mf_timeout_b[] = {
	{  1,  5, MF_ALIGNRIGHT	, 0 },
	{  8, 12, MF_ALIGNRIGHT	, 1 },
	{ 14, 20, 0		, 2 },
	{ 21,  0, 0		, 3 }
};
int isTimeOut(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
	if (!wizard) return 0;
	return (mip != NULL && !strncmp(mip->str, "Current time", 12));
}
NHWMenuFormat *formatTimeOut(NHWMenuItem *mip) {
	char *p;
	p = mip->str;
	if (*p == ' ') return mf_timeout_b;
	if (!strncmp(p, "timeout", 7))
	    return mf_timeout_h;
	return NULL;
}
#endif

/* ------------------------------------------------
	topten list
   ------------------------------------------------ */
NHWMenuFormat mf_topten_h[] = {
	{  1,  3, 0	       , 0 }, /* No */
	{  5, 11, 0	       , 1 }, /* Points */
	{ 16, 20, 0	       , 2 }, /* Name */
	{ 71, 73, 0	       , 3 }, /* Hp */
	{ 74,  0, MF_ALIGNRIGHT, 4 }  /* [max] */
};
NHWMenuFormat mf_topten_b[] = {
	{  0,  3, MF_ALIGNRIGHT, 0 },
	{  4, 14, MF_ALIGNRIGHT, 1 },
	{ 16, 69, 0	       , 2 },
	{ 69, 73, MF_ALIGNRIGHT, 3 },
	{ 73,  0, MF_ALIGNRIGHT, 4 }
};
char *toptenid = " No  Points";
int isTopTen(BYTE *menu) {
	NHWMenuItem *mip;
	mip = NHWFirstMenuItem(menu);
	return (mip != NULL && !strncmp(mip->str, toptenid, 11));
}
NHWMenuFormat *formatTopTen(NHWMenuItem *mip) {
	if (!strncmp(mip->str, toptenid, 11)) return mf_topten_h;
	return mf_topten_b;
}
//           1111111111222222222233333333334444444444555555555566666666667777777777
// 01234567890123456789012345678901234567890123456789012345678901234567890123456789
// RRR PPPPPPPPPP  dddddddddddddddddddddddddddddddddddddddddddddddddddddHHHH MMMMM
//  No  Points     Name                                                   Hp [max]
//   1      17176  Youkan-Bar-Hum-Mal-Neu died in The Dungeons of Doom
//                 on level 9 [max 10].  Killed by a human mummy.          -  [87]
//   2       3952  Youkan-Wiz-Orc-Fem-Cha quit in The Dungeons of Doom
//                 on level 5 [max 7].                                    16  [43]

/* ================================================
	Format recognition table
   ================================================ */
NHWMenuFormatInfo menu_format_info[] = {
	{ isSpells,	formatSpells },
	{ isSkills,	formatSkills },
	{ isTech,	formatTech },
	{ isOptions,	formatOptions },
	{ isBAttr,	formatBAttr },
	{ isUnpaid,	formatUnpaid },
#ifdef WIZARD
	{ isTimeOut,	formatTimeOut },
	{ isLightS,	formatLightS },
#endif
	{ isVanq,	formatVanqC },
	{ isTopTen,	formatTopTen },
	{ NULL, NULL }
};
