/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeOfRevolutionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeOfRevolutionFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

vtkStandardNewMacro(vtkVolumeOfRevolutionFilter);

namespace
{
struct AxisOfRevolution
{
  double Position[3];
  double Direction[3];
};

void RevolvePoint(const double in[3], const AxisOfRevolution* axis,
                  double angleInRadians, double out[3])
{
  double c = cos(angleInRadians);
  double cm = 1. - c;
  double s  = sin(angleInRadians);

  double translated[3];
  vtkMath::Subtract(in, axis->Position, translated);

  double dot = vtkMath::Dot(translated, axis->Direction);
  double cross[3];
  vtkMath::Cross(translated, axis->Direction, cross);

  for (vtkIdType i=0;i<3;i++)
  {
    out[i] = ((translated[i]*c + axis->Direction[i]*dot*cm - cross[i]*s) +
              axis->Position[i]);
  }
}

void RevolvePoints(vtkDataSet* pts, vtkPoints* newPts, AxisOfRevolution* axis,
                   double sweepAngle, int resolution, vtkPointData *outPd,
                   bool partialSweep)
{
  double angleInRadians = vtkMath::RadiansFromDegrees(sweepAngle/resolution);

  vtkIdType n2DPoints = pts->GetNumberOfPoints();
  vtkIdType counter = 0;
  double p2d[3], p3d[3];

  for (int i=0; i<resolution + partialSweep; i++)
  {
    for (int id = 0; id < n2DPoints; id++)
    {
      pts->GetPoint(id, p2d);
      RevolvePoint(p2d, axis, i * angleInRadians, p3d);
      newPts->SetPoint(counter, p3d);
      outPd->CopyData(pts->GetPointData(), i, counter);
      counter++;
    }
  }
}

template <int CellType>
void Revolve(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
             vtkCellArray *connectivity, vtkUnsignedCharArray *types,
             vtkIdTypeArray *locations, vtkCellData *inCd, vtkIdType cellId,
             vtkCellData *outCd, bool partialSweep);

template <>
void Revolve<VTK_VERTEX>(vtkIdList* pointIds, vtkIdType n2DPoints,
                         int resolution, vtkCellArray *connectivity,
                         vtkUnsignedCharArray *types, vtkIdTypeArray *locations,
                         vtkCellData *inCd, vtkIdType cellId,
                         vtkCellData *outCd, bool partialSweep)
{
  vtkIdType newPtIds[2], newCellId;

  newPtIds[0] = pointIds->GetId(0);

  for (int i=0; i<resolution; i++)
  {
    newPtIds[1] = (pointIds->GetId(0) +
                   ((i+1)%(resolution + partialSweep))*n2DPoints);
    newCellId = connectivity->InsertNextCell(2, newPtIds);
    locations->InsertNextValue(connectivity->GetTraversalLocation());
    types->InsertNextValue(VTK_LINE);
    outCd->CopyData(inCd, cellId, newCellId);
    newPtIds[0] = newPtIds[1];
  }
}

template <>
void Revolve<VTK_POLY_VERTEX>(vtkIdList* pointIds, vtkIdType n2DPoints,
                              int resolution, vtkCellArray *connectivity,
                              vtkUnsignedCharArray *types,
                              vtkIdTypeArray *locations, vtkCellData *inCd,
                              vtkIdType cellId, vtkCellData *outCd,
                              bool partialSweep)
{
  vtkNew<vtkIdList> pointId;
  pointId->SetNumberOfIds(1);
  for (vtkIdType i=0; i<pointIds->GetNumberOfIds(); i++)
  {
    pointId->SetId(0,pointIds->GetId(i));
    Revolve<VTK_VERTEX>(pointId.GetPointer(), n2DPoints, resolution,
                        connectivity, types, locations, inCd, cellId, outCd,
                        partialSweep);
  }
}

template <>
void Revolve<VTK_LINE>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
                       vtkCellArray *connectivity, vtkUnsignedCharArray *types,
                       vtkIdTypeArray *locations, vtkCellData *inCd,
                       vtkIdType cellId, vtkCellData *outCd, bool partialSweep)
{
  static const int nPoints = 2;

  vtkIdType newPtIds[2*nPoints], newCellId;

  for (vtkIdType i=0;i<nPoints;i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i=0; i<resolution; i++)
  {
    for (vtkIdType j=0; j<nPoints; j++)
    {
      newPtIds[2*nPoints-1-j] =
        pointIds->GetId(j) + ((i+1)%(resolution + partialSweep))*n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2*nPoints, newPtIds);
    locations->InsertNextValue(connectivity->GetTraversalLocation());
    types->InsertNextValue(VTK_QUAD);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j=0;j<nPoints;j++)
    {
      newPtIds[nPoints-1-j] = newPtIds[j+nPoints];
    }
  }
}

