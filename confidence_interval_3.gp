reset

set terminal png font " Times_New_Roman,12 "
set output "statistic_model3.png"
set title "Time to calculate fib(x)"
set ylabel "time(ns)"
set xlabel "nth Fibonacci number"

 plot "ans.txt" using 2 with line lw 3 title "Normal data", \
 "ans3.txt" using 2 with line lw 3 title "99.7% within 3 standard deviation", \
