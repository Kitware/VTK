#!/bin/sh

set -e

if [ "$( uname -m )" != "x86_64" ]; then
   exit 0
fi

readonly openusd_version="25.11"
readonly openusd_tarball="v$openusd_version.tar.gz"
readonly openusd_sha256sum="c37c633b5037a4552f61574670ecca8836229b78326bd62622f3422671188667"

readonly openusd_root="$HOME/openusd"

readonly openusd_src="$openusd_root/src"
readonly openusd_build="$openusd_root/build"

mkdir -p "$openusd_root" \
    "$openusd_src" "$openusd_build"
cd "$openusd_root"

echo "$openusd_sha256sum  $openusd_tarball" > openusd.sha256sum
curl -OL "https://github.com/PixarAnimationStudios/OpenUSD/archive/refs/tags/$openusd_tarball"
sha256sum --check openusd.sha256sum

tar -C "$openusd_src" --strip-components=1 -xf "$openusd_tarball"

# Patch to add pxr namespace to targets exported from openusd. This is needed
# to avoid a target name collision between VTK's vendored pegtl and OpenUSD's
# vendored pegtl
cd "$openusd_src/"
echo "diff --git a/cmake/macros/Private.cmake b/cmake/macros/Private.cmake
index b90b743..ce61efa 100644
--- a/cmake/macros/Private.cmake
+++ b/cmake/macros/Private.cmake
@@ -1407,6 +1407,10 @@ function(_pxr_library NAME)
         endforeach()
     endif()

+    # Add a prefix to the exported target name to avoid name clashes between
+    # PXR libraries and CMake projects that depend on them.
+    set_target_properties(\${NAME} PROPERTIES EXPORT_NAME \"pxr::\${NAME}\")
+
     if(isObject)
         # Despite not producing any install outputs, we still want to include
         # object libraries in the export set so that their properties (such as
" | patch -p1

cd "$openusd_build"

cmake -GNinja "$openusd_src/" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DPXR_BUILD_EXAMPLES:BOOL=OFF \
    -DPXR_BUILD_IMAGING:BOOL=OFF \
    -DPXR_BUILD_MONOLITHIC:BOOL=ON \
    -DPXR_BUILD_TESTS:BOOL=OFF \
    -DPXR_BUILD_TUTORIALS:BOOL=OFF \
    -DPXR_BUILD_USD_TOOLS:BOOL=OFF \
    -DPXR_ENABLE_GL_SUPPORT:BOOL=OFF \
    -DPXR_ENABLE_PRECOMPILED_HEADERS:BOOL=OFF \
    -DPXR_ENABLE_PYTHON_SUPPORT:BOOL=OFF \
    -DPXR_INSTALL_LOCATION:STRING=../lib/usd
ninja
cmake --install .

cd

rm -rf "$openusd_root"
