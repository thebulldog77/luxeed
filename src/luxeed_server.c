
#include "luxeed.h"
#include "luxeed_server.h"
#include "luxeed_endpoint.h"
#include <ctype.h>


/********************************************************************/


/********************************************************************/

static char *parse_word(char **buf)
{
  char *s = *buf;
  char *b = 0;

  if ( ! s )
    return 0;

  while ( *s && isspace(*s) )
    ++ s;

  if ( *s ) {
    b = s;
    while ( *s && ! isspace(*s) )
      ++ s;
    *(s ++) = '\0';
  }

  while ( *s && isspace(*s) )
    ++ s;

  *buf = s;
  
  return b;
}


int luxeed_client_read_command(luxeed_client *cli)
{
  int result = 0;
  char buf[2048];
  size_t buf_size = sizeof(buf);
  const unsigned char *pixel = 0;
  const char *error = 0;
  const char *error2 = "";

  PDEBUG(cli, "(%p)", cli);

  /* Force buffering only up till newline so that select() still
  ** has other read(0) pending.
  */
  setlinebuf(cli->ep.in);

  memset(buf, 0, buf_size);
  if ( fgets(buf, buf_size - 1, cli->ep.in) == 0 ) {
    result = -1;
  } else {
    char out_buf[1024];
    size_t out_buf_size = sizeof(out_buf);
    char *s = buf;
    char *cmd = parse_word(&s);
    char *word = 0;
    int force_output = 0;
    error = 0;

    if ( cli->opts.debug ) {
      fprintf(stderr, "read command from %d: %s %s\n", cli->ep.in_fd, cmd, s);
    }

    memset(out_buf, 0, sizeof(out_buf));

    if ( 0 ) {
      fprintf(stderr, "cmd = %p '%s'\n", cmd, cmd);
      fprintf(stderr, "cmd = '%c'\n", cmd && cmd[0]);
    }

    /* Blank line? */
    if ( ! cmd ) {
      return 0;
    }

    switch ( cmd[0] ) {
    case '\0': case '#':
      break;

    case 'h':
      if ( cli->ep.out ) {
	fprintf(cli->ep.out, "\
help: \n\
g <key-id>     : get key's current color. \n\
s r g b <key-id> ... : set key(s) to current color. \n\
u              : update keyboard colors. \n\
w <n>          : wait for n microseconds. \n\
\n\
");
      }
      break;

    case 'v':
      if ( (word = parse_word(&s)) ) {
	cli->opts.verbose = atoi(word);
      }
      break;

    case 's': // set r g b key_id ...
      /* r */
      if ( (word = parse_word(&s)) ) {
	sscanf(word, "%2x", &cli->color[0]);
      }
      /* g */
      if ( (word = parse_word(&s)) ) {
	sscanf(word, "%2x", &cli->color[1]);
      }
      /* b */
      if ( (word = parse_word(&s)) ) {
	sscanf(word, "%2x", &cli->color[2]);
      }

      while ( (word = parse_word(&s)) ) {
	luxeed_key *key = luxeed_device_key_by_name(cli->srv->dev, word);
	if ( key && (pixel = luxeed_device_set_key_color(cli->srv->dev, key, cli->color[0], cli->color[1], cli->color[2])) ) {
	  if ( cli->opts.debug > 1 ) {
	    fprintf(stderr, "word %s => key->id = %d, key->name[0] = %s\n", word, key->id, key->name[0]);
	  }
	  snprintf(out_buf, out_buf_size, "%s %x %x %x #%d", cmd, pixel[0], pixel[1], pixel[2], key->id);
	} else {
	  error = "BAD KEY";
	  error2 = word;
	}
      }
      break;

    case 'g': // get key_id
      while ( (word = parse_word(&s)) ) {
	luxeed_key *key = luxeed_device_key_by_name(cli->srv->dev, word);
	if ( key && (pixel = cli->srv->dev ? luxeed_device_pixel(cli->srv->dev, key->id) : 0) ) {
	  if ( cli->opts.debug > 1 ) {
	    fprintf(stderr, "word %s => key->id = %d, key->name[0] = %s\n", word, key->id, key->name[0]);
	  }
	  snprintf(out_buf, out_buf_size, "%s %x %x %x #%d", cmd, pixel[0], pixel[1], pixel[2], key->id);
	  force_output = 1;
	} else {
	  error = "BAD KEY";
	  error2 = word;
	}
      }
      break;

    case 'u': // update
      if ( cli->srv->dev ) {
	/* Do not force update. */
	if ( luxeed_device_update(cli->srv->dev, 0) ) {
	  error = "UPDATE FAILED";
	}
      }
      break;

    case 'w': // wait
      {
	double wait = 1.0;
	sscanf(s, "%lg", &wait);
	luxeed_client_sleep(cli, wait);
	result = 1;  /* Prevent caller from enabling read event. */
      }
      break;

    default:
      error = "BAD COMMAND";
      break;
    }

    if ( cli->ep.out ) {
      if ( error ) {
	fprintf(cli->ep.out, "ERROR %s %s %s\n", cmd, error, error2);
      } else {
	if ( cli->opts.verbose || force_output ) {
	  if ( out_buf[0] ) {
	    fprintf(cli->ep.out, "OK %s\n", out_buf);
	  } else {
	    fprintf(cli->ep.out, "OK %s\n", cmd);
	  }
	}
      }
      
      fflush(cli->ep.out);
    }
  }

  PDEBUG(cli, "(%p) => %d", cli, result);

  return result;
}



