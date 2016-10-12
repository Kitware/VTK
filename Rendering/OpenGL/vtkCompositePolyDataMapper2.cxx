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
#include "vtkScalarsToColors.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

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
  this->SelectionCompositePainter = selectionPainter;
  this->LastOpaqueCheckTime = 0;
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

  if (input->GetMTime() < this->BoundsMTime &&
      this->GetMTime() < this->BoundsMTime)
  {
    return;
  }

  // computing bounds with only visible blocks
  vtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
    this->CompositeAttributes, input, this->Bounds);
  this->BoundsMTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkCompositePolyDataMapper2::GetIsOpaque()
{
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));
  vtkMTimeType lastMTime = std::max(input ? input->GetMTime() : 0, this->GetMTime());
  if (lastMTime <= this->LastOpaqueCheckTime)
  {
    return this->LastOpaqueCheckValue;
  }
  this->LastOpaqueCheckTime = lastMTime;
  if (this->ScalarVisibility && input &&
      (this->ColorMode == VTK_COLOR_MODE_DEFAULT ||
       this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS))
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
        if (scalars &&
            (scalars->IsA("vtkUnsignedCharArray") ||
             this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS) &&
          (scalars->GetNumberOfComponents() ==  4 /*(RGBA)*/ ||
           scalars->GetNumberOfComponents() == 2 /*(LuminanceAlpha)*/))
        {
          int opacityIndex = scalars->GetNumberOfComponents() - 1;
          unsigned char opacity = 0;
          switch (scalars->GetDataType())
          {
            vtkTemplateMacro(
              vtkScalarsToColors::ColorToUChar(
                static_cast<VTK_TT>(scalars->GetRange(opacityIndex)[0]),
                &opacity));
          }
          if (opacity < 255)
          {
            // If the opacity is 255, despite the fact that the user specified
            // RGBA, we know that the Alpha is 100% opaque. So treat as opaque.
            this->LastOpaqueCheckValue = false;
            return false;
          }
        }
      }
    }
  }
  else if(this->CompositeAttributes &&
    this->CompositeAttributes->HasBlockOpacities())
  {
    this->LastOpaqueCheckValue = false;
    return false;
  }

  this->LastOpaqueCheckValue = this->Superclass::GetIsOpaque();
  return this->LastOpaqueCheckValue;
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
void vtkCompositePolyDataMapper2::SetBlockColor(unsigned int index, double color[3])
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockColor(index, color);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double* vtkCompositePolyDataMapper2::GetBlockColor(unsigned int index)
{
  (void) index;

  return 0;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockColor(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColor(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockColors()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockColors();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::SetBlockOpacity(unsigned int index, double opacity)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->SetBlockOpacity(index, opacity);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double vtkCompositePolyDataMapper2::GetBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    return this->CompositeAttributes->GetBlockOpacity(index);
  }
  return 1.;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockOpacity(unsigned int index)
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacity(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::RemoveBlockOpacities()
{
  if(this->CompositeAttributes)
  {
    this->CompositeAttributes->RemoveBlockOpacities();
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
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes*
vtkCompositePolyDataMapper2::GetCompositeDataDisplayAttributes()
{
  return this->CompositeAttributes;
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::UpdatePainterInformation()
{
  this->Superclass::UpdatePainterInformation();
  this->PainterInformation->Set(
    vtkCompositePainter::DISPLAY_ATTRIBUTES(),
    this->CompositeAttributes);
}

//----------------------------------------------------------------------------
void vtkCompositePolyDataMapper2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
