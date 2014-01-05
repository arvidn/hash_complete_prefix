set term png size 1200,700
set output "complete_bit_prefixes.png"
set xrange [1:*]
set logscale x
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
set xlabel "bucket"
set ylabel "number of results"
set style data steps

plot "sha-dist.dat" using 1:2 title "SHA-1", \
	"crc-dist.dat" using 1:2 title "CRC32"
