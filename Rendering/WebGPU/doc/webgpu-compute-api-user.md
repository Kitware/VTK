# Description

The new WebGPU compute API allows offloading computations from the CPU to the GPU using WebGPU compute shaders
through the new `vtkWebGPUComputePass` and `vtkWebGPUComputePipeline` classes.

A `vtkWebGPUComputePass` is used to set the inputs and outputs to the compute shader, execute it and get the
results back to the CPU. A `vtkWebGPUComputePipeline` contains one or more `vtkWebGPUComputePass` that can be
executed one after the other in a single `vtkWebGPUComputePipeline->Dispatch()` call.

# Compute pass & pipeline - Usage outside a rendering pipeline

The basic usage of a compute pass outside a rendering pipeline is:
 - Create a vtkWebGPUComputePipeline
 - Obtain a compute pass from this compute pipeline
 - Set its shader source code
 - Set its shader entry point
 - Create the vtkWebGPUComputeBuffers that contain the data manipulated by the compute pass
 - Add the buffers to the compute pass
 - Set the number of workgroups
 - Dispatch the compute pass
 - ReadBufferFromGPU() to make results from the GPU available to the CPU
 - Update() the pipeline so that the compute pass is executed

Let's now review these steps in more details:

A `vtkWebGPUComputePipeline` cannot really do much on its own. The hard work is done by the `vtkWebGPUComputePass`.
To obtain a compute pass, you simply have to call `vtkWebGPUComputePipeline->CreateComputePass()`. This returns a
`vtkWebGPUComputePass` object that you then need to configure.

You must first indicate the source code of the compute shader to use. This is done through the `SetShaderSource()` method.
This method expects raw WGSL shader code. On top of setting the shader source code, you must also give the entry point
(function name) of the compute shader via `SetShaderEntryPoint()`.

The next (although the order of these compute pass set-up operations does not matter) step is to set the input
and output buffers that will be used by the compute shader. This is done through the `AddBuffer()` method.
This method expect a `vtkWebGPUComputeBuffer` object as input. A `vtkWebGPUComputeBuffer` object represents a set of parameters
that will be used to create the buffer on the GPU and upload data to it.

To be valid, all `vtkWebGPUComputeBuffer` objects need to be given a group (via `SetGroup()`), a binding (via `SetBinding()`)
and a mode (modes are described by `vtkWebGPUComputeBuffer::BufferMode`) using `SetMode()`.
All groups/bindings combination must be unique as these two values define the "location" of your buffer in the shader.

You also need to indicate what data to use for input buffers using `SetData()`.
`SetData()` accepts both `vtkDataArray` and `std::vector` data containers.
Note that the data isn't uploaded (copied to the GPU) until the buffer is actually added to the compute pass
by `vtkWebGPUComputePass::AddBuffer()` so the data needs to stay valid (i.e. not destroyed) on the CPU until `AddBuffer()` is called.
Because you can give either `std::vector` data or `vtkDataArray` data to the buffer but only one of two is going to be used when
sending data to the GPU, you must specify which one to use by calling `vtkWebGPUComputeBuffer::SetDataType()`.

For buffers used as outputs of the compute shader, you only need to specify their size using `SetByteSize()`.
The size of input buffers is automatically determined when `SetData()` is called. `SetByteSize()` should not be called on input buffers.

Once a buffer is set up, it can be added to the pass using `AddBuffer()`. This uploads the data to the GPU and the
CPU-side data can be safely be destroyed.

After the shader source and its entry point as well as the buffers have been set on the pipeline, one more step needs to be
taken care of before you can dispatch the compute shader: setting the number of workgroups using `SetWorkgroups(X, Y, Z)`. The number of
workgroups you need to set depends on the size of your data and the `@compute @workgroup_size(WX, WY, WZ)` annotation in your shader. As a simple
rule, the result `X * Y * Z * WX * WY * WZ` must be at least as big as the length of your input data. You can have a look at the
WebGPU compute API tests under `Rendering/WebGPU/Testing` for some more details.

Once all of that setup has been done, you can then `Dispatch()` the compute pass. Note that `Dispatch()` does not actually
execute the compute shader, the `Update()` method of the `vtkWebGPUComputePipeline` does (see below).

The next step is get the data back from the GPU to the CPU. Because the CPU cannot directly read data that is on the GPU, you need to
copy the data from the GPU back to the CPU using the `ReadBufferFromGPU()` method.
This method takes a buffer index as its first argument (returned by the `AddBuffer()` method). This is the index of the buffer that is
going to be copied to the CPU.
The second argument is a function that will be called after the buffer has been successfully copied from the GPU.
This is typically the function that will copy the data into a CPU-side buffer (a `std::vector` or equivalent for example). That buffer
can be passed through the third argument which is a pointer to some data that you want accessible from the callback (which is the second argument).
This third argument can also be a structure if you need more than one argument to be available in the callback.

