// ----------------------------------------------------------------------------
// Sets up a base configuration for VTK-wasm with rendering.
const BaseConfig = {
    //Pipes std::cout and std::cerr into debug and error in dev console.
    'print': (function () {
        return function (text) {
            text = Array.prototype.slice.call(arguments).join(' ');
            console.debug(text);
        };
    })(),
    'printErr': function (text) {
        text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
    }
};

// ----------------------------------------------------------------------------
export function makeWebGPUConfig(webgpuCanvas, webgpuDevice) {
    var cfg = BaseConfig;
    cfg.canvas = (function () {
        return webgpuCanvas;
    })()
    cfg.preRun = [function (module) {
        // select WebGPU backend
        module.ENV.VTK_FACTORY_PREFER = 'RenderingBackend=WebGPU';
    }];
    // Set the device from JS. This can be done in C++ as well.
    // See https://github.com/kainino0x/webgpu-cross-platform-demo/blob/main/main.cpp#L51
    cfg.preinitializedWebGPUDevice = webgpuDevice;
    return cfg;
}

// ----------------------------------------------------------------------------
export function makeWebGLConfig(webglCanvas) {
    var cfg = BaseConfig;
    cfg.canvas = (function () {
        webglCanvas.addEventListener(
            "webglcontextlost",
            function (e) {
                console.error('WebGL context lost. You will need to reload the page.');
                e.preventDefault();
            },
            false
        );
        return webglCanvas;
    })()
    return cfg;
}
