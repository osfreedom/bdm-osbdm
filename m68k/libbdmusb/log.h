extern int bdmGetDebugFlag (void);
#define bdm_print if (bdmGetDebugFlag () > 1) bdmPrint
void bdmPrint(const char *format, ...);
void bdm_print_dump(unsigned char *data, unsigned int size);
