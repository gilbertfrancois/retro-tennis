# Retro Tennis

_Gilbert François Duivesteijn_

## Abstract

_Retro Tennis, (also known as Pong) is a table tennis–themed twitch arcade sports video game, featuring simple two-dimensional graphics, manufactured by Atari and originally released in 1972._ (Source: Wikipedia)

This program runs as a native desktop application or in the browser as a WebAssembly application.

| ![Desktop](./resources/images/native.png) | ![Web](./resources/images/browser.png) |
| ----------------------------------------- | -------------------------------------- |
| Native application                        | WebAssembly in the browser             |


## Live demo

[https://pong.blitzblit.com](https://pong.blitzblit.com)

## Keymaps

| key     | description      | key  | description   |
| ------- | ---------------- | ---- | ------------- |
| [w]     | player 1 up      | [u]  | player 2 up   |
| [s]     | player 1 down    | [j]  | player 2 down |
| [mouse] | player 1 up/down |      |               |
|         |                  |      |               |
| [1]     | 1 player         | [0]  | demo mode     |
| [2]     | 2 players        | [r]  | restart       |

| key         | description (desktop version only) |
| ----------- | ---------------------------------- |
| [f]         | Toggle full screen                 |
| [q] / [esc] | Quit                               |



## Building the native C application

Compiling the project should be straight forward, thanks to vcpkg. After compiling and installing, the program `pong` is located in the `<project_folder>/dist`.

```sh
git clone https://github.com/gilbertfrancois/retro-tennis.git

cd retro-tennis

# Important!
git submodule update --init --recursive

# Build the project and its dependencies
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
```

Test and run with:

```sh
cd dist
./pong
```



## Local development — WebAssembly in the browser

Build the C code with Emscripten; the WASM artifacts are installed directly
into `web/src/` so you can serve them straight away with Python.

```sh
git clone https://github.com/gilbertfrancois/retro-tennis.git

cd retro-tennis

# Important!
git submodule update --init --recursive

# Install emscripten (only first time)
./3rdparty/emsdk/emsdk install latest

# Activate the build toolchain
./3rdparty/emsdk/emsdk activate latest
source 3rdparty/emsdk/emsdk_env.sh

# Build — outputs pong.js / pong.wasm / pong.data into web/src/
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DWASM=1
cmake --build build
cmake --install build

# Serve
cd web/src
python -m http.server 8000
```

Open a browser and go to http://localhost:8000



## Build & publish the production Docker image

The Dockerfile builds the WASM binary with Emscripten (clones vcpkg at build
time — no local toolchain needed) and packages everything into an
`nginx:alpine` image.

```sh
./build_images.sh
```

This builds for `linux/amd64` and pushes to Docker Hub as
`gilbertfrancois/retro-tennis:latest`.

To test the image locally before pushing:

```sh
docker buildx build -t gilbertfrancois/retro-tennis:latest --load .
docker run --rm -p 8000:80 gilbertfrancois/retro-tennis:latest
```

Open a browser and go to http://localhost:8000



## Deploy on the server

`blitzblit-master` must already be deployed and the `blitzblit-net` docker
network must exist before running this playbook.

```sh
cd ansible
ansible-playbook playbook.yml
```

The playbook pulls the latest image from Docker Hub and (re)starts the
container. Master nginx in `blitzblit-master` routes
`https://pong.blitzblit.com` → `http://retro-tennis-frontend:80` over the
shared `blitzblit-net` network automatically.

**First-time only** — issue the Let's Encrypt certificate on the server:

```sh
# In blitzblit-master, after running its own ansible playbook:
cd ~/sites/blitzblit-master
source ./env
./init_letsencrypt.sh
```



## File layout

```
retro-tennis/
├── Dockerfile                  emsdk (WASM build) → nginx:alpine
├── docker-compose.yml          one service, joins blitzblit-net
├── build_images.sh             docker buildx build --push (linux/amd64)
├── env                         no build-time vars needed
├── start.sh, stop.sh
├── app/src/                    C source (pong.c, digit.c, sound.c)
├── resources/                  fonts, images, sound assets
├── web/src/                    static web assets (html, css, keymaps)
│                               pong.js / pong.wasm / pong.data land here
│                               after cmake --install (git-ignored)
├── 3rdparty/emsdk/             Emscripten SDK (git submodule)
├── 3rdparty/vcpkg/             vcpkg (git submodule)
└── ansible/                    deploy playbook
```



## Notes on compiling native C & SDL2 programs to WASM

- The usual main loop in C, a `while(true) {...}` loop cannot be used in WASM. You have to use the browser's `requestAnimationFrame` mechanism to prevent blocking all resources of the browser. Look for the `emscripten_set_main_loop_arg` and `emscripten_set_main_loop_timing` functions in the Emscripten documentation. [[source: Emscripten API]](https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop_arg)

- Make sure that the SDL2 Audio / Mixer is loaded *after* the user clicked on the canvas. Modern web browsers will not permit web pages to produce sound before the user has interacted with them. SDL-based apps also have to deal with this problem. If the user hasn't interacted with the page, SDL_OpenAudioDevice will fail and the WASM app crashes. Retro-Tennis solves it by having an intro screen with a bouncing ball. Only when the user has clicked on the canvas to start the game, the audio is initialised and plays sound successfully. [[source: SDL Wiki]](https://wiki.libsdl.org/SDL2/README/emscripten)

- The audio files need to be preloaded and packed as `target_filename.data` file, by adding `--preload-file [filename]`. If you omit this step, you won't get enough memory assigned by the `target_filename.js` WASM loader and your WASM application will crash due to memory errors.

- The `-s ENVIRONMENT='web'` flag in `CMakeLists.txt` restricts Emscripten's output to browser-only code, removing Node.js-specific runtime paths (e.g. `require('fs')`). This is required when serving the `.js` file directly from a web server without a bundler. [[source: stackoverflow]](https://stackoverflow.com/questions/59487224/webpack-throws-error-with-emscripten-cant-resolve-fs)
