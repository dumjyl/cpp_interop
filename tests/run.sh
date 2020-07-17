set -e
set -u

./tools/build.sh
nim cpp --gc:arc -r -d:release --path=src tests/runner.nim $@
