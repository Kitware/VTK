/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricFunctionSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricFunctionSource.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkPoints.h"
#include "vtkTriangleFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"

#include <cmath>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkParametricFunctionSource, "1.2");

vtkParametricFunctionSource::vtkParametricFunctionSource(void) :
  NumberOfUPoints(0)
  , NumberOfVPoints(0)
  , MinimumU(0)
  , MaximumU(0)
  , MinimumV(0)
  , MaximumV(0)
  , JoinUTessellation(0)
  , JoinVTessellation(0)
  , TwistUTessellation(0)
  , TwistVTessellation(0)
  , ClockwiseOrdering(1)
  , DerivativesSupplied(1)
  , ScalarMode(vtkParametricFunctionSource::SCALAR_NONE)
{}


vtkParametricFunctionSource::~vtkParametricFunctionSource(void)
{
}

void vtkParametricFunctionSource::MakeTriangleStrips ( vtkCellArray * strips, 
                                                 int PtsU, int PtsV )
{
  int id1;
  int id2;

  vtkDebugMacro(<< "Executing MakeTriangleStrips()");

  for ( int i = 0; i < PtsU - 1; ++i )
    {
    // Allocate space
    if ( this->JoinVTessellation )
      {
      strips->InsertNextCell( PtsV * 2 + 2 );
      }
    else
      {
      strips->InsertNextCell( PtsV * 2 );
      }

    // Fill the allocated space with the indexes to the points.
    for ( int j = 0; j < PtsV; ++j )
      {
      id1 = j + i * PtsV;
      id2 = (i + 1 ) * PtsV + j;
      if ( this->ClockwiseOrdering )
        { 
        strips->InsertCellPoint(id1);
        strips->InsertCellPoint(id2); 
        }
      else
        {
        strips->InsertCellPoint(id2);
        strips->InsertCellPoint(id1);
        }
      }
    // If necessary, connect the ends of the triangle strip.
    if ( this->JoinVTessellation )
      {
      if ( this->TwistVTessellation )
        {
        id1 = (i + 1) * PtsV;
        id2 = i * PtsV;
        }
      else
        {
        id1 = i * PtsV;
        id2 = (i + 1) * PtsV;
        }
      if ( this->ClockwiseOrdering )
        { 
        strips->InsertCellPoint(id1);
        strips->InsertCellPoint(id2); 
        }
      else
        {
        strips->InsertCellPoint(id2);
        strips->InsertCellPoint(id1);
        }
      }
    }
  // If required, connect the last triangle strip to the first by 
  // adding a new triangle strip and filling it with the indexes
  // to the points.
  if ( this->JoinUTessellation )
    {
    if ( this->JoinVTessellation )
      {
      strips->InsertNextCell( PtsV * 2 + 2 );
      }
    else
      {
      strips->InsertNextCell( PtsV * 2 );
      }
    for ( int j = 0; j < PtsV; ++j )
      {
      if ( this->TwistUTessellation )
        {
        id1 = ( PtsU - 1 ) * PtsV + j;
        id2 = PtsV - 1 - j;
        }
      else
        {
        id1 = ( PtsU - 1 ) * PtsV + j;
        id2 = j;
        }
      if ( this->ClockwiseOrdering )
        { 
        strips->InsertCellPoint(id1);
        strips->InsertCellPoint(id2); 
        }
      else
        {
        strips->InsertCellPoint(id2);
        strips->InsertCellPoint(id1);
        }
      }

    // If necessary, connect the ends of the triangle strip.
    if ( this->JoinVTessellation )
      {
      if ( this->TwistUTessellation )
        {
        if ( this->TwistVTessellation )
          {
          id1 = PtsV - 1;
          id2 = ( PtsU - 1 ) * PtsV;
          }
        else
          {
          id1 = ( PtsU - 1 ) * PtsV;
          id2 = PtsV - 1;
          }
        }
      else
        {
        if ( this->TwistVTessellation )
          {
          id1 = 0;
          id2 = ( PtsU - 1 ) * PtsV;
          }
        else
          {
          id1 = ( PtsU - 1 ) * PtsV;
          id2 = 0;
          }
        }
      if ( this->ClockwiseOrdering )
        { 
        strips->InsertCellPoint(id1);
        strips->InsertCellPoint(id2); 
        }
      else
        {
        strips->InsertCellPoint(id2);
        strips->InsertCellPoint(id1);
        }
      }
    }
  vtkDebugMacro(<< "MakeTriangleStrips() finished.");
}

