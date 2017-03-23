/*************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __COMM_C__

#include "conf.h"
#include "sysdep.h"
#include "screen.h"

#ifdef CIRCLE_WINDOWS		/* Includes for Win32 */
#include <direct.h>
#include <mmsystem.h>
#else				/* Includes for UNIX */
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "olc.h"
#include "clan.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define YES	1
#define NO	0

/* externs */
extern int exp_to_level(struct char_data *);
extern int game_restrict;
extern int mini_mud;
extern int no_rent_check;
extern FILE *player_fl;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int MAX_PLAYERS;
extern int MAX_DESCRIPTORS_AVAILABLE;
void proc_color(char *inbuf, int color);

extern void cleanup_clan_edit(struct descriptor_data *d, int method);

extern struct room_data *world;	/* In db.c */
extern int top_of_world;	/* In db.c */
extern struct time_info_data time_info;		/* In db.c */
extern char help[];

/* local globals */
struct descriptor_data *descriptor_list = NULL;		/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */
struct timeval null_time;	/* zero-valued time structure */

static bool fCopyOver;          /* Are we booting in copyover mode? */
int  mother_desc;        /* Now a global */
int     port;

/* functions in this file */
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_max_players(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void close_socket(struct descriptor_data *d);
struct timeval timediff(struct timeval a, struct timeval b);
struct timeval timeadd(struct timeval a, struct timeval b);
void flush_queues(struct descriptor_data *d);
void nonblock(socket_t s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
void make_prompt(struct descriptor_data *point);
void check_idle_passwords(void);
void heartbeat(int pulse);
void    init_descriptor (struct descriptor_data *newd, int desc);


/* extern fcnts */
void boot_db(void);
void boot_world(void);
void zone_update(void);
void affect_update(void);	/* In spells.c */
void point_update(void);	/* In limits.c */
void mobile_activity(void);
void string_add(struct descriptor_data *d, char *str);
void perform_violence(void);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname);
void weather_and_time(int mode);
struct drunk_struct
{
        int     min_drunk_level;
        int     number_of_rep;
        char    *replacement[11];
};

char    *makedrunk(char *string ,struct char_data *ch);

/* How to make a string look drunk... by Apex (robink@htsa.hva.nl) */
/* Modified and enhanced for envy(2) by the Maniac from Mythran    */
/* Ported to Stock Circle 3.0 by Haddixx (haddixx@megamed.com)     */

char * makedrunk (char *string, struct char_data * ch)
{

/* This structure defines all changes for a character */
  struct drunk_struct drunk[] =
  {
    {3, 10,
      {"a", "a", "a", "A", "aa", "ah", "Ah", "ao", "aw", "oa", "ahhhh"}},
    {8, 5,
     {"b", "b", "b", "B", "B", "vb"}},
    {3, 5,
     {"c", "c", "C", "cj", "sj", "zj"}},
    {5, 2,
     {"d", "d", "D"}},
    {3, 3,
     {"e", "e", "eh", "E"}},
    {4, 5,
     {"f", "f", "ff", "fff", "fFf", "F"}},
    {8, 2,
     {"g", "g", "G"}},
    {9, 6,
     {"h", "h", "hh", "hhh", "Hhh", "HhH", "H"}},
    {7, 6,
     {"i", "i", "Iii", "ii", "iI", "Ii", "I"}},
    {9, 5,
     {"j", "j", "jj", "Jj", "jJ", "J"}},
    {7, 2,
     {"k", "k", "K"}},
    {3, 2,
     {"l", "l", "L"}},
    {5, 8,
     {"m", "m", "mm", "mmm", "mmmm", "mmmmm", "MmM", "mM", "M"}},
    {6, 6,
     {"n", "n", "nn", "Nn", "nnn", "nNn", "N"}},
    {3, 6,
     {"o", "o", "ooo", "ao", "aOoo", "Ooo", "ooOo"}},
    {3, 2,
     {"p", "p", "P"}},
    {5, 5,
     {"q", "q", "Q", "ku", "ququ", "kukeleku"}},
    {4, 2,
     {"r", "r", "R"}},
    {2, 5,
     {"s", "ss", "zzZzssZ", "ZSssS", "sSzzsss", "sSss"}},
    {5, 2,
     {"t", "t", "T"}},
    {3, 6,
     {"u", "u", "uh", "Uh", "Uhuhhuh", "uhU", "uhhu"}},
    {4, 2,
     {"v", "v", "V"}},
    {4, 2,
     {"w", "w", "W"}},
    {5, 6,
     {"x", "x", "X", "ks", "iks", "kz", "xz"}},
    {3, 2,
     {"y", "y", "Y"}},
    {2, 9,
     {"z", "z", "ZzzZz", "Zzz", "Zsszzsz", "szz", "sZZz", "ZSz", "zZ", "Z"}}
  };

  char buf[MAX_STRING_LENGTH];      /* this should be enough (?) */
  char temp;
  int pos = 0;
  int randomnum;

  if(GET_COND(ch, DRUNK) > 0)  /* character is drunk */
  {
     do
     {
       temp = toupper(*string);
       if( (temp >= 'A') && (temp <= 'Z') )
       {
         if(GET_COND(ch, DRUNK) > drunk[(temp - 'A')].min_drunk_level)
         {
           randomnum = number(0, (drunk[(temp - 'A')].number_of_rep));
           strcpy(&buf[pos], drunk[(temp - 'A')].replacement[randomnum]);
           pos += strlen(drunk[(temp - 'A')].replacement[randomnum]);
         }
         else
           buf[pos++] = *string;
       }
       else
       {
         if ((temp >= '0') && (temp <= '9'))
         {
           temp = '0' + number(0, 9);
           buf[pos++] = temp;
         }
         else
           buf[pos++] = *string;
       }
     }while (*string++);

     buf[pos] = '\0';          /* Mark end of the string... */
     strcpy(string, buf);
     return(string);
  }
  return (string); /* character is not drunk, just return the string */
}


/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

/* Windows doesn't have gettimeofday, so we'll simulate it. */
#ifdef CIRCLE_WINDOWS

void gettimeofday(struct timeval *t, struct timezone *dummy)
{
  DWORD millisec = GetTickCount();

  t->tv_sec = (int) (millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}

#endif


int main(int argc, char **argv)
{
  char buf[512];
  int pos = 1;
  char *dir;

  port = DFLT_PORT;
  dir = DFLT_DIR;

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
      fCopyOver = TRUE;
      mother_desc = atoi(argv[pos]+2);
      break;
    case 'd':
      if (*(argv[pos] + 2))
	dir = argv[pos] + 2;
      else if (++pos < argc)
	dir = argv[pos];
      else {
	log("Directory arg expected after option -d.");
	exit(1);
      }
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      log("Running in minimized mode & with no rent check.");
      break;
    case 'c':
      scheck = 1;
      log("Syntax check mode enabled.");
      break;
    case 'q':
      no_rent_check = 1;
      log("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      game_restrict = 1;
      log("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      log("Suppressing assignment of special routines.");
      break;
    default:
      sprintf(buf, "SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
      log(buf);
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      fprintf(stderr, "Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      fprintf(stderr, "Illegal port number.\n");
      exit(1);
    }
  }
#ifdef CIRCLE_WINDOWS
  if (_chdir(dir) < 0) {
#else
  if (chdir(dir) < 0) {
#endif
    perror("Fatal error changing to data directory");
    exit(1);
  }
  sprintf(buf, "Using %s as data directory.", dir);
  log(buf);

  if (scheck) {
    boot_world();
    log("Done.");
    exit(0);
  } else {
    sprintf(buf, "Running game on port %d.", port);
    log(buf);
    init_game(port);
  }

  return 0;
}

int enter_player_game(struct descriptor_data *d);
 
/* Reload players after a copyover */
void copyover_recover()
{
	struct descriptor_data *d;
	FILE *fp;
	char host[1024];
    struct char_file_u tmp_store;
	int desc, player_i;
    bool fOld;
    char name[MAX_INPUT_LENGTH];
	
 	log ("Copyover recovery initiated");
 	
 	fp = fopen (COPYOVER_FILE, "r");
 	
 	if (!fp) /* there are some descriptors open which will hang forever then ? */
 	{
 		perror ("copyover_recover:fopen");
 		log ("Copyover file not found. Exitting.\n\r");
 		exit (1);
 	}
 
 	unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading	*/
 	
 	for (;;)
     {
         fOld = TRUE;
 		fscanf (fp, "%d %s %s\n", &desc, name, host);
 		if (desc == -1)
 			break;
 
 		/* Write something, and check if it goes error-free */		
 		if (write_to_descriptor (desc, "\n\rRestoring from copyover...\n\r") < 0)
 		{
 			close (desc); /* nope */
 			continue;
 		}
 		
         /* create a new descriptor */
         CREATE (d, struct descriptor_data, 1);
         memset ((char *) d, 0, sizeof (struct descriptor_data));
 		init_descriptor (d,desc); /* set up various stuff */
 		
 		strcpy(d->host, host);
		d->next = descriptor_list;
 		descriptor_list = d;
 
         d->connected = CON_CLOSE;
 		/* Now, find the pfile */
 		
         CREATE(d->character, struct char_data, 1);
         clear_char(d->character);
         CREATE(d->character->player_specials, struct player_special_data, 1);
         d->character->desc = d;
 
         if ((player_i = load_char(name, &tmp_store)) >= 0)
         {
             store_to_char(&tmp_store, d->character);
             GET_PFILEPOS(d->character) = player_i;
             if (!PLR_FLAGGED(d->character, PLR_DELETED))
                 REMOVE_BIT(PLR_FLAGS(d->character),PLR_WRITING | PLR_MAILING | PLR_CRYO);
             else
                 fOld = FALSE;
         }
         else
             fOld = FALSE;
 		
 		if (!fOld) /* Player file not found?! */
 		{
 			write_to_descriptor (desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r");
 			close_socket (d);			
 		}
 		else /* ok! */
 		{
             write_to_descriptor (desc, "\n\rCopyover recovery complete.\n\r");
             enter_player_game(d);
             d->connected = CON_PLAYING;
             look_at_room(d->character, 0);
 		}
 		
 	} 	
 	fclose (fp);
}
 


/* Init sockets, run game, and cleanup sockets */
void init_game(int port)
{
  srandom(time(0));

  log("Finding player limit.");
  max_players = get_max_players();

  if (!fCopyOver) /* If copyover mother_desc is already set up */
  {
    log ("Opening mother connection.");
    mother_desc = init_socket (port);
  }

  boot_db();

#ifndef CIRCLE_WINDOWS
  log("Signal trapping.");
  signal_setup();
#endif
  if (fCopyOver) /* reload players */
    copyover_recover();

  log("Entering game loop.");

  game_loop(mother_desc);

  log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  CLOSE_SOCKET(mother_desc);
  fclose(player_fl);

  if (circle_reboot) {
    log("Rebooting.");
    exit(52);			/* what's so great about HHGTTG, anyhow? */
  }
  log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so ths point is (hopefully) moot.
   */

#ifdef CIRCLE_WINDOWS
  {
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      log("WinSock not available!\n");
      exit(1);
    }
    if ((wsaData.iMaxSockets - 4) < max_players) {
      max_players = wsaData.iMaxSockets - 4;
    }
    sprintf(buf, "Max players set to %d", max_players);
    log(buf);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      fprintf(stderr, "Error opening network connection: Winsock err #%d\n", WSAGetLastError());
      exit(1);
    }
  }
#else
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error creating socket");
    exit(1);
  }
#endif				/* CIRCLE_WINDOWS */

#if defined(SO_SNDBUF)
  opt = LARGE_BUFSIZE + GARBAGE_SPACE;
  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt SNDBUF");
    exit(1);
  }
#endif

#if defined(SO_REUSEADDR)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEADDR");
    exit(1);
  }
#endif

#if defined(SO_LINGER)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0) {
      perror("setsockopt LINGER");
      exit(1);
    }
  }
