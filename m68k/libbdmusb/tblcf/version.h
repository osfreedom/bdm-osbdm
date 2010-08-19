#define TBLCF_DLL_VERSION		0x10	/* version of the DLL in BCD */

/* Changes:

1.0	01/04/2006	initial coding

*/

#define TBLCF_BOOTLOADER_VERSION	0x10	/* version of the bootloader application in BCD */

/* Changes:

1.0	10/03/2006	initial coding

*/

#define TBLCF_CCS_VERSION			0x10	/* version of the CCS application in BCD */
#define TBLCF_CCS_SERVER_VERSION	0x040D	/* version reported to CCSAPI */

/* Changes:

1.0	15/04/2006	initial coding

*/

#define TBLCF_GDI_VERSION			0x13	/* version of the GDI DLL */

/* Changes:

1.0	29/04/2006	initial coding
1.1 	19/06/2006	changed return value in case of target stopped to DI_WAIT_UNKNOWN
				removed reset to BDM mode executed during start-up of the GDI interface ("ResetHalt" in the debugger cfg file can be used instead)
1.2	22/06/2006	added conditional resynchronization into each call of GetStatus
				added conditional compilation for returnning errors, the debugger is able to attach to a running target if no errors are returned
1.3	22/06/2006	added conditional default values for memory and register reads
*/

#define TBLCF_UNSEC_VERSION			0x10	/* version of the unsecure utility */

/* Changes:

1.0	11/05/2006	initial coding

*/



