export function getConfiguration(viewAPI) {
  // Sets up a base configuration for VTK-wasm with WebGL2 rendering.
  if (viewAPI === "webgl") {
    const configuration = {
      //Pipes std::cout and std::cerr into debug and error in dev console.
      print: (text) => console.debug(text),
      printErr: (text) => console.error(text),
      preRun: [function (module) {
        // select OpenGL backend (equivalent to WebGL2 in Emscripten)
        module.ENV.VTK_FACTORY_PREFER = 'RenderingBackend=OpenGL;Platform=WebAssembly';
      }],
    };
    return configuration;
  } else if (viewAPI === "webgpu") {
    // Sets up a base configuration for VTK-wasm with WebGPU rendering.
    const configuration = {
      //Pipes std::cout and std::cerr into debug and error in dev console.
      print: (text) => console.debug(text),
      printErr: (text) => console.error(text),
      preRun: [function (module) {
        // select WebGPU backend
        module.ENV.VTK_FACTORY_PREFER = 'RenderingBackend=WebGPU;Platform=WebAssembly';
      }],
    };
    return configuration;
  } else {
    console.warn(`Unsupported viewAPI: ${viewAPI}`);
    return null;
  }
}