#endif

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("bind");
    CLOSE_SOCKET(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return s;
}


int get_max_players(void)
{
    return MAX_PLAYERS;
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(int mother_desc)
{
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, before_sleep, opt_time, process_time, now, timeout;
  char comm[MAX_STRING_LENGTH];
  struct descriptor_data *d, *next_d;
  int pulse = 0, missed_pulses, maxdesc, aliased;

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  FD_ZERO(&null_set);

  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
      log("No connections.  Going to sleep.");
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
	if (errno == EINTR)
	  log("Waking up to process signal.");
	else
	  perror("Select coma");
      } else
	log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);

    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
#ifndef CIRCLE_WINDOWS
      if (d->descriptor > maxdesc)
	maxdesc = d->descriptor;
#endif
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, we have completed all input, output and heartbeat
     * activity from the previous iteration, so we have to put ourselves
     * to sleep until the next 0.1 second tick.  The first step is to
     * calculate how long we took processing the previous iteration.
     */
    
    gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
    process_time = timediff(before_sleep, last_time);

    /*
     * If we were asleep for more than one pass, count missed pulses and sleep
     * until we're resynchronized with the next upcoming pulse.
     */
    if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
      missed_pulses = 0;
    } else {
      missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
      missed_pulses += process_time.tv_usec / OPT_USEC;
      process_time.tv_sec = 0;
      process_time.tv_usec = process_time.tv_usec % OPT_USEC;
    }

    /* Calculate the time we should wake up */
    last_time = timeadd(before_sleep, timediff(opt_time, process_time));

    /* Now keep sleeping until that time has come */
    gettimeofday(&now, (struct timezone *) 0);
    timeout = timediff(last_time, now);

    /* go to sleep */
    do {
#ifdef CIRCLE_WINDOWS
      Sleep(timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#else
      if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0) {
	if (errno != EINTR) {
	  perror("Select sleep");
	  exit(1);
	}
      }
#endif /* CIRCLE_WINDOWS */
      gettimeofday(&now, (struct timezone *) 0);
      timeout = timediff(last_time, now);
    } while (timeout.tv_usec || timeout.tv_sec);

    /* poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("Select poll");
      return;
    }
    /* If there are new connections waiting, accept them. */
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);

    /* kick out the freaky folks in the exception set */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
	FD_CLR(d->descriptor, &input_set);
	FD_CLR(d->descriptor, &output_set);
	close_socket(d);
      }
    }

    /* process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
	if (process_input(d) < 0)
	  close_socket(d);
    }

    /* process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      if ((--(d->wait) <= 0) && get_from_q(&d->input, comm, &aliased)) {
	if (d->character) {
	  /* reset the idle timer & pull char back from void if necessary */
	  d->character->char_specials.timer = 0;
	  if (!d->connected && GET_WAS_IN(d->character) != NOWHERE) {
	    if (d->character->in_room != NOWHERE)
	      char_from_room(d->character);
	    char_to_room(d->character, GET_WAS_IN(d->character));
	    GET_WAS_IN(d->character) = NOWHERE;
	    act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
	  }
	}
	d->wait = 1;
	d->prompt_mode = 1;

	 /* reversed these top 2 if checks so that you can use the page_string */
	 /* function in the editor */
	if (d->showstr_count)	/* reading something w/ pager     */
	  show_string(d, comm);
	else if (d->str)		/* writing boards, mail, etc.     */
	  string_add(d, comm);
	else if (d->connected != CON_PLAYING)	/* in menus, etc. */
	  nanny(d, comm);
	else {			/* else: we're playing normally */
	  if (aliased)		/* to prevent recursive aliases */
	    d->prompt_mode = 0;
	  else {
	    if (perform_alias(d, comm))		/* run it through aliasing system */
	      get_from_q(&d->input, comm, &aliased);
	  }
	  command_interpreter(d->character, comm);	/* send it to interpreter */
	}
      }
    }

    /* send queued output out to the operating system (ultimately to user) */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &output_set) && *(d->output))
      {
	if (process_output(d) < 0)
	  close_socket(d);
	else
       	  d->prompt_mode = 1;
      }
    }

    /* kick out folks in the CON_CLOSE state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE)
	close_socket(d);
    }

    /* give each descriptor an appropriate prompt */
    for (d = descriptor_list; d; d = d->next) {
      if (d->prompt_mode) {
	make_prompt(d);
	d->prompt_mode = 0;
      }
    }

    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0) {
      log("SYSERR: MISSED_PULSES IS NONPOSITIVE!!!");
      missed_pulses = 1;
    }

    /* If we missed more than 30 seconds worth of pulses, forget it */
    if (missed_pulses > (30 * PASSES_PER_SEC)) {
      log("Warning: Missed more than 30 seconds worth of pulses");
      missed_pulses = 30 * PASSES_PER_SEC;
    }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
      heartbeat(++pulse);

    /* Roll pulse over after 10 hours */
    if (pulse >= (600 * 60 * PASSES_PER_SEC))
      pulse = 0;

    /* Update tics for deadlock protection (UNIX only) */
    tics++;
  }
}


