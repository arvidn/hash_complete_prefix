set term png small size 800,500
set output "complete_bit_prefixes.png"
set grid
set xrange [1:*]
set logscale x
set ytics 1
set xtics 8
set xlabel "number of (valid) input values"
set ylabel "output complete bit prefix (bits)"
set style data lines
set key box
plot "sha.dat" using 1:2 title "SHA-1", \
	"crc32c.dat" using 1:2 title "CRC32C", \
	"adler.dat" using 1:2 title "adler32", \
	"murmur3.dat" using 1:2 title "murmur3"

set nologscale x
set output "hash_distribution.png"
set xrange [0:*]
set yrange [0:*]
set xtics 50
set ytics autofreq
set xlabel "bucket"
set ylabel "number of results"
set style data steps

plot "sha-dist.dat" using 1:2 title "SHA-1", \
	"crc32c-dist.dat" using 1:2 title "CRC32C"

set output "hash_distribution_class_C.png"
plot "crc32c1-dist.dat" using 1:2 title "CRC32C /24"

set output "hash_distribution_class_B.png"
plot "crc32c2-dist.dat" using 1:2 title "CRC32C /16"

set output "hash_distribution_class_A.png"
plot "crc32c3-dist.dat" using 1:2 title "CRC32C /8"

set output "bucket-distribution.png"
plot "crc32-distribution.dat" using 1:2 title "crc32 DHT routing table bucket distribution", \
	"sha1-distribution.dat" using 1:2 title "sha1 DHT routing table bucket distribution", \
	"adler-distribution.dat" using 1:2 title "adler DHT routing table bucket distribution", \
	"murmur-distribution.dat" using 1:2 title "murmur DHT routing table bucket distribution", \
	"murmur-ref-distribution.dat" using 1:2 title "bucket distribution of IPs"

