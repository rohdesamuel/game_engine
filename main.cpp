#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <cstdarg>
#include <map>
#include <queue>
#include <type_traits>
#include <boost/algorithm/string/join.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/map.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/type_traits/function_traits.hpp>

#include "queue.h"
#include "timer.h"
#include "abstract_process.h"
#include "abstract_scheduler.h"
#include "vector_math.h"

#include "console_draw.h"

using std::vector;

#define ASSERT(expr) if (expr) { std::cout<<"SUCCEEDED\n"; } else std::cout << "TEST FAILED:\n\tFILE: " << __FILE__ << "\n\tLINE: " << __LINE__ << "\n\tREASON: "
#define QASSERT(expr) if (!(expr)) std::cout << "TEST FAILED:\n\tFILE: " << __FILE__ << "\n\tLINE: " << __LINE__ << "\n\tREASON: "

#define TEST_ECS_V3

#ifdef TESTING_OBJECTPOOL
INT log_2(UINT64 v) {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
	static const char LogTable256[] =
	{
		-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
		LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
		LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
	};
	register UINT64 t; // temporary
	if (t = v >> 56) {
		return 56 + LogTable256[t];
	}
	else if (t = v >> 48) {
		return 48 + LogTable256[t];
	}
	else if (t = v >> 40) {
		return 40 + LogTable256[t];
	}
	else if (t = v >> 32) {
		return 32 + LogTable256[t];
	}
	else if (t = v >> 24) {
		return 24 + LogTable256[t];
	}
	else if (t = v >> 16) {
		return 16 + LogTable256[t];
	}
	else if (t = v >> 8) {
		return 8 + LogTable256[t];
	}
	else {
		return LogTable256[v];
	}
}

UINT count_bits(UINT v) {
	static const unsigned char BitsSetTable256[256] =
	{
#   define B2(n) n,     n+1,     n+1,     n+2
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
		B6(0), B6(1), B6(1), B6(2)
	};
	return
		BitsSetTable256[v & 0xff] +
		BitsSetTable256[(v >> 8) & 0xff] +
		BitsSetTable256[(v >> 16) & 0xff] +
		BitsSetTable256[v >> 24];
}

UINT count_bits(UINT64 v) {
	static const unsigned char BitsSetTable256[256] =
	{
#   define B2(n) n,     n+1,     n+1,     n+2
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
		B6(0), B6(1), B6(1), B6(2)
	};
	return
		BitsSetTable256[v & 0xff] +
		BitsSetTable256[(v >> 8) & 0xff] +
		BitsSetTable256[(v >> 16) & 0xff] +
		BitsSetTable256[(v >> 24) & 0xff] +
		BitsSetTable256[(v >> 32) & 0xff] +
		BitsSetTable256[(v >> 40) & 0xff] +
		BitsSetTable256[(v >> 48) & 0xff] +
		BitsSetTable256[v >> 56];
}

template<class Ty_>
class BuddySystemAllocator {
private:
	struct Block {
		UINT32 order;
		// Null implies that the child is free memory.
		struct Block* next = nullptr;
	};

	UINT8* mem_;
	Block** free_blocks_;

	UINT capacity_;
	UINT tot_size_;
	UINT32 order_;
	UINT size_;

	inline bool in_range(Ty_* mem) {
		return (UINT8*)mem >= mem_ && (UINT8*)mem < mem_ + tot_size_;
	}

	inline Ty_* block_to_ptr(Block* b) {
		return (Ty_*)((UINT8*)b + sizeof(Block));
	}

	inline Block* ptr_to_block(Ty_* mem) {
		return (Block*)((UINT8*)mem - sizeof(Block));
	}

	Block* merge(Block* head, Block* tail, Block** other) {
		if (head < tail && tail == buddy_of(head)) {
			++head->order;
			head->next = nullptr;
			*other = tail;
			return head;
		} else if (head > tail && head == buddy_of(tail)) {
			++tail->order;
			tail->next = nullptr;
			*other = head;
			return tail;
		}
		return nullptr;
	}

	inline Block* buddy_of(Block* block) {
		return (Block*)((UINT8*)block + size_ * (1 << block->order));
	}

	void release(Block* block) {
		UINT order = block->order;

		Block* node = free_blocks_[order];
		Block* prev = node;
		if (!node->next) {
			node->next = block;
		} else {
			while (node->next) {
				prev = node;
				node = node->next;

				Block* other = nullptr;
				Block* merged = merge(block, node, &other);
				if (merged) {
					prev->next = prev->next->next;
					other->order = 0;
					other->next = nullptr;
					release(merged);
					return;
				}
			}
		}
	}

	Block* split(Block* head) {
		--head->order;
		Block* tail = buddy_of(head);
		tail->order = head->order;
		return tail;
	}

	Block* alloc_block(UINT order) {
		if (order <= order_) {
			// Get the block of memory to allocate from
			Block* sentinel = free_blocks_[order];
			Block* mem = sentinel->next;

			// If there are no free blocks, we need to allocate some.
			if (mem) {
				// Otherwise, we can get some memory right away!
				sentinel->next = mem->next;
				return mem;
			} else {
				// Recurse up to split the block.
				Block* head = alloc_block(order + 1);

				// If we get a nullptr, that means we have run out of memory.
				if (head) {
					// Split the block and push it onto the working memory.
					Block* tail = split(head);
					head->next = sentinel->next;
					sentinel->next = head;
					return tail;
				}
			}
		}
		return nullptr;
	}

public:
	BuddySystemAllocator(const UINT count) {
		size_ = sizeof(Block) + sizeof(Ty_);
		order_ = log_2(count) + (count_bits(count) == 1 ? 0 : 1);
		capacity_ = 1 << order_;
		tot_size_ = size_ * capacity_;
		mem_ = (UINT8*)std::calloc(1, tot_size_);
		((Block*)(mem_))->order = order_;
		((Block*)(mem_))->next = nullptr;

		free_blocks_ = (Block**)std::calloc(order_ + 1, sizeof(Block*));
		for (UINT i = 0; i < order_ + 1; ++i) {
			free_blocks_[i] = (Block*)std::calloc(1, sizeof(Block));
			free_blocks_[i]->order = 0;
			free_blocks_[i]->next = nullptr;
		}
		free_blocks_[order_]->next = (Block*)mem_;
	}

	~BuddySystemAllocator() {
		for (UINT i = 0; i < order_ + 1; ++i) {
			std::free(free_blocks_[i]);
		}
		std::free(free_blocks_);
		std::free(mem_);
	}

	Ty_* alloc(UINT count) {
		if (count == 0 || count > capacity_) {
			return nullptr;
		}
		UINT order = log_2(count) + (count_bits(count) == 1 ? 0 : 1);
		return block_to_ptr(alloc_block(order));
	}

	void free(Ty_* mem) {
		if (in_range(mem)) {
			Block* block = ptr_to_block(mem);
			release(block);
		}
	}

	void clear() {
		for (UINT i = 0; i < order_ + 1; ++i) {
			free_blocks_[i]->order = 0;
			free_blocks_[i]->next = nullptr;
		}
		free_blocks_[order_]->order = order_;
		free_blocks_[order_]->next = (Block*)mem_;
	}

	UINT capacity() {
		return capacity_;
	}

	template <class Allocator>
	friend class ObjectPoolTester;
};

template<class Ty_>
class ObjectPool
{
private:
	struct Block {
		UINT8 order;
		UINT8 flags;
	};

	typedef ::std::vector<Block*> FreeMem;
	typedef ::std::vector<FreeMem> FreeMemList;

	static const UINT8 HEAD_BIT = 0x1;
	static const UINT8 FREE_BIT = 0x2;

	UINT capacity_;
	UINT tot_size_;
	UINT size_;
	INT order_;
	FreeMemList free_mem_;

	Ty_* mem_;
	Ty_ *start_, *end_;

	inline bool in_range(Ty_* p) {
		return p > start_ && p < end_;
	}

	inline Ty_* block_to_ptr(Block* p) {
		return (Ty_*)((char*)(p) + sizeof(Block));
	}

	inline Block* ptr_to_block(Ty_* p) {
		return (Block*)((char*)(p) - sizeof(Block));
	}

	inline Block* buddy_of(Block* b) {
		UINT count = 1 << b->order;
		if (b->flags & HEAD_BIT) {
			return (Block*)((UINT)b + (UINT)(size_ * count));
		} else {
			return (Block*)((UINT)b - (UINT)(size_ * count));
		}
	}

	Block* merge_block(Block* buddy1, Block* buddy2) {
		Block *head = buddy2, *tail = buddy1;
		if (buddy1->flags & HEAD_BIT) {
			head = buddy1;
			tail = buddy2;
		}
		head->flags = FREE_BIT | HEAD_BIT;
		++head->order;

		tail->flags = FREE_BIT;
		++tail->order;

		return head;
	}

	Block* split_block(Block* head, UINT order) {
		head->order = order_ - (order + 1);

		// Is head.
		head->flags |= HEAD_BIT;

		// Is not free.
		head->flags &= ~FREE_BIT;

		Block* tail = buddy_of(head);
		*tail = *head;

		// Tail is not the head.
		tail->flags = FREE_BIT;

		return tail;
	}

	Block* make(UINT order) {
		// Get the block of memory to allocate from
		FreeMem& mem = free_mem_[order];

		// If there are no free blocks, we need to allocate some.
		if (mem.size() == 0) {

			// Recurse up to split the block.
			Block* head = order ? make(order - 1) : nullptr;

			// If we get a nullptr, that means we have run out of memory.
			if (head) {
				// Split the block and push it onto the working memory.
				Block* tail = split_block(head, order - 1);
				mem.push_back(tail);
				return head;
			}
			return nullptr;
		} else {
			// Otherwise, we can get some memory right away!
			Block* ret = mem.back();

			// Is not free.
			ret->flags &= ~FREE_BIT;

			mem.pop_back();
			return ret;
		}
	}

	void release(Block* b) {
		// Only recurse until we hit the top node.
		if (b->order < order_) {
			QASSERT(buddy_of(b)->order == b->order) << "Order not the same\n";

			// Get the buddy, if it's free merge and recurse.
			FreeMem& mem = free_mem_[order_ - b->order];
			UINT size = mem.size();
			if (size == 0) {
				// Release to the free memory.
				free_mem_[order_ - b->order].push_back(b);
				b->flags |= FREE_BIT;
			} else if (size == 1) {
				Block* buddy = mem.back();
				mem.clear();
				Block* head = merge_block(b, buddy);
				free_mem_[order_ - head->order].push_back(head);
				if (free_mem_[order_ - head->order].size() == 2) {
					release(head);
				}
			} else {
				Block *b1 = mem[0],
					  *b2 = mem[1];
				mem.clear();
				Block* head = merge_block(b1, b2);
				free_mem_[order_ - head->order].push_back(head);
				if (free_mem_[order_ - head->order].size() == 2) {
					release(head);
				}
			}
		}
	}

public:
	ObjectPool(const UINT count) :
		size_(sizeof(Ty_) + sizeof(Block)) {
		order_ = log_2(count) + (count_bits(count) == 1 ? 0 : 1);
		capacity_ = 1 << order_;
		tot_size_ = size_ * capacity_;
		mem_ = (Ty_*)::std::malloc(tot_size_);
		start_ = mem_;
		end_ = mem_ + tot_size_;

		for (int i = 0; i < order_ + 1; ++i) {
			free_mem_.push_back(FreeMem());
		}
		Block* head = (Block*)mem_;
		head->order = order_;
		head->flags = HEAD_BIT | FREE_BIT;
		free_mem_[0].push_back(head);
	}

	~ObjectPool() {
		::std::free((void*)mem_);
	}

	Ty_* alloc(UINT count) {
		if (count > capacity_) {
			return nullptr;
		}
		UINT order = order_ - (log_2(count) + (count_bits(count) == 1 ? 0 : 1));
		Block* ret = make(order);
		return ret == nullptr ? nullptr : block_to_ptr(ret);
	}

	void free(Ty_* mem) {
		if (in_range(mem)) {
			release(ptr_to_block(mem));
		}
	}

