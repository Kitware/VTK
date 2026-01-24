async function testNullInState() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const remoteSession = new vtkWASM.vtkStandaloneSession();
  let meshId = remoteSession.create("vtkPolyData");
  if (remoteSession.get(meshId).Points !== null) {
    throw new Error("vtkPolyData.Points should be null initially!");
  }
}

async function testInvokeWithNullInArgs() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const remoteSession = new vtkWASM.vtkStandaloneSession();
  let actorId = remoteSession.create("vtkActor");
  remoteSession.invoke(actorId, "SetMapper", [null]);
  if (remoteSession.get(actorId).Mapper !== null) {
    throw new Error("SetMapper with null argument should have set mapper to null!");
  }
}

async function testInvokeWithNullInReturn() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const remoteSession = new vtkWASM.vtkStandaloneSession();
  let actorId = remoteSession.create("vtkActor");
  let ret = remoteSession.invoke(actorId, "GetMapper", []);
  if (ret !== null) {
    throw new Error("GetMapper should have returned null!");
  }
}

const tests = [
  {
    description: "Null in state",
    test: testNullInState,
  },
  {
    description: "Invoke with null in args",
    test: testInvokeWithNullInArgs,
  },
  {
    description: "Invoke with null in return",
    test: testInvokeWithNullInReturn,
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
