/*	SCCS Id: @(#)rumors.c	3.4	1996/04/20	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "dlb.h"

/*	[note: this comment is fairly old, but still accurate for 3.1]
 * Rumors have been entirely rewritten to speed up the access.  This is
 * essential when working from floppies.  Using fseek() the way that's done
 * here means rumors following longer rumors are output more often than those
 * following shorter rumors.  Also, you may see the same rumor more than once
 * in a particular game (although the odds are highly against it), but
 * this also happens with real fortune cookies.  -dgk
 */

/*	3.1
 * The rumors file consists of a "do not edit" line, a hexadecimal number
 * giving the number of bytes of useful/true rumors, followed by those
 * true rumors (one per line), followed by the useless/false/misleading/cute
 * rumors (also one per line).  Number of bytes of untrue rumors is derived
 * via fseek(EOF)+ftell().
 *
 * The oracles file consists of a "do not edit" comment, a decimal count N
 * and set of N+1 hexadecimal fseek offsets, followed by N multiple-line
 * records, separated by "---" lines.  The first oracle is a special case,
 * and placed there by 'makedefs'.
 */

STATIC_DCL void FDECL(init_rumors, (dlb *));
STATIC_DCL void FDECL(init_oracles, (dlb *));

static long true_rumor_start,  true_rumor_size,  true_rumor_end,
	    false_rumor_start, false_rumor_size, false_rumor_end;
static int oracle_flg = 0;  /* -1=>don't use, 0=>need init, 1=>init done */
static unsigned oracle_cnt = 0;
static long *oracle_loc = 0;

STATIC_OVL void
init_rumors(fp)
dlb *fp;
{
	char line[BUFSZ];

	(void) dlb_fgets(line, sizeof line, fp); /* skip "don't edit" comment */
	(void) dlb_fgets(line, sizeof line, fp);
	if (sscanf(line, "%6lx\n", &true_rumor_size) == 1 &&
	    true_rumor_size > 0L) {
	    (void) dlb_fseek(fp, 0L, SEEK_CUR);
	    true_rumor_start  = dlb_ftell(fp);
	    true_rumor_end    = true_rumor_start + true_rumor_size;
	    (void) dlb_fseek(fp, 0L, SEEK_END);
	    false_rumor_end   = dlb_ftell(fp);
	    false_rumor_start = true_rumor_end;	/* ok, so it's redundant... */
	    false_rumor_size  = false_rumor_end - false_rumor_start;
	} else
	    true_rumor_size = -1L;	/* init failed */
}

/* exclude_cookie is a hack used because we sometimes want to get rumors in a
 * context where messages such as "You swallowed the fortune!" that refer to
 * cookies should not appear.  This has no effect for true rumors since none
 * of them contain such references anyway.
 */
char *
getrumor(truth, rumor_buf, exclude_cookie)
int truth; /* 1=true, -1=false, 0=either */
char *rumor_buf;
boolean exclude_cookie; 
{
	dlb	*rumors;
	long tidbit, beginning;
	char	*endp, line[BUFSZ], xbuf[BUFSZ];

	rumor_buf[0] = '\0';
	if (true_rumor_size < 0L)	/* we couldn't open RUMORFILE */
		return rumor_buf;

	rumors = dlb_fopen(RUMORFILE, "r");

	if (rumors) {
	    int count = 0;
	    int adjtruth;

	    do {
		rumor_buf[0] = '\0';
		if (true_rumor_size == 0L) {	/* if this is 1st outrumor() */
		    init_rumors(rumors);
		    if (true_rumor_size < 0L) {	/* init failed */
			Sprintf(rumor_buf, "Error reading \"%.80s\".",
				RUMORFILE);
			return rumor_buf;
		    }
		}
		/*
		 *	input:      1    0   -1
		 *	 rn2 \ +1  2=T  1=T  0=F
		 *	 adj./ +0  1=T  0=F -1=F
		 */
		switch (adjtruth = truth + rn2(2)) {
		  case  2:	/*(might let a bogus input arg sneak thru)*/
		  case  1:  beginning = true_rumor_start;
#ifdef RAND_MT
			    tidbit = rn2(true_rumor_size);
#else
			    tidbit = Rand() % true_rumor_size;
#endif
			break;
		  case  0:	/* once here, 0 => false rather than "either"*/
		  case -1:  beginning = false_rumor_start;
#ifdef RAND_MT
			    tidbit = rn2(false_rumor_size);
#else
			    tidbit = Rand() % false_rumor_size;
#endif
			break;
		  default:
			    impossible("strange truth value for rumor");
			return strcpy(rumor_buf, "Oops...");
		}
		(void) dlb_fseek(rumors, beginning + tidbit, SEEK_SET);
		(void) dlb_fgets(line, sizeof line, rumors);
		if (!dlb_fgets(line, sizeof line, rumors) ||
		    (adjtruth > 0 && dlb_ftell(rumors) > true_rumor_end)) {
			/* reached end of rumors -- go back to beginning */
			(void) dlb_fseek(rumors, beginning, SEEK_SET);
			(void) dlb_fgets(line, sizeof line, rumors);
		}
		if ((endp = index(line, '\n')) != 0) *endp = 0;
		Strcat(rumor_buf, xcrypt(line, xbuf));
	    } while(count++ < 50 && exclude_cookie &&
#ifndef JP
		    (strstri(rumor_buf, "fortune") || strstri(rumor_buf, "pity"))
#else
		    (strstri(rumor_buf, "占い") || strstri(rumor_buf, "なんてこった") ||
		     strstri(rumor_buf, "おみくじ"))
#endif /*JP*/
		   );
	    (void) dlb_fclose(rumors);
	    if (count >= 50)
		impossible("Can't find non-cookie rumor?");
	    else
		exercise(A_WIS, (adjtruth > 0));
	} else {
		pline("Can't open rumors file!");
		true_rumor_size = -1;	/* don't try to open it again */
	}
	return rumor_buf;
}

