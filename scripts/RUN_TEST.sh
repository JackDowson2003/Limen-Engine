#!/usr/bin/env bash
set -e

if [ -f "sub_module.sh" ]; then
    cd ..
fi
cmake --preset ninja-debug -DLIMEN_ENABLE_OPENGL_GPU_TESTS=ON
cmake --build --preset build-debug --target \
    LimenPostProcessReferenceTests LimenAxisAlignedBoundingBoxTests \
    LimenAssetPathTests \
    LimenOpenGLPostProcessTests \
    LimenOpenGLRenderer2DPipelineTests
ctest --test-dir out/cmake-build-debug-clang17 \
    -R '^(LimenPostProcessReference|LimenAxisAlignedBoundingBox|LimenAssetPath|LimenOpenGLPostProcess|LimenOpenGLRenderer2DPipeline)$' \
    --output-on-failure