The very last step is simple: you need to call the `Update()` method of the `vtkWebGPUComputePipeline` to actually execute the GPU
commands (queued by the `Dispatch()` and `ReadBufferFromGPU()` calls).

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
  inputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputBuffer->SetData(inputValues);
  inputBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  // Creating the output buffer of the compute shader
  vtkNew<vtkWebGPUComputeBuffer> outputBuffer;
  outputBuffer->SetGroup(0);
  outputBuffer->SetBinding(1);
  outputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputBuffer->SetByteSize(inputValues.size() * sizeof(float));

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> computePipeline;
  vtkSmartPointer<vtkWebGPUComputePass> computePass = computePipeline->CreateComputePass();
  computePass->SetShaderSource(computeShaderSource);
  computePass->SetShaderEntryPoint("computeMainFunction");
  computePass->AddBuffer(inputBuffer);
  // Getting the index of the output buffer for later mapping with ReadBufferFromGPU()
  int outputBufferIndex = computePipeline->AddBuffer(outputBuffer);

  computePass->SetWorkgroups(workgroupsX, workgroupsY, workgroupsZ);

  // We've set up everything, ready to dispatch
  computePass->Dispatch();

  // Vector that will contain the results of the compute shader
  std::vector<float> outputData(outputBufferSize);

  // Function called by ReadBufferFromGPU() whose purpose is to copy
  // the data into your result buffer (outputData here).
  auto onBufferMapped = [](const void* mappedData, void* userdata)
  {
    std::vector<float>* outputDataVector = reinterpret_cast<std::vector<float>*>(userdata);

    // You must know in advance how many elements you're going to read from the mappedData.
    // We're using the size of outputDataVector here because we resized it to just the right
    // size so that's just the right number of elements. If you don't want or can't use a
    // std::vector for that purpose, you can still pass the number of elements as the third
    // argument of ReadBufferFromGPU().
    vtkIdType elementCount = outputDataVector->size();

    const float* mappedDataAsF32 = static_cast<const float*>(mappedData);
    for (int i = 0; i < elementCount; i++)
    {
      (*outputDataVector)[i] = mappedDataAsF32[i];
    }
  };

  // Mapping the buffer on the CPU to get the results from the GPU
  computePass->ReadBufferFromGPU(outputBufferIndex, onBufferMapped, &outputData);
  // Update() to actually execute WebGPU commands. Without this, the compute shader won't execute.
  // Update() is called on the compute pipeline and not the compute pass because it is the
  // compute pipeline that orchestrates the execution of all compute passes
  computePipeline->Update();

  // ... Do something with the output data
