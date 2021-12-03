/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocatorInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellLocatorInterpolatedVelocityField.h"

#include "vtkAbstractCellLocator.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLocator.h"

vtkStandardNewMacro(vtkCellLocatorInterpolatedVelocityField);

//------------------------------------------------------------------------------
typedef std::vector<vtkSmartPointer<vtkAbstractCellLocator>> CellLocatorsTypeBase;
class vtkCellLocatorInterpolatedVelocityFieldCellLocatorsType : public CellLocatorsTypeBase
{
};

//------------------------------------------------------------------------------
vtkCellLocatorInterpolatedVelocityField::vtkCellLocatorInterpolatedVelocityField()
{
  // Create the default FindCellStrategy. Note that it is deleted by the
  // superclass.
  this->FindCellStrategy = vtkCellLocatorStrategy::New();

  // These variables are for backwards compatibility and should be deleted
  // one day.
  this->CellLocatorPrototype = nullptr;
}

//------------------------------------------------------------------------------
vtkCellLocatorInterpolatedVelocityField::~vtkCellLocatorInterpolatedVelocityField()
{
  this->SetCellLocatorPrototype(nullptr);
}

//------------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::SetCellLocatorPrototype(
  vtkAbstractCellLocator* prototype)
{
  // Make sure the find cell strategy is appropriate for using a
  // cell Locator
  if (!vtkCellLocatorStrategy::SafeDownCast(this->FindCellStrategy))
  {
    vtkNew<vtkCellLocatorStrategy> strat;
    this->SetFindCellStrategy(strat);
  }

  vtkCellLocatorStrategy::SafeDownCast(this->FindCellStrategy)->SetCellLocator(prototype);
}

//------------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::AddDataSet(vtkDataSet* dataset)
{
  if (!dataset)
  {
    vtkErrorMacro(<< "Dataset nullptr!");
    return;
  }

  // insert the dataset (do NOT register the dataset to 'this')
  this->DataSets->push_back(dataset);

  int size = dataset->GetMaxCellSize();
  if (size > this->WeightsSize)
  {
    this->WeightsSize = size;
    delete[] this->Weights;
    this->Weights = new double[size];
  }
}

//------------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::SetLastCellId(vtkIdType c, int dataindex)
{
  this->LastCellId = c;
  this->LastDataSet = (*this->DataSets)[dataindex];

  // If the dataset changes, then the cached cell is invalidated. We might as
  // well prefetch the cached cell either way.
  if (this->LastCellId != -1)
  {
    this->LastDataSet->GetCell(this->LastCellId, this->GenCell);
  }

  this->LastDataSetIndex = dataindex;
}

//------------------------------------------------------------------------------
int vtkCellLocatorInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  vtkDataSet* ds = nullptr;

  if (!this->LastDataSet && !this->DataSets->empty())
  {
    ds = (*this->DataSets)[0];
    this->LastDataSet = ds;
    this->LastDataSetIndex = 0;
  }
  else
  {
    ds = this->LastDataSet;
  }

  int retVal = this->FunctionValues(ds, x, f);

  if (!retVal)
  {
    for (this->LastDataSetIndex = 0;
         this->LastDataSetIndex < static_cast<int>(this->DataSets->size());
         this->LastDataSetIndex++)
    {
      ds = this->DataSets->operator[](this->LastDataSetIndex);
      if (ds && ds != this->LastDataSet)
      {
        this->ClearLastCellId();
        retVal = this->FunctionValues(ds, x, f);
        if (retVal)
        {
          this->LastDataSet = ds;
          return retVal;
        }
      }
    }

    this->LastCellId = -1;
    this->LastDataSetIndex = 0;
    this->LastDataSet = (*this->DataSets)[0];
    return 0;
  }

  return retVal;
}

//------------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
