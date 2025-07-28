async function testInvokeRemote() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const remoteSession = new vtkWASM.vtkRemoteSession();
  remoteSession.registerState({
    "ClassName": "vtkCamera", "SuperClassNames": ["vtkObject"], "vtk-object-manager-kept-alive": true, "Id": 1
  });

  remoteSession.updateObjectsFromStates();

  remoteSession.updateStateFromObject(1);
  let state = remoteSession.getState(1);
  if (JSON.stringify(state.Position) != JSON.stringify([0, 0, 1])) {
    throw new Error("Failed to initialize camera state");
  }

  // Invoke a method named "Elevation" on the camera with argument 10.0
  remoteSession.invoke(1, "Elevation", [10.0]);

  remoteSession.updateStateFromObject(1);
  state = remoteSession.getState(1);
  if (JSON.stringify(state.Position) != JSON.stringify([0, 0.17364817766693033, 0.9848077530122081])) {
    throw new Error("vtkCamera::Elevation(10) did not work!");
  }
}
async function testInvokeStandalone() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const standaloneSession = new vtkWASM.vtkStandaloneSession();
  let cameraId = standaloneSession.create("vtkCamera");

  // Invoke a method named "Elevation" on the camera with argument 10.0
  await standaloneSession.invoke(cameraId, "Elevation", [10.0]);

  let cameraState = standaloneSession.get(cameraId);
  if (JSON.stringify(cameraState.Position) != JSON.stringify([0, 0.17364817766693033, 0.9848077530122081])) {
    throw new Error("vtkCamera::Elevation(10) did not work!");
  }

  // Create a renderer and set the camera as the active camera
  // by passing the cameraId to the SetActiveCamera method
  let rendererId = standaloneSession.create("vtkRenderer");
  await standaloneSession.invoke(rendererId, "SetActiveCamera", [{Id: cameraId}]);
  // retrieve the active camera from the renderer
  // and check that it is the same as the one we set
  let camera = await standaloneSession.invoke(rendererId, "GetActiveCamera", []);
  if (camera.Id !== cameraId) {
    throw new Error("vtkRenderer::SetActiveCamera did not work!");
  }
  // Set information on the renderer
  let informationId = standaloneSession.create("vtkInformation");
  await standaloneSession.invoke(rendererId, "SetInformation", [{Id: informationId}]);
  // Check that calling method with null argument works.
  await standaloneSession.invoke(rendererId, "SetInformation", [null]);
  // Check that method return values can be null.
  let information = await standaloneSession.invoke(rendererId, "GetInformation", []);
  if (information !== null) {
    throw new Error("vtkRenderer::SetInformation(null) did not work!");
  }
}

const tests = [
  {
    description: "Invoke methods in remote session",
    test: testInvokeRemote,
  },
  {
    description: "Invoke methods in standalone session",
    test: testInvokeStandalone,
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