```

Note also that `Update()` does not need to be called after every call to the compute pipeline. Calling it once
at the end is valid.

The `Update()` method executes commands on the GPU in the order they were added.

This means that:

```c++
computePass->ReadBufferFromGPU(outputBufferIndex, /* callback parameters */);
computePass->Dispatch();
computePipeline->Update();
```

is not going to produce the expected results as the buffer would be mapped (and read by the CPU)
before the compute shader executed.

# Chained compute passes

Thanks to the concept of compute pipeline, it is possible to chain compute passes so that they execute
one after the other, potentially using the output of one `vtkWebGPUComputePass` as the input to another pass.

Following is an example of two compute passes chained together that operate on a vectors of numbers. The first compute
pass multiplies two vectors together while the second compute pass adds the result of the first pass with a new (third)
vector. The output of the second pass is then read back to the CPU.

```c++
  const int DATA_SIZE = 1000;

  const char* secondPassShader = R"(
  @group(0) @binding(0) var<storage, read_write> inputOutput: array<f32>;

  @compute @workgroup_size(32, 1, 1)
  fn computeFunctionAdd(@builtin(global_invocation_id) id: vec3<u32>)
  {
      if (id.x >= arrayLength(&inputOutput))
      {
          return;
      }

      inputOutput[id.x] = inputOutput[id.x] + 42.0f;
  })";

  const char* firstPassShader = R"(
  @group(0) @binding(0) var<storage, read> inputVector1: array<i32>;
  @group(0) @binding(1) var<storage, read> inputVector2: array<f32>;
  @group(0) @binding(2) var<storage, read_write> outputData: array<f32>;

  @compute @workgroup_size(32, 1, 1)
  fn computeFunction(@builtin(global_invocation_id) id: vec3<u32>)
  {
      if (id.x >= arrayLength(&inputVector1))
      {
          return;
      }

      outputData[id.x] = f32(inputVector1[id.x]) * inputVector2[id.x];
  })";

  // This first vector will be using a vtkDataArray as its data source
  vtkNew<vtkIntArray> inputDataArray;
  // Fill inputDataArray ...

  // The second input to the compute pass uses a std::vector.
  std::vector<float> inputVector2Values(DATA_SIZE);
  // Fill inputVector2Values...

  // Creating the input buffer to the compute pass
  vtkNew<vtkWebGPUComputeBuffer> inputValues1Buffer;
  inputValues1Buffer->SetLabel("First input buffer");
  inputValues1Buffer->SetGroup(0);
  inputValues1Buffer->SetBinding(0);
  inputValues1Buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputValues1Buffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);
  inputValues1Buffer->SetData(inputDataArray);

  // Creating the second input buffer for the compute pass
  vtkNew<vtkWebGPUComputeBuffer> inputValues2Buffer;
  inputValues2Buffer->SetLabel("Second input buffer");
  inputValues2Buffer->SetGroup(0);
  inputValues2Buffer->SetBinding(1);
  inputValues2Buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputValues2Buffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  inputValues2Buffer->SetData(inputVector2Values);

  // Creating the output buffer to the compute pass
  vtkNew<vtkWebGPUComputeBuffer> outputBuffer;
  outputBuffer->SetLabel("Output buffer");
  outputBuffer->SetGroup(0);
  outputBuffer->SetBinding(2);
  outputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputBuffer->SetByteSize(DATA_SIZE * sizeof(float));

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> computePipeline;
  // Creating the compute pass
  vtkSmartPointer<vtkWebGPUComputePass> multiplyComputePass = computePipeline->CreateComputePass();

  // Configuring the compute pass
  multiplyComputePass->SetShaderSource(firstPassShader);
  multiplyComputePass->SetShaderEntryPoint("computeFunction");
  multiplyComputePass->AddBuffer(inputValues1Buffer);
  multiplyComputePass->AddBuffer(inputValues2Buffer);
  multiplyComputePass->AddBuffer(outputBuffer);

  vtkSmartPointer<vtkWebGPUComputePass> addComputePass = computePipeline->CreateComputePass();
  addComputePass->SetShaderSource(secondPassShader);
  addComputePass->SetShaderEntryPoint("computeFunctionAdd");
  // Setting the output buffer (of the first compute pass) on the (0, 0) binding point of
  // the second pass
  outputBuffer->SetGroup(0);
  outputBuffer->SetBinding(0);
  // Using the output buffer of the first as the input and output (we're going to read and write
  // from it) of the second pass. We're also storing its index because we're going to need the
  // index when reading the buffer from the GPU back to the CPU
  int outputBufferIndex = addComputePass->AddBuffer(outputBuffer);

  // Dispatching the compute with
  int nbXGroups = std::ceil(DATA_SIZE / 32.0f);
  multiplyComputePass->SetWorkgroups(nbXGroups, 1, 1);
  multiplyComputePass->Dispatch();

  addComputePass->SetWorkgroups(nbXGroups, 1, 1);
  addComputePass->Dispatch();

  // Output buffer for the result data
  std::vector<float> outputData;
  outputData.resize(DATA_SIZE);

  auto onBufferMapped = [](const void* mappedData, void* userdata) {
    std::vector<float>* out = reinterpret_cast<std::vector<float>*>(userdata);
    vtkIdType elementCount = out->size();

    const float* mappedDataTyped = static_cast<const float*>(mappedData);
    for (int i = 0; i < elementCount; i++)
    {
      (*out)[i] = mappedDataTyped[i];
    }
  };

  // Mapping the buffer on the CPU to get the results from the GPU
  addComputePass->ReadBufferFromGPU(outputBufferIndex, onBufferMapped, &outputData);
  // Update() the compute pipeline to execute commands in the order they were emitted:
  // 1) multiplyComputePass->Dispatch()
  // 2) addComputePass->Dispatch()
  // 3) addComputePass->ReadBufferFromGPU()
  computePipeline->Update();
```

# Uniforms

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
  inputBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  // Creating a buffer for the additional uniform
  float myUniform = 2.5f;
  std::vector<float> uniformData = { myUniform };
  vtkNew<vtkWebGPUComputeBuffer> uniformBuffer;
  uniformBuffer->SetGroup(0);
  uniformBuffer->SetBinding(1);
  uniformBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  uniformBuffer->SetData(uniformData);
  uniformBuffer->SetLabel("Uniform buffer");
  uniformBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  // .... Add the uniformBuffer to a compute pass as usual
}
```

