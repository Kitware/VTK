// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputeFrustumCuller.h"
#include "FrustumCullingShader.h" // For the frustum culling shader source code
#include "vtkActor.h"             // for safe downcasting in the bounds recomputed callback
#include "vtkCamera.h"            // for manipulating the camera data used by the shader
#include "vtkCommand.h"           // for handling observers on bounds recomputed
#include "vtkMatrix4x4.h"         // for manipulating the view-projection matrix
#include "vtkNew.h"               // for vtk standard new macro
#include "vtkObjectFactory.h"
#include "vtkProp.h"     // for manipulating props
#include "vtkRenderer.h" // for manipulating the renderer

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputeFrustumCuller);

//------------------------------------------------------------------------------
vtkWebGPUComputeFrustumCuller::vtkWebGPUComputeFrustumCuller()
{
  // General pipeline settings
  this->Pipeline = vtkSmartPointer<vtkWebGPUComputePipeline>::New();
  this->Pipeline->SetShaderSource(FrustumCullingShader);
  this->Pipeline->SetShaderEntryPoint("frustumCullingEntryPoint");

  // Buffer that will contain the number of objects that have not been culled
  vtkSmartPointer<vtkWebGPUComputeBuffer> outputCountBuffer =
    vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  outputCountBuffer->SetGroup(0);
  outputCountBuffer->SetBinding(2);
  outputCountBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputCountBuffer->SetByteSize(sizeof(int));
  outputCountBuffer->SetLabel("Frustum culler output count buffer");
  outputCountBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  this->OutputObjectCountBufferIndex = this->Pipeline->AddBuffer(outputCountBuffer);
}

