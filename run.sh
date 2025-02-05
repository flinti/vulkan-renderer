#!/bin/bash

./build.sh && cd ./data && ../build/vulkan-renderer "$@"
