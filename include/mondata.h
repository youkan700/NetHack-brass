/*	SCCS Id: @(#)mondata.h	3.4	2003/01/08	*/
/* Copyright (c) 1989 Mike Threepoint				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONDATA_H
#define MONDATA_H

#define monsndx(ptr)		((int)(ptr)->mnum)

#define verysmall(ptr)		((ptr)->msize < MZ_SMALL)
#define bigmonst(ptr)		((ptr)->msize >= MZ_LARGE)

#define pm_resistance(ptr,typ)	(((ptr)->mresists & (typ)) != 0)

#define resists_fire(mon)	(((mon)->mintrinsics & MR_FIRE) != 0)
#define resists_cold(mon)	(((mon)->mintrinsics & MR_COLD) != 0)
#define resists_sleep(mon)	(((mon)->mintrinsics & MR_SLEEP) != 0)
#define resists_disint(mon)	(((mon)->mintrinsics & MR_DISINT) != 0)
#define resists_elec(mon)	(((mon)->mintrinsics & MR_ELEC) != 0)
#define resists_poison(mon)	(((mon)->mintrinsics & MR_POISON) != 0)
#define resists_acid(mon)	(((mon)->mintrinsics & MR_ACID) != 0)
#define resists_ston(mon)	(((mon)->mintrinsics & MR_STONE) != 0)
#define resists_paraly(mon)	(((mon)->data == &mons[PM_ORANGE_DRAGON]) || \
				((mon)->data == &mons[PM_BABY_ORANGE_DRAGON]))

#define is_lminion(mon)		(is_minion((mon)->data) && \
				 (mon)->data->maligntyp >= A_COALIGNED && \
				 ((mon)->data != &mons[PM_ANGEL] || \
				  getmaligntyp(mon) == A_LAWFUL))

#define is_flyer(ptr)		(((ptr)->mflags1 & M1_FLY) != 0L)
#define is_floater(ptr)		((ptr)->mlet == S_EYE)
#define is_clinger(ptr)		(((ptr)->mflags1 & M1_CLING) != 0L)
#define is_swimmer(ptr)		(((ptr)->mflags1 & M1_SWIM) != 0L)
#define breathless(ptr)		(((ptr)->mflags1 & M1_BREATHLESS) != 0L)
#define amphibious(ptr)		(((ptr)->mflags1 & (M1_AMPHIBIOUS | M1_BREATHLESS)) != 0L)
#define passes_walls(ptr)	(((ptr)->mflags1 & M1_WALLWALK) != 0L)
#define amorphous(ptr)		(((ptr)->mflags1 & M1_AMORPHOUS) != 0L)
#define noncorporeal(ptr)	((ptr)->mlet == S_GHOST)
#define tunnels(ptr)		(((ptr)->mflags1 & M1_TUNNEL) != 0L)
#define needspick(ptr)		(((ptr)->mflags1 & M1_NEEDPICK) != 0L)
#define hides_under(ptr)	(((ptr)->mflags1 & M1_CONCEAL) != 0L)
#define is_hider(ptr)		(((ptr)->mflags1 & M1_HIDE) != 0L)
#define haseyes(ptr)		(((ptr)->mflags1 & M1_NOEYES) == 0L)
#define eyecount(ptr)		(!haseyes(ptr) ? 0 : \
				 ((ptr)->mnum == PM_CYCLOPS || \
				  (ptr)->mnum == PM_FLOATING_EYE) ? 1 : 2)
#define nohands(ptr)		(((ptr)->mflags1 & M1_NOHANDS) != 0L)
#define nolimbs(ptr)		(((ptr)->mflags1 & M1_NOLIMBS) == M1_NOLIMBS)
#define notake(ptr)		(((ptr)->mflags1 & M1_NOTAKE) != 0L)
#define has_head(ptr)		(((ptr)->mflags1 & M1_NOHEAD) == 0L)
#define has_horns(ptr)		(num_horns(ptr) > 0)
#define is_whirly(ptr)		((ptr)->mlet == S_VORTEX || \
				 (ptr)->mnum == PM_AIR_ELEMENTAL)
#define flaming(ptr)		((ptr)->mnum == PM_FIRE_VORTEX || \
				 (ptr)->mnum == PM_FLAMING_SPHERE || \
				 (ptr)->mnum == PM_FIRE_ELEMENTAL || \
				 (ptr)->mnum == PM_SALAMANDER)
#define is_silent(ptr)		((ptr)->msound == MS_SILENT)
#define unsolid(ptr)		(((ptr)->mflags1 & M1_UNSOLID) != 0L)
#define mindless(ptr)		(((ptr)->mflags1 & M1_MINDLESS) != 0L)
#define humanoid(ptr)		(((ptr)->mflags1 & M1_HUMANOID) != 0L)
#define is_animal(ptr)		(((ptr)->mflags1 & M1_ANIMAL) != 0L)
#define slithy(ptr)		(((ptr)->mflags1 & M1_SLITHY) != 0L)
#define is_wooden(ptr)		((ptr)->mnum == PM_WOOD_GOLEM)
#define thick_skinned(ptr)	(((ptr)->mflags1 & M1_THICK_HIDE) != 0L)
#define lays_eggs(ptr)		(((ptr)->mflags1 & M1_OVIPAROUS) != 0L)
#define regenerates(ptr)	(((ptr)->mflags1 & M1_REGEN) != 0L)
#define perceives(ptr)		(((ptr)->mflags1 & M1_SEE_INVIS) != 0L)
#define can_teleport(ptr)	(((ptr)->mflags1 & M1_TPORT) != 0L)
#define control_teleport(ptr)	(((ptr)->mflags1 & M1_TPORT_CNTRL) != 0L)
#define telepathic(ptr)		((ptr)->mnum == PM_FLOATING_EYE || \
				 (ptr)->mnum == PM_MIND_FLAYER || \
				 (ptr)->mnum == PM_MASTER_MIND_FLAYER)
#define is_armed(ptr)		attacktype(ptr, AT_WEAP)
#define acidic(ptr)		(((ptr)->mflags1 & M1_ACID) != 0L)
#define poisonous(ptr)		(((ptr)->mflags1 & M1_POIS) != 0L)
#define carnivorous(ptr)	(((ptr)->mflags1 & M1_CARNIVORE) != 0L)
#define herbivorous(ptr)	(((ptr)->mflags1 & M1_HERBIVORE) != 0L)
#define metallivorous(ptr)	(((ptr)->mflags1 & M1_METALLIVORE) != 0L)
#define polyok(ptr)		(((ptr)->mflags2 & M2_NOPOLY) == 0L)
#define is_undead(ptr)		(((ptr)->mflags2 & M2_UNDEAD) != 0L)
#define is_were(ptr)		(((ptr)->mflags2 & M2_WERE) != 0L)
#define is_elf(ptr)		(((ptr)->mflags2 & M2_ELF) != 0L)
#define is_dwarf(ptr)		(((ptr)->mflags2 & M2_DWARF) != 0L)
#define is_gnome(ptr)		(((ptr)->mflags2 & M2_GNOME) != 0L)
#define is_orc(ptr)		(((ptr)->mflags2 & M2_ORC) != 0L)
#define is_human(ptr)		(((ptr)->mflags2 & M2_HUMAN) != 0L)
#define your_race(ptr)		(((ptr)->mflags2 & urace.selfmask) != 0L)
#define is_bat(ptr)		((ptr)->mnum == PM_BAT || \
				 (ptr)->mnum == PM_GIANT_BAT || \
				 (ptr)->mnum == PM_VAMPIRE_BAT)
#define is_bird(ptr)		((ptr)->mlet == S_BAT && !is_bat(ptr))
#define is_giant(ptr)		(((ptr)->mflags2 & M2_GIANT) != 0L)
#define is_golem(ptr)		((ptr)->mlet == S_GOLEM)
#define is_domestic(ptr)	(((ptr)->mflags2 & M2_DOMESTIC) != 0L)
#define is_demon(ptr)		(((ptr)->mflags2 & M2_DEMON) != 0L)
#define is_mercenary(ptr)	(((ptr)->mflags2 & M2_MERC) != 0L)
#define is_male(ptr)		(((ptr)->mflags2 & M2_SEX) == M2_MALE)
#define is_female(ptr)		(((ptr)->mflags2 & M2_SEX) == M2_FEMALE)
#define is_neuter(ptr)		(((ptr)->mflags2 & M2_SEX) == M2_NEUTER)
#define is_wanderer(ptr)	(((ptr)->mflags2 & M2_WANDER) != 0L)
#define always_hostile(ptr)	(((ptr)->mflags2 & M2_HOSTILE) != 0L)
#define always_peaceful(ptr)	(((ptr)->mflags2 & M2_PEACEFUL) != 0L)
#define race_hostile(ptr)	(((ptr)->mflags2 & urace.hatemask) != 0L)
#define race_peaceful(ptr)	(((ptr)->mflags2 & urace.lovemask) != 0L)
#define extra_nasty(ptr)	(((ptr)->mflags2 & M2_NASTY) != 0L)
#define strongmonst(ptr)	(((ptr)->mflags2 & M2_STRONG) != 0L)
#define can_breathe(ptr)	attacktype(ptr, AT_BREA)
#define cantwield(ptr)		(nohands(ptr) || verysmall(ptr))
#define could_twoweap(ptr)	((ptr)->mattk[1].aatyp == AT_WEAP)
#define cantweararm(ptr)	(breakarm(ptr) || sliparm(ptr))
#define throws_rocks(ptr)	(((ptr)->mflags2 & M2_ROCKTHROW) != 0L)
#define type_is_pname(ptr)	(((ptr)->mflags2 & M2_PNAME) != 0L)
#define is_lord(ptr)		(((ptr)->mflags2 & M2_LORD) != 0L)
#define is_prince(ptr)		(((ptr)->mflags2 & M2_PRINCE) != 0L)
#define is_ndemon(ptr)		(is_demon(ptr) && \
				 (((ptr)->mflags2 & (M2_LORD|M2_PRINCE)) == 0L))
#define is_dlord(ptr)		(is_demon(ptr) && is_lord(ptr))
#define is_dprince(ptr)		(is_demon(ptr) && is_prince(ptr))
#define is_minion(ptr)		((ptr)->mflags2 & M2_MINION)
#define likes_gold(ptr)		(((ptr)->mflags2 & M2_GREEDY) != 0L)
#define likes_gems(ptr)		(((ptr)->mflags2 & M2_JEWELS) != 0L)
#define likes_objs(ptr)		(((ptr)->mflags2 & M2_COLLECT) != 0L || \
				 is_armed(ptr))
#define likes_magic(ptr)	(((ptr)->mflags2 & M2_MAGIC) != 0L)
#define webmaker(ptr)		((ptr)->mnum == PM_CAVE_SPIDER || \
				 (ptr)->mnum == PM_GIANT_SPIDER)
#define is_unicorn(ptr)		((ptr)->mlet == S_UNICORN && likes_gems(ptr))
#define is_longworm(ptr)	(((ptr)->mnum == PM_BABY_LONG_WORM) || \
				 ((ptr)->mnum == PM_LONG_WORM) || \
				 ((ptr)->mnum == PM_LONG_WORM_TAIL))
#define is_covetous(ptr)	((ptr->mflags3 & M3_COVETOUS))
#define infravision(ptr)	((ptr->mflags3 & M3_INFRAVISION))
#define infravisible(ptr)	((ptr->mflags3 & M3_INFRAVISIBLE))
#define is_mplayer(ptr)		(((ptr)->mnum >= PM_ARCHEOLOGIST) && \
				 ((ptr)->mnum <= PM_WIZARD))
#define is_rider(ptr)		((ptr)->mnum == PM_DEATH || \
				 (ptr)->mnum == PM_FAMINE || \
				 (ptr)->mnum == PM_PESTILENCE)
#define is_placeholder(ptr)	((ptr)->mnum == PM_ORC || \
				 (ptr)->mnum == PM_GIANT || \
				 (ptr)->mnum == PM_ELF || \
				 (ptr)->mnum == PM_HUMAN)
/* return TRUE if the monster tends to revive */
#define is_reviver(ptr)		(is_rider(ptr) || (ptr)->mlet == S_TROLL)
#define is_animobj(ptr)		((ptr)->mnum == PM_ANIMATED_OBJECT)

