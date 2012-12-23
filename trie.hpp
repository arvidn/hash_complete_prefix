#include <vector>
#include <stdint.h>

enum
{
	branch_width = 4, // 4 bits per branch
	branch_factor = 1 << branch_width, // number of child nodes
	value_width = 32, // 32 bits in values
	depth = value_width / branch_width
};

struct bitmask
{
	bitmask() : bits(0) { }
	void set_bit(int b) { bits |= 1 << b; }
	bool get_bit(int b) const { return bits & (1 << b); }
	uint16_t bits;
};

bool has_prefix(bitmask const& m, int test, int num_bits)
{
	const int shift = branch_width - num_bits;
	for (int i = 0; i < (1 << shift); ++i)
	{
		if (m.get_bit((test << shift) | i)) return true;
	}
	return false;
}

bool has_complete_prefix(bitmask const& m, int num_bits)
{
	for (int i = 0; i < (1 << num_bits); ++i)
	{
		if (!has_prefix(m, i, num_bits)) return false;
	}
	return true;
}

int count_complete_mask(bitmask const& m)
{
	// fast path, if all bits are set
	if (m.bits == 1 << branch_width) return branch_width;

	for (int num_bits = branch_width - 1; num_bits > 0; --num_bits)
	{
		if (has_complete_prefix(m, num_bits)) return num_bits;
	}
	return 0;
}

struct trie
{

	trie() { m_table.push_back(trie_level()); }

	void insert(uint32_t value)
	{
		insert_impl(0, 0, value);
	}

	bool find(uint32_t value) const
	{
		return find_impl(0, 0, value);
	}

	int count_complete_prefix() const
	{
		return count_complete_impl(0, 0);
	}

private:

	struct trie_level
	{
		trie_level() : num_used(0) { memset(next_level, 0, sizeof(next_level)); }
		// these are indices into m_table
		// 0 means there are no children
		// ~0 means all branches below are set. early exit
		uint32_t next_level[1 << branch_width];
		uint32_t num_used;
	};

	int get_branch(uint32_t value, int level) const
	{
		return (value >> ( - (level + 1) * branch_width)) & ~(~0 << branch_width);
	}

	int count_complete_impl(int trie_pos, int level) const
	{
		if (level < depth - 2)
		{
			if (m_table[trie_pos].num_used == 16)
			{
				int ret = INT_MAX;
				for (int i = 0; i < branch_factor; ++i)
				{
					ret = (std::min)(ret, count_complete_impl(m_table[trie_pos].next_level[i], level + 1));
				}
				return ret + 4;
			}

			bitmask mask;
			for (int i = 0; i < branch_factor; ++i)
			{
				if (m_table[trie_pos].next_level[i] != 0) mask.set_bit(i);
			}

			return count_complete_mask(mask);
		}

		return count_complete_mask(m_bitmasks[trie_pos]);
	}

	bool find_impl(int trie_pos, int level, uint32_t value) const
	{
		// pick out the relevant nibble
		int branch = get_branch(value, level);
		
		if (level < depth - 2)
		{
			if (m_table[trie_pos].next_level[branch] == 0) return false;
			return find_impl(m_table[trie_pos].next_level[branch], level + 1, value);
		}

		// we're at a leaf. the branch now refers to a bitmask
		if (m_table[trie_pos].next_level[branch] == 0) return false;
		int mask_index = m_table[trie_pos].next_level[branch] - 1;
		return m_bitmasks[mask_index].get_bit(value & 0xf);
	}

	// level refers to which octet we are considering. When
	// we're at octet 3, we just need a bitfield
	void insert_impl(int trie_pos, int level, uint32_t value)
	{
		// pick out the relevant nibble
		int branch = get_branch(value, level);

		if (level < depth - 2)
		{
			if (m_table[trie_pos].next_level[branch] == 0)
			{
				uint32_t nb = allocate_new_branch();
				m_table[trie_pos].next_level[branch] = nb;
				++m_table[trie_pos].num_used;
			}
			insert_impl(m_table[trie_pos].next_level[branch], level + 1, value);
			return;
		}

		// we're at a leaf. the branch now refers to a bitmask
		if (m_table[trie_pos].next_level[branch] == 0)
		{
			m_table[trie_pos].next_level[branch] = allocate_leaf();
			++m_table[trie_pos].num_used;
		}
	
		// the bitmask index is always +1, to reserve the 0
		// for not-in-use
		int mask_index = m_table[trie_pos].next_level[branch] - 1;
		m_bitmasks[mask_index].set_bit(value & 0xf);
	}

	uint32_t allocate_new_branch()
	{
		m_table.push_back(trie_level());
		return m_table.size() - 1;
	}

	uint32_t allocate_leaf()
	{
		// since 0 refers to not-in-use, the bitmask indices must be +1
		// here, to reserve 0.
		m_bitmasks.push_back(bitmask());
		return m_bitmasks.size();
	}

	// this is a flat trie structure with a branch factor of 256.
	// when looking up an element, the uint32 is a 
	std::vector<trie_level> m_table;

	// the leafs instead refers to a bitmask in this vector
	std::vector<bitmask> m_bitmasks;
};

