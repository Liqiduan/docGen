#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <google/cmockery.h>

#include "que.h"
#include "spinlock.h"
#include "order.h"

uint32_t mock_rx(uint32_t port, void **pkts, uint32_t num)
{
	uint32_t i;
	uint32_t nb_rx;
	void** rx_pkt;

	nb_rx = (uint32_t)mock();
	rx_pkt = (void**)mock();

	for (i = 0; i < nb_rx; i ++ ) {
		pkts[i] = rx_pkt[i];
	}
	
	return nb_rx;
}

uint32_t mock_tx(uint32_t port, void **pkts, uint32_t num)
{
	check_expected(num);	
 	check_expected(pkts);
	return (uint32_t)mock();
}

uint32_t mock_fwd(uint32_t port, void **pkts, uint32_t num)
{
	reorder_send(port, pkts, num);
	return 0;
}

int pkt_cmp(const uintmax_t value, const uintmax_t check)
{
	void **src = (void**)value;
	void **dest = (void**)check;
	while(src) {
		assert_int_not_equal(*src, *dest);
		src++;
		dest++;
	}

	return 0;
}

uint32_t spinlock_trylock(spinlock* lock)
{
	return (uint32_t)mock();
}

void spinlock_unlock(spinlock *lock)
{
	check_expected(lock);
	return ;
}

uint32_t get_coreid()
{
	return mock();
}

port_ops mock_port = {
	.rx = mock_rx,
	.tx = mock_tx,
	.fwd = mock_fwd,
	.init = NULL,
};

void single_core_send_recv(void **state)
{
	uint32_t port = 1;
	void* pkts[] = {(void*)1,(void*)2,(void*)3};

	reorder_init();
    reorder_add_port(1, &mock_port);		

	will_return_count(get_coreid, 1, -1);

	will_return(mock_rx, 3);
	will_return(mock_rx, pkts);
	will_return(spinlock_trylock, 1);
	expect_any(spinlock_unlock, lock);
	reorder_rx_work((void*)&port);

	expect_value_count(mock_tx, num, 1, 3);
	expect_value(mock_tx, pkts, pkts[0]);
	expect_value(mock_tx, pkts, pkts[1]);
	expect_value(mock_tx, pkts, pkts[2]);
	will_return_count(mock_tx, 0, -1);
	will_return(spinlock_trylock, 1);
	expect_any(spinlock_unlock, lock);
	reorder_tx_work(&port);
}

void que_operate(void **state)
{
	int ret;

	ring r;
	void* p = NULL;
	int *d;

	ring_init(&r, 4);

	uint32_t i;
	for (i=0; i<3; i++) {
		ret = ring_enque(&r, &p[i]);
		assert_int_equal(ret, 0);
	}

	ret = ring_enque(&r, &p[3]);
	assert_int_equal(ret, -1);

	for (i=0; i<3; i++) {
		d = ring_deque(&r);
		assert_int_equal(d, &p[i]);
	}

	d = ring_deque(&r);
	assert_int_equal(d, 0);
}

const UnitTest tests[] = 
{
	unit_test(single_core_send_recv),
	unit_test(que_operate),
};

int main()
{
	run_tests(tests);
	return 0;
}

