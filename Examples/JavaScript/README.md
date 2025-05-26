1. Build VTK for WebAssembly along with wrappers using the `-DVTK_ENABLE_WRAPPING=ON` flag.

2. Copy `*.html` and `*.js` files into your bin folder alongside the `vtkweb.js` and `vtkweb.wasm` files.

3. Run `python3 -m http.server` from the `bin/` folder.