	void clear() {
		Block* head = (Block*)mem_;
		head->order = order_;
		head->flags = HEAD_BIT | FREE_BIT;

		UINT size = free_mem_.size();
		for (UINT i = 0; i < size; ++i) {
			free_mem_[i].clear();
		}
		free_mem_[0].push_back(head);
	}

	UINT capacity() {
		return capacity_;
	}
	template <class Allocator>
	friend class ObjectPoolTester;
};

struct Big {
	char mem[256];
};

#define TIME(expr) do{ Timer t; t.start(); do { expr; } while(0); t.stop(); std::cout << "TIMER: " << t.getElapsed() << "\n"; } while(0);
template <class Allocator>
class ObjectPoolTester
{
public:
	void run_tests() {
		{
			Allocator pool(100);
			UINT capacity = pool.capacity();
			ASSERT(capacity == 128) << "Capacity should be 128.\n";
		}

		{
			Allocator pool(100);
			int* b = pool.alloc(129);
			ASSERT(b == nullptr) << "Block should be null.\n";
		}
		
		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(128));
			ASSERT(b->order == 7) << "Block order should be 7. Is " << b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(60));
			ASSERT(b->order == 6) << "Block order should be 6. Is " << (UINT)b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(32));
			ASSERT(b->order == 5) << "Block order should be 5. Is " << b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(16));
			ASSERT(b->order == 4) << "Block order should be 4. Is " << b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(8));
			ASSERT(b->order == 3) << "Block order should be 3. Is " << b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(4));
			ASSERT(b->order == 2) << "Block order should be 2. Is " << b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(2));
			ASSERT(b->order == 1) << "Block order should be 1. Is " << b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(1));
			ASSERT(b->order == 0) << "Block order should be 0. Is " << b->order << "\n";
		}

		{
			Allocator pool(100);
			Allocator::Block* b = pool.ptr_to_block(pool.alloc(60));
			b = pool.ptr_to_block(pool.alloc(30));
			b = pool.ptr_to_block(pool.alloc(15));
			b = pool.ptr_to_block(pool.alloc(7));
			b = pool.ptr_to_block(pool.alloc(3));
			b = pool.ptr_to_block(pool.alloc(1));
			ASSERT(b->order == 0) << "Block order should be 0. Is " << b->order << "\n";
		}
		
		{
			Allocator pool(128);
			Allocator::Block* b;
			for (int i = 0; i < 64; ++i) {
				b = pool.ptr_to_block(pool.alloc(1));
				QASSERT(b->order == 0) << "Block order should be 0. Is " << b->order << "\n";
			}
			b = pool.ptr_to_block(pool.alloc(64));
			QASSERT(b->order == 6) << "Block order should be 6. Is " << b->order << "\n";
		}

		/*{
			Allocator<Big> pool(16000);
			UINT null_count = 0;
			for (int i = 0; i < 16000; ++i) {
				Big* b = pool.alloc(1);
				if (!b) {
					++null_count;
				}
			}
			ASSERT(null_count == 0) << "All memory should be used. " << null_count << " unused." << "\n";
			
		}*/

		/*{
			Allocator pool(100);
			for (int i = 0; i < 100; ++i) {
				Allocator::Block* b = pool.ptr_to_block(pool.alloc(1));
				pool.free(pool.block_to_ptr(b));
			}
			ASSERT(pool.free_mem_[0].size() == 1) << "All memory should be freed.";
		}*/

		{
			Allocator pool(100);
			std::vector<int*> used;
			std::vector<int*> garbage;
			std::vector<int> values;
			for (int i = 0; i < 75; ++i) {
				int* a = (int*)pool.alloc(1);
				*a = i;
				used.push_back(a);
				values.push_back(*a);
			}
			for (int i = 0; i < 25; ++i) {
				garbage.push_back((int*)pool.alloc(1));
			}
			for (int i = 0; i < 25; ++i) {
				int* p = garbage.back();
				garbage.pop_back();
				*p = i + 100;
				pool.free(p);
			}
			bool same = true;
			for (int i = 0; i < 75; ++i) {
				same &= *used[i] == values[i];
			}
			ASSERT(same) << "Memory should be immutable if other memory is freed.";
		}

		{
			std::vector<int*> mem;
			Allocator pool(16);
			for (int i = 0; i < 9; ++i) {
				int* new_mem = pool.alloc(1);
				mem.push_back(new_mem);
				if (i % 2 == 0) {
					pool.free(mem.front());
					mem.erase(mem.begin());
				}
			}

			std::set<int*> unique_mem;
			for (int i = 0; i < mem.size(); ++i) {
				if (mem[i] != nullptr) {
					std::set<int*>::iterator it = unique_mem.find(mem[i]);
					if (it == unique_mem.end()) {
						unique_mem.insert(mem[i]);
					}
					QASSERT(it == unique_mem.end()) << "All memory should be unique.\n";
				}
			}
		}
		/*
		{
			std::vector<int*> mem;
			Allocator pool(1 << 25);
			for (int i = 0; i < 1 << 20; ++i) {
				int count = rand() % 100;
				int* new_mem = pool.alloc(count);
				mem.push_back(new_mem);
			}

			std::set<int*> unique_mem;
			for (int i = 0; i < mem.size(); ++i) {
				if (mem[i] != nullptr) {
					std::set<int*>::iterator it = unique_mem.find(mem[i]);
					if (it == unique_mem.end()) {
						unique_mem.insert(mem[i]);
					}
					QASSERT(it == unique_mem.end()) << "All memory should be unique.";
				}
			}

			UINT count = 0;
			for (int i = 0; i < pool.free_mem_.size(); ++i) {
				count += pool.free_mem_[i].size();
			}
			ASSERT(count == 0) << "All memory should be used";

			while (!mem.empty()) {
				UINT count = mem.size();
				int* old_mem = mem.back();
				pool.free(old_mem);
				mem.pop_back();
			}
			count = 0;
			for (int i = 0; i < pool.free_mem_.size(); ++i) {
				count += pool.free_mem_[i].size();
			}
			ASSERT(count == 1) << "All memory should be freed.";
		}
		{
			std::vector<int*> mem;
			Allocator pool(1 << 25);
			for (int i = 0; i < 1 << 20; ++i) {
				int count = rand() % 100;
				int* new_mem = pool.alloc(count);
				mem.push_back(new_mem);
			}

			std::set<int*> unique_mem;
			for (int i = 0; i < mem.size(); ++i) {
				if (mem[i] != nullptr) {
					std::set<int*>::iterator it = unique_mem.find(mem[i]);
					if (it == unique_mem.end()) {
						unique_mem.insert(mem[i]);
					}
					QASSERT(it == unique_mem.end()) << "All memory should be unique.";
				}
			}

			UINT count = 0;
			for (int i = 0; i < pool.free_mem_.size(); ++i) {
				count += pool.free_mem_[i].size();
			}
			ASSERT(count == 0) << "All memory should be used";

			pool.clear();
			count = 0;
			for (int i = 0; i < pool.free_mem_.size(); ++i) {
				count += pool.free_mem_[i].size();
			}
			ASSERT(count == 1) << "All memory should be freed.";
		}
		*/
	}
};

int main() {
	{
		ObjectPoolTester<BuddySystemAllocator<int>> tester;
		tester.run_tests();
	}

	typedef Big TestType;
	UINT test_count = 100;
	UINT alloc_count = 100;
	{
		std::cout << "\nObjectPool:\n";
		Timer alloc_t, free_t;
		for (UINT num = 0; num < test_count; ++num) {
			std::vector<TestType*> mem;
			BuddySystemAllocator<TestType> pool(1 << 22);
			for (int i = 0; i < 1000; ++i) {
				int count = rand() % alloc_count;
				if (count == 0) {
					count = 1;
				}
				alloc_t.start();
				TestType* new_mem = pool.alloc(count);
				alloc_t.stop();
				if (new_mem) {
					mem.push_back(new_mem);
				}
			}

			while (!mem.empty()) {
				UINT count = mem.size();
				TestType* old_mem = mem.back();
				free_t.start();
				pool.free(old_mem);
				free_t.stop();
				mem.pop_back();
			}
		}

		std::cout << "\tAlloc Time: "<< alloc_t.getAvgElapsed() << '\n';
		std::cout << "\tFree Time: " << free_t.getAvgElapsed() << '\n';
		std::cout << "\tTotal Time: " << alloc_t.getAvgElapsed() + free_t.getAvgElapsed() << '\n';
	}

	{
		std::cout << "\nMalloc:\n";
		Timer alloc_t, free_t;
		for (UINT num = 0; num < test_count; ++num) {
			std::vector<TestType*> mem;
			for (int i = 0; i < 1000; ++i) {
				int count = rand() % alloc_count;
				if (count == 0) {
					count = 1;
				}
				alloc_t.start();
				TestType* new_mem = new TestType[count];
				alloc_t.stop();
				if (new_mem) {
					mem.push_back(new_mem);
				}
			}

			while (!mem.empty()) {
				UINT count = mem.size();
				TestType* old_mem = mem.back();
				free_t.start();
				delete[] old_mem;
				free_t.stop();
				mem.pop_back();
			}
		}

		std::cout << "\tAlloc Time: " << alloc_t.getAvgElapsed() << '\n';
		std::cout << "\tFree Time: " << free_t.getAvgElapsed() << '\n';
		std::cout << "\tTotal Time: " << alloc_t.getAvgElapsed() + free_t.getAvgElapsed() << '\n';
	}

	while (1);
	return 0;
}


#if 0
int main() {
	Timer t;
	UINT count = 500;
	ObjectPool<Big> p(count);
	Vector<Big*> mem;
	Queue<Big*> q(500);
	mem.reserve(count);
	for (int i = 0; i < 1000000; ++i) {
		t.start();
		for (int j = 0; j < 1; ++j) {
			q.enqueue(nullptr);
			q.dequeue();
		}
		t.stop();
		
		if (i % 1000 == 0) {
			//std::cout << i << std::endl;
		}
	}
	std::cout << t.getAvgCount() << std::endl;
	std::cout << t.getAvgElapsed() << std::endl;

	t.reset();
	Big* pmem;
	for (int i = 0; i < 1000000; ++i) {
		t.start();
		pmem = new Big[500];
		delete[] pmem;
		t.stop();
		if (i % 1000 == 0) {
			//std::cout << i << std::endl;
		}
	}
	std::cout << t.getAvgCount() << std::endl;
	std::cout << t.getAvgElapsed();

	while (1);
	return 0;
}
#endif
#endif  // TESTING_OBJECTPOOL

#ifdef TESTING_PROMISE


template <typename T>
struct function_traits
	: public function_traits<decltype(&T::operator())>
{};
// For generic types, directly use the result of the signature of its 'operator()'

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
	// we specialize for pointers to member function
{
	enum { arity = sizeof...(Args) };
	// arity is the number of arguments.

	typedef ReturnType result_type;

	template <size_t i>
	struct arg
	{
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		// the i-th argument is equivalent to the i-th tuple element of a tuple
		// composed of those arguments.
	};
};




class SuperPromise
{
public:
	//virtual void resolve() = 0;
	virtual void resolve_(char first...) = 0;
	virtual void resolve_(char first, SuperPromise* promise) = 0;
	virtual void resolve_(char first, std::function<void(char...)> f) = 0;

protected:
	enum class State {
		PENDING,
		FULFILLED,
		REJECTED
	};
	State state_;
	std::queue<SuperPromise*> promises_;
	std::string reason_;
};

#define resolve(...) resolve_(0, __VA_ARGS__)

template<typename T>
class Promise;

template <typename Ret_, typename Param_>
class Promise<Ret_(Param_)> : public SuperPromise
{
public:
	using ret_type = Ret_;
	using param_type = Param_;
	using Function = std::function<ret_type(param_type)>;

private:
	enum class State {
		PENDING,
		FULFILLED,
		REJECTED
	};

	std::queue<SuperPromise*> promises_;
	State state_;

	Function on_fulfilled_ = nullptr;
	Function on_rejected_ = nullptr;
	ret_type val_;
	std::string reason_;

public:
	Promise() { }
	Promise(Function on_fulfilled, Function on_rejected) :
		on_fulfilled_(on_fulfilled),
		on_rejected_(on_rejected),
		state_(State::PENDING) {
	}

