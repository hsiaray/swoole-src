#include "swoole.h"
#include <sys/ipc.h>
#include <sys/msg.h>

static int swPipeUnsock_read(swPipe *p, void *data, int length);
static int swPipeUnsock_write(swPipe *p, void *data, int length);
static int swPipeUnsock_getFd(swPipe *p, int isWriteFd);
static int swPipeUnsock_close(swPipe *p);

typedef struct _swPipeUnsock
{
	int socks[2];
} swPipeUnsock;

static int swPipeUnsock_getFd(swPipe *p, int isWriteFd)
{
	swPipeUnsock *this = p->object;
	return isWriteFd == 1 ? this->socks[1] : this->socks[0];
}

static int swPipeUnsock_close(swPipe *p)
{
	int ret1, ret2;
	swPipeUnsock *this = p->object;
	ret1 = close(this->socks[0]);
	ret2 = close(this->socks[1]);
	sw_free(this);
	return 0-ret1-ret2;
}

int swPipeUnsock_create(swPipe *p, int blocking, int protocol)
{
	int ret;
	swPipeUnsock *object = sw_malloc(sizeof(swPipeUnsock));
	if (object == NULL)
	{
		swWarn("swPipeUnsock_create malloc fail");
		return SW_ERR;
	}
	p->blocking = blocking;
	ret = socketpair(AF_UNIX, protocol, 0, object->socks);
	if (ret < 0)
	{
		swWarn("swPipeUnsock_create fail. Error: %s [%d]", strerror(errno), errno);
		return SW_ERR;
	}
	else
	{
		//Nonblock
		if (blocking == 0)
		{
			swSetNonBlock(object->socks[0]);
			swSetNonBlock(object->socks[1]);
		}

		int sbsize = SW_WORKER_UNSOCK_BUFSIZE;
		setsockopt(object->socks[1], SOL_SOCKET, SO_SNDBUF, &sbsize, sizeof(sbsize));
		setsockopt(object->socks[1], SOL_SOCKET, SO_RCVBUF, &sbsize, sizeof(sbsize));
		setsockopt(object->socks[0], SOL_SOCKET, SO_SNDBUF, &sbsize, sizeof(sbsize));
		setsockopt(object->socks[0], SOL_SOCKET, SO_RCVBUF, &sbsize, sizeof(sbsize));

		p->object = object;
		p->read = swPipeUnsock_read;
		p->write = swPipeUnsock_write;
		p->getFd = swPipeUnsock_getFd;
		p->close = swPipeUnsock_close;
	}
	return 0;
}

static int swPipeUnsock_read(swPipe *p, void *data, int length)
{
	return read(((swPipeUnsock *) p->object)->socks[0], data, length);
}

static int swPipeUnsock_write(swPipe *p, void *data, int length)
{
	return write(((swPipeUnsock *) p->object)->socks[1], data, length);
}

