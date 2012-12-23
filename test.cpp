#include <assert.h>

#include "trie.hpp"

int main(int argc, char const* argv[])
{

	bitmask mask;
	mask.set_bit(0x8);
	mask.set_bit(0x0);
	assert(count_complete_mask(mask) == 1);

	{
	trie t;
	t.insert(0xcdcdcdcd);
	assert(t.find(0xcdcdcdcd));
	assert(!t.find(0xcdcdcdce));
	assert(!t.find(0xcecdcdcd));
	assert(!t.find(0xcdcecdcd));
	assert(!t.find(0xcdcdcecd));

	t.insert(0xdeadbeef);
	assert(t.find(0xdeadbeef));
	assert(!t.find(0xceadbeef));
	}

	{
	trie t;

	t.insert(0x80000000);
	t.insert(0x00000000);
	assert(t.count_complete_prefix() == 1);

	t.insert(0xc0000000);
	t.insert(0x40000000);
	assert(t.count_complete_prefix() == 2);

	t.insert(0x60000000);
	t.insert(0x20000000);
	t.insert(0xa0000000);
	t.insert(0xe0000000);
	assert(t.count_complete_prefix() == 3);

	t.insert(0xf0000000);
	t.insert(0xd0000000);
	t.insert(0x10000000);
	t.insert(0x30000000);
	t.insert(0x50000000);
	t.insert(0x70000000);
	t.insert(0x90000000);
	t.insert(0xb0000000);
	assert(t.count_complete_prefix() == 4);

	t.insert(0x88000000);
	t.insert(0x08000000);
	t.insert(0xc8000000);
	t.insert(0x48000000);
	t.insert(0x68000000);
	t.insert(0x28000000);
	t.insert(0xa8000000);
	t.insert(0xe8000000);
	t.insert(0xf8000000);
	t.insert(0xd8000000);
	t.insert(0x18000000);
	t.insert(0x38000000);
	t.insert(0x58000000);
	t.insert(0x78000000);
	t.insert(0x98000000);
	t.insert(0xb8000000);
	assert(t.count_complete_prefix() == 5);
	}

}

