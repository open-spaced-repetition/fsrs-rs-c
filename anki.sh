#!/bin/bash
set -euxo pipefail
cc -o anki anki.c -L./target/debug -Iinclude/ -lfsrs_rs_c -lm -Wall -Wextra -Wpedantic
LD_LIBRARY_PATH=./target/debug ./anki