	template <typename Ret>
	Promise<Ret(ret_type)>& then(std::function<Ret(ret_type)> on_fulfilled) {
		Promise<Ret(ret_type)>* ret = new Promise<Ret(ret_type)>(on_fulfilled, nullptr);
		promises_.push(ret);
		return *ret;
	}

	void resolve_(char first...) override {
		va_list args;
		va_start(args, first);
		Param_ param = va_arg(args, Param_);
		if (on_fulfilled_) {
			val_ = on_fulfilled_(param);
		}
		while (!promises_.empty()) {
			SuperPromise* promise = promises_.front();
			promises_.pop();
			promise->resolve(val_);
		}
		va_end(args);
	};

	void resolve_(char first, SuperPromise* promise) override {
		if (promise != this) {
			*this = *promise;
		}
	}

	void resolve_(char first, std::function<void(char...)> f) override {
	}

	//void resolve_(char first, SuperPromise*) override { }

	//void resolve(Ret_ ret);
	void reject(const std::string& reason);
	//Promise* then(VoidFunction on_fulfilled);
};

template <typename Param_>
class Promise<void(Param_)> : public SuperPromise
{
public:
	using ret_type = void;
	using param_type = Param_;
	using Function = std::function<ret_type(param_type)>;

public:
	Promise() { }

	template <typename Ret>
	Promise<Ret(param_type)>& then(std::function<Ret(param_type)> on_fulfilled) {
		Promise<Ret(param_type)>* ret = new Promise<Ret(param_type)>(on_fulfilled, nullptr);
		promises_.push(ret);
		return *ret;
	}

	void resolve_(char first...) override {
		va_list args;
		va_start(args, first);
		param_type param = va_arg(args, param_type);
		va_end(args);
		while (!promises_.empty()) {
			SuperPromise* promise = promises_.front();
			promises_.pop();
			promise->resolve(param);
		}
	};

	void resolve_(char first, SuperPromise* promise) override {
		if (promise != this) {
			*this = *promise;
		}
	}

	void resolve_(char first, std::function<void(char...)> f) override {
	}

	//void resolve(Ret_ ret);
	void reject(const std::string& reason);
	//Promise* then(VoidFunction on_fulfilled);
};

/*template <typename Ret_, typename Param_>
void SubPromise<Ret_(Param_)>::resolve(Ret_ ret) {
	this->val_ = ret;
}*/

/*template <typename Ret_, typename Param_>
void SubPromise<Ret_(Param_)>::reject(const std::string& reason) {
	this->reason_ = reason;
}*/

/*template <typename Ret_, typename Param_>
Promise* SubPromise<Ret_(Param_)>::then(SubPromise<Ret_(Param_)>::Function on_fulfilled) {
	return nullptr;
}*/


class Any
{
	virtual Any* clone() = 0;
	virtual void destroy() = 0;
};

template<typename Ty_>
class Val : public Any
{
public:
	Ty_ val;
	Any* clone() override {
		return new Val(val);
	}

	void destroy() override {
		delete this;
	}
};

int main() {
	Promise<void(char)> p;
	p.then <double> ([](char a) {
		std::cout << a << '\n';
		return 2.0;
	}).then<int>([](double b) {
		std::cout << b << '\n';
		return 1;
	}).then<Promise<int(int)>>([]() {
	});
	p.resolve('s');
	
	while (1);
}
#endif  // TESTING_PROMISE

#ifdef TESTING_EVENT

class MockScheduler : public AbstractScheduler
{
private:
	std::queue<AbstractProcess*> queued_;
public:
	MockScheduler(const std::string& name): AbstractScheduler(name) {}

	// Queue an already made process to be scheduled.
	INT queue(AbstractProcess* process) {
		queued_.push(process);
		return 0;
	}

	// Queue a batch of processes at once.
	INT queue_batch(AbstractProcess** process, UINT count) {
		for (UINT i = 0; i < count; ++i) {
			queued_.push(process[i]);
		}
		return 0;
	}

	// Start the scheduler.
	INT start() {
		std::cout << "Scheduler '" + name() + "' started.\n";
		return 0;
	}

	// Stop the scheduler.
	INT stop() {
		std::cout << "Scheduler '" + name() + "' stopped.\n";
		return 0;
	}

	INT step() {
		if (!queued_.empty()) {
			AbstractProcess* process = queued_.front();
			queued_.pop();
			if (process->on_start) {
				process->on_start(process);
			}
			if (process->on_run) {
				process->on_run(process);
			}
			if (process->on_stop) {
				process->on_stop(process);
			}
		}
		return 0;
	}
};

namespace helper
{
	template <int... Args_>
	struct index {};

	template <int N, int... Args_>
	struct gen_seq : gen_seq<N - 1, N - 1, Args_...> {};

	template <int... Args_>
	struct gen_seq<0, Args_...> : index<Args_...> {};
}

class AbstractEvent : public AbstractProcess
{
private:
	static INT on_run_(AbstractProcess* me) {
		return ((AbstractEvent*)me)->on_run();
	}

public:
	AbstractEvent() : AbstractProcess(this) {
		((AbstractProcess*)this)->on_run = AbstractEvent::on_run_;
	}

	virtual ~AbstractEvent() { }

	virtual INT on_run() = 0;
};

template <typename... Args_>
class Event : public AbstractEvent
{
private:
	std::function<INT(Args_...)> f;
	std::tuple<Args_...> args_;

	template <typename... Args, int... Is>
	INT func(std::tuple<Args...>& tup, helper::index<Is...>) {
		return f(std::get<Is>(tup)...);
	}

	template <typename... Args>
	INT func(std::tuple<Args...>& tup) {
		return func(tup, helper::gen_seq<sizeof...(Args)>{});
	}

public:
	template <typename F>
	Event(F&& func)
		: f(std::forward<F>(func))
	{}

	template <typename... Args>
	INT set_args(Args&&... args) {
		args_ = std::tuple<Args_...>(std::forward<Args>(args)...);
		return 0;
	}
	
	INT on_run() override {
		return func(args_);
	}
};

template <>
class Event<void> : public AbstractEvent
{
private:
	std::function<INT(void)> f;

public:
	template <typename F>
	Event(F&& func)
		: f(std::forward<F>(func))
	{}

	template <typename... Args>
	INT set_args(Args&&... args) {
		return 0;
	}

	INT on_run() override {
		return f();
	}
};

template <typename F, typename... Args>
Event<Args...> make_event(F&& f, Args&&... args) {
	return Event<Args...>(std::forward<F>(f), std::forward<Args>(args)...);
}

class AbstractEventType
{
public:

};


template <typename... Args_>
class EventType : public AbstractEventType
{
private:
	std::function<INT(Args_...)> func_;

public:
	template <typename F>
	EventType(F&& f):
	func_(std::forward<F>(f)) { }

	Event<Args_...>* make() {
		return new Event<Args_...>(func_);
	}
};


class EventManager
{
private:
	typedef std::vector<AbstractEventType*> Processes;
	typedef std::map<std::string, Processes*> Events;
	
	Events events_;
	AbstractScheduler* scheduler_;

public:
	EventManager(AbstractScheduler* scheduler)
		:scheduler_(scheduler) {}

	template <typename... Args>
	INT publish(const char* event_name, Args&&... args) {
		Events::iterator it = events_.find(event_name);
		if (it == events_.end()) {
			return -1;
		}
		std::vector<AbstractEvent*> new_processes;
		Processes *processes = it->second;
		UINT size = processes->size();
		for (UINT i = 0; i < size; ++i) {
			EventType<Args...>* type = (EventType<Args...>*)processes->at(i);
			Event<Args...>* event = type->make();
			event->set_args(std::forward<Args...>(args)...);
			new_processes.push_back(event);
		}
		return scheduler_->queue_batch((AbstractProcess**)new_processes.data(), new_processes.size());
	}

	template <typename F, typename... Args>
	INT subscribe(const char* event_name, F&& func) {
		Events::iterator it = events_.find(event_name);
		EventType<Args...>* new_type = new EventType<Args...>(func);
		if (it == events_.end()) {
			(events_[event_name] = new Processes())->push_back(new_type);
		} else {
			it->second->push_back(new_type);
		}
		return 0;
	}

	INT unsubscribe(const char* event_name, AbstractProcess* process) {
		/*Events::iterator it = events_.find(event_name);
		if (it != events_.end()) {
			Processes* processes = it->second;
			Processes::iterator it = std::find(processes->begin(), processes->end(), process);
			processes->erase(it);
		}*/
		return 0;
	}
};

MockScheduler scheduler("mock-scheduler");
EventManager event_manager(&scheduler);

//#define bind(member_function, self, ...) [&](){ ((self).*(member_function))(__VA_ARGS__); }

class PrintObj
{
public:
	INT on_print(const char* msg) {
		std::cout << msg << std::endl;
		return 0;
	}
};

int main() {
	/*
	event_manager.subscribe<const char*>("print", [](const char* msg) {
		std::cout << msg << std::endl;
		return 0;
	});*/

	PrintObj printer;
	event_manager.subscribe("print", std::bind(&PrintObj::on_print, printer));

	event_manager.publish("print", (const char*)"hello");
	event_manager.publish("print", (const char*)"world");
	scheduler.step();
	scheduler.step();

	while (1);
	return 0;
}

#endif  // TESTING_EVENT

#ifdef TESTING_EVENTS_SECOND_TRY

class MockScheduler : public AbstractScheduler
{
private:
	std::queue<AbstractProcess*> queued_;
public:
	MockScheduler(const std::string& name) : AbstractScheduler(name) {}

	// Queue an already made process to be scheduled.
	INT queue(AbstractProcess* process) {
		queued_.push(process);
		return 0;
	}

	// Queue a batch of processes at once.
	INT queue_batch(AbstractProcess** process, UINT count) {
		for (UINT i = 0; i < count; ++i) {
			queued_.push(process[i]);
		}
		return 0;
	}

	// Start the scheduler.
	INT start() {
		std::cout << "Scheduler '" + name() + "' started.\n";
		return 0;
	}

	// Stop the scheduler.
	INT stop() {
		std::cout << "Scheduler '" + name() + "' stopped.\n";
		return 0;
	}

	INT step() {
		if (!queued_.empty()) {
			AbstractProcess* process = queued_.front();
			queued_.pop();
			if (process->on_start) {
				process->on_start(process);
			}
			if (process->on_run) {
				process->on_run(process);
			}
			if (process->on_stop) {
				process->on_stop(process);
			}
		}
		return 0;
	}
};

class AbstractEvent;

class EventManager
{
public:
	typedef UINT64 Id;

	class Subscription {
	public:
		Id id;
	};

private:
	typedef std::function<void(AbstractEvent*)> EventWrapper;
	typedef std::unordered_map<Id, EventWrapper*> EventList;
	typedef std::unordered_map<std::string, EventList*> Events;

	typedef std::tuple<std::string, AbstractEvent*> EventElement;
	typedef std::queue<EventElement> EventQueue;

	EventQueue queue_[2];
	EventQueue& read_queue_;
	EventQueue& write_queue_;

	Events events_;

	Id current_id_;


	// This lock may not be necessary all the time, but it's needed to attain
	// correctness. Some code may want to subscribe at the same time, or may want
	// to subscribe while the events are flushing.
	std::mutex events_lock_;

	// We need the write_queue_lock because many people may simulataneously
	// want to publish an event, or we need to swap the read/write buffers.
	std::mutex write_queue_lock_;

public:
	EventManager(): read_queue_(queue_[0]), write_queue_(queue_[1]), current_id_(0) {
	}

	void publish(const std::string& event_type, AbstractEvent* event) {
		std::lock_guard<std::mutex> lock(write_queue_lock_);
		write_queue_.push(std::move(EventElement{ event_type, event }));
	}

