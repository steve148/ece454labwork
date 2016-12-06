make clean
make
for run in {1..3}
do 
    /usr/bin/time -f "%e real" ./gol 10000 inputs/1k.pbm outputs/1k.pbm
    diff outputs/1k.pbm outputs/1k_verify_out.pbm
done
