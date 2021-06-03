#!/bin/bash

./tty -e ./hi &
sleep 2
set -x
ls --color=always -l /proc/`pidof tty`/fd
ls --color=always -l /proc/`pidof sh`/fd
