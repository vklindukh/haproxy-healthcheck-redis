#include <hiredis/hiredis.h>

int redis_connect();
int redis_update_status(struct server *s, int status);

