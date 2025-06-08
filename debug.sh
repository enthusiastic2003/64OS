#!/bin/bash

KERNEL_ELF="./kernel/bin-x86_64/kernel"
GDB_PORT=1234

gdb "$KERNEL_ELF" -ex "target remote :$GDB_PORT"
