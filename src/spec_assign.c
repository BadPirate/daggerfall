/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern struct room_data *world;
extern int top_of_world;
extern int mini_mud;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname))
{
  if (real_mobile(mob) >= 0)
    mob_index[real_mobile(mob)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant mob #%d",
	    mob);
    log(buf);
  }
}

void ASSIGNOBJ(int obj, SPECIAL(fname))
{
  if (real_object(obj) >= 0)
    obj_index[real_object(obj)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant obj #%d",
	    obj);
    log(buf);
  }
}

void ASSIGNROOM(int room, SPECIAL(fname))
{
  if (real_room(room) >= 0)
    world[real_room(room)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant rm. #%d",
	    room);
    log(buf);
  }
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  SPECIAL(corpse_guy);
  SPECIAL(arena_sentinel);
  SPECIAL(quest_token_master);
  SPECIAL(gamble_master);
  SPECIAL(postmaster);
  SPECIAL(cityguard);
  SPECIAL(receptionist);
  SPECIAL(cryogenicist);
  SPECIAL(guild_guard);
  SPECIAL(temple_cleric);
  SPECIAL(guild);
  SPECIAL(monkey);
  SPECIAL(obj_sentinel);
  SPECIAL(puff);
  SPECIAL(fido);
  SPECIAL(janitor);
  SPECIAL(mayor);
  SPECIAL(snake);
  SPECIAL(thief);
  SPECIAL(magic_user);

  ASSIGNMOB(1, puff);

  /* New Classes */
  ASSIGNMOB(1709, guild);
  ASSIGNMOB(149, guild);
  ASSIGNMOB(106, guild);
  ASSIGNMOB(108, guild);
  ASSIGNMOB(110, guild);
  ASSIGNMOB(113, guild);
  ASSIGNMOB(115, guild);
  ASSIGNMOB(148, guild_guard);
  ASSIGNMOB(105, guild_guard);
  ASSIGNMOB(107, guild_guard);
  ASSIGNMOB(109, guild_guard);
  ASSIGNMOB(112, guild_guard);
  ASSIGNMOB(114, guild_guard);
  ASSIGNMOB(138, guild_guard);

  /* Immortal Zone */
  ASSIGNMOB(1200, receptionist);
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(1202, janitor);

  /* Midgaard */
  ASSIGNMOB(3005, receptionist);
  ASSIGNMOB(3010, postmaster);
  ASSIGNMOB(3020, guild);
  ASSIGNMOB(3021, guild);
  ASSIGNMOB(3022, guild);
  ASSIGNMOB(3023, guild);
  ASSIGNMOB(3024, guild_guard);
  ASSIGNMOB(3025, guild_guard);
  ASSIGNMOB(3026, guild_guard);
  ASSIGNMOB(3027, guild_guard);
  ASSIGNMOB(3059, cityguard);
  ASSIGNMOB(3060, cityguard);
  ASSIGNMOB(3061, janitor);
  ASSIGNMOB(3062, fido);
  ASSIGNMOB(3066, fido);
  ASSIGNMOB(3067, cityguard);
  ASSIGNMOB(3068, janitor);
  ASSIGNMOB(3095, cryogenicist);
  ASSIGNMOB(3105, mayor);

  /* MORIA */
  ASSIGNMOB(4000, snake);
  ASSIGNMOB(4001, snake);
  ASSIGNMOB(4053, snake);
  ASSIGNMOB(4100, magic_user);
  ASSIGNMOB(4102, snake);
  ASSIGNMOB(4103, thief);

  /* Redferne's */
//  ASSIGNMOB(7900, cityguard);

  /* PYRAMID */
  ASSIGNMOB(5300, snake);
  ASSIGNMOB(5301, snake);
  ASSIGNMOB(5304, thief);
  ASSIGNMOB(5305, thief);
  ASSIGNMOB(5309, magic_user); /* should breath fire */
  ASSIGNMOB(5311, magic_user);
  ASSIGNMOB(5313, magic_user); /* should be a cleric */
  ASSIGNMOB(5314, magic_user); /* should be a cleric */
  ASSIGNMOB(5315, magic_user); /* should be a cleric */
  ASSIGNMOB(5316, magic_user); /* should be a cleric */
  ASSIGNMOB(5317, magic_user);

  /* High Tower Of Sorcery */
