/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResizingWindowToImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResizingWindowToImageFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkWindowToImageFilter.h"

#include <algorithm> // for std::max
#include <cmath>
#include <set>

vtkStandardNewMacro(vtkResizingWindowToImageFilter);

//------------------------------------------------------------------------------
vtkResizingWindowToImageFilter::vtkResizingWindowToImageFilter()
{
  this->Input = nullptr;
  this->Size[0] = this->Size[1] = 0;
  this->InputBufferType = VTK_RGB;
  this->SizeLimit = 4000;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkResizingWindowToImageFilter::~vtkResizingWindowToImageFilter()
{
  if (this->Input)
  {
    this->Input->UnRegister(this);
    this->Input = nullptr;
  }
}

//------------------------------------------------------------------------------
vtkImageData* vtkResizingWindowToImageFilter::GetOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
void vtkResizingWindowToImageFilter::SetInput(vtkWindow* input)
{
  if (input != this->Input)
  {
    if (this->Input)
    {
      this->Input->UnRegister(this);
    }
    this->Input = input;
    if (this->Input)
    {
      this->Input->Register(this);
    }
    this->Modified();
  }
}
//------------------------------------------------------------------------------
void vtkResizingWindowToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Input)
  {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Input: (none)\n";
  }
  os << indent << "Size: " << this->Size[0] << ", " << this->Size[1] << "\n";
  os << indent << "InputBufferType: " << this->InputBufferType << "\n";
}

//------------------------------------------------------------------------------
// This method returns the largest region that can be generated.
void vtkResizingWindowToImageFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (this->Input == nullptr)
  {
    vtkErrorMacro(<< "Please specify a renderer as input!");
    return;
  }

  // set the extent
  int wExtent[6];
  wExtent[0] = 0;
  wExtent[1] = this->Size[0] - 1;
  wExtent[2] = 0;
  wExtent[3] = this->Size[1] - 1;
  wExtent[4] = 0;
  wExtent[5] = 0;

  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent, 6);

  switch (this->InputBufferType)
  {
    case VTK_RGB:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 3);
      break;
    case VTK_RGBA:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 4);
      break;
    case VTK_ZBUFFER:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
      break;
    default:
      // VTK_RGB configuration by default
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 3);
      break;
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkResizingWindowToImageFilter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->RequestData(request, inputVector, outputVector);
    return 1;
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkResizingWindowToImageFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* out = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  out->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  out->AllocateScalars(outInfo);

  if (!this->Input)
  {
    return;
  }

  vtkRenderWindow* renWin = vtkRenderWindow::SafeDownCast(this->Input);
  if (!renWin)
  {
    vtkWarningMacro("The window passed to window to image should be a OpenGLRenderWindow or one of "
                    "its subclasses");
    return;
  }

  if (!((out->GetScalarType() == VTK_UNSIGNED_CHAR &&
          (this->InputBufferType == VTK_RGB || this->InputBufferType == VTK_RGBA)) ||
        (out->GetScalarType() == VTK_FLOAT && this->InputBufferType == VTK_ZBUFFER)))
  {
    vtkErrorMacro("mismatch in scalar types!");
    return;
  }

  int newSize[2] = { 1, 1 };
  int scale[2] = { 1, 1 };
  bool approximate;
  this->GetScaleFactorsAndSize(this->Size, newSize, scale, &approximate);

  // save window state
  int* oldptr = renWin->GetSize();
  int oldSize[2] = { oldptr[0], oldptr[1] };
  bool oldOffScreen = renWin->GetUseOffScreenBuffers();
  vtkTypeBool oldSwap = renWin->GetSwapBuffers();

  renWin->SetUseOffScreenBuffers(true);
  renWin->SwapBuffersOff();
  // calling SetSize this way instead of just ->SetSize() we bypass
  // the vtkCommand::WindowResizeEvent which would cause the window on the
  // screen to be redrawn.
  renWin->vtkRenderWindow::SetSize(newSize[0], newSize[1]);
  renWin->Render();

  vtkNew<vtkWindowToImageFilter> windowToImageFilter;
  windowToImageFilter->ReadFrontBufferOff();
  windowToImageFilter->SetInput(renWin);
  windowToImageFilter->SetScale(scale[0], scale[1]);
  windowToImageFilter->Update();
  out->ShallowCopy(windowToImageFilter->GetOutput());

  // restore window state
  renWin->vtkRenderWindow::SetSize(oldSize[0], oldSize[1]);
  renWin->SetUseOffScreenBuffers(oldOffScreen);
  renWin->SetSwapBuffers(oldSwap);
}

