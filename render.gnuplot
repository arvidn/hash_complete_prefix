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
	"crc.dat" using 1:2 title "CRC32", \
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
	"crc-dist.dat" using 1:2 title "CRC32"

set output "hash_distribution_class_C.png"
plot "crc1-dist.dat" using 1:2 title "CRC32 /24"

set output "hash_distribution_class_B.png"
plot "crc2-dist.dat" using 1:2 title "CRC32 /16"

set output "hash_distribution_class_A.png"
plot "crc3-dist.dat" using 1:2 title "CRC32 /8"

