
declare -a useropts=("-g -pg" "-g -fprofile-arcs -ftest-coverage" "-g" "-O2" "-O3" "-Os")
declare -a useropts2=("gprof" "gcov" "-g" "-O2" "-O3" "-Os")
declare -a parallelmake=("1" "2" "4" "8")
declare -a gprofflags=("-g -pg" "-O2 -pg" "-O3 -pg")

#3.3

#for ((i=0;i<6;i++))
#do
#  echo "${useropts2[$i]}"
#  for ((n=0;n<5;n++))
#  do
#    make clean &>/dev/null
#    USER_OPTS="${useropts[$i]}" /usr/bin/time -f%U make -s
#  done
#done

#3.4
#for((i=0;i<4;i++))
#do
#  echo "Parallel ${parallelmake[$i]}"
#  for ((n=0;n<5;n++))
#  do
#    make clean &>/dev/null
#    USER_OPTS="-O3" /usr/bin/time -f%e make -s -j "${parallelmake[$i]}"
#  done
#done

#3.5
#for ((i=0;i<6;i++))
#do
#  echo "${useropts2[$i]}"
#  for ((n=0;n<5;n++))
#  do
#    make clean &>/dev/null
#    USER_OPTS="${useropts[$i]}" make -s
#    ls -l | grep 'vpr$'
#  done
#done

#3.6
#for ((i=0;i<6;i++))
#do
#  echo "${useropts2[$i]}"
#  for ((n=0;n<5;n++))
#  do
#    make clean &>/dev/null
#    USER_OPTS="${useropts[$i]}" make -s
#    /usr/bin/time -f%U vpr iir1.map4.latren.net k4-n10.xml place.out route.out -nodisp -place_only -seed 0 >/dev/null
#  done
#done

#3.7
#for ((i=0;i<3;i++))
#do
#  echo "${gprofflags[$i]}"
#  for ((n=0;n<1;n++))
#  do
#    make clean &>/dev/null
#    USER_OPTS="${gprofflags[$i]}" make -s
#    vpr iir1.map4.latren.net k4-n10.xml place.out route.out -nodisp -place_only -seed 0 >/dev/null
#    gprof vpr > output/${gprofflags[$i]}_${n}.txt
#  done
#done

#3.8
make clean &>/dev/null
USER_OPTS="-O3" make -s
/usr/bin/time -f%U vpr iir1.map4.latren.net k4-n10.xml place.out route.out -nodisp -place_only -seed 0 >/dev/null
#gcov -b vprc > output/gcov.txt

