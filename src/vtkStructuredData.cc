/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredData.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkStructuredData.hh"

vtkStructuredData::vtkStructuredData()
{
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->Dimensions[2] = 1;
  this->DataDescription = VTK_SINGLE_POINT;
  
  this->Blanking = 0;
  this->PointVisibility = NULL;
}

vtkStructuredData::vtkStructuredData(const vtkStructuredData& sds)
{
  this->Dimensions[0] = sds.Dimensions[0];
  this->Dimensions[1] = sds.Dimensions[1];
  this->Dimensions[2] = sds.Dimensions[2];
  this->DataDescription = sds.DataDescription;

  this->Blanking = sds.Blanking;
  if ( sds.PointVisibility != NULL )
    this->PointVisibility = new vtkBitArray(*sds.PointVisibility);
  else
    this->PointVisibility = NULL;
}

vtkStructuredData::~vtkStructuredData()
{
}

// Description:
// Return the topological dimension of the data (e.g., 0, 1, 2, or 3D).
int vtkStructuredData::GetDataDimension()
{
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: return 0;

    case VTK_X_LINE: case VTK_Y_LINE: case VTK_Z_LINE: return 1;

    case VTK_XY_PLANE: case VTK_YZ_PLANE: case VTK_XZ_PLANE: return 2;

    case VTK_XYZ_GRID: return 3;

    default:
      return -1;                       
    }
}

// Description:
// Set the i-j-k dimensions of the data.
void vtkStructuredData::SetDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetDimensions(dim);
}

void vtkStructuredData::SetDimensions(int dim[3])
{
  int dataDim, i;

  vtk_DebugMacro(<< " setting Dimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->Dimensions[0] || dim[1] != this->Dimensions[1] ||
  dim[2] != this->Dimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vtk_ErrorMacro (<< "Bad Dimensions, retaining previous values");
      return;
      }

    for (dataDim=0, i=0; i<3 ; i++)
      {
      this->Dimensions[i] = dim[i];
      if (dim[i] > 1) dataDim++;
      }

    if ( dataDim == 3 )
      {
      this->DataDescription = VTK_XYZ_GRID;
      }
    else if ( dataDim == 2)
      {
      if ( dim[0] == 1 ) this->DataDescription = VTK_YZ_PLANE;
      else if ( dim[1] == 1 ) this->DataDescription = VTK_XZ_PLANE;
      else this->DataDescription = VTK_XY_PLANE;
      }
    else if ( dataDim == 1 )
      {
      if ( dim[0] != 1 ) this->DataDescription = VTK_X_LINE;
      else if ( dim[1] != 1 ) this->DataDescription = VTK_Y_LINE;
      else this->DataDescription = VTK_Z_LINE;
      }
    else
      {
      this->DataDescription = VTK_SINGLE_POINT;
      }

    this->_Modified();
    }
}

int *vtkStructuredData::GetDimensions() 
{ 
  return this->Dimensions;
}

void vtkStructuredData::GetDimensions(int dim[3])
{ 
  for (int i=0; i<3; i++) dim[i] = this->Dimensions[i];
}


// Description:
// Turn on data blanking. Data blanking is the ability to turn off
// portions of the grid when displaying or operating on it. Some data
// (like finite difference data) routinely turns off data to simulate
// solid obstacles.
void vtkStructuredData::BlankingOn()
{
  this->Blanking = 1;
  this->_Modified();

  if ( !this->PointVisibility )
    {
    this->PointVisibility = new vtkBitArray(this->_GetNumberOfPoints(),1000);
    for (int i=0; i<this->_GetNumberOfPoints(); i++)
      {
      this->PointVisibility->InsertValue(i,1);
      }
    }
}

// Description:
// Turn off data blanking.
void vtkStructuredData::BlankingOff()
{
  this->Blanking = 0;
  this->_Modified();
}

// Description:
// Turn off a particular data point.
void vtkStructuredData::BlankPoint(int ptId)
{
  if ( !this->PointVisibility ) this->BlankingOn();
  this->PointVisibility->InsertValue(ptId,0);
}

// Description:
// Turn on a particular data point.
void vtkStructuredData::UnBlankPoint(int ptId)
{
  if ( !this->PointVisibility ) this->BlankingOn();
  this->PointVisibility->InsertValue(ptId,1);
}

int vtkStructuredData::_GetNumberOfCells()
{
  int nCells=1;
  int i;

  for (i=0; i<3; i++)
    if (this->Dimensions[i] > 1)
      nCells *= (this->Dimensions[i]-1);

  return nCells;
}

int vtkStructuredData::_GetNumberOfPoints()
{
  return Dimensions[0]*Dimensions[1]*Dimensions[2];
}

void vtkStructuredData::_Initialize()
{
  this->SetDimensions(1,1,1);
  this->Blanking = 0;

  if ( this->PointVisibility )
    {
    this->PointVisibility->Delete();
    this->PointVisibility = NULL;
    }
}

void vtkStructuredData::_GetCellPoints(int cellId, vtkIdList& ptIds)
{
  int idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
 
  ptIds.Reset();

  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      iMin = iMax = jMin = jMax = kMin = kMax = 0;
      break;

    case VTK_X_LINE:
      jMin = jMax = kMin = kMax = 0;
      iMin = cellId;
      iMax = cellId + 1;
      break;

    case VTK_Y_LINE:
      iMin = iMax = kMin = kMax = 0;
      jMin = cellId;
      jMax = cellId + 1;
      break;

    case VTK_Z_LINE:
      iMin = iMax = jMin = jMax = 0;
      kMin = cellId;
      kMax = cellId + 1;
      break;

    case VTK_XY_PLANE:
      kMin = kMax = 0;
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      break;

    case VTK_YZ_PLANE:
      iMin = iMax = 0;
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      break;

    case VTK_XZ_PLANE:
      jMin = jMax = 0;
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      break;
    }

  // Extract point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        idx = loc[0] + loc[1]*this->Dimensions[0] + loc[2]*d01;
        ptIds.InsertId(npts++,idx);
        }
      }
    }
}

void vtkStructuredData::_GetPointCells(int ptId, vtkIdList& cellIds)
{
  int ptDim[3], cellDim[3];
  int ptLoc[3], cellLoc[3];
  int i, j, cellId;
  static int offset[8][3] = {{-1,0,0}, {-1,-1,0}, {-1,-1,-1}, {-1,0,-1},
                             {0,0,0},  {0,-1,0},  {0,-1,-1},  {0,0,-1}};

  for (i=0; i<3; i++) 
    {
    ptDim[i] = this->Dimensions[i];
    cellDim[i] = ptDim[i] - 1;
    }
//
//  Get the location of the point
//
  ptLoc[0] = ptId % ptDim[0];
  ptLoc[1] = (ptId / ptDim[0]) % ptDim[1];
  ptLoc[2] = ptId / (ptDim[0]*ptDim[1]);
//
//  From the point location, compute the cell locations.  There are at
//  most eight possible.
//
  cellIds.Reset();

  for (j=0; j<8; j++) 
    {
    for (i=0; i<3; i++) 
      {
      cellLoc[i] = ptLoc[i] + offset[j][i];
      if ( cellLoc[i] < 0 || cellLoc[i] >= cellDim[i] ) 
        break;
      }
    if ( i >= 3 ) //add cell
      {
      cellId = cellLoc[0] + cellLoc[1]*cellDim[0] + 
                            cellLoc[2]*cellDim[0]*cellDim[1];
      cellIds.InsertNextId(cellId);
      }
    }

  return;
}

void vtkStructuredData::_PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLWObject::_PrintSelf(os,indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";
}
