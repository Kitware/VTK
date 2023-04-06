import { BaseConfig, options, initCanvas } from "./config_base.js";

function makeWebGLConfig() {
    var cfg = BaseConfig;
    cfg.canvas = (function () {
        var canvas = document.getElementById('canvas');
        canvas.addEventListener(
            "webglcontextlost",
            function (e) {
                console.error('WebGL context lost. You will need to reload the page.');
                e.preventDefault();
            },
            false
        );
        return canvas;
    })()
    cfg.arguments = [options.nx, options.ny, options.nz, options.mapperIsStatic];
    return cfg;
}

var canvas = document.createElement('canvas');
canvas.setAttribute('id', 'canvas');
canvas.setAttribute('class', 'canvas');
document.body.appendChild(canvas)

let cfg = makeWebGLConfig();

let Runtime = await createConeMultiBackendModule(cfg);
console.log(`WASM runtime initialized with arguments ${Runtime.arguments}`);
initCanvas(canvas);
