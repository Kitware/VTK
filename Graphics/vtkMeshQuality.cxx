/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeshQuality.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMeshQuality.h"
#include "vtkMath.h"
#include "vtkTetra.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMeshQuality, "1.1");
vtkStandardNewMacro(vtkMeshQuality);

//----------------------------------------------------------------------------
// Constructor
vtkMeshQuality::vtkMeshQuality() 
{
 this->GeometryOff();
 this->TopologyOff();
 this->FieldDataOff();
 this->PointDataOff();
 this->CellDataOn();
 
 this->Volume = 1;
 this->Ratio = 1;
}

//----------------------------------------------------------------------------
//destructor
vtkMeshQuality::~vtkMeshQuality() 
{ 
}

//----------------------------------------------------------------------------
// Compute the incenter (center[3]) and radius (method return value) of
// a tetrahedron defined by the four points p1, p2, p3, and p4.
double vtkMeshQuality::Insphere(double  p1[3], double p2[3], double p3[3], 
                             double p4[3], double center[3])
{
  double u[3], v[3], w[3];
  double p[3], q[3], r[3];
  double O1[3],O2[3];
  double y[3], s[3], t;
  
  u[0] = p2[0]-p1[0];
  u[1] = p2[1]-p1[1];
  u[2] = p2[2]-p1[2];
  
  v[0] = p3[0]-p1[0];
  v[1] = p3[1]-p1[1];
  v[2] = p3[2]-p1[2];
  
  w[0] = p4[0]-p1[0];
  w[1] = p4[1]-p1[1];
  w[2] = p4[2]-p1[2];
  
  vtkMath::Cross(u,v,p);
  vtkMath::Normalize(p);
  
  vtkMath::Cross(v,w,q);
  vtkMath::Normalize(q);
  
  vtkMath::Cross(w,u,r);
  vtkMath::Normalize(r);

  O1[0] = p[0]-q[0];
  O1[1] = p[1]-q[1];
  O1[2] = p[2]-q[2];

  O2[0] = q[0]-r[0];
  O2[1] = q[1]-r[1];
  O2[2] = q[2]-r[2];

  vtkMath::Cross(O1,O2,y);
  
  O1[0] = u[0]-w[0];
  O1[1] = u[1]-w[1];
  O1[2] = u[2]-w[2];

  O2[0] = v[0]-w[0];
  O2[1] = v[1]-w[1];
  O2[2] = v[2]-w[2];
  
  vtkMath::Cross(O1,O2,s);
  vtkMath::Normalize(s);
  
  s[0] = -1 * s[0];
  s[1] = -1 * s[1];
  s[2] = -1 * s[2];
  
  O1[0] = s[0]-p[0];
  O1[1] = s[1]-p[1];
  O1[2] = s[2]-p[2];
  
  t = vtkMath::Dot(w,s)/vtkMath::Dot(y,O1);
  center[0] = p1[0] + (t * y[0]);
  center[1] = p1[1] + (t * y[1]);
  center[2] = p1[2] + (t * y[2]);

  return (fabs(t* vtkMath::Dot(y,p)));
}

//----------------------------------------------------------------------------
void vtkMeshQuality::Execute()
{
  int j;
  vtkDataSet *input = this->GetInput();
  vtkIdType numCells=input->GetNumberOfCells();
  vtkIdList *id = vtkIdList::New();
  vtkCellData *celld = vtkCellData::New();
  vtkFloatArray *scalars = vtkFloatArray::New();
  if (this->Volume && this->Ratio)
    {
    scalars->SetNumberOfComponents(2);
    }
  scalars->SetNumberOfTuples(numCells);
  
  float p1[3],p2[3],p3[3],p4[3]; 
  double dp1[3],dp2[3],dp3[3],dp4[3];
  float volume, ratio;
  double incenter[3], circenter[3];
  
  for (j=0; j<numCells; j++)
    {
    input->GetCellPoints(j,id);
    input->GetPoint(id->GetId(0),p1);
    dp1[0]=p1[0]; dp1[1]=p1[1]; dp1[2]=p1[2];
    input->GetPoint(id->GetId(1),p2);
    dp2[0]=p2[0]; dp2[1]=p2[1]; dp2[2]=p2[2];
    input->GetPoint(id->GetId(2),p3);
    dp3[0]=p3[0]; dp3[1]=p3[1]; dp3[2]=p3[2];
    input->GetPoint(id->GetId(3),p4);
    dp4[0]=p4[0]; dp4[1]=p4[1]; dp4[2]=p4[2];
    
    if (this->Volume && this->Ratio)
      {
      volume = fabs(vtkTetra::ComputeVolume(dp1,dp2,dp3,dp4));
      ratio = sqrt(vtkTetra::Circumsphere(dp1,dp2,dp3,dp4, circenter))/\
        vtkMeshQuality::Insphere(dp1,dp2,dp3,dp4,incenter);
      
      ratio = ratio/3;
      
      scalars->SetTuple2(j,volume,ratio);
      }
    else if (this->Ratio)
      {
      ratio = sqrt(vtkTetra::Circumsphere(dp1,dp2,dp3,dp4, circenter))/\
        vtkMeshQuality::Insphere(dp1,dp2,dp3,dp4,incenter);
      ratio = ratio/3;
      scalars->SetTuple1(j,ratio);
      }
    else if (this->Volume)
      {
      volume = fabs(vtkTetra::ComputeVolume(dp1,dp2,dp3,dp4));
      scalars->SetTuple1(j,volume);
      }
    else
      {
      vtkErrorMacro(<<"Nothing to be calculated!!!!");
      }
    }
     
  celld->SetScalars(scalars);
  this->GetOutput()->SetFieldData(celld);
  celld->Delete();
  id->Delete();
  scalars->Delete();
}

//----------------------------------------------------------------------------
void vtkMeshQuality::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataObjectFilter::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Volume: " << (this->Volume ? "On\n" : "Off\n");
  os << indent << "Ratio: " << (this->Ratio ? "On\n" : "Off\n");
}