	void flush() {

		// Flush and publish can be run asynchronously with respect to each other.
		// Thus, we need to swap the buffers in an atomic operation.
		{
			std::lock_guard<std::mutex> lock(write_queue_lock_);
			std::swap(read_queue_, write_queue_);
		}

		std::lock_guard<std::mutex> lock(events_lock_);

		while (!read_queue_.empty()) {
			EventElement el = read_queue_.front();
			AbstractEvent* event = std::get<1>(el);

			Events::iterator event_it = events_.find(std::get<0>(el));
			if (event_it != events_.end()) {
				EventList* list = event_it->second;
				EventList::iterator it = list->begin();
				EventList::iterator end = list->end();
				while (it != end) {
					EventWrapper& wrapper = *(it->second);
					wrapper(event);
					++it;
				}
			}
			read_queue_.pop();
		}
	}

	void unsubscribe(const std::string& event_type, Subscription subscription) {
		if (subscription.id != 0) {
			std::lock_guard<std::mutex> lock(events_lock_);
			Events::iterator it = events_.find(event_type);
			if (it != events_.end()) {
				EventList* list = it->second;
				EventList::iterator event_it = list->find(subscription.id);
				if (event_it != list->end()) {
					delete event_it->second;
					list->erase(event_it);
				}
			}
		}
	}

	template<class Ty, class F>
	Subscription subscribe(const std::string& event_type, F&& handler) {
		static_assert(
			std::is_base_of<AbstractEvent, Ty>::value,
			"Ty_ must be a descendant of AbstractEvent"
			);
		std::lock_guard<std::mutex> lock(events_lock_);

		EventWrapper wrapper = [=](AbstractEvent* event) {
			handler((Ty*)event);
		};

		Subscription sub{ ++current_id_ };
		Events::iterator it = events_.find(event_type);
		EventList* list;
		if (it == events_.end()) {
			list = new EventList();
			events_[event_type] = list;
		} else {
			list = it->second;
		}
		list->insert(std::move(std::pair<Id, EventWrapper*>(sub.id, new EventWrapper(wrapper))));
		return sub;
	}

	template<class Ty, class F, class C>
	Subscription subscribe(const std::string& event_type, F&& handler, C&& c) {
		static_assert(
			std::is_base_of<AbstractEvent, Ty>::value,
			"Ty_ must be a descendant of AbstractEvent"
			);
		std::lock_guard<std::mutex> lock(events_lock_);

		EventWrapper wrapper = [=](AbstractEvent* event) {
			((c)->*handler)((Ty*)event);
		};

		Subscription sub{ ++current_id_ };
		Events::iterator it = events_.find(event_type);
		EventList* list;
		if (it == events_.end()) {
			list = new EventList();
			events_[event_type] = list;
		}
		else {
			list = it->second;
		}
		list->insert(std::move(std::pair<Id, EventWrapper*>(sub.id, new EventWrapper(wrapper))));
		return sub;
	}
};

EventManager eventManager;

class AbstractEvent
{

};

class AbstractEventDispatch
{
protected:
	EventManager* manager_;

public:
	AbstractEventDispatch() {
		manager_ = &eventManager;
	}
};

class KeyboardEvent : public AbstractEvent
{
public:
	KeyboardEvent() : key('\0') { }
	KeyboardEvent(char key) : key(key) { }
	char key;
};

#define DEFINE_NEW_EVENT(event_name) \
EventManager::Subscription event_name##_subscription;																	\
template<class F> void subscribe_to_##event_name(F&& f) {																\
	event_name##_subscription = manager_->subscribe<EventType>(#event_name, std::forward<F>(f));						\
}																														\
template<class F, class C> void subscribe_to_##event_name(F&& f, C&& c) {												\
	event_name##_subscription = manager_->subscribe<EventType>(#event_name, std::forward<F>(f), std::forward<C>(c));	\
}																														\
void unsubscribe_to_##event_name () {																					\
	manager_->unsubscribe(#event_name, event_name##_subscription);														\
}																														\
void send_##event_name(EventType* to_event) {																			\
	manager_->publish(#event_name, to_event);																			\
}																														\
template<class... Args>	void send_##event_name(Args&&... args) {														\
	manager_->publish(#event_name, new EventType(std::forward<Args...>(args)...));										\
}																														\
template<> void send_##event_name() {																					\
	manager_->publish(#event_name, new EventType());																	\
}

#define DEFINE_NEW_EVENT_DISPATCH(event_name, event_type, events) \
class event_name : public AbstractEventDispatch {	\
public:												\
	typedef event_type EventType;					\
	events											\
}

class KeyboardEvents : public AbstractEventDispatch {
public:
		typedef KeyboardEvent EventType;
		~KeyboardEvents() {
			unsubscribe_to_key_press();
			unsubscribe_to_key_release();
		};

		DEFINE_NEW_EVENT(key_press);
		DEFINE_NEW_EVENT(key_release);
};

#undef DEFINE_NEW_EVENT_DISPATCH
#undef DEFINE_NEW_EVENT

class TestObj
{
public:
	KeyboardEvents keyboard_events;

	int on_key_press(KeyboardEvents::EventType* event) {
		KeyboardEvents keyboard_events;
		std::cout << event->key;
		return 0;
	}

	void construct() {
		keyboard_events.subscribe_to_key_press(&TestObj::on_key_press, this);
		keyboard_events.subscribe_to_key_release([this](KeyboardEvents::EventType*) {
			this->move();
		});
	}

	void move() {

	}
};

int main()
{
	KeyboardEvents keyboard_events;
	TestObj obj;
	obj.construct();

	keyboard_events.send_key_press('h');
	keyboard_events.send_key_press('e');
	keyboard_events.send_key_press('l');
	keyboard_events.send_key_press('l');
	keyboard_events.send_key_press('o');
	keyboard_events.send_key_press(',');
	keyboard_events.send_key_press(' ');
	keyboard_events.send_key_press('w');
	keyboard_events.send_key_press('o');
	keyboard_events.send_key_press('r');
	keyboard_events.send_key_press('l');
	keyboard_events.send_key_press('d');
	keyboard_events.send_key_press('!');
	keyboard_events.send_key_press('\n');

	eventManager.flush();

	keyboard_events.send_key_press('h');
	keyboard_events.send_key_press('e');
	keyboard_events.send_key_press('l');
	keyboard_events.send_key_press('l');
	keyboard_events.send_key_press('o');
	keyboard_events.send_key_press(' ');
	keyboard_events.send_key_press('w');
	keyboard_events.send_key_press('o');
	keyboard_events.send_key_press('r');
	keyboard_events.send_key_press('l');
	keyboard_events.send_key_press('d');
	keyboard_events.send_key_press('!');
	keyboard_events.send_key_press('\n');
	eventManager.flush();

	while (1);
	return 0;
}

#endif

#ifdef TESTING_DYNAMIC_TYPES



class Type
{

};

int main()
{
	return 0;
}

#endif  // TESTING_DYNAMIC_TYPES

#ifdef TESTING_PIPELINE

class ObjectType
{
public:
	virtual void update() = 0;
	virtual void render() = 0;
};

class CacheBall : ObjectType
{
public:
	struct UpdateState {
		double x, y, z;
		double vx, vy, vz;
	};

	struct RenderState {
		char render_state[128];
		double x, y, z;
	};
};

class NoCacheBall
{
public:
	NoCacheBall() {
		x = y = z = 0;
		vx = vy = vz = 0;
	}
	double x, y, z;
	double vx, vy, vz;
	int render_state[128];

	void update() {
		x += vx;
	}

	void render() {
		x += vx;
	}
};

template<class ObjectType_>
class ObjectCollection
{
private:
	template <class Ty_>
	using Collection = std::vector<Ty_>;

	typedef typename ObjectType_::UpdateState UpdateState;
	typedef typename ObjectType_::RenderState RenderState;

	Collection<UpdateState> update_states_;
	Collection<RenderState> render_states_;

	UINT size_;

public:
	void add(UINT count) {
		update_states_.reserve(size_ + count);
		for (UINT i = 0; i < count; ++i) {
			update_states_.push_back(std::move(UpdateState()));
			render_states_.push_back(std::move(RenderState()));
		}
		size_ = update_states_.size();
	}

	template<class F>
	void update(F&& f) {
		for (UINT i = 0; i < size_; ++i) {
			f(update_states_[i]);
		}
	}

	template<class F>
	void render(F&& f) {
		for (UINT i = 0; i < size_; ++i) {
			f(render_states_[i]);
		}
	}
};

double test_cache_lambda(int count, int times) {
	std::cout << __func__ << ": ";
	ObjectCollection<CacheBall> balls;
	balls.add(count);
	Timer t;

	auto update = [](CacheBall::UpdateState& state) {
		state.x += state.vx;
	};

	auto interpolate = [](CacheBall::UpdateState& state, const CacheBall::UpdateState& prev_state) {
		state = prev_state;
	};

	auto render = [](CacheBall::UpdateState& state, const CacheBall::UpdateState& prev_state) {
		state.x += state.vx;
	};

	for (int i = 0; i < times; ++i) {
		t.start();
		balls.update(std::move(interpolate));
		balls.update(std::move(update));
		balls.update(std::move(render));
		t.stop();
	}
	return t.getAvgElapsed();
}

void interpolate_cache(CacheBall::UpdateState& state, const CacheBall::UpdateState& prev_state) {
	state = prev_state;
}

void update_cache(CacheBall::UpdateState& state) {
	state.x += state.vx;
}

void render_cache(CacheBall::UpdateState& state, const CacheBall::UpdateState& prev_state) {
	state.x += state.vx;
}

double test_cache_fn_ptr(int count, int times) {
	std::cout << __func__ << ": ";
	ObjectCollection<CacheBall> balls;
	balls.add(count);
	Timer t;
	for (int i = 0; i < times; ++i) {
		t.start();
		balls.update(interpolate_cache);
		balls.update(update_cache);
		balls.update(render_cache);
		t.stop();
	}
	return t.getAvgElapsed();
}

void update(std::vector<NoCacheBall>& b) {
	UINT size = b.size();
	for (UINT j = 0; j < size; ++j) {
		b[j].update();
	}
}

double test_update_normal(int count, int times) {
	std::cout << __func__ << ": ";
	std::vector<NoCacheBall> b(count);
	Timer t;
	for (int i = 0; i < times; ++i) {
		t.start();
		update(b);
		t.stop();
	}
	return t.getAvgElapsed();
}

int main()
{
	std::cout << test_cache_lambda(10000, 1000) / 1e6 << "ms" << std::endl;
	std::cout << test_cache_fn_ptr(10000, 1000) / 1e6 << "ms" << std::endl;
	std::cout << test_update_normal(10000, 1000) / 1e6 << "ms" << std::endl;
	while (1);
}

#endif

#ifdef TESTING_NEW_GAME_LOOP

struct Position {
	float x, y;
};

struct Velocity {
	float vx, vy;
};

struct Data {
	UINT size = 0;
	INT data[10];
	INT* handles[10];
};

template <class TableMetadata>
using TableData = std::map<typename TableMetadata::Key, typename TableMetadata::Value>;

template <class... Args>
using Entry = std::tuple<Args...>;

struct InstanceTable {
	// Entity
	typedef Entry<Id> Key;

	// Instance
	typedef Entry<Id> Value;

	typedef TableData<InstanceTable> DataType;

	TableData<InstanceTable> data;
};

struct ComponentTable {
	// Entity
	typedef Entry<Id> Key;

	// Component
	typedef Entry<Id> Value;

	typedef TableData<ComponentTable> DataType;

	TableData<ComponentTable> data;
};

struct Component { };

typedef std::map<Id, Component*> ComponentTables;

struct PositionTable : Component {
	// Entity, Instance
	typedef Entry<Id, Id> Key;

	// Position
	typedef Entry<Position> Value;

	typedef TableData<PositionTable> DataType;

	DataType data;
};

struct VelocityTable : Component {
	// Entity, Instance
	typedef Entry<Id, Id> Key;

	// Velocity
	typedef Entry<Velocity> Value;

	typedef TableData<VelocityTable> DataType;

	DataType data;
};

namespace Query
{

template<class Table>
using TableLambda = ::std::function<INT(typename Table::Key&, typename Table::Value&)>;

template<class Table>
INT forEach(Table& table, TableLambda<Table>&& f) {
	Table::TableData::iterator it = table.data.begin();
	return -1;
}

template<class Table>
INT sort(Table& table, TableLambda<Table>&& f) {
	return -1;
}

template<class Table>
INT join(Table& table, TableLambda<Table>&& f) {
	return -1;
}

}

