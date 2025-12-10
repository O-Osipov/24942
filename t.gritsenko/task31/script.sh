#!/bin/sh

# Терминал 1
yes AAAAA | head -c 100 | ./client &

# Терминал 2
yes BBBBB | head -c 100 | ./client &
