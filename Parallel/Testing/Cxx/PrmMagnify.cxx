/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PrmMagnify.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  Copyright 2005 Sandia Corporation. Under the terms of Contract
  DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
  or on behalf of the U.S. Government. Redistribution and use in source and
  binary forms, with or without modification, are permitted provided that this
  Notice and any statement of authorship are reproduced on all copies.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDummyController.h"
#include "vtkIdFilter.h"
#include "vtkObjectFactory.h"
#include "vtkParallelRenderManager.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"

#include "vtkSmartPointer.h"

#define VTK_CREATE(type, var)   \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

//-----------------------------------------------------------------------------

class vtkTestMagnifyRenderManager : public vtkParallelRenderManager
{
public:
  vtkTypeRevisionMacro(vtkTestMagnifyRenderManager, vtkParallelRenderManager);
  static vtkTestMagnifyRenderManager *New();

protected:
  vtkTestMagnifyRenderManager() { }
  ~vtkTestMagnifyRenderManager() { }

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

private:
  vtkTestMagnifyRenderManager(const vtkTestMagnifyRenderManager &);  // Not implemented.
  void operator=(const vtkTestMagnifyRenderManager &);  // Not implemented.
};

vtkCxxRevisionMacro(vtkTestMagnifyRenderManager, "1.2");
vtkStandardNewMacro(vtkTestMagnifyRenderManager);

void vtkTestMagnifyRenderManager::PreRenderProcessing()
{
  this->RenderWindow->SwapBuffersOff();
}

void vtkTestMagnifyRenderManager::PostRenderProcessing()
{
  this->FullImage->SetNumberOfComponents(4);
  this->FullImage->SetNumberOfTuples(this->FullImageSize[0]*this->FullImageSize[1]);

  int fullImageViewport[4], reducedImageViewport[4];

  // Read in image as RGBA.
  this->UseRGBA = 1;
  this->ReadReducedImage();

  fullImageViewport[0] = 0;  fullImageViewport[1] = 0;
  fullImageViewport[2] = this->FullImageSize[0]/2;
  fullImageViewport[3] = this->FullImageSize[1]/2;
  reducedImageViewport[0] = 0;  reducedImageViewport[1] = 0;
  reducedImageViewport[2] = this->ReducedImageSize[0]/2;
  reducedImageViewport[3] = this->ReducedImageSize[1]/2;
  this->MagnifyImageNearest(this->FullImage, this->FullImageSize,
                            this->ReducedImage, this->ReducedImageSize,
                            fullImageViewport, reducedImageViewport);

  fullImageViewport[0] = this->FullImageSize[0]/2;
  fullImageViewport[1] = 0;
  fullImageViewport[2] = this->FullImageSize[0];
  fullImageViewport[3] = this->FullImageSize[1]/2;
  reducedImageViewport[0] = this->ReducedImageSize[0]/2;
  reducedImageViewport[1] = 0;
  reducedImageViewport[2] = this->ReducedImageSize[0];
  reducedImageViewport[3] = this->ReducedImageSize[1]/2;
  this->MagnifyImageLinear(this->FullImage, this->FullImageSize,
                           this->ReducedImage, this->ReducedImageSize,
                           fullImageViewport, reducedImageViewport);

  // Read in image as RGB.
  this->UseRGBA = 0;
  this->ReducedImageUpToDate = 0;
  this->ReadReducedImage();

  fullImageViewport[0] = 0;
  fullImageViewport[1] = this->FullImageSize[1]/2;
  fullImageViewport[2] = this->FullImageSize[0]/2;
  fullImageViewport[3] = this->FullImageSize[1];
  reducedImageViewport[0] = 0;
  reducedImageViewport[1] = this->ReducedImageSize[1]/2;
  reducedImageViewport[2] = this->ReducedImageSize[0]/2;
  reducedImageViewport[3] = this->ReducedImageSize[1];
  this->MagnifyImageNearest(this->FullImage, this->FullImageSize,
                            this->ReducedImage, this->ReducedImageSize,
                            fullImageViewport, reducedImageViewport);

  fullImageViewport[0] = this->FullImageSize[0]/2;
  fullImageViewport[1] = this->FullImageSize[1]/2;
  fullImageViewport[2] = this->FullImageSize[0];
  fullImageViewport[3] = this->FullImageSize[1];
  reducedImageViewport[0] = this->ReducedImageSize[0]/2;
  reducedImageViewport[1] = this->ReducedImageSize[1]/2;
  reducedImageViewport[2] = this->ReducedImageSize[0];
  reducedImageViewport[3] = this->ReducedImageSize[1];
  this->MagnifyImageLinear(this->FullImage, this->FullImageSize,
                           this->ReducedImage, this->ReducedImageSize,
                           fullImageViewport, reducedImageViewport);

  this->FullImageUpToDate = 1;

  this->WriteFullImage();

  this->RenderWindow->SwapBuffersOn();
  this->RenderWindow->Frame();
}


//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  VTK_CREATE(vtkDummyController, controller);
  controller->Initialize(&argc, &argv);

  VTK_CREATE(vtkTestMagnifyRenderManager, prm);
  prm->SetController(controller);

  VTK_CREATE(vtkSphereSource, sphere);
  sphere->SetEndPhi(90.0);
  sphere->SetPhiResolution(4);

  VTK_CREATE(vtkIdFilter, colors);
  colors->SetInputConnection(sphere->GetOutputPort());
  colors->PointIdsOff();
  colors->CellIdsOn();
  colors->FieldDataOff();
  colors->Update();

  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(colors->GetOutputPort());
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(colors->GetOutput()->GetCellData()
                         ->GetScalars()->GetRange());

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer = prm->MakeRenderer();
  renderer->Delete();   // Remove duplicate reference.
  renderer->AddActor(actor);

  vtkSmartPointer<vtkRenderWindow> renwin = prm->MakeRenderWindow();
  renwin->Delete();     // Remove duplicate reference.
  renwin->SetSize(299, 299);
  renwin->AddRenderer(renderer);
  prm->SetRenderWindow(renwin);

  prm->ResetAllCameras();
  prm->SetImageReductionFactor(8);

  // Run the regression test.
  renwin->Render();
  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    VTK_CREATE(vtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    renwin->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  controller->Finalize();

  return !retVal;
}
