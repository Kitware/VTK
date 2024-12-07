import { options } from "./options.js";

let canvas = document.querySelector('#canvas');
canvas.style.display = 'block';

let runtime = await createConeMultiBackendModule({ 'arguments': [options.nx, options.ny, options.nz, options.mapperIsStatic] });
console.log(`WASM runtime initialized with arguments ${runtime.arguments}`);