void heartbeat(int pulse)
{
  extern void clan_save(int, struct char_data *, int);
  extern void arena_pulse(void);
  static int mins_since_crashsave = 0;
  void auction_update();
 
  if (!(pulse % PULSE_ZONE))
    zone_update();

  if (!(pulse % (15 * PASSES_PER_SEC)))		/* 15 seconds */
    check_idle_passwords();
  if (!(pulse % (15 * PASSES_PER_SEC)))
    auction_update();

  if (!(pulse % (25 * PASSES_PER_SEC)))
    arena_pulse();  

  if (!(pulse % PULSE_MOBILE))
    mobile_activity();

  if (!(pulse % PULSE_VIOLENCE))
    perform_violence();

  if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
    weather_and_time(1);
    affect_update();
    point_update();
    fflush(player_fl);
  }
  if (auto_save && !(pulse % (60 * PASSES_PER_SEC))) {	/* 1 minute */
    if (++mins_since_crashsave >= autosave_time) {
      mins_since_crashsave = 0;
      Crash_save_all();
      clan_save(YES,0,NO);
      House_save_all();
    }
  }
  if (!(pulse % (5 * 60 * PASSES_PER_SEC)))	/* 5 minutes */
    record_usage();
}


/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
struct timeval timediff(struct timeval a, struct timeval b)
{
  struct timeval rslt;

