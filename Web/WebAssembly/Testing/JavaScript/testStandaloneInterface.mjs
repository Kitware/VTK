async function testCreateObjects() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const standaloneSession = new vtkWASM.vtkStandaloneSession();
  const actorId = standaloneSession.create("vtkActor");
  if (actorId === 0) {
    throw new Error("Failed to create vtkActor");
  }
}

async function testGet() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const standaloneSession = new vtkWASM.vtkStandaloneSession();
  const actorId = standaloneSession.create("vtkActor");
  const actorState = standaloneSession.get(actorId);
  if (!actorState.SuperClassNames.includes("vtkActor")) {
    throw new Error(`Expected superclass contains vtkActor but got ${actorState.SuperClassNames}`);
  }
  if (actorState.Id !== actorId) {
    throw new Error(`Expected id ${actorId} but got ${actorState.Id}`);
  }
}

async function testSet() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const standaloneSession = new vtkWASM.vtkStandaloneSession();
  const actorId = standaloneSession.create("vtkActor");
  if (standaloneSession.set(actorId, { Visibility: false }) === false) {
    throw new Error("Failed to set visibility to false");
  }
  const mapperId = standaloneSession.create("vtkPolyDataMapper");
  const result = standaloneSession.set(actorId, { Mapper: {Id: mapperId} });
  if (result === false) {
    throw new Error("Failed to set mapper");
  }
  // Verify the changes
  const actorState = standaloneSession.get(actorId);
  if (actorState.Visibility != false) {
    throw new Error(`Expected visibility false but got ${actorState.Visibility}`);
  }
  if (actorState.Mapper.Id !== mapperId) {
    throw new Error(`Expected mapper Id ${mapperId} but got ${actorState.Mapper.Id}`);
  }
}

async function testDestroy() {
  // This error message is expected when `get` is called on a destroyed object.
  const skipRegex = /Cannot update state for object at id=1 because there is no such object/gm;
  let sawError = false;
  const vtkWASM = await globalThis.createVTKWASM({
    print: console.log,
    printErr: function (x) {
      if (skipRegex.exec(x)) {
        sawError = true;
        return;
      }
      else {
        console.error(x);
      }
    },
  })
  const standaloneSession = new vtkWASM.vtkStandaloneSession();
  const actorId = standaloneSession.create("vtkActor");
  if (standaloneSession.destroy(actorId) === false) {
    throw new Error("Failed to destroy vtkActor");
  }
  const actorState = standaloneSession.get(actorId);
  if (!sawError) {
    throw new Error(`Expected error message '${skipRegex}' when getting destroyed object`);
  }
  if (actorState !== null) {
    throw new Error("Expected get to return null for destroyed object");
  }
  if (standaloneSession.destroy(234) === true) {
    throw new Error("Expected destroy to return false for non-existing object");
  }
}

async function testObservers() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const standaloneSession = new vtkWASM.vtkStandaloneSession();
  const actorId = standaloneSession.create("vtkActor");
  let observedChanges = 0;
  const observerId = standaloneSession.observe(actorId, 'ModifiedEvent', (objId, propName, newValue) => {
    observedChanges++;
  });
  standaloneSession.set(actorId, { Visibility: false });
  standaloneSession.invoke(actorId, "RotateZ", [45]);
  if (observedChanges !== 2) {
    throw new Error(`Expected 2 observed changes but got ${observedChanges}`);
  }
  standaloneSession.unObserve(actorId, observerId);
  standaloneSession.set(actorId, { Visibility: true });
  if (observedChanges !== 2) {
    throw new Error(`Expected 2 observed changes after unobserve but got ${observedChanges}`);
  }
}

const tests = [
  {
    description: "Create objects",
    test: testCreateObjects,
  },
  {
    description: "Get",
    test: testGet,
  },
  {
    description: "Set",
    test: testSet,
  },
  {
    description: "Destroy",
    test: testDestroy,
  },
  {
    description: "Observe/Unobserve",
    test: testObservers,
  }
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
