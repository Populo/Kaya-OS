
#include "../h/types.h"
#include "../h/const.h"


HIDDEN adl_t *delaydFree_h;
HIDDEN adl_t *activeDelayd_h;

/* search delaydFree for node before address we are looking for */
HIDDEN adl_t *searchDelaydFree(int *wake);

