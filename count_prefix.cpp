#include <openssl/sha.h>
#include <mutex>
#include <thread>
#include <zlib.h>

#include "trie.hpp"

uint32_t sha(uint32_t input)
{
	SHA_CTX ctx;
	SHA_Init(&ctx);
	SHA_Update(&ctx, &input, 4);
	uint32_t digest[5];
	SHA_Final((unsigned char*)digest, &ctx);
	return digest[0];
}

uint32_t sha_xor(uint32_t input)
{
	SHA_CTX ctx;
	SHA_Init(&ctx);
	SHA_Update(&ctx, &input, 4);
	uint32_t digest[5];
	SHA_Final((unsigned char*)digest, &ctx);
	return digest[0] ^ digest[1] ^ digest[2] ^ digest[3] ^ digest[4];
}

uint32_t crc(uint32_t input)
{
	uint32_t out = crc32(0L, Z_NULL, 0);
	return crc32(out, (const Bytef*)&input, 4);
}

uint32_t adler(uint32_t input)
{
	uint32_t out = adler32(0L, Z_NULL, 0);
	return adler32(out, (const Bytef*)&input, 4);
}

template <class F>
void for_each_input(F fun, char const* filename)
{
	// the collection of all output values, as
	// a dense set representation
	trie out_values;

	FILE* fout = fopen(filename, "w+");

	uint32_t output = fun(0);
	out_values.insert(output);

	for (int bits = 0; bits < 32; ++bits)
	{
		// the way we produce the increasing number of input "entropy"
		// is by first setting the first bit, then for every combination
		// of bit prefixes, set the next bit
		// round 1:
		// 10000000 00000000 00000000 00000000

		// round 2:
		// 01000000 00000000 00000000 00000000
		// 11000000 00000000 00000000 00000000

		// round 3:
		// 00100000 00000000 00000000 00000000
		// 01100000 00000000 00000000 00000000
		// 10100000 00000000 00000000 00000000
		// 11100000 00000000 00000000 00000000
		// and so on. For each round we have a complete
		// bit prefix of 'bits' in the input. We then count
		// the complete bit prefix of the SHA-1 digest

		const uint32_t max = 1 << bits;
		for (uint32_t i = 0; i < max; ++i)
		{
			uint32_t input = (i << (32 - bits)) | (1 << (31 - bits));
			uint32_t output = fun(input);
			out_values.insert(output);
		}

		int in_bits = bits + 1;
		int complete_bit_prefix = out_values.count_complete_prefix();
		fprintf(fout, "%d\t%d\n", in_bits, complete_bit_prefix);
		fflush(fout);
	}

	fclose(fout);
}

int main(int argc, char const* argv[])
{
	std::vector<std::thread> pool;

	// the expensive part of this program is to insert into the trie.
	// the cost of crc or SHA-1 is negligible in comparison. That's
	// why it makes most sense to parallelize per trie, rather than
	// across invocation of the hasher, even though it provides limited
	// parallelism (4 threads). The alternative would essentially cause
	// everything to serialize around inserting (which would need to be
	// mutex protected anyway).
	pool.emplace_back([](){for_each_input(sha, "sha.dat"); } );
	pool.emplace_back([](){for_each_input(sha_xor, "sha_xor.dat"); } );
	pool.emplace_back([](){for_each_input(crc, "crc.dat"); } );
	pool.emplace_back([](){for_each_input(adler, "adler.dat"); } );

	for (auto& t : pool)
		t.join();
}