template <>
void Revolve<VTK_POLY_LINE>(vtkIdList* pointIds, vtkIdType n2DPoints,
                            int resolution, vtkCellArray *connectivity,
                            vtkUnsignedCharArray *types,
                            vtkIdTypeArray *locations, vtkCellData *inCd,
                            vtkIdType cellId, vtkCellData *outCd,
                            bool partialSweep)
{
  vtkNew<vtkIdList> newPointIds;
  newPointIds->SetNumberOfIds(2);
  newPointIds->SetId(0,pointIds->GetId(0));
  for (vtkIdType i=1; i<pointIds->GetNumberOfIds(); i++)
  {
    newPointIds->SetId(1,pointIds->GetId(i));
    Revolve<VTK_LINE>(newPointIds.GetPointer(), n2DPoints, resolution,
                      connectivity, types, locations, inCd, cellId, outCd,
                      partialSweep);
    newPointIds->SetId(0,pointIds->GetId(i));
  }
}

template <>
void Revolve<VTK_TRIANGLE>(vtkIdList* pointIds, vtkIdType n2DPoints,
                           int resolution, vtkCellArray *connectivity,
                           vtkUnsignedCharArray *types,
                           vtkIdTypeArray *locations, vtkCellData *inCd,
                           vtkIdType cellId, vtkCellData *outCd,
                           bool partialSweep)
{
  static const int nPoints = 3;

  vtkIdType newPtIds[2*nPoints], newCellId;

  for (vtkIdType i=0;i<nPoints;i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i=0; i<resolution; i++)
  {
    for (vtkIdType j=0; j<nPoints; j++)
    {
      newPtIds[j+nPoints] =
        pointIds->GetId(j) + ((i+1)%(resolution + partialSweep))*n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2*nPoints, newPtIds);
    locations->InsertNextValue(connectivity->GetTraversalLocation());
    types->InsertNextValue(VTK_WEDGE);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j=0;j<nPoints;j++)
    {
      newPtIds[j] = newPtIds[j+nPoints];
    }
  }
}

template <>
void Revolve<VTK_TRIANGLE_STRIP>(vtkIdList* pointIds, vtkIdType n2DPoints,
                                 int resolution, vtkCellArray *connectivity,
                                 vtkUnsignedCharArray *types,
                                 vtkIdTypeArray *locations, vtkCellData *inCd,
                                 vtkIdType cellId, vtkCellData *outCd,
                                 bool partialSweep)
{
  vtkNew<vtkIdList> newPointIds;
  newPointIds->SetNumberOfIds(3);
  newPointIds->SetId(0,pointIds->GetId(0));
  newPointIds->SetId(1,pointIds->GetId(1));
  for (vtkIdType i=2; i<pointIds->GetNumberOfIds(); i++)
  {
    newPointIds->SetId(2,pointIds->GetId(i));
    Revolve<VTK_TRIANGLE>(newPointIds.GetPointer(), n2DPoints, resolution,
                      connectivity, types, locations, inCd, cellId, outCd,
                          partialSweep);
    newPointIds->SetId(0,pointIds->GetId(i));
    newPointIds->SetId(1,pointIds->GetId(i-1));
  }
}

template <>
void Revolve<VTK_QUAD>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
                       vtkCellArray *connectivity, vtkUnsignedCharArray *types,
                       vtkIdTypeArray *locations, vtkCellData *inCd,
                       vtkIdType cellId, vtkCellData *outCd, bool partialSweep)
{
  static const int nPoints = 4;

  vtkIdType newPtIds[2*nPoints], newCellId;

  for (vtkIdType i=0;i<nPoints;i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i=0; i<resolution; i++)
  {
    for (vtkIdType j=0; j<nPoints; j++)
    {
      newPtIds[j+nPoints] =
        pointIds->GetId(j) + ((i+1)%(resolution + partialSweep))*n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2*nPoints, newPtIds);
    locations->InsertNextValue(connectivity->GetTraversalLocation());
    types->InsertNextValue(VTK_HEXAHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j=0; j<nPoints; j++)
    {
      newPtIds[j] = newPtIds[j+nPoints];
    }
  }
}

