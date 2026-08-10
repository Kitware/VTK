async function testFloat64Arrays() {
  const vtkWASM = await globalThis.createVTKWASM({});
  const session = new vtkWASM.vtkStandaloneSession();
  const coordinates = session.create("vtkTypeFloat64Array");
  session.set(coordinates, { "NumberOfComponents": 3 });
  // 1. Create a JavaScript TypedArray
  const jsArray = new Float64Array([-1, -1, 0, 1, -1, 0]);
  // 2. Allocate sufficient bytes on WASM heap for the typed array.
  const byteLength = jsArray.byteLength;
  const bufferPtr = vtkWASM._malloc(jsArray.byteLength);
  // 3. Calculate the correct index offset for the specific typed heap view
  const heapIndex = bufferPtr / jsArray.BYTES_PER_ELEMENT;
  // 4. Copy the JS array directly into the wasm heap
  vtkWASM.HEAPF64.set(jsArray, heapIndex);
  // 5. Pass the bufferPtr to the vtk array
  session.invoke(coordinates, "SetArray", [bufferPtr, 6, 0]);
  for (let i = 0; i < 6; ++i) {
    if (jsArray[i] != session.invoke(coordinates, "GetValue", [i])) {
      throw new Error(`The ${i}'th element is incorrect.`);
    }
  }
  // 6. Verify that GetTypedTuple works
  if (JSON.stringify(session.invoke(coordinates, "GetTypedTuple", [1])) !== JSON.stringify([1, -1, 0])) {
    throw new Error("Tuple at position 1 is incorrect");
  }
}

async function testAffineFloat32Arrays() {
  const vtkWASM = await globalThis.createVTKWASM({});
  const session = new vtkWASM.vtkStandaloneSession();
  const coordinates = session.create("vtkAffineTypeFloat32Array");
  session.invoke(coordinates, "ConstructBackend", [1.0, 0.0]);
  session.set(coordinates, { "NumberOfComponents": 3 });
  console.log(session.get(coordinates))
}

async function testInt64Arrays() {
  const vtkWASM = await globalThis.createVTKWASM({});
  // The 64-bit integer heap views are only exported in a wasm64 build.
  if (vtkWASM.HEAP64 === undefined) {
    console.log("  (skipped, HEAP64 is not exported in a wasm32 build)");
    return;
  }
  const session = new vtkWASM.vtkStandaloneSession();
  const ids = session.create("vtkTypeInt64Array");
  session.set(ids, { "NumberOfComponents": 3 });
  // 1. Create a JavaScript TypedArray
  const jsArray = new BigInt64Array([-1n, -1n, 0n, 1n, -1n, 0n]);
  // 2. Allocate sufficient bytes on WASM heap for the typed array.
  const bufferPtr = vtkWASM._malloc(jsArray.byteLength);
  // 3. Calculate the correct index offset for the specific typed heap view
  const heapIndex = bufferPtr / jsArray.BYTES_PER_ELEMENT;
  // 4. Copy the JS array directly into the wasm heap
  vtkWASM.HEAP64.set(jsArray, heapIndex);
  // 5. Pass the bufferPtr to the vtk array
  session.invoke(ids, "SetArray", [bufferPtr, 6, 0]);
  for (let i = 0; i < 6; ++i) {
    // values round trip through JSON as numbers, so compare against Number(...)
    if (Number(jsArray[i]) != session.invoke(ids, "GetValue", [i])) {
      throw new Error(`The ${i}'th element is incorrect.`);
    }
  }
  // 6. Verify that GetTypedTuple works
  if (JSON.stringify(session.invoke(ids, "GetTypedTuple", [1])) !== JSON.stringify([1, -1, 0])) {
    throw new Error("Tuple at position 1 is incorrect");
  }
}

const tests = [
  {
    description: "Test typed VTK affine arrays",
    test: testAffineFloat32Arrays,
  },
  {
    description: "Test typed VTK arrays",
    test: testFloat64Arrays,
  },
  {
    description: "Test 64-bit integer VTK arrays",
    test: testInt64Arrays,
  },
];

let exitCode = 0;
for (let test of tests) {
  try {
    await test.test();
    console.log("✓", test.description);
    exitCode |= 0;
  }
  catch (error) {
    console.log("x", test.description);
    console.log(error);
    exitCode |= 1;
  }
}
process.exit(exitCode);
