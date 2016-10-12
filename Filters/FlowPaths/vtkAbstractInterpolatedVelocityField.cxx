/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractInterpolatedVelocityField.h"

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
const double vtkAbstractInterpolatedVelocityField::TOLERANCE_SCALE = 1.0E-8;
const double vtkAbstractInterpolatedVelocityField::SURFACE_TOLERANCE_SCALE = 1.0E-5;

//---------------------------------------------------------------------------
vtkAbstractInterpolatedVelocityField::vtkAbstractInterpolatedVelocityField()
{
  this->NumFuncs     = 3; // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->Weights      = 0;
  this->WeightsSize  = 0;

  this->Caching    = true; // Caching on by default
  this->CacheHit   = 0;
  this->CacheMiss  = 0;

  this->LastCellId = -1;
  this->LastDataSet= 0;
  this->LastPCoords[0] = 0.0;
  this->LastPCoords[1] = 0.0;
  this->LastPCoords[2] = 0.0;

  this->VectorsType = 0;
  this->VectorsSelection = 0;
  this->NormalizeVector  = false;
  this->ForceSurfaceTangentVector  = false;
  this->SurfaceDataset = false;

  this->Cell     = vtkGenericCell::New();
  this->GenCell  = vtkGenericCell::New();
}

//---------------------------------------------------------------------------
vtkAbstractInterpolatedVelocityField::~vtkAbstractInterpolatedVelocityField()
{
  this->NumFuncs     = 0;
  this->NumIndepVars = 0;

  this->LastDataSet  = 0;
  this->SetVectorsSelection(0);

  delete[] this->Weights;
  this->Weights = 0;

  if ( this->Cell )
  {
    this->Cell->Delete();
    this->Cell = NULL;
  }

  if ( this->GenCell )
  {
    this->GenCell->Delete();
    this->GenCell = NULL;
  }
}

//---------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::FunctionValues
  ( vtkDataSet * dataset, double * x, double * f )
{
  int i, j, numPts, id;
  vtkDataArray * vectors = NULL;
  double vec[3];

  f[0] = f[1] = f[2] = 0.0;

  // See if a dataset has been specified and if there are input vectors
  if ( !dataset)
  {
    vtkErrorMacro( << "Can't evaluate dataset!" );
    vectors = NULL;
    return 0;
  }
  if(!this->VectorsSelection) //if a selection is not speicifed,
  {
    //use the first one in the point set (this is a behavior for backward compatability)
    vectors =  dataset->GetPointData()->GetVectors(0);
  }
  else
  {
    vectors = dataset->GetAttributesAsFieldData(this->VectorsType)->GetArray(this->VectorsSelection);
  }

  if(!vectors)
  {
    vtkErrorMacro( << "Can't evaluate dataset!" );
    return 0;
  }


  if (!this->FindAndUpdateCell(dataset, x))
  {
      vectors = NULL;
      return  0;
  }

  // if the cell is valid
  if (this->LastCellId >= 0)
  {
    numPts = this->GenCell->GetNumberOfPoints();

    // interpolate the vectors
    if(this->VectorsType==vtkDataObject::POINT)
    {
      for ( j = 0; j < numPts; j ++ )
      {
        id = this->GenCell->PointIds->GetId( j );
        vectors->GetTuple( id, vec );
        for ( i = 0; i < 3; i ++ )
        {
          f[i] +=  vec[i] * this->Weights[j];
        }
      }
    }
    else
    {
      vectors->GetTuple(this->LastCellId, f);
    }

    if (this->ForceSurfaceTangentVector)
    {
      vtkNew<vtkIdList> ptIds;
      dataset->GetCellPoints(this->LastCellId, ptIds.Get());
      if (ptIds->GetNumberOfIds() < 3)
      {
        vtkErrorMacro(<<"Cannot compute normal on cells with less than 3 points");
      }
      else
      {
        double p1[3];
        double p2[3];
        double p3[3];
        double normal[3];
        double v1[3], v2[3];
        double k;

        dataset->GetPoint(ptIds->GetId(0), p1);
        dataset->GetPoint(ptIds->GetId(1), p2);
        dataset->GetPoint(ptIds->GetId(2), p3);

        // Compute othogonal component
        v1[0] = p2[0] - p1[0];
        v1[1] = p2[1] - p1[1];
        v1[2] = p2[2] - p1[2];
        v2[0] = p3[0] - p1[0];
        v2[1] = p3[1] - p1[1];
        v2[2] = p3[2] - p1[2];

        vtkMath::Cross(v1, v2, normal);
        vtkMath::Normalize(normal);
        k = vtkMath::Dot(normal, f);

        // Remove non orthogonal component.
        f[0] = f[0] - (normal[0] * k);
        f[1] = f[1] - (normal[1] * k);
        f[2] = f[2] - (normal[2] * k);
      }
    }

    if ( this->NormalizeVector == true )
    {
      vtkMath::Normalize( f );
    }
  }
  // if not, return false
  else
  {
    vectors = NULL;
    return  0;
  }

  vectors = NULL;
  return  1;
}

