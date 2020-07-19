set -e
set -u

ensnare_hash_file="/tmp/ensnare_hash"

if [ ! -f "/tmp/ensnare_hash" ]; then
  touch /tmp/ensnare_hash
fi

if cmp --silent $ensnare_hash_file "src/ensnare/private/main.hpp"; then
  nim cpp --gc:arc --path=src --no_main:on -d:release src/ensnare.nim
else
  nim cpp --gc:arc --path=src --no_main:on -d:release -f src/ensnare.nim
  cp "src/ensnare/private/main.hpp" $ensnare_hash_file
fi
