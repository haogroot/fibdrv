reset

set terminal png font " Times_New_Roman,12 "
set output "statistic_model1.png"
set title "Time to calculate fib(x)"
set ylabel "time(ns)"
set xlabel "nth Fibonacci number"

 plot "ans.txt" using 2 with line lw 3 title "Normal data", \
 "ans1.txt" using 2 with line lw 3 title "68% within 1 standard deviation", \
