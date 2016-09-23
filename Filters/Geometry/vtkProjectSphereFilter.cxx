/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectSphereFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProjectSphereFilter.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <map>

namespace
{
  void ConvertXYZToLatLonDepth(double xyz[3], double lonLatDepth[3], double center[3])
  {
    double dist2 = vtkMath::Distance2BetweenPoints(xyz, center);
    lonLatDepth[2] = sqrt(dist2);
    double radianAngle = atan2(xyz[1]-center[1], xyz[0]-center[0]);
    lonLatDepth[0] = radianAngle*180./vtkMath::Pi()-180.;
    lonLatDepth[1] = 90.-acos((xyz[2]-center[2])/lonLatDepth[2])*180./vtkMath::Pi();
  }

  template<class data_type>
  void TransformVector(
    double* transformMatrix, data_type* data)
  {
    double d0 = static_cast<double>(data[0]);
    double d1 = static_cast<double>(data[1]);
    double d2 = static_cast<double>(data[2]);
    data[0] = static_cast<data_type>(
      transformMatrix[0]*d0+transformMatrix[1]*d1+transformMatrix[2]*d2);
    data[1] = static_cast<data_type>(
      transformMatrix[3]*d0+transformMatrix[4]*d1+transformMatrix[5]*d2);
    data[2] = static_cast<data_type>(
      transformMatrix[6]*d0+transformMatrix[7]*d1+transformMatrix[8]*d2);
  }
} // end anonymous namespace

vtkStandardNewMacro(vtkProjectSphereFilter);

//-----------------------------------------------------------------------------
vtkProjectSphereFilter::vtkProjectSphereFilter() : SplitLongitude(-180)
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0;
  this->KeepPolePoints = false;
  this->TranslateZ = false;
}

//-----------------------------------------------------------------------------
vtkProjectSphereFilter::~vtkProjectSphereFilter()
{
}

//-----------------------------------------------------------------------------
void vtkProjectSphereFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Center: ("
     << this->Center[0] << ", "
     << this->Center[1] << ", "
     << this->Center[2] << ")\n";
  os << indent << "KeepPolePoints " << this->KeepPolePoints << "\n";
  os << indent << "TranslateZ " << this->TranslateZ << "\n";
}

//-----------------------------------------------------------------------------
int vtkProjectSphereFilter::FillInputPortInformation(int vtkNotUsed(port),
                                                     vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkProjectSphereFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  vtkDebugMacro("RequestData");

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPointSet *input
    = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(vtkPolyData* poly = vtkPolyData::SafeDownCast(input))
  {
    if( poly->GetVerts()->GetNumberOfCells() > 0 ||
        poly->GetLines()->GetNumberOfCells() > 0 ||
        poly->GetStrips()->GetNumberOfCells() > 0 )
    {
      vtkErrorMacro("Can only deal with vtkPolyData polys.");
      return 0;
    }
  }

  vtkPointSet *output
    = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkNew<vtkIdList> polePointIds;
  this->TransformPointInformation(input, output, polePointIds.GetPointer());
  this->TransformCellInformation(input, output, polePointIds.GetPointer());
  output->GetFieldData()->ShallowCopy(input->GetFieldData());

  vtkDebugMacro("Leaving RequestData");

  return 1;
}

//-----------------------------------------------------------------------------
void vtkProjectSphereFilter::TransformPointInformation(
  vtkPointSet* input, vtkPointSet* output, vtkIdList* polePointIds)
{
  polePointIds->Reset();
  output->GetPointData()->CopyAllOn();
  output->GetPointData()->PassData(input->GetPointData());
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(input->GetNumberOfPoints());

  double zTranslation = ( this->TranslateZ == true ?
                          this->GetZTranslation(input) : 0. );

  output->SetPoints(points.GetPointer());
  vtkIdType numberOfPoints = input->GetNumberOfPoints();
  double minDist2ToCenterLine = VTK_DOUBLE_MAX;
  for(vtkIdType i=0;i<numberOfPoints;i++)
  {
    double coordIn[3], coordOut[3];
    input->GetPoint(i, coordIn);
    ConvertXYZToLatLonDepth(coordIn, coordOut, this->Center);
    // if we allow the user to specify SplitLongitude we have to make
    // sure that we respect their choice since the output of atan
    // is from -180 to 180.
    if(coordOut[0] < this->SplitLongitude)
    {
      coordOut[0] += 360.;
    }
    coordOut[2] -= zTranslation;

    // acbauer -- a hack to make the grid look better by forcing it to be flat.
    // leaving this in for now even though it's commented out. if I figure out
    // a proper way to do this i'll replace it.
    // if(this->TranslateZ)
    //   {
    //   coordOut[2] = 0;
    //   }
    points->SetPoint(i, coordOut);

    // keep track of the ids of the points that are closest to the
    // centerline between -90 and 90 latitude. this is done as a single
    // pass algorithm.
    double dist2 =
      (coordIn[0]-this->Center[0])*(coordIn[0]-this->Center[0])+
      (coordIn[1]-this->Center[1])*(coordIn[1]-this->Center[1]);
    if(dist2 < minDist2ToCenterLine)
    {
      // we found a closer point so throw out the previous closest
      // point ids.
      minDist2ToCenterLine = dist2;
      polePointIds->SetNumberOfIds(1);
      polePointIds->SetId(0, i);
    }
    else if(dist2 == minDist2ToCenterLine)
    {
      // this point is just as close as the current closest point
      // so we just add it to our list.
      polePointIds->InsertNextId(i);
    }
    this->TransformTensors(i, coordIn, output->GetPointData());
  }
  this->ComputePointsClosestToCenterLine(minDist2ToCenterLine, polePointIds);
}

