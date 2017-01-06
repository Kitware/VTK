/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleMeshPointNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTriangleMeshPointNormals.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkTriangleMeshPointNormals);

namespace
{
template<typename ptDataType>
const char* computeNormalsDirection(vtkPolyData* mesh, float *n)
{
  ptDataType *points = reinterpret_cast<ptDataType*>
    (mesh->GetPoints()->GetData()->GetVoidPointer(0));
  vtkIdType *cells = reinterpret_cast<vtkIdType*>
    (mesh->GetPolys()->GetData()->GetVoidPointer(0));
  vtkIdType v0Offset, v1Offset, v2Offset;
  ptDataType *p0, *p1, *p2;
  float a[3], b[3], tn[3];
  for (vtkIdType i = 0; i < mesh->GetNumberOfPolys(); ++i)
  {
    // First value in cellArray indicates number of points in cell.
    // We need 3 to compute normals.
    if (*cells == 3)
    {
      // vertex offsets
      v0Offset = 3 * cells[1];
      v1Offset = 3 * cells[2];
      v2Offset = 3 * cells[3];
      // pointer to each vertex
      p0 = points + v0Offset;
      p1 = points + v1Offset;
      p2 = points + v2Offset;
      // two vectors
      a[0] = p2[0] - p1[0];
      a[1] = p2[1] - p1[1];
      a[2] = p2[2] - p1[2];
      b[0] = p0[0] - p1[0];
      b[1] = p0[1] - p1[1];
      b[2] = p0[2] - p1[2];
      // cell normals by cross-product
      // (no need to normalize those + it's faster not to)
      tn[0] = (a[1] * b[2] - a[2] * b[1]);
      tn[1] = (a[2] * b[0] - a[0] * b[2]);
      tn[2] = (a[0] * b[1] - a[1] * b[0]);
      // append triangle normals to point normals
      *(n + v0Offset) += tn[0];
      *(n + v0Offset + 1) += tn[1];
      *(n + v0Offset + 2) += tn[2];
      *(n + v1Offset) += tn[0];
      *(n + v1Offset + 1) += tn[1];
      *(n + v1Offset + 2) += tn[2];
      *(n + v2Offset) += tn[0];
      *(n + v2Offset + 1) += tn[1];
      *(n + v2Offset + 2) += tn[2];
      // move to next cells
      cells += (*cells + 1); // current cell nbr of pts + 1
    }
    // If degenerate cell
    else if (*cells < 3)
    {
      return "Some cells are degenerate (less than 3 points). "
             "Use vtkCleanPolyData beforehand to correct this.";
    }
    // If cell not triangle
    else
    {
      return "Some cells have too many points (more than 3 points). "
             "Use vtkTriangulate to correct this.";
    }
  }
  return NULL;
}
}

// Generate normals for polygon meshes
int vtkTriangleMeshPointNormals::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Generating surface normals");

  vtkIdType numPts = input->GetNumberOfPoints(); // nbr of points from input
  if ( numPts < 1 )
  {
    vtkDebugMacro(<<"No data to generate normals for!");
    return 1;
  }

  if (input->GetVerts()->GetNumberOfCells() != 0 ||
      input->GetLines()->GetNumberOfCells() != 0 ||
      input->GetStrips()->GetNumberOfCells() != 0)
  {
    vtkErrorMacro(<< "Can not compute normals for a mesh with Verts, Lines or Strips, as it will "
                  << "corrupt the number of points used during the normals computation."
                  << "Make sure your input PolyData only has triangles (Polys with 3 components)).");
    return 0;
  }

  // Copy structure and cell data
  output->CopyStructure(input);
  output->GetCellData()->PassData(input->GetCellData());

  // If there is nothing to do, pass the point data through
  if (input->GetNumberOfPolys() < 1)
  {
    output->GetPointData()->PassData(input->GetPointData());
    return 1;
  }
  // Else pass everything but normals
  output->GetPointData()->CopyNormalsOff();
  output->GetPointData()->PassData(input->GetPointData());

  // Prepare array for normals
  vtkFloatArray *normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(numPts);
  normals->SetName("Normals");
  normals->FillValue(0.0);
  output->GetPointData()->SetNormals(normals);
  normals->Delete();

  this->UpdateProgress(0.1);

  // Compute normals direction
  float *n = reinterpret_cast<float*>(normals->GetVoidPointer(0));
  switch (output->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(
      const char *warning = computeNormalsDirection<VTK_TT>(output, n);
      if(warning)
      {
        vtkWarningMacro( << warning);
      }
    );
  }
  this->UpdateProgress(0.5);

  // Normalize point normals
  float l;
  unsigned int i3;
  for (vtkIdType i = 0; i < numPts; ++i)
  {
    i3 = i * 3;
    if ((l = sqrt(n[i3] * n[i3] +
        n[i3 + 1] * n[i3 + 1] +
        n[i3 + 2] * n[i3 + 2])) != 0.0)
    {
        n[i3] /= l;
        n[i3 + 1] /= l;
        n[i3 + 2] /= l;
    }
  }
  this->UpdateProgress(0.9);

  // Update modified time
  normals->Modified();

  return 1;
}

void vtkTriangleMeshPointNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
