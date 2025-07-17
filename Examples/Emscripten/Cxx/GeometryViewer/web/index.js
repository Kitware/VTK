import { maxSize } from "./ArrayBufferLimits.js";
import { chunkify } from "./chunkify.js";
import { hexToRgb } from "./hexToRgb.js";
import { getConfiguration } from './wasmConfigure.js';
import { download } from "./fileDownload.js";
import createGeometryViewerModule from "./GeometryViewer.js";
import { openDirectory } from "./openDirectory.js";
import { hasWebGPU } from "./wasmWebGPUInit.js";
import { geometryViewerPropertiesFromURL } from "./properties.js";

var GUI = lil.GUI;

// Default properties
const initialProps = {
  viewApi: "webgl",
  url: "",
  showControls: true,
  representation: 2,
  vertexVisibility: false,
  renderPointsAsSpheres: false,
  pointSize: 1.0,
  edgeVisibility: false,
  renderLinesAsTubes: false,
  lineWidth: 1.0,
  colorByArray: 'Solid',
  colorMapPreset: 'Warm',
  interpolateScalarsBeforeMapping: false,
  solidColor: 0xffffff,
  vertexColor: 0x80ff80,
  edgeColor: 0x000000,
  opacity: 1.0,
  edgeOpacity: 1.0,
  mouseWheelMotionFactor: 0.15,
  backgroundColor1: 0x000000,
  backgroundColor2: 0x1d2671,
  highlightOnHover: 0,
  ditherGradient: true,
  orthographic: false,
};

const options = {
  ...geometryViewerPropertiesFromURL(new URL(window.location), initialProps),
  simulateFileInput: () => document.getElementById('vtk-input')?.click(),
  resetSession: onResetSession,
  simulateDirectoryInput: onDirectoryChanged,
  colorArrays: ['Solid'],
};

let pendingCompositeFileName = "";
let wasmModule = null;
let viewer = null;
let gui = null;
let fpsScript = null;
let supportsWebGPU = false;
let colorByArraysController = null;

// Utility functions
async function writeFileToWASMMemory(file) {
  if (!wasmModule) return 0;
  let chunks = chunkify(file, Number(maxSize));
  let offset = 0;
  let ptr = wasmModule._malloc(file.size);
  for (let chunk of chunks) {
    let data = new Uint8Array(await chunk.arrayBuffer());
    wasmModule.HEAPU8.set(data, ptr + offset);
    offset += data.byteLength;
  }
  return ptr;
}

async function postFileLoad() {
  if (gui) {
    let pointDataArrays = await viewer.getPointDataArrays();
    let cellDataArrays = await viewer.getCellDataArrays();
    let colorArrays = [
      'Solid',
      ...(pointDataArrays ? pointDataArrays.split(';') : []),
      ...(cellDataArrays ? cellDataArrays.split(';') : [])
    ];
    colorByArraysController.options(colorArrays.filter(el => el.length > 0));
    colorByArraysController.onChange(async () => {
      await viewer.setColorMapPreset(options.colorMapPreset);
      await viewer.setColorByArray(options.colorByArray);
      await viewer.render();
    });
    await viewer.setColorMapPreset(options.colorMapPreset);
    await viewer.setColorByArray(options.colorByArray);
    await viewer.render();
  } else {
    await viewer.setColorMapPreset(options.colorMapPreset);
    await viewer.setColorByArray(options.colorByArray);
    await viewer.render();
  }
  await viewer.resetView();
  await viewer.render();
}

async function loadFileFromBlob(file) {
  if (!wasmModule || !viewer) return;
  let ptr = await writeFileToWASMMemory(file);
  await viewer.loadDataFileFromMemory(file.name, ptr, file.size);
  wasmModule._free(ptr);
  await postFileLoad();
}

async function loadFileByName(filename) {
  if (!wasmModule || !viewer) return;
  await viewer.loadDataFile(filename);
  await postFileLoad();
}

async function onResetSession() {
  await viewer.removeAllActors();
}

async function onFilesChanged() {
  if (!wasmModule || !viewer) return;
  let inputEl = document.getElementById('vtk-input');
  let files = inputEl.files;
  if (files.length === 0) return;
  if (files[0].name.endsWith('.vtm') || files[0].name.endsWith('.vtpc')) {
    pendingCompositeFileName = files[0].name;
    let ptr = await writeFileToWASMMemory(files[0]);
    await viewer.writeDataFileToVirtualFS(files[0].name, ptr, files[0].size);
    wasmModule._free(ptr);
    alert("You have selected a composite dataset file. Please click on Choose directory and pick the accompanying directories.");
  } else {
    await loadFileFromBlob(files[0]);
  }
}

