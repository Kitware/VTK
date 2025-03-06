import { options } from "./options.js";

if (navigator.gpu === undefined) {
    var webgpuSupportEl = document.createElement('h1');
    webgpuSupportEl.innerText = "Your browser does not support webgpu!";
    document.body.appendChild(webgpuSupportEl)
    var explainEl = document.createElement('p');
    explainEl.innerText = "On supported browsers, maybe serve from localhost instead of ip?";
    document.body.appendChild(explainEl)
} else {
    let canvas = document.querySelector('#canvas');
    canvas.style.display = 'block';

    let cfg = {
        arguments: [options.nx, options.ny, options.nz, options.mapperIsStatic],
        preRun: [function (module) {
            // select WEBGPU backend
            module.ENV.VTK_GRAPHICS_BACKEND = 'WEBGPU';
        }],
    };

    let runtime = await createConeMultiBackendModule(cfg);
    console.log(`WASM runtime initialized with arguments ${runtime.arguments}`);
}
