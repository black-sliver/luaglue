#!/bin/sh
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
NORMAL='\033[0m'

if [ -z "$CXX" ]; then
  CXX="g++"
fi

LIBS="-lgtest_main -lgtest -llua"
TEST_DIR="$(dirname $0)"
TEST_FILES="$TEST_DIR/*/*.cpp"
TEST_BUILD_DIR="$TEST_DIR/build"
TEST_EXE="$TEST_BUILD_DIR/test"
TEST_EXE_NO_EX="$TEST_BUILD_DIR/test-no-exceptions"

mkdir -p "$TEST_BUILD_DIR"
"$CXX" -o "$TEST_EXE" $TEST_FILES $LIBS
"$CXX" -o "$TEST_EXE_NO_EX" $TEST_FILES $LIBS -fno-exceptions

set +e
echo -e "Testing with exceptions ${GREEN}ON${NORMAL}"
"$TEST_EXE"
OK_EX=$?
echo ""
echo -e "Testing with exceptions ${RED}OFF${NORMAL}"
"$TEST_EXE_NO_EX"
OK_NO_EX=$?
set -e

echo ""
if [ $OK_EX -eq 0 ] && [ $OK_NO_EX -eq 0 ]; then
  echo -e "${GREEN}All tests passed!${NORMAL}"
else
  if [ $OK_EX -ne 0 ]; then
    echo -e "${RED}Some with-exceptions tests failed!${NORMAL}" >&2
  fi
  if [ $OK_NO_EX -ne 0 ]; then
    echo -e "${RED}Some no-exceptions tests failed!${NORMAL}" >&2
  fi
  exit 1
fi
