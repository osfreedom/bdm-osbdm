/*
 * Motorola Background Debug Mode Remote Server
 * Copyright (C) 1994, 95, 96, 1997 Free Software Foundation, Inc.
 * Contributed by Chris Johns
 *
 * Pieces of code are taken from the gnatsd amd gdbserver.
 *
 * 31-11-1999 Chris Johns (ccj@acm.org)
 *
 * Chris Johns
 * Objective Design Systems
 * 35 Cairo Street
 * Cammeray, Sydney, 2062, Australia
 *
 * ccj@acm.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <BDMlib.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <syslog.h> 
#include <unistd.h>

#ifndef __CYGWIN32__
#include <netinet/tcp.h>
#endif

char *xmalloc (unsigned n);
void xfree (char *p);

/*
 * This should be the same as the client.
 */

#define BDM_SERVER_TIMEOUT   (5)
#define BDM_SERVER_BUF_SIZE  (4096)

/*
 * Local data.
 */
static const char *version_string = "0.0";
static char       myname[MAXHOSTNAMELEN];
static char       *current_host;
static char       *current_addr;
static char       *program_name;
static int        debug = 0;

/*
 * Define the messages between the client and the server.
 */

enum bdm_message_id
{
  HELO,
  SRVCTL,
  IOINT,
  IOCMD,
  IOIO,
  READ,
  WRITE,
  QUIT,
  INVALID
};
  
struct bdm_message
{
  char                *label;
  enum bdm_message_id id;
};

struct bdm_message messages[] =
{
  { "HELO",   HELO },
  { "SRVCTL", SRVCTL },
  { "IOINT",  IOINT },
  { "IOCMD",  IOCMD },
  { "IOIO",   IOIO},
  { "READ",   READ },
  { "WRITE",  WRITE },
  { "QUIT",   QUIT }
};

/*
 * Ioctl code translation.
 */
static const int ioctl_code_table[] = {
  BDM_INIT,
  BDM_RESET_CHIP,
  BDM_RESTART_CHIP,
  BDM_STOP_CHIP,
  BDM_STEP_CHIP,
  BDM_GET_STATUS,
  BDM_SPEED,
  BDM_DEBUG,
  BDM_RELEASE_CHIP,
  BDM_GO,
  BDM_READ_REG,
  BDM_READ_SYSREG,
  BDM_READ_LONGWORD,
  BDM_READ_WORD,
  BDM_READ_BYTE,
  BDM_WRITE_REG,
  BDM_WRITE_SYSREG,
  BDM_WRITE_LONGWORD,
  BDM_WRITE_WORD,
  BDM_WRITE_BYTE,
  BDM_GET_DRV_VER,
  BDM_GET_CPU_TYPE,
  BDM_GET_IF_TYPE
};

/*
 * Get the name of the host.
 */

char *
get_name (struct in_addr *host)
{
  char           *buf;
  int            i;
  struct hostent *hp;
#ifdef h_addr
  char           **pp;
#endif

  hp = gethostbyaddr ((char *) host, sizeof (host), AF_INET);
  
  if (hp == NULL)
    return NULL;
  
  i = strlen (hp->h_name);
  buf = (char *) xmalloc (i + 1);
  strcpy (buf, hp->h_name);
  hp = gethostbyname (buf);
  if (hp == NULL)
    return NULL;

#ifdef h_addr
  for (pp = hp->h_addr_list; *pp; pp++)
    if (memcmp ((char *) &host->s_addr, (char *) *pp, hp->h_length) == 0)
      break;
  if (*pp == NULL)
    return NULL;
#else
  if (memcmp ((char *) &host->s_addr, (char *) hp->h_addr, hp->h_length) != 0)
    return NULL;
#endif

  return buf;
}

/*
 * Start an inetd connection.
 */

