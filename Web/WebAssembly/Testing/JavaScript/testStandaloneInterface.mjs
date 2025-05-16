// This file can be run interactively in a browser environment with `python -m http.server` in the current directory
// To run this in `node` environment, just run `node standalone.js`. It won't start the interactor, but will still create the VTK objects and log their properties.
// import "./vtkWebAssemblyInterface.mjs";

function makeQuadMesh(nx, ny) {
    // Create a grid of points on the XY plane from (0, 0) to (nx, ny)
    let pointJSArray = []
    for (let i = 0; i < ny + 1; i++) {
        for (let j = 0; j < nx + 1; j++) {
            let x = (j - 0.5 * nx) / nx;
            let y = (i - 0.5 * ny) / ny;
            pointJSArray.push(x); // x-coordinate
            pointJSArray.push(y); // y-coordinate
            pointJSArray.push(Math.sqrt(x * x + y * y) * Math.sin(x) * Math.cos(y)); // z-coordinate
        }
    }

    let connectivityJSArray = [];
    let offsetsJSArray = [];
    for (let i = 0; i < ny; i++) {
        for (let j = 0; j < nx; j++) {
            offsetsJSArray.push(connectivityJSArray.length);
            connectivityJSArray.push(j + i * (nx + 1));
            connectivityJSArray.push(j + i * (nx + 1) + 1);
            connectivityJSArray.push(j + i * (nx + 1) + nx + 2);
            connectivityJSArray.push(j + i * (nx + 1) + nx + 1);
        }
    }
    offsetsJSArray.push(connectivityJSArray.length);
    return { points: pointJSArray, offsets: offsetsJSArray, connectivity: connectivityJSArray };
}

const vtkWasm = await globalThis.createVTKWASM({
    preRun: [function (module) {
        /// select WEBGPU backend
        module.ENV.VTK_GRAPHICS_BACKEND = "WEBGPU";
        /// enable logging for debugging purposes (optional)
        // module.ENV.VTK_DESERIALIZER_LOG_VERBOSITY = "INFO";
        // module.ENV.VTK_INVOKER_LOG_VERBOSITY = "INFO";
        // module.ENV.VTK_OBJECT_MANAGER_LOG_VERBOSITY = "INFO";
    }],
});
let session = new vtkWasm.vtkStandaloneSession();

let mesh = makeQuadMesh(100, 100);

let points = session.create("vtkPoints");
let quads = session.create("vtkCellArray");
let connectivity = session.create("vtkTypeInt32Array");
let offsets = session.create("vtkTypeInt32Array");
let polyData = session.create("vtkPolyData");

// The SetArray method is available on any vtkDataArray object.
// It takes a typed array as input and sets the data for the object.
// The typed array must be of the same type as the vtkDataArray object.
await session.invoke(session.get(points).Data.Id, "SetArray", [new Float32Array(mesh.points)]);
await session.invoke(connectivity, "SetArray", [new Int32Array(mesh.connectivity)]);
await session.invoke(offsets, "SetArray", [new Int32Array(mesh.offsets)]);
// Set the data for the quads cell array
await session.invoke(quads, "SetData", [{ Id: offsets }, { Id: connectivity }]);
// Create a polydata object and set the points and quads
await session.invoke(polyData, "SetPoints", [{ Id: points }]);
await session.invoke(polyData, "SetPolys", [{ Id: quads }]);
console.log("NumberOfPoints:", await session.invoke(polyData, "GetNumberOfPoints", [])); // Should print the number of points in the polydata
console.log("NumberOfCells:", await session.invoke(polyData, "GetNumberOfCells", [])); // Should print the number of cells in the polydata
console.log("PolyDataBounds:", await session.invoke(polyData, "GetBounds", [])); // Should print the bounds of the polydata

let mapper = session.create("vtkPolyDataMapper");
await session.invoke(mapper, "SetInputData", [{ Id: polyData }]);

let bounds = await session.invoke(mapper, "GetBounds", []);
console.log("MapperBounds:", bounds); // Should print the bounds of the mapper

let actor = session.create("vtkActor");
session.set(session.get(actor).Property.Id, { EdgeVisibility: true });
await session.invoke(actor, "SetMapper", [{ Id: mapper }]);

let renderer = session.create("vtkRenderer");
await session.invoke(renderer, "AddActor", [{ Id: actor }]);
await session.invoke(renderer, "ResetCamera", []); // Reset the camera to fit the actor

let renderWindow = session.create("vtkRenderWindow");
// Set the canvas selector for the render window
session.set(renderWindow, { CanvasSelector: "#vtk-wasm-renderer-canvas-1" });
await session.invoke(renderWindow, "AddRenderer", [{ Id: renderer }]);

// add observers to log rendering events
session.observe(renderWindow, "StartEvent", async () => {
    console.log("Render window started rendering.");
    let camera = await session.invoke(renderer, "GetActiveCamera", []);
    console.log("Camera position:", camera.Position);
});
session.observe(renderWindow, "EndEvent", () => {
    console.log("Render window finished rendering.");
});

let interactor = session.create("vtkRenderWindowInteractor");
// Set the canvas selector for the render window interactor
session.set(interactor, { CanvasSelector: "#vtk-wasm-renderer-canvas-1", RenderWindow: { Id: renderWindow } });
console.log("Interactor properties get:", session.get(interactor));

// Check if the document is available before starting the interactor
if (typeof document !== "undefined") {
    // Ensure the canvas is available before starting the interactor
    if (document.querySelector("#vtk-wasm-renderer-canvas-1")) {
        await session.invoke(interactor, "Render", []);
        await session.invoke(interactor, "Start", []);
    } else {
        console.error("Canvas not found. Please ensure the canvas is available before starting the interactor.");
    }
}