Id new_entity(Id* entity_index) {
	return *entity_index++;
}

void new_component(Id entity) {
}

template<class Ty_>
Ty_& get_component(ComponentTables& components, Id entity, Id instance, Id component) {

}



void game_loop() {
	Id entity_index;
	Id ball = new_entity(&entity_index);
	Id block = new_entity(&entity_index);

	InstanceTable instances;
	ComponentTable components;
	VelocityTable velocities;
	PositionTable positions;
	
	while (1) {
	}
}

int main() {
	std::tuple<int, int, int> a{ 0,0,0 };
	std::tuple<int, int, int> b{ 0,0,1 };
	std::pair<std::pair<int, int>, int> p00{ { 0,0 }, 0 };
	std::pair<std::pair<int, int>, int> p01{ { 0,1 }, 0 };
	std::pair<std::pair<int, int>, int> p02{ { 0,2 }, 0 };
	std::pair<std::pair<int, int>, int> p10{ { 1,0 }, 0 };
	std::pair<std::pair<int, int>, int> p11{ { 1,1 }, 0 };
	std::pair<std::pair<int, int>, int> p12{ { 1,2 }, 0 };

	std::cout << (p12 > p11) << std::endl;
	std::cout << (p11 > p10) << std::endl;
	std::cout << (p10 > p02) << std::endl;
	std::cout << (p02 > p02) << std::endl;
	std::cout << (p02 > p01) << std::endl;
	std::cout << (p10 > p02) << std::endl;

	while (1);
}

#endif  // TESTING_NEW_GAME_LOOP

#ifdef TESTING_ECS

#ifdef __ENGINE_DEBUG__
#define LOG(stream) stream << "\nFILE: " << __FILE__ << "\nLINE: " << __LINE__ << "\n"
#define INFO ::std::cout
#define ERR ::std::cerr
#else
#define LOG(stream) if (false) {} else std::cout
#define INFO
#define ERR
#endif

#if 0

template <class... Tables>
using JoinType = ComponentTable<std::tuple<typename Tables::Component*...>>;

template<class Left, class Right>
void join(Left* left, Right* right, JoinType<Left, Right>* out) {
	if (left->data.size() <= right->data.size()) {
		typename Left::Data::iterator left_it = left->data.begin();
		typename Left::Data::iterator left_end_it = left->data.end();
		typename Right::Data::iterator right_end_it = right->data.end();

		while (left_it != left_end_it) {
			typename Right::Data::iterator right_it = right->data.find(left_it->first);
			if (right_it != right_end_it) {
				out->data[left_it->first] = std::tuple<typename Left::Component*, typename Right::Component*>(&left_it->second, &right_it->second);
			}
			++left_it;
		}
	} else {
		typename Right::Data::iterator right_it = right->data.begin();
		typename Right::Data::iterator right_end_it = right->data.end();
		typename Left::Data::iterator left_end_it = left->data.end();

		while (right_it != right_end_it) {
			typename Left::Data::iterator left_it = left->data.find(right_it->first);
			if (left_it != left_end_it) {
				out->data[right_it->first] = std::tuple<typename Left::Component*, typename Right::Component*>(&left_it->second, &right_it->second);
			}
			++right_it;
		}
	}
}

template<class Left, class Right, class F>
void join(Left* left, Right* right, F& f) {
	if (left->data.size() <= right->data.size()) {
		typename Left::Data::iterator left_it = left->data.begin();
		typename Left::Data::iterator left_end_it = left->data.end();
		typename Right::Data::iterator right_end_it = right->data.end();

		while (left_it != left_end_it) {
			typename Right::Data::iterator right_it = right->data.find(left_it->first);
			if (right_it != right_end_it) {
				f(left_it->first, std::tuple<Left::Component*, Right::Component*>(&left_it->second, &right_it->second));
			}
			++left_it;
		}
	} else {
		typename Right::Data::iterator right_it = right->data.begin();
		typename Right::Data::iterator right_end_it = right->data.end();
		typename Left::Data::iterator left_end_it = left->data.end();

		while (right_it != right_end_it) {
			typename Left::Data::iterator left_it = left->data.find(right_it->first);
			if (left_it != left_end_it) {
				f(right_it->first, std::tuple<Left::Component*, Right::Component*>(&left_it->second, &right_it->second));
			}
			++right_it;
		}
	}
}
#endif

struct PhysicsComponent {
	Vec2 position;
	Vec2 velocity;
};

struct BulletComponent {
	bool can_penetrate_walls = false;
};

struct RenderingComponent {
	Vec3 color;
	int width;
	int height;
};

typedef Id Entity;

template <class TableMetadata>
using TableData = std::map<Entity, typename TableMetadata::Component>;

class TableInterface {
public:
	virtual INT make(Entity entity) = 0;

	virtual INT destroy(Entity entity) = 0;
};

template <class ComponentType>
class ComponentTable : public TableInterface {
public:
	typedef ComponentType Component;

	typedef TableData<ComponentTable<Component>> Data;

	INT make(Entity entity) override {
		data[entity] = Component();
		return 0;
	}

	INT destroy(Entity entity) override {
		data.erase(data.find(entity));
		return 0;
	}

	Data data;
};

class SystemInterface {
public:
	virtual INT run() = 0;
};

template <class ComponentTable>
class System : public SystemInterface {
public:
	INT run() override {

	}
};

enum class ComponentIndices {
	Physics,
	Bullet,
	Rendering,
	Positions,
	Velocities
};

namespace Test
{

typedef INT64 Handle;

template<class... Keys>
using Key = std::tuple<Keys...>;

template<class... Components>
using Component = std::tuple<Components...>;

template <class Key, class Component>
class SortedTable {
public:
	typedef Key Key;
	typedef Component Component;
	typedef std::vector<Key> Keys;
	typedef std::vector<Component> Data;
	
	INT make(Key&& key, Component&& component) {
		if (keys.size() > 0) {
			UINT index = find(key);
			if (index == end()) {
				keys.insert(keys.begin() + index, key);
				data.insert(data.begin() + index, std::forward<Component>(component));
			}
		} else {
			keys.emplace_back(key);
			data.emplace_back(std::forward<Component>(component));
		}
		return 0;
	}

	Component& operator[](Key key) {
		return data[find(key)];
	}

	const Component& operator[](Key key) const {
		return data[find(key)];
	}

	INT destroy(Key key) {
		return 0;
	}

	Data data;
	Keys keys;

private:
	UINT find(Key key) {
		INT low = 0;
		INT high = keys.size() - 1;
		INT mid;
		INT search_range = 16;
		while (low <= high - search_range) {
			mid = low + ((high - low) / 2);
			Key& search = keys[mid];
			if (search > key) {
				high = mid - 1;
			} else {
				low = mid + 1;
			}
		}
		UINT index = low;
		UINT size = keys.size();
		while(index < high + 1){
			if (key == keys[index]) {
				break;
			}
			++index;
		}
		return index;
	}

	UINT begin() const {
		return 0;
	}

	UINT end() const {
		return keys.size();
	}
};

template<class V>
void print_vector(const std::string& name, V&& vector) {
	std::cout << name << ":" << std::endl;
	std::cout << "[";
	for (UINT i = 0; i < vector.size(); ++i) {
		std::cout << vector[i] << " ";
	}
	std::cout << "]";
	std::cout << std::endl;
}

template <class Component, class Allocator=std::allocator<Component>>
class Table {
public:
	typedef Entity Key;
	typedef Component Component;

	typedef ::boost::container::vector<Key> Keys;
	typedef ::boost::container::vector<Component> Components;

	// For fast lookup if you have the handle to an entity.
	typedef ::boost::container::vector<UINT> Handles;
	typedef ::boost::container::vector<Handle> FreeHandles;

	// For fast lookup by Entity Id.
	typedef ::boost::container::map<Key, Handle> Index;

	Table() {}

	Table(std::vector<std::tuple<Key, Component>>&& init_data) {
		for (auto& t : init_data) {
			insert(std::move(std::get<0>(t)), std::move(std::get<1>(t)));
		}
	}

	Handle insert(Entity e, Component&& component) {
		Handle handle = make_handle();

		index_[e] = handle;

		components.push_back(std::move(component));
		keys.push_back(e);
		return handle;
	}

	Component& operator[](Handle handle) {
		return components[handles_[handle]];
	}

	const Component& operator[](Handle handle) const {
		return components[handles_[handle]];
	}

	Handle find(Entity e) {
		Index::iterator it = index_.find(e);
		if (it != index_.end()) {
			return it->second;
		}
		return -1;
	}

	INT remove(Entity e) {
		Key& k_from = e;
		Key& k_to = keys.back();

		Handle h_from = index_[k_from];
		Handle h_to = index_[k_to];

		UINT& i_from = handles[h_from];
		UINT& i_to = handles[h_to];

		Component& c_from = components[i_from];
		Component& c_to = components[i_to];
		
		release_handle(h_from);

		index.erase(k_from);
		std::swap(i_from, i_to);
		std::swap(k_from, k_to); keys.pop_back();
		std::swap(c_from, c_to); components.pop_back();

		return 0;
	}

	void print() {
		print_vector("Data", data);
		print_vector("Keys", keys);
		print_vector("Handles", handles_);
		print_vector("Free Handles", free_handles_);
		std::cout << std::endl;
	}

	Keys keys;
	Components components;

private:
	Handle make_handle() {
		if (free_handles_.size()) {
			Handle h = free_handles_.back();
			free_handles_.pop_back();
			return h;
		}
		handles_.push_back(handles_.size());
		return handles_.back();
	}

	void release_handle(Handle h) {
		free_handles_.push_back(h);
	}

