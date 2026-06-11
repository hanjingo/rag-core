# rag-core
RAG Core Component

macOS / Linux:

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DBUILD_TEST=ON -DBUILD_BENCH=ON
cmake --build build
ctest --test-dir build
```

## Grpc

linux:

```bash
vcpkg install grpc

${VCPKG_ROOT}/installed/x64-linux/tools/protobuf/protoc --cpp_out=./ ./src/api.proto

${VCPKG_ROOT}/installed/x64-linux/tools/protobuf/protoc -I=./src --grpc_out=./src --plugin=protoc-gen-grpc=${VCPKG_ROOT}/installed/x64-linux/tools/grpc/grpc_cpp_plugin ./src/api.proto
```