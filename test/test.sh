#!/bin/sh
set -e

RED="\e[0;31m"
GREEN="\e[0;32m"
NORMAL="\e[0m"

if [ -z "$CXX" ]; then
  CXX="g++"
fi

LIBS="-lgtest_main -lgtest -llua"
TEST_DIR="$(dirname "$0")"
TEST_FILES="$TEST_DIR/*/*.cpp"
TEST_BUILD_DIR="$TEST_DIR/build"
TEST_EXE="$TEST_BUILD_DIR/test"
TEST_EXE_NO_EX="$TEST_BUILD_DIR/test-no-exceptions"

mkdir -p "$TEST_BUILD_DIR"
# shellcheck disable=SC2086
"$CXX" -o "$TEST_EXE" $TEST_FILES $LIBS
# shellcheck disable=SC2086
"$CXX" -o "$TEST_EXE_NO_EX" $TEST_FILES $LIBS -fno-exceptions

set +e
printf "%s ${GREEN}%s${NORMAL}\n" "Testing with exceptions" "ON"
"$TEST_EXE"
OK_EX=$?
printf "\n%s ${RED}%s${NORMAL}\n" "Testing with exceptions" "OFF"
"$TEST_EXE_NO_EX"
OK_NO_EX=$?
set -e

printf "\n"
if [ $OK_EX -eq 0 ] && [ $OK_NO_EX -eq 0 ]; then
  printf "${GREEN}%s${NORMAL}\n" "All tests passed!"
else
  if [ $OK_EX -ne 0 ]; then
    printf "${RED}%s${NORMAL}\n" "Some with-exceptions tests failed!" >&2
  fi
  if [ $OK_NO_EX -ne 0 ]; then
    printf "${RED}%s${NORMAL}\n" "Some no-exceptions tests failed!" >&2
  fi
  exit 1
fi
