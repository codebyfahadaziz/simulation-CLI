# CPU Scheduling Simulator (C++)
OS Lab PBL - Project 1 | IST, Department of Computing

## Features
FCFS, SJF (non-preemptive), SRTF, Priority (preemptive and non-preemptive), Round Robin (user quantum),
Multilevel Feedback Queue (bonus), per-process CT/TAT/WT/RT, averages, CPU utilization, text Gantt chart,
comparison mode with summary table + text bar graph (bonus), matplotlib graphs (bonus).

## Compile and run (Ubuntu)
    sudo apt install g++ make        # once
    make
    ./scheduler                      # menu
    ./scheduler tests/test1.csv      # preload a CSV

## Input CSV format
    pid,arrival,burst,priority
    1,0,8,2
Lower priority number = higher priority.

## Rules
Ties are broken by earlier arrival, then smaller PID. CPU idle gaps are shown as IDLE in the Gantt chart.
MLFQ: Q0 quantum 4, Q1 quantum 8, Q2 FCFS; new processes enter Q0; higher queue preempts lower.

## Bonus graphs
Run "Compare ALL" (creates results.csv and gantt.csv), then:
    pip3 install matplotlib
    python3 plot_results.py

## Test cases
tests/test1.csv (normal), tests/test2.csv (equal arrival, tied burst/priority), tests/test3.csv (idle gaps).
