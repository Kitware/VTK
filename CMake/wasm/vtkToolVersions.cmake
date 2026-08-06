#[==[
Versions and hashes of the developer tools the wasm build downloads.

Everything that changes when a tool is bumped lives here, because the version
also names the cache directory the tool is unpacked into
(`<cache>/vtk/<tool>-<version>/`). Keeping the two in one place means a bump
lands in a fresh directory instead of colliding with what is already cached.

Bump a version and its hash together.
#]==]

set(vtk_emsdk_version "4.0.20")
set(vtk_emsdk_hash "9153c801da503d266541bf018d68924bda4e405da45aeea608ed30959ed75a17")

set(vtk_node_version "24.9.0")
set(vtk_node_hash_linux_x64 "d57d6c28a35785f58f33899a0aa0bfc83f7a8ef4448b6cf3f7d0961efc7b9189")
set(vtk_node_hash_win_x64 "6873514c3e6a012917cc6f95ce48a6289253370d025f1b69db290d70feebfa6e")
set(vtk_node_hash_darwin_x64 "6c9ac12d3160538d96d456dc59a8fec1479e3f8b20bfc0d61bc809eb9ec11417")
set(vtk_node_hash_darwin_arm64 "961024296c2a8e60daed0784f8b61e0fab5c51d197502a92eff052c72b53209b")
