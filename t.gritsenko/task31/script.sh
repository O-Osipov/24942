#!/bin/sh
timeout 3 yes x | tr -d '\n' | head -c 10000 | ./client &
timeout 3 yes y | tr -d '\n' | head -c 10000 | ./client &
