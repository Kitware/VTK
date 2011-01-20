/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositedSynchronizedRenderers.h"

#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTreeCompositer.h"

vtkStandardNewMacro(vtkCompositedSynchronizedRenderers);
vtkCxxSetObjectMacro(vtkCompositedSynchronizedRenderers,
  Compositer, vtkCompositer);
//----------------------------------------------------------------------------
vtkCompositedSynchronizedRenderers::vtkCompositedSynchronizedRenderers()
{
  this->Compositer = vtkTreeCompositer::New();
}

//----------------------------------------------------------------------------
vtkCompositedSynchronizedRenderers::~vtkCompositedSynchronizedRenderers()
{
  this->Compositer->Delete();
}

//----------------------------------------------------------------------------
void vtkCompositedSynchronizedRenderers::MasterEndRender()
{
  vtkRawImage& rawImage = (this->ImageReductionFactor == 1)?
    this->FullImage : this->ReducedImage;
  rawImage = this->CaptureRenderedImage();
  vtkFloatArray* depth_buffer = vtkFloatArray::New();
  this->CaptureRenderedDepthBuffer(depth_buffer);

  this->Compositer->SetController(this->ParallelController);
  vtkUnsignedCharArray* resultColor = vtkUnsignedCharArray::New();
  resultColor->SetNumberOfComponents(rawImage.GetRawPtr()->GetNumberOfComponents());
  resultColor->SetNumberOfTuples(rawImage.GetRawPtr()->GetNumberOfTuples());

  vtkFloatArray* result_depth = vtkFloatArray::New();
  result_depth->SetNumberOfTuples(depth_buffer->GetNumberOfTuples());

  this->Compositer->CompositeBuffer(rawImage.GetRawPtr(), depth_buffer,
    resultColor, result_depth);
  depth_buffer->Delete();
  result_depth->Delete();
  resultColor->Delete();
}

//----------------------------------------------------------------------------
void vtkCompositedSynchronizedRenderers::SlaveEndRender()
{
  vtkRawImage &rawImage = this->CaptureRenderedImage();
  vtkFloatArray* depth_buffer = vtkFloatArray::New();
  this->CaptureRenderedDepthBuffer(depth_buffer);
  this->Compositer->SetController(this->ParallelController);

  vtkUnsignedCharArray* resultColor = vtkUnsignedCharArray::New();
  resultColor->SetNumberOfComponents(rawImage.GetRawPtr()->GetNumberOfComponents());
  resultColor->SetNumberOfTuples(rawImage.GetRawPtr()->GetNumberOfTuples());
  vtkFloatArray* result_depth = vtkFloatArray::New();
  result_depth->SetNumberOfTuples(depth_buffer->GetNumberOfTuples());

  this->Compositer->CompositeBuffer(rawImage.GetRawPtr(), depth_buffer,
    resultColor, result_depth);
}

//----------------------------------------------------------------------------
void vtkCompositedSynchronizedRenderers::CaptureRenderedDepthBuffer(
  vtkFloatArray* depth_buffer)
{
  double viewport[4];
  vtkRenderer* ren = this->Renderer;
  ren->GetViewport(viewport);

  int window_size[2];
  window_size[0] = ren->GetVTKWindow()->GetActualSize()[0];
  window_size[1] = ren->GetVTKWindow()->GetActualSize()[1];

  int image_size[2];
  image_size[0] = static_cast<int>(window_size[0] * (viewport[2]-viewport[0]));
  image_size[1] = static_cast<int>(window_size[1] * (viewport[3]-viewport[1]));

  // using RGBA always?
  depth_buffer->SetNumberOfComponents(1);
  depth_buffer->SetNumberOfTuples(image_size[0]*image_size[1]);

  ren->GetRenderWindow()->GetZbufferData(
    static_cast<int>(window_size[0] * viewport[0]),
    static_cast<int>(window_size[1] * viewport[1]),
    static_cast<int>(window_size[0] * viewport[2])-1,
    static_cast<int>(window_size[1] * viewport[3])-1,
    depth_buffer->GetPointer(0));
}

//----------------------------------------------------------------------------
void vtkCompositedSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compositer: ";
  if (this->Compositer)
    {
    this->Compositer->PrintSelf(os, indent.GetNextIndent());;
    }
  else
    {
    os << "(none)" << endl;
    }
}