int luxeed_client_sleep(luxeed_client *cli, double sec)
{
  int result = 0;

  PDEBUG(cli, "(%p, %g)", cli, sec);

  if ( sec < 0 ) {
    return result;
  }

  /* Stop reading from client. */
  event_del(&cli->ep.read_ev);

  /* Add the timer. */
  cli->ep.timer_timeval.tv_sec = (time_t) sec;
  cli->ep.timer_timeval.tv_usec = (long) ((sec - (time_t) sec) * 1000000.0);
  evtimer_add(&cli->ep.timer_ev, &cli->ep.timer_timeval);

  PDEBUG(cli, "(%p, %g) => %d", cli, sec, result);

  return result;
}


void luxeed_client_sleep_finished(int fd, short event, void *data)
{
  luxeed_client *cli = data;
  int result = 0;

  PDEBUG(cli, "(%d, %d, %p)", (int) fd, (int) event, data);

  /* Remove the timer. */
  evtimer_del(&cli->ep.timer_ev);

  /* Continue reading from client. */
  event_add(&cli->ep.read_ev, 0);

  PDEBUG(cli, "(%d, %d, %p) => %d", (int) fd, (int) event, data, result);
}


/********************************************************************/


void luxeed_client_read(int fd, short event, void *data)
{
  luxeed_client *cli = data;
  int result = 0;

  PDEBUG(cli, "(%d, %d, %p)", (int) fd, (int) event, data);

  result = luxeed_client_read_command(cli);

  if ( result < 0 ) {
    luxeed_client_close(cli);
    free(cli);
  } 
  else if ( result == 0 ) {
    event_add(&cli->ep.read_ev, 0);
  }

  PDEBUG(cli, "(%d, %d, %p) => %d", (int) fd, (int) event, data, result);
}


int luxeed_client_open(luxeed_client *cli, luxeed_server *srv, int in_fd, int out_fd)
{
  int result = 0;

  do {
    PDEBUG(cli, "(%p, %p, %d, %d)", cli, srv, (int) in_fd, (int) out_fd);

    cli->srv = srv;
    cli->opts = srv->opts;
    cli->ep.opts = cli->opts;

    luxeed_endpoint_open(&cli->ep, in_fd, out_fd);

    cli->color[0] = cli->color[1] = cli->color[2] = 0;
    
    /* Start reading on cli socket. */
    event_set(&cli->ep.read_ev, cli->ep.in_fd, EV_READ, &luxeed_client_read, cli); 
    event_add(&cli->ep.read_ev, 0);

    /* Prepare a timer for "wait" commands. */
    evtimer_set(&cli->ep.timer_ev, luxeed_client_sleep_finished, (void *) cli);
  
  } while ( 0 );

  PDEBUG(cli, "(%p, %p, %d, %d) => %d", cli, srv, (int) in_fd, (int) out_fd, (int) result);

  return result;
}


int luxeed_client_close(luxeed_client *cli)
{
  int result = 0;

  PDEBUG(cli, "(%p)", cli);

  if ( ! cli ) return 0;

  /* Remove timer event. */
  evtimer_del(&cli->ep.timer_ev);

  /* Stop read events. */
  event_del(&cli->ep.read_ev);

  /* Close FDs and FILEs. */
  luxeed_endpoint_close(&cli->ep);

  PDEBUG(cli, "(%p) => %d", cli, result);

  return result;
}



/********************************************************************/


void luxeed_server_accept(int fd, short event, void *data)
{
  int result = 0;
  luxeed_server *srv = data;
  luxeed_client *cli = 0;

  PDEBUG(srv, "(%d, %d, %p)", fd, event, data);

  do {
    /* Accept another. */
    event_add(&srv->ep.accept_ev, 0);
    
    /* Open client. */
    cli = malloc(sizeof(*cli));
    memset(cli, 0, sizeof(*cli));

    /* Accept client */
    if ( luxeed_endpoint_accept(&srv->ep, &cli->ep) < 0 ) {
      perror(luxeed_error_action = "luxeed_endpoint_accept()");
      result = -1;
      break;
    }

    luxeed_client_open(cli, srv, cli->ep.in_fd, cli->ep.in_fd);
  } while ( 0 );

  if ( result ) {
    luxeed_client_close(cli);
  }

  PDEBUG(srv, "(%p) => %d", srv, result);

}


/********************************************************************/


