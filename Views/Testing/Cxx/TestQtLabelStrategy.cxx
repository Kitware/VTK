/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtLabelStrategy.cxx

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
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkGraphLayoutView.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTestUtilities.h"
#include "vtkUnicodeString.h"
#include "vtkUnicodeStringArray.h"
#include "vtkXMLTreeReader.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkActor2D.h"
#include "vtkPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkTextProperty.h"
#include "vtkQtLabelRenderStrategy.h"

#include <sstream>
#include <time.h>

#include <QApplication>
#include <QFontDatabase>

using std::string;

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


int TestQtLabelStrategy(int argc, char* argv[])
{
  int n = 1000;

  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char **>(argv));
  QString fontFileName = testHelper->GetDataRoot();
//  fontFileName.append("/Data/Infovis/PintassilgoPrints_-_Talvez_Assim.ttf");
  fontFileName.append("/Data/Infovis/martyb_-_Ridiculous.ttf");
//  fontFileName.append("/Data/Infovis/DaveDS_-_Sketchy.ttf");

  QApplication app(argc, argv);

  QFontDatabase::addApplicationFont(fontFileName);

  VTK_CREATE(vtkPolyData, pd);
  VTK_CREATE(vtkPoints, pts);
  VTK_CREATE(vtkCellArray, verts);
  VTK_CREATE(vtkDoubleArray, orient);
  orient->SetName("orientation");
  VTK_CREATE(vtkStringArray, label);
  label->SetName("label");

  srand( time(NULL) );

  for( int i = 0; i < n; i++ )
    {
    pts->InsertNextPoint((double)(rand()%100), (double)(rand()%100), (double)(rand()%100));
    verts->InsertNextCell(1);
    verts->InsertCellPoint(i);
    orient->InsertNextValue((double)(rand()%100)*3.60);
    vtkStdString s;
    std::stringstream out;
    out << i;
    s = out.str();
    label->InsertNextValue(s);
    }

  pd->SetPoints(pts);
  pd->SetVerts(verts);
  pd->GetPointData()->AddArray(label);
  pd->GetPointData()->AddArray(orient);

  VTK_CREATE(vtkPointSetToLabelHierarchy, hier);
  hier->SetInput(pd);
  hier->SetOrientationArrayName("orientation");
  hier->SetLabelArrayName("label");
  hier->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
//  hier->GetTextProperty()->SetFontFamilyAsString("Talvez assim");
  hier->GetTextProperty()->SetFontFamilyAsString("Ridiculous");
//  hier->GetTextProperty()->SetFontFamilyAsString("Sketchy");
  hier->GetTextProperty()->SetFontSize(72);

  VTK_CREATE(vtkLabelPlacementMapper, lmapper);
  lmapper->SetInputConnection(hier->GetOutputPort());
  lmapper->SetShapeToRoundedRect();
  lmapper->SetBackgroundColor(1.0, 1.0, 0.7);
  lmapper->SetBackgroundOpacity(0.8);
  lmapper->SetMargin(3);

  VTK_CREATE(vtkQtLabelRenderStrategy, strategy);
  lmapper->SetRenderStrategy(strategy);

  VTK_CREATE(vtkActor2D, lactor);
  lactor->SetMapper(lmapper);

  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInput(pd);
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(lactor);
  ren->AddActor(actor);
  ren->ResetCamera();

  VTK_CREATE(vtkRenderWindow, win);
  win->SetSize(600,600);
  win->AddRenderer(ren);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);

  int retVal = vtkRegressionTestImage(win);
//  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}