//---------------------------------------------------------------------------
bool vtkAbstractInterpolatedVelocityField::CheckPCoords(double pcoords [3])
{
  for (int i = 0; i < 3; i++)
  {
    if (pcoords[i] < 0 || pcoords[i] > 1)
    {
      return false;
    }
  }
  return true;
}

//---------------------------------------------------------------------------
bool vtkAbstractInterpolatedVelocityField::FindAndUpdateCell(vtkDataSet* dataset, double* x)
{
  double tol2, dist2;
  if (this->SurfaceDataset)
  {
    tol2 = dataset->GetLength() *  dataset->GetLength() *
      vtkAbstractInterpolatedVelocityField::SURFACE_TOLERANCE_SCALE;
  }
  else
  {
    tol2 = dataset->GetLength() *  dataset->GetLength() *
           vtkAbstractInterpolatedVelocityField::TOLERANCE_SCALE;
  }

  double closest[3];
  bool found = false;
  if (this->Caching)
  {
    bool out = false;

    // See if the point is in the cached cell
    if (this->LastCellId != -1)
    {
      // Use cache cell only if point is inside
      // or , with surface , not far and in pccords
      int ret = this->GenCell->EvaluatePosition
          (x, closest, this->LastSubId, this->LastPCoords, dist2, this->Weights);
      if (ret == -1
          || (ret == 0 && !this->SurfaceDataset)
          || (this->SurfaceDataset && (dist2 > tol2 || !this->CheckPCoords(this->LastPCoords))))
      {
        out = true;
      }

      if (out)
      {
        this->CacheMiss++;

        dataset->GetCell(this->LastCellId, this->Cell);

        // Search around current cached cell to see if there is a cell within tolerance
        this->LastCellId =
          dataset->FindCell(x, this->Cell, this->GenCell, this->LastCellId,
                            tol2, this->LastSubId, this->LastPCoords, this->Weights);

        if (this->LastCellId != -1 && (!this->SurfaceDataset || this->CheckPCoords(this->LastPCoords)))
        {
          dataset->GetCell(this->LastCellId, this->GenCell);
          found = true;
        }
      }
      else
      {
        this->CacheHit++;
        found = true;
      }
    }
  }
  if (!found)
  {
    // if the cell is not found in cache, do a global search (ignore initial
    // cell if there is one)
    this->LastCellId =
      dataset->FindCell(x, 0, this->GenCell, -1, tol2,
                        this->LastSubId, this->LastPCoords, this->Weights);

    if (this->LastCellId != -1 && (!this->SurfaceDataset || this->CheckPCoords(this->LastPCoords)))
    {
      dataset->GetCell(this->LastCellId, this->GenCell);
    }
    else
    {
      if (this->SurfaceDataset)
      {
        // Still cannot find cell, use point locator to find a (arbitrary) cell, for 2D surface
        vtkIdType idPoint = dataset->FindPoint(x);
        if (idPoint < 0)
        {
          this->LastCellId = -1;
          return false;
        }

        vtkNew<vtkIdList> cellList;
        dataset->GetPointCells(idPoint, cellList.Get());
        double minDist2 = dataset->GetLength() * dataset->GetLength();
        vtkIdType minDistId = -1;
        for (vtkIdType idCell = 0; idCell < cellList->GetNumberOfIds(); idCell++)
        {
          this->LastCellId = cellList->GetId(idCell);
          dataset->GetCell(this->LastCellId, this->GenCell);
          int ret = this->GenCell->EvaluatePosition
            (x, closest, this->LastSubId, this->LastPCoords, dist2, this->Weights);
          if (ret != -1 && dist2 < minDist2)
          {
            minDistId = this->LastCellId;
            minDist2 = dist2;
          }
        }

        if (minDistId == -1)
        {
          this->LastCellId = -1;
          return false;
        }

        // Recover closest cell info
        this->LastCellId = minDistId;
        dataset->GetCell(this->LastCellId, this->GenCell);
        int ret = this->GenCell->EvaluatePosition
            (x, closest, this->LastSubId, this->LastPCoords, dist2, this->Weights);

        // Find Point being not perfect to find cell, check for closer cells
        vtkNew<vtkIdList> boundaryPoints;
        vtkNew<vtkIdList> neighCells;
        bool edge = false;
        bool closer;
        while (true)
        {
            this->GenCell->CellBoundary(this->LastSubId, this->LastPCoords, boundaryPoints.Get());
            dataset->GetCellNeighbors(this->LastCellId, boundaryPoints.Get(), neighCells.Get());
            if (neighCells->GetNumberOfIds() == 0)
            {
              edge = true;
              break;
            }
            closer = false;
            for (vtkIdType neighCellId = 0; neighCellId < neighCells->GetNumberOfIds(); neighCellId++)
            {
                this->LastCellId = neighCells->GetId(neighCellId);
                dataset->GetCell(this->LastCellId, this->GenCell);
                ret = this->GenCell->EvaluatePosition
                  (x, closest, this->LastSubId, this->LastPCoords, dist2, this->Weights);
                if (ret != -1 && dist2 < minDist2)
                {
                  minDistId = this->LastCellId;
                  minDist2 = dist2;
                  closer = true;
                }
            }
            if (!closer)
            {
              break;
            }
        }

        // Recover closest cell info
        if (!edge)
        {
          this->LastCellId = minDistId;
          dataset->GetCell(this->LastCellId, this->GenCell);
          this->GenCell->EvaluatePosition
              (x, closest, this->LastSubId, this->LastPCoords, dist2, this->Weights);
        }
        if (minDist2 > tol2 || (!this->CheckPCoords(this->LastPCoords) && edge))
        {
          this->LastCellId = -1;
          return false;
        }
      }
      else
      {
        this->LastCellId = -1;
        return  false;
      }
    }
  }
  return true;
}
//----------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::GetLastWeights( double * w )
{
  if ( this->LastCellId < 0 )
  {
    return 0;
  }

  int   numPts = this->GenCell->GetNumberOfPoints();
  for ( int i = 0; i < numPts; i ++ )
  {
    w[i] = this->Weights[i];
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::GetLastLocalCoordinates( double pcoords[3] )
{
  if ( this->LastCellId < 0 )
  {
    return 0;
  }

  pcoords[0] = this->LastPCoords[0];
  pcoords[1] = this->LastPCoords[1];
  pcoords[2] = this->LastPCoords[2];

  return 1;
}

//----------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::FastCompute
  ( vtkDataArray * vectors, double f[3] )
{
  int    pntIdx;
  int    numPts = this->GenCell->GetNumberOfPoints();
  double vector[3];
  f[0] = f[1] = f[2] = 0.0;

  for ( int i = 0; i < numPts; i ++ )
  {
    pntIdx = this->GenCell->PointIds->GetId( i );
    vectors->GetTuple( pntIdx, vector );
    f[0] += vector[0] * this->Weights[i];
    f[1] += vector[1] * this->Weights[i];
    f[2] += vector[2] * this->Weights[i];
  }
}

//----------------------------------------------------------------------------
bool vtkAbstractInterpolatedVelocityField::InterpolatePoint
  ( vtkPointData * outPD, vtkIdType outIndex )
{
  if ( !this->LastDataSet )
  {
    return 0;
  }

  outPD->InterpolatePoint( this->LastDataSet->GetPointData(), outIndex,
                           this->GenCell->PointIds, this->Weights );
  return 1;
}

//----------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "VectorsSelection: "
     << ( this->VectorsSelection ? this->VectorsSelection : "(none)" ) << endl;
  os << indent << "NormalizeVector: "
     << ( this->NormalizeVector ? "on." : "off." ) << endl;
  os << indent << "ForceSurfaceTangentVector: "
     << ( this->ForceSurfaceTangentVector ? "on." : "off." ) << endl;
  os << indent << "SurfaceDataset: "
     << ( this->SurfaceDataset ? "on." : "off." ) << endl;

  os << indent << "Caching Status: "     << ( this->Caching ? "on." : "off." )
     << endl;
  os << indent << "Cache Hit: "          << this->CacheHit         << endl;
  os << indent << "Cache Miss: "         << this->CacheMiss        << endl;
  os << indent << "Weights Size: "       << this->WeightsSize      << endl;

  os << indent << "Last Dataset: "       << this->LastDataSet      << endl;
  os << indent << "Last Cell Id: "       << this->LastCellId       << endl;
  os << indent << "Last Cell: "          << this->Cell             << endl;
  os << indent << "Current Cell: "       << this->GenCell          << endl;
  os << indent << "Last P-Coords: "      << this->LastPCoords[0]
               << ", "                   << this->LastPCoords[1]
               << ", "                   << this->LastPCoords[2]   << endl;
  os << indent << "Last Weights: "       << this->Weights          << endl;
}

void vtkAbstractInterpolatedVelocityField::SelectVectors(int associationType, const char * fieldName )
{
  this->VectorsType = associationType;
  this->SetVectorsSelection(fieldName);
}
