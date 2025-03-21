/*	SCCS Id: @(#)allmain.c	3.4	2003/04/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* various code that was replicated in *main.c */

#include "hack.h"

#ifndef NO_SIGNAL
#include <signal.h>
#endif

STATIC_DCL void NDECL(once_per_turn_things);
STATIC_DCL void NDECL(once_per_player_input_things);

#ifdef POSITIONBAR
STATIC_DCL void NDECL(do_positionbar);
#endif

#ifdef OVL0

int wtcap = 0;
boolean didmove = FALSE;

void
moveloop()
{
    int moveamt = 0;
    boolean monscanmove = FALSE;

    flags.moonphase = phase_of_the_moon();
    if(flags.moonphase == FULL_MOON) {
	You(E_J("are lucky!  Full moon tonight.",
		"運がいい！ 今夜は満月だ。"));
	change_luck(1);
    } else if(flags.moonphase == NEW_MOON) {
	pline(E_J("Be careful!  New moon tonight.",
		  "注意せよ！ 今夜は新月だ。"));
    }
    flags.friday13 = friday_13th();
    if (flags.friday13) {
	pline(E_J("Watch out!  Bad things can happen on Friday the 13th.",
		  "気をつけろ！ 13日の金曜日には良くないことが起きる。"));
	change_luck(-1);
    }

    initrack();


    /* Note:  these initializers don't do anything except guarantee that
	    we're linked properly.
    */
    decl_init();
    monst_init();
    monstr_init();	/* monster strengths */
    objects_init();

#ifdef WIZARD
    if (wizard) add_debug_extended_commands();
#endif

    (void) encumber_msg(); /* in case they auto-picked up something */

    u.uz0.dlevel = u.uz.dlevel;
    youmonst.movement = NORMAL_SPEED;	/* give the hero some movement points */

    for(;;) {
	get_nh_event();
#ifdef POSITIONBAR
	do_positionbar();
#endif

	didmove = flags.move;
	if(didmove) {
	    /* actual time passed */
	    youmonst.movement -= NORMAL_SPEED;

	    do { /* hero can't move this turn loop */
		wtcap = encumber_msg();

		flags.mon_moving = TRUE;
		do {
		    monscanmove = movemon();
		    if (youmonst.movement > NORMAL_SPEED)
			break;	/* it's now your turn */
		} while (monscanmove);
		flags.mon_moving = FALSE;

		if (!monscanmove && youmonst.movement < NORMAL_SPEED) {
		    /* both you and the monsters are out of steam this round */
		    /* set up for a new turn */
		    struct monst *mtmp;
		    mcalcdistress();	/* adjust monsters' trap, blind, etc */

		    /* reallocate movement rations to monsters */
		    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			mtmp->movement += mcalcmove(mtmp);

		    if(!level.flags.nomongen &&
			!rn2(u.uevent.udemigod ? 25 :
			    (depth(&u.uz) > depth(&stronghold_level)) ? 50 : 70))
			(void) makemon((struct permonst *)0, 0, 0, MM_MONSTEEDOK/*NO_MM_FLAGS*/);

		    /* calculate how much time passed. */
#ifdef STEED
		    if (u.usteed && u.umoved) {
			/* your speed doesn't augment steed's speed */
			moveamt = mcalcmove(u.usteed);
		    } else
#endif
		    {
			moveamt = youmonst.data->mmove;

			if (Very_fast) {	/* wand or potion */
			    /* average movement is 1.67 times normal (=20) */
			    moveamt += NORMAL_SPEED / 2;
			    if (rn2(3) == 0) moveamt += NORMAL_SPEED / 2;
			    /* no bonus by speed boots */
			} else if (Fast) {
			    /* average movement is 1.33 times normal (=16) */
			    if (rn2(3) != 0) moveamt += NORMAL_SPEED / 2;
			    /* speed boots (+2 bonus) */
			    if (BFast) moveamt += NORMAL_SPEED / 6;
			} else if (BFast) {
			    /* if normal movement with speed boots, same as Fast */
			    if (rn2(3) != 0) moveamt += NORMAL_SPEED / 2;
			}
			moveamt += speed_bonus();
		    }

		    switch (wtcap) {
			case UNENCUMBERED: break;
			case SLT_ENCUMBER: moveamt -= (moveamt / 4); break;
			case MOD_ENCUMBER: moveamt -= (moveamt / 2); break;
			case HVY_ENCUMBER: moveamt -= ((moveamt * 3) / 4); break;
			case EXT_ENCUMBER: moveamt -= ((moveamt * 7) / 8); break;
			default: break;
		    }

		    youmonst.movement += moveamt;
		    if (youmonst.movement < 0) youmonst.movement = 0;
		    settrack();

		    monstermoves++;
		    moves++;

		    /********************************/
		    /* once-per-turn things go here */
		    /********************************/
		    once_per_turn_things();

		}
	    } while (youmonst.movement<NORMAL_SPEED); /* hero can't move loop */

	    /******************************************/
	    /* once-per-hero-took-time things go here */
	    /******************************************/


	} /* actual time passed */

	/****************************************/
	/* once-per-player-input things go here */
	/****************************************/
	once_per_player_input_things();

    }
}

