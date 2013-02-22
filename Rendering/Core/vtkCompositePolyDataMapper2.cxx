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
#include "vtkCompositeDataDisplayAttributes.h"
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
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

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

//-----------------------------------------------------------------------------
bool vtkCompositePolyDataMapper2::GetIsOpaque()
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));
  if (this->ScalarVisibility &&
    this->ColorMode == VTK_COLOR_MODE_DEFAULT && input)
    {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(input->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
        {
        int cellFlag;
        vtkDataArray* scalars = this->GetScalars(pd,
          this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
          this->ArrayName, cellFlag);
        if (scalars && scalars->IsA("vtkUnsignedCharArray") &&
          (scalars->GetNumberOfComponents() ==  4 /*(RGBA)*/ ||
           scalars->GetNumberOfComponents() == 2 /*(LuminanceAlpha)*/))
          {
          vtkUnsignedCharArray* colors =
            static_cast<vtkUnsignedCharArray*>(scalars);
          if ((colors->GetNumberOfComponents() == 4 && colors->GetValueRange(3)[0] < 255) ||
            (colors->GetNumberOfComponents() == 2 && colors->GetValueRange(1)[0] < 255))
            {
            // If the opacity is 255, despite the fact that the user specified
            // RGBA, we know that the Alpha is 100% opaque. So treat as opaque.
            return false;
            }
          }
        }
      }
    }
  return this->Superclass::GetIsOpaque();
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::SetBlockVisibility(unsigned int index, bool visible)
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->SetBlockVisibility(index, visible);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkCompositePolyDataMapper2::GetBlockVisibility(unsigned int index) const
{
  if(this->CompositeAttributes)
    {
    return this->CompositeAttributes->GetBlockVisibility(index);
    }
  else
    {
    return true;
    }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockVisibility(unsigned int index)
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockVisibility(index);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockVisibilites()
{
  if(this->CompositeAttributes)
    {
    this->CompositeAttributes->RemoveBlockVisibilites();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::SetCompositeDataDisplayAttributes(
  vtkCompositeDataDisplayAttributes *attributes)
{
  if(this->CompositeAttributes != attributes)
    {
    this->CompositeAttributes = attributes;

    // set composite display attributes on the composite painter
    vtkDefaultPainter *defaultPainter =
      vtkDefaultPainter::SafeDownCast(this->Painter);
    if(defaultPainter)
      {
      vtkCompositePainter *compositePainter =
        defaultPainter->GetCompositePainter();

      compositePainter->
        SetCompositeDataDisplayAttributes(this->CompositeAttributes);
      }
    }
}

//----------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes*
vtkCompositePolyDataMapper2::GetCompositeDataDisplayAttributes()
{
  return this->CompositeAttributes;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

