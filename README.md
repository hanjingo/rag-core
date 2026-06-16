# rag-core
RAG Core Component

macOS / Linux:

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DBUILD_TEST=ON -DBUILD_BENCH=ON
cmake --build build
ctest --test-dir build
```

Windows (PowerShell):

```powershell
# from project root
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DBUILD_TEST=ON -DBUILD_BENCH=ON
cmake --build build --config Debug
cmake --build build --target tests --config Debug
```

## Grpc

linux:

```bash
vcpkg install grpc

${VCPKG_ROOT}/installed/x64-linux/tools/protobuf/protoc --cpp_out=./ ./src/api.proto

${VCPKG_ROOT}/installed/x64-linux/tools/protobuf/protoc -I=./src --grpc_out=./src --plugin=protoc-gen-grpc=${VCPKG_ROOT}/installed/x64-linux/tools/grpc/grpc_cpp_plugin ./src/api.proto
```

Windows (PowerShell):

```powershell
& "$env:VCPKG_ROOT/installed/x64-windows/tools/protobuf/protoc.exe" --cpp_out=./ ./src/api.proto

& "$env:VCPKG_ROOT/installed/x64-windows/tools/protobuf/protoc.exe" --proto_path "./src" --grpc_out "./src" --plugin "protoc-gen-grpc=$env:VCPKG_ROOT/installed/x64-windows/tools/grpc/grpc_cpp_plugin.exe" "./src/api.proto"
```

## Crash Trace

linux:

```bash
breakpad_binary_path/minidump_stackwalk your_crash_file.dmp ./symbols > decoded_stacktrace.txt
```