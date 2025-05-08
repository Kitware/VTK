async function testBindRenderWindow() {
    const manager = await globalThis.createVTKWasmSceneManager({});
    manager.initialize();
    manager.registerStateJSON(
        {
            Id: 1,
            ClassName: "vtkCocoaRenderWindow",
            SuperClassNames: ["vtkRenderWindow"],
            Interactor: { Id: 2 },
            "vtk-object-manager-kept-alive": true,
        });
    manager.registerStateJSON(
        {
            Id: 2,
            ClassName: "vtkCocoaRenderWindowInteractor",
            SuperClassNames: ["vtkRenderWindowInteractor"],
            RenderWindow: { Id: 1 },
        });
    manager.updateObjectsFromStates();

    manager.bindRenderWindow(1, "#my-canvas-id");

    manager.updateStateFromObject(1);
    if (manager.getState(1).CanvasSelector !== "#my-canvas-id") {
        throw new Error("CanvasSelector was not set correctly on RenderWindow.");
    }

    manager.updateStateFromObject(2);
    if (manager.getState(2).CanvasSelector !== "#my-canvas-id") {
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
