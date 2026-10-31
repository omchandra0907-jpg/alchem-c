#!/usr/bin/env bash
set -e
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

echo "Building release binary..."
make release

echo "Starting 10-second alchem-c chemical reaction showcase..."
./alchem-c --demo
