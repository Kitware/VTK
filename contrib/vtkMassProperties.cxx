/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMassProperties.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkMassProperties.h"

#define  VTK_CUBE_ROOT(x) ((x<0.0)?(-pow((-x),0.333333333333333)):(pow((x),0.333333333333333)))

// Constructs with initial 0 values.
vtkMassProperties::vtkMassProperties()
{
  this->Input = NULL;
  this->SurfaceArea = 0.0;
  this->Volume  = 0.0;
  this->VolumeX = 0.0;
  this->VolumeY = 0.0;
  this->VolumeZ = 0.0;
  this->Kx = 0.0;
  this->Ky = 0.0;
  this->Kz = 0.0;
  this->NormalizedShapeIndex = 0.0;
}

// Constructs with initial 0 values.
vtkMassProperties::~vtkMassProperties()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input == NULL;
    }
}

// Description:
// Specifies the input data...
void vtkMassProperties::SetInput(vtkPolyData *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    if (this->Input) {this->Input->UnRegister(this);}
    this->Input = (vtkPolyData *) input;
    if (this->Input) {this->Input->Register(this);}
    this->Modified();
    }
}

// Description:
// Make sure input is available then call up execute method...
void vtkMassProperties::Update()
{

 // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  this->Input->Update();

  if (this->Input->GetMTime() > this->ExecuteTime || 
      this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() )
      {
      this->Input->ForceUpdate();
      }
    if ( this->StartMethod )
      {
      (*this->StartMethod)(this->StartMethodArg);
      }

    // reset Abort flag
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute )
      {
      this->UpdateProgress(1.0);
      }

    if ( this->EndMethod )
      {
      (*this->EndMethod)(this->EndMethodArg);
      }
    }
  if ( this->Input->ShouldIReleaseData() )
    {
    this->Input->ReleaseData();
    }

}

// Description:
// This method measures volume, surface area, and normalized shape index.
// Currently, the input is a ploydata which consists of triangles.
void vtkMassProperties::Execute()
{
  vtkIdList ptIds(VTK_CELL_SIZE);
  vtkPolyData *input=(vtkPolyData *)this->Input;
  int cellId, numCells, numPts, numIds;
  float *p;
  
  ptIds.ReferenceCountingOff();
  numCells=input->GetNumberOfCells();
  numPts = input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
    {
      vtkErrorMacro(<<"No data to measure...!");
      return;
    }
  //
  // Traverse all cells, obtaining node coordinates.
  //
  double   vol[3],kxyz[3];
  double   munc[3],wxyz,wxy,wxz,wyz;
  double   area,surfacearea;
  double   a,b,c,s;
  float    x[3],y[3],z[3];
  float    i[3],j[3],k[3],u[3],absu[3],length;
  float    ii[3],jj[3],kk[3];
  float    xavg,yavg,zavg;
  int      idx;

  //
  // Initialize variables ...
  //
  surfacearea = 0.0;
  wxyz = 0; wxy = 0.0; wxz = 0.0; wyz = 0.0;
  for ( idx =0; idx < 3 ; idx++ ) 
    {
    munc[idx] = 0.0;
    vol[idx]  = 0.0;
    kxyz[idx] = 0.0;
    }

  for (cellId=0; cellId < numCells; cellId++)
    {
    if ( input->GetCellType(cellId) != VTK_TRIANGLE)
      {
      vtkErrorMacro(<<"Sorry Data type has to be VTK_TRIANGLE not " << input->GetCellType(cellId));
      return;
      }
    input->GetCellPoints(cellId,ptIds);
    numIds = ptIds.GetNumberOfIds();
    //
    // store current vertix (x,y,z) corrdinates ...
    //
    for (idx=0; idx < numIds; idx++)
      {
      p = input->GetPoint(ptIds.GetId(idx));
      x[idx] = p[0]; y[idx] = p[1]; z[idx] = p[2];
      }
    //
    // get i j k vectors ... 
    //
    i[0] = ( x[1] - x[0]); j[0] = (y[1] - y[0]); k[0] = (z[1] - z[0]);
    i[1] = ( x[2] - x[0]); j[1] = (y[2] - y[0]); k[1] = (z[2] - z[0]);
    i[2] = ( x[2] - x[1]); j[2] = (y[2] - y[1]); k[2] = (z[2] - z[1]);
    //
    // cross product between two vectors, to determine normal vector
    //
    u[0] = ( j[0] * k[1] - k[0] * j[1]);
    u[1] = ( k[0] * i[1] - i[0] * k[1]);
    u[2] = ( i[0] * j[1] - j[0] * i[1]);
    //
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

    //	
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
      vtkErrorMacro(<<"Unpredicted situation...!");
      return; 
      }
    //
    // This is reduced to ...
    //
    ii[0] = i[0] * i[0]; ii[1] = i[1] * i[1]; ii[2] = i[2] * i[2];
    jj[0] = j[0] * j[0]; jj[1] = j[1] * j[1]; jj[2] = j[2] * j[2];
    kk[0] = k[0] * k[0]; kk[1] = k[1] * k[1]; kk[2] = k[2] * k[2];

    //
    // area of a triangle...
    //
    a = sqrt(ii[1] + jj[1] + kk[1]);
    b = sqrt(ii[0] + jj[0] + kk[0]);
    c = sqrt(ii[2] + jj[2] + kk[2]);
    s = 0.5 * (a + b + c);
    area = sqrt( s*(s-a)*(s-b)*(s-c));
    surfacearea += area;

    // 
    // volume elements ... 
    //
    zavg = (z[0] + z[1] + z[2]) / 3.0;
    yavg = (y[0] + y[1] + y[2]) / 3.0;
    xavg = (x[0] + x[1] + x[2]) / 3.0;

    vol[2] += (area * (double)u[2] * (double)zavg);
    vol[1] += (area * (double)u[1] * (double)yavg);
    vol[0] += (area * (double)u[0] * (double)xavg);
    }
  //
  //  Surface Area ...
  //
  this->SurfaceArea = (double)surfacearea;

  //
  //  Weighting factors in Discrete Divergence theorem for volume calculation...
  //      
  kxyz[0] = (munc[0] + (wxyz/3.0) + ((wxy+wxz)/2.0)) /(double)(numCells);
  kxyz[1] = (munc[1] + (wxyz/3.0) + ((wxy+wyz)/2.0)) /(double)(numCells);
  kxyz[2] = (munc[2] + (wxyz/3.0) + ((wxz+wyz)/2.0)) /(double)(numCells);
  this->VolumeX = vol[0];
  this->VolumeY = vol[1];
  this->VolumeZ = vol[2];
  this->Kx = kxyz[0];
  this->Ky = kxyz[1];
  this->Kz = kxyz[2];
  this->Volume =  (kxyz[0] * vol[0] + kxyz[1] * vol[1] + kxyz[2]  * vol[2]);
  this->Volume =  fabs(this->Volume);
  this->NormalizedShapeIndex = (sqrt(surfacearea)/VTK_CUBE_ROOT(this->Volume))/2.199085233;
}

void vtkMassProperties::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProcessObject::PrintSelf(os,indent);

  if (!this->Input) 
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
  os << indent << "Surface Area: " << this->GetSurfaceArea () << "\n";
  os << indent << "Normalized Shape Index: " << this->GetNormalizedShapeIndex () << "\n";
}
