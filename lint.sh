#!/usr/bin/env bash

set -e

cd "$(dirname "$0")"

cpplint --recursive --config .cpplint src include tests
