/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHighestDensityRegionsStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHighestDensityRegionsStatistics.h"

#include "vtkDataArrayCollection.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkTable.h"

#include <algorithm>
#include <set>
#include <sstream>

vtkStandardNewMacro(vtkHighestDensityRegionsStatistics);

// ----------------------------------------------------------------------
vtkHighestDensityRegionsStatistics::vtkHighestDensityRegionsStatistics()
{
  // Initialize H smooth matrix to Identity.
  this->SmoothHC1[0] = 1.0;
  this->SmoothHC1[1] = 0.0;
  this->SmoothHC2[0] = 0.0;
  this->SmoothHC2[1] = 1.0;

  //  At the construction, no columns pair are requested yet
  this->NumberOfRequestedColumnsPair = 0;
}

// ----------------------------------------------------------------------
vtkHighestDensityRegionsStatistics::~vtkHighestDensityRegionsStatistics()
{
}

// ----------------------------------------------------------------------
void vtkHighestDensityRegionsStatistics::PrintSelf(ostream& os,
                                                   vtkIndent indent)
  {
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Smooth matrix: " <<
    this->SmoothHC1[0] << ", " <<
    this->SmoothHC1[1] << ", " <<
    this->SmoothHC2[0] << ", " <<
    this->SmoothHC2[1] << "\n";
  }

// ----------------------------------------------------------------------
void vtkHighestDensityRegionsStatistics::SetSigma(double sigma)
{
  if (this->SmoothHC1[0] == sigma &&
    this->SmoothHC1[1] == 0.0 &&
    this->SmoothHC2[0] == 0.0 &&
    this->SmoothHC2[1] == sigma)
    {
    return;
    }
  // Force H matrix to be equal to sigma * Identity.
  this->SmoothHC1[0] = sigma;
  this->SmoothHC1[1] = 0.0;
  this->SmoothHC2[0] = 0.0;
  this->SmoothHC2[1] = sigma;
  this->Modified();
}

// ----------------------------------------------------------------------
void vtkHighestDensityRegionsStatistics::Learn(vtkTable* inData,
                                               vtkTable* vtkNotUsed(inParameters),
                                               vtkMultiBlockDataSet* outMeta)
{
  if (!inData || !outMeta)
    {
    return;
    }

  vtkNew<vtkTable> outputColumns;

  std::set<std::set<vtkStdString> >::const_iterator reqIt;

  // Make sure the number of requested pairs of columns is 0
  // before the computation.
  this->NumberOfRequestedColumnsPair = 0;

  // Populate outputColumns with columns that are requested from
  // the input dataset
  for (reqIt = this->Internals->Requests.begin();
    reqIt != this->Internals->Requests.end(); ++ reqIt)
    {
    // Each request contains only one pair of columns of interest
    // (if there are others, they are ignored).
    std::set<vtkStdString>::const_iterator colIt = reqIt->begin();
    const vtkStdString &colY = *colIt;
    if (!inData->GetColumnByName(colY.c_str()))
      {
      vtkWarningMacro("InData table does not have a column "
        << colY.c_str()
        << ". Ignoring this pair.");
      continue;
      }

    ++colIt;
    const vtkStdString &colX = *colIt;
    if (!inData->GetColumnByName(colX.c_str()))
      {
      vtkWarningMacro("InData table does not have a column "
        << colX.c_str()
        << ". Ignoring this pair.");
      continue;
      }

    // Verify column types
    vtkDataArray *inputColX =
      vtkDataArray::SafeDownCast(inData->GetColumnByName(colX.c_str()));
    vtkDataArray *inputColY =
      vtkDataArray::SafeDownCast(inData->GetColumnByName(colY.c_str()));
    if (!inputColX || !inputColY)
      {
      vtkErrorMacro(
        << "HDR cannot work with columns that are not of vtkDataArray type");
      return;
      }

    vtkDataArray* arrX =
      vtkDataArray::CreateDataArray(inputColX->GetDataType());
    arrX->DeepCopy(inputColX);
    arrX->SetName(inputColX->GetName());
    outputColumns->AddColumn(arrX);

    vtkDataArray* arrY =
      vtkDataArray::CreateDataArray(inputColY->GetDataType());
    arrY->DeepCopy(inputColY);
    arrY->SetName(inputColY->GetName());
    outputColumns->AddColumn(arrY);

    // Compute for the two columns and each observations the estimator of
    // density. Create a double Array that contains number of requested data
    // series components. Each tuple will contain the correspondent value
    // casted if necessary into a double.

    vtkNew<vtkDoubleArray> inObservations;
    inObservations->SetNumberOfComponents(2);
    inObservations->SetNumberOfTuples(outputColumns->GetNumberOfRows());

    inObservations->CopyComponent(0, inputColX, 0);
    inObservations->CopyComponent(1, inputColY, 0);

    // outObservations store the density vector
    vtkDataArray* outObservations =
      vtkDataArray::CreateDataArray(inObservations->GetDataType());
    outObservations->SetNumberOfComponents(1);
    outObservations->SetNumberOfTuples(inObservations->GetNumberOfTuples());

    this->ComputeHDR(inObservations.GetPointer(), outObservations);
    std::stringstream ss;
    ss <<"HDR (" << inputColX->GetName() << "," << inputColY->GetName() << ")";
    outObservations->SetName(ss.str().c_str());
    outputColumns->AddColumn(outObservations);

    arrX->Delete();
    arrY->Delete();
    outObservations->Delete();

    // One requested pair of columns has been added.
    this->NumberOfRequestedColumnsPair++;
    } // End requests iteration.

  outMeta->SetNumberOfBlocks(1);
  outMeta->SetBlock(0, outputColumns.GetPointer());
  vtkInformation* info =
    outMeta->GetMetaData(static_cast<unsigned int>(0));
  info->Set(vtkCompositeDataSet::NAME(), "Estimator of density Data");
}

