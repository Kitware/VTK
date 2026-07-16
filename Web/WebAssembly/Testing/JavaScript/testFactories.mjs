function makeStates(prefix) {
  return [{
    Id: 1,
    ClassName: `vtk${prefix}RenderWindow`,
    Interactor: { Id: 2 },
    Renderers: { Id: 3 },
    Size: [800, 600],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkWindow",
      "vtkRenderWindow",
    ],
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
      "vtkCollection",
    ],
  },
  {
    Id: 4,
    ClassName: `vtk${prefix}Renderer`,
    ViewProps: { Id: 5 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkViewport",
      "vtkRenderer",
    ],
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
      "vtkCollection",
    ],
  },
  {
    Id: 6,
    ClassName: `vtk${prefix}Actor`,
    Mapper: { Id: 7 },
    Property: { Id: 8 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProp",
      "vtkActor",
    ],
  },
  {
    Id: 7,
    ClassName: `vtkPolyDataMapper`,
    InputDataObjects: [[{ Id: 9 }]],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkAlgorithm",
      "vtkAbstractMapper",
      "vtkAbstractMapper3D",
      "vtkMapper",
    ],
    UseLookupTableScalarRange: 0
  },
  {
    Id: 8,
    ClassName: `vtkProperty`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
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
    ClassName: `vtkActor`,
    Mapper: { Id: 11 },
    Property: { Id: 12 },
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkProp",
    ],
  },
  {
    Id: 11,
    ClassName: `vtkGlyph3DMapper`,
    InputDataObjects: [[{ Id: 13 }], [{ Id: 13 }]],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkAlgorithm",
      "vtkAbstractMapper",
      "vtkAbstractMapper3D",
      "vtkMapper",
    ],
    UseLookupTableScalarRange: 0
  },
  {
    Id: 12,
    ClassName: `vtkProperty`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
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
      "vtkProp",
    ],
  },
  {
    Id: 15,
    ClassName: `vtkPolyDataMapper2D`,
    InputDataObjects: [[{ Id: 17 }]],
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
      "vtkAlgorithm",
      "vtkAbstractMapper",
      "vtkMapper2D",
    ],
    UseLookupTableScalarRange: 0
  },
  {
    Id: 16,
    ClassName: `vtkProperty`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
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
      "vtkProp",
    ],
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
    ClassName: `vtkProperty`,
    SuperClassNames: [
      "vtkObjectBase",
      "vtkObject",
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

/**
 * Deserialize a scene whose states use `statePrefix` backend specific class names
 * (e.g. "OpenGL", "WebGPU" or "" for abstract base class names) while the object factory
 * is configured to prefer the `preferBackend` rendering backend. Every object that ends up
 * with a backend specific class name is expected to match `preferBackend` regardless of the
 * backend that was used to author the states.
 */
async function testOverrides({ preferBackend, statePrefix }) {
  const vtkWASM = await globalThis.createVTKWASM({
    preRun: [function (module) {
      /// select the rendering backend the object factory should prefer
      module.ENV.VTK_FACTORY_PREFER = `RenderingBackend=${preferBackend};Platform=WebAssembly`
      /// enable logging for debugging purposes (optional)
      // module.ENV.VTK_DESERIALIZER_LOG_VERBOSITY = "INFO";
      // module.ENV.VTK_INVOKER_LOG_VERBOSITY = "INFO";
      // module.ENV.VTK_OBJECT_MANAGER_LOG_VERBOSITY = "INFO";
    }],
  });
  const session = new vtkWASM.vtkRemoteSession();
  const states = makeStates(statePrefix);
  for (const state of states) {
    session.registerState(state);
  }
  session.updateObjectsFromStates();
  // The created objects must match the preferred backend, never the other one.
  const otherBackend = preferBackend === "OpenGL" ? "WebGPU" : "OpenGL";
  for (const state of states) {
    const objectState = session.get(state.Id);
    if (objectState.ClassName.includes(otherBackend) &&
      !objectState.ClassName.includes(preferBackend)) {
      throw new Error(
        `ClassName ${objectState.ClassName} should include ${preferBackend} instead of ${otherBackend}`);
    }
  }
}

const tests = [
  {
    // 1. abstract class names + OpenGL backend -> OpenGL classes
    description: "Test abstract class names with OpenGL backend",
    test: () => testOverrides({ preferBackend: "OpenGL", statePrefix: "" }),
  },
  {
    // 2. abstract class names + WebGPU backend -> WebGPU classes
    description: "Test abstract class names with WebGPU backend",
    test: () => testOverrides({ preferBackend: "WebGPU", statePrefix: "" }),
  },
  {
    // 3. OpenGL class names in state + WebGPU backend -> WebGPU classes
    description: "Test OpenGL states with WebGPU backend",
    test: () => testOverrides({ preferBackend: "WebGPU", statePrefix: "OpenGL" }),
  },
  {
    // 4. WebGPU class names in state + OpenGL backend -> OpenGL classes
    description: "Test WebGPU states with OpenGL backend",
    test: () => testOverrides({ preferBackend: "OpenGL", statePrefix: "WebGPU" }),
  },
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
