/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractFunctionalBagPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractFunctionalBagPlot.h"

#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <algorithm>
#include <vector>

vtkStandardNewMacro(vtkExtractFunctionalBagPlot);

//-----------------------------------------------------------------------------
vtkExtractFunctionalBagPlot::vtkExtractFunctionalBagPlot()
{
  this->SetNumberOfInputPorts(2);
}

//-----------------------------------------------------------------------------
vtkExtractFunctionalBagPlot::~vtkExtractFunctionalBagPlot()
{
}

//-----------------------------------------------------------------------------
void vtkExtractFunctionalBagPlot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
class DensityVal
{
public:
  DensityVal(double d, vtkAbstractArray* arr) : Density(d), Array(arr) {}
  bool operator<(const DensityVal& b) const
  {
    return this->Density > b.Density;
  }
  double Density;
  vtkAbstractArray* Array;
};

//-----------------------------------------------------------------------------
int vtkExtractFunctionalBagPlot::RequestData(vtkInformation* /*request*/,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{

  vtkTable* inTable = vtkTable::GetData(inputVector[0]);
  vtkTable* inTableDensity = vtkTable::GetData(inputVector[1]);
  vtkTable* outTable = vtkTable::GetData(outputVector, 0);

  vtkIdType inNbColumns = inTable->GetNumberOfColumns();

  if (!inTable)
    {
    vtkDebugMacro(<< "Update event called with no input table.");
    return false;
    }

  if (!inTableDensity)
    {
    vtkDebugMacro(<< "Update event called with no density input table.");
    return false;
    }

  vtkDoubleArray *density = vtkDoubleArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess(0, inTableDensity));
  if (!density)
    {
    vtkDebugMacro(<< "Update event called with non double density array.");
    return false;
    }

  vtkStringArray *varName = vtkStringArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess(1, inTableDensity));
  if (!varName)
    {
    vtkDebugMacro(<< "Update event called with no variable name array.");
    return false;
    }

  vtkIdType nbPoints = varName->GetNumberOfValues();
  // Sort the density array
  std::vector<vtkIdType> ids;
  ids.resize(nbPoints);
  double sum = 0.0;
  for (vtkIdType i = 0; i < nbPoints; i++)
    {
    sum += density->GetTuple1(i);
    ids[i] = i;
    }

  vtkNew<vtkDoubleArray> nDensity;
  // Normalize the density array if needed
  if (fabs(sum - 1.0) > 1.0e-12)
    {
    sum = 1.0 / sum;
    nDensity->SetNumberOfComponents(1);
    nDensity->SetNumberOfTuples(nbPoints);
    for (vtkIdType i = 0; i < nbPoints; i++)
      {
      nDensity->SetTuple1(i, density->GetTuple1(ids[i]) * sum);
      }
    density = nDensity.GetPointer();
    }

  // Fetch and sort arrays according their density
  std::vector<DensityVal> varNames;
  for (int i = 0; i < varName->GetNumberOfValues(); i++)
    {
    varNames.push_back(
      DensityVal(density->GetValue(i),
        inTable->GetColumnByName(varName->GetValue(i))));
    }

  std::sort(varNames.begin(), varNames.end());

  std::vector<vtkAbstractArray*> medianLines;
  std::vector<vtkAbstractArray*> q3Lines;

  size_t nbVarNames = varNames.size();
  for (size_t i = 0; i < nbVarNames; i++)
    {
    if (i <= static_cast<size_t>(nbPoints * 0.5))
      {
      medianLines.push_back(varNames[i].Array);
      }
    if (i <= static_cast<size_t>(nbPoints * 0.75))
      {
      q3Lines.push_back(varNames[i].Array);
      }
    else
      {
      break;
      }
    }

  // Generate the quad strip arrays
  vtkIdType nbRows = inTable->GetNumberOfRows();

  vtkNew<vtkDoubleArray> q3Points;
  q3Points->SetName("Q3Points");
  q3Points->SetNumberOfComponents(2);
  q3Points->SetNumberOfTuples(nbRows);

  vtkNew<vtkDoubleArray> q2Points;
  q2Points->SetName("QMedPoints");
  q2Points->SetNumberOfComponents(2);
  q2Points->SetNumberOfTuples(nbRows);

  size_t medianCount = medianLines.size();
  size_t q3Count = q3Lines.size();
  for (vtkIdType i = 0; i < nbRows; i++)
    {
    double vMin = VTK_DOUBLE_MAX;
    double vMax = VTK_DOUBLE_MIN;
    for (size_t j = 0; j < medianCount; j++)
      {
      double v = medianLines[j]->GetVariantValue(i).ToDouble();
      if (v < vMin) { vMin = v; }
      if (v > vMax) { vMax = v; }
      }
    q2Points->SetTuple2(i, vMin, vMax);

    vMin = VTK_DOUBLE_MAX;
    vMax = VTK_DOUBLE_MIN;
    for (size_t j = 0; j < q3Count; j++)
      {
      double v = q3Lines[j]->GetVariantValue(i).ToDouble();
      if (v < vMin) { vMin = v; }
      if (v > vMax) { vMax = v; }
      }
    q3Points->SetTuple2(i, vMin, vMax);
    }

  // Add the 2 "bag" columns into the output table
  outTable->AddColumn(q3Points.GetPointer());
  outTable->AddColumn(q2Points.GetPointer());

  // Then append the input columns
  for (vtkIdType i = 0; i < inNbColumns; i++)
    {
    outTable->AddColumn(inTable->GetColumn(i));
    }

  return 1;
}