  if (a.tv_sec < b.tv_sec)
    return null_time;
  else if (a.tv_sec == b.tv_sec) {
    if (a.tv_usec < b.tv_usec)
      return null_time;
    else {
      rslt.tv_sec = 0;
      rslt.tv_usec = a.tv_usec - b.tv_usec;
      return rslt;
    }
  } else {			/* a->tv_sec > b->tv_sec */
    rslt.tv_sec = a.tv_sec - b.tv_sec;
    if (a.tv_usec < b.tv_usec) {
      rslt.tv_usec = a.tv_usec + 1000000 - b.tv_usec;
      rslt.tv_sec--;
    } else
      rslt.tv_usec = a.tv_usec - b.tv_usec;
    return rslt;
  }
}

/* add 2 timevals */
struct timeval timeadd(struct timeval a, struct timeval b)
{
  struct timeval rslt;

  rslt.tv_sec = a.tv_sec + b.tv_sec;
  rslt.tv_usec = a.tv_usec + b.tv_usec;

  while (rslt.tv_usec >= 1000000) {
    rslt.tv_usec -= 1000000;
    rslt.tv_sec++;
  }

  return rslt;
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;
  char buf[256];

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (!d->connected)
      sockets_playing++;
  }

  sprintf(buf, "nusage: %-3d sockets connected, %-3d sockets playing",
	  sockets_connected, sockets_playing);
  log(buf);

#ifdef RUSAGE
  {
    struct rusage ru;

    getrusage(0, &ru);
    sprintf(buf, "rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
	    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
    log(buf);
  }
#endif

}



/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) TELOPT_NAOFFD,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  SEND_TO_Q(on_string, d);
}


// this little function right here is taken from my inline color code
// I wrote some two years ago from the long gone Krimson DIKUMUD... I
// didn't want to rely upon the easy_color patch (and besides, which,
// I still don't know the actual codes it uses)  This is how the colors
// are on my (currently siteless) MUD.
int is_color(char c) {
  switch (c) {
  case 'x': return 30; break;
  case 'r': return 31; break;
  case 'g': return 32; break;
  case 'y': return 33; break;
  case 'b': return 34; break;
  case 'm': return 35; break;
  case 'c': return 36; break;
  case 'w': return 37; break;
  
  case '0': return 40; break;
  case '1': return 44; break;
  case '2': return 42; break;
  case '3': return 46; break;
  case '4': return 41; break;
  case '5': return 45; break;
  case '6': return 43; break;
  case '7': return 47; break;
  
  case 'f': return  1; break;
  case '&': return -1; break;
  default : return  0; break;
  }
}


