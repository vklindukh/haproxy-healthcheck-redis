#include <common/errors.h>
#include <proto/log.h>
#include <types/global.h>
#include <hiredis/hiredis.h>

redisContext *redis_context = NULL;

static char tmpcmd[512];
/* perform minimal connect to redis, report 0 in case of error, 1 if OK. */
int redis_connect()
{
	if (global.redis_srv_addr==NULL) {
		Warning("Redis server not configured. Healthcheck status to redis disabled.\n");
		return ERR_NONE;
	}
        redis_context = redisConnect(global.redis_srv_addr, global.redis_srv_port);
	if (redis_context->err) {
		Alert("Cannot connect to redis %s:%i: %s\n",global.redis_srv_addr, global.redis_srv_port, redis_context->errstr);
		return ERR_FATAL;
	}
	return ERR_NONE;
}

int redis_update_status(struct server *s, int status)
{
	void *reply;
	char *cmd;

	if (global.redis_srv_addr==NULL)
		return ERR_NONE;

	cmd = tmpcmd;
	status = (status == HCHK_STATUS_L7OKD ? 1 : 0);
	snprintf(cmd, sizeof(tmpcmd), "SET _haproxy_server_status_%s %i", s->id, status);
	Warning("Update redis for %s with status %i\n", s->id, status);
	if (!(reply = redisCommand(redis_context, cmd))) { 
		// try to reconnect first
		Warning("Trying to reconnect to redis\n");
		if (redis_connect() & ERR_NONE) {
			if (!(reply = redisCommand(redis_context, cmd))) {
				Alert("Cannot update redis: %s\n", redis_context->errstr);
				return ERR_FATAL;
			}
		} else 
			Alert("Cannot reconnect to redis\n");
	}
	return ERR_NONE;
}