	Handles handles_;
	FreeHandles free_handles_;
	Index index_;
};

struct Transformation {
	Vec3 p;
	Vec3 v;
};

#define TESTING_ECS_HASH
#ifdef TESTING_ECS_HASH
typedef Table<::boost::container::vector<std::tuple<Entity, float>>> Springs;
typedef Table<Vec3> Positions;
typedef Table<Vec3> Velocities;
typedef Table<Transformation> Transformations;

void foo(UINT count, UINT iterations) {
	Transformations transformations;
	Springs springs;

	for (UINT i = 0; i < count; ++i) {
		transformations.insert(i, { Vec3{ (float)i, (float)i, 0.0 }, Vec3{ (float)i, (float)i, 0.0 } });
	}

	Entity a = rand() % count;
	Entity b = rand() % count;
	Entity c = rand() % count;

	//ASSERT(a != b != c);

	springs.insert(a, Springs::Component({ { b, 0.01f } }));
	springs.insert(a, Springs::Component({ { c, 0.02f } }));
	springs.insert(b, Springs::Component({ { a, 0.10f } }));
	springs.insert(b, Springs::Component({ { c, 0.12f } }));
	springs.insert(c, Springs::Component({ { a, 0.20f } }));
	springs.insert(c, Springs::Component({ { b, 0.21f } }));

	std::cout << "Transformations size: " << transformations.components.size() << std::endl;
	std::cout << "Springs size: " << springs.components.size() << std::endl;

	Timer frame_timer;
	Timer spring_timer;
	Timer array_timer;
	Timer t1, t2, t3, t4, t5;
//#define TIME_EXPR(timer, expr) timer.start(); expr; timer.stop()
#define TIME_EXPR(timer, expr) expr
	for (UINT i = 0; i < iterations; ++i) {
		//frame_timer.start();
		spring_timer.start();
		for (UINT j = 0; j < springs.components.size(); ++j) {
			TIME_EXPR(t1, const Entity& self = springs.keys[j]); // 9.1
			TIME_EXPR(t2, const auto& connections = springs.components[j]); // 11.88			
			for (const auto& connection : connections) {
				TIME_EXPR(t3, const Entity& other = std::get<0>(connection)); // 20.90
				TIME_EXPR(t4, const float& spring_k = std::get<1>(connection)); // 6.87
				TIME_EXPR(t5, transformations[self].v += (transformations[other].p - transformations[self].p) * spring_k); // 231.31
			}
		}
		spring_timer.stop();

		for (UINT j = 0; j < transformations.components.size(); ++j) {
			array_timer.start();
			Transformation& t = transformations.components[j];
			t.p += t.v;
			array_timer.stop();
		}
		//frame_timer.stop();
	}
	//std::cout << frame_timer.getAvgElapsed() << std::endl;
	std::cout << spring_timer.getAvgElapsed() << std::endl;
	std::cout << array_timer.getAvgElapsed() << std::endl << std::endl;
	std::cout << t1.getAvgElapsed() << std::endl;
	std::cout << t2.getAvgElapsed() << std::endl;
	std::cout << t3.getAvgElapsed() << std::endl;
	std::cout << t4.getAvgElapsed() << std::endl;
	std::cout << t5.getAvgElapsed() << std::endl;
}
#endif

#ifdef TESTING_ECS_MAP

typedef SortedTable<Key<Entity, Entity>, float> Springs;
typedef SortedTable<Key<Entity>, Vec3> Positions;
typedef SortedTable<Key<Entity>, Vec3> Velocities;
typedef SortedTable<Key<Entity>, Transformation> Transformations;

void foo(UINT count, UINT iterations) {
	Positions positions;
	Velocities velocities;
	Springs springs;
	for (UINT i = 0; i < count; ++i) {
		positions.make({ i }, Vec3{ (float)i, (float)i, 0.0 });
		velocities.make({ i }, Vec3{ 0.0, 0.0, 0.0 });
	}

	springs.make({ count - 1, count - 2 }, 0.01f);
	springs.make({ count - 1, count - 3 }, 0.02f);
	springs.make({ count - 2, count - 1 }, 0.10f);
	springs.make({ count - 2, count - 3 }, 0.12f);
	springs.make({ count - 3, count - 1 }, 0.20f);
	springs.make({ count - 3, count - 2 }, 0.21f);

	Timer spring_timer;
	Timer array_timer;
	for (UINT i = 0; i < iterations; ++i) {
		spring_timer.start();
		for (UINT i = 0; i < springs.data.size(); ++i) {
			auto& k = springs.data[i];
			auto& key = springs.keys[i];
			Entity self = std::get<0>(key);
			Entity other = std::get<1>(key);

			velocities[self] += (positions[other] - positions[self]) * k;
		}
		spring_timer.stop();

		for (UINT i = 0; i < positions.data.size(); ++i) {
			array_timer.start();
			positions.data[i] += velocities.data[i];
			array_timer.stop();
		}

	}
	std::cout << spring_timer.getAvgElapsed() << std::endl;
	std::cout << array_timer.getAvgElapsed() << std::endl;
}

void bar(UINT count, UINT iterations) {
	Transformations transformations;
	Springs springs;

	for (UINT i = 0; i < count; ++i) {
		transformations.make({ i }, { Vec3{ (float)i, (float)i, 0.0 }, Vec3{ 0.0, 0.0, 0.0 } });
	}

	springs.make({ count - 1, count - 2 }, 0.01f);
	springs.make({ count - 1, count - 3 }, 0.02f);
	springs.make({ count - 2, count - 1 }, 0.10f);
	springs.make({ count - 2, count - 3 }, 0.12f);
	springs.make({ count - 3, count - 1 }, 0.20f);
	springs.make({ count - 3, count - 2 }, 0.21f);

	Timer spring_timer;
	Timer array_timer;
	for (UINT i = 0; i < iterations; ++i) {
		spring_timer.start();
		for (UINT i = 0; i < springs.data.size(); ++i) {

			auto& k = springs.data[i];
			auto& key = springs.keys[i];
			Entity self = std::get<0>(key);
			Entity other = std::get<1>(key);

			transformations[self].v += (transformations[other].p - transformations[self].p) * k;
			
		}
		spring_timer.stop();

		for (UINT i = 0; i < transformations.data.size(); ++i) {
			array_timer.start();
			Transformation& t = transformations.data[i];
			t.p += t.v;
			array_timer.stop();
		}

	}
	std::cout << spring_timer.getAvgElapsed() << std::endl;
	std::cout << array_timer.getAvgElapsed() << std::endl;
}
#endif

}


typedef std::unordered_map<ComponentIndices, TableInterface*> Components;
typedef std::set<ComponentIndices> EntityType;
typedef std::unordered_map<Entity, EntityType> EntityTypes;

typedef ComponentTable<BulletComponent> BulletTable;
typedef ComponentTable<PhysicsComponent> PhysicsTable;
typedef ComponentTable<RenderingComponent> RenderingTable;
typedef ComponentTable<Vec2> VelocitiesTable;
typedef ComponentTable<Vec2> PositionsTable;

namespace Entities
{

Entity make(Entity* entity_index) {
	return (*entity_index)++;
}

INT instantiate(Components& components, EntityTypes& entity_types, Entity entity) {
	EntityType type = entity_types[entity];
	EntityType::iterator it = type.begin();
	while (it != type.end()) {
		components[*it]->make(entity);
		++it;
	}
	return 0;
}

INT deinstantiate(Components& components, EntityTypes& entity_types, Entity entity) {
	EntityType type = entity_types[entity];
	EntityType::iterator it = type.begin();
	while (it != type.end()) {
		components[*it]->destroy(entity);
		++it;
	}
	return 0;
}

}

namespace Queries
{

inline bool contains(Entity entity) {
	return true;
}

template<class PrimaryTable, class... Tables>
inline bool contains(Entity entity, PrimaryTable* table, Tables*... tables) {
	return table->data.find(entity) != table->data.end() && contains(entity, tables...);
}


}

namespace Systems
{

template<class... Component>
using Interface = std::function<void(Entity entity, Component&...)>;

template<class F, class PrimaryTable, class... Tables>
void run(F& f, PrimaryTable* table, Tables*... tables) {
	typename PrimaryTable::Data::iterator it = table->data.begin();
	while (it != table->data.end()) {
		if (Queries::contains(it->first, tables...)) {
			f(it->first, it->second, tables->data[it->first]...);
		}
		++it;
	}
}

template<class F, class PrimaryTable, class... Tables>
void join(F& f, PrimaryTable* table, Tables*... tables) {
	typename PrimaryTable::Data::iterator it = table->data.begin();
	while (it != table->data.end()) {
		if (Queries::contains(it->first, tables...)) {
			f(it->first, it->second, tables->data[it->first]...);
		}
		++it;
	}
}

}

typedef Systems::Interface<Vec2, Vec2> PhysicsSystem;
typedef Systems::Interface<RenderingComponent> RenderingSystem;

void game_loop() {
	Entity entity_index = 0;
	Entity bullet = Entities::make(&entity_index);
	Entity wall = Entities::make(&entity_index);

	Components components;
	components[ComponentIndices::Physics] = new PhysicsTable;
	components[ComponentIndices::Bullet] = new BulletTable;
	components[ComponentIndices::Rendering] = new RenderingTable;
	components[ComponentIndices::Positions] = new PositionsTable;
	components[ComponentIndices::Velocities] = new VelocitiesTable;

	EntityTypes entity_types;
	entity_types[bullet] = { ComponentIndices::Positions, ComponentIndices::Velocities, ComponentIndices::Bullet };
	entity_types[wall] = { ComponentIndices::Positions,  ComponentIndices::Rendering };

	PhysicsSystem physics_system = [](Entity entity, auto& p, auto& v) {
		p += v;
	};

	RenderingSystem rendering_system = [](Entity entity, auto& c) { };
	
	Entities::instantiate(components, entity_types, bullet);
	Entities::instantiate(components, entity_types, wall);

	RenderingTable* rendering_table = (RenderingTable*)components[ComponentIndices::Rendering];
	PhysicsTable* physics_table = (PhysicsTable*)components[ComponentIndices::Physics];
	PositionsTable* positions_table = (PositionsTable*)components[ComponentIndices::Positions];
	VelocitiesTable* velocities_table = (VelocitiesTable*)components[ComponentIndices::Velocities];
	while (1) {
		Systems::run(rendering_system, rendering_table);
		Systems::run(physics_system, positions_table, velocities_table);
		Systems::run([](Entity e, auto& r, auto& p) {
			std::cout << r.color.r << std::endl;
		}, rendering_table, positions_table);
	}
}


int main() {
	UINT count = 1000000;
	UINT iterations = 1000;

	/*
	UINT* list_1 = new UINT[count];
	UINT* list_2 = new UINT[count];

	for (UINT i = 0; i < count; ++i) {
		list_1[i] = rand();
		list_2[i] = rand();
	}

	Timer t;
	for (UINT iteration = 0; iteration < iterations; ++iteration) {
		t.start();
		for (UINT i = 0; i < count; ++i) {
			UINT index = rand() % count;
			list_1[index] += list_2[index];
		}
		t.stop();
	}

	std::cout << t.getAvgElapsed() / count << std::endl;
	std::cout << t.getAvgCount() / count;
	*/
	Test::foo(count, iterations);
	while (1);
	//game_loop();
}
#endif  // TESTING_NEW_GAME_LOOP

#ifdef TEST_ECS_V2

typedef Id Entity;

template<class... Keys>
using Key = std::tuple<Keys...>;

template<class... Components>
using Component = std::tuple<Components...>;

template <class Key, class Component>
class Table {
public:
	typedef Key Key;
	typedef Component Component;
	typedef std::vector<Key> Keys;
	typedef std::vector<Component> Data;

	INT insert(Key&& key, Component&& component) {
		if (keys.size() > 0) {
			UINT index = find(key);
			if (index == end()) {
				keys.insert(keys.begin() + index, key);
				data.insert(data.begin() + index, std::forward<Component>(component));
			}
		} else {
			keys.emplace_back(key);
			data.emplace_back(std::forward<Component>(component));
		}
		return 0;
	}

	Component& operator[](Key key) {
		return data[find_no_checks(key)];
	}

	const Component& operator[](Key key) const {
		return data[find_no_checks(key)];
	}

	INT remove(Key key) {
		return 0;
	}

	Data data;
	Keys keys;

private:
	UINT find(Key key) const {
		typename Keys::const_iterator it = std::lower_bound(keys.begin(), keys.end(), key);
		if (it == keys.end()) {
			return end();
		}
		return std::distance(keys.begin(), it);
	}

	UINT find_no_checks(Key key) const {
		return std::distance(keys.begin(), std::lower_bound(keys.begin(), keys.end(), key));
	}

	UINT begin() const {
		return 0;
	}

	UINT end() const {
		return keys.size();
	}
};

namespace Systems
{
template <class Component>
class Interface
{
public:

};

}

struct Transformation {
	Vec3 p;
	Vec3 v;
	Vec4 rot;
};

typedef Table<Key<Entity, Entity>, float> Springs;
typedef Table<Key<Entity>, Vec3> Positions;
typedef Table<Key<Entity>, Vec3> Velocities;
typedef Table<Key<Entity>, Transformation> Transformations;

void user_update_loop() {
	Transformations transformations;
	for (UINT i = 0; i < 100; ++i) {
		Vec3 p;
		Vec3 v;
		v.x = ((float)(rand() % 200) / 200.0) - 1.0;
		v.y = ((float)(rand() % 200) / 200.0) - 1.0;
		v.z = ((float)(rand() % 200) / 200.0) - 1.0;
		transformations.insert({ i }, { p, v });
	}

	Transformations transformations_buffers[2];
	transformations_buffers[0] = transformations;
	transformations_buffers[1] = transformations;

	Transformations* read = &transformations_buffers[0];
	Transformations* write = &transformations_buffers[1];
	while (1) {
		for (UINT i = 0; i < write->data.size(); ++i) {
			write->data[i].p += read->data[i].v;
			std::cout << write->data[i].p.x;
			std::cout << write->data[i].p.y;
		}

		std::swap(read, write);
	}
}

