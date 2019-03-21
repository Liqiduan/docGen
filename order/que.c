#include <stdlib.h>
#include <stdint.h>
#include "que.h"


int ring_init(ring* ring, uint32_t size)
{
	ring->en = 0;
	ring->de = 0;
	ring->size = size;
	ring->data = malloc(sizeof(void*) * size);
	ring->mask = size-1;
	
	return 0;
}

int ring_enque(ring* ring, void *data)
{
	if (((ring->en + 1) & ring->mask) == (ring->de & ring->mask)) {
		return -1;
	}

	ring->en += 1;
	ring->data[ring->en & ring->mask] = data;
	return 0;
}

void* ring_deque(ring* ring)
{
	if (ring->en == ring->de) {
		return NULL;
	}

	void *data;
	ring->de += 1;
	data = ring->data[ring->de & ring->mask];
	return data;
}