char *interpret_colors(char *str, bool parse) {
  int clr = 37, bg_clr = 40, flash = 0;
  static char cbuf[MAX_STRING_LENGTH];
  char *cp, *tmp;  
  char i[256];

  if (!strchr(str, '&'))
    return (str);

  cp = cbuf;
  
  for (;;) {
    if (*str == '&') {
      str++;
      if ((clr = is_color(LOWER(*str))) > 0 && parse) {
        if (IS_UPPER(*str)) sprintf(i, "\x1b[1;");
        else                sprintf(i, "\x1b[0;");
          
        if (clr >= 40) {
          bg_clr = 40;
          str++;
          continue;
        } else if (clr == 1) {
          flash = !flash;
          str++;
          continue;
        }
          
        sprintf(i, "%s%s%d;%dm", i, (flash ? "5;" : ""), bg_clr, clr);
        tmp = i;
      } else if (clr == -1) {
        *(cp++) = '&';
        str++;
        continue;
      } else {
        str++;
        continue;
      }
      while ((*cp = *(tmp++)))
	cp++;
      str++;
    } else if (!(*(cp++) = *(str++)))
      break;
  }
  
  *cp = '\0';
  return (cbuf);
}

/* Initialize a descriptor */
void init_descriptor (struct descriptor_data *newd, int desc)
{
  static int last_desc = 0;
	newd->descriptor = desc;
	newd->connected = CON_GET_NAME;
	newd->idle_tics = 0;
	newd->wait = 1;
	newd->output = newd->small_outbuf;
	newd->bufspace = SMALL_BUFSIZE - 1;
	newd->next = descriptor_list;
	newd->login_time = time (0);

	if (++last_desc == 1000)
		last_desc = 1;
	newd->desc_num = last_desc;
}

char *prompt_str(struct char_data *ch) {
  struct char_data *vict = FIGHTING(ch);  
  static char pbuf[MAX_STRING_LENGTH];  
  char *str = GET_PROMPT(ch);
  struct char_data *tank;
  int perc, color;  
  char *cp, *tmp;
  char i[256];
  
  if (!str || !*str)
    str = "&nDaggerfall: &mSet your prompt '&bhelp prompt&m' or '&bDisplay&n'> ";

  color = (PRF_FLAGGED(ch, PRF_COLOR_1 | PRF_COLOR_2) ? 1 : 0);
    
  if (!strchr(str, '%'))
    return (str);
  
  cp = pbuf;
  
  for (;;) {
    if (*str == '%') {
      switch (*(++str)) {
      case 'h': // current hitp
        sprintf(i, "%d", GET_HIT(ch));
        tmp = i;
        break;
      case 'H': // maximum hitp
        sprintf(i, "%d", GET_MAX_HIT(ch));
        tmp = i;
        break;
      case 'm': // current mana
        sprintf(i, "%d", GET_MANA(ch));
        tmp = i;
        break;
      case 'M': // maximum mana
        sprintf(i, "%d", GET_MAX_MANA(ch));
        tmp = i;
        break;
      case 'v': // current moves
        sprintf(i, "%d", GET_MOVE(ch));
        tmp = i;
        break;
      case 'V': // maximum moves
        sprintf(i, "%d", GET_MAX_MOVE(ch));
        tmp = i;
        break;
      case 'P':
      case 'p': // percentage of hitp/move/mana
        str++;
        switch (LOWER(*str)) {
        case 'h':
          perc = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
          break;
        case 'm':
          perc = (100 * GET_MANA(ch)) / GET_MAX_MANA(ch);
          break;
        case 'v':
          perc = (100 * GET_MOVE(ch)) / GET_MAX_MOVE(ch);
          break;
        case 'x':
          perc = (100 * GET_EXP(ch)) / (exp_to_level(ch));
          break;
        default :
          perc = 0;
          break;
        }
        sprintf(i, "%d%%", perc);
        tmp = i;
        break;
      case 'O':
      case 'o': // opponent
        if (vict) {
          perc = (100*GET_HIT(vict)) / GET_MAX_HIT(vict);
          sprintf(i, "%s (%s)&n", PERS(vict, ch),
                  (perc >= 95 ? "unscathed" :
                   perc >= 75 ? "scratched" :
                   perc >= 50 ? "beaten-up" :
                   perc >= 25 ? "bloody"    : "near death"));
          tmp = i;
        } else {
          str++;
          continue;
        }
        break;
      case 'x': // current exp
        sprintf(i, "%d", GET_EXP(ch));
        tmp = i;
        break;
      case 'X': // exp to level
        sprintf(i, "%d", exp_to_level(ch));
        tmp = i;
        break;
      case 'g': // gold on hand
        sprintf(i, "%d", GET_GOLD(ch));
        tmp = i;
        break;
      case 'G': // gold in bank
        sprintf(i, "%d", GET_BANK_GOLD(ch));
        tmp = i;
        break;
      case 'T':
      case 't': // tank
        if (vict && (tank = FIGHTING(vict)) && tank != ch) {
          perc = (100*GET_HIT(tank)) / GET_MAX_HIT(tank);
          sprintf(i, "%s (%s)", PERS(tank, ch),
                  (perc >= 95 ? "unscathed" :
                   perc >= 75 ? "scratched" :
                   perc >= 50 ? "beaten-up" :
                   perc >= 25 ? "bloody"    : "near death"));
          tmp = i;
        } else {
          str++;
          continue;
        }
        break;
        
      case '_':
        tmp = "\r\n";
        break;
      case '%':
        *(cp++) = '%';
        str++;
        continue;
        break;
      
      default :
        str++;
        continue;
        break;
      }
      
      while ((*cp = *(tmp++)))
        cp++;
      str++;
    } else if (!(*(cp++) = *(str++)))
      break;
  }
  
  *cp = '\0';
  
  strcat(pbuf, " &n");
  return (pbuf);
}