//-----------------------------------------------------------------------------
void vtkProjectSphereFilter::TransformCellInformation(
  vtkPointSet* input, vtkPointSet* output, vtkIdList* polePointIds)
{
  // a map from the old point to the newly created point for split cells
  std::map<vtkIdType,vtkIdType> boundaryMap;

  double TOLERANCE = .0001;
  vtkNew<vtkMergePoints> locator;
  locator->InitPointInsertion(output->GetPoints(), output->GetBounds(),
                              output->GetNumberOfPoints());
  double coord[3];
  for(vtkIdType i=0;i<output->GetNumberOfPoints();i++)
  {
    // this is a bit annoying but required for building up the locator properly
    // otherwise it won't either know these points exist or will start
    // counting new points at index 0.
    output->GetPoint(i, coord);
    locator->InsertNextPoint(coord);
  }

  vtkIdType numberOfCells = input->GetNumberOfCells();
  vtkCellArray* connectivity = NULL;
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(output);
  vtkPolyData* poly = vtkPolyData::SafeDownCast(output);
  if(ugrid)
  {
    ugrid->Allocate(numberOfCells);
    connectivity = ugrid->GetCells();
  }
  else if(poly)
  {
    poly->Allocate(numberOfCells);
    connectivity = poly->GetPolys();
  }
  output->GetCellData()->CopyAllOn();
  output->GetCellData()->CopyAllocate(input->GetCellData(),
                                      input->GetNumberOfCells());
  vtkPointData* pointData = output->GetPointData();
  pointData->CopyAllOn();
  pointData->CopyAllocate(pointData, output->GetNumberOfPoints());

  vtkNew<vtkIdList> cellPoints;
  vtkNew<vtkIdList> skippedCells;
  vtkIdType mostPointsInCell = 0;
  for(vtkIdType cellId=0;cellId<numberOfCells;cellId++)
  {
    bool onLeftBoundary = false;
    bool onRightBoundary = false;
    bool leftSideInterior = false; //between SplitLongitude and SplitLongitude+90
    bool rightSideInterior = false;// between SplitLongitude+270 and SplitLongitude+360
    bool middleInterior = false; //between SplitLongitude+90 and SplitLongitude+270

    bool skipCell = false;
    bool splitCell = false;
    double xyz[3];
    input->GetCellPoints(cellId, cellPoints.GetPointer());
    mostPointsInCell = (mostPointsInCell > cellPoints->GetNumberOfIds() ?
                        mostPointsInCell : cellPoints->GetNumberOfIds() );
    for(vtkIdType pt=0;pt<cellPoints->GetNumberOfIds();pt++)
    {
      output->GetPoint(cellPoints->GetId(pt), xyz);
      if(xyz[0] < this->SplitLongitude+TOLERANCE)
      {
        onLeftBoundary = true;
      }
      else if(xyz[0] > this->SplitLongitude+360.-TOLERANCE)
      {
        onRightBoundary = true;
      }
      else if(xyz[0] < this->SplitLongitude+90.)
      {
        leftSideInterior = true;
      }
      else if(xyz[0] > this->SplitLongitude+270.)
      {
        rightSideInterior = true;
      }
      else
      {
        middleInterior = true;
      }
      if(polePointIds->IsId(cellPoints->GetId(pt)) != -1 && this->KeepPolePoints == false)
      {
        skipCell = true;
        skippedCells->InsertNextId(cellId);
        continue;
      }
    }
    if(skipCell)
    {
      continue;
    }
    if( (onLeftBoundary || onRightBoundary ) && rightSideInterior && leftSideInterior)
    { // this cell stretches across the split longitude
      splitCell = true;
    }
    else if( onLeftBoundary && rightSideInterior )
    {
      for(vtkIdType pt=0;pt<cellPoints->GetNumberOfIds();pt++)
      {
        output->GetPoint(cellPoints->GetId(pt), xyz);
        if(xyz[0] < this->SplitLongitude+TOLERANCE)
        {
          std::map<vtkIdType,vtkIdType>::iterator it =
            boundaryMap.find(cellPoints->GetId(pt));
          if(it == boundaryMap.end())
          { // need to create another point
            xyz[0] += 360.;
            vtkIdType id = locator->InsertNextPoint(xyz);
            boundaryMap[cellPoints->GetId(pt)] = id;
            pointData->CopyData(pointData, cellPoints->GetId(pt), id);
            cellPoints->SetId(pt, id);
          }
          else
          {
            cellPoints->SetId(pt, it->second);
          }
        }
      }
    }
    else if( onRightBoundary && leftSideInterior )
    {
      for(vtkIdType pt=0;pt<cellPoints->GetNumberOfIds();pt++)
      {
        output->GetPoint(cellPoints->GetId(pt), xyz);
        if(xyz[0] > this->SplitLongitude+360.-TOLERANCE)
        {
          std::map<vtkIdType,vtkIdType>::iterator it =
            boundaryMap.find(cellPoints->GetId(pt));
          if(it == boundaryMap.end())
          { // need to create another point
            xyz[0] -= 360.;
            vtkIdType id = locator->InsertNextPoint(xyz);
            boundaryMap[cellPoints->GetId(pt)] = id;
            pointData->CopyData(pointData, cellPoints->GetId(pt), id);
            cellPoints->SetId(pt, id);
          }
          else
          {
            cellPoints->SetId(pt, it->second);
          }
        }
      }
    }
    else if( (onLeftBoundary || onRightBoundary ) && middleInterior)
    {
      splitCell = true;
    }
    else if( leftSideInterior && rightSideInterior)
    {
      splitCell = true;
    }
    if(splitCell)
    {
      this->SplitCell(input, output, cellId, locator.GetPointer(), connectivity, 0);
      this->SplitCell(input, output, cellId, locator.GetPointer(), connectivity, 1);
    }
    else if(ugrid)
    {
      ugrid->InsertNextCell(input->GetCellType(cellId), cellPoints.GetPointer());
      output->GetCellData()->CopyData(input->GetCellData(), cellId,
                                      output->GetNumberOfCells()-1);
    }
    else if(poly)
    {
      poly->InsertNextCell(input->GetCellType(cellId), cellPoints.GetPointer());
      output->GetCellData()->CopyData(input->GetCellData(), cellId,
                                      output->GetNumberOfCells()-1);
    }
  }

  if(poly)
  {
    // we have to rebuild the polydata cell data structures since when
    // we split a cell we don't do it right away due to the expense
    poly->DeleteCells();
    poly->BuildCells();
  }

  // deal with cell data
  std::vector<double> weights(mostPointsInCell);
  vtkIdType skipCounter = 0;
  for(vtkIdType cellId=0;cellId<input->GetNumberOfCells();cellId++)
  {
    if(skippedCells->IsId(cellId) != -1)
    {
      skippedCells->DeleteId(cellId);
      skipCounter++;
      continue;
    }
    int subId = 0;
    double parametricCenter[3];
    vtkCell* cell = input->GetCell(cellId);
    cell->GetParametricCenter(parametricCenter);
    cell->EvaluateLocation(subId, parametricCenter, coord, &weights[0]);
    this->TransformTensors(cellId-skipCounter, coord, output->GetCellData());
  }
}

