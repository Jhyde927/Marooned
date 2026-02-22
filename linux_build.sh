#/bin/bash
mkdir -p build Marooned
rm -rf build/* Marooned/* Marooned_Linux_x86_64.tar.gz

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cp -r assets Marooned/
cp build/marooned Marooned/

# Copy Linux libraries
cp /lib/x86_64-linux-gnu/libstdc++.so.6 Marooned/
cp /lib/x86_64-linux-gnu/libm.so.6 Marooned/
cp /lib/x86_64-linux-gnu/libgcc_s.so.1 Marooned/
cp /lib/x86_64-linux-gnu/libc.so.6 Marooned/

tar -czf Marooned_Linux_x86_64.tar.gz Marooned/*
rm -rf Marooned/
