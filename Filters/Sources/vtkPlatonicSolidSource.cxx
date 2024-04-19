// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPlatonicSolidSource.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlatonicSolidSource);
VTK_ABI_NAMESPACE_END

// Wrapping this in namespaces because the short names (a, b, c, etc) are
// throwing warnings on MSVC when inlined methods in vtkGenericDataArray are
// being used ('warning C4459: declaration of 'c' hides global declaration')
namespace
{
namespace vtkPlatonicSolidSourceDetail
{
// The geometry and topology of each solid. Solids are centered at
// the origin with radius 1.0.
// The golden ration phi = (1+sqrt(5))/2=1.61803398875 enters into many
// of these values.
constexpr double TetraPoints[] = {
  1.0, 1.0, 1.0,   //
  -1.0, 1.0, -1.0, //
  1.0, -1.0, -1.0, //
  -1.0, -1.0, 1.0  //
};
constexpr vtkIdType TetraVerts[] = {
  0, 2, 1, //
  1, 2, 3, //
  0, 3, 2, //
  0, 1, 3  //
};

constexpr double CubePoints[] = {
  -1.0, -1.0, -1.0, //
  1.0, -1.0, -1.0,  //
  1.0, 1.0, -1.0,   //
  -1.0, 1.0, -1.0,  //
  -1.0, -1.0, 1.0,  //
  1.0, -1.0, 1.0,   //
  1.0, 1.0, 1.0,    //
  -1.0, 1.0, 1.0    //
};
constexpr vtkIdType CubeVerts[] = {
  0, 1, 5, 4, //
  0, 4, 7, 3, //
  4, 5, 6, 7, //
  3, 7, 6, 2, //
  1, 2, 6, 5, //
  0, 3, 2, 1  //
};

constexpr double OctPoints[] = {
  -1.0, -1.0, 0.0,            //
  1.0, -1.0, 0.0,             //
  1.0, 1.0, 0.0,              //
  -1.0, 1.0, 0.0,             //
  0.0, 0.0, -1.4142135623731, //
  0.0, 0.0, 1.4142135623731   //
};
constexpr vtkIdType OctVerts[] = {
  4, 1, 0, //
  4, 2, 1, //
  4, 3, 2, //
  4, 0, 3, //
  0, 1, 5, //
  1, 2, 5, //
  2, 3, 5, //
  3, 0, 5  //
};

constexpr double a_0 = 0.61803398875;
constexpr double b = 0.381966011250;
constexpr double DodePoints[] = {
  b, 0, 1,          //
  -b, 0, 1,         //
  b, 0, -1,         //
  -b, 0, -1,        //
  0, 1, -b,         //
  0, 1, b,          //
  0, -1, -b,        //
  0, -1, b,         //
  1, b, 0,          //
  1, -b, 0,         //
  -1, b, 0,         //
  -1, -b, 0,        //
  -a_0, a_0, a_0,   //
  a_0, -a_0, a_0,   //
  -a_0, -a_0, -a_0, //
  a_0, a_0, -a_0,   //
  a_0, a_0, a_0,    //
  -a_0, a_0, -a_0,  //
  -a_0, -a_0, a_0,  //
  a_0, -a_0, -a_0   //
};
constexpr vtkIdType DodeVerts[] = {
  0, 16, 5, 12, 1,   //
  1, 18, 7, 13, 0,   //
  2, 19, 6, 14, 3,   //
  3, 17, 4, 15, 2,   //
  4, 5, 16, 8, 15,   //
  5, 4, 17, 10, 12,  //
  6, 7, 18, 11, 14,  //
  7, 6, 19, 9, 13,   //
  8, 16, 0, 13, 9,   //
  9, 19, 2, 15, 8,   //
  10, 17, 3, 14, 11, //
  11, 18, 1, 12, 10  //
};

constexpr double c = 0.5;
constexpr double d = 0.30901699;
constexpr double IcosaPoints[] = {
  0.0, d, -c,  //
  0.0, d, c,   //
  0.0, -d, c,  //
  -d, c, 0.0,  //
  -d, -c, 0.0, //
  d, c, 0.0,   //
  d, -c, 0.0,  //
  0.0, -d, -c, //
  c, 0.0, d,   //
  -c, 0.0, d,  //
  -c, 0.0, -d, //
  c, 0.0, -d   //
};
constexpr vtkIdType IcosaVerts[] = {
  0, 3, 5,  //
  1, 5, 3,  //
  1, 9, 2,  //
  1, 2, 8,  //
  0, 11, 7, //
  0, 7, 10, //
  2, 4, 6,  //
  7, 6, 4,  //
  3, 10, 9, //
  4, 9, 10, //
  5, 8, 11, //
  6, 11, 8, //
  1, 3, 9,  //
  1, 8, 5,  //
  0, 10, 3, //
  0, 5, 11, //
  7, 4, 10, //
  7, 11, 6, //
  2, 9, 4,  //
  2, 6, 8   //
};
} // end namespace detail
} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
vtkPlatonicSolidSource::vtkPlatonicSolidSource()
{
  this->SolidType = VTK_SOLID_TETRAHEDRON;
  this->OutputPointsPrecision = SINGLE_PRECISION;
  this->SetNumberOfInputPorts(0);
}

int vtkPlatonicSolidSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int i;
  double solidScale = 1.0;
  const double* pptr;
  const double* solidPoints = nullptr;
  vtkIdType numPts = 0, numCells = 0, cellSize = 0;
  const vtkIdType* cptr;
  const vtkIdType* solidVerts = nullptr;

  vtkDebugMacro(<< "Creating Platonic solid");

  // Based on type, select correct connectivity and point arrays
  //
  switch (this->SolidType)
  {
    case VTK_SOLID_TETRAHEDRON:
      numPts = 4;
      cellSize = 3;
      numCells = 4;
      solidPoints = vtkPlatonicSolidSourceDetail::TetraPoints;
      solidVerts = vtkPlatonicSolidSourceDetail::TetraVerts;
      solidScale = 1.0 / sqrt(3.0);
      break;

    case VTK_SOLID_CUBE:
      numPts = 8;
      cellSize = 4;
      numCells = 6;
      solidPoints = vtkPlatonicSolidSourceDetail::CubePoints;
      solidVerts = vtkPlatonicSolidSourceDetail::CubeVerts;
      solidScale = 1.0 / sqrt(3.0);
      break;

    case VTK_SOLID_OCTAHEDRON:
      numPts = 6;
      cellSize = 3;
      numCells = 8;
      solidPoints = vtkPlatonicSolidSourceDetail::OctPoints;
      solidVerts = vtkPlatonicSolidSourceDetail::OctVerts;
      solidScale = 1.0 / sqrt(2.0);
      break;

    case VTK_SOLID_ICOSAHEDRON:
      numPts = 12;
      cellSize = 3;
      numCells = 20;
      solidPoints = vtkPlatonicSolidSourceDetail::IcosaPoints;
      solidVerts = vtkPlatonicSolidSourceDetail::IcosaVerts;
      solidScale = 1.0 / 0.58778524999243;
      break;

    case VTK_SOLID_DODECAHEDRON:
      numPts = 20;
      cellSize = 5;
      numCells = 12;
      solidPoints = vtkPlatonicSolidSourceDetail::DodePoints;
      solidVerts = vtkPlatonicSolidSourceDetail::DodeVerts;
      solidScale = 1.0 / 1.070466269319;
      break;
  }

  // Create the solids
  //
  vtkPoints* pts = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    pts->SetDataType(VTK_DOUBLE);
  }
  else
  {
    pts->SetDataType(VTK_FLOAT);
  }

  pts->SetNumberOfPoints(numPts);
  vtkCellArray* polys = vtkCellArray::New();
  polys->AllocateEstimate(numCells, cellSize);
  vtkIntArray* colors = vtkIntArray::New();
  colors->SetNumberOfComponents(1);
  colors->SetNumberOfTuples(numCells);

  // Points
  for (i = 0, pptr = solidPoints; i < numPts; i++, pptr += 3)
  {
    pts->SetPoint(i, solidScale * (pptr[0]), solidScale * (pptr[1]), solidScale * (pptr[2]));
  }

  // Cells
  for (i = 0, cptr = solidVerts; i < numCells; i++, cptr += cellSize)
  {
    polys->InsertNextCell(cellSize, cptr);
    colors->SetTuple1(i, i);
  }

  // Assemble the output
  output->SetPoints(pts);
  output->SetPolys(polys);
  int idx = output->GetCellData()->AddArray(colors);
  output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);

  pts->Delete();
  polys->Delete();
  colors->Delete();

  return 1;
}

void vtkPlatonicSolidSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Solid Type: "
     << "\n";
  if (this->SolidType == VTK_SOLID_TETRAHEDRON)
  {
    os << "Tetrahedron\n";
  }
  else if (this->SolidType == VTK_SOLID_CUBE)
  {
    os << "Cube\n";
  }
  else if (this->SolidType == VTK_SOLID_OCTAHEDRON)
  {
    os << "Octahedron\n";
  }
  else if (this->SolidType == VTK_SOLID_ICOSAHEDRON)
  {
    os << "Icosahedron\n";
  }
  else // if ( this->SolidType == VTK_SOLID_DODECAHEDRON )
  {
    os << "Dodecahedron\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