//-----------------------------------------------------------------------------
void vtkProjectSphereFilter::TransformTensors(
  vtkIdType pointId, double* coord, vtkDataSetAttributes* dataArrays  )
{
   double theta = atan2(
     sqrt( (coord[0]-this->Center[0])*(coord[0]-this->Center[0]) +
           (coord[1]-this->Center[1])*(coord[1]-this->Center[1]) ),
     coord[2]-this->Center[2] );
   double phi = atan2( coord[1]-this->Center[1], coord[0]-this->Center[0] );
   double sinTheta = sin(theta);
   double cosTheta = cos(theta);
   double sinPhi = sin(phi);
   double cosPhi = cos(phi);
   double transformMatrix[9] = {
     -sinPhi, cosPhi, 0.,
     cosTheta*cosPhi, cosTheta*sinPhi, -sinTheta,
     sinTheta*cosPhi, sinTheta*sinPhi, cosTheta };
   for(int i=0;i<dataArrays->GetNumberOfArrays();i++)
   {
     vtkDataArray* array = dataArrays->GetArray(i);
     if(array->GetNumberOfComponents() == 3)
     {
       switch (array->GetDataType())
       {
         vtkTemplateMacro(TransformVector(
                            transformMatrix,
                            static_cast<VTK_TT *>(array->GetVoidPointer(pointId*array->GetNumberOfComponents())) ) );
       }
     }
   }
}

