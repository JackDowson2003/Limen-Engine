if [ -f "README.md" ]; then
    cd ..
fi
cmake --preset ninja-debug
cmake --build out/cmake-build-debug-clang17 --target LimenPostProcessReferenceTests
#ctest --test-dir out/cmake-build-debug-clang17 -R '^LimenPostProcessReference$' --output-on-failure
./out/cmake-build-debug-clang17/tests/LimenOpenGLPostProcessTests