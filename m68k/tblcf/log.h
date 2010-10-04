extern int bdmGetDebugFlag (void);
#define tblcf_print if (bdmGetDebugFlag () > 1) bdmPrint
void bdmPrint(const char *format, ...);
void tblcf_print_dump(unsigned char *data, unsigned int size);