// ----------------------------------------------------------------------
void vtkHighestDensityRegionsStatistics::Derive(vtkMultiBlockDataSet*)
{
}

// ----------------------------------------------------------------------
double vtkHighestDensityRegionsStatistics::ComputeHDR(vtkDataArray *inObs,
                                                      vtkDataArray *outDensity)
{
  vtkIdType nbObservations = inObs->GetNumberOfTuples();

  if (nbObservations == 0)
    {
    vtkErrorMacro(<< "Empty observation array");
    return 0.0;
    }
  double sum = 0.0;

  double denom = 1.0 / static_cast<double>(nbObservations);

  // Let's compute the HDR for each points of the observations
  for (vtkIdType i = 0; i < nbObservations; i++)
    {
    double currentXi[2];
    double currentXj[2];
    double hdr = 0.0;

    // We are working in a bivariate model.
    inObs->GetTuple(i, currentXi);
    // Sum all gaussian kernel
    for (vtkIdType j = 0; j < nbObservations; j++)
      {
      // Avoid case where point is compared to itself
      if (i == j)
        {
        continue;
        }
      inObs->GetTuple(j, currentXj);

      hdr += this->ComputeSmoothGaussianKernel(
        inObs->GetNumberOfComponents(),
        currentXi[0] - currentXj[0],
        currentXi[1] - currentXj[1]);
      }
    double d = denom * hdr;
    outDensity->SetTuple1(i, d);
    sum += d;
    }

  return sum;
}

// ----------------------------------------------------------------------
double vtkHighestDensityRegionsStatistics::ComputeSmoothGaussianKernel(
  int dimension, double khx, double khy)
{
  double HDeterminant =
    vtkMath::Determinant2x2(this->SmoothHC1, this->SmoothHC2);
  if (HDeterminant > 0.0)
    {
    HDeterminant = 1.0 / sqrt(HDeterminant);
    }

  // We need to multiply the input vector by the smooth square root of
  // H matrix parameter: sqrt(H) * [khx, khy] -> random vector of the
  // standard gaussian input.

  // If a H coefficient is equal to 0.0. we don't compute its sqrt to avoid
  // domain error.
  double SHC10 = 0.0;
  double SHC11 = 0.0;
  double SHC20 = 0.0;
  double SHC21 = 0.0;

  if (this->SmoothHC1[0] != 0.0)
    {
    SHC10 = 1.0 / sqrt(this->SmoothHC1[0]);
    }
  if (this->SmoothHC1[1] != 0.0)
    {
    SHC11 = 1.0 / sqrt(this->SmoothHC1[1]);
    }
  if (this->SmoothHC2[0] != 0.0)
    {
    SHC20 = 1.0 / sqrt(this->SmoothHC2[0]);
    }
  if (this->SmoothHC2[1] != 0.0)
    {
    SHC21 = 1.0 / sqrt(this->SmoothHC2[1]);
    }

  // Call the standard gaussian kernel with the new random vector.
  return HDeterminant *
    this->ComputeStandardGaussianKernel(dimension,
    SHC10 * khx + SHC11 * khy,
    SHC20 * khx + SHC21 * khy);
}

// ----------------------------------------------------------------------
double vtkHighestDensityRegionsStatistics::ComputeStandardGaussianKernel(
  int vtkNotUsed(dimension), double kx, double ky)
{
  return exp(-(kx * kx + ky * ky) / 2.0) / (2.0 * vtkMath::Pi());
}
