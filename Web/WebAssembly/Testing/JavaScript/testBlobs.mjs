import { readFile } from "fs/promises";
import path from "path";

async function testBlobs() {
  const dataDirectoryIndex = process.argv.indexOf("-D") + 1;
  if (dataDirectoryIndex <= 0) {
    throw new Error("Please provide path to a blobs file using -D");
  }
  const dataDirectory = process.argv[dataDirectoryIndex];
  const blobs = JSON.parse(await readFile(path.join(dataDirectory, "Data", "WasmSceneManager", "simple.blobs.json")));
  const manager = await globalThis.createVTKWasmSceneManager({})
  if (!manager.initialize()) {
    throw new Error("Failed to initialize scene manager");
  }

  for (let hash in blobs) {
    if (!manager.registerBlob(hash, new Uint8Array(blobs[hash].bytes))) {
      throw new Error(`Failed to register blob with hash=${hash}`);
    }
  }
  for (let hash in blobs) {
    const blob = manager.getBlob(hash);
    if (!(blob instanceof Uint8Array)) {
      throw new Error(`getBlob did not return a Uint8Array for hash=${hash}`);
    }
    if (blob.toString() !== blobs[hash].bytes.toString()) {
      throw new Error(`blob for hash=${hash} does not match registered blob.`);
    }
  }
}

const tests = [
  {
    description: "Register blobs with hashes",
    test: testBlobs,
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
