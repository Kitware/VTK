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

#include "vtkOTIncludes.h"
#include "vtkOTUtilities.h"

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
vtkOTKernelSmoothing::~vtkOTKernelSmoothing() {}

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
int vtkOTKernelSmoothing::Process(Sample* input)
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
void vtkOTKernelSmoothing::ComputePDF(
  Sample* input, KernelSmoothing* ks, double* range, const char* pdfName)
{
  ks->setBoundaryCorrection(this->BoundaryCorrection);

  Distribution dist = ks->build(*input);
  Sample gridX;
  Sample gridY = dist.getImplementation()->computePDF(range[0], range[1], this->PointNumber, gridX);
  this->AddToOutput(&gridY, pdfName);
}
