import { readFile } from "fs/promises";
import path from "path";

const exepected_dependencies = [1, 2, 3, 65, 66, 5, 33, 35, 4, 67, 6, 45, 48, 52, 54, 55, 56, 59, 62, 34, 12, 36, 37, 38, 39, 40, 41, 42, 43, 44, 7, 46, 49, 50, 51, 53, 57, 60, 63, 13, 8, 9, 11, 47, 58, 61, 64, 10, 14, 15, 16, 17, 18, 21, 23, 26, 29, 32, 19, 20, 22, 24, 25, 27, 28, 30, 31]

async function testStates() {
  const dataDirectoryIndex = process.argv.indexOf("-D") + 1;
  if (dataDirectoryIndex <= 0) {
    throw new Error("Please provide path to a blobs file using -D");
  }
  const dataDirectory = process.argv[dataDirectoryIndex];
  const blobs = JSON.parse(await readFile(path.join(dataDirectory, "Data", "WasmSceneManager", "scalar-bar-widget.blobs.json")));
  const states = JSON.parse(await readFile(path.join(dataDirectory, "Data", "WasmSceneManager", "scalar-bar-widget.states.json")));
  const object_ids = Object.keys(states).map((k) => Number(k)).sort((a, b) => a - b);
  const vtkWASM = await globalThis.createVTKWASM({})
  const remoteSession = new vtkWASM.vtkRemoteSession();
  for (let i = 0; i < object_ids.length; ++i) {
    const object_id = object_ids[i];
    if (!remoteSession.registerState(states[object_id])) {
      throw new Error(`Failed to register state at object_id=${object_id}`);
    }
  }
  for (let hash in blobs) {
    if (!remoteSession.registerBlob(hash, new Uint8Array(blobs[hash].bytes))) {
      throw new Error(`Failed to register blob with hash=${hash}`);
    }
  }
  remoteSession.updateObjectsFromStates();
  const activeIds = remoteSession.getAllDependencies(0);
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
