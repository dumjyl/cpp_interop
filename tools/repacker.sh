set -e
set -x
wget --quiet --no-clobber https://raw.githubusercontent.com/dumjyl/clang-tooling-nim/master/src/clang_tooling/build.nim
nim cpp -r --out_dir="\$nimcache" repacker.nim "$@"
DIR_NAME="clang-tooling-$2-$3"
zip -rq  $DIR_NAME.zip $DIR_NAME
