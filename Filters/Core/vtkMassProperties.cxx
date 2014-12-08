/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMassProperties.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMassProperties.h"

#include "vtkObjectFactory.h"
#include "vtkCell.h"
#include "vtkDataObject.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

vtkStandardNewMacro(vtkMassProperties);

#define  VTK_CUBE_ROOT(x) \
  ((x<0.0)?(-pow((-x),0.333333333333333)):(pow((x),0.333333333333333)))

//----------------------------------------------------------------------------
// Constructs with initial 0 values.
vtkMassProperties::vtkMassProperties()
{
  this->SurfaceArea = 0.0;
  this->MinCellArea = 0.0;
  this->MaxCellArea = 0.0;
  this->Volume  = 0.0;
  this->VolumeProjected = 0.0;
  this->VolumeX = 0.0;
  this->VolumeY = 0.0;
  this->VolumeZ = 0.0;
  this->Kx = 0.0;
  this->Ky = 0.0;
  this->Kz = 0.0;
  this->NormalizedShapeIndex = 0.0;

  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
// Destroy any allocated memory.
vtkMassProperties::~vtkMassProperties()
{
}

//----------------------------------------------------------------------------
// Description:
// This method measures volume, surface area, and normalized shape index.
// Currently, the input is a ploydata which consists of triangles.
int vtkMassProperties::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  vtkInformation *inInfo =
    inputVector[0]->GetInformationObject(0);

  // call ExecuteData
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, numCells, numPts, numIds;
  double p[3];

  numCells = input->GetNumberOfCells();
  numPts = input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
    {
    vtkErrorMacro( << "No data to measure...!");
    return 1;
    }

  vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
  ptIds->Allocate(VTK_CELL_SIZE);

  // Traverse all cells, obtaining node coordinates.
  //
  double    vol[3],kxyz[3];
  double    xp[3]; // to compute volumeproj
  double    munc[3],wxyz,wxy,wxz,wyz;
  double    area,surfacearea;
  double    volumeproj;
  double    mincellarea, maxcellarea;
  double    a,b,c,s;
  double    x[3],y[3],z[3];
  double    i[3],j[3],k[3],u[3],absu[3],length;
  double    ii[3],jj[3],kk[3];
  double    xavg,yavg,zavg;
  vtkIdType idx;

  // Initialize variables ...
  //
  surfacearea = 0.0;
  volumeproj = 0.0;
  mincellarea = VTK_DOUBLE_MAX; maxcellarea = 0.0;
  wxyz = 0; wxy = 0.0; wxz = 0.0; wyz = 0.0;
  for ( idx = 0; idx < 3 ; idx++ )
    {
    munc[idx] = 0.0;
    vol[idx]  = 0.0;
    kxyz[idx] = 0.0;
    }

  for (cellId=0; cellId < numCells; cellId++)
    {
    if ( input->GetCellType(cellId) != VTK_TRIANGLE)
      {
      vtkWarningMacro(<< "Input data type must be VTK_TRIANGLE not "
                      << input->GetCellType(cellId));
      continue;
      }
    input->GetCellPoints(cellId,ptIds);
    numIds = ptIds->GetNumberOfIds();
    assert(numIds == 3);

    // store current vertex (x,y,z) coordinates ...
    //
    for (idx=0; idx < numIds; idx++)
      {
      input->GetPoint(ptIds->GetId(idx), p);
      x[idx] = p[0]; y[idx] = p[1]; z[idx] = p[2];
      }

    // get i j k vectors ...
    //
    i[0] = ( x[1] - x[0]); j[0] = (y[1] - y[0]); k[0] = (z[1] - z[0]);
    i[1] = ( x[2] - x[0]); j[1] = (y[2] - y[0]); k[1] = (z[2] - z[0]);
    i[2] = ( x[2] - x[1]); j[2] = (y[2] - y[1]); k[2] = (z[2] - z[1]);

    // cross product between two vectors, to determine normal vector
    //
    u[0] = ( j[0] * k[1] - k[0] * j[1]);
    u[1] = ( k[0] * i[1] - i[0] * k[1]);
    u[2] = ( i[0] * j[1] - j[0] * i[1]);

    // normalize normal vector to 1
    //
    length = sqrt( u[0]*u[0] + u[1]*u[1] + u[2]*u[2]);
    if ( length != 0.0)
      {
      u[0] /= length;
      u[1] /= length;
      u[2] /= length;
      }
    else
      {
      u[0] = u[1] = u[2] = 0.0;
      }

    // determine max unit normal component...
    //
    absu[0] = fabs(u[0]); absu[1] = fabs(u[1]); absu[2] = fabs(u[2]);

    if (( absu[0] > absu[1]) && ( absu[0] > absu[2]) )
      {
      munc[0]++;
      }
    else if (( absu[1] > absu[0]) && ( absu[1] > absu[2]) )
      {
      munc[1]++;
      }
    else if (( absu[2] > absu[0]) && ( absu[2] > absu[1]) )
      {
      munc[2]++;
      }
    else if (( absu[0] == absu[1])&& ( absu[0] == absu[2]))
      {
      wxyz++;
      }
    else if (( absu[0] == absu[1])&& ( absu[0] > absu[2]) )
      {
      wxy++;
      }
    else if (( absu[0] == absu[2])&& ( absu[0] > absu[1]) )
      {
      wxz++;
      }
    else if (( absu[1] == absu[2])&& ( absu[0] < absu[2]) )
      {
      wyz++;
      }
    else
      {
      vtkErrorMacro( << "Unpredicted situation...!" );
      return 1;
      }

    // This is reduced to ...
    //
    ii[0] = i[0] * i[0]; ii[1] = i[1] * i[1]; ii[2] = i[2] * i[2];
    jj[0] = j[0] * j[0]; jj[1] = j[1] * j[1]; jj[2] = j[2] * j[2];
    kk[0] = k[0] * k[0]; kk[1] = k[1] * k[1]; kk[2] = k[2] * k[2];

    // area of a triangle...
    //
    a = sqrt(ii[1] + jj[1] + kk[1]);
    b = sqrt(ii[0] + jj[0] + kk[0]);
    c = sqrt(ii[2] + jj[2] + kk[2]);
    s = 0.5 * (a + b + c);
    area = sqrt( fabs(s*(s-a)*(s-b)*(s-c)));
    surfacearea += area;
    if( area < mincellarea )
      {
      mincellarea = area;
      }
    if( area > maxcellarea )
      {
      maxcellarea = area;
      }

    // volume elements ...
    //
    zavg = (z[0] + z[1] + z[2]) / 3.0;
    yavg = (y[0] + y[1] + y[2]) / 3.0;
    xavg = (x[0] + x[1] + x[2]) / 3.0;

    vol[2] += (area * u[2] * zavg);
    vol[1] += (area * u[1] * yavg);
    vol[0] += (area * u[0] * xavg);

    // V  =  (z1+z2+z3)(x1y2-x2y1+x2y3-x3y2+x3y1-x1y3)/6
    // Volume under triangle is projected area of the triangle times
    // the average of the three z values
    vtkMath::Cross(x,y,xp);
    volumeproj += zavg * (xp[0]+xp[1]+xp[2]) / 2;
    }

  // Surface Area ...
  //
  this->SurfaceArea = surfacearea;
  this->MinCellArea = mincellarea;
  this->MaxCellArea = maxcellarea;

  // Weighting factors in Discrete Divergence theorem for volume calculation..
  //
  kxyz[0] = (munc[0] + (wxyz/3.0) + ((wxy+wxz)/2.0)) /numCells;
  kxyz[1] = (munc[1] + (wxyz/3.0) + ((wxy+wyz)/2.0)) /numCells;
  kxyz[2] = (munc[2] + (wxyz/3.0) + ((wxz+wyz)/2.0)) /numCells;
  this->VolumeX = vol[0];
  this->VolumeY = vol[1];
  this->VolumeZ = vol[2];
  this->Kx = kxyz[0];
  this->Ky = kxyz[1];
  this->Kz = kxyz[2];
  this->Volume =  (kxyz[0] * vol[0] + kxyz[1] * vol[1] + kxyz[2]  * vol[2]);
  this->Volume =  fabs(this->Volume);
  this->VolumeProjected = volumeproj;
  this->NormalizedShapeIndex =
    (sqrt(surfacearea)/VTK_CUBE_ROOT(this->Volume))/2.199085233;

  return 1;
}

