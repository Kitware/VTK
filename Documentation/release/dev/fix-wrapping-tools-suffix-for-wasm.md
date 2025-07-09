## Fix wrapping tools suffix for wasm

The wrapping tools now use a `.commonjs` suffix, preventing runtime errors when running with Node.js projects that use `"type": "module"` in their `package.json`.
