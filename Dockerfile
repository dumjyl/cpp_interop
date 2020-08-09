FROM alpine:3.12

RUN apk add --no-cache git build-base

RUN git clone --depth 1 https://github.com/nim-lang/Nim/ /nim
WORKDIR nim
COPY tools/nim_static_param.patch .
COPY tools/nim_untyped_resolution.patch .
RUN git apply nim_static_param.patch
RUN git apply nim_untyped_resolution.patch
RUN sh /nim/build_all.sh
ENV PATH="/nim/bin:${PATH}"

RUN apk add --no-cache lld llvm10-dev llvm10-static clang-dev clang-static zlib-static

WORKDIR /app
COPY deps deps
COPY src src
COPY nim.cfg .
RUN nim cpp -d:docker_alpine -d:lto -d:strip src/ensnare.nim
