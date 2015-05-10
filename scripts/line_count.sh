#!/bin/bash

echo "$(find -L src/cpp src/glsl -type f | grep -v .pch | xargs wc -l)"
