## Improve JPEG performances using SIMD extensions

The libjpeg-turbo thirdparty library can be compiled with SIMD extensions for better performances by enabling the `VTK_JPEG_ENABLE_SIMD` option.
On x84(-64) targets, NASM must be installed (or set its path by setting the `CMAKE_ASM_NASM_COMPILER` variable).
Arm64 targets don't need additionnal dependencies. This option is not compatible with WASM builds.