async function onDirectoryChanged() {
  if (!wasmModule || !viewer) return;
  if (pendingCompositeFileName) {
    let directoryFiles = await openDirectory();
    for (let file of directoryFiles) {
      let ptr = await writeFileToWASMMemory(file);
      await viewer.writeDataFileToVirtualFS(file.directoryHandle.name + "/" + file.name, ptr, file.size);
      wasmModule._free(ptr);
    }
    await loadFileByName(pendingCompositeFileName);
  }
}

async function setupUI() {
  // FPS counter
  fpsScript = document.createElement('script');
  fpsScript.onload = () => {
    var stats = new Stats();
    document.body.appendChild(stats.dom);
    requestAnimationFrame(function loop() {
      stats.update();
      requestAnimationFrame(loop);
    });
  };
  fpsScript.src = 'https://mrdoob.github.io/stats.js/build/stats.min.js';
  document.head.appendChild(fpsScript);

  // GUI controls
  gui = new GUI();
  gui.add(options, 'resetSession').name('Reset session');
  gui.add(options, 'simulateFileInput').name('Choose file');
  gui.add(options, 'simulateDirectoryInput').name('Choose directory');
  const meshFolder = gui.addFolder('Mesh');
  meshFolder.add(options, 'representation', { Points: 0, Wireframe: 1, Surface: 2 }).onChange(async () => {
    await viewer.setRepresentation(options.representation);
    await viewer.render();
  });
  meshFolder.add(options, 'vertexVisibility').onChange(async () => {
    await viewer.setVertexVisibility(options.vertexVisibility);
    await viewer.render();
  });
  meshFolder.add(options, 'renderPointsAsSpheres').onChange(async () => {
    await viewer.setRenderPointsAsSpheres(options.renderPointsAsSpheres);
    await viewer.render();
  });
  meshFolder.add(options, 'pointSize', 0.1, 10.0, 0.01).onChange(async () => {
    await viewer.setPointSize(options.pointSize);
    await viewer.render();
  });
  meshFolder.add(options, 'edgeVisibility').onChange(async () => {
    await viewer.setEdgeVisibility(options.edgeVisibility);
    await viewer.render();
  });
  meshFolder.add(options, 'renderLinesAsTubes').onChange(async () => {
    await viewer.setRenderLinesAsTubes(options.renderLinesAsTubes);
    await viewer.render();
  });
  meshFolder.add(options, 'lineWidth', 0.1, 5.0, 0.01).onChange(async () => {
    await viewer.setLineWidth(options.lineWidth);
    await viewer.render();
  });

  const colorFolder = gui.addFolder('Color');
  const scalarMapFolder = colorFolder.addFolder('Scalar Mapping');
  colorByArraysController = scalarMapFolder.add(options, 'colorByArray', options.colorArrays);
  let presets = await viewer.getColorMapPresets();
  let presetList = presets.length > 0 ? presets.split(';').filter(p => p.length > 0) : [];
  if (presetList.length > 0) {
    scalarMapFolder.add(options, 'colorMapPreset', presetList).onChange(async () => {
      await viewer.setColorMapPreset(options.colorMapPreset);
      await viewer.render();
    });
  }
  scalarMapFolder.add(options, 'interpolateScalarsBeforeMapping').onChange(async () => {
    await viewer.setInterpolateScalarsBeforeMapping(options.interpolateScalarsBeforeMapping);
    await viewer.render();
  });
  colorFolder.addColor(options, 'solidColor').onChange(async () => {
    const rgb = hexToRgb(options.solidColor);
    await viewer.setColor(rgb[0], rgb[1], rgb[2]);
    await viewer.render();
  });
  colorFolder.addColor(options, 'vertexColor').onChange(async () => {
    const rgb = hexToRgb(options.vertexColor);
    await viewer.setVertexColor(rgb[0], rgb[1], rgb[2]);
    await viewer.render();
  });
  colorFolder.addColor(options, 'edgeColor').onChange(async () => {
    const rgb = hexToRgb(options.edgeColor);
    await viewer.setEdgeColor(rgb[0], rgb[1], rgb[2]);
    await viewer.render();
  });
  colorFolder.add(options, 'opacity', 0.0, 1.0, 0.01).onChange(async () => {
    await viewer.setOpacity(options.opacity);
    await viewer.render();
  });
  colorFolder.add(options, 'edgeOpacity', 0.0, 1.0, 0.01).onChange(async () => {
    await viewer.setEdgeOpacity(options.edgeOpacity);
    await viewer.render();
  });

  const viewFolder = gui.addFolder('View');
  viewFolder.addColor(options, 'backgroundColor1').onChange(async () => {
    const rgb = hexToRgb(options.backgroundColor1);
    await viewer.setBackgroundColor1(rgb[0], rgb[1], rgb[2]);
    await viewer.render();
  });
  viewFolder.addColor(options, 'backgroundColor2').onChange(async () => {
    const rgb = hexToRgb(options.backgroundColor2);
    await viewer.setBackgroundColor2(rgb[0], rgb[1], rgb[2]);
    await viewer.render();
  });
  viewFolder.add(options, 'mouseWheelMotionFactor', 0.15, 1.0, 0.001).onChange(async () => {
    await viewer.setMouseWheelMotionFactor(options.mouseWheelMotionFactor);
  });
  viewFolder.add(options, 'highlightOnHover', { None: 0, Points: 1, Cells: 2 }).onChange(async () => {
    if (options.highlightOnHover == 0) {
      await viewer.setHighlightOnHover(false, false);
    } else if (options.highlightOnHover == 1) {
      await viewer.setHighlightOnHover(true, true);
    } else if (options.highlightOnHover == 2) {
      await viewer.setHighlightOnHover(true, false);
    }
    await viewer.render();
  });
  viewFolder.add(options, 'ditherGradient').onChange(async () => {
    await viewer.setDitherGradient(options.ditherGradient);
    await viewer.render();
  });
  viewFolder.add(options, 'orthographic').onChange(async () => {
    await viewer.setUseOrthographicProjection(options.orthographic);
    await viewer.render();
  });
  viewFolder.add({
    ResetView: async () => {
      await viewer.resetView();
      await viewer.render();
    }
  }, 'ResetView');
}