void vtkParametricFunctionSource::SetScalarModeToNone( void )
{
   this->ScalarMode = SCALAR_NONE;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToU( void )
{
   this->ScalarMode = SCALAR_U;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToV( void )
{
   this->ScalarMode = SCALAR_V;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToU0( void )
{
   this->ScalarMode = SCALAR_U0;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToV0( void )
{
   this->ScalarMode = SCALAR_V0;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToU0V0( void )
{
   this->ScalarMode = SCALAR_U0V0;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToModulus( void )
{
   this->ScalarMode = SCALAR_MODULUS;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToPhase( void )
{
   this->ScalarMode = SCALAR_PHASE;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToQuadrant( void )
{
   this->ScalarMode = SCALAR_QUADRANT;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToX( void )
{
   this->ScalarMode = SCALAR_X;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToY( void )
{
   this->ScalarMode = SCALAR_Y; 
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToZ( void )
{
   this->ScalarMode = SCALAR_Z;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToDistance( void )
{
   this->ScalarMode = SCALAR_DISTANCE;
   this->Modified();
}

void vtkParametricFunctionSource::SetScalarModeToUserDefined( void )
{
   this->ScalarMode = SCALAR_USER_DEFINED;
   this->Modified();
}

void vtkParametricFunctionSource::Execute()
{
  vtkDebugMacro(<< "Execute - started constructing the surface.");
  vtkPolyData * pd = vtkPolyData::New();
  vtkMath * math = vtkMath::New();

  // Adjust so the range this->MinimumU ... this->MaximumU, this->MinimumV
  // ... this->MaximumV is included in the triangulation.
  double MaxU = this->MaximumU + 
    (this->MaximumU - this->MinimumU)/this->NumberOfUPoints;
  int PtsU = this->NumberOfUPoints + 1;
  double MaxV = this->MaximumV + 
    (this->MaximumV - this->MinimumV)/this->NumberOfVPoints;
  int PtsV = this->NumberOfVPoints + 1;
  int totPts = PtsU * PtsV;

  // Scalars associated with each point 
  vtkFloatArray * sval = vtkFloatArray::New(); 
  sval->SetNumberOfTuples( totPts ); 

  // The normals to the surface
  vtkFloatArray * nval = vtkFloatArray::New();
  nval->SetNumberOfComponents(3);
  nval->SetNumberOfTuples(totPts); 

  vtkPoints * points = vtkPoints::New();
  points->SetNumberOfPoints( totPts );

  double uStep = ( MaxU - this->MinimumU ) / PtsU;
  double vStep = ( MaxV - this->MinimumV ) / PtsV;

  // Find the mid points of the (u,v) map.
  double u0 = this->MinimumU;
  double u_mp = (MaxU - u0)/2.0 + u0 - uStep;
  while ( u0 < u_mp ) 
    {
    u0 += uStep;
    }
  
  double v0 = this->MinimumV;
  double v_mp = (MaxV - v0)/2.0 + v0 - vStep;
  while ( v0 < v_mp )  
    {
    v0 += vStep;
    }
  u_mp += uStep;
  v_mp += vStep;

  // At this point (u_mp, v_mp) is the midpoint of the (u,v) map and (u0,v0)
  // corresponds to the nearest grid point to the midpoint of the (u,v) map.
  //
  double rel_u = 0; // will be u - u_mp
  double rel_v = 0; // will be v - v_mp

  int k = 0;
  double u = this->MinimumU - uStep;

  for ( int i = 0; i < PtsU; ++i )
    {
    u += uStep;
    double v = this->MinimumV - vStep;

    for ( int j = 0; j < PtsV; ++j )
      {
      v += vStep;

      // The point
      double Pt[3];
      // Partial derivative at Pt with respect to u.
      double Du[3];
      // Partial derivative at Pt with respect to v.
      double Dv[3];

      // Calculate fn(u,v)->(Pt,Du,Dv).
      this->Evaluate(u,v,Pt,Du,Dv);

      double scalar;

      // Insert the points and scalar.
      points->InsertPoint(k, Pt[0], Pt[1], Pt[2]);

      if ( this->ScalarMode != SCALAR_NONE )
        {
        switch ( this->ScalarMode )
          {
          case SCALAR_U:
            scalar = u;
            break;
          case SCALAR_V:
            scalar = v;
            break;
          case SCALAR_U0:
            scalar = u==u0?1:0;
            break;
          case SCALAR_V0:
            scalar = v==v0?1:0;
            break;
          case SCALAR_U0V0:
            scalar = 0;
            // u0, v0
            if ( u==u0 && v==v0 ) 
              scalar = 3;
            else
              {
              // u0 line
              if ( u==u0 ) 
                {
                scalar = 1;
                }
              else 
                {
                // v0 line
                if ( v==v0 ) scalar = 2;
                }
              }
            break;
          case SCALAR_MODULUS:
            rel_u = u - u_mp;
            rel_v = v - v_mp;
            scalar = sqrt(rel_u * rel_u + rel_v * rel_v);
            break;
          case SCALAR_PHASE:
            rel_u = u - u_mp;
            rel_v = v - v_mp;
            scalar = math->RadiansToDegrees() * atan2(rel_v,rel_u);
            if ( scalar < 0 )
              scalar += 360;
            break;
          case SCALAR_QUADRANT:
            if ( u>=u0 && v>=v0 )
              {
              scalar = 1;
              break;
              }
            if ( u<u0 && v>=v0 ) 
              {
              scalar = 2;
              break;
              }
            if ( u<u0 && v<v0 ) 
              scalar = 3;
            else 
              scalar = 4;
            break;
          case SCALAR_X:
            scalar = Pt[0];
            break;
          case SCALAR_Y:
            scalar = Pt[1];
            break;
          case SCALAR_Z:
            scalar = Pt[2];
            break;
          case SCALAR_DISTANCE:
            scalar = sqrt(Pt[0]*Pt[0] + Pt[1]*Pt[1] + Pt[2]*Pt[2]);
            break;
          case SCALAR_USER_DEFINED:
            scalar = this->EvaluateScalar(u, v, Pt, Du, Dv);
            break;
          case SCALAR_NONE:
          default:
            scalar = 0;
          }
        sval->SetValue(k, scalar);
        }

      // Calculate the normal.
      if ( this->DerivativesSupplied )
        {
        double n[3];
        math->Cross(Du,Dv,n);
        math->Normalize(n);
        nval->SetTuple3(k, n[0], n[1], n[2]);
        }

      ++k;
      }
    }

  // Make the triangle strips
  vtkCellArray * strips = vtkCellArray::New(); 
  // This is now a list of ID's defining the triangles.
  this->MakeTriangleStrips ( strips, PtsU, PtsV ); 

  pd->SetPoints( points ); 
  pd->SetVerts( strips );
  pd->SetStrips( strips );
  if ( this->ScalarMode != SCALAR_NONE )
    {
    pd->GetPointData()->SetScalars( sval );
    }

  if ( this->DerivativesSupplied )
    {
    pd->GetPointData()->SetNormals( nval );
    }
  pd->Modified();

  vtkTriangleFilter * tri = vtkTriangleFilter::New();
  vtkPolyDataNormals * norm = vtkPolyDataNormals::New();
  if ( this->DerivativesSupplied )
    {
    //Generate polygons from the triangle strips
    tri->SetInput(pd);
    }
  else
    {
    // Calculate Normals
    norm->SetInput(pd);
    // Generate polygons from the triangle strips
    tri->SetInput(norm->GetOutput());
    }
  tri->PassLinesOn();
  tri->PassVertsOff();
  tri->Update();

  this->GetOutput()->DeepCopy(tri->GetOutput());

  // Were done, clean up.
  math->Delete();
  points->Delete();
  sval->Delete();
  nval->Delete();
  strips->Delete();
  pd->Delete();
  tri->Delete();
  norm->Delete();

  vtkDebugMacro(<< "Execute - finished constructing the surface.");
  //vtkPolyData *output = this->GetOutput();

  //ConstructSurface( output );
}

void vtkParametricFunctionSource::GetAllParametricTriangulatorParameters (
  int & numberOfUPoints,
  int & numberOfVPoints,
  double & minimumU,
  double & maximumU,
  double & minimumV,
  double & maximumV,
  int & joinUTessellation,
  int & joinVTessellation,
  int & twistUTessellation,
  int & twistVTessellation,
  int & clockwiseOrdering,
  int & scalarMode)
{
  numberOfUPoints = this->NumberOfUPoints;
  numberOfVPoints = this->NumberOfVPoints;
  minimumU = this->MinimumU;
  maximumU = this->MaximumU;
  minimumV = this->MinimumV;
  maximumV = this->MaximumV;
  joinUTessellation = this->JoinUTessellation;
  joinVTessellation = this->JoinVTessellation;
  twistUTessellation = this->TwistUTessellation;
  twistVTessellation = this->TwistVTessellation;
  clockwiseOrdering = this->ClockwiseOrdering;
  scalarMode = this->ScalarMode;
}

void vtkParametricFunctionSource::SetAllParametricTriangulatorParameters (
  int numberOfUPoints,
  int numberOfVPoints,
  double minimumU,
  double maximumU,
  double minimumV,
  double maximumV,
  int joinUTessellation,
  int joinVTessellation,
  int twistUTessellation,
  int twistVTessellation,
  int clockwiseOrdering,
  int scalarMode)
{
  this->NumberOfUPoints = numberOfUPoints;
  this->NumberOfVPoints = numberOfVPoints;
  this->MinimumU = minimumU;
  this->MaximumU = maximumU;
  this->MinimumV = minimumV;
  this->MaximumV = maximumV;
  this->JoinUTessellation = joinUTessellation;
  this->JoinVTessellation = joinVTessellation;
  this->TwistUTessellation = twistUTessellation;
  this->TwistVTessellation = twistVTessellation;
  this->ClockwiseOrdering = clockwiseOrdering;
  this->ScalarMode = scalarMode;
  if ( ScalarMode < SCALAR_NONE || ScalarMode > SCALAR_USER_DEFINED )
    {
    this->ScalarMode = SCALAR_NONE;
    }
  this->Modified();
}


void vtkParametricFunctionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfUPoints: " << this->NumberOfUPoints << "\n";
  os << indent << "NumberOfVPoints: " << this->NumberOfVPoints << "\n";
  os << indent << "MinimumU: " << this->MinimumU << "\n";
  os << indent << "MaximumU: " << this->MaximumU << "\n";
  os << indent << "MinimumV: " << this->MinimumV << "\n";
  os << indent << "MaximumV: " << this->MaximumV << "\n";
  os << indent << "JoinUTessellation: " << this->JoinUTessellation << "\n";
  os << indent << "JoinVTessellation: " << this->JoinVTessellation << "\n";
  os << indent << "TwistUTessellation: " << this->TwistUTessellation << "\n";
  os << indent << "TwistVTessellation: " << this->TwistVTessellation << "\n";
  os << indent << "ClockwiseOrdering: " << this->ClockwiseOrdering << "\n";
  os << indent << "Derivatives Supplied: " << this->DerivativesSupplied << "\n";
  vtkstd::string s;
  switch ( this->ScalarMode )
  {
  case SCALAR_NONE:
    s = "SCALAR_NONE";
    break;
  case SCALAR_U:
    s = "SCALAR_U";
    break; 
  case SCALAR_V:
    s = "SCALAR_V";
    break;
  case SCALAR_U0:
    s = "SCALAR_U0";
    break;
  case SCALAR_V0:
    s = "SCALAR_V0";
    break;
  case SCALAR_U0V0:
    s = "SCALAR_U0V0";
    break;
  case SCALAR_MODULUS:
    s = "SCALAR_MODULUS";
    break;
  case SCALAR_PHASE:
    s = "SCALAR_PHASE";
    break;
  case SCALAR_QUADRANT:
    s = "SCALAR_QUADRANT";
    break;
  case SCALAR_X:
    s = "SCALAR_X";
    break;
  case SCALAR_Y:
    s = "SCALAR_Y";
    break; 
  case SCALAR_Z:
    s = "SCALAR_Z";
    break;
  case SCALAR_DISTANCE:
    s = "SCALAR_DISTANCE";
    break;
  case SCALAR_USER_DEFINED:
    s = "SCALAR_USER_DEFINED";
    break;
  default:
    s = "Unknown scalar mode.";
   }
  os << indent << "Scalar Mode: " << s.c_str() << "\n";

}