/* this returns the light's range, or 0 if none; if we add more light emitting
   monsters, we'll likely have to add a new light range field to mons[] */
#define emits_light(ptr)	(((ptr)->mlet == S_LIGHT || \
				  (ptr)->mnum == PM_FLAMING_SPHERE || \
				  (ptr)->mnum == PM_SHOCKING_SPHERE || \
				  (ptr)->mnum == PM_FIRE_VORTEX) ? 1 : \
				 ((ptr)->mnum == PM_FIRE_ELEMENTAL) ? 1 : 0)
/*	[note: the light ranges above were reduced to 1 for performance...] */
#define likes_lava(ptr)		((ptr)->mnum == PM_FIRE_ELEMENTAL || \
				 (ptr)->mnum == PM_SALAMANDER)
#define pm_invisible(ptr) ((ptr)->mnum == PM_STALKER || \
			   (ptr)->mnum == PM_BLACK_LIGHT)

/* could probably add more */
#define likes_fire(ptr)		((ptr)->mnum == PM_FIRE_VORTEX || \
				 (ptr)->mnum == PM_FLAMING_SPHERE || \
				 likes_lava(ptr))

#define touch_petrifies(ptr)	((ptr)->mnum == PM_COCKATRICE || \
				 (ptr)->mnum == PM_CHICKATRICE)