void
start_connection ()
{
  struct sockaddr_in s;
  int                len;
  
  len = sizeof (s);
  
  if (getpeername (0, (struct sockaddr *) &s, &len) < 0)
  {
    if (!isatty (0))
    {
      syslog (LOG_ERR, "%s: can't get peername %m", "?");
      exit (1);
    }
    current_host = "stdin";
    current_addr = current_host;
  }
  else
  {
    if (s.sin_family != AF_INET)
    {
      syslog (LOG_ERR, "%s: bad address family %ld",
              "?", (long) s.sin_family);
      exit (1);
    }
    
    current_addr = (char *) inet_ntoa (s.sin_addr);
    current_host = get_name (&s.sin_addr);
    
    if (!current_host) {
      current_host = current_addr;
    }
  }
}

/*
 * Message the message from the input handle.
 */

int
get_message (char *buf, int buf_len)
{
  int    numfds;
  fd_set readfds;

  while (1) {
    FD_ZERO (&readfds);
    FD_SET (0, &readfds);
  
    numfds = select (0 + 1, &readfds, 0, 0, 0);

    if (numfds > 0) {
      return read (0, buf, buf_len);
    }
  }
}

/*
 * Decode the id to an ioctl number.
 *
 * Note, watch checking the return value. Check for -1 not < 0.
 *
 * We should have version number checks between the client and server
 * to avoid these numbers skewing.
 */

int
decode_id (int id)
{
  if (id < (sizeof (ioctl_code_table) / sizeof (int)))
    return ioctl_code_table[id];

  errno = EINVAL;
  return -1;
}

/*
 * Decode a message.
 */

enum bdm_message_id
decode_message (char *message)
{
  int  id;
  char *c;
  char *s;

  /*
   * Make the first field (the message label) uppercase.
   */
   
  s = strchr (message, ' ');
  if (!s)
    s = message + strlen (message);
  for (c = message; c < s; c++)
    *c = toupper (*c);
  
  for (id = 0; id < (sizeof messages / sizeof (struct bdm_message)); id++)
    if (strncmp (message, messages[id].label, strlen (messages[id].label)) == 0)
      return messages[id].id;
  
  syslog (LOG_INFO, "invalid message: %s", message);
  
  return INVALID;
}
    
/*
 * Open the port using the requested device.
 */

void
helo (char *device)
{
  device += sizeof "HELO";
  
  if (bdmOpen (device) < 0) {
    syslog (LOG_INFO, "open error: %s (%d), opening `%s'",
            bdmErrorString (), errno, device);
  }
  printf ("HELO %d %s BDM server %s ready.", errno, myname, version_string);
}

/*
 * Server Control.
 */

void
server_control (char *message)
{
  char *s = message + sizeof "SRVCTL";
  
  if (strncmp (s, "DEBUG", sizeof "DEBUG" - 1) == 0) {
    s = strchr (s, '=');
    if (!s) {
      syslog (LOG_INFO, "srv-ctl error: invalid debug syntax (%s)", message);
      return;
    }
    s++;
    debug = strtoul (s, NULL, 0);
    syslog (LOG_INFO, "srv-ctl: debug level is %d", debug);
  }
}

/*
 * IO Int read/write.
 */

void
ioint (char *message)
{
  int id;
  int code;
  int var;

  message += sizeof "IOINT";
  id       = strtoul (message, NULL, 0);
  message  = strchr (message, ',') + 1;
  var      = strtoul (message, NULL, 0);

  code = decode_id (id);
  if (code == -1) {
    syslog (LOG_INFO, "ioint error: invalid ioctl id decode (%d)", errno);
  }

  if (bdmIoctlInt (code, &var) < 0) {
    syslog (LOG_INFO, "ioint error: %s (%d)", bdmErrorString (), errno);
  }
  
  printf ("IOINT %d,0x%x.", errno, var);
}

/*
 * IO Command.
 */

void
iocmd (char *message)
{
  int id;
  int code;

  message += sizeof "IOCMD";  
  id       = strtoul (message, NULL, 0);

  code = decode_id (id);
  if (code == -1) {
    syslog (LOG_INFO, "iocmd error: invalid ioctl id decode (%d)", errno);
  }

  if (bdmIoctlCommand (code) < 0) {
    syslog (LOG_INFO, "iocmd error: %s (%d)", bdmErrorString (), errno);
  }
  
  printf ("IOCMD %d.", errno);
}

/*
 * IO ioctl addressed access.
 */

