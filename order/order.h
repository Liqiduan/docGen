
typedef uint32_t (*ops_rx)(uint32_t portid, void **pkts, uint32_t num);
typedef uint32_t (*ops_tx)(uint32_t portid, void **pkts, uint32_t num);
typedef uint32_t (*ops_fwd)(uint32_t portid, void **pkts, uint32_t num);
typedef uint32_t (*ops_init)(uint32_t port);

typedef struct {
	ops_rx rx;
	ops_tx tx;
	ops_fwd fwd;
	ops_init init;
}port_ops;


int32_t reorder_init();
uint32_t reorder_rx_work(void *arg);
uint32_t reorder_tx_work(void *arg);
uint32_t reorder_add_port(uint32_t portid, port_ops *ops);
uint32_t reorder_send(uint32_t portid, void **pkts, uint32_t num);
