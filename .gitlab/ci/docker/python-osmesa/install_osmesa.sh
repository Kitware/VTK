#!/bin/sh

set -e

# LLVM
readonly llvm_version="15.0.6"
readonly llvm_sha256sum="9d53ad04dc60cb7b30e810faf64c5ab8157dadef46c8766f67f286238256ff92"
readonly llvm_filename="llvm-project-$llvm_version.src.tar.xz"
readonly llvm_url="https://github.com/llvm/llvm-project/releases/download/llvmorg-$llvm_version/$llvm_filename"

# Mesa
readonly mesa_version="22.3.2"
readonly mesa_sha256sum="c15df758a8795f53e57f2a228eb4593c22b16dffd9b38f83901f76cd9533140b"
readonly mesa_filename="mesa-$mesa_version.tar.xz"
readonly mesa_url="https://archive.mesa3d.org/$mesa_filename"

readonly osmesa_root="$HOME/osmesa"
readonly osmesa_prefix="/opt/osmesa"
readonly llvm_prefix="/opt/osmesa-llvm"

readonly llvm_src="$osmesa_root/llvm/src"
readonly llvm_build="$osmesa_root/llvm/build"
readonly mesa_src="$osmesa_root/mesa/src"
readonly mesa_build="$osmesa_root/mesa/build"

mkdir -p "$osmesa_root" \
    "$llvm_src" "$llvm_build" \
    "$mesa_src" "$mesa_build"

cd "$osmesa_root"

(
    echo "$llvm_sha256sum $llvm_filename"
    echo "$mesa_sha256sum $mesa_filename"
) > osmesa.sha256sum
curl -OL "$llvm_url"
curl -OL "$mesa_url"
sha256sum --check osmesa.sha256sum

tar -C "$llvm_src" --strip-components=1 -xf "$llvm_filename"
tar -C "$mesa_src" --strip-components=1 -xf "$mesa_filename"

cd "$llvm_build"

arch="$( uname -m )"
readonly arch
case "$arch" in
    i286|i386|i486|i586|i686|x86|amd64|x86_64)
        llvm_targets="X86"
        ;;
    sparc*)
        llvm_targets="Sparc"
        ;;
    powerpc|ppc64le)
        llvm_targets="PowerPC"
        ;;
    aarch64|arm64)
        llvm_targets="AArch64"
        ;;
    arm*)
        llvm_targets="ARM"
        ;;
    *)
        die "Unrecognized architecture: $arch"
        ;;
esac
readonly llvm_targets

ls "$llvm_src"

cmake -GNinja "$llvm_src/llvm" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_BUILD_LLVM_DYLIB=ON \
  "-DCMAKE_INSTALL_PREFIX=$llvm_prefix" \
  -DLLVM_ENABLE_RTTI=ON \
  -DLLVM_INSTALL_UTILS=ON \
  -DLLVM_ENABLE_LIBXML2=OFF \
  -DLLVM_ENABLE_BINDINGS=OFF \
  "-DLLVM_TARGETS_TO_BUILD=$llvm_targets"
ninja
ninja install

cd "$mesa_src"

python3 -m venv venv
venv/bin/pip install mako
. venv/bin/activate

cat >llvm.ini <<EOF
[binaries]
llvm-config = '$llvm_prefix/bin/llvm-config'
EOF

meson \
  --libdir lib \
  --buildtype=release \
  --native-file llvm.ini \
  "-Dprefix=$osmesa_prefix" \
  -Dauto_features=disabled \
  -Dgallium-drivers=swrast \
  -Dvulkan-drivers= \
  -Ddri-drivers= \
  -Dshared-glapi=enabled \
  -Degl=disabled \
  -Dllvm=enabled \
  -Dshared-llvm=enabled \
  -Dgles1=disabled \
  -Dgles2=disabled \
  -Dglx=disabled \
  -Dosmesa=true \
  -Dplatforms= \
  "$mesa_build"
ninja -C "$mesa_build"
ninja -C "$mesa_build" install

cd

rm -rf "$osmesa_root"
