/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNITransformWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include "vtkMNITransformWriter.h"

#include "vtkObjectFactory.h"

#include "vtkMath.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkMINCImageAttributes.h"
#include "vtkMINCImageWriter.h"
#include "vtkMINC.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCollection.h"
#include "vtkTransform.h"
#include "vtkHomogeneousTransform.h"
#include "vtkGeneralTransform.h"
#include "vtkThinPlateSplineTransform.h"
#include "vtkGridTransform.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPoints.h"

#include <ctime>
#include <ctype.h>
#include <stdio.h>

#include <stack>
#include <vtksys/SystemTools.hxx>

//--------------------------------------------------------------------------
vtkStandardNewMacro(vtkMNITransformWriter);

//-------------------------------------------------------------------------
vtkMNITransformWriter::vtkMNITransformWriter()
{
  this->FileName = 0;
  this->Transform = 0;
  this->Transforms = vtkCollection::New();
  this->Comments = 0;
}

//-------------------------------------------------------------------------
vtkMNITransformWriter::~vtkMNITransformWriter()
{
  if (this->Transforms)
    {
    this->Transforms->Delete();
    }
  if (this->Transform)
    {
    this->Transform->Delete();
    }
  delete [] this->FileName;
  delete [] this->Comments;
}

//-------------------------------------------------------------------------
void vtkMNITransformWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "none") << "\n";
  os << indent << "Transform: " << this->Transform << "\n";
  if (this->Transform)
    {
    this->Transform->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "NumberOfTransforms: "
     << this->Transforms->GetNumberOfItems() << "\n";
  os << indent << "Comments: "
     << (this->Comments ? this->Comments : "none") << "\n";
}

