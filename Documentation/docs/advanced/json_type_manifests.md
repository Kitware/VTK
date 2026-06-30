# JSON type manifests

When VTK is configured with `VTK_WRAP_SERIALIZATION=ON` and the advanced option
`VTK_BUILD_TYPES_JSON=ON`, `vtkWrapSerDes` emits one `<ClassName>.json` type
manifest per serializable class during the same header parse that produces
`<ClassName>SerDes.cxx`. Because the manifest is derived from the same
allowability predicates the serializer and invoker use, it describes *exactly*
the properties and methods that are marshalled, there is no distinction between the
generated C++ and the manifest.

The manifests install under `share/vtk<suffix>/types` and bundle into the
WebAssembly archive (see {doc}`build_wasm_emscripten`). They are consumed by
[`@kitware/vtk-wasm`](https://www.npmjs.com/package/@kitware/vtk-wasm)'s
`generate-types.mjs` to produce TypeScript definitions for the WebAssembly object
proxy.

## Manifest schema

One `<Class>.json` is emitted per serialized class:

```json
{
  "title": "vtkContourFilter",
  "type": "object",
  "inherits": "vtkPolyDataAlgorithm",
  "properties": {
    "ComputeScalars": { "type": "boolean" },
    "Value":          { "type": "array", "items": { "type": "float64" } },
    "Mapper":         { "$ref": "vtkMapper" },
    "Progress":       { "type": "float64", "readOnly": true }
  },
  "methods": {
    "SetValue": {
      "parameters": { "index": { "type": "int32" }, "value": { "type": "float64" } },
      "returns":    { "type": "null" }
    },
    "GetScalarTree": { "parameters": {}, "returns": { "$ref": "vtkScalarTree" } }
  }
}
```

- `title`: required. This is the class name, used as the interface name verbatim.
- `type`: ignored by the generator, kept for JSON-schema validity.
- `inherits`: single parent class name; omitted for base classes.
- `properties` / `methods`: PascalCase keys. A property with a Get but no Set is
  marked `"readOnly": true`. A `vtk*` value type is emitted as `{ "$ref": "<Class>" }`.

## Numeric types

Numeric `type` values are concrete C widths: `int8`, `uint8`, `int16`, `uint16`,
`int32`, `uint32`, `int64`, `uint64`, `float32`, `float64`.

The manifest is **architecture-specific**. Word-width C types (`long`, `size_t`,
`ssize_t`, `vtkIdType`, `vtkMTimeType`) are baked to a concrete width using the
target pointer size at build time: `int32`/`uint32` on wasm32, `int64`/`uint64`
on wasm64. Each wasm build emits its own manifest set, and the consumer selects
the wasm32 or wasm64 archive that matches its target.

## Type mapping

The mapping the generator understands (`tsType` in `generate-types.mjs`):

| manifest type            | TypeScript                         |
|--------------------------|------------------------------------|
| `{"type":"boolean"}`     | `boolean`                          |
| `int8`, `uint8`..`uint32`| `number` (fixed-width integer)     |
| `int64`, `uint64`        | `bigint`                           |
| `float32`, `float64`     | `number` (float)                   |
| `string`                 | `string`                           |
| `array` + `items`        | `T[]`                              |
| `{"$ref":"vtkX"}`        | `vtkX` (falls back if not wrapped) |
| `{"type":"object"}`      | `vtkObject` (untyped fallback)     |
| `{"type":"null"}`        | `void` / `null`                    |
| anything else / missing  | `any`                              |

## Notes for the C-side emitter

Two things are easy to miss in the SerDes metadata:

- **`readOnly`**: emit it only for properties that have a Get but no Set.
- **`$ref` vs `object`**: whenever the value type is a `vtk*` pointer, emit
  `$ref` with the concrete class name; reserve bare `"type":"object"` for
  genuinely opaque/unknown types. The `$ref` is what lets `x.getScalarTree()`
  chain into a typed proxy.
