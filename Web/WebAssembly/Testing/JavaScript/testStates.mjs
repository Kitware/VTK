import { readFile } from "fs/promises";
import path from "path";

const exepected_dependencies = [1, 2, 70, 71, 73, 3, 72, 74, 4, 34, 39, 5, 49, 52, 56, 58, 59, 60, 61, 64, 67, 35, 36, 37, 38, 11, 40, 41, 42, 43, 44, 45, 46, 47, 48, 6, 50, 53, 54, 55, 57, 62, 65, 68, 12, 7, 8, 10, 51, 63, 66, 69, 9, 13, 14, 15, 16, 17, 20, 22, 25, 28, 31, 18, 19, 21, 23, 24, 26, 27, 29, 30, 32, 33]

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
