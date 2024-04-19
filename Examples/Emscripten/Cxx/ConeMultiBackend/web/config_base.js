export var options = {
    nx: '10',
    ny: '10',
    nz: '100',
    mapperIsStatic: '1'
};

export const BaseConfig = {
    'canvas': (function () {
        var canvas = document.getElementById('canvas');
        return canvas;
    })(),
    'print': (function () {
        return function (text) {
            text = Array.prototype.slice.call(arguments).join(' ');
            console.info(text);
        };
    })(),
    'printErr': function (text) {
        text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
    }
};

export function initCanvas(canvas) {
    // sends a resize event so that the render window fills up browser tab dimensions.
    setTimeout(() => {
        window.dispatchEvent(new Event('resize'));
    }, 0);
    // focus on the canvas to grab keyboard inputs.
    canvas.setAttribute('tabindex', '0');
    // grab focus when the render window region receives mouse clicks.
    canvas.addEventListener('click', () => canvas.focus());
}
