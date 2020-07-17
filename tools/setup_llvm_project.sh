set -x
curl -O http://releases.llvm.org/9.0.0/llvm-9.0.0.src.tar.xz
curl -O http://releases.llvm.org/9.0.0/cfe-9.0.0.src.tar.xz
tar -xf llvm-9.0.0.src.tar.xz
tar -xf cfe-9.0.0.src.tar.xz
mkdir llvm-project
mv llvm-9.0.0.src llvm-project/llvm
mv cfe-9.0.0.src llvm-project/clang
