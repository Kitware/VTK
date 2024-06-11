### Regular buffers

The way rendering resources are managed by VTK internally imposed some constraints on the design
of the compute API. The goal of this document is to explain the order / flow in which WebGPU
resources are managed by the compute API and why it was made that way.

Let's start with a usage example of the compute API when not integrated in an existing rendering
pipeline:

```c++
  // 1)
  vtkNew<vtkWebGPUComputeBuffer> inputBuffer;
  inputBuffer->SetGroup(0);
  inputBuffer->SetBinding(0);
  inputBuffer->SetMode(...);
  inputBuffer->SetData(...);

  vtkNew<vtkWebGPUComputePipeline> computePipeline;
  computePipeline->SetShaderSource(...);
  computePipeline->SetShaderEntryPoint(...);

  // 2)
  computePipeline->AddBuffer(inputBuffer);

  // ...

  // 3)
  computePipeline->Dispatch();
```

1) `vtkWebGPUComputeBuffer` objects just hold buffer references and parameters, the real buffer-setup
work is done in `AddBuffer()` and `Dispatch()`.
2) `AddBuffer()` uses the information contained in `vtkWebGPUComputeBuffer` to create the buffer
on the device and upload the given data (for input buffers). The buffers (`wgpu::Buffer` as well as
`vtkWebGPUComputeBuffer`) are added to their respective list, kept as member variables of the
`vtkWebGPUComputeAPI`. These two lists are mainly used by `ReadBufferFromGPU()` when
the size of the buffer is needed (the `Buffers` list is used to retrieve the size) and the
`wgpu::Buffer` itself is needed in the mapping process.

```c++
// Adding the buffers to the lists
this->Buffers.push_back(buffer);
this->WGPUBuffers.push_back(wgpuBuffer);
```

The `AddBuffer()` method also creates the `BindGroupLayoutEntry` and `BindGroupEntry` associated with
the buffer. These entries will be used when creating the `BindGroupLayout` and `BindGroup` in `Dispatch()`:

```c++
// Creating the layout entry and the bind group entry for this buffer. These entries will be used
// later when creating the bind groups / bind group layouts
AddBindGroupLayoutEntry(buffer->GetGroup(), buffer->GetBinding(), buffer->GetMode());
AddBindGroupEntry(wgpuBuffer, buffer->GetGroup(), buffer->GetBinding(), buffer->GetMode(), 0);
```

3) `Dispatch()` not only dispatches the compute shader but also finishes the creation of the pipeline:

```c++
if (!Initialized)
{
  CreateShaderModule();
  CreateBindGroups();
  CreateComputePipeline();

  Initialized = true;
}
```

The `BindGroups` are created in the `Dispatch()` call because it is only when the user dispatches
the compute that we can be sure that all the buffers have been given and thus that we can actually
create the `BindGroups` (and the pipeline that goes with it).

The shader is compiled and the pipeline is also created in the `Dispatch()` call.

The takeaway point is that there are some operations that need to be done only when `Dispatch()` is called
because this is the only moment where we can be sure the user has provided all the relevant information we
need to fully setup the pipeline. An alternative would have been to provide a `Finish()` method that would
be called by the user before `Dispatch()`. However, this would be a non-intuitive additional step for the user
to perform as having to call a `Finish()` function would slightly expose the internal workings of the API
(that some operations need to be completed before actually being able to dispatch).

### Render buffers

We also uses the same logic (complete the setup of the pipeline when everything is configured) when the
compute pipeline is integrated in a rendering pipeline:

```c++
// Getting the WebGPUMapper to access the point attribute render buffers
vtkWebGPUPolyDataMapper* webGPUMapper = vtkWebGPUPolyDataMapper::SafeDownCast(mapper);

// 1)
int bufferGroup = 0, bufferBinding = 0;
int uniformsGroup = 0, uniformsBinding = 1;
vtkSmartPointer<vtkWebGPUComputeRenderBuffer> pointColorsRenderBuffer =
  webGPUMapper->AcquirePointAttributeComputeRenderBuffer(
    vtkWebGPUPolyDataMapper::PointDataAttributes::COLORS, bufferGroup, bufferBinding,
    uniformsGroup, uniformsBinding);

vtkNew<vtkWebGPUComputePipeline> myPipeline;
// 2)
myPipeline->AddRenderBuffer(pointColorsRenderBuffer);

// ...
// ...
// ...

// 3)
renWin->Render();

```

1) The role of `AcquirePointAttributeComputeRenderBuffer()` (or the cell attribute equivalent) is to set up the
`vtkWebGPUComputeRenderBuffer` (which is only a parameter holder, same as `vtkWebGPUComputeBuffer`) so that it
is ready to be created when we have all the information required. The buffer (which is returned by the function)
is also added to a list of `vtkWebGPUComputeRenderBuffer` held by the WebGPUMapper that will be used later.

