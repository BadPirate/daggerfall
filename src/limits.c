/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

extern int exp_to_level(struct char_data *);
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct room_data *world;
extern int max_exp_gain;
extern int max_exp_loss;

#define NEEDFOOD(ch) ((GET_COND((ch),FULL)<5) && (GET_COND((ch),FULL)>-1))
#define NEEDWATER(ch) ((GET_COND((ch),THIRST)<5) && (GET_COND((ch),THIRST)>-1))

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (age < 15)
    return (p0);		/* < 15   */
  else if (age <= 29)
    return (int) (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (age <= 44)
    return (int) (p2 + (((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (age <= 59)
    return (int) (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (age <= 79)
    return (int) (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);		/* >= 80 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data * ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch).year, 4, 8, 12, 16, 12, 10, 8);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain <<= 1;
      break;
    case POS_RESTING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_SITTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    }

    if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_CLERIC))
      gain <<= 1;
  }

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ( NEEDFOOD(ch) || NEEDWATER(ch) )
    gain >>= 2;

  if(IS_TROLL(ch)) {
     gain = 4* gain / 3;
  }

  if(ROOM_FLAGGED(ch->in_room, ROOM_HEAL)) gain=3*gain/2;
  if(ROOM_FLAGGED(ch->in_room, ROOM_OPPRESSIVE)) gain=0;
  return (gain);
}


int hit_gain(struct char_data * ch)
/* Hitpoint gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    gain = GET_LEVEL(ch);
    /* Neat and fast */
  } else {

    gain = graf(age(ch).year, 8, 12, 20, 32, 16, 10, 4);

    /* Class/Level calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
    }

    if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_CLERIC))
      gain >>= 1;
  }

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ( NEEDFOOD(ch) || NEEDWATER(ch) )
    gain >>= 2;

  if(IS_GNOME(ch)) {
     gain = 4* gain / 3;
  }

  if(ROOM_FLAGGED(ch->in_room, ROOM_HEAL)) gain=4*gain/3;
  if(ROOM_FLAGGED(ch->in_room, ROOM_OPPRESSIVE)) gain=0;
  return (gain);
}



int move_gain(struct char_data * ch)
/* move gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    return (GET_LEVEL(ch));
    /* Neat and fast */
  } else {
    gain = graf(age(ch).year, 16, 20, 24, 20, 16, 12, 10);

    /* Class/Level calculations */

    /* Skill/Spell calculations */


    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
    }
  }

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ( NEEDFOOD(ch) || NEEDWATER(ch) )
    gain >>= 2;

  if(ROOM_FLAGGED(ch->in_room, ROOM_HEAL)) gain=3*gain/2;
  if(ROOM_FLAGGED(ch->in_room, ROOM_OPPRESSIVE)) gain=0;

  if(IS_AFFECTED(ch,AFF_STATUE))
    gain = -10;

  return (gain);
}


void set_title(struct char_data * ch, char *title) 
{
  if (title == NULL)
    title = '\0';

  if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  GET_TITLE(ch) = str_dup(title);
}


void check_autowiz(struct char_data * ch)
{
  char buf[100];
  extern int use_autowiz;
  extern int min_wizlist_lev;
  pid_t getpid(void);

  if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
    mudlog("Initiating autowiz.", CMP, LVL_IMMORT, FALSE);
    system(buf);
  }
}



void gain_exp(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0, x;
  char buf[128];

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)) 
      && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }
  if (gain > 0) {
    gain = MIN(max_exp_gain, gain);	/* put a cap on the max gain per kill */
    GET_EXP(ch) += gain;
    while ((GET_LEVEL(ch) < (LVL_IMMORT-1) || GET_CLASS(ch) == CLASS_MASTER)
           && GET_EXP(ch) >= exp_to_level(ch)) {
      if(GET_LEVEL(ch) == 100 && GET_CLASS(ch) == CLASS_MASTER)
      {
        EXTRA_LEVEL(ch) = 0;
        GET_LEVEL(ch) += 1;
        num_levels++;
        advance_level(ch);
        is_altered = TRUE;
      }
      else if(GET_LEVEL(ch) == 101 && GET_CLASS(ch) == CLASS_MASTER)
      {
        EXTRA_LEVEL(ch) += 1;
        x = EXTRA_LEVEL(ch) + GET_LEVEL(ch);
        num_levels++;
        advance_level(ch);
        is_altered = TRUE;
      }
      else
      {
        GET_LEVEL(ch) += 1;
        num_levels++;
        advance_level(ch);
        is_altered = TRUE;
      }
    }

    if (is_altered) {
      if (num_levels == 1)
        send_to_char("\r\n&rYou rise a level!&n\r\n\r\n", ch);
      else {
	sprintf(buf, "\r\n&rYou rise %d levels!&n\r\n\r\n", num_levels);
	send_to_char(buf, ch);
      }
      check_autowiz(ch);
    }
  } else if (gain < 0) {
    gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
}


void gain_exp_regardless(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMMORT && GET_EXP(ch) >= exp_to_level(ch)) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      if (num_levels == 1)
        send_to_char("You rise a level!\r\n", ch);
      else {
	sprintf(buf, "You rise %d levels!\r\n", num_levels);
	send_to_char(buf, ch);
      }
      check_autowiz(ch);
    }
  }
}


