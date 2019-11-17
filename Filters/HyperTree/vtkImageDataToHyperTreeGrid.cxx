/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToHyperTreeGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDataToHyperTreeGrid.h"

#include <vtkBitArray.h>
#include <vtkCellData.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTree.h>
#include <vtkHyperTreeGrid.h>
#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>

#include <cmath>

#include "vtkHyperTreeGridNonOrientedCursor.h"

vtkStandardNewMacro(vtkImageDataToHyperTreeGrid);

//-----------------------------------------------------------------------------
vtkImageDataToHyperTreeGrid::vtkImageDataToHyperTreeGrid()
{
  this->NbColors = 256;
  this->DepthMax = 0;
}

//-----------------------------------------------------------------------------
vtkImageDataToHyperTreeGrid::~vtkImageDataToHyperTreeGrid() {}

//----------------------------------------------------------------------------
void vtkImageDataToHyperTreeGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NbColors : " << this->NbColors << endl;
  os << indent << "DepthMax : " << this->DepthMax << endl;
}

int vtkImageDataToHyperTreeGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Update progress
  this->UpdateProgress(0.);

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::GetData(outputVector, 0);
  if (!output)
  {
    return 0;
  }
  vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
  if (!input)
  {
    return 0;
  }

  this->InScalars = this->GetInputArrayToProcess(0, input);
  if (!this->InScalars)
  {
    vtkWarningMacro(<< "No scalar data to process");
    return 1;
  }

  int inSize[3];
  input->GetDimensions(inSize);

  int pow2 = pow(2, this->DepthMax);
  unsigned int size[3] = { (unsigned int)inSize[0] / pow2, (unsigned int)inSize[1] / pow2, 1 };
  if ((int)(size[0] * pow2) != inSize[0])
    size[0] = size[0] + 1;
  if ((int)(size[1] * pow2) != inSize[1])
    size[1] = size[1] + 1;

  size[0] += 1;
  size[1] += 1;
  output->SetDimensions(size); // JB ce n'est pas la GridCell
  size[0] -= 1;
  size[1] -= 1;

  output->SetBranchFactor(2);

  vtkNew<vtkDoubleArray> coordX;
  coordX->SetNumberOfValues(size[0] + 1);
  for (unsigned int i = 0; i <= size[0]; ++i)
  {
    coordX->SetValue(i, i);
  }
  output->SetXCoordinates(coordX);

  vtkNew<vtkDoubleArray> coordY;
  coordY->SetNumberOfValues(size[1] + 1);
  for (unsigned int i = 0; i <= size[1]; ++i)
  {
    coordY->SetValue(i, i);
  }
  output->SetYCoordinates(coordY);

  vtkNew<vtkDoubleArray> coordZ;
  coordZ->SetNumberOfValues(2);
  coordZ->SetValue(0, 0);
  coordZ->SetValue(1, 0);
  output->SetZCoordinates(coordZ);

  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  this->Color = vtkUnsignedCharArray::New();
  this->Color->SetName("Colors");
  this->Color->SetNumberOfComponents(3);

  this->Depth = vtkDoubleArray::New();
  this->Depth->SetName("Depth");
  this->Depth->SetNumberOfComponents(1);

  this->Mask = vtkBitArray::New();
  this->Mask->SetName("Mask");
  this->Mask->SetNumberOfComponents(1);

  output->SetMask(this->Mask);

  this->GlobalId = 0;
  this->OutData->AddArray(this->Color);
  this->OutData->AddArray(this->Depth);

  vtkIdType nbTrees = output->GetMaxNumberOfTrees();
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  for (vtkIdType itree = 0; itree < nbTrees; ++itree)
  {
    vtkIdType index = itree;

    unsigned int i, j, k;
    output->GetLevelZeroCoordinatesFromIndex(index, i, j, k);

    output->InitializeNonOrientedCursor(cursor, index, true);

    vtkNew<vtkIntArray> pixels;
    int nbPxl = pow(2, DepthMax);
    pixels->SetNumberOfValues(nbPxl * nbPxl);
    int id = 0;
    unsigned char pas = 256 / this->NbColors;
    for (int pj = 0; pj < nbPxl; ++pj)
    {
      for (int pi = 0; pi < nbPxl; ++pi, ++id)
      {
        int x = i * nbPxl + pi;
        int y = j * nbPxl + pj;
        if (x < inSize[0] && y < inSize[1])
        {
          unsigned char* val = static_cast<unsigned char*>(input->GetScalarPointer(x, y, 0));
          int grp = (val[0] / pas) + (val[1] / pas) * this->NbColors +
            (val[2] / pas) * this->NbColors * this->NbColors;
          pixels->SetValue(id, grp);
        }
        else
        {
          pixels->SetValue(id, -1);
        }
      }
    }
    ProcessPixels(pixels, cursor);
  }

  // Update progress and return
  this->UpdateProgress(1.);
  return 1;
}

void vtkImageDataToHyperTreeGrid::ProcessPixels(
  vtkIntArray* grps, vtkHyperTreeGridNonOrientedCursor* cursor)
{
  int nbPixel = grps->GetNumberOfValues();
  int val = grps->GetTuple1(0);
  bool raf = false;
  for (int i = 0; i < nbPixel && !raf; ++i)
  {
    if (val != grps->GetTuple1(i))
    {
      raf = true;
      break;
    }
  }

  int car = this->NbColors * this->NbColors;
  unsigned char pas = 256 / this->NbColors;

  this->Color->InsertTuple3(this->GlobalId, pas * (unsigned char)((val % car) % this->NbColors),
    pas * (unsigned char)((val % car) / this->NbColors), pas * (unsigned char)(val / car));

  this->Depth->InsertTuple1(this->GlobalId, cursor->GetLevel());

  if (val < 0)
  {
    this->Mask->InsertTuple1(this->GlobalId, true);
  }
  else
  {
    this->Mask->InsertTuple1(this->GlobalId, false);
  }

  cursor->SetGlobalIndexFromLocal(this->GlobalId++);
  if (raf)
  {
    cursor->SubdivideLeaf();
    int ichild = 0;
    for (int j = 0; j < 2; ++j)
    {
      for (int i = 0; i < 2; ++i, ++ichild)
      {
        cursor->ToChild(ichild);

        vtkNew<vtkIntArray> childPix;
        int nbPxl = sqrt(nbPixel) / 2;
        childPix->SetNumberOfValues(nbPxl * nbPxl);
        int id = 0;
        for (int pj = 0; pj < nbPxl; ++pj)
        {
          for (int pi = 0; pi < nbPxl; ++pi, ++id)
          {
            int grp = grps->GetTuple1(i * nbPxl + pi + (j * nbPxl + pj) * 2 * nbPxl);
            childPix->SetValue(id, grp);
          }
        }
        ProcessPixels(childPix, cursor);

        cursor->ToParent();
      }
    }
  }

  return;
}

int vtkImageDataToHyperTreeGrid::ProcessTrees(
  vtkHyperTreeGrid* vtkNotUsed(output), vtkDataObject* vtkNotUsed(input))
{
  return 1;
}

//-----------------------------------------------------------------------------
int vtkImageDataToHyperTreeGrid::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkImageDataToHyperTreeGrid::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
