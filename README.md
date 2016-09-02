CLMPI: Clock MPI
===========================

# Introduction

CLMPI is a PMPI wrapper for piggybacking Lamport-clock. The Lamport-clock follows below rules:

 * Initial logical clock is 10 (LC=10).
 * When an MPI process sends a message, it increments the clock by 1 (LC = LC + 1).
 * When an MPI process receives a message, update LC to max of its own clock and received clock, and then increment by 1 (LC = max[LC, received LC] + 1).

This package also contain PBMPI (Piggyback MPI). PBMPI is a PMPI wrapper to send piggyback messages (currently a single value) with MPI messages.

# Quick Start

## 1. Installing dependent

CLMPI has dependent a software:

 * STACKP

## 2. Get source code 

### From git repogitory

    $ git clone ssh://git@cz-stash.llnl.gov:7999/prun/rempi.git
    $ cd <rempi directory>
    $ ./autogen.sh

### From tarball

    $ tar zxvf ./clmpi_xxxxx.tar.bz
    $ cd <rempi directory>

## 3. Build ReMPI

    ./configure --prefix=<path to installation directory> --with-stack-pmpi=<path to STACKP directory>
    make 
    make install

## 3. Run examples

Assuming SLURM

     cd example
     make
     ./example.sh

# Configuration Option

For more details, run ./configure -h  

  * `--with-stack-pmpi`: path to STACKP

