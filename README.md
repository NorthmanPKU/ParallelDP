# ParallelDP

[Project Proposal](./Final_Project_Proposal.pdf)

[Milestone Report](./Milestone_Report.pdf)

[Final Report](./Final_Report.pdf)

## Introduction

We developed ParallelDP, a high-level parallel dynamic programming DSL library for multi-core CPUs. Users define DP problems through a simple DSL, while the backend automatically applies parallelization techniques such as the Cordon Algorithm. We implemented two sequential baselines (a naïve unoptimized DP and an optimized work-efficient DP) and three parallel versions using OpenMP, OpenCilk, and Parlay. We evaluated all implementations on the Pittsburgh Supercomputing Center (PSC) machines.

## Quick start

Firstly run:
```
./install.sh
```
This should handle all environment building procedure, including installation of libraries.

Then run:
```
./dsl
```
to see a demonstration of our DP DSL basic function.

You could also run
```
./build/lcs -m <seq_len> -n <pair_num> -k <lcs_len> -g <granularity;5000>
```
for a pure parallel DP problem solver display.