void
outrumor(truth, mechanism)
int truth; /* 1=true, -1=false, 0=either */
int mechanism;
{
	static const char fortune_msg[] =
		E_J("This cookie has a scrap of paper inside.",
		    "このクッキーには紙片が入っている。");
	const char *line;
	char buf[BUFSZ];
	boolean reading = (mechanism == BY_COOKIE ||
			   mechanism == BY_PAPER);

	if (reading) {
	    /* deal with various things that prevent reading */
	    if (is_fainted() && mechanism == BY_COOKIE)
	    	return;
	    else if (Blind) {
		if (mechanism == BY_COOKIE)
			pline(fortune_msg);
		pline(E_J("What a pity that you cannot read it!",
			  "なんてこった、あなたは読むことができない！"));
	    	return;
	    }
	}
	line = getrumor(truth, buf, reading ? FALSE : TRUE);
	if (!*line)
		line = E_J("NetHack rumors file closed for renovation.",
			   "NetHack噂ファイルは改装中です。");
	switch (mechanism) {
	    case BY_ORACLE:
	 	/* Oracle delivers the rumor */
#ifndef JP
		pline("True to her word, the Oracle %ssays: ",
		  (!rn2(4) ? "offhandedly " : (!rn2(3) ? "casually " :
		  (rn2(2) ? "nonchalantly " : ""))));
#else
		pline("契約に従い、巫女は%s告げた: ",
		  (!rn2(4) ? "無造作に" : (!rn2(3) ? "軽く" :
		  (rn2(2) ? "突き放すように" : ""))));
#endif /*JP*/
		verbalize("%s", line);
		exercise(A_WIS, TRUE);
		return;
	    case BY_COOKIE:
		pline(fortune_msg);
		/* FALLTHRU */
	    case BY_PAPER:
		pline(E_J("It reads:","こう書かれている:"));
		break;
	}
	pline("%s", line);
}

STATIC_OVL void
init_oracles(fp)
dlb *fp;
{
	register int i;
	char line[BUFSZ];
	int cnt = 0;

	/* this assumes we're only called once */
	(void) dlb_fgets(line, sizeof line, fp); /* skip "don't edit" comment*/
	(void) dlb_fgets(line, sizeof line, fp);
	if (sscanf(line, "%5d\n", &cnt) == 1 && cnt > 0) {
	    oracle_cnt = (unsigned) cnt;
	    oracle_loc = (long *) alloc((unsigned)cnt * sizeof (long));
	    for (i = 0; i < cnt; i++) {
		(void) dlb_fgets(line, sizeof line, fp);
		(void) sscanf(line, "%5lx\n", &oracle_loc[i]);
	    }
	}
	return;
}

void
save_oracles(fd, mode)
int fd, mode;
{
	if (perform_bwrite(mode)) {
	    bwrite(fd, (genericptr_t) &oracle_cnt, sizeof oracle_cnt);
	    if (oracle_cnt)
		bwrite(fd, (genericptr_t)oracle_loc, oracle_cnt*sizeof (long));
	}
	if (release_data(mode)) {
	    if (oracle_cnt) {
		free((genericptr_t)oracle_loc);
		oracle_loc = 0,  oracle_cnt = 0,  oracle_flg = 0;
	    }
	}
}

void
restore_oracles(fd)
int fd;
{
	mread(fd, (genericptr_t) &oracle_cnt, sizeof oracle_cnt);
	if (oracle_cnt) {
	    oracle_loc = (long *) alloc(oracle_cnt * sizeof (long));
	    mread(fd, (genericptr_t) oracle_loc, oracle_cnt * sizeof (long));
	    oracle_flg = 1;	/* no need to call init_oracles() */
	}
}

