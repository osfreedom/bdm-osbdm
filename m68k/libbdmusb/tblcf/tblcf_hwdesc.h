
/* addresses of parts of flash for bootloader operation */

#define TBLCF_FLASH_BOOT_START	0xFFD0
#define TBLCF_FLASH_BOOT_END	0xFFFF
#define TBLCF_FLASH_START		0xBA00
#define TBLCF_FLASH_END			0xF9FF
#define TBLCF_BOOT_STATE		0xF9CE	/* address of 2 byte bootloader state */
#define TBLCF_BOOT_STATE_BLANK	0xFFFF	/* value to be initially programmed in */
#define TBLCF_BOOT_STATE_APPLOK 0xFF01	/* value to be programmed in after flash contents has been verified */