/********************************/
/* once-per-turn things go here */
/********************************/
STATIC_DCL void
once_per_turn_things()
{
	int change = 0;

	if (flags.bypasses) clear_bypasses();
	if(Glib) glibr();
	nh_timeout();
	run_regions();
	run_cloud();

	if (u.ublesscnt)  u.ublesscnt--;
	if (u.urest)      u.urest--;
	if(flags.time && !flags.run)
	    flags.botl = 1;

	/* One possible result of prayer is healing.  Whether or
	 * not you get healed depends on your current hit points.
	 * If you are allowed to regenerate during the prayer, the
	 * end-of-prayer calculation messes up on this.
	 * Another possible result is rehumanization, which requires
	 * that encumbrance and movement rate be recalculated.
	 */
	if (u.uinvulnerable) {
	    /* for the moment at least, you're in tiptop shape */
	    wtcap = UNENCUMBERED;
	} else if (Upolyd && youmonst.data->mlet == S_EEL && !is_pool(u.ux,u.uy) && !Is_waterlevel(&u.uz)) {
	    if (u.mh > 1) {
		u.mh--;
		flags.botl = 1;
	    } else if (u.mh < 1)
		rehumanize();
	} else if (Upolyd && u.mh < u.mhmax) {
	    int hcnt = moves % 20;
	    /* if you rest, heal faster */
	    if (hcnt == 10 && u.urest >= 20) {
		hcnt = 0;	u.urest = 0;
	    }
	    if (u.mh < 1)
		rehumanize();
	    else if (Regeneration ||
			(wtcap < MOD_ENCUMBER && !hcnt)) {
		flags.botl = 1;
		u.mh++;
	    }
	} else if (u.uhp < u.uhpmax &&
	     (wtcap < MOD_ENCUMBER || !u.umoved || Regeneration)) {
	    int heal = 0;
	    int hcnt, hspn;
	    hspn = (u.ulevel > 9) ? 12/*3*/ : (16-u.ulevel/2);
	    hcnt = moves % hspn;
	    /* if you rest, heal faster */
	    if (hcnt == (hspn/2) && u.urest >= hspn) {
		hcnt = 0;	u.urest = 0;
	    }
	    if (u.ulevel > 9 && !hcnt) {
		int Con = (int) ACURR(A_CON);

		if (Con <= 12) {
		    heal++;
		} else {
		    heal = rnd(Con/3);
		    if (heal > u.ulevel-9) heal = u.ulevel-9;
		}
	    } else if (!hcnt)
		    heal++;
		  /* formerly: ((MAXULEV+12) / (u.ulevel+2) + 1) */
	    if (Regeneration) heal++;
	    if (heal) {
		u.uhp += heal;
		if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
		flags.botl = 1;
	    }
	}

	/* moving around while encumbered is hard work */
	if (wtcap > MOD_ENCUMBER && u.umoved) {
	    if(!(wtcap < EXT_ENCUMBER ? moves%30 : moves%10)) {
		if (Upolyd && u.mh > 1) {
		    u.mh--;
		} else if (!Upolyd && u.uhp > 1) {
		    u.uhp--;
		} else {
		    You(E_J("pass out from exertion!","疲労で意識を失ってしまった！"));
		    exercise(A_CON, FALSE);
		    fall_asleep(-10, FALSE);
		}
	    }
	}

	if ((u.uen < u.uenmax) &&
	    ((wtcap < MOD_ENCUMBER &&
	      (!(moves%((MAXULEV + 8 - u.ulevel) *
			(Role_if(PM_WIZARD) ? 3 : 4) / 6))))
	     || Energy_regeneration)) {
	    u.uen += rn1((int)(ACURR(A_WIS) + ACURR(A_INT)) / 15 + 1,1);
	    if (u.uen > u.uenmax)  u.uen = u.uenmax;
	    flags.botl = 1;
	}

	if(!u.uinvulnerable) {
	    if(Teleportation && !rn2(85)) {
		xchar old_ux = u.ux, old_uy = u.uy;
		tele();
		if (u.ux != old_ux || u.uy != old_uy) {
		    if (!next_to_u()) {
			check_leash(old_ux, old_uy);
		    }
#ifdef REDO
		    /* clear doagain keystrokes */
		    pushch(0);
		    savech(0);
#endif
		}
	    }
	    /* delayed change may not be valid anymore */
	    if ((change == 1 && !Polymorph) ||
		(change == 2 && u.ulycn == NON_PM))
		change = 0;
	    if(Polymorph && !rn2(100))
		change = 1;
	    else if (u.ulycn >= LOW_PM && !Upolyd &&
		     !rn2(80 - (20 * night())))
		change = 2;
	    if (change && !Unchanging) {
		if (multi >= 0) {
		    if (occupation)
			stop_occupation();
		    else
			nomul(0);
		    if (change == 1) polyself(FALSE);
		    else you_were();
		    change = 0;
		}
	    }
	    /* uncontrolled levitation */
	    if (BLevAtWill && !Levitation && !rn2(100))
		levitation_on(0L, FALSE);
	}

	if(Searching && multi >= 0) (void) dosearch0(1);
	dosounds();
	do_storms();
	gethungry();
	age_spells();
	exerchk();
	invault();
	if (u.uhave.amulet) amulet();
	if (!rn2(40+(int)(ACURR(A_DEX)*3)))
	    u_wipe_engr(rnd(3));
	if (u.uevent.udemigod && !u.uinvulnerable) {
	    if (u.udg_cnt) u.udg_cnt--;
	    if (!u.udg_cnt) {
		intervene();
		u.udg_cnt = rn1(200, 50);
	    }
	}
	if (uarm && (uarm->otyp == CHROMATIC_DRAGON_SCALES ||
		     uarm->otyp == CHROMATIC_DRAGON_SCALE_MAIL)) {
	    if (uarm->age) uarm->age--;
	    if (!uarm->age) {
		uarm->age = rn1(200, 50) / (uarm->recharged + 1);
		curse_of_chromatic_dragon(uarm);
	    }
	}
	restore_attrib();
	/* underwater and waterlevel vision are done here */
	if (Is_waterlevel(&u.uz))
	    movebubbles();
	else if (Underwater)
	    under_water(0);
	/* vision while buried done here */
	else if (u.uburied) under_ground(0);

	/* when immobile, count is in turns */
	if(multi < 0) {
	    if (++multi == 0) {	/* finished yet? */
		unmul((char *)0);
		/* if unmul caused a level change, take it now */
		if (u.utotype) deferred_goto();
	    }
	}
}

