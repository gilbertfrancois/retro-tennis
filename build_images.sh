#!/usr/bin/env bash
set -euo pipefail

PLATFORM=linux/amd64

docker buildx build \
    -t gilbertfrancois/retro-tennis:latest \
    --platform=$PLATFORM \
    --push \
    .
