#!/bin/bash

#include "helpers.bash"

set -e

for i in {0..20}; do
    cleanup_sim
    flo-torture --seed "$RANDOM"
    run_sim
done

echo "Test passed"
