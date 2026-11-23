#!/bin/bash
set -e

protoc -I . --cpp_out=. fdrpc.proto
protoc -I . --python_out=. fdrpc.proto