template <>
void Revolve<VTK_PIXEL>(vtkIdList* pointIds, vtkIdType n2DPoints,
                        int resolution, vtkCellArray *connectivity,
                        vtkUnsignedCharArray *types, vtkIdTypeArray *locations,
                        vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                        bool partialSweep)
{
  static const int nPoints = 4;

  vtkIdType newPtIds[2*nPoints], newCellId;

  for (vtkIdType i=0;i<nPoints;i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i=0; i<resolution; i++)
  {
    for (vtkIdType j=0; j<nPoints; j++)
    {
      newPtIds[j+nPoints] =
        pointIds->GetId(j) + ((i+1)%(resolution + partialSweep))*n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2*nPoints, newPtIds);
    locations->InsertNextValue(connectivity->GetTraversalLocation());
    types->InsertNextValue(VTK_HEXAHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j=0; j<nPoints; j++)
    {
      newPtIds[j] = newPtIds[j+nPoints];
    }
  }
}

template <>
void Revolve<VTK_POLYGON>(vtkIdList* pointIds, vtkIdType n2DPoints,
                          int resolution, vtkCellArray *connectivity,
                          vtkUnsignedCharArray *types,
                          vtkIdTypeArray *locations, vtkCellData *inCd,
                          vtkIdType cellId, vtkCellData *outCd,
                          bool partialSweep)
{
  // A swept polygon creates a polyhedron with two polygon faces and <nPoly>
  // quad faces, comprised from 2*<nPoly> points. Because polyhedra have a
  // special connectivity format, the length of the connectivity array is
  // 1 + (<nPoly>+2) + 2*<nPoly> + 4*<nPoly> = 7*<nPoly> + 3.
  // ^        ^           ^           ^
  // integer describing # of faces (<nPoly> + 2)
  //          ^           ^           ^
  //          integers describing # of points per face
  //                      ^           ^
  //                      point ids for the two polygon faces
  //                                  ^
  //                                  point ids for the 4 quad faces

  int nPoly = pointIds->GetNumberOfIds();
  std::vector<vtkIdType> newPtIds(7*nPoly + 3);
  // newFacePtIds are pointers to the point arrays describing each face
  std::vector<vtkIdType*> newFacePtIds(nPoly + 2);
  vtkIdType newCellId;

  newPtIds[0] = nPoly + 2;
  newPtIds[1] = nPoly;
  newPtIds[nPoly + 2] = nPoly;
  newFacePtIds[0] = &newPtIds[2];
  newFacePtIds[1] = &newPtIds[nPoly+3];
  for (vtkIdType i=0;i<nPoly;i++)
  {
    // All of the subsequent faces have four point ids
    newPtIds[3 + 2*nPoly + 5*i] = 4;
    newFacePtIds[2+i] = &newPtIds[4 + 2*nPoly + 5*i];
    newFacePtIds[0][i] = pointIds->GetId(i);
  }

  for (int i=0; i<resolution; i++)
  {
    for (vtkIdType j=0; j<nPoly; j++)
    {
      newFacePtIds[1][nPoly-1-j] =
        pointIds->GetId(j) + ((i+1)%(resolution + partialSweep))*n2DPoints;
    }
    for (vtkIdType j=0; j<nPoly; j++)
    {
      newFacePtIds[j+2][0] = newFacePtIds[0][j];
      newFacePtIds[j+2][1] = newFacePtIds[0][(j+1)%nPoly];
      newFacePtIds[j+2][2] = newFacePtIds[1][(2*nPoly-2-j)%nPoly];
      newFacePtIds[j+2][3] = newFacePtIds[1][nPoly-1-j];
    }
    newCellId = connectivity->InsertNextCell(7*nPoly+3, &newPtIds[0]);
    locations->InsertNextValue(connectivity->GetTraversalLocation());
    types->InsertNextValue(VTK_POLYHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j=0; j<nPoly; j++)
    {
      newFacePtIds[0][j] = newFacePtIds[1][nPoly-1-j];
    }
  }
}

