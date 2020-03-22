reset

set terminal png font " Times_New_Roman,12 "
set output "statistic_model2.png"
set title "Time to calculate fib(x)"
set ylabel "time(ns)"
set xlabel "nth Fibonacci number"

 plot "ans.txt" using 2 with line lw 3 title "Normal data", \
 "ans2.txt" using 2 with line lw 3 title "95% within 2 standard deviation", \