/*void game_loop() {
	Entity entity_index = 0;
	Entity bullet = Entities::make(&entity_index);
	Entity wall = Entities::make(&entity_index);

	Components components;
	components[ComponentIndices::Physics] = new PhysicsTable;
	components[ComponentIndices::Bullet] = new BulletTable;
	components[ComponentIndices::Rendering] = new RenderingTable;
	components[ComponentIndices::Positions] = new PositionsTable;
	components[ComponentIndices::Velocities] = new VelocitiesTable;

	EntityTypes entity_types;
	entity_types[bullet] = { ComponentIndices::Positions, ComponentIndices::Velocities, ComponentIndices::Bullet };
	entity_types[wall] = { ComponentIndices::Positions,  ComponentIndices::Rendering };

	PhysicsSystem physics_system = [](Entity entity, auto& p, auto& v) {
		p += v;
	};

	RenderingSystem rendering_system = [](Entity entity, auto& c) {};

	Entities::instantiate(components, entity_types, bullet);
	Entities::instantiate(components, entity_types, wall);

	RenderingTable* rendering_table = (RenderingTable*)components[ComponentIndices::Rendering];
	PhysicsTable* physics_table = (PhysicsTable*)components[ComponentIndices::Physics];
	PositionsTable* positions_table = (PositionsTable*)components[ComponentIndices::Positions];
	VelocitiesTable* velocities_table = (VelocitiesTable*)components[ComponentIndices::Velocities];
	while (1) {
		Systems::run(rendering_system, rendering_table);
		Systems::run(physics_system, positions_table, velocities_table);
		Systems::run([](Entity e, auto& r, auto& p) {
			std::cout << r.color.r << std::endl;
		}, rendering_table, positions_table);
	}
}*/

int main() {
	user_update_loop();
}

#endif

#ifdef TEST_ECS_V3

typedef INT64 Handle;
typedef INT64 Offset;
typedef UINT64 Entity;

template<class... Keys>
using Key = std::tuple<Keys...>;

template<class V>
void print_vector(const std::string& name, V&& vector) {
	std::cout << name << ":" << std::endl;
	std::cout << "[";
	for (UINT i = 0; i < vector.size(); ++i) {
		std::cout << vector[i] << " ";
	}
	std::cout << "]";
	std::cout << std::endl;
}

template <typename T>
struct function_traits
	: public function_traits<decltype(&T::operator())> {};
// For generic types, directly use the result of the signature of its 'operator()'

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
	// we specialize for pointers to member function
{
	enum {
		arity = sizeof...(Args)
	};
	// arity is the number of arguments.

	typedef ReturnType result_type;

	template <size_t i>
	struct arg {
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		// the i-th argument is equivalent to the i-th tuple element of a tuple
		// composed of those arguments.
	};
};

template<class ElementIn, class ElementOut, class F>
class ElementSystem {
private:
	F f_;
public:
	typedef ElementIn In;
	typedef ElementOut Out;

	ElementSystem(F f):f_(f) {}
	Out operator()(In element_in) const {
		return f_(std::forward<In>(element_in));
	}

	template<class Inner>
	ElementSystem<typename Out, typename Inner::In, std::function<typename Out(typename Inner::In)>> operator*(const Inner& inner) const {
		return ElementSystem<typename Out, typename Inner::In, std::function<typename Out(typename Inner::In)>>([=](typename Inner::In el) {
			return f_(inner(std::forward<typename Inner::In>(el)));
		});
	}
};

template<class ElementOut, class F>
class ElementSystem<void, ElementOut, F> {
private:
	F f_;
public:
	typedef void In;
	typedef ElementOut Out;

	ElementSystem(F f) :f_(f) {}
	Out operator()() const {
		return f_();
	}
};

template<class ElementIn, class F>
class ElementSystem<ElementIn, void, F> {
private:
	F f_;
public:
	typedef ElementIn In;
	typedef void Out;

	ElementSystem(F f) :f_(f) {}
	void operator()(In element_in) const {
		f_(std::forward<In>(element_in));
	}

	template<class Inner>
	ElementSystem<void, typename Inner::In, std::function<void(typename Inner::In)>> operator*(const Inner& inner) const {
		return ElementSystem<void, typename Inner::In, std::function<void(typename Inner::In)>>([=](typename Inner::In el) {
			f_(inner(std::forward<typename Inner::In>(el)));
		});
	}
};

template<class F>
class ElementSystem<void, void, F> {
private:
	F f_;
public:
	typedef void In;
	typedef void Out;

	ElementSystem(F f) :f_(f) {}
	void operator()() const {
		f_();
	}
};

template<class ElementIn, class ElementOut, class F>
ElementSystem<ElementIn, ElementOut, F> make_element_system(F f) {
	return ElementSystem<ElementIn, ElementOut, F>(f);
}

template<class F>
auto make_element_system(F f) {
	return ElementSystem<typename function_traits<decltype(f)>::arg<0>::type, typename function_traits<decltype(f)>::result_type, decltype(f)>(f);
}

class BaseTable {};

template <class Key, class Component, class Allocator = std::allocator<Component>>
class Table : public BaseTable {
public:
	typedef Key Key;
	typedef Component Component;

	typedef ::boost::container::vector<Key> Keys;
	typedef ::boost::container::vector<Component> Components;

	// For fast lookup if you have the handle to an entity.
	typedef ::boost::container::vector<UINT> Handles;
	typedef ::boost::container::vector<Handle> FreeHandles;

	// For fast lookup by Entity Id.
	typedef ::boost::container::map<Key, Handle> Index;

	Table() {}

	Table(std::vector<std::tuple<Key, Component>>&& init_data) {
		for (auto& t : init_data) {
			insert(std::move(std::get<0>(t)), std::move(std::get<1>(t)));
		}
	}

	Handle insert(Key&& key, Component&& component) {
		Handle handle = make_handle();

		index_[key] = handle;

		components.push_back(std::move(component));
		keys.push_back(key);
		return handle;
	}

	Handle insert(const Key& key, Component&& component) {
		Handle handle = make_handle();

		index_[key] = handle;

		components.push_back(std::move(component));
		keys.push_back(key);
		return handle;
	}

	Component& operator[](Handle handle) {
		return components[handles_[handle]];
	}

	const Component& operator[](Handle handle) const {
		return components[handles_[handle]];
	}

	Handle find(Key&& key) const {
		Index::const_iterator it = index_.find(key);
		if (it != index_.end()) {
			return it->second;
		}
		return -1;
	}

	Handle find(const Key& key) const {
		Index::const_iterator it = index_.find(key);
		if (it != index_.end()) {
			return it->second;
		}
		return -1;
	}

	INT remove(Handle handle) {
		Handle h_from = handle;
		UINT& i_from = handles_[h_from];
		Key& k_from = keys[i_from];
		
		Key& k_to = keys.back();
		Handle h_to = index_[k_to];
		UINT& i_to = handles_[h_to];

		Component& c_from = components[i_from];
		Component& c_to = components[i_to];

		release_handle(h_from);

		index_.erase(k_from);
		std::swap(i_from, i_to);
		std::swap(k_from, k_to); keys.pop_back();
		std::swap(c_from, c_to); components.pop_back();

		return 0;
	}

	void print() {
		print_vector("Data", data);
		print_vector("Keys", keys);
		print_vector("Handles", handles_);
		print_vector("Free Handles", free_handles_);
		std::cout << std::endl;
	}

	Keys keys;
	Components components;

private:
	Handle make_handle() {
		if (free_handles_.size()) {
			Handle h = free_handles_.back();
			free_handles_.pop_back();
			return h;
		}
		handles_.push_back(handles_.size());
		return handles_.back();
	}

	void release_handle(Handle h) {
		free_handles_.push_back(h);
	}

	Handles handles_;
	FreeHandles free_handles_;
	Index index_;
};

template <class Table>
class View {
	Table* table_;

public:
	typedef Table Table;

	static_assert(std::is_base_of<BaseTable, Table>::value, "Type is not a derived class of BaseTable.");

	View(Table* table) : table_(table) {}

	inline const typename Table::Component& operator[](Handle handle) const {
		return table_->operator[](handle);
	}

	inline Handle find(typename Table::Key&& key) const {
		return table_->find(std::move(key));
	}

	inline Handle find(const typename Table::Key& key) const {
		return table_->find(key);
	}

	inline const typename Table::Key& key(UINT index) const {
		return table_->keys[index];
	}

	inline const typename Table::Component& component(UINT index) const {
		return table_->components[index];
	}
};

struct Transformation {
	Vec3 p;
	Vec3 v;
};

typedef ::boost::container::vector<std::tuple<Handle, float>> Spring;

struct GameState {
	typedef ::boost::unordered_map<std::string, BaseTable*> Tables;

	Tables tables;
	UINT frame;
};

typedef Table<Handle, Spring> Springs;
typedef Table<Entity, Transformation> Transformations;

#if 0
void foo(UINT count, UINT iterations) {
	Transformations transformations;
	Springs springs;

	for (UINT i = 0; i < count; ++i) {
		transformations.insert(i, { Vec3{ (float)i, (float)i, 0.0 }, Vec3{ (float)i, (float)i, 0.0 } });
	}

	Entity a = rand() % count;
	Entity b = rand() % count;
	Entity c = rand() % count;

	//ASSERT(a != b != c);

	springs.insert(a, Springs::Component({ { b, 0.01f } }));
	springs.insert(a, Springs::Component({ { c, 0.02f } }));
	springs.insert(b, Springs::Component({ { a, 0.10f } }));
	springs.insert(b, Springs::Component({ { c, 0.12f } }));
	springs.insert(c, Springs::Component({ { a, 0.20f } }));
	springs.insert(c, Springs::Component({ { b, 0.21f } }));

	std::cout << "Transformations size: " << transformations.components.size() << std::endl;
	std::cout << "Springs size: " << springs.components.size() << std::endl;

	Timer frame_timer;
	Timer spring_timer;
	Timer array_timer;
	Timer t1, t2, t3, t4, t5;
	//#define TIME_EXPR(timer, expr) timer.start(); expr; timer.stop()
#define TIME_EXPR(timer, expr) expr
	for (UINT i = 0; i < iterations; ++i) {
		//frame_timer.start();
		spring_timer.start();
		for (UINT j = 0; j < springs.components.size(); ++j) {
			TIME_EXPR(t1, const Entity& self = springs.keys[j]); // 9.1
			TIME_EXPR(t2, const auto& connections = springs.components[j]); // 11.88			
			for (const auto& connection : connections) {
				TIME_EXPR(t3, const Entity& other = std::get<0>(connection)); // 20.90
				TIME_EXPR(t4, const float& spring_k = std::get<1>(connection)); // 6.87
				TIME_EXPR(t5, transformations[self].v += (transformations[other].p - transformations[self].p) * spring_k); // 231.31
			}
		}
		spring_timer.stop();

		for (UINT j = 0; j < transformations.components.size(); ++j) {
			array_timer.start();
			Transformation& t = transformations.components[j];
			t.p += t.v;
			array_timer.stop();
		}
		//frame_timer.stop();
	}
	//std::cout << frame_timer.getAvgElapsed() << std::endl;
	std::cout << spring_timer.getAvgElapsed() << std::endl;
	std::cout << array_timer.getAvgElapsed() << std::endl << std::endl;
	std::cout << t1.getAvgElapsed() << std::endl;
	std::cout << t2.getAvgElapsed() << std::endl;
	std::cout << t3.getAvgElapsed() << std::endl;
	std::cout << t4.getAvgElapsed() << std::endl;
	std::cout << t5.getAvgElapsed() << std::endl;
}
#endif

template<class... T>
using System = std::function<T...>;

typedef std::list<System<void(void)>> SystemQueue;

enum class IndexedBy {
	OFFSET = 0,
	HANDLE,
	KEY,
	ENTITY
} indexed_by;

template<class Table>
struct Pipeline {
	typedef Table Table;
	typedef System<void(std::tuple<Handle, typename Table::Component>)> Writer;
	
	template<IndexedBy index_type>
	struct Element {
		typedef void IndexType;
		typename Table::Component component;
	};

	template<>
	struct Element <IndexedBy::OFFSET> {
		typedef Offset IndexType;

		static Element<IndexedBy::OFFSET> read_from(Table* table, Offset offset) {
			return{ offset, table->components[offset] };
		}