Because uniforms are buffers, you could also have 'myUniform' be an array in the shader and upload
more than one float when calling `SetData()`.

# Textures

Besides buffer, compute passes can also use textures and texture views. Although the concept
of texture may be self explanatory, texture views on the other hand may not. A texture view
gives access to a certain part of a texture with certain parameters.

For example, a texture view can only refer to a given subregion of the texture instead of
the whole texture. The shader won't see the difference and will use it as if it was a whole texture.
Texture views can also access textures in read and/or write mode. This could be useful in a compute
shader that computes the mipmaps of a given texture where you would one texture view for reading
the mip level N of the texture and another texture view for writing to level N-1 of the same texture.

Crucially, this can all be done without having to duplicate the texture. Texture views are very lightweight.
Also note that texture views are not just some kind of convenient objects, they are actually needed. The
shader samples texture views, not textures directly.

In code, textures and texture views are easy to use:

```C++
// First creating the texture
vtkNew<vtkWebGPUComputeTexture> myTexture;

// Configuring the texture
myTexture->SetDimension(vtkWebGPUComputeTexture::TextureDimension::DIMENSION_2D)
myTexture->SetFormat(vtkWebGPUComputeTexture::TextureFormat::RGBA8_UNORM);
myTexture->SetMode(vtkWebGPUComputeTexture::TextureMode::READ_WRITE_STORAGE);
myTexture->SetSampleType(vtkWebGPUComputeTexture::TextureSampleType::FLOAT);
myTexture->SetSize(1280, 720);
myTexture->SetMipLevelCount(1);

int textureIndex = myComputePass->AddTexture(myTexture);

vtkSmartPointer<vtkWebGPUComputeTextureView> textureView;
textureView = myComputePasss->CreateTextureView(textureIndex);
textureView->SetBaseMipLevel(0);
textureView->SetMode(vtkWebGPUComputeTextureView::TextureViewMode::WRITE_ONLY_STORAGE);
// The shader interacts with texture views, not textures so the binding/group is
// on the texture view, not the texture
textureView->SetGroup(0);
textureView->SetBinding(0);

myComputePass->AddTextureView(textureView);
```

Some comments on the parameters used during the configuration of the texture & texture view:
- Dimension: Whether the texture is 1D, 2D or 3D. Typical textures are 2D.
- Format: Internal format of the texture for storing the texels. HDR textures (but not limited
  to) would use the float format whereas a simple texture could probably get away with the
  RGBA 8-bit per channel format.
- Mode: Read/write/read-write/mappable etc... This works the same way as buffers
- Sample type: This parameter defines what is going to be returned when sampling the texture in
  a shader. In such a situation, the aspect allows to choose whether you're only interested
  in sampling the depth part of the depth + stencil texture or only the stencil part.
  For typical color textures, `ASPECT_ALL` is the one to use and is the default.
- Aspect: This is useful for textures that contains multiple type of information such as a depth + stencil texture for example.

Note that all these parameters have reasonable defaults so you may not have to set them all
for each one of your textures.

# Compute pass & pipeline - Integration in an existing rendering pipeline

A compute pass can also be used to access and modify data buffers used for rendering
(point/cell data attributes: colors, positions, normals, ...).

The following documentation only discusses point data attributes but everything applies for cell data attributes
as well (using `vtkWebGPUPolyDataMapper::AcquireCellAttributeComputeRenderBuffer()`).

The global usage is the same as when using the compute pass and pipeline outside of the rendering pipeline, the main difference
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
the returned buffer. `AcquirePointAttributeComputeRenderBuffer()` does not return a buffer that contains only what
you requested. To identify which part of the buffer is relevant, you ask for a specific attribute using the first
parameter of `vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer()`.

The next 2 parameters indicate where the buffer is going to be bound in your compute shader, same as when creating a buffer
that does not come from a rendering pipeline.

Because the buffer you get from `vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer()` contains all the point data of
the mapper you called the function on (not only the attribute you requested), you are going to need a way to identify where
the part of the buffer that you requested (positions, colors, ... depending on the first parameter of the function) begins and ends.

This is made available through the last 2 parameters of the function. The compute pipeline will automatically bind a
uniform buffer of two u32 values to your shader. These u32 values are, in order:
- The buffer offset, corresponding to the location where the attributes you requested start in the whole buffer, expressed in **number of float elements**.
- The buffer length, in **number of attributes**. For example, if you requested colors and the buffer length uniform is 2, then you
have 8 floats of relevant data (a color value corresponding to 4 floats).