// Main initialization
async function main() {
  // Create DOM elements
  const appDiv = document.createElement('div');
  appDiv.style.position = 'absolute';
  appDiv.style.left = '0';
  appDiv.style.top = '0';
  appDiv.style.width = '100vw';
  appDiv.style.height = '100vh';

  // File input
  const fileInput = document.createElement('input');
  fileInput.type = 'file';
  fileInput.id = 'vtk-input';
  fileInput.accept = '.obj, .ply, .vtk, .vtp, .vtu, .vtm, .vtpc';
  fileInput.required = true;
  fileInput.style.display = 'none';
  fileInput.addEventListener('change', onFilesChanged);

  // Canvas
  const canvasContainer = document.createElement('div');
  canvasContainer.className = 'canvas_container';
  const canvas = document.createElement('canvas');
  canvas.id = 'vtk-3d-canvas';
  canvas.className = 'GeometryViewer' + options.viewApi + 'Canvas';
  canvasContainer.appendChild(canvas);

  // Tooltip
  const tooltip = document.createElement('div');
  tooltip.className = 'tooltip';
  tooltip.style.visibility = 'hidden';
  tooltip.style.backgroundColor = 'black';
  tooltip.style.color = '#fff';
  tooltip.style.textAlign = 'center';
  tooltip.style.borderRadius = '6px';
  tooltip.style.padding = '5px';
  tooltip.style.position = 'absolute';
  tooltip.style.opacity = '0.7';
  tooltip.style.zIndex = '1';
  tooltip.setAttribute('unselectable', 'on');
  tooltip.setAttribute('onselectstart', 'return false;');
  tooltip.setAttribute('onmousedown', 'return false;');
  tooltip.setAttribute('oncontextmenu', 'event.preventDefault()');

  appDiv.appendChild(canvasContainer);
  appDiv.appendChild(tooltip);
  document.body.appendChild(fileInput);
  document.body.appendChild(appDiv);

  // WebGPU support check
  if (options.viewApi === "webgpu") {
    supportsWebGPU = await hasWebGPU();
    if (!supportsWebGPU) {
      const msgDiv = document.createElement('div');
      msgDiv.innerHTML = `<h3>This application cannot run because your browser does not support WebGPU!</h3>
                <p>Your browser did not provide a GPU adapter. Known to happen on Linux!</p>`;
      document.body.appendChild(msgDiv);
      return;
    }
  }

  // Initialize viewer
  let configuration = getConfiguration(options.viewApi);
  wasmModule = await createGeometryViewerModule(configuration);
  viewer = new wasmModule.GeometryViewer();
  await viewer.initialize();
  await viewer.resetView();
  await viewer.render();
  await viewer.start();

  // Drag and drop events
  canvas.addEventListener('dragover', e => e.preventDefault(), false);
  canvas.addEventListener('dragenter', () => canvas.classList.add('drag-active'));
  canvas.addEventListener('dragleave', () => canvas.classList.remove('drag-active'));
  canvas.addEventListener('drop', async e => {
    e.preventDefault();
    canvas.classList.remove('drag-active');
    const dataTransfer = e.dataTransfer;
    await loadFileFromBlob(dataTransfer.files[0]);
  });

  // Controls
  if (options.showControls) {
    await setupUI();
  } else {
    await viewer.setBackgroundColor1(255, 255, 255);
    await viewer.setBackgroundColor2(255, 255, 255);
  }

  // Load default file
  if (options.url.length) {
    let { blob, filename } = await download(options.url);
    await loadFileFromBlob(new File([blob], filename));
  }
}

// Cleanup
window.addEventListener('beforeunload', () => {
  if (fpsScript) document.head.removeChild(fpsScript);
  if (gui) gui.destroy();
  if (viewer) viewer.delete();
  wasmModule = null;
});

// Start app
await main();
