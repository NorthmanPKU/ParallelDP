# ParallelDP

[Project Proposal](./Final_Project_Proposal.pdf)

[Milestone Report](./Milestone_Report.pdf)

[Final Report](./Final_Report.pdf)

## Quick start

Firstly run:
```
./install.sh
```
This should handle all environment building procedure, including installation of OpenCilk.

Then run:
```
./build/lcs -m <seq_len> -n <pair_num> -k <lcs_len> -g <granularity;5000>
```
