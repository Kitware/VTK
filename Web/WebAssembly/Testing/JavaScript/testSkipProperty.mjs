async function testSkipProperty() {
    const vtkWASM = await globalThis.createVTKWASM({})
    const remoteSession = new vtkWASM.vtkRemoteSession();
    remoteSession.registerState({
        "ClassName": "vtkCamera", "SuperClassNames": ["vtkObject"], "vtk-object-manager-kept-alive": true, "Id": 1
    });

    remoteSession.updateObjectsFromStates();

    // Skip Position and update object.
    remoteSession.skipProperty("vtkOpenGLCamera", "Position");
    remoteSession.updateObjectFromState({
        "Id": 1,
        "Position": [0, 1, 2]
    });

    // Verify property was skipped
    let state = remoteSession.get(1);
    if (JSON.stringify(state.Position) == JSON.stringify([0, 1, 2])) {
        throw new Error("vtkCamera::Position did not get skipped!");
    }

    // UnSkip Position and update object.
    remoteSession.unSkipProperty("vtkOpenGLCamera", "Position");
    remoteSession.updateObjectFromState({
        "Id": 1,
        "Position": [3, 4, 5]
    });

    // Verify property was deserialized
    state = remoteSession.get(1);
    if (JSON.stringify(state.Position) != JSON.stringify([3, 4, 5])) {
        throw new Error("vtkCamera::Position did not get unskipped!");
    }
}

const tests = [
    {
        description: "Invoke methods",
        test: testSkipProperty,
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
