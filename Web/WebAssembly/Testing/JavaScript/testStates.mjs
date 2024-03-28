import { readFile } from "fs/promises";
import path from "path";

const object_ids = [1, 2, 3, 41, 5, 42, 44, 4, 6, 33, 35, 38, 40, 43, 11, 45, 46, 47, 48, 49, 50, 51, 7, 34, 36, 37, 39, 12, 8, 9, 10, 13, 14, 15, 16, 19, 21, 24, 27, 30, 17, 18, 20, 22, 23, 25, 26, 28, 29, 31, 32]
const exepected_dependencies = [1, 2, 3, 41, 5, 42, 44, 4, 6, 33, 35, 38, 40, 43, 11, 45, 46, 47, 48, 49, 50, 51, 7, 34, 36, 37, 39, 12, 8, 9, 10, 13, 14, 15, 16, 19, 21, 24, 27, 30, 17, 18, 20, 22, 23, 25, 26, 28, 29]

async function testStates() {
  const dataDirectoryIndex = process.argv.indexOf("-D") + 1;
  if (dataDirectoryIndex <= 0) {
    throw new Error("Please provide path to a blobs file using -D");
  }
  const dataDirectory = process.argv[dataDirectoryIndex];
  const blobs = JSON.parse(await readFile(path.join(dataDirectory, "Data", "WasmSceneManager", "scalar-bar-widget.blobs.json")));
  const states = JSON.parse(await readFile(path.join(dataDirectory, "Data", "WasmSceneManager", "scalar-bar-widget.states.json")));
  const manager = await globalThis.createVTKWasmSceneManager({});
  if (!manager.initialize()) {
    throw new Error("Failed to initialize scene manager");
  }
  for (let i = 0; i < object_ids.length; ++i) {
    const object_id = object_ids[i];
    if (!manager.registerState(JSON.stringify(states[object_id]))) {
      throw new Error(`Failed to register state at object_id=${object_id}`);
    }
  }
  for (let hash in blobs) {
    if (!manager.registerBlob(hash, new Uint8Array(blobs[hash].bytes))) {
      throw new Error(`Failed to register blob with hash=${hash}`);
    }
  }
  manager.updateObjectsFromStates();
  const activeIds = manager.getAllDependencies(0);
  if (!(activeIds instanceof Uint32Array)) {
    throw new Error("getAllDependencies did not return a Uint32Array");
  }
  if (activeIds.toString() != exepected_dependencies.toString()) {
    throw new Error(`${activeIds} != ${exepected_dependencies}`);
  }
}

const tests = [
  {
    description: "Register states",
    test: testStates,
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
