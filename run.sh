#!/bin/bash
set -euxo pipefail
cargo build

for f in ./examples/*.c; do
    cc -o ${f%.c} $f -Iinclude/ -L./target/debug -lfsrs_rs_c -lm -Wall -Wextra -Wpedantic
    LD_LIBRARY_PATH=./target/debug ./${f%.c}
    rm ${f%.c}
done
