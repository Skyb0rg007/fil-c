#!/bin/bash

set -e
set -x

./build_postlc6.sh

./build_postlc7_continuation.sh
