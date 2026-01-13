function makeStatesForGraphicsAPI(graphicsAPI) {
  return [{
    Id: 1,
    ClassName: `vtk${graphicsAPI}RenderWindow`,
    Interactor: { Id: 2 },
    Renderers: { Id: 3 },
    Size: [800, 600],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkWindow",
      "vtkRenderWindow"],
    "vtk-object-manager-kept-alive": true,
  },
  {
    Id: 2,
    ClassName: "vtkRenderWindowInteractor",
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject"]
  },
  {
    Id: 3,
    ClassName: "vtkRendererCollection",
    Items: [
      { Id: 4 },
    ],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkCollection",],
  },
  {
    Id: 4,
    ClassName: `vtk${graphicsAPI}Renderer`,
    ViewProps: { Id: 5 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkViewport",
      "vtkRenderer",],
  },
  {
    Id: 5,
    ClassName: "vtkPropCollection",
    Items: [
      { Id: 6 }, // actor -> vtkPolyDataMapper
      { Id: 10 }, // actor -> vtkGlyph3DMapper
      { Id: 14 }, // actor2D -> vtkPolyDataMapper2D
      { Id: 18 }, // actor2D -> vtkCompositePolyDataMapper
    ],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkCollection",],
  },
  {
    Id: 6,
    ClassName: `vtk${graphicsAPI}Actor`,
    Mapper: { Id: 7 },
    Property: { Id: 8 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProp",
      "vtkActor",],
  },
  {
    Id: 7,
    ClassName: `vtk${graphicsAPI}PolyDataMapper`,
    InputDataObjects: [[{ Id: 9 }]],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkAlgorithm",
      "vtkAbstractMapper",
      "vtkAbstractMapper3D",
      "vtkMapper",
      "vtkPolyDataMapper",
    ],
    UseLookupTableScalarRange: 0
  },
  {
    Id: 8,
    ClassName: `vtk${graphicsAPI}Property`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProperty",
    ],
  },
  {
    Id: 9,
    ClassName: "vtkPolyData",
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkDataObject",
      "vtkDataSet",
      "vtkPointSet",
    ],
  }, {
    Id: 10,
    ClassName: `vtk${graphicsAPI}Actor`,
    Mapper: { Id: 11 },
    Property: { Id: 12 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProp",
      "vtkActor",],
  },
  {
    Id: 11,
    ClassName: `vtk${graphicsAPI}Glyph3DMapper`,
    InputDataObjects: [[{ Id: 13 }], [{ Id: 13 }]],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkAlgorithm",
      "vtkAbstractMapper",
      "vtkAbstractMapper3D",
      "vtkMapper",
      "vtkGlyph3DMapper",
    ],
    UseLookupTableScalarRange: 0
  },
  {
    Id: 12,
    ClassName: `vtk${graphicsAPI}Property`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProperty",
    ],
  },
  {
    Id: 13,
    ClassName: "vtkPolyData",
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkDataObject",
      "vtkDataSet",
      "vtkPointSet",
    ],
  }, {
    Id: 14,
    ClassName: `vtkActor2D`,
    Mapper: { Id: 15 },
    Property: { Id: 16 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProp",],
  },
  {
    Id: 15,
    ClassName: `vtk${graphicsAPI}PolyDataMapper2D`,
    InputDataObjects: [[{ Id: 17 }]],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkAlgorithm",
      "vtkAbstractMapper",
      "vtkMapper2D",
      "vtkPolyDataMapper2D",
    ],
    UseLookupTableScalarRange: 0
  },
  {
    Id: 16,
    ClassName: `vtk${graphicsAPI}Property`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProperty",
    ],
  },
  {
    Id: 17,
    ClassName: "vtkPolyData",
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkDataObject",
      "vtkDataSet",
      "vtkPointSet",
    ],
  }, {
    Id: 18,
    ClassName: `vtkActor`,
    Mapper: { Id: 19 },
    Property: { Id: 20 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProp",],
  },
  {
    Id: 19,
    ClassName: `vtkCompositePolyDataMapper`,
    InputDataObjects: [[{ Id: 21 }]],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkAlgorithm",
      "vtkAbstractMapper",
      "vtkAbstractMapper3D",
      "vtkMapper",
      "vtkPolyDataMapper",
    ],
    UseLookupTableScalarRange: 0
  },
  {
    Id: 20,
    ClassName: `vtk${graphicsAPI}Property`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProperty",
    ],
  },
  {
    Id: 21,
    ClassName: "vtkPolyData",
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkDataObject",
      "vtkDataSet",
      "vtkPointSet",
    ],
  }]
}

async function testOpenGLOverrides() {
  const vtkWASM = await globalThis.createVTKWASM({
    preRun: [function (module) {
      /// select OpenGL backend
      module.ENV.VTK_GRAPHICS_BACKEND = "OPENGL";
      /// enable logging for debugging purposes (optional)
      // module.ENV.VTK_DESERIALIZER_LOG_VERBOSITY = "INFO";
      // module.ENV.VTK_INVOKER_LOG_VERBOSITY = "INFO";
      // module.ENV.VTK_OBJECT_MANAGER_LOG_VERBOSITY = "INFO";
    }],
  });
  const session = new vtkWASM.vtkRemoteSession();
  const states = makeStatesForGraphicsAPI("WebGPU");
  for (const state of states) {
    session.registerState(state);
  }
  session.updateObjectsFromStates();
  // Since the OpenGL backend is used, the classes with WebGPU in their names should have
  // be replaced with OpenGL overrides.
  for (const state of states) {
    const objectState = session.get(state.Id);
    if (objectState.ClassName.includes("WebGPU") && !objectState.ClassName.includes("OpenGL")) {
      throw new Error(`Object ${JSON.stringify(objectState)} should include OpenGL instead of WebGPU`);
    }
  }
}

async function testWebGPUOverrides() {
  const vtkWASM = await globalThis.createVTKWASM({
    preRun: [function (module) {
      /// select OpenGL backend
      module.ENV.VTK_GRAPHICS_BACKEND = "WEBGPU";
      /// enable logging for debugging purposes (optional)
      // module.ENV.VTK_DESERIALIZER_LOG_VERBOSITY = "INFO";
      // module.ENV.VTK_INVOKER_LOG_VERBOSITY = "INFO";
      // module.ENV.VTK_OBJECT_MANAGER_LOG_VERBOSITY = "INFO";
    }],
  });
  const session = new vtkWASM.vtkRemoteSession();
  const states = makeStatesForGraphicsAPI("OpenGL");
  for (const state of states) {
    session.registerState(state);
  }
  session.updateObjectsFromStates();
  // Since the WebGPU backend is used, the classes with OpenGL in their names should have
  // be replaced with WebGPU overrides.
  for (const state of states) {
    const objectState = session.get(state.Id);
    if (objectState.ClassName.includes("OpenGL") && !objectState.ClassName.includes("WebGPU")) {
      throw new Error(`Object ${JSON.stringify(objectState)} should include WebGPU instead of OpenGL`);
    }
  }
}
const tests = [
  {
    description: "Test OpenGL overrides",
    test: testOpenGLOverrides,
  },
  {
    description: "Test WebGPU overrides",
    test: testWebGPUOverrides,
  }
];

let exitCode = 0;
for (let test of tests) {
  try {
    await test.test();
    console.log("✓", test.description);
    exitCode |= 0;
  }
  catch (error) {
    console.log("x", test.description);
    console.log(error);
    exitCode |= 1;
  }
}
process.exit(exitCode);