int luxeed_server_open_socket(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    if ( (result = luxeed_endpoint_bind(&srv->ep)) ) {
      perror(luxeed_error_action = "luxeed_endpoint_bind");
      break;
    }

    /* Register for accept events. */
    event_set(&srv->ep.accept_ev, srv->ep.in_fd, EV_READ, &luxeed_server_accept, srv); 
    event_add(&srv->ep.accept_ev, 0);
  } while ( 0 );

  PDEBUG(srv, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_open_inet(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    /* Create socket address. */
    srv->ep.socket_family = AF_INET;

    result = luxeed_server_open_socket(srv);
  } while ( 0 );

  PDEBUG(srv, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_open_uds(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    /* Create socket address. */
    srv->ep.socket_family = AF_UNIX;

    /* Delete old file. */
    {
      struct stat s;

      if ( stat(srv->opts.uds, &s) == 0 ) {
	if ( unlink(srv->opts.uds) < 0 ) {
	  perror(luxeed_error_action = "unlink");
	  // result = -1;
	  // break;
	}
      }
    }

    result = luxeed_server_open_socket(srv);
  } while ( 0 );

  PDEBUG(srv, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_open_fifo(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    /* Create a named pipe (fifo). */
    if ( mknod(srv->server_path, S_IFIFO | 0666, 0) < 0 ) {
      perror(luxeed_error_action = "mknod");
      // result = -1;
      // break;
    }

    do {
      luxeed_client *cli;
      int client_fd;

      PDEBUG(srv, ": open(%s) ...", srv->server_path);
      
      if ( (client_fd = open(srv->server_path, O_RDONLY)) < 0 ) {
	perror(luxeed_error_action = "open");
	result = -1;
	break;
      }

      PDEBUG(srv, ": open(%s) => %d", srv->server_path, client_fd);

      /* Open client. */
      cli = malloc(sizeof(*cli));
      memset(cli, 0, sizeof(*cli));

      luxeed_client_open(cli, srv, client_fd, -1);

      /* Process events. */
      event_dispatch();
    } while ( 1 );
  } while ( 0 );

  PDEBUG(srv, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_open(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    // strcpy(srv->server_uri, "tcp://127.0.0.1:25324");
    strncpy(srv->server_path, "/tmp/luxeed", sizeof(srv->server_path));

    if ( ! srv->dev ) {
      srv->dev = luxeed_device_create();
      srv->dev->opts = srv->opts;
    }

    /* Find the luxeed device. */
    if ( luxeed_device_find(srv->dev, 0, 0) ) {
      fprintf(stderr, "%s: luxeed keyboard not found\n", srv->opts.progname);
      luxeed_error_action = "luxeed_device_find";
      // result = -1;
      // break;
    }

    /* Attempt to open the device now. */
    if ( srv->dev ) {
      if ( (result = luxeed_device_open(srv->dev)) ) {
	luxeed_error_action = "luxeed_device_open";
	// result = -1;
	// break;
      }
    }

    srv->ep.opts = srv->opts;

    if ( srv->opts.fifo ) {
      if ( luxeed_server_open_fifo(srv) ) {
	luxeed_error_action = "luxeed_server_open_fifo";
	result = -1;
	break;
      }
    }

    if ( srv->opts.uds ) {
      if ( luxeed_server_open_uds(srv) ) {
	luxeed_error_action = "luxeed_server_open_uds";
	result = -1;
	break;
      }
    }

    if ( srv->opts.host ) {
      if ( luxeed_server_open_inet(srv) ) {
	luxeed_error_action = "luxeed_server_open_inet";
	result = -1;
	break;
      }
    }

  } while ( 0 );

  if ( result ) {
    luxeed_server_close(srv);
  }

  PDEBUG(srv, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_close(luxeed_server *srv)
{
  int result = 0;

  if ( ! srv ) return 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    event_del(&srv->ep.accept_ev);

    if ( srv->dev ) {
      luxeed_device_destroy(srv->dev);
    }

    unlink(srv->server_path);

    /* Close the device. */
    luxeed_device_destroy(srv->dev);
    srv->dev = 0;
  } while ( 0 );

  PDEBUG(srv, "(%p) => %d", srv, result);

  return result;
}


/********************************************************************/


int luxeed_server_main(int argc, char **argv)
{
  int result = 0;
  luxeed_server _srv, *srv = &_srv;
 
  do {
    /* Initialize libevent. */
    event_init();
    
    /* Initialize server. */
    memset(srv, 0, sizeof(*srv));

    srv->opts = luxeed_options;
    // srv->opts.debug = 0;

    if ( ! srv->opts.host ) {
      srv->opts.host = "127.0.0.1";
    }
    if ( ! srv->opts.port ) {
      srv->opts.port = 12345;
    }

    /* Open server and device. */
    result = luxeed_server_open(srv);
    if ( result ) break;

    /* Process events. */
    event_dispatch();

  } while ( 0 );

  if ( result ) {
    fprintf(stderr, "%s: error: %d\n", luxeed_options.progname, result);
    perror(luxeed_error_action ? luxeed_error_action : luxeed_options.progname);
  }

  /* Close server and device. */
  luxeed_server_close(srv);

  return result != 0 ? 1 : 0;
}


/* EOF */

