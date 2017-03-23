/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern int pk_allowed;
extern int summon_allowed;
extern int charm_allowed;
extern int sleep_allowed;

/* extern functions */
void raw_kill(struct char_data * ch);
int levelchance(struct char_data *ch);

ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Whom do you wish to assist?\r\n", ch);
  else if (!(helpee = get_char_room_vis(ch, arg)))
    send_to_char(NOPERSON, ch);
  else if (helpee == ch)
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  else {
    for (opponent = world[ch->in_room].people;
	 opponent && (FIGHTING(opponent) != helpee);
	 opponent = opponent->next_in_room)
		;

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!pk_allowed && !IS_NPC(opponent) &&  
             !ROOM_FLAGGED(ch->in_room, ROOM_ARENA) &&       
             !(PRF_FLAGGED(ch, PRF_PKILL) && PRF_FLAGGED(opponent, PRF_PKILL)))
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}


ACMD(do_bite)
{
  struct char_data *vict;
  struct follow_type *k;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Bite who?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if (vict == ch) {
    send_to_char("You bite yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
    return;
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't bite $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!pk_allowed) {
      if (!IS_NPC(vict) && !IS_NPC(ch) &&
          !ROOM_FLAGGED(ch->in_room, ROOM_ARENA) &&
          !(PRF_FLAGGED(ch, PRF_PKILL) && PRF_FLAGGED(vict, PRF_PKILL))) 
      {
	send_to_char("You can't bite other players.\r\n", ch);
	return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player */
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      hit(ch, vict, TYPE_UNDEFINED);
      if (GET_CLASS(ch) == CLASS_VAMPYRE)
      {
        GET_COND(ch, THIRST) = 24;
        GET_COND(ch, FULL) = 24;
        send_to_char("The &rblood&n fills you.\r\n",ch);
      }
      else
        send_to_char("The blood tastes salty, and unfullfilling.\r\n",ch);
      for (k = ch->followers; k; k=k->next) 
      {
        if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
          (k->follower->in_room == ch->in_room))
          do_assist(k->follower, GET_NAME(ch), 0, 0);
      }
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }
}

  
ACMD(do_hit)
{
  struct char_data *vict;
  struct follow_type *k;
  ACMD(do_assist);

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Hit who?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!pk_allowed) {
      if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER) &&
          !ROOM_FLAGGED(ch->in_room, ROOM_ARENA) &&
          !(PRF_FLAGGED(ch, PRF_PKILL) && PRF_FLAGGED(vict, PRF_PKILL))) 
      {
	send_to_char("Use 'murder' to hit another player.\r\n", ch);
	return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player */
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      hit(ch, vict, TYPE_UNDEFINED);
      for (k = ch->followers; k; k=k->next) 
      {
        if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
          (k->follower->in_room == ch->in_room))
          do_assist(k->follower, GET_NAME(ch), 0, 0);
      }
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }
}



ACMD(do_kill)
{
  struct char_data *vict;

  if ((GET_LEVEL(ch) < LVL_IMPL) || IS_NPC(ch)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_room_vis(ch, arg)))
      send_to_char("They aren't here.\r\n", ch);
    else if (ch == vict)
      send_to_char("Your mother would be so sad.. :(\r\n", ch);
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict);
    }
  }
}



ACMD(do_backstab)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, buf);

  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char("Backstab who?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("How can you sneak up on yourself?\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char("You can't backstab a fighting person -- they're too alert!\r\n", ch);
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }


  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BACKSTAB);
  
  if (AWAKE(vict) && (percent > prob))
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);

  WAIT_STATE(ch, PULSE_VIOLENCE);
}



