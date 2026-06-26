async function testArrays() {
  const vtkWASM = await globalThis.createVTKWASM({});
  const session = new vtkWASM.vtkStandaloneSession();
  const coordinates = session.create("vtkTypeFloat64Array");
  session.set(coordinates, { "NumberOfComponents": 3 });
  const typedArray = new Float64Array([-1, -1, 0, 1, -1, 0]);
  session.invoke(coordinates, "SetArray", [typedArray]);
  let tuple = session.invoke(coordinates, "GetTuple3", [0]);
  console.log(tuple);
  console.log(session.get(coordinates));
}

const tests = [
  {
    description: "Test typed VTK arrays",
    test: testArrays,
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