/****************************************/
/* once-per-player-input things go here */
/****************************************/
STATIC_DCL void
once_per_player_input_things()
{
#if defined(MICRO) || defined(WIN32)
    char ch;
    int abort_lev;
#endif

	find_ac();
	if(!flags.mv || Blind) {
	    /* redo monsters if hallu or wearing a helm of telepathy */
	    if (Hallucination) {	/* update screen randomly */
		see_monsters();
		see_objects();
		see_traps();
		if (u.uswallow) swallowed(0);
	    } else if (Unblind_telepat) {
		see_monsters();
	    } else if (Warning || Warn_of_mon)
	     	see_monsters();

	    if (vision_full_recalc) vision_recalc(0);	/* vision! */
	}
	if(flags.botl || flags.botlx) bot();

	flags.move = 1;

	if(multi >= 0 && occupation) {
#if defined(MICRO) || defined(WIN32)
	    abort_lev = 0;
	    if (kbhit()) {
		if ((ch = Getchar()) == ABORT)
		    abort_lev++;
# ifdef REDO
		else
		    pushch(ch);
# endif /* REDO */
	    }
	    if (!abort_lev && (*occupation)() == 0)
#else
	    if ((*occupation)() == 0)
#endif
		occupation = 0;
	    if(
#if defined(MICRO) || defined(WIN32)
		   abort_lev ||
#endif
		   monster_nearby()) {
		stop_occupation();
		reset_eat();
	    }
#if defined(MICRO) || defined(WIN32)
	    if (!(++occtime % 7))
		display_nhwindow(WIN_MAP, FALSE);
#endif
	    return;
	}

	if ((u.uhave.amulet || Clairvoyant) &&
	    !In_endgame(&u.uz) && !BClairvoyant &&
	    !(moves % 15) && !rn2(2))
		do_vicinity_map();

	if(u.utrap && u.utraptype == TT_LAVA) {
	    if(!is_lava(u.ux,u.uy))
		u.utrap = 0;
	    else if (!u.uinvulnerable) {
		u.utrap -= (1<<8);
		if(u.utrap < (1<<8)) {
		    killer_format = KILLED_BY;
		    killer = E_J("molten lava","煮えたぎる溶岩に沈み");
		    You(E_J("sink below the surface and die.",
			    "溶岩の表面下に沈み、死んだ。"));
		    done(DISSOLVED);
		} else if(didmove && !u.umoved) {
		    Norep(E_J("You sink deeper into the lava.",
			      "あなたは溶岩の中に沈み込んでいく。"));
		    u.utrap += rnd(4);
		}
	    }
	}

#ifdef WIZARD
	if (iflags.sanity_check)
	    sanity_check();
#endif

#ifdef CLIPPING
	/* just before rhack */
	cliparound(u.ux, u.uy);
#endif

	u.umoved = FALSE;

	if (multi > 0) {
	    lookaround();
	    if (!multi) {
		/* lookaround may clear multi */
		flags.move = 0;
		if (flags.time) flags.botl = 1;
		return;
	    }
	    if (flags.mv) {
		if(multi < COLNO && !--multi)
		    flags.travel = iflags.travel1 = flags.mv = flags.run = 0;
		domove();
	    } else {
		--multi;
		rhack(save_cm);
	    }
	} else if (multi == 0) {
#ifdef MAIL
	    ckmailstatus();
#endif
	    rhack((char *)0);
	}
	if (u.utotype)		/* change dungeon level */
	    deferred_goto();	/* after rhack() */
	/* !flags.move here: multiple movement command stopped */
	else if (flags.time && (!flags.move || !flags.mv))
	    flags.botl = 1;

	if (vision_full_recalc) vision_recalc(0);	/* vision! */
	/* when running in non-tport mode, this gets done through domove() */
	if ((!flags.run || iflags.runmode == RUN_TPORT) &&
		(multi && (!flags.travel ? !(multi % 7) : !(moves % 7L)))) {
	    if (flags.time && flags.run) flags.botl = 1;
	    display_nhwindow(WIN_MAP, FALSE);
	}
}

