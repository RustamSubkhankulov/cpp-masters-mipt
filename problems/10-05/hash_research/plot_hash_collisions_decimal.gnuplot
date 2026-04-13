set terminal pngcairo size 1600,900
set output 'hash_collisions_decimal.png'
set datafile separator ','
set title 'Hash collision curves for decimal strings'
set xlabel 'Number of hashed strings'
set ylabel 'Collisions'
set key outside
plot \
    'hash_collisions_decimal.csv' using 1:2 with lines lw 2 title 'RSHash', \
    'hash_collisions_decimal.csv' using 1:3 with lines lw 2 title 'JSHash', \
    'hash_collisions_decimal.csv' using 1:4 with lines lw 2 title 'PJWHash', \
    'hash_collisions_decimal.csv' using 1:5 with lines lw 2 title 'ELFHash', \
    'hash_collisions_decimal.csv' using 1:6 with lines lw 2 title 'BKDRHash', \
    'hash_collisions_decimal.csv' using 1:7 with lines lw 2 title 'SDBMHash', \
    'hash_collisions_decimal.csv' using 1:8 with lines lw 2 title 'DJBHash', \
    'hash_collisions_decimal.csv' using 1:9 with lines lw 2 title 'DEKHash', \
    'hash_collisions_decimal.csv' using 1:10 with lines lw 2 title 'APHash'
