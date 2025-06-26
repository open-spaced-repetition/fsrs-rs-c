#!/bin/bash
cargo build
cc -o simple ./examples/simple.c -Iinclude/ -L./target/debug -lfsrs_rs_c -lm
LD_LIBRARY_PATH=./target/debug ./simple
rm simple
cc -o optimize ./examples/optimize.c -Iinclude/ -L./target/debug -lfsrs_rs_c -lm
LD_LIBRARY_PATH=./target/debug ./optimize
rm optimize
cc -o schedule ./examples/schedule.c -Iinclude/ -L./target/debug -lfsrs_rs_c -lm
LD_LIBRARY_PATH=./target/debug ./schedule
rm schedule
