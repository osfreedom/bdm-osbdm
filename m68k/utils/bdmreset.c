/*
 * Reset the target connected to the BDM port.
 *
 * $ gcc -o bdmreset bdmreset.c -lBDM
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <BDMlib.h>

void
clean_exit (int exit_code)
{
  if (bdmIsOpen ())
  {
    bdmSetDriverDebugFlag (0);
    bdmClose ();
  }
  exit (exit_code);
}
  
void
show_error (char *msg)
{
  printf ("%s failed: %s\n", msg, bdmErrorString ());
  clean_exit (1);
}

void
do_reset (int cpu, int verbose)
{
  unsigned long csr;
  
  /*
   * Reset the target
   */
  if (bdmReset () < 0)
    show_error ("Reset");

  /*
   * Get the target status
   */
  if (cpu == BDM_COLDFIRE)
  {
    if (bdmReadSystemRegister (BDM_REG_CSR, &csr) < 0)
      show_error ("reading CSR");
    
    if ((csr & 0x01000000) == 0)
    {
      printf ("CSR break not set, target failed to break, CSR = 0x%08lx\n", csr);
      clean_exit (1);
    }

    if (verbose)
    {
      printf ("CSR break set, target stopped.\n");
      printf ("Debug module version is %d, (%s)\n",
              (unsigned char) (csr >> 20) & 0x0f,
              ((csr >> 20) & 0x0f) == 0 ? "5206(e)" : "5307");
    }
  }
}

void
usage ()
{
  printf ("bdmreset -d [level] -D [delay] -v [device]\n"
   " where :\n"
   "    -d [level]   : enable driver debug output\n"
   "    -D [delay]   : delay count for the clock generation\n"
   "    -v           : verbose\n"
   "    [device]     : the bdm device, eg /dev/bdmcf0\n");
  exit (1);
}

int
main (int argc, char **argv)
{
  char          *dev = NULL;
  int           arg;
  int           delay_counter = 0;
  int           debug_level = 0;
  int           verbose = 0;
  unsigned int  ver;
  int           cpu;
  int           iface;

  if (argc <= 1)
    usage ();

  for (arg = 1; arg < argc; arg++)
  {
    if (argv[arg][0] != '-')
    {
      if (dev)
      {
        printf(" device name specified more then once");
        exit (1);
      }
      dev = argv[arg];
    }
    else
    {
      switch (argv[arg][1])
      {
        case 'd':
          arg++;
          if (arg == argc)
          {
            printf (" -d option requires a parameter");
            exit (1);
          }
          debug_level = strtoul (argv[arg], NULL, 0);
          break;

        case 'D':
          arg++;
          if (arg == argc)
          {
            printf (" -D option requires a parameter");
            exit (1);
          }
          delay_counter = strtoul (argv[arg], NULL, 0);
          break;

        case 'v':
          verbose = 1;
          break;
          
        default:
          printf(" unknown option!");
    
        case '?':
        case 'h':
          usage ();
          break;
      }
    }
  }

  if (!dev)
  {
    printf(" ERROR: no device set, check your options as some require parameters.\n");
    exit (1);
  }

  /*
   * Open the BDM interface driver
   */
  if (bdmOpen (dev) < 0)
    show_error ("open");

  if (debug_level)
    bdmSetDriverDebugFlag (debug_level);

  if (delay_counter)
    bdmSetDelay (delay_counter);

  /*
   * Get the driver version
   */

  if (verbose)
  {
    if (bdmGetDrvVersion (&ver) < 0)
      show_error ("GetDrvVersion");

    printf ("Driver Ver : %x.%x\n", ver >> 8, ver & 0xff);

    /*
     * Get the processor
     */
    if (bdmGetProcessor (&cpu) < 0)
      show_error ("GetProcessor");

    switch (cpu)
    {
      case BDM_CPU32:
        printf ("Processor  : CPU32\n");
        break;
      case BDM_COLDFIRE:
        printf ("Processor  : Coldfire\n");
        break;
      default:
        printf ("unknown processor type!\n");
        clean_exit (1);
      break;
    }
  
    /*
     * Get the interface
     */
    if (bdmGetInterface (&iface) < 0)
      show_error ("GetInterface");
    
    switch (iface) {
      case BDM_CPU32_ERIC:
        printf ("Interface  : Eric's CPU32\n");
        break;
      case BDM_COLDFIRE:
        printf ("Interface  : P&E Coldfire\n");
        break;
      case BDM_CPU32_ICD:
        printf ("Interface  : ICD CPU32\n");
        break;
      default:
        printf ("unknown interface type!\n");
        clean_exit (1);
      break;
    }
  }
  
  do_reset (cpu, verbose);
  
  clean_exit (0);

  return 0;
}
