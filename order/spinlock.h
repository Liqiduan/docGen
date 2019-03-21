typedef struct {
	uint32_t lock;
}spinlock;

uint32_t spinlock_trylock(spinlock* lock);
void spinlock_unlock(spinlock* lock);
