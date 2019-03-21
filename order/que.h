typedef struct {
	uint32_t en;
	uint32_t de;
	uint32_t size;	
	uint32_t mask;
	void** data;
}ring;

int ring_init(ring* ring, uint32_t size);
int ring_enque(ring* ring, void *data);
void* ring_deque(ring* ring);
