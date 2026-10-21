#!/bin/bash

set -e
set -x

./build_recover_postlc6.sh
./build_postlc7_continuation.sh

echo OK