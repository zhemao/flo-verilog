#!/bin/bash

#include "helpers.bash"

set -e

flo-patterns --list | while read pattern
do
    flo-patterns --list "$pattern" | while read size
    do
        cleanup_sim
        flo-patterns --show "$pattern" "$size"
        run_sim
    done
done
