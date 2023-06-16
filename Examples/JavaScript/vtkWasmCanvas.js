import { makeWebGLConfig, makeWebGPUConfig } from "./vtkWasmConfiguration.js";

// ----------------------------------------------------------------------------
function reportUnsupportedBrowser(reason) {
  var webgpuSupportEl = document.createElement('h1');
  webgpuSupportEl.innerText = "Your browser does not support webgpu!";
  document.body.appendChild(webgpuSupportEl)
  var explainEl = document.createElement('p');
  explainEl.innerText = reason;
  document.body.appendChild(explainEl)
}

// ----------------------------------------------------------------------------
export async function initializeVTKForRenderingWithWebGL() {
  return new Promise((resolve, reject) => {
    var vtkCanvas = document.createElement('canvas');
    vtkCanvas.setAttribute('id', 'canvas');
    vtkCanvas.setAttribute('style', "position: absolute; left: 0; top: 0;");
    document.body.appendChild(vtkCanvas);
    resolve(makeWebGLConfig(vtkCanvas));
  });
}

// ----------------------------------------------------------------------------
export async function initializeVTKForRenderingWithWebGPU(powerPreference, deviceDescriptor) {
  return new Promise((resolve, reject) => {
    var vtkCanvas = document.createElement('canvas');
    vtkCanvas.setAttribute('id', 'canvas');
    vtkCanvas.setAttribute('style', "position: absolute; left: 0; top: 0;");
    document.body.appendChild(vtkCanvas);

    if (navigator.gpu === undefined) {
      reportUnsupportedBrowser("On supported browsers, maybe serve from localhost instead of ip?");
    } else {
      navigator.gpu.requestAdapter(powerPreference).then(adapter => {
        if (adapter === null) {
          reportUnsupportedBrowser("Your browser did not provide a GPU adapter. Known to happen on Linux!");
          reject("WebGPU is not supported");
        }
        adapter.requestDevice(deviceDescriptor).then(device => {
          resolve(makeWebGPUConfig(vtkCanvas, device));
        })
      })
    }
  });
}
