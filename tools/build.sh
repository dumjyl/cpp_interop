set -e
set -u

ensnare_hash_file="/tmp/ensnare_hash"

if [ ! -f "/tmp/ensnare_hash" ]; then
  touch /tmp/ensnare_hash
fi

nim cpp --gc:arc --path=src --no_main:on $@ src/ensnare.nim
