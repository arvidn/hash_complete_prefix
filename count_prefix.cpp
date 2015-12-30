#include <openssl/sha.h>
#include <mutex>
#include <thread>
#include "crc32c.h"
#include <zlib.h>
#include <random>

#include "trie.hpp"
#include "MurmurHash3.h"

const int num_buckets = 1000;

uint32_t murmur3(uint32_t input)
{
	uint32_t ret;
	MurmurHash3_x86_32(&input, 4, 0, &ret);
	return ret;
}

uint32_t sha(uint32_t input)
{
	SHA_CTX ctx;
	SHA_Init(&ctx);
	SHA_Update(&ctx, &input, 4);
	uint32_t digest[5];
	SHA_Final((unsigned char*)digest, &ctx);
	return digest[0];
}

uint32_t crc32c(uint32_t input)
{
	return crc32c_sw(0, &input, 4);
}

uint32_t adler(uint32_t input)
{
	uint32_t out = adler32(0L, Z_NULL, 0);
	return adler32(out, (const Bytef*)&input, 4);
}

template <class F, class P>
void for_each_input(F fun, P predicate, char const* filename)
{
	// the collection of all output values, as
	// a dense set representation
	trie out_values;

	FILE* fout = fopen(filename, "w+");
	const uint64_t max = 0x100000000LL;
	uint64_t next_print = 1;
	uint64_t num_ips = 0;
	for (uint64_t i = 0; i < max; ++i)
	{
		if (!predicate(i)) continue;

		// also, only include the values where the filtered-out
		// bits are zero. All values where these bits aren't
		// zero will be masked into the versions with zeros
		// anyway, and we might as well only run the hash function
		// once, instead of for each duplicate.
		if ((i & ~0x030f3fff) != 0) continue;

		uint32_t input = htonl(i);
		for (int r = 0; r < 8; ++r)
		{
			input &= htonl(~0xe0000000);
			input |= htonl(r << 29);
			uint32_t output = fun(input);

			out_values.insert(output);

			++num_ips;
			if (num_ips == next_print) {
				int complete_bit_prefix = out_values.count_complete_prefix();
				fprintf(fout, "%lld\t%d\n", num_ips, complete_bit_prefix);
				next_print <<= 1;
			}
		}
	}

	int complete_bit_prefix = out_values.count_complete_prefix();
	fprintf(fout, "%lld\t%d\n", num_ips, complete_bit_prefix);

	fclose(fout);
}

bool mask(uint32_t in)
{
	// don't include IP addresses that are invalid on
	// the internet
	if ((in & 0xff000000) == 0x0a000000 // 10.x.x.x
		|| (in & 0xfff00000) == 0xac100000 // 172.16.x.x
		|| (in & 0xffff0000) == 0xc0a80000 // 192.168.x.x
		|| (in & 0xffff0000) == 0xa9fe0000 // 169.254.x.x
		|| (in & 0xff000000) == 0x7f000000 // 127.x.x.x
		|| (in & 0xff000000) == 0 // reserved for special use
		)
		return false;

	uint8_t first_octet = in >> 24;

	// reserved for multicast and future use
	if (first_octet >= 224)
		return false;

	// some /8 blocks don't appear to be in use. Filter those
	// out to get a more realistic set of IPs we would see
	// in the wild.

	if (first_octet == 3
		|| first_octet == 6
		|| first_octet == 7
		|| first_octet == 9
		|| first_octet == 11
		|| first_octet == 19
		|| first_octet == 21
		|| first_octet == 22
		|| first_octet == 25
		|| first_octet == 26
		|| first_octet == 28
		|| first_octet == 29
		|| first_octet == 30
		|| first_octet == 33
		|| first_octet == 48
		|| first_octet == 51
		|| first_octet == 52
		|| first_octet == 56
		|| first_octet == 102
		|| first_octet == 104)
		return false;

	return true;
}

