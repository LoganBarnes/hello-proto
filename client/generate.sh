#!/usr/bin/env bash

# Path to this plugin, Note this must be an abolsute path on Windows (see #15)
PROTOC_GEN_TS_PATH="./node_modules/.bin/protoc-gen-ts"

# Directory to write generated code to (.js and .d.ts files)
OUT_DIR="./generated"
mkdir -p ${OUT_DIR}

PROTO_FILES="$(find protos/ -type f -name '*.proto')"
echo ${PROTO_FILES}

protoc \
    -I=./protos \
    --plugin="protoc-gen-ts=${PROTOC_GEN_TS_PATH}" \
    --js_out="import_style=commonjs,binary:${OUT_DIR}" \
    --ts_out="service=true:${OUT_DIR}" \
    ${PROTO_FILES}