#define is_mind_flayer(ptr)	((ptr)->mnum == PM_MIND_FLAYER || \
				 (ptr)->mnum == PM_MASTER_MIND_FLAYER)

#define nonliving(ptr)		(is_golem(ptr) || is_undead(ptr) || \
				 is_animobj(ptr) || \
				 (ptr)->mlet == S_VORTEX || \
				 (ptr)->mnum == PM_MANES)

#define vs_death_factor(ptr)	(nonliving(ptr) || is_demon(ptr) || \
				 (ptr)->mnum == PM_DEEP_DRAGON || \
				 (ptr)->mnum == PM_BABY_DEEP_DRAGON || \
				 (ptr)->mnum == PM_DEATH)

#define likes_swamp(ptr)	((ptr)->mlet == S_PUDDING || \
				 (ptr)->mlet == S_FUNGUS || \
				 (ptr)->mnum == PM_OCHRE_JELLY)

/* Used for conduct with corpses, tins, and digestion attacks */
/* G_NOCORPSE monsters might still be swallowed as a purple worm */
/* Maybe someday this could be in mflags... */
#define vegan(ptr)		((ptr)->mlet == S_BLOB || \
				 (ptr)->mlet == S_JELLY ||            \
				 (ptr)->mlet == S_FUNGUS ||           \
				 (ptr)->mlet == S_VORTEX ||           \
				 (ptr)->mlet == S_LIGHT ||            \
				((ptr)->mlet == S_ELEMENTAL &&        \
				 (ptr)->mnum != PM_STALKER) ||       \
				((ptr)->mlet == S_GOLEM &&            \
				 (ptr)->mnum != PM_FLESH_GOLEM &&    \
				 (ptr)->mnum != PM_LEATHER_GOLEM) || \
				 noncorporeal(ptr))