void
outoracle(special, delphi)
boolean special;
boolean delphi;
{
	char	line[COLNO];
	char	*endp;
	dlb	*oracles;
	int oracle_idx;
	char xbuf[BUFSZ];

	if(oracle_flg < 0 ||			/* couldn't open ORACLEFILE */
	   (oracle_flg > 0 && oracle_cnt == 0))	/* oracles already exhausted */
		return;

	oracles = dlb_fopen(ORACLEFILE, "r");

	if (oracles) {
		winid tmpwin;
		if (oracle_flg == 0) {	/* if this is the first outoracle() */
			init_oracles(oracles);
			oracle_flg = 1;
			if (oracle_cnt == 0) return;
		}
		/* oracle_loc[0] is the special oracle;		*/
		/* oracle_loc[1..oracle_cnt-1] are normal ones	*/
		if (oracle_cnt <= 1 && !special) return;  /*(shouldn't happen)*/
		oracle_idx = special ? 0 : rnd((int) oracle_cnt - 1);
		(void) dlb_fseek(oracles, oracle_loc[oracle_idx], SEEK_SET);
		if (!special) oracle_loc[oracle_idx] = oracle_loc[--oracle_cnt];

		tmpwin = create_nhwindow(NHW_TEXT);
		if (delphi)
#ifndef JP
		    putstr(tmpwin, 0, special ?
		          "The Oracle scornfully takes all your money and says:" :
		          "The Oracle meditates for a moment and then intones:");
#else
		    putstr(tmpwin, 0, special ?
		          "巫女はさげすむようにあなたの金をすべて取り上げ、言った:" :
		          "巫女はしばらく瞑想し、そして告げた:");
#endif /*JP*/
		else
		    putstr(tmpwin, 0, E_J("The message reads:","こう書いてある:"));
		putstr(tmpwin, 0, "");

		while(dlb_fgets(line, COLNO, oracles) && strcmp(line,"---\n")) {
			if ((endp = index(line, '\n')) != 0) *endp = 0;
			putstr(tmpwin, 0, xcrypt(line, xbuf));
		}
		display_nhwindow(tmpwin, TRUE);
		destroy_nhwindow(tmpwin);
		(void) dlb_fclose(oracles);
	} else {
		pline("Can't open oracles file!");
		oracle_flg = -1;	/* don't try to open it again */
	}
}

int
doconsult(oracl)
register struct monst *oracl;
{
	int u_pay, minor_cost = 10, major_cost = 100 /*+ 50 * u.ulevel*/;
	int add_xpts;
	char qbuf[QBUFSZ];

	multi = 0;

	if (!oracl) {
		There(E_J("is no one here to consult.",
			  "助言を求める者は誰もいない。"));
		return 0;
	} else if (!oracl->mpeaceful) {
		pline(E_J("%s is in no mood for consultations.",
			  "%sは神託を告げるような雰囲気ではない。"), Monnam(oracl));
		return 0;
	} else if (!u.ugold) {
		You(E_J("have no money.","金を持っていない。"));
		return 0;
	}

	Sprintf(qbuf,
		E_J("\"Wilt thou settle for a minor consultation?\" (%d %s)",
		    "「汝、低位の神託で満足とするか？」(%d %s)"),
		minor_cost, currency((long)minor_cost));
	switch (ynq(qbuf)) {
	    default:
	    case 'q':
		return 0;
	    case 'y':
		if (u.ugold < (long)minor_cost) {
		    You(E_J("don't even have enough money for that!",
			    "それに払えるだけの金すら持っていない！"));
		    return 0;
		}
		u_pay = minor_cost;
		break;
	    case 'n':
		if (u.ugold <= (long)minor_cost ||	/* don't even ask */
		    (oracle_cnt == 1 || oracle_flg < 0)) return 0;
		Sprintf(qbuf,
			E_J("\"Then dost thou desire a major one?\" (%d %s)",
			    "「ならば、高位の神託を求むるか？」(%d %s)"),
			major_cost, currency((long)major_cost));
		if (yn(qbuf) != 'y') return 0;
		u_pay = (u.ugold < (long)major_cost ? (int)u.ugold
						    : major_cost);
		break;
	}
	u.ugold -= (long)u_pay;
	oracl->mgold += (long)u_pay;
	flags.botl = 1;
	add_xpts = 0;	/* first oracle of each type gives experience points */
	if (u_pay == minor_cost) {
		outrumor(1, BY_ORACLE);
		if (!u.uevent.minor_oracle)
		    add_xpts = u_pay / (u.uevent.major_oracle ? 25 : 10);
		    /* 5 pts if very 1st, or 2 pts if major already done */
		u.uevent.minor_oracle = TRUE;
	} else {
		boolean cheapskate = u_pay < major_cost;
		outoracle(cheapskate, TRUE);
		if (!cheapskate && !u.uevent.major_oracle)
		    add_xpts = u_pay / (u.uevent.minor_oracle ? 25 : 10);
		    /* ~100 pts if very 1st, ~40 pts if minor already done */
		u.uevent.major_oracle = TRUE;
		exercise(A_WIS, !cheapskate);
	}
	if (add_xpts) {
		more_experienced(add_xpts, u_pay/50);
		newexplevel();
	}
	return 1;
}

/*rumors.c*/
