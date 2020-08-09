set -e
set -u
nim cpp $@ src/ensnare
nim cpp -r tests/runner.nim
