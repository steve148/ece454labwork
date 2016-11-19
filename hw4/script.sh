echo "Make sure executable is up to date and clean"

make clean
make all

echo "Measuring randtrack with 1 thread and 50 samples to skip"
/usr/bin/time ./randtrack 1 $1 > r1.out
sort -n r1.out > r1.outs
rm r1.out

echo "Measuring randtrack with 2 threads and 50 samples to skip"
/usr/bin/time ./randtrack 2 $1 > r2.out
sort -n r2.out > r2.outs
rm r2.out

echo "Measuring randtrack with 4 threads and 50 samples to skip"
/usr/bin/time ./randtrack 4 $1 > r4.out
sort -n r4.out > r4.outs
rm r4.out

echo "Measuring randtrack_tm with 1 thread and 50 samples to skip"
/usr/bin/time ./randtrack_tm 1 $1 > r1.out
sort -n r1.out > r1.outs
rm r1.out

echo "Measuring randtrack_tm with 2 threads and 50 samples to skip"
/usr/bin/time ./randtrack_tm 2 $1 > r2.out
sort -n r2.out > r2.outs
rm r2.out

echo "Measuring randtrack_tm with 4 threads and 50 samples to skip"
/usr/bin/time ./randtrack_tm 4 $1 > r4.out
sort -n r4.out > r4.outs
rm r4.out

echo "Measuring randtrack_global_lock with 1 thread and 50 samples to skip"
/usr/bin/time ./randtrack_global_lock 1 $1 > r1.out
sort -n r1.out > r1.outs
rm r1.out

echo "Measuring randtrack_global_lock with 2 threads and 50 samples to skip"
/usr/bin/time ./randtrack_global_lock 2 $1 > r2.out
sort -n r2.out > r2.outs
rm r2.out

echo "Measuring randtrack_global_lock with 4 threads and 50 samples to skip"
/usr/bin/time ./randtrack_global_lock 4 $1 > r4.out
sort -n r4.out > r4.outs
rm r4.out

echo "Measuring randtrack_element_lock with 1 thread and 50 samples to skip"
/usr/bin/time ./randtrack_element_lock 1 $1 > r1.out
sort -n r1.out > r1.outs
rm r1.out

echo "Measuring randtrack_element_lock with 2 threads and 50 samples to skip"
/usr/bin/time ./randtrack_element_lock 2 $1 > r2.out
sort -n r2.out > r2.outs
rm r2.out

echo "Measuring randtrack_element_lock with 4 threads and 50 samples to skip"
/usr/bin/time ./randtrack_element_lock 4 $1 > r4.out
sort -n r4.out > r4.outs
rm r4.out

echo "Measuring randtrack_list_lock with 1 thread and 50 samples to skip"
/usr/bin/time ./randtrack_list_lock 1 $1 > r1.out
sort -n r1.out > r1.outs
rm r1.out

echo "Measuring randtrack_list_lock with 2 threads and 50 samples to skip"
/usr/bin/time ./randtrack_list_lock 2 $1 > r2.out
sort -n r2.out > r2.outs
rm r2.out

echo "Measuring randtrack_list_lock with 4 threads and 50 samples to skip"
/usr/bin/time ./randtrack_list_lock 4 $1 > r4.out
sort -n r4.out > r4.outs
rm r4.out