template <class F, class P>
void chi_square_distribution(F fun, P predicate, uint64_t* buckets
	, uint64_t start, uint64_t end)
{
	const uint64_t max = 0x100000000LL;
	for (uint64_t i = start; i < end; ++i)
	{
		if (!predicate(i)) continue;

		uint32_t input = htonl(i & 0x030f3fff);

		for (int r = 0; r < 8; ++r)
		{
			input &= htonl(~0xe0000000);
			input |= htonl(r << 29);
			uint64_t output = fun(input);
			++buckets[output * num_buckets / 0x100000000LL];
		}
	}

}

template <class F>
uint32_t generate_id(F fun, uint32_t ip, uint32_t r)
{
	const uint32_t mask = 0x030f3fff;
	int const num_octets = 4;

	ip &= mask;
	ip |= (r & 0x7) << 29;

	return fun(ip) & 0xfffff800;
}

int clz(uint32_t x)
{
	for (int i = 0; i < 32; ++i, x <<= 1)
	{
		if (x & 0x80000000) return i;
	}
	return 32;
}

template <class F>
void bucket_distribution(F fun
	, uint64_t* buckets
	, uint64_t* ref_buckets)
{
	int const num_samples = 10000;

	std::random_device dev;
	std::mt19937 engine(dev());
	std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);

	uint32_t ref_ip = dist(engine);
	uint32_t ref_id = generate_id(fun, ref_ip, dist(engine));

	for (int i = 0; i < num_samples; ++i)
	{
		uint32_t const ip = dist(engine);
		uint32_t const nid = generate_id(fun, ip, dist(engine));
#ifdef __GNUC__
		int const bucket = __builtin_clz(nid ^ ref_id);
		int const ref_bucket = __builtin_clz(ip ^ ref_ip);
#else
		int const bucket = clz(nid ^ ref_id);
		int const ref_bucket = clz(ip ^ ref_ip);
#endif
		++buckets[bucket];
		++ref_buckets[ref_bucket];
	}
}

void print_results(uint64_t* buckets1, uint64_t* buckets2
	, uint64_t* buckets3, uint64_t* buckets4, char const* filename
	, int num_slots = num_buckets)
{
	uint64_t num_samples = 0;

	FILE* fout = fopen(filename, "w+");

	for (int i = 0; i < num_slots; ++i)
	{
		if (buckets2)
			buckets1[i] += buckets2[i];
		if (buckets3)
			buckets1[i] += buckets3[i];
		if (buckets4)
			buckets1[i] += buckets4[i];
		num_samples += buckets1[i];
		fprintf(fout, "%d\t%lld\n", i, buckets1[i]);
	}

	double chi_square = 0.;
	const double expected = double(num_samples) / double(num_slots);

	for (int i = 0; i < num_slots; ++i)
	{
		double diff = double(buckets1[i]) - expected;
		chi_square += diff * diff;
	}

	printf("%s-chi-square: %f num-samples: %lld expected: %f\n"
		, filename, chi_square, num_samples, expected);

	fclose(fout);
}