/*
  ASSIGNMOB(2501, magic_user);
  ASSIGNMOB(2504, magic_user);
  ASSIGNMOB(2507, magic_user);
  ASSIGNMOB(2508, magic_user);
  ASSIGNMOB(2510, magic_user);
  ASSIGNMOB(2511, thief);
  ASSIGNMOB(2514, magic_user);
  ASSIGNMOB(2515, magic_user);
  ASSIGNMOB(2516, magic_user);
  ASSIGNMOB(2517, magic_user);
  ASSIGNMOB(2518, magic_user);
  ASSIGNMOB(2520, magic_user);
  ASSIGNMOB(2521, magic_user);
  ASSIGNMOB(2522, magic_user);
  ASSIGNMOB(2523, magic_user);
  ASSIGNMOB(2524, magic_user);
  ASSIGNMOB(2525, magic_user);
  ASSIGNMOB(2526, magic_user);
  ASSIGNMOB(2527, magic_user);
  ASSIGNMOB(2528, magic_user);
  ASSIGNMOB(2529, magic_user);
  ASSIGNMOB(2530, magic_user);
  ASSIGNMOB(2531, magic_user);
  ASSIGNMOB(2532, magic_user);
  ASSIGNMOB(2533, magic_user);
  ASSIGNMOB(2534, magic_user);
  ASSIGNMOB(2536, magic_user);
  ASSIGNMOB(2537, magic_user);
  ASSIGNMOB(2538, magic_user);
  ASSIGNMOB(2540, magic_user);
  ASSIGNMOB(2541, magic_user);
  ASSIGNMOB(2548, magic_user);
  ASSIGNMOB(2549, magic_user);
  ASSIGNMOB(2552, magic_user);
  ASSIGNMOB(2553, magic_user);
  ASSIGNMOB(2554, magic_user);
  ASSIGNMOB(2556, magic_user);
  ASSIGNMOB(2557, magic_user);
  ASSIGNMOB(2559, magic_user);
  ASSIGNMOB(2560, magic_user);
  ASSIGNMOB(2562, magic_user);
  ASSIGNMOB(2564, magic_user);
*/

  /* SEWERS */
  ASSIGNMOB(7006, snake);
  ASSIGNMOB(7009, magic_user);
//  ASSIGNMOB(7200, magic_user);
//  ASSIGNMOB(7201, magic_user);
//  ASSIGNMOB(7202, magic_user);

  /* FOREST */
  ASSIGNMOB(6112, magic_user);
  ASSIGNMOB(6113, snake);
  ASSIGNMOB(6114, magic_user);
  ASSIGNMOB(6115, magic_user);
  ASSIGNMOB(6116, magic_user); /* should be a cleric */
  ASSIGNMOB(6117, magic_user);

  /* ARACHNOS */
  ASSIGNMOB(6302, magic_user);
  ASSIGNMOB(6309, magic_user);
  ASSIGNMOB(6312, magic_user);
  ASSIGNMOB(6314, magic_user);
  ASSIGNMOB(6315, magic_user);

  /* Desert */
//  ASSIGNMOB(5004, magic_user);
//  ASSIGNMOB(5005, guild_guard); /* brass dragon */
//  ASSIGNMOB(5010, magic_user);
//  ASSIGNMOB(5014, magic_user);

  /* Drow City */
  ASSIGNMOB(5103, magic_user);
  ASSIGNMOB(5104, magic_user);
  ASSIGNMOB(5107, magic_user);
  ASSIGNMOB(5108, magic_user);

  /* Old Thalos */
  ASSIGNMOB(5200, magic_user);
  ASSIGNMOB(5201, magic_user);
  ASSIGNMOB(5209, magic_user);

  /* New Thalos */