//-------------------------------------------------------------------------
int vtkMNITransformWriter::WriteLinearTransform(
  ostream &outfile, vtkHomogeneousTransform *transform)
{
  vtkMatrix4x4 *matrix = transform->GetMatrix();

  if (matrix->GetElement(3,0) != 0.0 ||
      matrix->GetElement(3,1) != 0.0 ||
      matrix->GetElement(3,2) != 0.0 ||
      matrix->GetElement(3,3) != 1.0)
    {
    vtkErrorMacro("WriteLinearTransform: The transform is not linear");
    return 0;
    }

  outfile << "Linear_Transform =";

  char text[256];
  for (int i = 0; i < 3; i++)
    {
    outfile << "\n";
    sprintf(text, " %.15g %.15g %.15g %.15g",
            matrix->GetElement(i, 0),
            matrix->GetElement(i, 1),
            matrix->GetElement(i, 2),
            matrix->GetElement(i, 3));
    outfile << text;
    }
  outfile << ";\n";

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformWriter::WriteThinPlateSplineTransform(
  ostream &outfile, vtkThinPlateSplineTransform *transform)
{
  // Write the inverse flag if necessary
  if (transform->GetInverseFlag())
    {
    outfile << "Invert_Flag = True;\n";
    }

  // Get the landmarks
  vtkPoints *source = transform->GetSourceLandmarks();
  vtkPoints *target = transform->GetTargetLandmarks();
  int n = source->GetNumberOfPoints();

  if (target->GetNumberOfPoints() != n)
    {
    // An error will be printed out by the transform Update
    return 0;
    }

  // Find the dimensionality of the transform
  int i, j, k;
  int ndim = 1;
  for (i = 0; i < n; i++)
    {
    double p1[3], p2[3];
    source->GetPoint(i, p1);
    target->GetPoint(i, p2);
    if (ndim == 1)
      {
      if (p1[1] != 0.0 || p2[1] != 0.0)
        {
        ndim = 2;
        }
      if (ndim == 2)
        {
        if (p1[2] != 0.0 || p2[2] != 0.0)
          {
          ndim = 3;
          break;
          }
        }
      }
    }

  // Make sure the dimensionality is correct
  if (ndim == 1)
    {
    vtkErrorMacro("Thin plate spline points are co-linear!");
    return 0;
    }
  if (ndim == 2 && transform->GetBasis() != VTK_RBF_R2LOGR)
    {
    vtkErrorMacro("Spline dimensionality is 2, but RBF is not R2LogR!");
    return 0;
    }
  if (ndim == 3 && transform->GetBasis() != VTK_RBF_R)
    {
    vtkErrorMacro("Spline dimensionality is 3, but RBF is not R!");
    return 0;
    }

  // Write out the number of dimensions
  outfile << "Number_Dimensions = " << ndim << ";\n";

  // Write out the points
  outfile << "Points =";
  for (i = 0; i < n; i++)
    {
    double p[3];
    source->GetPoint(i, p);

    outfile << "\n";

    for (j = 0; j < ndim; j++)
      {
      char text[64];
      sprintf(text, " %.15g", p[j]);

      outfile << text;
      }
    }

  outfile << ";\n";

  // Create two matrices
  int msize = n + ndim + 1;
  double **X = new double *[ndim];
  double **L = new double *[msize];
  int storagelen = ndim*msize + msize*msize;
  double *storage = new double[storagelen];

  for (i = 0; i < storagelen; i++)
    {
    storage[i] = 0.0;
    }

  // Create the X and L matrices
  for (i = 0; i < ndim; i++)
    {
    X[i] = &storage[i*msize];
    }
  for (i = 0; i < msize; i++)
    {
    L[i] = &storage[ndim*msize + i*msize];
    }

  // Fill in L matrix
  for (i = 0; i < n; i++)
    {
    double p[3];
    source->GetPoint(i, p);
    L[n][i] = L[i][n] = 1.0;
    for (k = 0; k < ndim; k++)
      {
      L[n + k + 1][i] = L[i][n + k + 1] = p[k];
      }
    for (j = 0; j < i; j++)
      {
      double p1[3];
      source->GetPoint(j, p1);
      double r = 0.0;
      for (k = 0; k < ndim; k++)
        {
        r += (p[k] - p1[k])*(p[k] - p1[k]);
        }
      r = sqrt(r);
      if (ndim == 2)
        {
        r = r*r*log(r);
        }
      L[i][j] = L[j][i] = r;
      }
    }

  // Fill in X matrix
  for (i = 0; i < n; i++)
    {
    double *p = target->GetPoint(i);
    for (k = 0; k < ndim; k++)
      {
      X[k][i] = p[k];
      }
    }

  // Solve to make X into the thin-plate spline matrix
  int *pivots = new int[msize];
  double *tmpstore = new double[msize];
  vtkMath::LUFactorLinearSystem(L, pivots, msize, tmpstore);
  for (i = 0; i < ndim; i++)
    {
    vtkMath::LUSolveLinearSystem(L, pivots, X[i], msize);
    }
  delete [] tmpstore;
  delete [] pivots;

  // Write out the matrix as "Displacements"
  outfile << "Displacements =";
  for (i = 0; i < msize; i++)
    {
    outfile << "\n";

    for (j = 0; j < ndim; j++)
      {
      char text[64];
      sprintf(text, " %.15g", X[j][i]);

      outfile << text;
      }
    }

  outfile << ";\n";

  delete [] storage;
  delete [] L;
  delete [] X;

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformWriter::WriteGridTransform(
  ostream &outfile, vtkGridTransform *transform)
{
  // Write the inverse flag if necessary
  if (transform->GetInverseFlag())
    {
    outfile << "Invert_Flag = True;\n";
    }

  // Split FileName into directory and filename
  std::vector<std::string> xfmpath;
  vtksys::SystemTools::SplitPath(this->FileName, xfmpath);
  if (xfmpath.size() < 1)
    {
    vtkErrorMacro("Can't split filename " << this->FileName);
    return 0;
    }

  // Replace the ".xfm" extension of the filename with "_grid.mnc"
  size_t i = xfmpath.size() - 1;
  std::string filename =
    vtksys::SystemTools::GetFilenameWithoutLastExtension(xfmpath[i]);
  filename.append("_grid.mnc");
  xfmpath[i] = filename;

  // Write the minc filename into the .xfm file
  outfile << "Displacement_Volume = " << filename << ";\n";

  // Use the full path to write the minc file
  vtkMINCImageWriter *writer = vtkMINCImageWriter::New();
  writer->SetFileName(vtksys::SystemTools::JoinPath(xfmpath).c_str());
  writer->SetInputData(transform->GetDisplacementGrid());
  if (transform->GetDisplacementShift() != 0.0 ||
      transform->GetDisplacementScale() != 1.0)
    {
    writer->SetRescaleIntercept(transform->GetDisplacementShift());
    writer->SetRescaleSlope(transform->GetDisplacementScale());
    }

  // Write the file
  writer->Write();
  writer->Delete();

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformWriter::WriteTransform(
  ostream &outfile, vtkAbstractTransform *transform)
{
  outfile << "Transform_Type = ";

  if (transform->IsA("vtkHomogeneousTransform"))
    {
    outfile << "Linear;\n";
    return this->WriteLinearTransform(
      outfile, (vtkHomogeneousTransform *)transform);
    }
  else if (transform->IsA("vtkThinPlateSplineTransform"))
    {
    outfile << "Thin_Plate_Spline_Transform;\n";
    return this->WriteThinPlateSplineTransform(
      outfile, (vtkThinPlateSplineTransform *)transform);
    }
  else if (transform->IsA("vtkGridTransform"))
    {
    outfile << "Grid_Transform;\n";
    return this->WriteGridTransform(
      outfile, (vtkGridTransform *)transform);
    }

  vtkErrorMacro("Unsupported transform type "
                << transform->GetClassName());

  return 0;
}

//-------------------------------------------------------------------------
int vtkMNITransformWriter::WriteFile()
{
  // Check that a transform has been set.
  if (!this->Transform)
    {
    vtkErrorMacro("WriteFile: No input transform has been set.");
    return 0;
    }
  // Check that the file name has been set.
  if (!this->FileName)
    {
    vtkErrorMacro("WriteFile: No file name has been set.");
    return 0;
    }

  // Open the file.
  ofstream outfile(this->FileName, ios::out);

  if (outfile.fail())
    {
    vtkErrorMacro("WriteFile: Can't create the file " << this->FileName);
    return 0;
    }

  // Write the header
  outfile << "MNI Transform File\n";

  // Get the local time and write as first comment line
  char buf[1024];
  time_t t;
  time(&t);
  strftime(buf, sizeof(buf), "%Y:%m:%d %H:%M:%S", localtime(&t));

  outfile << "% Creation time: " << buf << "\n";

  // Write user comments
  if (this->Comments)
    {
    char *cp = this->Comments;
    while (*cp)
      {
      if (*cp != '%')
        {
        outfile << "% ";
        }
      while (*cp && *cp != '\n')
        {
        if (isprint(*cp) || *cp == '\t')
          {
          outfile << *cp;
          }
        cp++;
        }
      outfile << "\n";
      if (*cp == '\n')
        {
        cp++;
        }
      }
    }

  // Add a blank line
  outfile << "\n";

  // Push the transforms onto the stack in reverse order
  std::stack<vtkAbstractTransform *> tstack;
  int i = this->Transforms->GetNumberOfItems();
  while (i > 0)
    {
    tstack.push(
      ((vtkAbstractTransform *)this->Transforms->GetItemAsObject(--i)));
    }
  tstack.push(this->Transform);

  // Write out all the transforms on the stack
  int status = 1;
  while (status != 0 && !tstack.empty())
    {
    vtkAbstractTransform *transform = tstack.top();
    tstack.pop();

    if (transform->IsA("vtkGeneralTransform"))
      {
      // Decompose general transforms
      vtkGeneralTransform *gtrans = (vtkGeneralTransform *)transform;
      int n = gtrans->GetNumberOfConcatenatedTransforms();
      while (n > 0)
        {
        tstack.push(gtrans->GetConcatenatedTransform(--n));
        }
      }
    else
      {
      // Write all other kinds of transforms
      status = this->WriteTransform(outfile, transform);
      }
    }

  outfile.close();

  if (status == 0)
    {
    // delete file
    }

  return status;
}

//-------------------------------------------------------------------------
int vtkMNITransformWriter::ProcessRequest(vtkInformation *request,
                                    vtkInformationVector **inputVector,
                                    vtkInformationVector *outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    if (this->Transform)
      {
      this->Transform->Update();
      }
    int n = this->Transforms->GetNumberOfItems();
    for (int i = 0; i < n; i++)
      {
      ((vtkAbstractTransform *)this->Transforms->GetItemAsObject(i))
        ->Update();
      }
    return this->WriteFile();
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-------------------------------------------------------------------------
void vtkMNITransformWriter::Write()
{
  this->Modified();
  this->Update();
}

//-------------------------------------------------------------------------
int vtkMNITransformWriter::GetNumberOfTransforms()
{
  if (this->Transform == 0)
    {
    return 0;
    }

  return (1 + this->Transforms->GetNumberOfItems());
}

//-------------------------------------------------------------------------
void vtkMNITransformWriter::SetTransform(vtkAbstractTransform *transform)
{
  if (transform == this->Transform)
    {
    return;
    }

  if (this->Transform != 0)
    {
    this->Transform->Delete();
    }

  if (transform != 0)
    {
    transform->Register(this);
    }

  this->Transform = transform;
  this->Transforms->RemoveAllItems();
  this->Modified();
}

//-------------------------------------------------------------------------
void vtkMNITransformWriter::AddTransform(vtkAbstractTransform *transform)
{
  if (transform == 0)
    {
    return;
    }

  if (this->Transform == 0)
    {
    this->SetTransform(transform);
    }
  else
    {
    this->Transforms->AddItem(transform);
    this->Modified();
    }
}
