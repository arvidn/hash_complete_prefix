set term png size 1200,700
set output "complete_bit_prefixes.png"
set xrange [0:*]
set xlabel "input complete bit prefix (bits)"
set ylabel "output complete bit prefix (bits)"
set style data lines
set key box
plot "sha.dat" using 1:2 title "SHA-1", \
	"sha_xor.dat" using 1:2 title "SHA-1 XOR", \
	"crc.dat" using 1:2 title "CRC32", \
	"adler.dat" using 1:2 title "adler32"

