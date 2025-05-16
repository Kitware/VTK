async function testInitialize() {
  const vtkWASM = await globalThis.createVTKWASM({})
  const remoteSession = new vtkWASM.vtkRemoteSession();
  console.log("remote session init success ", remoteSession);
  const standaloneSession = new vtkWASM.vtkStandaloneSession();
  console.log("standalone init success ", standaloneSession);
}
const tests = [
  {
    description: "Initialize VTK sessions",
    test: testInitialize,
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