void make_prompt(struct descriptor_data *d)
{
  char * short_char_cond(struct char_data *);
  char prompt[MAX_INPUT_LENGTH];

   /* reversed these top 2 if checks so that page_string() would work in */
   /* the editor */
if (d->showstr_count) {
    sprintf(prompt,
	    "\r[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]",
            d->showstr_page, d->showstr_count);
      write_to_descriptor(d->descriptor, prompt);
    } else if (!d->connected) {
      if (IS_NPC(d->character))
      {
        sprintf(prompt, "MOB> ");
      }
      else
      {
        if(GET_LEVEL(d->character) >= LVL_IMMORT &&
           GET_INVIS_LEV(d->character) > 0)
             sprintf(prompt, "i%d %s",GET_INVIS_LEV(d->character), 
                     prompt_str(d->character));
        else
          sprintf(prompt, "%s", prompt_str(d->character));
      }
    proc_color(prompt, (clr(d->character, C_NRM)));
    if (!PLR_FLAGGED(d->character, PLR_WRITING))
      write_to_descriptor(d->descriptor, prompt);
  }
}


void write_to_q(char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *new;

  CREATE(new, struct txt_block, 1);
  CREATE(new->text, char, strlen(txt) + 1);
  strcpy(new->text, txt);
  new->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    new->next = NULL;
    queue->head = queue->tail = new;
  } else {
    queue->tail->next = new;
    queue->tail = new;
    new->next = NULL;
  }
}



int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return 0;

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return 1;
}



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
}

/* Add a new string to a player's output queue */
void write_to_output(const char *txt, struct descriptor_data *t)
{
  int size;

  size = strlen(txt);

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size) {
    strcpy(t->output + t->bufptr, txt);
    t->bufspace -= size;
    t->bufptr += size;
    return;
  }
  /*
   * If we're already using the large buffer, or if even the large buffer
   * is too small to handle this new text, chuck the text and switch to the
   * overflow state.
   */
  if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
    t->bufptr = -1;
    buf_overflows++;
    return;
  }
  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else {			/* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
  t->output = t->large_outbuf->text;	/* make big buffer primary */
  strcat(t->output, txt);	/* now add new text */

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - strlen(t->output);

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);
}



/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


int new_descriptor(int s) {
  socket_t desc;
  int sockets_connected = 0;
  unsigned long addr;
  int i;
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;
  extern char *GREETINGS;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET) {
    perror("accept");
    return -1;
  }
  /* keep it from blocking */
  nonblock(desc);

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= max_players) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... please try again later!\r\n");
    CLOSE_SOCKET(desc);
    return 0;
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the sitename */
  if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
				      sizeof(peer.sin_addr), AF_INET))) {

    /* resolution failed */
    if (!nameserver_is_slow)
      perror("gethostbyaddr");

    /* find the numeric site address */
    addr = ntohl(peer.sin_addr.s_addr);
    sprintf(newd->host, "%03u.%03u.%03u.%03u", (int) ((addr & 0xFF000000) >> 24),
     (int) ((addr & 0x00FF0000) >> 16), (int) ((addr & 0x0000FF00) >> 8),
	    (int) ((addr & 0x000000FF)));
  } else {
    strncpy(newd->host, from->h_name, HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    CLOSE_SOCKET(desc);
    sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
    mudlog(buf2, CMP, LVL_GOD, TRUE);
    free(newd);
    return 0;
  }
#if 0
  /* Log new connections - probably unnecessary, but you may want it */
  sprintf(buf2, "New connection from [%s]", newd->host);
  mudlog(buf2, CMP, LVL_GOD, FALSE);
#endif
  init_descriptor(newd, desc);

  /* prepend to list */
  descriptor_list = newd;

  SEND_TO_Q(GREETINGS, newd);

  return 0;
}



int process_output(struct descriptor_data *t)
{
  static char i[LARGE_BUFSIZE + GARBAGE_SPACE];
  static int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");

  /* now, append the 'real' output */
  strcpy(i + 2, t->output);
  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    strcat(i, "**OVERFLOW**");

  /* add the extra CRLF if the person isn't in compact mode */
  if (!t->connected && t->character && !PRF_FLAGGED(t->character, PRF_COMPACT))
    strcat(i + 2, "\r\n");
  if(t->character)
     proc_color(i, (clr(t->character, C_NRM)));

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (!t->prompt_mode)		/* && !t->connected) */
    result = write_to_descriptor(t->descriptor, i);
  else
    result = write_to_descriptor(t->descriptor, i + 2);

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  return result;
}