2) `AddRenderBuffer()` also acts as a setup step. No creation is done here (contrary to `AddBuffer()` which does
some buffer creation / data upload) because we do not have the data attributes buffer of the mapper yet. This
data attribute buffer is created only on calling render so that's where most of the work is done.

3) Calling the render window's `Render()` function will in turn end up calling `vtkWebGPURenderer::DeviceRender()`.
It is the `DeviceRender()` method that will actually create the device buffers by calling `UpdateComputeBuffers()`:

```c++
void vtkWebGPURenderer::DeviceRender()
{
  // 1)
  // mappers prepare geometry SSBO and pipeline layout.
  this->UpdateGeometry();

  this->CreateBuffers(); // 2)
  this->UpdateBufferData(); // 3)

  // 4)
  this->UpdateComputePipelines();

  // 5)
  this->ComputePass();

  // ...
}
```

1) The actual data buffer that contains the point/cell attributes is created by the `UpdateGeometry()` call.

2) `CreateBuffers()` creates the transform / light buffers of the scene.

3) `UpdateBufferData` uploads the transform / light data to their buffers.

4) Now that the mapper data buffer has been created, it is possible to completely setup the render buffer
(previously added to a list held by the mapper) and add it to the pipeline. This is done by `UpdateComputePipelines()`.
This function loops over all the pipelines that have yet to be configured and sets their `wgpu::Device` and
`wgpu::Adapter` to be the same as the one from the `vtkWebGPURenderWindow`. This is necessary because if we
want our compute pipeline to use buffers that have been created by the `wgpu::Device` of the render window,
the compute pipeline is going to have to use the same `wgpu::Device` (and adapter). If a compute pipeline
was set up (which means that it wasn't set up before), we also loop through all the actors of the renderer.
For each actor, we retrieve the render buffers that have yet to be set up and we set them up. The setup of
each buffer can now be completed since we have access to the size of the mapper's data (point/cell attributes)
buffer which wasn't created until now. Once the render buffer is set up, it is added to its compute pipeline
with `vtkWebGPUComputePipeline::SetupRenderBuffer()`:

```c++
void vtkWebGPURenderer::UpdateComputePipelines()
{
  for (vtkSmartPointer<vtkWebGPUComputePipeline> computePipeline : this->NotSetupComputePipelines)
  {
    computePipeline->SetAdapter(webGPURenderWindow->GetAdapter());
    computePipeline->SetDevice(webGPURenderWindow->GetDevice());

    this->UpdateComputeBuffers(computePipeline);
  }
}

void vtkWebGPURenderer::UpdateComputeBuffers()
{
  for (vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer : wgpuMapper->GetComputeRenderBuffers())
  {
    vtkWebGPUPolyDataMapper::PointDataAttributes bufferAttribute = renderBuffer->GetBufferAttribute();

    // Setup of the render buffer
    renderBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
    renderBuffer->SetByteSize(wgpuMapper->GetPointAttributeByteSize(bufferAttribute));
    renderBuffer->SetRenderBufferOffset(wgpuMapper->GetPointAttributeByteOffset(bufferAttribute) / sizeof(float));
    renderBuffer->SetRenderBufferElementCount(wgpuMapper->GetPointAttributeByteSize(bufferAttribute) / wgpuMapper->GetPointAttributeElementSize(bufferAttribute));
    renderBuffer->SetWGPUBuffer(wgpuMapper->GetPointDataWGPUBuffer());

    // Setup done, the render buffer can be added to the pipeline
    vtkWebGPUComputePipeline* associatedPipeline = renderBuffer->GetAssociatedPipeline();
    associatedPipeline->SetupRenderBuffer(renderBuffer);
  }
}
```

5) After all the render buffers have been added to their corresponding pipelines (done only once on the first frame),
the pipelines can be dispatched:

```c++
void vtkWebGPURenderer::DeviceRender()
{
  // ...
  this->ComputePass();
  // ...
}

void vtkWebGPURenderer::ComputePass()
{
  // Executing the compute pipelines before the rendering so that the
  // render can take the compute pipelines results into account
  for (vtkWebGPUComputePipeline* pipeline : this->ComputePipelines)
  {
    pipeline->Dispatch();
  }
}
```

The bind groups of the pipelines will be created by the `Dispatch()` call as explained
in the 'Regular buffers' section and everything will be in order.

So overall, the reason why some parts of the setup are done immediately while other parts are done
only when rendering a frame / calling `Dispatch()` is because we may not have all required
pieces of information until rendering the first frame / calling `Dispatch()`.