These two values are bound in your shader as a uniform buffer at the group and binding given as the third and fourth parameters.

The returned render buffer can then be added to the compute pass by calling `AddRenderBuffer()` (not `AddBuffer()`,
since we're manipulating a buffer used by the rendering pipeline here):

```c++
vtkSmartPointer<vtkWebGPUComputePass> myComputePass = myComputePipeline->CreateComputePass()
myComputePass->AddRenderBuffer(pointColorsRenderBuffer);
```

The compute pipeline (to which the compute pass belongs) then needs to be added to a `vtkWebGPURenderer`.
You can add the `vtkWebGPUComputePipeline` to the renderer so that it executes before rendering a frame or
after rendering a frame, depending on yours needs. To give examples, if you intend on modifying the geometry of some actors
through your compute pipeline, you probably want to add the compute pipeline to the renderer so that it executes
before the rendering process, allowing for the changes to the geometry to be reflected once the frame is rendered.
On the other hand, if you need the depth buffer of the render window for your compute pipeline, you will want to add
the compute pipeline so that it executes after the frame is rendered (so that the depth buffer has been constructed).

You can decide that through the `AddPreRenderComputePipeline()` and `AddPostRenderComputePipeline()` functions.

```c++
vtkWebGPURenderer* wegpuRenderer = vtkWebGPURenderer::SafeDownCast(renWin->GetRenderers()->GetFirstRenderer());

// The compute pipeline will execute **before** rendering a frame.
// Using AddPostRenderComputePipeline() will have the pipeline execute after
// a frame is rendered
wegpuRenderer->AddPreRenderComputePipeline(myComputePipeline);
```

Warning: to add a compute pipeline to a `vtkWebGPURenderer`, the `vtkRenderWindow` of the renderer must be "initialized". This
means that you must call `renWin->Initialize()` before adding the compute pipeline to the renderer. This is because
the `Initialize()` method creates the WebGPU internal state of the render window which the compute pipeline of the renderer
is going to need.

And that's it. You do not need to call the `Update()` method for the compute pipeline to be executed.
This is done automatically by the rendering pipeline on each frame.

`vtkWebGPURenderWindow` also exposes methods to access render textures used by the rendering pipeline such as the depth buffer
(only texture exposed for now). These textures are accessible through `vtkWebGPUComputeRenderTexture`s (analogous to `vtkWebGPUComputeRenderBuffer`s).
They can be acquired with methods such as `AcquireDepthBufferRenderTexture()`. Here again, you will need to call `vtkRenderWindow::Initialize()` before
calling `AcquireDepthBufferRenderTexture()` because it is the `Initialize()` method that creates the textures of the render window.

Render textures are used as regular textures. Add them to a compute pass with `vtkWebGPUComputePass::AddRenderTexture()`. Store the
index of the texture returned by that call. Use that texture index to create a texture view with `vtkWebGPUComputePass::CreateTextureView()`.
Configure the texture view, add it to the compute pass and you're done. Your render texture is ready to be used in one of your shaders.

# Using a dedicated GPU device

`vtkWebGPUComputePipeline` automatically initializes a webgpu Device to execute the compute passes.
You can specify a different device by using the `SetWGPUConfiguration` method with a `vtkWebGPUConfiguration`
object. In fact, this capability allows a `vtkWebGPUComputePipeline` to integrate in an existing rendering
pipeline. If you recall from the previous section, we added a compute pipeline to a `vtkWebGPURenderer`.
Under the covers, VTK shares the `vtkWebGPUConfiguration` instance of the `vtkWebGPURenderWindow` with the
compute pipeline.

Here's how you can build a compute pipeline to run on a device with low power consumption and a Vulkan
backend assuming that Vulkan is available on the platform.

```c++
// Creating a vtkWebGPUConfiguration
vtkNew<vtkWebGPUConfiguration> wgpuConfig;
wgpuConfig->SetBackend(vtkWebGPUConfiguration::BackendType::Vulkan);
wgpuConfig->SetPowerPreference(vtkWebGPUConfiguration::PowerPreferenceType::LowPower);

// Creating a vtkWebGPUComputePipeline to use custom device
vtkNew<vtkWebGPUComputePipeline> computePipeline;
computePipeline->SetWGPUConfiguration(wgpuConfig);
```

The `vtkWebGPUComputePipeline` automatically initializes it's own `wgpu::Device` in the constructor. This
may cause a hang. You can specify the default timeout by calling the static method `vtkWebGPUConfiguration::SetDefaultTimeout(double timeout)` with a lower timeout value in milliseconds.