#define vegetarian(ptr)		(vegan(ptr) || \
				((ptr)->mlet == S_PUDDING &&         \
				 (ptr)->mnum != PM_BLACK_PUDDING))

#define befriend_with_obj(ptr, obj) (((obj)->oclass == FOOD_CLASS || \
				      (obj)->otyp == SHEAF_OF_STRAW) && \
				     is_domestic(ptr))

#define wants_steed(ptr)	(((ptr)->mflags2 & M2_RIDE) != 0L)

#define	can_use_gun(ptr)	(is_mercenary(ptr) ||			\
				 (is_human(ptr) &&			\
				  (ptr)->mnum == PM_ARCHEOLOGIST ||	\
				  (ptr)->mnum == PM_HEALER ||		\
				  (ptr)->mnum == PM_RANGER ||		\
				  (ptr)->mnum == PM_ROGUE ||		\
				  (ptr)->mnum == PM_TOURIST))

#define can_be_frozen(ptr)	(!((ptr)->geno & G_NOCORPSE) && \
				 !((ptr)->mresists & MR_COLD) && \
				 !flaming(ptr) )

#ifdef MONSTEED
#define	is_mriding(mon)		((mon)->mlinktyp == MLT_STEED)
#define	is_mridden(mon)		((mon)->mlinktyp == MLT_RIDER)
#define	is_swallowing(mon)	((mon)->mlinktyp == MLT_SWALLOWING)
#define	is_swallowed(mon)	((mon)->mlinktyp == MLT_SWALLOWER)
#endif /*MONSTEED*/

#endif /* MONDATA_H */
