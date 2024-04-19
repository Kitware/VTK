async function testInitialize() {
  const manager = await globalThis.createVTKWasmSceneManager({});
  if (!manager.initialize()) {
    throw new Error();
  }
}
const tests = [
  {
    description: "Initialize VTK scene manager",
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
