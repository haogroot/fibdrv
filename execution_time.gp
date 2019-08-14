reset

set terminal png font " Times_New_Roman,12 "
set output "statistic.png"
set title "Time to calculate fib(x)"
set ylabel "time(ns)"
set xlabel "nth fibnocci number"

plot "client_output.txt" using 2 with line lw 3 title "user space", \
'' using 3 with line lw 3 title "kernel space"
