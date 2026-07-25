#!/bin/sh
# Compile et exécute les tests unitaires natifs de pitch_math.h.
set -e
cd "$(dirname "$0")/.."
g++ -std=c++11 -Wall -Wextra -o /tmp/test_pitch_math tests/test_pitch_math.cpp
/tmp/test_pitch_math