//----------------------------------------------------------------------------
void vtkMassProperties::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  vtkPolyData *input = vtkPolyData::SafeDownCast(this->GetInput(0));
  if (!input)
    {
    return;
    }
  os << indent << "VolumeX: " << this->GetVolumeX () << "\n";
  os << indent << "VolumeY: " << this->GetVolumeY () << "\n";
  os << indent << "VolumeZ: " << this->GetVolumeZ () << "\n";
  os << indent << "Kx: " << this->GetKx () << "\n";
  os << indent << "Ky: " << this->GetKy () << "\n";
  os << indent << "Kz: " << this->GetKz () << "\n";
  os << indent << "Volume:  " << this->GetVolume  () << "\n";
  //os << indent << "Volume Projected:  " << this->GetVolumeProjected  () << "\n";
  //os << indent << "Volume Error:  " <<
  //  fabs(this->GetVolume() - this->GetVolumeProjected())   << "\n";
  os << indent << "Surface Area: " << this->GetSurfaceArea () << "\n";
  os << indent << "Min Cell Area: " << this->GetMinCellArea () << "\n";
  os << indent << "Max Cell Area: " << this->GetMaxCellArea () << "\n";
  os << indent << "Normalized Shape Index: "
     << this->GetNormalizedShapeIndex () << "\n";
}
