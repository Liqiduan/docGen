#include <stdlib.h>
#include <stdint.h>

#include "que.h"
#include "spinlock.h"

#include "order.h"

#define ORDER_BURST_NUM 32
#define ORDER_TX_CACHE_SIZE 32

typedef struct {
	uint32_t is_ready;
	uint32_t num;

	struct {
		uint32_t port;
		void* mbuf;
	}pkts[ORDER_TX_CACHE_SIZE];
}tx_cache;

typedef struct {
	uint32_t id;
	spinlock rx_lock;
	port_ops* ops;
} reorder_port;

typedef struct {
	tx_cache* tx_cache;
} reorder_core;

typedef struct {
	struct {
		spinlock lock;

		tx_cache* pend;
		ring ring;

		ring pool;
	}tx;

	reorder_core cores[32];
	reorder_port ports[32];
} reorder;

reorder g_reorder;
uint32_t do_port_rx(reorder_port* port, void** pkts, uint32_t num);
uint32_t do_port_tx(tx_cache* cache);
uint32_t do_port_fwd(reorder_port* port, void** pkts, uint32_t num);

void enter_reorder(tx_cache* tx_cache);
void exit_reorder();

reorder_port* get_port(uint32_t portid);
reorder_core* get_core(uint32_t coreid);

tx_cache* alloc_tx_cache();
void free_tx_cache();

extern uint32_t get_coreid();
int32_t reorder_init()
{
	int ret;
	tx_cache *p;

	ring_init(&g_reorder.tx.pool, 128);
	ring_init(&g_reorder.tx.ring, 128);

	do {
		p = (tx_cache*)malloc(sizeof(tx_cache));
		ret = ring_enque(&g_reorder.tx.pool, p);
	} while(ret == 0);
	return 0;
}

uint32_t reorder_rx_work(void *arg)
{
	reorder* reorder = &g_reorder;
	reorder_port *port = get_port(*(uint32_t*)arg);

	if (spinlock_trylock(&port->rx_lock) != 1) {
		return 0;
	}

	tx_cache* tx_cache;
	tx_cache = alloc_tx_cache();
	ring_enque(&reorder->tx.ring, tx_cache);

	uint32_t nb_rx;
	void* pkts[ORDER_BURST_NUM];
	nb_rx = do_port_rx(port, pkts, ORDER_BURST_NUM);

	spinlock_unlock(&port->rx_lock);

	enter_reorder(tx_cache);

	do_port_fwd(port, pkts, nb_rx);

	exit_reorder();
	return 0;
}

uint32_t reorder_tx_work(void *arg)
{
	reorder *reorder = &g_reorder;
	reorder_port *port = get_port(*(uint32_t*)arg);

	tx_cache *cache;
	
	if (spinlock_trylock(&reorder->tx.lock) != 1) {
		return 0;
	}
	
	if (reorder->tx.pend) {
		cache = reorder->tx.pend;
		if (!cache->is_ready) {
			goto exit;
		} else {
			do_port_tx(cache);
		}
	}

	while(cache = ring_deque(&reorder->tx.ring)) {
		if (cache->is_ready) {
			do_port_tx(cache);
		} else {
			reorder->tx.pend = cache;
			goto exit;
		}
	}

exit:
	spinlock_unlock(&reorder->tx.lock);
	return 0;
}

uint32_t reorder_send(uint32_t portid, void **pkts, uint32_t num)
{
	reorder_core *core = get_core(get_coreid());
	tx_cache* cache = core->tx_cache;
	
	uint32_t i;
	for (i=cache->num; num > 0; num--,i++) {
		cache->pkts[i].port = portid;
		cache->pkts[i].mbuf = *pkts;

		pkts++;
		cache->num++;
	}
	return 0;
}

void enter_reorder(tx_cache *tx_cache)
{
	reorder_core *core = get_core(get_coreid());

	tx_cache->num = 0;
	tx_cache->is_ready = 0;
	core->tx_cache = tx_cache;
}

void exit_reorder()
{
	reorder_core *core = get_core(get_coreid());

	core->tx_cache->is_ready = 1;
}

uint32_t reorder_add_port(uint32_t portid, port_ops *ops)
{
	reorder* reorder = &g_reorder;
	reorder->ports[portid].id = portid;
	reorder->ports[portid].ops = ops;
	return 0;
}

reorder_port* get_port(uint32_t portid)
{
	reorder* reorder = &g_reorder;
	return &reorder->ports[portid];
}

reorder_core* get_core(uint32_t coreid)
{
	return &g_reorder.cores[coreid];
}

uint32_t do_port_rx(reorder_port* port, void** pkts, uint32_t num)
{
	return port->ops->rx(port->id, pkts, num);
}

uint32_t do_port_tx(tx_cache* cache)
{
	uint32_t i;
	
	for (i=0; i<cache->num; i++) {
		reorder_port* port = get_port(cache->pkts[i].port);
		port->ops->tx(port->id, cache->pkts[i].mbuf, 1);
	}

	/* TODO: Free not send pkts */

	free_tx_cache(cache);
	return i;
}

uint32_t do_port_fwd(reorder_port* port, void** pkts, uint32_t num)
{
	return port->ops->fwd(port->id, pkts, num);
}

tx_cache* alloc_tx_cache()
{
	return ring_deque(&g_reorder.tx.pool);
}

void free_tx_cache(tx_cache* cache)
{
	ring_enque(&g_reorder.tx.pool, cache);
}
