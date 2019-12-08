/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutlineFilter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkPolyData.h"

#include <set>

class vtkOutlineFilter::vtkIndexSet : public std::set<unsigned int>
{
};

vtkStandardNewMacro(vtkOutlineFilter);

//----------------------------------------------------------------------------
vtkOutlineFilter::vtkOutlineFilter()
{
  this->GenerateFaces = 0;
  this->CompositeStyle = vtkOutlineFilter::ROOT_AND_LEAFS;
  this->OutputPointsPrecision = SINGLE_PRECISION;
  this->Indices = new vtkOutlineFilter::vtkIndexSet();
}

//----------------------------------------------------------------------------
vtkOutlineFilter::~vtkOutlineFilter()
{
  delete this->Indices;
}

//----------------------------------------------------------------------------
void vtkOutlineFilter::AddIndex(unsigned int index)
{
  if (this->Indices->find(index) == this->Indices->end())
  {
    this->Indices->insert(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOutlineFilter::RemoveIndex(unsigned int index)
{
  if (this->Indices->find(index) != this->Indices->end())
  {
    this->Indices->erase(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOutlineFilter::RemoveAllIndices()
{
  if (!this->Indices->empty())
  {
    this->Indices->clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOutlineFilter::AppendOutline(
  vtkPoints* points, vtkCellArray* lines, vtkCellArray* faces, double bounds[6])
{
  // Since points have likely been inserted before, keep track of the point
  // offsets
  vtkIdType offset[8];

  // Insert points
  double x[3];
  x[0] = bounds[0];
  x[1] = bounds[2];
  x[2] = bounds[4];
  offset[0] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[2];
  x[2] = bounds[4];
  offset[1] = points->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[3];
  x[2] = bounds[4];
  offset[2] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[3];
  x[2] = bounds[4];
  offset[3] = points->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[2];
  x[2] = bounds[5];
  offset[4] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[2];
  x[2] = bounds[5];
  offset[5] = points->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[3];
  x[2] = bounds[5];
  offset[6] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[3];
  x[2] = bounds[5];
  offset[7] = points->InsertNextPoint(x);

  // Produce topology, either lines or quad faces
  vtkIdType pts[4];

  // Always generate wire edges. I think this is dumb but historical....
  pts[0] = offset[0];
  pts[1] = offset[1];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[2];
  pts[1] = offset[3];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[4];
  pts[1] = offset[5];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[6];
  pts[1] = offset[7];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[0];
  pts[1] = offset[2];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[1];
  pts[1] = offset[3];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[4];
  pts[1] = offset[6];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[5];
  pts[1] = offset[7];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[0];
  pts[1] = offset[4];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[1];
  pts[1] = offset[5];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[2];
  pts[1] = offset[6];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[3];
  pts[1] = offset[7];
  lines->InsertNextCell(2, pts);

  if (this->GenerateFaces)
  {
    pts[0] = offset[1];
    pts[1] = offset[0];
    pts[2] = offset[2];
    pts[3] = offset[3];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[0];
    pts[1] = offset[1];
    pts[2] = offset[5];
    pts[3] = offset[4];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[2];
    pts[1] = offset[0];
    pts[2] = offset[4];
    pts[3] = offset[6];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[3];
    pts[1] = offset[2];
    pts[2] = offset[6];
    pts[3] = offset[7];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[1];
    pts[1] = offset[3];
    pts[2] = offset[7];
    pts[3] = offset[5];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[7];
    pts[1] = offset[6];
    pts[2] = offset[4];
    pts[3] = offset[5];
    faces->InsertNextCell(4, pts);
  }
}

//----------------------------------------------------------------------------
int vtkOutlineFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Differentiate between composite and typical datasets.
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkCompositeDataSet* compInput =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkCompositeDataSet::DATA_OBJECT()));
  if (input == nullptr && compInput == nullptr)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Creating outline");

  // Each outline is passed down to the core generation function
  vtkNew<vtkPoints> pts;
  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    pts->SetDataType(VTK_DOUBLE);
  }
  else
  {
    pts->SetDataType(VTK_FLOAT);
  }

  vtkNew<vtkCellArray> lines;
  vtkNew<vtkCellArray> faces;

  // If vtkDataSet input just create one bounding box. Composites may require multiple.
  double bds[6];
  if (input)
  {
    // dataset bounding box
    input->GetBounds(bds);
    this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
  }
  else
  {
    // Root bounding box
    if (this->CompositeStyle == ROOT_LEVEL || this->CompositeStyle == ROOT_AND_LEAFS)
    {
      compInput->GetBounds(bds);
      this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
    }

    // Leafs
    if (this->CompositeStyle == LEAF_DATASETS || this->CompositeStyle == ROOT_AND_LEAFS)
    {
      vtkCompositeDataIterator* iter = compInput->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (ds)
        {
          ds->GetBounds(bds);
          this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
        }
      }
      iter->Delete();
    }

    // Specified flat indices
    if (this->CompositeStyle == SPECIFIED_INDEX)
    {
      vtkCompositeDataIterator* iter = compInput->NewIterator();
      vtkDataObjectTreeIterator* treeIter = vtkDataObjectTreeIterator::SafeDownCast(iter);
      if (treeIter)
      {
        treeIter->VisitOnlyLeavesOff();
      }
      unsigned int index;
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (ds)
        {
          index = iter->GetCurrentFlatIndex();
          if (this->Indices->find(index) != this->Indices->end())
          {
            ds->GetBounds(bds);
            this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
          }
        } // if a dataset
      }
      iter->Delete();
    }
  }

  // Specify output
  output->SetPoints(pts.GetPointer());
  output->SetLines(lines.GetPointer());

  if (this->GenerateFaces)
  {
    output->SetPolys(faces.GetPointer());
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkOutlineFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Faces: " << (this->GenerateFaces ? "On\n" : "Off\n");
  os << indent << "Composite Style: " << this->CompositeStyle << endl;
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
  os << indent
     << "Composite indices: " << (!this->Indices->empty() ? "(Specified)\n" : "(Not specified)\n");
}
