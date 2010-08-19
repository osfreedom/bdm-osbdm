/**
 *  This file specifies properties by which the hardware can be identified
 */

/// Real Vendors
/// Vendor ID for Motorola
#define MOTORLA_VID	(0x0425)
/// Vendor ID for Freescale
#define FREESCALE_VID	(0x15A2)
/// Vendor ID for PEmicro
#define PEMICRO_VID 0x1357

/// Vendor ID for TBDML
#define TBDML_VID	MOTORLA_VID
/// Product ID for TBDML
#define TBDML_PID	(0x1000)

/// Vendor ID for OSBDM
#define OSBDM_VID	FREESCALE_VID
/// Product ID for OSBDM
#define OSBDM_PID	(0x0021)

/// Vendor ID for TBLCF
#define TBLCF_VID	MOTORLA_VID
/// Product ID for TBLCF
#define TBLCF_PID	(0x1001)
/// The ICP (In Circuit Programmer) for JB16
#define	JB16ICP_PID 	0xFF02

/// Vendor ID for PEmicro
#define PEMICRO_VID 0x1357
/// Product ID for Multilink 
#define MLCFE_PID 0x0503
/// Product ID for DEMOJM
#define DEMOJM_PID 0x0504


/// How long to wait for response on USB Bus
#define TIMEOUT 1000 // ms 10s originally