int write_to_descriptor(socket_t desc, char *txt)
{
  int total, bytes_written;

  total = strlen(txt);

  do {
#ifdef CIRCLE_WINDOWS
    if ((bytes_written = send(desc, txt, total, 0)) < 0) {
      if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
    if ((bytes_written = write(desc, txt, total)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno == EAGAIN)
#endif /* CIRCLE_WINDOWS */
	log("process_output: socket write would block, about to close");
      else
	perror("Write to socket");
      return -1;
    } else {
      txt += bytes_written;
      total -= bytes_written;
    }
  } while (total > 0);

  return 0;
}


/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data *t)
{
  int buf_length, bytes_read, space_left, failed_subst;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH + 8];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      log("process_input: about to close connection: input overflow");
      return -1;
    }
#ifdef CIRCLE_WINDOWS
    if ((bytes_read = recv(t->descriptor, read_point, space_left, 0)) < 0) {
      if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
    if ((bytes_read = read(t->descriptor, read_point, space_left)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno != EAGAIN) {
#endif /* CIRCLE_WINDOWS */
	perror("process_input: about to lose connection");
	return -1;		/* some error condition was encountered on
				 * read */
      } else
	return 0;		/* the read would have blocked: just means no
				 * data there but everything's okay */
    } else if (bytes_read == 0) {
      log("EOF on socket read (connection broken by peer)");
      return -1;
    }
    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';	/* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if !defined(POSIX_NONBLOCK_BROKEN)
  } while (nl_pos == NULL);
#else
  } while (0);

  if (nl_pos == NULL)
    return 0;
#endif /* POSIX_NONBLOCK_BROKEN */

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b') {	/* handle backspacing */
	if (write_point > tmp) {
	  if (*(--write_point) == '$') {
	    write_point--;
	    space_left += 2;
	  } else
	    space_left++;
	}
      } else if (isascii(*ptr) && isprint(*ptr)) {
	if ((*(write_point++) = *ptr) == '$') {		/* copy one character */
	  *(write_point++) = '$';	/* if it's a $, double it */
	  space_left -= 2;
	} else
	  space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
	return -1;
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if (*tmp == '!')
      strcpy(tmp, t->last_input);
    else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);
    } else
      strcpy(t->last_input, tmp);

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return 1;
}



/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char new[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(new, orig, (strpos - orig));
  new[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(new, second, (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(new, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* terminate the string in case of an overflow from strncat */
  new[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, new);

  return 0;
}



void close_socket(struct descriptor_data *d)
{
  char buf[128];
  struct descriptor_data *temp;
  long target_idnum = -1;

  CLOSE_SOCKET(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }

  /*. Kill any OLC stuff .*/
  switch(d->connected)
  { case CON_OEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
      cleanup_olc(d, CLEANUP_ALL);
      break;
    case CON_CLAN_EDIT:
      cleanup_clan_edit(d, CLEAN_ALL);
      break;
    default:
      break;
  }

  if (d->character) {
    target_idnum = GET_IDNUM(d->character);
    if (d->connected == CON_PLAYING) {
      save_char(d->character, NOWHERE);
/*    act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM); */
      sprintf(buf,"%s has slipped into the RL dimension.\r\n",
              GET_NAME(d->character));
      send_to_all(buf);
      sprintf(buf, "Closing link to: %s.", GET_NAME(d->character));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
      d->character->desc = NULL;
    } else {
      sprintf(buf, "Losing player: %s.",
	      GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      mudlog(buf, CMP, LVL_IMMORT, TRUE);
      free_char(d->character);
    }
  } else
    mudlog("Losing descriptor without char.", CMP, LVL_IMMORT, TRUE);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  REMOVE_FROM_LIST(d, descriptor_list, next);

  if (d->showstr_head)
    free(d->showstr_head);
  if (d->showstr_count)
    free(d->showstr_vector);
  if (d->storage)
    free(d->storage);

  free(d);
}



void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    } else {
      echo_on(d);
      SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
      STATE(d) = CON_CLOSE;
    }
  }
}



/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#ifdef CIRCLE_WINDOWS

void nonblock(socket_t s)
{
  long val;

  val = 1;
  ioctlsocket(s, FIONBIO, &val);
}

#else

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}


/* ******************************************************************
*  signal-handling functions (formerly signals.c)                   *
****************************************************************** */


RETSIGTYPE checkpointing()
{
  if (!tics) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated");
    abort();
  } else
    tics = 0;
}


RETSIGTYPE reread_wizlists()
{
  void reboot_wizlists(void);

  mudlog("Signal received - rereading wizlists.", CMP, LVL_IMMORT, TRUE);
  reboot_wizlists();
}


RETSIGTYPE unrestrict_game()
{
  extern struct ban_list_element *ban_list;
  extern int num_invalid;

  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	 BRF, LVL_IMMORT, TRUE);
  ban_list = NULL;
  game_restrict = 0;
  num_invalid = 0;
}


RETSIGTYPE hupsig()
{
  log("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  exit(0);			/* perhaps something more elegant should
				 * substituted */
}


/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;

  return oact.sa_handler;
}
#endif				/* NeXT */


