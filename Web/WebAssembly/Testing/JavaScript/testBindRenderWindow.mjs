async function testBindRenderWindow() {
    const vtkWASM = await globalThis.createVTKWASM({});
    const remoteSession = new vtkWASM.vtkRemoteSession();
    remoteSession.registerState(
        {
            Id: 1,
            ClassName: "vtkCocoaRenderWindow",
            SuperClassNames: ["vtkRenderWindow"],
            Interactor: { Id: 2 },
            "vtk-object-manager-kept-alive": true,
        });
    remoteSession.registerState(
        {
            Id: 2,
            ClassName: "vtkCocoaRenderWindowInteractor",
            SuperClassNames: ["vtkRenderWindowInteractor"],
            RenderWindow: { Id: 1 },
        });
    remoteSession.updateObjectsFromStates();

    remoteSession.bindRenderWindow(1, "#my-canvas-id");

    if (remoteSession.get(1).CanvasSelector !== "#my-canvas-id") {
        throw new Error("CanvasSelector was not set correctly on RenderWindow.");
    }

    if (remoteSession.get(2).CanvasSelector !== "#my-canvas-id") {
        throw new Error("CanvasSelector was not set correctly on RenderWindowInteractor.");
    }
}
const tests = [
    {
        description: "Bind RenderWindow to Canvas",
        test: testBindRenderWindow,
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
