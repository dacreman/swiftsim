#!/bin/bash

# make run.sh fail if a subcommand fails
set -e

if [ ! -f 'randomized-sine.hdf5' ]; then
    echo "Generating ICs"
    python3 makeIC.py
fi

# Run SWIFT with RT
../../swift \
    --hydro \
    --threads=9 \
    --verbose=0  \
    --radiation \
    --self-gravity \
    --stars \
    --feedback \
    ./randomized-rt.yml 2>&1 | tee output.log

echo "running sanity checks"
python3 ../UniformBox_3D/rt_sanity_checks.py | tee sanity_check.log
echo "running checks for uniform box test"
python3 ../UniformBox_3D/rt_uniform_box_checks.py | tee box_check.log
echo "running GEAR sanity checks"
python3 ../UniformBox_3D/rt_sanity_checks-GEAR.py
echo "running GEAR checks for uniform box test"
python3 ../UniformBox_3D/rt_uniform_box_checks-GEAR.py