int
speed_bonus()
{
	int sbon;

	/* speed bonus by light-weight */
	sbon = u.uspdbon1;

	/* rocket skate */
	if (Rocketskate) {
		sbon += 10 + ((Very_fast) ? 0 : (Fast) ? 4 : 8);
	}
	return sbon;
}

#endif /* OVL0 */
#ifdef OVL1

void
stop_occupation()
{
	if(occupation) {
		if (!maybe_finished_meal(TRUE))
		    You(E_J("stop %s.","%sを中断した。"), occtxt);
		occupation = 0;
		flags.botl = 1; /* in case u.uhs changed */
/* fainting stops your occupation, there's no reason to sync.
		sync_hunger();
*/
#ifdef REDO
		nomul(0);
		pushch(0);
#endif
	}
}

#endif /* OVL1 */
#ifdef OVLB

void
display_gamewindows()
{
    WIN_MESSAGE = create_nhwindow(NHW_MESSAGE);
    WIN_STATUS = create_nhwindow(NHW_STATUS);
    WIN_MAP = create_nhwindow(NHW_MAP);
    WIN_INVEN = create_nhwindow(NHW_MENU);
//    WIN_HPBAR = create_nhwindow(NHW_HPBAR);

#ifdef MAC
    /*
     * This _is_ the right place for this - maybe we will
     * have to split display_gamewindows into create_gamewindows
     * and show_gamewindows to get rid of this ifdef...
     */
	if ( ! strcmp ( windowprocs . name , "mac" ) ) {
	    SanePositions ( ) ;
	}
#endif

    /*
     * The mac port is not DEPENDENT on the order of these
     * displays, but it looks a lot better this way...
     */
    display_nhwindow(WIN_STATUS, FALSE);
    display_nhwindow(WIN_MESSAGE, FALSE);
    clear_glyph_buffer();
    display_nhwindow(WIN_MAP, FALSE);
}