//-----------------------------------------------------------------------------
double vtkProjectSphereFilter::GetZTranslation(vtkPointSet* input)
{
  double maxRadius2 = 0; // squared radius
  double coord[3];
  for(vtkIdType i=0;i<input->GetNumberOfPoints();i++)
  {
    input->GetPoint(i, coord);
    double dist2 = vtkMath::Distance2BetweenPoints(coord, this->Center);
    if(dist2 > maxRadius2)
    {
      maxRadius2 = dist2;
    }
  }
  return sqrt(maxRadius2);
}

//-----------------------------------------------------------------------------
void vtkProjectSphereFilter::SplitCell(
  vtkPointSet* input, vtkPointSet* output, vtkIdType inputCellId,
  vtkIncrementalPointLocator* locator, vtkCellArray* connectivity,
  int splitSide)
{
  // i screw up the canonical ordering of the cell but apparently this
  // gets fixed by vtkCell::Clip().
  vtkCell* cell = input->GetCell(inputCellId);
  vtkNew<vtkDoubleArray> cellScalars;
  cellScalars->SetNumberOfTuples(cell->GetNumberOfPoints());
  double coord[3];
  for(vtkIdType pt=0;pt<cell->GetNumberOfPoints();pt++)
  {
    output->GetPoint(cell->GetPointId(pt), coord);
    if(splitSide == 0 && coord[0] > this->SplitLongitude+180.)
    {
      coord[0] -= 360.;
    }
    else if(splitSide == 1 && coord[0] < this->SplitLongitude+180.)
    {
      coord[0] += 360.;
    }
    cellScalars->SetValue(pt, coord[0]);
    cell->GetPoints()->SetPoint(pt, coord);
  }
  vtkIdType numberOfCells = output->GetNumberOfCells();
  double splitLocation = (splitSide == 0 ? -180. : 180.);
  cell->Clip(splitLocation, cellScalars.GetPointer(), locator, connectivity,
             output->GetPointData(), output->GetPointData(), input->GetCellData(),
             inputCellId, output->GetCellData(), splitSide);
  // if the grid was an unstructured grid we have to update the cell
  // types and locations for the created cells.
  if(vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(output))
  {
    this->SetCellInformation(ugrid, cell, output->GetNumberOfCells()-numberOfCells);
  }
}

//-----------------------------------------------------------------------------
void vtkProjectSphereFilter::SetCellInformation(
  vtkUnstructuredGrid* output, vtkCell* cell, vtkIdType numberOfNewCells)
{
  for(vtkIdType i=0;i<numberOfNewCells;i++)
  {
    vtkIdType prevCellId = output->GetNumberOfCells()+i-numberOfNewCells-1;
    vtkIdType newCellId = prevCellId + 1;
    vtkIdType *pts, numPts;
    vtkIdType loc = output->GetCellLocationsArray()->GetValue(prevCellId);
    output->GetCells()->GetCell(loc,numPts,pts);

    output->GetCellLocationsArray()->InsertNextValue(loc+numPts+1);
    output->GetCells()->GetCell(loc+numPts+1,numPts,pts);
    if(cell->GetCellDimension() == 0)
    {
      if(numPts > 2)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_POLY_VERTEX);
      }
      else
      {
        vtkErrorMacro("Cannot handle 0D cell with " << numPts << " number of points.");
      }
    }
    else if(cell->GetCellDimension() == 1)
    {
      if(numPts == 2)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_LINE);
      }
      else if(numPts > 2)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_POLY_LINE);
      }
      else
      {
        vtkErrorMacro("Cannot handle 1D cell with " << numPts << " number of points.");
      }
    }
    else if(cell->GetCellDimension() == 2)
    {
      if(numPts == 3)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_TRIANGLE);
      }
      else if(numPts > 3 && cell->GetCellType() == VTK_TRIANGLE_STRIP)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_TRIANGLE_STRIP);
      }
      else if(numPts == 4)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_QUAD);
      }
      else
      {
        vtkErrorMacro("Cannot handle 2D cell with " << numPts << " number of points.");
      }
    }
    else  // 3D cell
    {
      if(numPts == 4)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_TETRA);
      }
      else if(numPts == 5)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_PYRAMID);
      }
      else if(numPts == 6)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_WEDGE);
      }
      else if(numPts == 8)
      {
        output->GetCellTypesArray()->InsertValue(newCellId, VTK_HEXAHEDRON);
      }
      else
      {
        vtkErrorMacro("Unknown 3D cell type.");
      }
    }
  }
}