//------------------------------------------------------------------------------
double vtkWebGPUComputeFrustumCuller::Cull(
  vtkRenderer* ren, vtkProp** propList, int& listLength, int& initialized)
{
  if (listLength != this->PreviousPropsCount)
  {
    this->ResizeCuller(propList, listLength);
  }

  this->TriggerBoundsRecomputation(propList, listLength);
  // Reuploads the bounds if some bounds actually need reupload
  this->UpdateBoundsBuffer(propList, listLength);
  // Reuploads the camera data if the camera was modified since last time
  this->UpdateCamera(ren);

  this->PreviousPropsCount = listLength;

  // Zeroing the counter of objects that passed the culling test
  std::vector<int> zero = { 0 };
  this->Pipeline->UpdateBufferData(OutputObjectCountBufferIndex, zero);
  this->Pipeline->Dispatch();
  this->Pipeline->ReadBufferFromGPU(this->OutputObjectCountBufferIndex,
    vtkWebGPUComputeFrustumCuller::OutputObjectCountMapCallback, &listLength);

  vtkWebGPUComputeFrustumCuller::OutputIndicesCallbackData callbackData;
  callbackData.indicesCount = &listLength;
  callbackData.propList = propList;
  callbackData.scratchList = &this->CallbackScratchList;
  this->Pipeline->ReadBufferFromGPU(this->OutputIndicesBufferIndex,
    vtkWebGPUComputeFrustumCuller::OutputObjectIndicesMapCallback, &callbackData);
  this->Pipeline->Update();

  // The allocated time of the props has now been initialized
  initialized = true;

  // We need to return the total time allocated to all the props.
  // Because we're either completely culling a prop or not, each not culled prop has an allocated
  // time of 1 Thus, the total time allocated is just the number of props that were not culled
  return listLength;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::ResizeCuller(vtkProp** propList, int newPropsCount)
{
  // Recomputing the number of workgroups needed to cover the new number of props in the compute
  // shader
  int groupsX = std::ceil(newPropsCount / 32.0f);
  this->Pipeline->SetWorkgroups(groupsX, 1, 1);

  this->ResizeBoundsBuffer(propList, newPropsCount);
  this->ResizeOutputIndicesBuffer(newPropsCount);
  this->ResizeScratchList(propList, newPropsCount);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::CreateInputBoundsBuffer(
  vtkProp** propList, unsigned int nbProps)
{
  // Gathering the bounds of the actors to upload to the GPU buffer
  std::vector<float> propsBounds(6 * nbProps);

  for (unsigned int i = 0; i < nbProps; i++)
  {
    double* bounds = propList[i]->GetBounds();

    for (int j = 0; j < 6; j++)
    {
      propsBounds[i * 6 + j] = static_cast<float>(bounds[j]);
    }
  }

  vtkSmartPointer<vtkWebGPUComputeBuffer> inputBoundsBuffer =
    vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  inputBoundsBuffer->SetGroup(0);
  inputBoundsBuffer->SetBinding(0);
  inputBoundsBuffer->SetLabel("Input bounds buffer");
  inputBoundsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputBoundsBuffer->SetData(propsBounds);

  this->InputBoundsBufferIndex = this->Pipeline->AddBuffer(inputBoundsBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::CreateOutputIndicesBuffer(int nbProps)
{
  vtkSmartPointer<vtkWebGPUComputeBuffer> outputIndicesBuffer =
    vtkSmartPointer<vtkWebGPUComputeBuffer>::New();

  outputIndicesBuffer->SetGroup(0);
  outputIndicesBuffer->SetBinding(1);
  outputIndicesBuffer->SetLabel("Frustum culler output indices buffer");
  outputIndicesBuffer->SetByteSize(nbProps * sizeof(unsigned int));
  outputIndicesBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);

  this->OutputIndicesBufferIndex = this->Pipeline->AddBuffer(outputIndicesBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::CreateViewProjMatrixBuffer(const std::vector<float>& vpMat)
{
  vtkSmartPointer<vtkWebGPUComputeBuffer> viewProjMatBuffer =
    vtkSmartPointer<vtkWebGPUComputeBuffer>::New();

  viewProjMatBuffer->SetGroup(0);
  viewProjMatBuffer->SetBinding(3);
  viewProjMatBuffer->SetMode(vtkWebGPUComputeBuffer::UNIFORM_BUFFER);
  viewProjMatBuffer->SetData(vpMat);
  viewProjMatBuffer->SetLabel("Camera view-projection matrix uniform buffer");

  this->CameraViewProjMatrixBufferIndex = this->Pipeline->AddBuffer(viewProjMatBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::ResizeBoundsBuffer(vtkProp** propList, int newPropsCount)
{
  if (this->InputBoundsBufferIndex != -1)
  {
    // If the buffer already exists, resizing it

    // WebGPU doesn't support double precision floating point numbers (as of may 2024). We're thus
    // resizing the buffer with floats size because the compute shader is going to use floats, not
    // doubles which would be the matching type for VTK's bounds.
    this->Pipeline->ResizeBuffer(this->InputBoundsBufferIndex, newPropsCount * 6 * sizeof(float));
  }
  else
  {
    // If the buffer doesn't already exist, creating it
    this->CreateInputBoundsBuffer(propList, newPropsCount);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::ResizeOutputIndicesBuffer(int newPropsCount)
{
  if (this->OutputIndicesBufferIndex != -1)
  {
    // If the buffer already exists, resizing it

    this->Pipeline->ResizeBuffer(this->OutputIndicesBufferIndex, newPropsCount * sizeof(int));
  }
  else
  {
    // If the buffer doesn't already exist, creating it
    this->CreateOutputIndicesBuffer(newPropsCount);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::ResizeScratchList(vtkProp** propList, int listLength)
{
  this->CallbackScratchList.resize(listLength);
  for (int i = 0; i < listLength; i++)
  {
    this->CallbackScratchList[i] = propList[i];
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::TriggerBoundsRecomputation(vtkProp** propList, int listLength)
{
  for (int i = 0; i < listLength; i++)
  {
    vtkProp* prop = propList[i];
    prop->GetBounds();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::UpdateBoundsBuffer(vtkProp** propList, int listLength)
{
  std::vector<float> allBounds(listLength * 6);
  for (int i = 0; i < listLength; i++)
  {
    vtkProp* prop = propList[i];
    double* propBounds = prop->GetBounds();

    for (int j = 0; j < 6; j++)
    {
      allBounds[i * 6 + j] = static_cast<float>(propBounds[j]);
    }
  }

  this->Pipeline->UpdateBufferData(this->InputBoundsBufferIndex, allBounds);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::UpdateCamera(vtkRenderer* ren)
{
  vtkCamera* camera = ren->GetActiveCamera();
  // Getting the view-projection matrix
  vtkMatrix4x4* viewMatrix = camera->GetModelViewTransformMatrix();

  // We're using [0, 1] for znear and zfar here to align with WebGPU convention
  vtkMatrix4x4* projectionMatrix =
    camera->GetProjectionTransformMatrix(ren->GetTiledAspectRatio(), 0, 1);
  vtkNew<vtkMatrix4x4> viewProj;
  vtkMatrix4x4::Multiply4x4(projectionMatrix, viewMatrix, viewProj);
  // WebGPU uses column major matrices but VTK is row major
  viewProj->Transpose();

  // Getting the matrix data as floats
  std::vector<float> matrixData(16);
  for (int i = 0; i < 16; i++)
  {
    matrixData[i] = static_cast<float>(viewProj->GetData()[i]);
  }

  // Creating / updating the view-proj matrix uniform buffer
  if (this->CameraViewProjMatrixBufferIndex == -1)
  {
    this->CreateViewProjMatrixBuffer(matrixData);
  }
  else
  {
    this->Pipeline->UpdateBufferData(this->CameraViewProjMatrixBufferIndex, matrixData);
  }
}

//------------------------------------------------------------------------------
int vtkWebGPUComputeFrustumCuller::ReuploadBoundsBufferThreshold()
{
  // Simply returns the constant for now but this function allows for a smarter implmentation in the
  // future
  return vtkWebGPUComputeFrustumCuller::WHOLE_BUFFER_REUPLOAD_THRESHOLD;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::OutputObjectCountMapCallback(
  const void* mappedData, void* userdata)
{
  int* listLength = static_cast<int*>(userdata);

  const int* mappedIntData = static_cast<const int*>(mappedData);
  *listLength = *mappedIntData;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeFrustumCuller::OutputObjectIndicesMapCallback(
  const void* mappedData, void* userdata)
{
  vtkWebGPUComputeFrustumCuller::OutputIndicesCallbackData* data =
    reinterpret_cast<vtkWebGPUComputeFrustumCuller::OutputIndicesCallbackData*>(userdata);

  const int* nonCulledIndices = reinterpret_cast<const int*>(mappedData);
  for (int i = 0; i < *data->indicesCount; i++)
  {
    data->propList[i] = (*data->scratchList)[nonCulledIndices[i]];
  }
}

VTK_ABI_NAMESPACE_END
