/****************************************************************************/
/*
 *	Do all our LP setup through the standard System/Sdevice setup
 */
/****************************************************************************/

#include "config.h"

/****************************************************************************/

unsigned int bdm_ports[] = {
#ifdef BDM_0
	BDM_0_SIOA,
#else
	-1,
#endif
#ifdef BDM_1
	BDM_1_SIOA,
#else
	-1,
#endif
#ifdef BDM_2
	BDM_2_SIOA,
#else
	-1,
#endif
#ifdef BDM_3
	BDM_3_SIOA,
#else
	-1,
#endif
};

int bdm_debug_level = 0;

/****************************************************************************/
