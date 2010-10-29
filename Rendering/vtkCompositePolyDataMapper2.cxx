/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositePolyDataMapper2.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePainter.h"
#include "vtkDefaultPainter.h"
#include "vtkDisplayListPainter.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataPainter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColorsPainter.h"

vtkStandardNewMacro(vtkCompositePolyDataMapper2);
//----------------------------------------------------------------------------
vtkCompositePolyDataMapper2::vtkCompositePolyDataMapper2()
{
  // Insert the vtkCompositePainter in the selection pipeline, so that the
  // selection painter can handle composite datasets as well.
  vtkCompositePainter* selectionPainter = vtkCompositePainter::New();
  selectionPainter->SetDelegatePainter(this->SelectionPainter);
  this->SetSelectionPainter(selectionPainter);
  selectionPainter->FastDelete();
}

//----------------------------------------------------------------------------
vtkCompositePolyDataMapper2::~vtkCompositePolyDataMapper2()
{
}

//----------------------------------------------------------------------------
int vtkCompositePolyDataMapper2::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkCompositePolyDataMapper2::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
//Looks at each DataSet and finds the union of all the bounds
void vtkCompositePolyDataMapper2::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
    {
    this->Superclass::ComputeBounds();
    return;
    }

  vtkCompositeDataIterator* iter = input->NewIterator();
  vtkBoundingBox bbox;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
    if (pd)
      {
      double bounds[6];
      pd->GetBounds(bounds);
      bbox.AddBounds(bounds);
      }
    }
  iter->Delete();
  bbox.GetBounds(this->Bounds);
//  this->BoundsMTime.Modified();
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

