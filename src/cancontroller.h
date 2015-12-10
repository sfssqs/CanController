#include <stdio.h>

#ifndef CANCONTROLLER_H_
#define CANCONTROLLER_H_
#endif /* CANCONTROLLER_H_ */

#define myerr(str)	fprintf(stderr, "%s, %s, %d: %s\n", __FILE__, __func__, __LINE__, str)
#define errout(_s)	fprintf(stderr, "error class: %s\n", (_s))
#define errcode(_d) fprintf(stderr, "error code: %02x\n", (_d))

