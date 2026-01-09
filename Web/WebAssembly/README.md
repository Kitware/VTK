# VTK WebAssembly JavaScript

The `vtkWebAssembly.mjs` module provides JavaScript bindings to VTK WebAssembly using Emscripten. It includes two key classes:

- [`vtkStandaloneSession`](#vtkstandalonesession): For standalone VTK WebAssembly apps capable of creating and manipulating VTK objects.
- [`vtkRemoteSession`](#vtkremotesession): For remote VTK WebAssembly apps meant to sync with a VTK application on a remote server.

---

## Global Functions

| Function | Description |
|---------|-------------|
| `getVTKVersion()` | Returns the short VTK version string (e.g., `"9.4.2"` or `"9.4.20250508`). |
| `getVTKVersionFull()` | Returns the full VTK version string (e.g., `"9.4.2-2017-gd453d24731"`). |

---

## vtkStandaloneSession

Provides methods to directly create, manage, and manipulate VTK objects.

### Constructor
```js
new vtkStandaloneSession()
```

### Methods
| Signature                                                                      | Description                                         |
|--------------------------------------------------------------------------------|-----------------------------------------------------|
| `create(className: string) => number`                                          | Creates a new VTK object.                           |
| `destroy(objectId: number) => boolean`                                         | Destroys a previously created VTK object.           |
| `set(objectId: number, properties: object) => undefined`                       | Apply properties from JSON object to the VTK object.|
| `get(objectId: number) => object`                                              | Retrieves the state of the given object.            |
| `invoke(objectId: number, methodName: string, args: object) => object`         | Invokes a method on the object with JSON arguments. |
| `observe(objectId: number, eventName: string, callback: Function) => number`   | Registers a JavaScript callback for a VTK event.    |
| `unObserve(objectId: number, tag: number) => boolean`                          | Unregisters an observer using its tag.              |

## vtkRemoteSession

Wraps around VTK session C API for client-server WebAssembly rendering. This is more suitable when the rendering state is controlled remotely.

### Constructor
```js
new vtkRemoteSession()
```

### Methods
| Signature                                                                      | Description                                                |
|--------------------------------------------------------------------------------|------------------------------------------------------------|
| `registerState(state: object) => boolean`                                      | Registers a VTK object state.                              |
| `unRegisterState(objectId: number) => boolean`                                 | Unregisters an object’s state.                             |
| `set(objectId: number, properties: object) => undefined`                       | Apply properties from JSON object to the VTK object.       |
| `get(objectId: number) => object`                                              | Retrieves the state of the given object.                   |
| `skipProperty(className: string, propertyName: string) => void`                | Prevents a property from being (de)serialized.             |
| `unSkipProperty(className: string, propertyName: string) => void`              | Re-enables serialization for a property.                   |
| `registerBlob(hash: string, jsArray: Uint8Array) => boolean`                   | Registers a binary blob.                                   |
| `unRegisterBlob(hash: string) => boolean`                                      | Unregisters a blob using its hash.                         |
| `getBlob(hash: string) => Uint8Array`                                          | Retrieves a registered blob.                               |
| `invoke(objectId: number, methodName: string, args: object) => object`         | Calls a method on a VTK object.                            |
| `getAllDependencies(objectId: number) => Uint32Array`                          | Returns object dependency handles.                         |
| `updateObjectFromState(state: object) => void`                                 | Applies a JSON state to an object.                         |
| `updateStateFromObject(objectId: number) => void`                              | Updates the state JSON from the object.                    |
| `setSize(objectId: number, width: number, height: number) => boolean`          | Sets the size of a render window.                          |
| `render(objectId: number) => boolean`                                          | Triggers a render operation.                               |
| `resetCamera(objectId: number) => boolean`                                     | Resets the camera for the given scene.                     |
| `startEventLoop(objectId: number) => boolean`                                  | Begins the VTK interactor event loop.                      |
| `stopEventLoop(objectId: number) => boolean`                                   | Stops the VTK interactor event loop.                       |
| `bindRenderWindow(objectId: number, canvasSelector: string) => boolean`        | Binds a VTK render window to an HTML canvas element.       |
| `observe(objectId: number, eventName: string, callback: Function) => number`   | Observes events on an object.                              |
| `unObserve(objectId: number, tag: number) => boolean`                          | Removes an observer by tag.                                |
| `export(fileName: string) => void`                                             | Exports the state and blobs.                               |
| `import(stateFile: string, blobFile: string) => void`                          | Imports a scene and associated data.                       |
| `updateObjectsFromStates() => void`                                            | Applies all registered states to their respective objects. |
| `updateStatesFromObjects() => void`                                            | Refreshes all states based on their current object data.   |
| `pruneUnusedBlobs() => void`                                                   | Deletes unused blobs.                                      |
| `pruneUnusedObjects() => void`                                                 | Deletes unused objects.                                    |
| `pruneUnusedStates() => void`                                                  | Deletes unused states.                                     |
| `clear() => void`                                                              | Clears all objects, blobs, and state.                      |
| `getTotalBlobMemoryUsage() => number`                                          | Returns memory usage of all blobs.                         |
| `getTotalVTKDataObjectMemoryUsage() => number`                                 | Returns memory usage of all VTK data objects.              |
| `printSceneManagerInformation() => void`                                       | Prints internal scene manager details.                     |
| `setDeserializerLogVerbosity(level: string) => void`                           | Sets verbosity for deserialization.                        |
| `setInvokerLogVerbosity(level: string) => void`                                | Sets verbosity for method invocations.                     |
| `setObjectManagerLogVerbosity(level: string) => void`                          | Sets verbosity for the object manager.                     |
| `setSerializerLogVerbosity(level: string) => void`                             | Sets verbosity for serialization.                          |

## Notes

- All `objectId` are an unsigned integer.

- All states must be passed as JSON and will be returned as JSON. Do not pass stringified JSON.

- Blob support is useful for caching binary data such as images or meshes to ease network traffic in the remote session use case.
