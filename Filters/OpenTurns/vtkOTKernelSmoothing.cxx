/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTKernelSmoothing.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOTKernelSmoothing.h"

#include "vtkObjectFactory.h"

#include "openturns/DistributionFactoryImplementation.hxx"
#include "openturns/DistributionImplementation.hxx"
#include "openturns/Epanechnikov.hxx"
#include "openturns/KernelSmoothing.hxx"
#include "openturns/NumericalPoint.hxx"
#include "openturns/NumericalSample.hxx"
#include "openturns/Triangular.hxx"

vtkStandardNewMacro(vtkOTKernelSmoothing);

using namespace OT;

//-----------------------------------------------------------------------------
vtkOTKernelSmoothing::vtkOTKernelSmoothing()
{
  this->PointNumber = 129;
  this->GaussianPDF = true;
  this->TriangularPDF = false;
  this->EpanechnikovPDF = false;
  this->BoundaryCorrection = false;
}

//-----------------------------------------------------------------------------
vtkOTKernelSmoothing::~vtkOTKernelSmoothing()
{
}

//-----------------------------------------------------------------------------
void vtkOTKernelSmoothing::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PointNumber: " << this->PointNumber << endl;
  os << indent << "GaussianPDF: " << this->GaussianPDF << endl;
  os << indent << "TriangularPDF: " << this->TriangularPDF << endl;
  os << indent << "EpanechnikovPDF: " << this->EpanechnikovPDF << endl;
  os << indent << "BoundaryCorrection: " << this->BoundaryCorrection << endl;
}

//-----------------------------------------------------------------------------
int vtkOTKernelSmoothing::Process(NumericalSample* input)
{
  double range[2] = { input->getMin()[0], input->getMax()[0] };
  double enlarger = 0.05 * (range[1] - range[0]);
  range[0] -= enlarger;
  range[1] += enlarger;

  if (this->GaussianPDF)
  {
    KernelSmoothing* ks = new KernelSmoothing();
    this->ComputePDF(input, ks, range, "Gaussian");
    delete ks;
  }

  if (this->TriangularPDF)
  {
    KernelSmoothing* ks = new KernelSmoothing(Triangular());
    this->ComputePDF(input, ks, range, "Triangular");
    delete ks;
  }

  if (this->EpanechnikovPDF)
  {
    KernelSmoothing* ks = new KernelSmoothing(Epanechnikov());
    this->ComputePDF(input, ks, range, "Epanechnikov");
    delete ks;
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkOTKernelSmoothing::ComputePDF(NumericalSample* input,
  KernelSmoothing* ks,
  double* range,
  const char* pdfName)
{
  ks->setBoundaryCorrection(this->BoundaryCorrection);

  DistributionFactoryImplementation::Implementation dist = ks->build(*input);
  NumericalSample gridX;
  NumericalSample gridY = dist->computePDF(range[0], range[1], this->PointNumber, gridX);
  this->AddToOutput(&gridY, pdfName);
}
