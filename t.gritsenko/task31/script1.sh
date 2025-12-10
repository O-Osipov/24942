#!/bin/sh
yes x | head -c 3000 | ./client &