void
ioio (char *message)
{
  int             id;
  int             code;
  struct BDMioctl ioc;
    
  message    += sizeof "IOIO";  
  id          = strtoul (message, NULL, 0);
  message     = strchr (message, ',') + 1;
  ioc.address = strtoul (message, NULL, 0);
  message     = strchr (message, ',') + 1;
  ioc.value   = strtoul (message, NULL, 0);
  
  code = decode_id (id);
  if (code == -1) {
    syslog (LOG_INFO, "ioio error: invalid ioctl id decode (%d)", errno);
  }

  if (bdmIoctlIo (code, &ioc) < 0) {
    syslog (LOG_INFO, "ioio error: %s (%d)", bdmErrorString (), errno);
  }
  
  printf ("IOIO %d,0x%x,0x%x.", errno, ioc.address, ioc.value);
}

/*
 * Read a block of memory.
 */

void
bdm_read (char *message)
{
  long          read_nbytes;
  unsigned char *buf;
  unsigned long nbytes;
  unsigned long byte;

  message += sizeof "READ";
  nbytes   = strtoul (message, NULL, 0);
  buf      = xmalloc (nbytes);

  read_nbytes = bdmRead (buf, nbytes);
  
  if (debug)
    syslog (LOG_INFO, "read: nbytes %ld (%ld), have read %ld",
            nbytes, nbytes * 2, read_nbytes);
      
  if (read_nbytes < 0) {
    syslog (LOG_INFO, "read error: %s (%d)", bdmErrorString (), errno);
  }
  
  printf ("READ %d,%ld,", errno, read_nbytes);

  if (read_nbytes > 0) {
    for (byte = 0; byte < read_nbytes; byte++)
      printf ("%02x", buf[byte]);
  }
  
  xfree (buf);
}

/*
 * Write a block of memory.
 */

void
bdm_write (char *message, int msg_len, int msg_buf_len)
{
  long          written_nbytes;
  unsigned char *buf;
  unsigned long nbytes;
  unsigned long byte;
  unsigned long msg_bytes;
  int           msg_index;
  unsigned char octet = 0;

  message  += sizeof "WRITE";
  nbytes    = strtoul (message, NULL, 0);
  buf       = xmalloc (nbytes);
  byte      = 0;
  msg_bytes = 0;
  msg_index = strchr (message, ',') - message + 1;
  msg_len  -= msg_index + 1;
  
  if (debug)
    syslog (LOG_INFO, "write: nbytes %ld (%ld)", nbytes, nbytes * 2);
      
  while (byte < nbytes) {
    if (msg_len < 2) {
      int new_read;
      new_read = get_message (message + msg_len, msg_buf_len - msg_len);
      if (new_read < 0) {
        syslog (LOG_INFO, "write error: client timeout");
        xfree (buf);
        return;
      }
      msg_len += new_read;
    }
    
    if (debug)
      syslog (LOG_INFO, "write read: message len is %d (%ld/%ld)",
              msg_len, msg_bytes,  msg_len + msg_bytes);

    while ((byte < nbytes) && ((msg_len - msg_index) > 1)) {
      if ((message[msg_index] >= '0') && (message[msg_index] <= '9'))
        octet = message[msg_index] - '0';
      else if ((message[msg_index] >= 'a') && (message[msg_index] <= 'f')) {
        message[msg_index] = tolower (message[msg_index]);
        octet = message[msg_index] - 'a' + 10;
      }
      else {
        syslog (LOG_INFO, "write read: msg bytes %ld, index %d, invalid data %#2x",
                msg_bytes, msg_index, message[msg_index]);
      }
      
      msg_index++;
      msg_bytes++;
      
      octet <<= 4;

      if ((message[msg_index] >= '0') && (message[msg_index] <= '9'))
        octet |= message[msg_index] - '0';
      else if ((message[msg_index] >= 'a') && (message[msg_index] <= 'f')) {
        message[msg_index] = tolower (message[msg_index]);
        octet |= message[msg_index] - 'a' + 10;
      }
      else {
        syslog (LOG_INFO, "write read: msg bytes %ld, index %d, invalid data %#2x",
                msg_bytes, msg_index, message[msg_index]);
      }
    
      msg_index++;
      msg_bytes++;

      buf[byte] = octet;

      byte++;
    }

    if (msg_index < msg_len) {
      message[0] = message[msg_index];
      msg_len    = 1;
    }
    else
      msg_len = 0;
        
    msg_index = 0;
  }

  written_nbytes = bdmWrite (buf, nbytes);
  
  if (written_nbytes < 0) {
    syslog (LOG_INFO, "write error: %s (%d)", bdmErrorString (), errno);
  }
  
  printf ("WRITE %d,%ld", errno, written_nbytes);
  
  xfree (buf);
}