//------------------------------------------------------------------------------
int vtkResizingWindowToImageFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
//------------------------------------------------------------------------------
void vtkResizingWindowToImageFilter::Render()
{
  if (vtkRenderWindow* renWin = vtkRenderWindow::SafeDownCast(this->Input))
  {
    // if interactor is present, trigger render through interactor. This
    // allows for custom applications that provide interactors that
    // customize rendering e.g. ParaView.
    if (renWin->GetInteractor())
    {
      renWin->GetInteractor()->Render();
    }
    else
    {
      renWin->Render();
    }
  }
}

//------------------------------------------------------------------------------
namespace
{
int computeGCD(int a, int b)
{
  return b == 0 ? a : computeGCD(b, a % b);
}
std::set<int> computeFactors(int num)
{
  std::set<int> result;
  const int sroot = std::sqrt(num);
  for (int cc = 1; cc <= sroot; ++cc)
  {
    if (num % cc == 0)
    {
      result.insert(cc);
      if (cc * cc != num)
      {
        result.insert(num / cc);
      }
    }
  }
  return result;
}
}
//------------------------------------------------------------------------------
void vtkResizingWindowToImageFilter::GetScaleFactorsAndSize(
  const int requestedSize[2], int actualSize[2], int scale[2], bool* approximate)
{
  if (approximate)
  {
    *approximate = false;
  }
  scale[0] = 1;
  scale[1] = 1;

  if (requestedSize[0] <= this->SizeLimit && requestedSize[1] <= this->SizeLimit)
  {
    // easy! It just fits.
    actualSize[0] = requestedSize[0];
    actualSize[1] = requestedSize[1];
    return;
  }

  // First we need to see if we can find a magnification factor that preserves
  // aspect ratio. This is the best magnification factor. Do that, we get the
  // GCD for the target width and height and then see if factors of the GCD are
  // a good match.
  const int gcd = computeGCD(requestedSize[0], requestedSize[1]);
  if (gcd > 1)
  {
    const auto factors = computeFactors(gcd);
    for (auto fiter = factors.begin(); fiter != factors.end(); ++fiter)
    {
      const int magnification = *fiter;
      int potentialSize[2];
      potentialSize[0] = requestedSize[0] / magnification;
      potentialSize[1] = requestedSize[1] / magnification;
      if (potentialSize[0] > 1 && potentialSize[1] > 1 && potentialSize[0] <= this->SizeLimit &&
        potentialSize[1] <= this->SizeLimit)
      {
        // found a good fit that's non-trivial.
        actualSize[0] = potentialSize[0];
        actualSize[1] = potentialSize[1];
        scale[0] = magnification;
        scale[1] = magnification;
        return;
      }
    }
  }

  // Next, try to find scale factors at the cost of preserving aspect ratios
  // since that's not possible. For this, we don't worry about GCD. Instead deal
  // with each dimension separately, finding factors for target size and seeing
  // if we can find a good scale factor.
  for (int cc = 0; cc < 2; ++cc)
  {
    if (requestedSize[cc] > this->SizeLimit)
    {
      // first, do a quick guess.
      scale[cc] = std::ceil(requestedSize[cc] / static_cast<double>(this->SizeLimit));

      // now for a more accurate scale; it may not be possible to find one,
      // and hence we first do an approximate calculation.
      const auto factors = computeFactors(requestedSize[cc]);
      // Do not resize the image to less than half of the original size
      int minSize = std::max(1, this->SizeLimit / 2);
      for (auto fiter = factors.begin(); fiter != factors.end(); ++fiter)
      {
        const int potentialSize = requestedSize[cc] / *fiter;
        if (potentialSize > minSize && potentialSize <= this->SizeLimit)
        {
          // caching!
          scale[cc] = *fiter;
          break;
        }
      }
      actualSize[cc] = requestedSize[cc] / scale[cc];
    }
    else
    {
      actualSize[cc] = requestedSize[cc];
      scale[cc] = 1;
    }
  }

  if (approximate != nullptr)
  {
    *approximate = (actualSize[0] * scale[0] != requestedSize[0]);
    *approximate |= (actualSize[1] * scale[1] != requestedSize[1]);
  }
}
