/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoryLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCategoryLegend.h"

#include "vtkColorSeries.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkVariantArray.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkContextView.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestCategoryLegend(int argc, char* argv[])
{
  vtkNew<vtkVariantArray> values;
  values->InsertNextValue(vtkVariant("a"));
  values->InsertNextValue(vtkVariant("b"));
  values->InsertNextValue(vtkVariant("c"));

  vtkNew<vtkLookupTable> lut;
  for (int i = 0; i < values->GetNumberOfTuples(); ++i)
    {
    lut->SetAnnotation(values->GetValue(i), values->GetValue(i).ToString());
    }

  vtkNew<vtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(vtkColorSeries::BREWER_QUALITATIVE_SET3);
  colorSeries->BuildLookupTable(lut.GetPointer());

  vtkNew<vtkCategoryLegend> legend;
  legend->SetScalarsToColors(lut.GetPointer());
  legend->SetValues(values.GetPointer());
  legend->SetTitle("legend");

  vtkNew<vtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(legend.GetPointer());
  trans->Translate(180, 70);

  vtkNew<vtkContextView> contextView;
  contextView->GetScene()->AddItem(trans.GetPointer());
  contextView->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  contextView->GetRenderWindow()->SetSize(300,200);
  contextView->GetRenderWindow()->SetMultiSamples(0);
  contextView->GetRenderWindow()->Render();

  int retVal = vtkRegressionTestImage(contextView->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    contextView->GetRenderWindow()->Render();
    contextView->GetInteractor()->Start();
    retVal = vtkRegressionTester::PASSED;
    }
  return !retVal;
}