/*
 * Clean up and exit.
 */

void
quit ()
{
  bdmClose ();
  syslog (LOG_INFO, "host finished: %s (%s)", current_host, current_addr);
  exit (0);
}

/*
 * Process a message.
 */

void
process_message (char *message, int msg_len, int msg_buf_len)
{
  errno = 0;
  
  switch (decode_message (message)) {
    case HELO:
      helo (message);
      break;
      
    case SRVCTL:
      server_control (message);
      break;

    case IOINT:
      ioint (message);
      break;
      
    case IOCMD:
      iocmd (message);
      break;
      
    case IOIO:
      ioio (message);
      break;
      
    case READ:
      bdm_read (message);
      break;

    case WRITE:
      bdm_write (message, msg_len, msg_buf_len);
      break;
    
    case QUIT:
      quit ();
      break;

    default:
      break;
  }
}

void
usage ()
{
  fprintf (stderr,
           "Usage: %s [-h] [-n] [--help] [--not-inetd]\n",
           program_name);
  exit (1);
}

void
version ()
{
  printf ("bdmd %s\n", version_string);
}

/*
 * The server can be run from inetd or run from the
 * a tty allow debug to be watched.
 *
 */

int
main (int argc, char **argv)
{
  struct hostent *hp;
  int            optc;
  int            not_inetd = 0;
  char           buf[BDM_SERVER_BUF_SIZE];
  int            cread;
  
  program_name = argv[0];
  
  while ((optc = getopt (argc, argv, "dnVh")) != EOF)
  {
    switch (optc)
    {
      case 'd':
        debug++;
        break;

      case 'n':
        not_inetd = 1;
        break;

      case 'V':
        version ();
        exit (0);
        break;

      case 'h':
      default:
        usage ();
    }
  }

  /*
   * Process options.
   */

  umask (022);
  
  /*
   * chdir
   */

  fflush (stderr);
  fflush (stdout);
  
  closelog ();
  
#ifdef LOG_DAEMON
  openlog ("bdmd", (LOG_PID|LOG_CONS), LOG_DAEMON);
#else
  openlog ("bdmd", LOG_PID);
#endif
  
  if (gethostname (myname, MAXHOSTNAMELEN)) {
    fprintf (stderr, "%s: cannot find out the name of this host\n",
             program_name);
    exit (1);
  }

  /*
   * Now see if we can get a FQDN for this host.
   */
  
  hp = gethostbyname (myname);
  if (hp && (strlen (hp->h_name) > strlen (myname)))
    strncpy (myname, hp->h_name, MAXHOSTNAMELEN);

  /*
   * Don't pay attention to SIGPIPE; read will give us the problem
   * if it happens.
   */
  
  signal (SIGPIPE, SIG_IGN);

  if (!not_inetd)
    start_connection ();
  else
  {
    current_host = myname;
    if (hp)
      current_addr = (char *) inet_ntoa (*(struct in_addr *) hp->h_addr);
    else
      current_addr = myname;
  }

  syslog (LOG_INFO, "host connected: %s (%s)", current_host, current_addr);
  
  /*
   * Say hello to the remote end. It will be waiting.
   */
  
  while (1) {
    cread = get_message (buf, BDM_SERVER_BUF_SIZE);

    if (cread == 0) {
      break;
    }

    if (cread > 0) {
      if (debug > 1)
        syslog (LOG_INFO, "msg: %s", buf);

      process_message (buf, cread, BDM_SERVER_BUF_SIZE);

      fflush (stdout);
    }
  }

  bdmClose ();

  return 0;
}
