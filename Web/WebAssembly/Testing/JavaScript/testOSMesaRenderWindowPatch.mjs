async function testOSMesaRenderWindowPatch() {
    const manager = await globalThis.createVTKWasmSceneManager({});
    manager.initialize();
    manager.registerStateJSON({
        Id: 1,
        ClassName: "vtkOSOpenGLRenderWindow",
        SuperClassNames: ["vtkWindow", "vtkRenderWindow"],
        "vtk-object-manager-kept-alive": true,
    });
    if (manager.getState(1).ClassName !== "vtkWebAssemblyOpenGLRenderWindow") {
        throw new Error("RenderWindow state was not created as vtkWebAssemblyOpenGLRenderWindow.");
    }
    manager.updateObjectsFromStates();

    manager.updateObjectFromStateJSON({
        Id: 1,
        ClassName: "vtkOSOpenGLRenderWindow",
        SuperClassNames: ["vtkWindow", "vtkRenderWindow"],
        "vtk-object-manager-kept-alive": true,
    });
    if (manager.getState(1).ClassName !== "vtkWebAssemblyOpenGLRenderWindow") {
        throw new Error("RenderWindow state was not updated as vtkWebAssemblyOpenGLRenderWindow.");
    }
}
const tests = [
    {
        description: "Patch vtkOSOpenGLRenderWindow to vtkWebAssemblyOpenGLRenderWindow",
        test: testOSMesaRenderWindowPatch,
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