void signal_setup(void)
{
#ifndef CIRCLE_OS2
  struct itimerval itime;
#endif
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.  Doesn't work with
   * OS/2.
   */
#ifndef CIRCLE_OS2
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);
#endif

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);

#ifdef CIRCLE_OS2
#if defined(SIGABRT)
  my_signal(SIGABRT, hupsig);
#endif
#if defined(SIGFPE)
  my_signal(SIGFPE, hupsig);
#endif
#if defined(SIGILL)
  my_signal(SIGILL, hupsig);
#endif
#if defined(SIGSEGV)
  my_signal(SIGSEGV, hupsig);
#endif
#endif				/* CIRCLE_OS2 */

}

#endif				/* CIRCLE_WINDOWS */


/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

void send_to_char(char *messg, struct char_data *ch)
{
  if (ch->desc && messg)
    SEND_TO_Q(messg, ch->desc);
}


void send_to_all(char *messg)
{
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
	SEND_TO_Q(messg, i);
}


void send_to_outdoor(char *messg)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && AWAKE(i->character) &&
	OUTSIDE(i->character))
      SEND_TO_Q(messg, i);
}



void send_to_room(char *messg, int room)
{
  struct char_data *i;

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if (i->desc)
	SEND_TO_Q(messg, i->desc);
}



char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(char *orig, struct char_data *ch, struct obj_data *obj,
		 void *vict_obj, struct char_data *to)
{
  register char *i = NULL, *buf;
  static char lbuf[MAX_STRING_LENGTH];

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
	i = PERS(ch, to);
	break;
      case 'N':
	CHECK_NULL(vict_obj, PERS((struct char_data *) vict_obj, to));
	break;
      case 'm':
	i = HMHR(ch);
	break;
      case 'M':
	CHECK_NULL(vict_obj, HMHR((struct char_data *) vict_obj));
	break;
      case 's':
	i = HSHR(ch);
	break;
      case 'S':
	CHECK_NULL(vict_obj, HSHR((struct char_data *) vict_obj));
	break;
      case 'e':
	i = HSSH(ch);
	break;
      case 'E':
	CHECK_NULL(vict_obj, HSSH((struct char_data *) vict_obj));
	break;
      case 'o':
	CHECK_NULL(obj, OBJN(obj, to));
	break;
      case 'O':
	CHECK_NULL(vict_obj, OBJN((struct obj_data *) vict_obj, to));
	break;
      case 'p':
	CHECK_NULL(obj, OBJS(obj, to));
	break;
      case 'P':
	CHECK_NULL(vict_obj, OBJS((struct obj_data *) vict_obj, to));
	break;
      case 'a':
	CHECK_NULL(obj, SANA(obj));
	break;
      case 'A':
	CHECK_NULL(vict_obj, SANA((struct obj_data *) vict_obj));
	break;
      case 'T':
	CHECK_NULL(vict_obj, (char *) vict_obj);
	break;
      case 'F':
	CHECK_NULL(vict_obj, fname((char *) vict_obj));
	break;
      case '$':
	i = "$";
	break;
      default:
	log("SYSERR: Illegal $-code to act():");
	strcpy(buf1, "SYSERR: ");
	strcat(buf1, orig);
	log(buf1);
	break;
      }
      while ((*buf = *(i++)))
	buf++;
      orig++;
    } else if (!(*(buf++) = *(orig++)))
      break;
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  SEND_TO_Q(CAP(lbuf), to->desc);
}


#define SENDOK(ch) ((ch)->desc && (AWAKE(ch) || sleep) && \
		    !PLR_FLAGGED((ch), PLR_WRITING))

void act(char *str, int hide_invisible, struct char_data *ch,
	 struct obj_data *obj, void *vict_obj, int type)
{
  struct char_data *to;
  static int sleep;
  struct descriptor_data *i;

  if (!str || !*str)
    return;

  /*
   * Warning: the following TO_SLEEP code is a hack.
   * 
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((sleep = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;


  if (type == TO_CHAR) {
    if (ch && SENDOK(ch))
      perform_act(str, ch, obj, vict_obj, ch);
    return;
  }
  if (type == TO_VICT) {
    if ((to = (struct char_data *) vict_obj) && SENDOK(to))
      perform_act(str, ch, obj, vict_obj, to);
    return;  
  }

  if (type == TO_WORLD) {
    for (i = descriptor_list; i; i = i->next) {
      if (!i->connected && i != ch->desc && i->character &&
          !PLR_FLAGGED(i->character, PLR_WRITING) &&
	  !PRF_FLAGGED(i->character, PRF_NOGOSS) &&
          !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {
        send_to_char("[&yEmote&n] ", i->character);
        perform_act(str, ch, obj, vict_obj, i->character);
      }
    }
  }
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && ch->in_room != NOWHERE)
    to = world[ch->in_room].people;
  else if (obj && obj->in_room != NOWHERE)
    to = world[obj->in_room].people;
  else {
    log("SYSERR: no valid target to act()!");
    return;
  }

  for (; to; to = to->next_in_room)
    if (SENDOK(to) && !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
	(to != ch) && (type == TO_ROOM || (to != vict_obj)))
      perform_act(str, ch, obj, vict_obj, to);
}
