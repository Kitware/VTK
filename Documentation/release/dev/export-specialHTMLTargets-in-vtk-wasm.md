# Use HTML canvas in VTK.wasm even if the canvas does not have an id.

You can now interface a HTML `<canvas>` element with VTK even if it does not have an id.

```js
let canvasForVTK = document.createElement("canvas");
const wasmModule = await globalThis.createVTKWASM({});

// create mapping for your canvas.
wasmModule.specialHTMLTargets["!canvas_for_vtk_1"] = canvasForVTK;
// you can create more mappings.

// use the key from the specialHTMLTargets
renderWindow.SetCanvasSelector("!canvas_for_vtk_1");

// renderWindow will now draw into `canvasForVTK`
```
