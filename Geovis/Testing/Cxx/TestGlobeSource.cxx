/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkActor.h"
#include "vtkDoubleArray.h"
#include "vtkGlobeSource.h"
#include "vtkJPEGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"

#include <vtksys/SystemTools.hxx>

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New();

int TestGlobeSource(int argc, char* argv[])
{
  char* image = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/usa_image.jpg");

  vtkStdString imageFile = image;

  vtkSmartPointer<vtkJPEGReader> reader =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(imageFile.c_str());
  reader->Update();

  double latRange[]  = {24,    50};
  double longRange[] = {-126, -66};

  VTK_CREATE(vtkGlobeSource, globeSource);
  globeSource->SetStartLatitude(latRange[0]);
  globeSource->SetEndLatitude(latRange[1]);
  globeSource->SetStartLongitude(longRange[0]);
  globeSource->SetEndLongitude(longRange[1]);

  globeSource->Update();

  VTK_CREATE(vtkActor, actor);
  VTK_CREATE(vtkPolyDataMapper, mapper);

  VTK_CREATE(vtkDoubleArray, textureCoords);
  textureCoords->SetNumberOfComponents(2);

  vtkDoubleArray* array = vtkDoubleArray::SafeDownCast(
      globeSource->GetOutput(0)->GetPointData()->GetAbstractArray("LatLong"));

  double range[] = { (latRange[0]  - latRange[1]),
                     (longRange[0] - longRange[1])
                   };

  double val[2];
  double newVal[2];

  // Lower values of lat / long will correspond to
  // texture coordinate = 0 (for both s & t).
  for(int i=0; i < array->GetNumberOfTuples(); ++i)
    {

     array->GetTupleValue(i, val);

     // Get the texture coordinates in [0,1] range.
     newVal[1] = (latRange[0]  - val[0])  / range[0];
     newVal[0] = (longRange[0] - val[1]) / range[1];

     textureCoords->InsertNextTuple(newVal);
    }

  globeSource->GetOutput(0)->GetPointData()->SetTCoords(textureCoords);
  mapper->SetInputConnection( globeSource->GetOutputPort() );
  actor->SetMapper(mapper);

  VTK_CREATE(vtkTexture, texture);
  texture->SetInputConnection(reader->GetOutputPort());
  actor->SetTexture(texture);

  // Get the right view.
  VTK_CREATE(vtkTransform, transform);
  transform->RotateY(-90.0);
  transform->RotateX(-90.0);
  actor->SetUserMatrix(transform->GetMatrix());

  VTK_CREATE(vtkRenderWindow, renWin);
  VTK_CREATE(vtkRenderWindowInteractor, renWinInt);
  VTK_CREATE(vtkRenderer, ren);

  ren->AddActor(actor);

  renWin->AddRenderer(ren);
  renWinInt->SetRenderWindow(renWin);

  renWin->SetSize(400,400);
  renWin->Render();
  renWinInt->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    renWinInt->Start();
    }

  delete []image;
  return !retVal;
}