int main(int argc, char const* argv[])
{
	crc32c_init_sw();

	std::vector<std::thread> pool;

	// the expensive part of this program is to insert into the trie.
	// the cost of crc32c or SHA-1 is negligible in comparison. That's
	// why it makes most sense to parallelize per trie, rather than
	// across invocation of the hasher, even though it provides limited
	// parallelism (4 threads). The alternative would essentially cause
	// everything to serialize around inserting (which would need to be
	// mutex protected anyway).

	pool.emplace_back([](){for_each_input(sha, mask, "sha.dat"); } );
	pool.emplace_back([](){for_each_input(crc32c, mask, "crc32c.dat"); } );
	pool.emplace_back([](){for_each_input(adler, mask, "adler.dat"); } );
	pool.emplace_back([](){for_each_input(murmur3, mask, "murmur3.dat"); } );

	uint64_t buckets5[num_buckets];
	uint64_t buckets6[num_buckets];
	uint64_t buckets7[num_buckets];
	uint64_t buckets8[num_buckets];
	memset(buckets5, 0, sizeof(buckets5));
	memset(buckets6, 0, sizeof(buckets6));
	memset(buckets7, 0, sizeof(buckets7));
	memset(buckets8, 0, sizeof(buckets8));

	pool.emplace_back([&](){chi_square_distribution(sha, mask, buckets5, 0, 0x40000000); } );
	pool.emplace_back([&](){chi_square_distribution(sha, mask, buckets6, 0x40000000, 0x80000000); } );
	pool.emplace_back([&](){chi_square_distribution(sha, mask, buckets7, 0x80000000, 0xC0000000); } );
	pool.emplace_back([&](){chi_square_distribution(sha, mask, buckets8, 0xC0000000, 0x100000000); } );

	uint64_t buckets1[num_buckets];
	uint64_t buckets2[num_buckets];
	uint64_t buckets3[num_buckets];
	uint64_t buckets4[num_buckets];
	memset(buckets1, 0, sizeof(buckets1));
	memset(buckets2, 0, sizeof(buckets2));
	memset(buckets3, 0, sizeof(buckets3));
	memset(buckets4, 0, sizeof(buckets4));

	pool.emplace_back([&](){chi_square_distribution(crc32c, mask, buckets1, 0, 0x40000000); } );
	pool.emplace_back([&](){chi_square_distribution(crc32c, mask, buckets2, 0x40000000, 0x80000000); } );
	pool.emplace_back([&](){chi_square_distribution(crc32c, mask, buckets3, 0x80000000, 0xC0000000); } );
	pool.emplace_back([&](){chi_square_distribution(crc32c, mask, buckets4, 0xC0000000, 0x100000000); } );

	uint64_t dist_sha[32] = {0};
	uint64_t dist_crc32[32] = {0};
	uint64_t dist_adler[32] = {0};
	uint64_t dist_murmur[32] = {0};
	uint64_t dist_sha_ref[32] = {0};
	uint64_t dist_crc32_ref[32] = {0};
	uint64_t dist_adler_ref[32] = {0};
	uint64_t dist_murmur_ref[32] = {0};

	pool.emplace_back([&](){bucket_distribution(sha, dist_sha, dist_sha_ref); } );
	pool.emplace_back([&](){bucket_distribution(crc32c, dist_crc32, dist_crc32_ref); } );
	pool.emplace_back([&](){bucket_distribution(adler, dist_adler, dist_adler_ref); } );
	pool.emplace_back([&](){bucket_distribution(murmur3, dist_murmur, dist_murmur_ref); } );

	uint64_t buckets9[num_buckets];
	memset(buckets9, 0, sizeof(buckets9));
	chi_square_distribution(crc32c, mask, buckets9, 0x35353500, 0x35353600);
	print_results(buckets9, nullptr, nullptr, nullptr, "crc32c1-dist.dat");
	chi_square_distribution(crc32c, mask, buckets9, 0x35353600, 0x35360000);
	print_results(buckets9, nullptr, nullptr, nullptr, "crc32c2-dist.dat");
	chi_square_distribution(crc32c, mask, buckets9, 0x35360000, 0x36000000);
	print_results(buckets9, nullptr, nullptr, nullptr, "crc32c3-dist.dat");

	for (auto& t : pool)
		t.join();

	print_results(buckets1, buckets2, buckets3, buckets4, "crc32c-dist.dat");
	print_results(buckets5, buckets6, buckets7, buckets8, "sha-dist.dat");

	print_results(dist_sha, nullptr, nullptr, nullptr, "sha1-distribution.dat", 32);
	print_results(dist_crc32, nullptr, nullptr, nullptr, "crc32-distribution.dat", 32);
	print_results(dist_adler, nullptr, nullptr, nullptr, "adler-distribution.dat", 32);
	print_results(dist_murmur, nullptr, nullptr, nullptr, "murmur-distribution.dat", 32);

	print_results(dist_sha_ref, nullptr, nullptr, nullptr, "sha1-ref-distribution.dat", 32);
	print_results(dist_crc32_ref, nullptr, nullptr, nullptr, "crc32-ref-distribution.dat", 32);
	print_results(dist_adler_ref, nullptr, nullptr, nullptr, "adler-ref-distribution.dat", 32);
	print_results(dist_murmur_ref, nullptr, nullptr, nullptr, "murmur-ref-distribution.dat", 32);

}

