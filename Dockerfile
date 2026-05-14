# ---[ Stage 1: Compile C → WASM with Emscripten ]-----------------------------
FROM emscripten/emsdk:latest AS build-wasm

RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl zip unzip tar cmake ninja-build pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Bootstrap vcpkg (provides SDL2 + SDL2_mixer for the wasm32-emscripten triplet)
RUN git clone https://github.com/microsoft/vcpkg.git /vcpkg \
    && /vcpkg/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /work
COPY CMakeLists.txt vcpkg.json ./
COPY app ./app
COPY resources/sound ./resources/sound
# cmake --install writes pong.js / pong.wasm / pong.data here
RUN mkdir -p web/src

# Pass toolchain paths explicitly so the in-tree 3rdparty/ submodule stubs
# are not needed inside the Docker build context.
RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DWASM=1 \
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
        -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && cmake --build build \
    && cmake --install build

# ---[ Stage 2: Serve with nginx:alpine ]---------------------------------------
# Parcel is skipped here: newer Emscripten (emsdk:latest) emits ES-module
# syntax in pong.js, which Parcel rejects in a classic-script HTML context.
# pong.js is already minified/optimised by Emscripten, so bundling adds nothing.
FROM nginx:alpine
# One worker is plenty for a static-only site behind master nginx.
RUN sed -i 's/worker_processes  auto;/worker_processes 1;/' /etc/nginx/nginx.conf
# Static web assets (html, css, images) from source tree
COPY web/src /usr/share/nginx/html/
# WASM artifacts from the build stage (pong.js / pong.wasm / pong.data)
COPY --from=build-wasm /work/web/src/pong.js   /usr/share/nginx/html/pong.js
COPY --from=build-wasm /work/web/src/pong.wasm /usr/share/nginx/html/pong.wasm
COPY --from=build-wasm /work/web/src/pong.data /usr/share/nginx/html/pong.data
