var vtkWebAssemblyWebGPURenderWindowInitDevice = async () => {
  if (typeof navigator.gpu === 'undefined') {
    throw Error("Browser does not support WebGPU!")
  }
  const adapter = await navigator.gpu.requestAdapter();
  if (adapter === null) {
    throw Error("Browser does not support WebGPU!")
  }
  const requiredLimits = {};
  // App ideally needs as much GPU memory it can get.
  // when adapter limits are greater than default, copy adapter limits to device requirements.
  // maxBufferSize
  if (adapter.limits.maxBufferSize > 268435456) {
    requiredLimits.maxBufferSize = adapter.limits.maxBufferSize;
  }
  // maxStorageBufferBindingSize
  if (adapter.limits.maxStorageBufferBindingSize > 134217728) {
    requiredLimits.maxStorageBufferBindingSize = adapter.limits.maxStorageBufferBindingSize;
  }
  // maxUniformBufferBindingSize
  if (adapter.limits.maxUniformBufferBindingSize > 65536) {
    requiredLimits.maxUniformBufferBindingSize = adapter.limits.maxUniformBufferBindingSize;
  }
  return adapter.requestDevice(requiredLimits);
};

Module["preinitializedWebGPUDevice"] = await vtkWebAssemblyWebGPURenderWindowInitDevice();