		INT write_to(Table* table) const {
			table->components[index] = std::move(component);
			return 0;
		}

		IndexType index;
		typename Table::Component component;
	};

	template<>
	struct Element <IndexedBy::HANDLE> {
		typedef Handle IndexType;

		static Element<IndexedBy::HANDLE> read_from(Table* table, Offset offset) {
			return{ table->find(table->keys[offset]), table->components[offset] };
		}

		INT write_to(Table* table) const {
			(*table)[index] = std::move(component);
			return 0;
		}

		IndexType index;
		typename Table::Component component;
	};

	template<>
	struct Element <IndexedBy::KEY> {
		typedef typename Table::Key IndexType;

		static Element<IndexedBy::KEY> read_from(Table* table, Offset offset) {
			return{ table->keys[offset], table->components[offset] };
		}

		INT write_to(Table* table) const {
			table->components[table->find(index)] = std::move(component);
			return 0;
		}

		IndexType index;
		typename Table::Component component;
	};

	template<typename... Args>
	static Table create_table(Args... args) {
		return Table(std::forward<Args>(args)...);
	}

	static View<Table> create_view(Table* table) {
		return View<Table>(table);
	}

	static System<void(SystemQueue&&)> create_queue() {
		return [](SystemQueue&& queue) {
			for (auto& system : queue) {
				system();
			}
		};
	}

	// Create a standard reader.
	template<class Sys>
	static auto create_reader(Table* table, Sys system) {
		return make_element_system<void, void>([=]() {
#pragma omp parallel for
			for (INT64 i = 0; i < table->components.size(); ++i) {
				// Use std::decay to get the "base" type of the template type.
				// This makes sure that we can access the static class functions.
				system(std::move(std::decay<Sys::In>::type::read_from(table, i)));
			}
		});
	}

	// Create a custom reader.
	template<class Sys, class Reader>
	static auto create_reader(Table* table, Sys system, Reader reader) {
		return make_element_system<void, void>([=]() {
#pragma omp parallel for
			for (INT64 i = 0; i < table->components.size(); ++i) {
				// Use std::decay to get the "base" type of the template type.
				// This makes sure that we can access the static class functions.
				system(std::move(reader(table, i)));
			}
		});
	}

	// Create a standard writer.
	template<class Sys>
	static auto create_writer(Table* table, Sys system) {
		return make_element_system<typename Sys::In, void>([=](typename Sys::In el) {
			system(std::forward<typename Sys::In>(el)).write_to(table);
		});
	}

	// Create a custom writer.
	template<class Sys, class Writer>
	static auto create_writer(Table* table, Sys system, Writer writer) {
		return make_element_system<typename Sys::In, void>([=](typename Sys::In el) {
			writer(std::move(system(std::forward<typename Sys::In>(el))));
		});
	}
};


enum class MutateBy {
	INSERT = 0,
	REMOVE,
};

template<class Table>
struct Mutation {
	MutateBy mutate_by;
	typename Table::Key key;
	typename Table::Component component;
};

template<class Table>
class MutationQueue {
public:
	typedef Mutation<Table> Mutation;

	const UINT64 INITIAL_SIZE = 4096;

	std::function<void(Table*, Mutation&&)> default_resolver = [=](Table* table, Mutation m) {
		switch (m.mutate_by) {
			case MutateBy::INSERT:
				table->insert(std::move(m.key), std::move(m.component));
				break;
			case MutateBy::REMOVE:
				table->remove(table->find(m.key));
				break;
		}
	};

	bool push(Mutation&& m) {
		return mutations_.push(m);
	}

	MutationQueue() : mutations_(INITIAL_SIZE) { }

	UINT64 flush(Table* table) {
		return mutations_.consume_all([=](Mutation m) {
			switch (m.mutate_by) {
				case MutateBy::INSERT:
					table->insert(std::move(m.key), std::move(m.component));
					break;
				case MutateBy::REMOVE:
					table->remove(table->find(m.key));
					break;
			}
		});
	}

	template<class Resolver>
	UINT64 flush(Table* table, Resolver r = default_resolver) {
		return mutations_.consume_all([=](Mutation m) {
			r(table, std::move(m));
		});
	}

private:
	::boost::lockfree::queue<Mutation> mutations_;
};

struct ComponentType {};

typedef Table<std::string, class ObjectType> ObjectTypes;
typedef Table<std::string, class ComponentType> ComponentTypes;

class ObjectType {
public:
	typedef std::set<Handle> Components;
	Components components;

	static Handle create(ObjectTypes& object_types, ComponentTypes& component_types, const std::string& type_name, std::set<std::string>&& types) {
		ObjectType new_type;
		for (const auto& type : types) {
			Handle component_id = component_types.find(type);
			new_type.components.insert(component_id);
		}
		return object_types.insert(type_name, std::move(new_type));
	}

	static Handle compose(ObjectTypes& object_types, ComponentTypes& component_types, const std::string& type_name, const std::string& base_type_name, std::set<std::string>&& types) {
		ObjectType new_type = object_types[object_types.find(base_type_name)];
		for (const auto& type : types) {
			Handle component_id = component_types.find(type);
			new_type.components.insert(component_id);
		}
		return object_types.insert(type_name, std::move(new_type));
	}
};

Handle create_component_type(ComponentTypes& component_types, const std::string& type_name, ComponentType&& component_type) {
	return component_types.insert(type_name, std::move(component_type));
}

void instantiate(ObjectTypes& object_types, ComponentTypes& component_types, const std::string& type_name) {
	ObjectType& type = object_types.components[object_types.find(type_name)];
}

int main() {
	GameState state;

	const int MAX_HEIGHT = 640 / 8;
	const int MAX_WIDTH = 640 / 8;
	float collision_grid[MAX_WIDTH][MAX_HEIGHT];

	HANDLE hCD = CD_CreateWindow(0, 0, MAX_WIDTH, MAX_HEIGHT);
	
	typedef Pipeline<Springs> SpringsSchema;
	typedef Pipeline<Transformations> TransformationSchema;
	typedef Pipeline<ComponentTypes> ComponentTypeSchema;
	typedef Pipeline<ObjectTypes> ObjectTypeSchema;

	auto springs = SpringsSchema::create_table();
	auto transformations = TransformationSchema::create_table();
	auto component_types = ComponentTypeSchema::create_table();
	auto object_types = ObjectTypeSchema::create_table();

	MutationQueue<Transformations> transformations_queue;

	state.tables["Transformations"] = &transformations;
	state.tables["Springs"] = &springs;

	Handle transformations_id = create_component_type(component_types, "Transformations", {});
	Handle springs_id = create_component_type(component_types, "Springs", {});
	
	Handle ball_type = ObjectType::create(object_types, component_types, "ball", { "Transformations" });
	Handle grav_ball_type = ObjectType::compose(object_types, component_types, "grav_ball", "ball", { "Springs" });
	
	UINT count = 1000;

	for (UINT i = 0; i < count; ++i) {
		Vec3 p = { (float)(rand() % MAX_WIDTH), (float)(rand() % MAX_HEIGHT), 0 };
		Vec3 v;
		v.x = (((float)(rand() % 10000)) - 5000.0) / 5000.0;
		v.y = (((float)(rand() % 10000)) - 5000.0) / 5000.0;
		transformations_queue.push({ MutateBy::INSERT, i, { p, v } });
	}
	transformations_queue.flush(&transformations);

	for (UINT y = 0; y < MAX_HEIGHT; ++y) {
		for (UINT x = 0; x < MAX_WIDTH; ++x) {
		}
	}

	for (UINT i = 0; i < count; ++i) {
		for (UINT j = 0; j < count; ++j) {
			//springs.insert(i, Springs::Component({ { j, 0.001f } }));			
		}
	}

	auto view = TransformationSchema::create_view(&transformations);
	auto spring_view = SpringsSchema::create_view(&springs);

	auto spring = make_element_system([=](SpringsSchema::Element<IndexedBy::KEY>&& entry) {
		Handle handle = entry.index;
		Transformation accum = view[handle];

		for (const auto& connection : entry.component) {
			const Handle& other = std::get<0>(connection);
			const float& spring_k = std::get<1>(connection);
			Vec3 p = view[other].p - view[handle].p;
			if (p.length() < 10) {
				accum.v += p.normal() * spring_k;
			}
		}
		
		return TransformationSchema::Element<IndexedBy::HANDLE>{handle, accum};
	});

	auto spring_system = TransformationSchema::create_writer(&transformations, spring);
	



	auto move = make_element_system([&](TransformationSchema::Element<IndexedBy::OFFSET> && entry) -> auto&& {
		double l = entry.component.v.length();
		entry.component.v.normalize();
		entry.component.v *= min(l, 10.0);
		entry.component.p += entry.component.v;
		return std::move(entry);
	});

	auto bound_position = make_element_system([&](TransformationSchema::Element<IndexedBy::OFFSET>&& entry) -> auto&& {
		if (entry.component.p.x <= 0 || entry.component.p.x >= MAX_WIDTH - 1) {
			entry.component.p.x = min(max(entry.component.p.x, 0), MAX_WIDTH - 1);
			entry.component.v.x *= -0.9;
		}
		
		if (entry.component.p.y <= 0 || entry.component.p.y >= MAX_HEIGHT - 1) {
			entry.component.p.y = min(max(entry.component.p.y, 0), MAX_HEIGHT - 1);
			entry.component.v.y *= -0.9;
		}

		return std::move(entry);
	});

	auto move_system = TransformationSchema::create_writer(&transformations, bound_position * move);

	auto draw_system = make_element_system([&](TransformationSchema::Element<IndexedBy::OFFSET> el) {
		auto& c = el.component;
		CD_SetBufferChar(hCD, (int)c.p.x, (int)c.p.y, ' ', CD_BG_BLUE | CD_BG_INTENSE);
	});

	double delta_time = 1.0;

	auto physics_pipeline = TransformationSchema::create_queue();

	auto draw_pipeline = TransformationSchema::create_queue();

	auto springs_generator = SpringsSchema::create_reader(&springs, spring_system);

	auto transformation_generator = TransformationSchema::create_reader(&transformations, move_system);

	auto draw_generator = TransformationSchema::create_reader(&transformations, draw_system);

	WindowTimer fps(60);
	UINT frame = 0;
	while (1) {
		CD_Refresh(hCD);

		fps.start();
		if (frame % 5 == 0) {
			physics_pipeline({ springs_generator, transformation_generator, draw_generator });
		} else {
			physics_pipeline({ transformation_generator, draw_generator });
		}
		fps.stop();
		fps.step();

		std::ostringstream fps_stream;
		fps_stream << (int)(1e9 / fps.get_avg_elapsed_ns());
		std::string fps_str = fps_stream.str();

		while (fps_str.size() < 10) {
			fps_str = "." + fps_str;
		}

		for (UINT i = 0; i < fps_str.size(); ++i) {
			CD_SetBufferChar(hCD, i, 0, fps_str[i], CD_FG_GREEN | CD_FG_INTENSE);
		}

		CD_DrawScreen(hCD);	

		++frame;
	}
}

#if 0
GameState state;
state.components.insert((Entity)ComponentTypes::SPRINGS, &springs);
state.components.insert((Entity)ComponentTypes::TRANSFORMATIONS, &transformations);

System<INT> val(2);
std::cout << val() << std::endl;

System<INT(INT)> dbl = [](INT a) { return 2 * a; };
std::cout << dbl(2) << std::endl;

System<System<INT>, System<INT(INT)>> composed_dbl(val, dbl);
std::cout << composed_dbl() << std::endl;

System<System<INT(INT)>, System<INT(INT)>> composed_dbl_dbl(dbl, dbl);
std::cout << composed_dbl_dbl(2) << std::endl;

//Table<System> systems;
//systems.insert(SystemTypes::SPRINGS, System{});

Identity<INT> identity_system = [](auto a) {
	return a;
};
//typedef Composed<SpringSystem, TransformationSystem> PhysicsPipeline;
//PhysicsPipeline physics_pipeline = compose(spring_system, transformation_system);

//physics_pipeline(delta_time);

//physics.push_front(&transformation_system);
//physics.push_front(&spring_system);
#endif

#endif