// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariRenderWindow.h"

#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include "vtkAnariDevice.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariViewNodeFactory.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariRenderWindow);

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkAnariRenderWindow::CreateOverrideAttributes()
{
  auto* supportRenderPassAttribute =
    vtkOverrideAttribute::CreateAttributeChain("SupportRenderPass", "false", nullptr);
  auto* renderingBackendAttribute = vtkOverrideAttribute::CreateAttributeChain(
    "RenderingBackend", "ANARI", supportRenderPassAttribute);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkAnariRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->AnariSceneGraph)
  {
    this->AnariSceneGraph->PrintSelf(os, indent);
  }
  this->AnariDevice->PrintSelf(os, indent);
  this->AnariRenderer->PrintSelf(os, indent);
  this->AnariFactory->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkAnariRenderWindow::vtkAnariRenderWindow()
{
  this->OffScreenRenderingOn();

  this->AnariDevice->SetOnNewDeviceCallback(
    [&]() { this->AnariRenderer->SetAnariDevice(this->AnariDevice); });
}

//------------------------------------------------------------------------------
unsigned char* vtkAnariRenderWindow::GetPixelData(
  int x1, int y1, int x2, int y2, int vtkNotUsed(front), int vtkNotUsed(right))
{
  return this->SampleSourceColorBuffer(x1, y1, x2, y2, 3);
}

//------------------------------------------------------------------------------
unsigned char* vtkAnariRenderWindow::GetRGBACharPixelData(
  int x1, int y1, int x2, int y2, int vtkNotUsed(front), int vtkNotUsed(right))
{
  return this->SampleSourceColorBuffer(x1, y1, x2, y2, 4);
}

//------------------------------------------------------------------------------
int vtkAnariRenderWindow::GetPixelData(int x1, int y1, int x2, int y2, int vtkNotUsed(front),
  vtkUnsignedCharArray* data, int vtkNotUsed(right))
{
  return this->SampleSourceColorBuffer(x1, y1, x2, y2, 3, data);
}

//------------------------------------------------------------------------------
int vtkAnariRenderWindow::GetRGBACharPixelData(int x1, int y1, int x2, int y2,
  int vtkNotUsed(front), vtkUnsignedCharArray* data, int vtkNotUsed(right))
{
  return this->SampleSourceColorBuffer(x1, y1, x2, y2, 4, data);
}

//------------------------------------------------------------------------------
void vtkAnariRenderWindow::DoStereoRender()
{
  // Assuming there is one renderer
  vtkRenderer* renderer = this->GetRenderers()->GetFirstRenderer();

  if (!renderer)
  {
    return;
  }

  // Setup scene graph
  {
    anari::Device device = this->AnariDevice->GetHandle();

    const bool rebuildSceneGraph =
      !this->AnariSceneGraph || this->AnariSceneGraph->GetDeviceHandle() != device;
    if (rebuildSceneGraph)
    {
      vtkAnariSceneGraph* sceneGraph =
        vtkAnariSceneGraph::SafeDownCast(this->AnariFactory->CreateNode(renderer));
      this->AnariSceneGraph = vtkSmartPointer<vtkAnariSceneGraph>::Take(sceneGraph);

      this->AnariSceneGraph->SetAnariDevice(this->AnariDevice,
        this->AnariDevice->GetAnariDeviceExtensions(),
        this->AnariDevice->GetAnariDeviceExtensionStrings());
      this->AnariSceneGraph->SetAnariRenderer(this->AnariRenderer->GetHandle());
    }
    else if (this->AnariRenderer->GetHandle() != this->AnariSceneGraph->GetRendererHandle())
    {
      this->AnariSceneGraph->SetAnariRenderer(this->AnariRenderer->GetHandle());
    }
  }

  // Setup frame
  vtkAnariSceneGraph* anariRendererNode =
    vtkAnariSceneGraph::SafeDownCast(this->AnariSceneGraph->GetViewNodeFor(renderer));
  anariRendererNode->SetSize(this->GetSize());
  anariRendererNode->SetViewport(this->GetTileViewport());
  anariRendererNode->SetScale(this->GetTileScale());

  // Internal ANARI render call
  this->AnariSceneGraph->TraverseAllPasses();
}

//------------------------------------------------------------------------------
unsigned char* vtkAnariRenderWindow::SampleSourceColorBuffer(
  int x1, int y1, int x2, int y2, int channelCount)
{
  bool success;
  return this->SampleSourceColorBuffer(x1, y1, x2, y2, channelCount, success);
}

//------------------------------------------------------------------------------
unsigned char* vtkAnariRenderWindow::SampleSourceColorBuffer(
  int x1, int y1, int x2, int y2, int channelCount, bool& success)
{
  // Re-order rectangle
  int xBegin = std::min(x1, x2);
  int xEnd = std::max(x1, x2);
  int yBegin = std::min(y1, y2);
  int yEnd = std::max(y1, y2);

  int width = xEnd - xBegin + 1;
  int height = yEnd - yBegin + 1;

  if (width > this->GetWidth() || height > this->GetHeight())
  {
    success = false;
    return nullptr;
  }

  unsigned char* buffer = new unsigned char[width * height * channelCount];

  for (int y = 0; y < height; y++)
  {
    int sourceYPos = y + yBegin;
    for (int x = 0; x < width; x++)
    {
      int sourceXPos = x + xBegin;
      int destOffset = (y * width + x) * channelCount;
      const unsigned char* sourcePixelData = this->SampleSourceColorBuffer(sourceXPos, sourceYPos);
      if (!sourcePixelData)
      {
        success = false;
        return nullptr;
      }

      for (int channel = 0; channel < channelCount; channel++)
      {
        buffer[destOffset + channel] = sourcePixelData[channel];
      }
    }
  }

  success = true;
  return buffer;
}

//------------------------------------------------------------------------------
int vtkAnariRenderWindow::SampleSourceColorBuffer(
  int x1, int y1, int x2, int y2, int channelCount, vtkUnsignedCharArray* data)
{
  bool success = true;
  unsigned char* pixelBuffer = this->SampleSourceColorBuffer(x1, y1, x2, y2, channelCount, success);
  if (!success)
  {
    return VTK_ERROR;
  }

  unsigned int pixelBufferWidth = std::abs(x2 - x1) + 1;
  unsigned int pixelBufferHeight = std::abs(y2 - y1) + 1;
  data->SetVoidArray(static_cast<void*>(pixelBuffer), pixelBufferWidth * pixelBufferHeight, 0,
    vtkUnsignedCharArray::VTK_DATA_ARRAY_DELETE);

  return VTK_OK;
}

//------------------------------------------------------------------------------
const unsigned char* vtkAnariRenderWindow::SampleSourceColorBuffer(int x, int y) const
{
  const unsigned char* sourcePixelData = this->AnariSceneGraph->GetBuffer();
  if (!sourcePixelData)
  {
    return nullptr;
  }

  int pixelOffset = (y * this->GetWidth() + x) * 4;
  return sourcePixelData + pixelOffset;
}

//------------------------------------------------------------------------------
int vtkAnariRenderWindow::GetWidth() const
{
  return this->AnariSceneGraph->GetSize()[0];
}

//------------------------------------------------------------------------------
int vtkAnariRenderWindow::GetHeight() const
{
  return this->AnariSceneGraph->GetSize()[1];
}

VTK_ABI_NAMESPACE_END
