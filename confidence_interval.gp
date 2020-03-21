reset

set terminal png font " Times_New_Roman,12 "
set output "statistic_with_models.png"
set title "Time to calculate fib(x)"
set ylabel "time(ns)"
set xlabel "nth Fibonacci number"
set xtics 5
set ytics 100 

plot "ans1.txt" using 1:2 with linespoints linewidth 2 title "68% within 1 standard deviation", \
    "ans2.txt" using 1:2 with linespoints linewidth 2 title "95% within 2 standard deviation", \
    "ans3.txt" using 1:2 with linespoints linewidth 2 title "99.7% within 3 standard deviation", \
