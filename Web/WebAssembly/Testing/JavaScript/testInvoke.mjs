async function testInvoke() {
  const manager = await globalThis.createVTKWasmSceneManager({});
  manager.initialize();
  // manager.setDeserializerLogVerbosity("INFO");
  // manager.setObjectManagerLogVerbosity("INFO");
  // manager.setInvokerLogVerbosity("INFO");
  manager.registerStateJSON({
    "ClassName": "vtkCamera", "SuperClassNames": ["vtkObject"], "vtk-object-manager-kept-alive": true, "Id": 1
  });

  manager.updateObjectsFromStates();

  manager.updateStateFromObject(1);
  let state = manager.getState(1);
  if (JSON.stringify(state.Position) != JSON.stringify([0, 0, 1])) {
    throw new Error("Failed to initialize camera state");
  }

  // Invoke a method named "Elevation" on the camera with argument 10.0
  manager.invoke(1, "Elevation", [10.0]);

  manager.updateStateFromObject(1);
  state = manager.getState(1);
  if (JSON.stringify(state.Position) != JSON.stringify([0, 0.17364817766693033, 0.9848077530122081])) {
    throw new Error("vtkCamera::Elevation(10) did not work!");
  }
}

const tests = [
  {
    description: "Invoke methods",
    test: testInvoke,
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
