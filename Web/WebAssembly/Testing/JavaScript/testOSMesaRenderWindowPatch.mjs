async function testOSMesaRenderWindowPatch() {
    const vtkWASM = await globalThis.createVTKWASM({})
    const remoteSession = new vtkWASM.vtkRemoteSession();
    remoteSession.registerState({
        Id: 1,
        ClassName: "vtkOSOpenGLRenderWindow",
        SuperClassNames: ["vtkWindow", "vtkRenderWindow"],
        "vtk-object-manager-kept-alive": true,
    });
    remoteSession.updateObjectsFromStates();
    if (remoteSession.get(1).ClassName === "vtkOSOpenGLRenderWindow") {
        throw new Error("RenderWindow state should not have vtkOSOpenGLRenderWindow.");
    }


    remoteSession.updateObjectFromState({
        Id: 1,
        ClassName: "vtkOSOpenGLRenderWindow",
        SuperClassNames: ["vtkWindow", "vtkRenderWindow"],
        "vtk-object-manager-kept-alive": true,
    });
    if (remoteSession.get(1).ClassName === "vtkOSOpenGLRenderWindow") {
        throw new Error("RenderWindow state should not have vtkOSOpenGLRenderWindow.");
    }
}
const tests = [
    {
        description: "Patch vtkOSOpenGLRenderWindow to vtkRenderWindow",
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
