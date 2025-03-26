import { BaseConfig, options, initCanvas } from "./config_base.js";

function makeWebGPUConfig() {
    var cfg = BaseConfig;
    cfg.canvas = (function () {
        var canvas = document.getElementById('canvas');
        return canvas;
    })()
    cfg.preRun = [function (module) {
        // select WEBGPU backend
        module.ENV.VTK_GRAPHICS_BACKEND = 'WEBGPU';
    }];
    cfg.arguments = [options.nx, options.ny, options.nz, options.mapperIsStatic];
    return cfg;
}

if (navigator.gpu === undefined) {
    var webgpuSupportEl = document.createElement('h1');
    webgpuSupportEl.innerText = "Your browser does not support webgpu!";
    document.body.appendChild(webgpuSupportEl)
    var explainEl = document.createElement('p');
    explainEl.innerText = "On supported browsers, maybe serve from localhost instead of ip?";
    document.body.appendChild(explainEl)
} else {
    var canvas = document.createElement('canvas');
    canvas.setAttribute('id', 'canvas');
    canvas.setAttribute('class', 'canvas');
    document.body.appendChild(canvas)

    let cfg = makeWebGPUConfig();

    let adapter = await navigator.gpu.requestAdapter();
    console.log("Found an adapter");
    let device = await adapter.requestDevice();
    console.log("Obtained a device");

    // Set the device from JS. This can be done in C++ as well.
    // See https://github.com/kainino0x/webgpu-cross-platform-demo/blob/main/main.cpp#L51
    cfg.preinitializedWebGPUDevice = device;

    let Runtime = await createConeMultiBackendModule(cfg);
    console.log(`WASM runtime initialized with arguments ${Runtime.arguments}`);
    initCanvas(canvas);
}