/* 5481 - Cleric (or Mage... but he IS a high priest... *shrug*) */
/*
  ASSIGNMOB(5404, receptionist);
  ASSIGNMOB(5421, magic_user);
  ASSIGNMOB(5422, magic_user);
  ASSIGNMOB(5423, magic_user);
  ASSIGNMOB(5424, magic_user);
  ASSIGNMOB(5425, magic_user);
  ASSIGNMOB(5426, magic_user);
  ASSIGNMOB(5427, magic_user);
  ASSIGNMOB(5428, magic_user);
  ASSIGNMOB(5434, cityguard);
  ASSIGNMOB(5440, magic_user);
  ASSIGNMOB(5455, magic_user);
  ASSIGNMOB(5461, cityguard);
  ASSIGNMOB(5462, cityguard);
  ASSIGNMOB(5463, cityguard);
  ASSIGNMOB(5482, cityguard);
*/
/*
5400 - Guildmaster (Mage)
5401 - Guildmaster (Cleric)
5402 - Guildmaster (Warrior)
5403 - Guildmaster (Thief)
5456 - Guildguard (Mage)
5457 - Guildguard (Cleric)
5458 - Guildguard (Warrior)
5459 - Guildguard (Thief)
*/

  /* ROME */
  ASSIGNMOB(12009, magic_user);
  ASSIGNMOB(12018, cityguard);
  ASSIGNMOB(12020, magic_user);
  ASSIGNMOB(12021, cityguard);
  ASSIGNMOB(12025, magic_user);
  ASSIGNMOB(12030, magic_user);
  ASSIGNMOB(12031, magic_user);
  ASSIGNMOB(12032, magic_user);

  /* King Welmar's Castle (not covered in castle.c) */
  ASSIGNMOB(15015, thief);      /* Ergan... have a better idea? */
  ASSIGNMOB(15032, magic_user); /* Pit Fiend, have something better?  Use it */

  /* DWARVEN KINGDOM */
  ASSIGNMOB(6500, cityguard);
  ASSIGNMOB(6502, magic_user);
  ASSIGNMOB(6509, magic_user);
  ASSIGNMOB(6516, magic_user);

 /* KEVIN'S MOBS */
  ASSIGNMOB(158, corpse_guy);
  ASSIGNMOB(1111, magic_user);
  ASSIGNMOB(164, gamble_master);
  ASSIGNMOB(1103, temple_cleric);
  ASSIGNMOB(101, temple_cleric);
  ASSIGNMOB(2401, temple_cleric);
  ASSIGNMOB(150, quest_token_master);
//  ASSIGNMOB(603, monkey);
//  ASSIGNMOB(609, monkey);
  ASSIGNMOB(702, obj_sentinel);
  ASSIGNMOB(703, obj_sentinel);
  ASSIGNMOB(159, arena_sentinel);
  ASSIGNMOB(154, arena_sentinel);
  ASSIGNMOB(155, arena_sentinel);
  ASSIGNMOB(102, obj_sentinel);
  ASSIGNMOB(118, obj_sentinel);
  ASSIGNMOB(146, janitor);
}



/* assign special procedures to objects */
void assign_objects(void)
{
  SPECIAL(bank);
  SPECIAL(gen_board);
  SPECIAL(redbutton);
  SPECIAL(spread);

  ASSIGNOBJ(3096, gen_board);	/* social board */
  ASSIGNOBJ(3097, gen_board);	/* freeze board */
  ASSIGNOBJ(3098, gen_board);	/* immortal board */
  ASSIGNOBJ(3099, gen_board);	/* mortal board */
  ASSIGNOBJ(1782, gen_board);   /* Mist board */
  ASSIGNOBJ(19098, gen_board);  /* KoF board */
  ASSIGNOBJ(12089, gen_board);  /* Trium */
  ASSIGNOBJ(19097, gen_board);  /* Family 1 */
  ASSIGNOBJ(2403, gen_board);    /* Crane Clan Board */

  ASSIGNOBJ(1410, gen_board); /* Generic board #1 */
  ASSIGNOBJ(1411, gen_board); /* Generic board #2 */
  ASSIGNOBJ(1412, gen_board); /* Generic board #3 */
  ASSIGNOBJ(1413, gen_board); /* Generic board #4 */
  ASSIGNOBJ(1414, gen_board); /* Generic board #5 */
  ASSIGNOBJ(1415, gen_board); /* Generic board #6 */

  ASSIGNOBJ(106, redbutton);
  ASSIGNOBJ(3034, bank);	/* atm */
  ASSIGNOBJ(3036, bank);	/* cashcard */
  ASSIGNOBJ(100, spread);
  ASSIGNOBJ(104, spread);
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  extern int dts_are_dumps;
  int i;

  SPECIAL(dump);
  SPECIAL(pet_shops);
  SPECIAL(pray_for_items);

  ASSIGNROOM(3030, dump);
  ASSIGNROOM(3031, pet_shops);

  if (dts_are_dumps)
    for (i = 0; i < top_of_world; i++)
      if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH))
	world[i].func = dump;
}
