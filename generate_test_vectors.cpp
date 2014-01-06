#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <zlib.h>

using std::uint8_t;
using std::uint32_t;

void gen_node_id(uint8_t rand, uint8_t* ip, int num_octets)
{
	uint8_t node_id[20]; // resulting node ID

	printf("%d.%d.%d.%d %02d -> ", ip[0], ip[1], ip[2], ip[3], rand);

	uint8_t v4_mask[] = { 0x03, 0x0f, 0x3f, 0xff };
	uint8_t v6_mask[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
	uint8_t* mask = num_octets == 4 ? v4_mask : v6_mask;

	for (int i = 0; i < num_octets; ++i)
		ip[i] &= mask[i];

	uint8_t r = rand & 0x7;

	uint32_t crc = crc32(0, nullptr, 0);
	crc = crc32(crc, ip, num_octets);
	crc = crc32(crc, &r, 1);

	// only take the top 21 bits from crc
	node_id[0] = (crc >> 24) & 0xff;
	node_id[1] = (crc >> 16) & 0xff;
	node_id[2] = ((crc >> 8) & 0xf8) | (std::rand() & 0x7);
	for (int i = 3; i < 19; ++i) node_id[i] = std::rand();
	node_id[19] = rand;

	printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n"
		, node_id[0], node_id[1], node_id[2], node_id[3], node_id[4]
		, node_id[5], node_id[6], node_id[7], node_id[8], node_id[9]
		, node_id[10], node_id[11], node_id[12], node_id[13], node_id[14]
		, node_id[15], node_id[16], node_id[17], node_id[18], node_id[19]
		);
}

int main()
{
	uint8_t ip1[] = {124, 31, 75, 21};
	uint8_t ip2[] = {21, 75, 31, 124};
	uint8_t ip3[] = {65, 23, 51, 170};
	uint8_t ip4[] = {84, 124, 73, 14};
	uint8_t ip5[] = {43, 213, 53, 83};

	gen_node_id(1, ip1, 4);
	gen_node_id(86, ip2, 4);
	gen_node_id(22, ip3, 4);
	gen_node_id(65, ip4, 4);
	gen_node_id(90, ip5, 4);
}