ACMD(do_order)
{
  char name[100], message[256];
  char buf[256];
  bool found = FALSE;
  int org_room;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Order who to do what?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, name)) && !is_abbrev(name, "followers"))
    send_to_char("That person isn't here.\r\n", ch);
  else if (ch == vict)
    send_to_char("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !IS_AFFECTED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(OK, ch);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, vict, TO_ROOM);

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
	if (org_room == k->follower->in_room)
	  if (IS_AFFECTED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(OK, ch);
      else
	send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}

int better_flee_chance(struct char_data *ch)
{
  if (IS_HUMAN(ch)) return 1;
    else return 0;
}

ACMD(do_peace)
{
  struct char_data *f=FIGHTING(ch);

  if (!FIGHTING(ch)) {
    send_to_char("But you're not fighting anyone!\n",ch);
    return;
  }
  act("$n tries to pacify $N...", TRUE, ch, 0, f, TO_ROOM);
  send_to_char("You try to calm your foe...\n",ch);
  if ( IS_PIXIE(ch) ) {    
    if(levelchance(ch)) {
      act("$N calms down for a bit.", TRUE, ch, 0, f, TO_ROOM);
      stop_fighting(ch);
      stop_fighting(f);
    }
  } else {
    act("$N is unaffected by $n and continues attacking!", 
	TRUE, ch, 0, f, TO_ROOM);
  }
}

ACMD(do_flee)
{
  int i, attempt;
  extern void amount_exp_loss(int, struct char_data *, struct char_data *);


  if(!IS_NPC(ch))
    if(FIGHTING(ch) == NULL)
    {
      send_to_char("But your not fighting anyone!\r\n",ch);
      return;
    }

  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (!CAN_GO(ch,attempt)) {
        if (better_flee_chance(ch)) {
            attempt=number(0,NUM_OF_DIRS-1);
        }
    }
    if (CAN_GO(ch, attempt) &&
	!IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      if (do_simple_move(ch, attempt, TRUE)) {
        if (!IS_NPC(ch) && FIGHTING(ch) != NULL)
        {
          send_to_char("You lose exp.\r\n", ch);
          amount_exp_loss((GET_MAX_HIT(FIGHTING(ch))-GET_HIT(FIGHTING(ch))), ch, FIGHTING(ch));
        }
	send_to_char("You flee head over heels.\r\n", ch);
	if (FIGHTING(ch)) {
	  if (FIGHTING(FIGHTING(ch)) == ch)
	    stop_fighting(FIGHTING(ch));
	  stop_fighting(ch);
	}
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}


ACMD(do_bash)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC || 
      GET_CLASS(ch) == CLASS_BARD || GET_CLASS(ch) == CLASS_VAMPYRE)

  {
    send_to_char("You'd better leave all the martial arts to fighters.\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BASH);

  if (MOB_FLAGGED(vict, MOB_NOBASH))
    percent = 101;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
  } else {
    damage(ch, vict, 1, SKILL_BASH);
    GET_POS(vict) = POS_SITTING;
    WAIT_STATE(vict, PULSE_VIOLENCE);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("Whom do you want to rescue?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("What about fleeing instead?\r\n", ch);
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
    return;
  }
  for (tmp_ch = world[ch->in_room].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (GET_CLASS(ch) != CLASS_WARRIOR)
    send_to_char("But only true warriors can do this!", ch);
  else {
    percent = number(1, 101);	/* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_RESCUE);

    if (percent > prob) {
      send_to_char("You fail the rescue!\r\n", ch);
      return;
    }
    send_to_char("Banzai!  To the rescue...\r\n", ch);
    act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

    if (FIGHTING(vict) == tmp_ch)
      stop_fighting(vict);
    if (FIGHTING(tmp_ch))
      stop_fighting(tmp_ch);
    if (FIGHTING(ch))
      stop_fighting(ch);

    set_fighting(ch, tmp_ch);
    set_fighting(tmp_ch, ch);

    WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
  }

}



ACMD(do_kick)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101);	/* 101% is a complete
								 * failure */
  prob = GET_SKILL(ch, SKILL_KICK);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK);
  } else
    damage(ch, vict, GET_LEVEL(ch) >> 1, SKILL_KICK);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

void do_taunt(struct char_data *ch, struct char_data *vict)
{
  if(!IS_BROWNIE(ch)) return;
  if(!vict) return;

  if (!pk_allowed && !IS_NPC(vict) && 
      !ROOM_FLAGGED(ch->in_room, ROOM_ARENA) && 
      !(PRF_FLAGGED(ch, PRF_PKILL) && PRF_FLAGGED(vict, PRF_PKILL)))
    return;    
  if (IS_SET(world[ch->in_room].room_flags,ROOM_PEACEFUL)) return;
  if (GET_LEVEL(vict)>LVL_IMMORT) return;
  if (IS_NPC(vict))
    if (MOB_FLAGGED(vict, MOB_SENTINEL) || MOB_FLAGGED(vict, MOB_NOATTACK))
      return;
  
  if(levelchance(ch)) {
    act("You've enraged $N.", TRUE, ch, 0, vict, TO_CHAR);
    act("You've been enraged by $n!", TRUE, ch, 0, vict, TO_VICT);
    act("$n enrages $N, who attacks in a wild frenzy.",
	TRUE, ch, 0, vict, TO_NOTVICT);
    /* need to give the mob a nasty affect for a while */
    mag_affects(GET_LEVEL(ch),ch,vict,SKILL_TAUNT,0);
    hit(vict,ch,TYPE_UNDEFINED);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE);
}
