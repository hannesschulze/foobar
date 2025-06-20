pushd lib

rm -rf build
meson setup build
ninja -C build

cabal clean
LD_LIBRARY_PATH="build:$LD_LIBRARY_PATH" \
HASKELL_GI_GIR_SEARCH_PATH="build" \
HASKELL_GI_TYPELIB_SEARCH_PATH="build" \
cabal run buildgen build/libfoobar_static.a hs

popd

export LD_LIBRARY_PATH="$(pwd)/lib/build:$LD_LIBRARY_PATH"
export HASKELL_GI_GIR_SEARCH_PATH="$(pwd)/lib/build"
export HASKELL_GI_TYPELIB_SEARCH_PATH="$(pwd)/lib/build"