void
newgame()
{
	int i;

#ifdef MFLOPPY
	gameDiskPrompt();
#endif

	flags.ident = 1;

	for (i = 0; i < NUMMONS; i++)
		mvitals[i].mvflags = mons[i].geno & G_NOCORPSE;

	init_objects();		/* must be before u_init() */

	flags.pantheon = -1;	/* role_init() will reset this */
	role_init();		/* must be before init_dungeons(), u_init(),
				 * and init_artifacts() */

	init_dungeons();	/* must be before u_init() to avoid rndmonst()
				 * creating odd monsters for any tins and eggs
				 * in hero's initial inventory */
	init_artifacts();	/* before u_init() in case $WIZKIT specifies
				 * any artifacts */
	init_rndvault();
	u_init();

#ifndef NO_SIGNAL
	(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
#ifdef NEWS
	if(iflags.news) display_file(NEWS, FALSE);
#endif
	load_qtlist();	/* load up the quest text info */
/*	quest_init();*/	/* Now part of role_init() */

	mklev();
	u_on_upstairs();
	vision_reset();		/* set up internals for level (after mklev) */
	check_special_room(FALSE);

	flags.botlx = 1;

	/* Move the monster from under you or else
	 * makedog() will fail when it calls makemon().
	 *			- ucsfcgl!kneller
	 */
	if(MON_AT(u.ux, u.uy)) mnexto(m_at(u.ux, u.uy));
	(void) makedog();
	docrt();

	if (flags.legacy) {
		flush_screen(1);
		com_pager(1);
	}

#ifdef INSURANCE
	save_currentstate();
#endif
	program_state.something_worth_saving++;	/* useful data now exists */

	/* Success! */
	welcome(TRUE);
	return;
}

/* show "welcome [back] to nethack" message at program startup */
void
welcome(new_game)
boolean new_game;	/* false => restoring an old game */
{
    char buf[BUFSZ];
    boolean currentgend = Upolyd ? u.mfemale : flags.female;

    /*
     * The "welcome back" message always describes your innate form
     * even when polymorphed or wearing a helm of opposite alignment.
     * Alignment is shown unconditionally for new games; for restores
     * it's only shown if it has changed from its original value.
     * Sex is shown for new games except when it is redundant; for
     * restores it's only shown if different from its original value.
     */
    *buf = '\0';
    if (new_game || u.ualignbase[A_ORIGINAL] != u.ualignbase[A_CURRENT])
	Sprintf(eos(buf), E_J(" %s","%sの"), align_str(u.ualignbase[A_ORIGINAL]));
    if (!urole.name.f &&
	    (new_game ? (urole.allow & ROLE_GENDMASK) == (ROLE_MALE|ROLE_FEMALE) :
	     currentgend != flags.initgend))
	Sprintf(eos(buf), E_J(" %s","%sで"), genders[currentgend].adj);

#ifndef JP
    pline(new_game ? "%s %s, welcome to NetHack!  You are a%s %s %s."
		   : "%s %s, the%s %s %s, welcome back to NetHack!",
	  Hello((struct monst *) 0), plname, buf, urace.adj,
	  (currentgend && urole.name.f) ? urole.name.f : urole.name.m);
#else
    /* Hello() のバリエーションが無いので、呼び出し側で対応… */
    if (new_game) {
	char tmp[BUFSZ];
	switch (Role_switch) {
	  case PM_KNIGHT:
	  case PM_SAMURAI:
	    Sprintf(tmp, "%sよ、NetHackへよくぞ参られた", plname);
	    break;
	  default:
	    Sprintf(tmp, "%s%s、NetHackへようこそ",
		    Hello((struct monst *) 0), plname);
	    break;
	}
	pline("%s！ あなたは%s%s%sだ。", tmp, buf, urace.adj,
		(currentgend && urole.name.f) ? urole.name.f : urole.name.m);
    } else {
	char tmp[BUFSZ];
	switch (Role_switch) {
	  case PM_KNIGHT:
	  case PM_SAMURAI:
	    Strcpy(tmp, "%s、%s%s%sよ、NetHackへよくぞ戻られた！");
	    break;
	  default:
	    Strcpy(tmp, Hello((struct monst *) 0));
	    Strcat(tmp, "%s、%s%s%s、NetHackへお帰りなさい！");
	    break;
	}
	pline(tmp, plname, buf, urace.adj,
		(currentgend && urole.name.f) ? urole.name.f : urole.name.m);
    }
#endif /*JP*/
}

#ifdef POSITIONBAR
STATIC_DCL void
do_positionbar()
{
	static char pbar[COLNO];
	char *p;
	
	p = pbar;
	/* up stairway */
	if (upstair.sx &&
	   (glyph_to_cmap(level.locations[upstair.sx][upstair.sy].glyph) ==
	    S_upstair ||
 	    glyph_to_cmap(level.locations[upstair.sx][upstair.sy].glyph) ==
	    S_upladder)) {
		*p++ = '<';
		*p++ = upstair.sx;
	}
	if (sstairs.sx &&
	   (glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_upstair ||
 	    glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_upladder)) {
		*p++ = '<';
		*p++ = sstairs.sx;
	}

	/* down stairway */
	if (dnstair.sx &&
	   (glyph_to_cmap(level.locations[dnstair.sx][dnstair.sy].glyph) ==
	    S_dnstair ||
 	    glyph_to_cmap(level.locations[dnstair.sx][dnstair.sy].glyph) ==
	    S_dnladder)) {
		*p++ = '>';
		*p++ = dnstair.sx;
	}
	if (sstairs.sx &&
	   (glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_dnstair ||
 	    glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_dnladder)) {
		*p++ = '>';
		*p++ = sstairs.sx;
	}

	/* hero location */
	if (u.ux) {
		*p++ = '@';
		*p++ = u.ux;
	}
	/* fence post */
	*p = 0;

	update_positionbar(pbar);
}
#endif

#endif /* OVLB */

/*allmain.c*/
