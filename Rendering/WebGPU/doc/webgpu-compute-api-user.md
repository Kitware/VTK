### Description

The new WebGPU compute API allows offloading computations from the CPU to the GPU using WebGPU compute shaders
through the new `vtkWebGPUComputePipeline` class.

The compute API is used through a `vtkWebGPUComputePipeline` object. It is used to set the inputs and outputs to
the compute shader, execute it and get the results back to the CPU.

### Compute pipeline - Usage outside a rendering pipeline

The basic usage of a pipeline outside a rendering pipeline goes as follows:
 - Create a pipeline
 - Set its shader source code
 - Set its shader entry point
 - Create the vtkWebGPUComputeBuffers that contain the data manipulated by the compute shader
 - Add the buffers to the pipeline
 - Set the number of workgroups
 - Dispatch the compute shader
 - Update()
 - ReadBufferFromGPU() to make results from the GPU available to the CPU

Let's now review these steps in more details:

After creating a `vtkWebGPUComputePipeline` object, you must first indicate the source code of the compute shader
to use. This is done through the `SetShaderSource()` method. This method expects raw WGSL shader code. On top
of setting the shader source code, the entry point (function name) of the compute shader must also be given via
`SetShaderEntryPoint()`.

The next (although the order of these pipeline set-up operations does not matter) step is to set the input
and output buffers that will be used by the compute shader. This is done through the `AddBuffer()` method.
This method expect a `vtkWebGPUComputeBuffer` object as input. A `vtkWebGPUComputeBuffer` object represents a set of parameters
that will be used to create the buffer on the GPU and upload data to it.

To be valid, all `vtkWebGPUComputeBuffer` objects need to be given a group (via `SetGroup()`), a binding (via `SetBinding()`)
and a mode (modes are described by `vtkWebGPUComputeBuffer::BufferMode`) using `SetMode()`.
All groups/bindings combination must be unique as these two values define the "location" of your buffer in the shader.

You also need to indicate what data to use for input buffers using `SetData()`.
`SetData()` accepts both `vtkDataArray` and `std::vector` data containers.
Note that the data isn't uploaded (copied to the GPU) until the buffer is actually added to the pipeline by `vtkWebGPUComputePipeline::AddBuffer()`
so the data needs to stay valid (i.e. not destroyed) on the CPU until `AddBuffer()` is called.
Because you can give either `std::vector` data or `vtkDataArray` data to the buffer but only one of two is going to be used when
sending data to the GPU, you must precise which one to use by calling `vtkWebGPUComputeBuffer::SetBufferDataType()`.

For buffers used as outputs of the compute shader, you only need to specify their size using `SetByteSize()`.
The size of input buffers is automatically determined when `SetData()` is called. `SetByteSize()` should not be called on input buffers.

Once a buffer is set up, it can be added to the pipeline using `AddBuffer()`. This uploads the data to the GPU and the
CPU-side data can be safely be destroyed.

After the shader source and its entry point as well as the buffers have been set on the pipeline, one more step needs to be
taken care of before you can dispatch the compute shader: setting the number of workgroups using `SetWorkgroups(X, Y, Z)`. The number of
workgroups you need to set depends on the size of your data and the `@compute @workgroup_size(WX, WY, WZ)` annotation in your shader. As a simple
rule, the result `X * Y * Z * WX * WY * WZ` must be at least as big as the length of your input data. You can have a look at the compute pipeline tests
under `Rendering/WebGPU/Testing` for some more details.

You can then `Dispatch()` the compute pipeline. Note that `Dispatch()` does not actually execute the compute shader, the `Update()` method does
(see below).