int RevolveCell(int cellType, vtkIdList* pointIds, vtkIdType n2DPoints,
                int resolution, vtkCellArray *connectivity,
                vtkUnsignedCharArray *types, vtkIdTypeArray *locations,
                vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                bool partialSweep)
{
  int returnValue = 0;
#define RevolveCellCase(CellType)                                       \
  case CellType:                                                        \
    Revolve<CellType>(pointIds, n2DPoints, resolution, connectivity,    \
                      types, locations, inCd, cellId, outCd, partialSweep); \
    break

  switch (cellType)
  {
    RevolveCellCase(VTK_VERTEX);
    RevolveCellCase(VTK_POLY_VERTEX);
    RevolveCellCase(VTK_LINE);
    RevolveCellCase(VTK_POLY_LINE);
    RevolveCellCase(VTK_TRIANGLE);
    RevolveCellCase(VTK_TRIANGLE_STRIP);
    RevolveCellCase(VTK_POLYGON);
    RevolveCellCase(VTK_PIXEL);
    RevolveCellCase(VTK_QUAD);
    default:
      returnValue = 1;
  }
  return returnValue;
#undef RevolveCellCase
}
}

//----------------------------------------------------------------------------
vtkVolumeOfRevolutionFilter::vtkVolumeOfRevolutionFilter()
{
  this->SweepAngle = 360.0;
  this->Resolution = 12; // 30 degree increments
  this->AxisPosition[0] = this->AxisPosition[1] = this->AxisPosition[2] = 0.;
  this->AxisDirection[0] = this->AxisDirection[1] = 0.;
  this->AxisDirection[2] = 1.;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
vtkVolumeOfRevolutionFilter::~vtkVolumeOfRevolutionFilter()
{
}

//----------------------------------------------------------------------------
int vtkVolumeOfRevolutionFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData* inPd = input->GetPointData();
  vtkCellData* inCd = input->GetCellData();
  vtkPointData* outPd = output->GetPointData();
  vtkCellData* outCd = output->GetCellData();

  vtkNew<vtkPoints> outPts;

  // Check to see that the input data is amenable to this operation
  vtkCellIterator* it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    int cellDimension = it->GetCellDimension();
    if (cellDimension > 2)
    {
      vtkErrorMacro(<<"All cells must have a topological dimension < 2.");
      return 1;
    }
  }
  it->Delete();

  // Set up output points
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    vtkPointSet *inputPointSet = vtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      outPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      outPts->SetDataType(VTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }

  // determine whether or not the sweep angle is a full 2*pi
  bool partialSweep = false;
  if (fabs(360. - fabs(this->SweepAngle)) > 1024*VTK_DBL_EPSILON)
  {
     partialSweep = true;
  }

  // Set up output points and point data
  outPts->SetNumberOfPoints(input->GetNumberOfPoints() * (this->Resolution +
                                                          partialSweep));
  outPd->CopyAllocate(inPd, input->GetNumberOfPoints() * (this->Resolution +
                                                          partialSweep));

  // Set up output cell data
  vtkIdType nNewCells = input->GetNumberOfCells() * this->Resolution;
  outCd->CopyAllocate(inCd, nNewCells);

  vtkNew<vtkUnsignedCharArray> outTypes;
  vtkNew<vtkIdTypeArray> outLocations;
  vtkNew<vtkCellArray> outCells;

  AxisOfRevolution axis;
  for (vtkIdType i=0; i<3; i++)
  {
    axis.Position[i] = this->AxisPosition[i];
    axis.Direction[i] = this->AxisDirection[i];
  }

  RevolvePoints(input, outPts.GetPointer(), &axis, this->SweepAngle,
                this->Resolution, outPd, partialSweep);

  it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    if (RevolveCell(it->GetCellType(), it->GetPointIds(),
                    input->GetNumberOfPoints(), this->Resolution,
                    outCells.GetPointer(), outTypes.GetPointer(),
                    outLocations.GetPointer(), inCd, it->GetCellId(), outCd,
                    partialSweep) == 1)
    {
      vtkWarningMacro(<<"No method for revolving cell type "
                      << it->GetCellType() <<". Skipping.");
    }
  }
  it->Delete();

  output->SetPoints(outPts.GetPointer());
  output->SetCells(outTypes.GetPointer(), outLocations.GetPointer(),
                   outCells.GetPointer());

  return 1;
}

//----------------------------------------------------------------------------
int vtkVolumeOfRevolutionFilter::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkVolumeOfRevolutionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Sweep Angle: " << this->SweepAngle << "\n";
  os << indent << "Axis Position: (" << this->AxisPosition[0] << ","
     << this->AxisPosition[1] << "," << this->AxisPosition[2] << ")\n";
  os << indent << "Axis Direction: (" << this->AxisPosition[0] << ","
     << this->AxisDirection[1] << "," << this->AxisDirection[2] << ")\n";
  os << indent << "Output Points Precision: "<<this->OutputPointsPrecision
     << "\n";
}
