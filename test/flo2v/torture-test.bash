#!/bin/bash

set -e

FLO2V="$PWD/bin/flo2v"
STEP2TB="$PWD/bin/step2tb"

for i in {0..20}; do
    rm -f *.vcd *.v *.step *.flo
    rm -rf torture torture.daidir
    flo-torture --seed "$RANDOM"
    $FLO2V Torture.flo > Torture.v
    vcd2step Torture.vcd Torture.flo Torture.step
    $STEP2TB Torture.step Torture.flo > Torture_tb.v
    vcs -full64 -q -o torture -Mupdate Torture_tb.v Torture.v > /dev/null
    ./torture > /dev/null
    vcddiff --raise-b-signals=1 --b-tspc=2 Torture.vcd Torture-test.vcd
done

echo "Test passed"
