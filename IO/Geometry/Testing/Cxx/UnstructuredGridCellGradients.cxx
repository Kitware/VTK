/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnstructuredGridCellGradients.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkActor.h"
#include "vtkArrowSource.h"
#include "vtkAssignAttribute.h"
#include "vtkCamera.h"
#include "vtkCellCenters.h"
#include "vtkExtractEdges.h"
#include "vtkGlyph3D.h"
#include "vtkGradientFilter.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStdString.h"
#include "vtkTubeFilter.h"
#include "vtkUnstructuredGridReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

int UnstructuredGridCellGradients(int argc, char *argv[])
{
  int i;
  // Need to get the data root.
  const char *data_root = NULL;
  for (i = 0; i < argc-1; i++)
  {
    if (strcmp("-D", argv[i]) == 0)
    {
      data_root = argv[i+1];
      break;
    }
  }
  if (!data_root)
  {
    cout << "Need to specify the directory to VTK_DATA_ROOT with -D <dir>." << endl;
    return 1;
  }

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  vtkStdString filename;
  filename = data_root;
  filename += "/Data/uGridEx.vtk";
  cout << "Loading " << filename.c_str() << endl;
  VTK_CREATE(vtkUnstructuredGridReader, reader);
  reader->SetFileName(filename.c_str());

  VTK_CREATE(vtkExtractEdges, edges);
  edges->SetInputConnection(reader->GetOutputPort());

  VTK_CREATE(vtkTubeFilter, tubes);
  tubes->SetInputConnection(edges->GetOutputPort());
  tubes->SetRadius(0.0625);
  tubes->SetVaryRadiusToVaryRadiusOff();
  tubes->SetNumberOfSides(32);

  VTK_CREATE(vtkPolyDataMapper, tubesMapper);
  tubesMapper->SetInputConnection(tubes->GetOutputPort());
  tubesMapper->SetScalarRange(0.0, 26.0);

  VTK_CREATE(vtkActor, tubesActor);
  tubesActor->SetMapper(tubesMapper);

  VTK_CREATE(vtkPointDataToCellData, pd2cd);
  pd2cd->SetInputConnection(reader->GetOutputPort());

  VTK_CREATE(vtkGradientFilter, gradients);
  gradients->SetInputConnection(pd2cd->GetOutputPort());
  gradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS,
                             vtkDataSetAttributes::SCALARS);

  VTK_CREATE(vtkCellCenters, cellCenters);
  cellCenters->SetInputConnection(gradients->GetOutputPort());

  VTK_CREATE(vtkAssignAttribute, vectors);
  vectors->SetInputConnection(cellCenters->GetOutputPort());
  vectors->Assign("Gradients", vtkDataSetAttributes::VECTORS,
                  vtkAssignAttribute::POINT_DATA);

  VTK_CREATE(vtkArrowSource, arrow);

  VTK_CREATE(vtkGlyph3D, glyphs);
  glyphs->SetInputConnection(0, vectors->GetOutputPort());
  glyphs->SetInputConnection(1, arrow->GetOutputPort());
  glyphs->ScalingOn();
  glyphs->SetScaleModeToScaleByVector();
  glyphs->SetScaleFactor(0.25);
  glyphs->OrientOn();
  glyphs->ClampingOff();
  glyphs->SetVectorModeToUseVector();
  glyphs->SetIndexModeToOff();

  VTK_CREATE(vtkPolyDataMapper, glyphMapper);
  glyphMapper->SetInputConnection(glyphs->GetOutputPort());
  glyphMapper->ScalarVisibilityOff();

  VTK_CREATE(vtkActor, glyphActor);
  glyphActor->SetMapper(glyphMapper);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(tubesActor);
  renderer->AddActor(glyphActor);
  renderer->SetBackground(0.328125, 0.347656, 0.425781);

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetMultiSamples(0);
  renwin->AddRenderer(renderer);
  renwin->SetSize(350, 500);

  renderer->ResetCamera();
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->Elevation(-85.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-5.0);
  camera->OrthogonalizeViewUp();
  camera->Elevation(-10.0);
  camera->Azimuth(55.0);

  int retVal = vtkTesting::Test(argc, argv, renwin, 5.0);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    VTK_CREATE(vtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  if (retVal == vtkRegressionTester::PASSED)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