void gain_condition(struct char_data * ch, int condition, int value)
{
  extern struct townstart_struct townstart[];
  void raw_kill(struct char_data *);
  void death_cry(struct char_data *);

  bool intoxicated, safe = FALSE;
  int x;

  for(x=0; townstart[x].minlevel != -1 && safe == FALSE; x++)
    if ( ch->in_room == real_room(townstart[x].loadroom) ||
         ch->in_room == real_room(townstart[x].corpseroom) ||
         ch->in_room == real_room(townstart[x].deathroom) ||
         ch->in_room == real_room(1) || ch->in_room == real_room(0) ||
         GET_LEVEL(ch) <= 10)
      safe = TRUE;  

  if (ch->in_room == real_room(1460))
    safe = TRUE;

  if (GET_COND(ch, condition) == -1) {	/* No change */
    return; }

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING)) {
    return; }

  switch (condition) {
  case FULL:
    send_to_char("You are hungry.\r\n", ch);
    if (GET_COND(ch, FULL) < 3 && safe == FALSE)
    {
      send_to_char("&rYou are so hungry it hurts...\r\n&n",ch);
      GET_HIT(ch) -= number(1,10);
      update_pos(ch);
      if(GET_POS(ch) <= POS_STUNNED)
      {
        death_cry(ch);
        sprintf(buf, "%s is killed by hunger!\r\n", GET_NAME(ch));
        send_to_all(buf);
        raw_kill(ch);
      }
    }
    return;
  case THIRST:
    send_to_char("You are thirsty.\r\n", ch);
    if (GET_COND(ch, THIRST) < 3 && safe == FALSE)
    {
      send_to_char("&rYou are so thirsty it hurts...&n\r\n",ch);
      GET_HIT(ch) -= number(1,10);
      update_pos(ch);
      if(GET_POS(ch) <= POS_STUNNED)
      {
        death_cry(ch);
        sprintf(buf, "%s is dying for a drink (dead)!\r\n", GET_NAME(ch));
        send_to_all(buf);
        raw_kill(ch);
      }
    }
    return;
  case DRUNK:
    if (intoxicated)
      send_to_char("You feel more sober.\r\n", ch);
    return;
  default:
    break;
  }
}


void check_idling(struct char_data * ch)
{
  extern int free_rent;
  void Crash_rentsave(struct char_data *ch, int cost);

  if (++(ch->char_specials.timer) > 8)
  {
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
      save_char(ch, NOWHERE);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->char_specials.timer > 48) {
      if (ch->in_room != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc)
	close_socket(ch->desc);
      ch->desc = NULL;
      if (free_rent)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);
      sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
      mudlog(buf, CMP, LVL_GOD, TRUE);
      extract_char(ch);
    }
  }
}



/* Update PCs, NPCs, and objects */
void point_update(void)
{
  void update_char_objects(struct char_data * ch);	/* handler.c */
  void extract_obj(struct obj_data * obj);	/* handler.c */
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;
	
    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);
	
    if (GET_POS(i) >= POS_STUNNED) {
      GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
      if (IS_AFFECTED(i, AFF_POISON))
	damage(i, i, 2, SPELL_POISON);
      if (GET_POS(i) <= POS_STUNNED)
	update_pos(i);
    } else if (GET_POS(i) == POS_INCAP)
      damage(i, i, 1, TYPE_SUFFERING);
    else if (GET_POS(i) == POS_MORTALLYW)
      damage(i, i, 2, TYPE_SUFFERING);
    if (!IS_NPC(i)) {
      update_char_objects(i);
      if (GET_LEVEL(i) < LVL_GOD)
	check_idling(i);
    }
  }

  /* objects */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */

    /* If this is a corpse */
    if (((GET_OBJ_TYPE(j) == ITEM_CONTAINER) && GET_OBJ_VAL(j, 3)) || GET_OBJ_TYPE(j) ==
        ITEM_PORTAL) 
    {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

	if (j->carried_by)
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
	else if ((j->in_room != NOWHERE) && (world[j->in_room].people)) 
        {
          if(GET_OBJ_TYPE(j) == ITEM_PORTAL)
          {
            act("A $p fades from existance.",
	        TRUE, world[j->in_room].people, j, 0, TO_ROOM);
            act("A $p fades from existance.",
	        TRUE, world[j->in_room].people, j, 0, TO_CHAR);
          }
          else
          {
  	    act("A quivering horde of maggots consumes $p.",
	        TRUE, world[j->in_room].people, j, 0, TO_ROOM);
	    act("A quivering horde of maggots consumes $p.",
	        TRUE, world[j->in_room].people, j, 0, TO_CHAR);
          }
	}
	for (jj = j->contains; jj; jj = next_thing2) {
	  next_thing2 = jj->next_content;	/* Next in inventory */
	  obj_from_obj(jj);

	  if (j->in_obj)
	    obj_to_obj(jj, j->in_obj);
	  else if (j->carried_by)
	    obj_to_room(jj, j->carried_by->in_room);
	  else if (j->in_room != NOWHERE)
	    obj_to_room(jj, j->in_room);
	  else
	    assert(FALSE);
	}
	extract_obj(j);
      }
    }
  }
}