The next step is get the data back from the GPU to the CPU. Because the CPU cannot directly read data that is on the GPU, you need to
copy the data from the GPU back to the CPU using the `ReadBufferFromGPU()` method.
This method takes a buffer index as its first argument (returned by the `AddBuffer()` method). This is the index of the buffer that is
going to be sent to the CPU.
The second argument is a function that will be called after the buffer has been successfully sent from the GPU.
This is typically the function that will copy the data into a CPU-side buffer. That buffer can be passed through the third argument
which is a pointer to some data that you want accessible from the callback (second argument). This third argument can also be
a structure if you need more than one argument to be available in the callback.

The very last step is simple: you need to call the `Update()` method to actually execute the GPU commands (queued by the `Dispatch()` and `ReadBufferFromGPU()`
calls).

Here's an example of how this all can look like in practice:

```c++
  std::vector<float> inputValues;
  // ...
  // Fill the input vector with data
  // ...

  // Creating the input buffer to the compute shader
  vtkNew<vtkWebGPUComputeBuffer> inputBuffer;
  inputBuffer->SetGroup(0);
  inputBuffer->SetBinding(0);
  inputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::INPUT_COMPUTE_STORAGE);
  inputBuffer->SetData(inputValues);
  inputBuffer->SetBufferDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  // Creating the output buffer of the compute shader
  vtkNew<vtkWebGPUComputeBuffer> outputBuffer;
  outputBuffer->SetGroup(0);
  outputBuffer->SetBinding(1);
  outputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::OUTPUT_COMPUTE_STORAGE);
  outputBuffer->SetByteSize(inputValues.size() * sizeof(OutputDataType));

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> computePipeline;
  computePipeline->SetShaderSource(computeShaderSource);
  computePipeline->SetShaderEntryPoint("computeMainFunction");
  computePipeline->AddBuffer(inputBuffer);
  // Getting the index of the output buffer for later mapping with ReadBufferFromGPU()
  int outputBufferIndex = computePipeline->AddBuffer(outputBuffer);

  computePipeline->SetWorkgroups(workgroupsX, workgroupsY, workgroupsZ);

  // We've set up everything, ready to dispatch
  computePipeline->Dispatch();

  std::vector<OutputDataType> outputData(outputBufferSize);
  auto onBufferMapped = [](const void* mappedData, void* userdata)
  {
    std::vector<OutputDataType>* outputDataVector = reinterpret_cast<std::vector<OutputDataType>*>(userdata);
    vtkIdType elementCount = outputDataVector->Size();

    const OutputDataType* mappedData = static_cast<const OutputDataType*>(mappedData);
    for (int i = 0; i < elementCount; i++)
    {
      (*outputDataVector)[i] = mappedData[i];
    }
  };

  // Mapping the buffer on the CPU to get the results from the GPU
  computePipeline->ReadBufferFromGPU(outputBufferIndex, onBufferMapped, &outputData);
  // Update() to actually execute WebGPU commands. Without this, the compute shader won't execute and
  // the data that we try to map here may not be available yet
  computePipeline->Update();

  // ... Do something with the output data here
```

Note also that `Update()` does not need to be called after every call to the compute pipeline. Calling it once
at the end is valid.

The `Update()` executes commands on the GPU in the order they were added.

This means that:

```c++
computePipeline->ReadBufferFromGPU(outputBufferIndex, /* callback parameters */);
computePipeline->Dispatch();
computePipeline->Update();
```

is not going to produce the expected results as the buffer would be mapped (and read by the CPU)
before the compute shader executed.

### Uniforms

Uniforms are simply treated as `vtkWebGPUComputeBuffer` with the `vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER` mode.

Example:

```c++
const char* shaderSource = R"(
@group(0) @binding(0) var<storage, read> inputBuffer: array<i32, 128>;
@group(0) @binding(1) var<uniform> myUniform: f32;

@compute @workgroup_size(32, 1, 1)
fn computeFunction(@builtin(global_invocation_id) id: vec3<u32>)
{
  // ...
  // Do something
  // ...
})";

int main()
{
  // Ipnut data vector
  std::vector<int> inputVector1Values(128);
  // ...
  // Fill the data
  // ...

  // Creating the input buffer to the compute shader
  vtkNew<vtkWebGPUComputeBuffer> inputBuffer;
  inputBuffer->SetGroup(0);
  inputBuffer->SetBinding(0);
  inputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputBuffer->SetData(inputVector1Values);
  inputBuffer->SetLabel("First input buffer");
  inputBuffer->SetBufferDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  // Creating a buffer for the additional uniform
  float myUniform = 2.5f;
  std::vector<float> uniformData = { myUniform };
  vtkNew<vtkWebGPUComputeBuffer> uniformBuffer;
  uniformBuffer->SetGroup(0);
  uniformBuffer->SetBinding(1);
  uniformBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  uniformBuffer->SetData(uniformData);
  uniformBuffer->SetLabel("Uniform buffer");
  uniformBuffer->SetBufferDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
}
```

Because uniforms are buffers, you could also have 'myUniform' be an array in the shader and upload
more than one float when calling `SetData()`.

### Compute pipeline - Integration in an existing rendering pipeline

A compute pipeline can also be used to access and modify data buffers used for rendering
(point/cell data attributes: colors, positions, normals, ...).

The following documentation only discusses point data attributes but everything applies for cell data attributes
as well (using `vtkWebGPUPolyDataMapper::AcquireCellAttributeComputeRenderBuffer()`).

The global usage is the same as when using the pipeline outside of the rendering pipeline, the main difference
being that you do not create the attribute buffers yourself (since you're using the existing buffers from the
render pipeline). You can retrieve the buffers by calling `vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer()`:

```c++
int bufferGroup = 0, bufferBinding = 0;
int uniformsGroup = 0, uniformsBinding = 1;

vtkSmartPointer<vtkWebGPUComputeRenderBuffer> pointColorsRenderBuffer =
  webGPUMapper->AcquirePointAttributeComputeRenderBuffer(vtkWebGPUPolyDataMapper::PointDataAttributes::COLORS,
  bufferGroup, bufferBinding, uniformsGroup, uniformsBinding);
```

All the points attributes data (positions, colors, normals, tangents, UVs, ...) are actually contained within
the returned buffer. To identify which part of the buffer is relevant, you ask for a specific attribute using the first
parameter of `vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer()`.

The next 2 parameters indicate where the buffer is going to be bound in your compute shader, same as when creating a buffer
that does not come from a rendering pipeline.

Because the buffer you get from `vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer()` contains all the point data of
the mapper you called the function on (not only the attribute you requested), you are going to need a way to identify where
the part of the buffer that you requested (positions, colors, ... depending on the first parameter of the function) begins and ends.

This is made available through the last 2 parameters of the function. The compute pipeline will automatically bind a
uniform buffer of two u32 values to your shader. These u32 values are, in order:
- The buffer offset, corresponding to the location where the attributes you requested start in the whole buffer, expressed in **number of float elements**.
- The buffer length, in terms of **number of attributes**. For example, if you requested colors and the buffer length uniform is 2, then you
have 8 floats of relevant data (a color value corresponding to 4 floats).

These two values are bound in your shader as a uniform buffer at the group and binding given as the third and fourth parameters.

The returned render buffer can then be added to the pipeline by calling `AddRenderBuffer()` (not `AddBuffer()`,
since we're manipulating a buffer used by the rendering pipeline here):

```c++
myComputePipeline->AddRenderBuffer(pointColorsRenderBuffer);
```

The compute pipeline then needs to be added to a `vtkWebGPURenderer`:

```c++
vtkWebGPURenderer* wegpuRenderer = vtkWebGPURenderer::SafeDownCast(renWin->GetRenderers()->GetFirstRenderer());
wegpuRenderer->AddComputePipeline(myComputePipeline);
```

And that's it. You do not need to call the `Update()` method for the compute pipeline to be executed.
This is done automatically by the rendering pipeline on each frame.
