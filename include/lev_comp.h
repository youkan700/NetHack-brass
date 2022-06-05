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
#define	CHAR	258
#define	INTEGER	259
#define	BOOLEAN	260
#define	PERCENT	261
#define	MESSAGE_ID	262
#define	MAZE_ID	263
#define	LEVEL_ID	264
#define	LEV_INIT_ID	265
#define	GEOMETRY_ID	266
#define	NOMAP_ID	267
#define	OBJECT_ID	268
#define	COBJECT_ID	269
#define	MONSTER_ID	270
#define	TRAP_ID	271
#define	DOOR_ID	272
#define	DRAWBRIDGE_ID	273
#define	MAZEWALK_ID	274
#define	WALLIFY_ID	275
#define	REGION_ID	276
#define	FILLING	277
#define	RANDOM_OBJECTS_ID	278
#define	RANDOM_MONSTERS_ID	279
#define	RANDOM_PLACES_ID	280
#define	ALTAR_ID	281
#define	LADDER_ID	282
#define	STAIR_ID	283
#define	NON_DIGGABLE_ID	284
#define	NON_PASSWALL_ID	285
#define	ROOM_ID	286
#define	PORTAL_ID	287
#define	TELEPRT_ID	288
#define	BRANCH_ID	289
#define	LEV	290
#define	CHANCE_ID	291
#define	CORRIDOR_ID	292
#define	GOLD_ID	293
#define	ENGRAVING_ID	294
#define	FOUNTAIN_ID	295
#define	POOL_ID	296
#define	SINK_ID	297
#define	NONE	298
#define	RAND_CORRIDOR_ID	299
#define	DOOR_STATE	300
#define	LIGHT_STATE	301
#define	CURSE_TYPE	302
#define	ENGRAVING_TYPE	303
#define	WALLSIGN_ID	304
#define	ROOMSHAPE	305
#define	DIRECTION	306
#define	RANDOM_TYPE	307
#define	O_REGISTER	308
#define	M_REGISTER	309
#define	P_REGISTER	310
#define	A_REGISTER	311
#define	ALIGNMENT	312
#define	LEFT_OR_RIGHT	313
#define	CENTER	314
#define	TOP_OR_BOT	315
#define	ALTAR_TYPE	316
#define	UP_OR_DOWN	317
#define	SUBROOM_ID	318
#define	NAME_ID	319
#define	FLAGS_ID	320
#define	FLAG_TYPE	321
#define	MON_ATTITUDE	322
#define	MON_ALERTNESS	323
#define	MON_APPEARANCE	324
#define	CONTAINED	325
#define	MPICKUP	326
#define	MINVENT	327
#define	BURIED	328
#define	OBJ_RANK	329
#define	INITMAP_TYPE	330
#define	STRING	331
#define	MAP_ID	332


extern YYSTYPE yylval;
