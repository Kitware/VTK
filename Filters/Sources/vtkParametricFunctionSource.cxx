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
#include "vtkParametricFunction.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkPoints.h"
#include "vtkTriangleFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSmartPointer.h"

#include <math.h>
#include <string>

vtkStandardNewMacro(vtkParametricFunctionSource);
vtkCxxSetObjectMacro(vtkParametricFunctionSource,ParametricFunction,vtkParametricFunction);

//----------------------------------------------------------------------------
vtkParametricFunctionSource::vtkParametricFunctionSource() :
  ParametricFunction(NULL)
  , UResolution(50)
  , VResolution(50)
  , WResolution(50)
  , GenerateTextureCoordinates(0)
  , ScalarMode(vtkParametricFunctionSource::SCALAR_NONE)
  , OutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION)
{
  this->SetNumberOfInputPorts(0);
  this->GenerateNormals = 1;
}


//----------------------------------------------------------------------------
vtkParametricFunctionSource::~vtkParametricFunctionSource()
{
  this->SetParametricFunction(NULL);
}

namespace {
  void addTriCells(vtkIdType *&idPtr,
    int id1, int id2, int id3, int id4,
    bool clockwise)
    {
      *(idPtr++) = 3;
      if ( clockwise )
        {
        *(idPtr++) = id1;
        *(idPtr++) = id2;
        *(idPtr++) = id3;
        *(idPtr++) = 3;
        *(idPtr++) = id1;
        *(idPtr++) = id3;
        *(idPtr++) = id4;
        }
      else
        {
        *(idPtr++) = id1;
        *(idPtr++) = id3;
        *(idPtr++) = id2;
        *(idPtr++) = 3;
        *(idPtr++) = id1;
        *(idPtr++) = id4;
        *(idPtr++) = id3;
        }
      }
    }

//----------------------------------------------------------------------------
void vtkParametricFunctionSource::MakeTriangles ( vtkCellArray * cells,
                                                   int PtsU, int PtsV )
{
  int id1 = 0;
  int id2 = 0;
  int id3 = 0;
  int id4 = 0;

  vtkDebugMacro(<< "Executing MakeTriangles()");

  bool clockwise = (this->ParametricFunction->GetClockwiseOrdering() != 0);

  vtkIdType numCells = (PtsU + this->ParametricFunction->GetJoinU() - 1)*
    (PtsV + this->ParametricFunction->GetJoinV() - 1)*2;
  cells->Allocate(numCells*4,1000);
  vtkIdType *idPtr = cells->WritePointer(numCells,numCells*4);

  for ( int i = 0; i < PtsU - 1; ++i )
    {
    // Fill the allocated space with the indexes to the points.
    for ( int j = 0; j < PtsV - 1; ++j )
      {
      id1 = j + i * PtsV;
      id2 = id1 + PtsV;
      id3 = id2 + 1;
      id4 = id1 + 1;
      addTriCells(idPtr,id1,id2,id3,id4,clockwise);
      }
    // If necessary, connect the ends of the triangle strip.
    if ( this->ParametricFunction->GetJoinV() )
      {
      id1 = id4;
      id2 = id3;
      if ( this->ParametricFunction->GetTwistV() )
        {
        id3 = (i + 1) * PtsV;
        id4 = i * PtsV;
        }
      else
        {
        id3 = i * PtsV;
        id4 = (i + 1) * PtsV;
        }
      addTriCells(idPtr,id1,id2,id3,id4,clockwise);
      }
    }
  // If required, connect the last triangle strip to the first by
  // adding a new triangle strip and filling it with the indexes
  // to the points.
  if ( this->ParametricFunction->GetJoinU() )
    {
    for ( int j = 0; j < PtsV - 1; ++j )
      {
      id1 = j + (PtsU - 1) * PtsV;
      id3 = id1 + 1;
      if ( this->ParametricFunction->GetTwistU() )
        {
        id2 = PtsV - 1 - j;
        id4 = id2 - 1;
        }
      else
        {
        id2 = j;
        id4 = id2 + 1;
        }
      addTriCells(idPtr,id1,id2,id3,id4,clockwise);
      }

    // If necessary, connect the ends of the triangle strip.
    if ( this->ParametricFunction->GetJoinV() )
      {
      id1 = id3;
      id2 = id4;
      if ( this->ParametricFunction->GetTwistU() )
        {
        if ( this->ParametricFunction->GetTwistV() )
          {
          id3 = PtsV - 1;
          id4 = ( PtsU - 1 ) * PtsV;
          }
        else
          {
          id3 = ( PtsU - 1 ) * PtsV;
          id4 = PtsV - 1;
          }
        }
      else
        {
        if ( this->ParametricFunction->GetTwistV() )
          {
          id3 = 0;
          id4 = ( PtsU - 1 ) * PtsV;
          }
        else
          {
          id3 = ( PtsU - 1 ) * PtsV;
          id4 = 0;
          }
        }
      addTriCells(idPtr,id1,id2,id3,id4,clockwise);
      }
    }
  cells->Modified();
  vtkDebugMacro(<< "MakeTriangles() finished.");
}

//----------------------------------------------------------------------------
int vtkParametricFunctionSource::RequestData(vtkInformation *vtkNotUsed(info),
                                             vtkInformationVector **vtkNotUsed(inputV),
                                             vtkInformationVector *output)
{
  vtkDebugMacro(<< "Executing");

  // Check that a parametric function has been defined
  if ( !this->ParametricFunction )
    {
    vtkErrorMacro(<<"Parametric function not defined");
    return 1;
    }

  switch ( this->ParametricFunction->GetDimension() )
    {
    case 1:
      this->Produce1DOutput(output);
      break;
    case 2:
      this->Produce2DOutput(output);
      break;
    default:
      vtkErrorMacro("Functions of dimension "
                    << this->ParametricFunction->GetDimension()
                    << " are not supported.");
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkParametricFunctionSource::Produce1DOutput(vtkInformationVector *output)
{
  vtkIdType numPts = this->UResolution + 1;
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
    pts->SetDataType(VTK_DOUBLE);
    }
  else
    {
    pts->SetDataType(VTK_FLOAT);
    }

  pts->SetNumberOfPoints(numPts);
  vtkIdType i;
  double x[3], Du[3], t[3];

  lines->Allocate(lines->EstimateSize(1,numPts));
  lines->InsertNextCell(numPts);

  // Insert points and cell points
  for (i=0; i<numPts; i++)
    {
    t[0] = (double) i/this->UResolution;
    this->ParametricFunction->Evaluate(t,x,Du);
    pts->SetPoint(i,x);
    lines->InsertCellPoint(i);
    }

  vtkInformation *outInfo = output->GetInformationObject(0);
  vtkPolyData *outData = static_cast<vtkPolyData*>
    (outInfo->Get( vtkDataObject::DATA_OBJECT() ));
  outData->SetPoints(pts);
  outData->SetLines(lines);
}

//----------------------------------------------------------------------------
void vtkParametricFunctionSource::Produce2DOutput(vtkInformationVector *output)
{
  // Adjust so the ranges:
  // this->MinimumU ... this->ParametricFunction->GetMaximumU(),
  // this->MinimumV ... this->ParametricFunction->GetMaximumV()
  // are included in the triangulation.
  double MaxU = this->ParametricFunction->GetMaximumU() +
    (this->ParametricFunction->GetMaximumU() - this->ParametricFunction->GetMinimumU()) /
    (this->UResolution-1);
  int PtsU = this->UResolution;
  double MaxV = this->ParametricFunction->GetMaximumV() +
    (this->ParametricFunction->GetMaximumV() - this->ParametricFunction->GetMinimumV()) /
    (this->VResolution-1);
  int PtsV = this->VResolution;
  int totPts = PtsU * PtsV;

  // Scalars associated with each point
  vtkSmartPointer<vtkFloatArray> sval = vtkSmartPointer<vtkFloatArray>::New();
  if ( this->ScalarMode != SCALAR_NONE )
    {
    sval->SetNumberOfTuples(totPts);
    sval->SetName("Scalars");
    }

  // The normals to the surface
  vtkSmartPointer<vtkFloatArray> nval = vtkSmartPointer<vtkFloatArray>::New();
  if (this->GenerateNormals)
    {
    nval->SetNumberOfComponents(3);
    nval->SetNumberOfTuples(totPts);
    nval->SetName("Normals");
    }

  // Texture coordinates
  vtkSmartPointer<vtkFloatArray> newTCoords = vtkSmartPointer<vtkFloatArray>::New();
  if ( this->GenerateTextureCoordinates != 0 )
    {
    newTCoords->SetNumberOfComponents(2);
    newTCoords->Allocate(2*totPts);
    newTCoords->SetName("Textures");
    }

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
    points->SetDataType(VTK_DOUBLE);
    }
  else
    {
    points->SetDataType(VTK_FLOAT);
    }

  points->SetNumberOfPoints( totPts );

  double uStep = ( MaxU - this->ParametricFunction->GetMinimumU() ) / PtsU;
  double vStep = ( MaxV - this->ParametricFunction->GetMinimumV() ) / PtsV;

  // Find the mid points of the (u,v) map.
  double u0 = this->ParametricFunction->GetMinimumU();
  double u_mp = (MaxU - u0)/2.0 + u0 - uStep;
  while ( u0 < u_mp )
    {
    u0 += uStep;
    }

  double v0 = this->ParametricFunction->GetMinimumV();
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
  double uv[3];
  uv[0] = this->ParametricFunction->GetMinimumU() - uStep;

  float MaxI = PtsU - 1;
  float MaxJ = PtsV - 1;

  for ( int i = 0; i < PtsU; ++i )
    {
    uv[0] += uStep;
    uv[1] = this->ParametricFunction->GetMinimumV() - vStep;

    double tc[2];
    if ( this->GenerateTextureCoordinates != 0 )
      {
      tc[0] = i/MaxI;
      }

    for ( int j = 0; j < PtsV; ++j )
      {
      uv[1] += vStep;

      if ( this->GenerateTextureCoordinates != 0 )
        {
        tc[1] = 1.0 - j/MaxJ;
        newTCoords->InsertNextTuple(tc);
        }

      // The point
      double Pt[3];
      // Partial derivative at Pt with respect to u,v,w.
      double Du[9];
      // Partial derivative at Pt with respect to v.
      double *Dv = Du+3;

      // Calculate fn(u)->(Pt,Du).
      this->ParametricFunction->Evaluate(uv,Pt,Du);

      // Insert the points and scalar.
      points->InsertPoint(k, Pt[0], Pt[1], Pt[2]);
      double scalar;

      if ( this->ScalarMode != SCALAR_NONE )
        {
        switch ( this->ScalarMode )
          {
          case SCALAR_U:
            scalar = uv[0];
            break;
          case SCALAR_V:
            scalar = uv[1];
            break;
          case SCALAR_U0:
            scalar = uv[0] == u0 ? 1 : 0;
            break;
          case SCALAR_V0:
            scalar = uv[1] == v0 ? 1 : 0;
            break;
          case SCALAR_U0V0:
            scalar = 0;
            // u0, v0
            if ( uv[0] == u0 && uv[1] == v0 )
              {
              scalar = 3;
              }
            else
              {
              // u0 line
              if ( uv[0] == u0 )
                {
                scalar = 1;
                }
              else
                {
                // v0 line
                if ( uv[1] == v0 ) scalar = 2;
                }
              }
            break;
          case SCALAR_MODULUS:
            rel_u = uv[0] - u_mp;
            rel_v = uv[1] - v_mp;
            scalar = sqrt(rel_u * rel_u + rel_v * rel_v);
            break;
          case SCALAR_PHASE:
            rel_u = uv[0] - u_mp;
            rel_v = uv[1] - v_mp;
            if ( rel_v == 0 && rel_u == 0 )
            {
            scalar = 0;
            }
            else
            {
            scalar = vtkMath::DegreesFromRadians( atan2( rel_v, rel_u ) );
            if ( scalar < 0 ) scalar += 360;
            }
            break;
          case SCALAR_QUADRANT:
            if ( uv[0] >= u0 && uv[1] >= v0 )
              {
              scalar = 1;
              break;
              }
            if ( uv[0] < u0 && uv[1] >= v0 )
              {
              scalar = 2;
              break;
              }
            if ( uv[0] < u0 && uv[1] < v0 )
              {
              scalar = 3;
              }
            else
              {
              scalar = 4;
              }
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
          case SCALAR_FUNCTION_DEFINED:
            scalar = this->ParametricFunction->EvaluateScalar(uv, Pt, Du);
            break;
          case SCALAR_NONE:
          default:
            scalar = 0;
          }
        sval->SetValue(k, scalar);
        }

      // Calculate the normal.
      if ( this->ParametricFunction->GetDerivativesAvailable() &&
           this->GenerateNormals)
        {
        double n[3];
        vtkMath::Cross(Du,Dv,n);
        vtkMath::Normalize(n);
        nval->SetTuple3(k, n[0], n[1], n[2]);
        }

      ++k;
      }
    }

  vtkInformation *outInfo = output->GetInformationObject(0);
  vtkPolyData *outData = static_cast<vtkPolyData*>(outInfo->Get( vtkDataObject::DATA_OBJECT() ));
  vtkCellArray *tris = vtkCellArray::New();
  this->MakeTriangles ( tris, PtsU, PtsV );
  outData->SetPoints( points );
  outData->SetPolys(tris);

  if (this->GenerateNormals)
    {
    if (this->ParametricFunction->GetDerivativesAvailable())
      {
      outData->GetPointData()->SetNormals( nval );
      }
    else
      {
      // Used to hold the surface
      vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
      pd->SetPoints( points );
      pd->SetPolys( tris );
      vtkSmartPointer<vtkPolyDataNormals> norm = vtkSmartPointer<vtkPolyDataNormals>::New();
      norm->SetInputData(pd);
      norm->Update();
      outData->DeepCopy(norm->GetOutput());
      }
    }

  tris->Delete();
  if ( this->ScalarMode != SCALAR_NONE )
    {
    outData->GetPointData()->SetScalars( sval );
    }
  if ( this->GenerateTextureCoordinates != 0 )
    {
    outData->GetPointData()->SetTCoords( newTCoords );
    }
  outData->Modified();
}

/*
//----------------------------------------------------------------------------
void vtkParametricFunctionSource::GetAllParametricTriangulatorParameters (
  int & numberOfUPoints,
  int & numberOfVPoints,
  double & minimumU,
  double & maximumU,
  double & minimumV,
  double & maximumV,
  int & joinU,
  int & joinV,
  int & twistU,
  int & twistV,
  int & clockwiseOrdering,
  int & scalarMode)
{
  uResolution = this->UResolution;
  vResolution = this->VResolution;
  minimumU = this->MinimumU;
  maximumU = this->ParametricFunction->GetMaximumU();
  minimumV = this->ParametricFunction->GetMinimumV();
  maximumV = this->ParametricFunction->GetMaximumV();
  joinU = this->ParametricFunction->GetJoinU();
  joinV = this->ParametricFunction->GetJoinV;
  twistU = this->ParametricFunction->GetTwistU();
  twistV = this->ParametricFunction->GetTwistV();
  clockwiseOrdering = this->ParametricFunction->GetClockwiseOrdering();
  scalarMode = this->ScalarMode;
}

//----------------------------------------------------------------------------
void vtkParametricFunctionSource::SetAllParametricTriangulatorParameters (
  int uResolution,
  int vResolution,
  double minimumU,
  double maximumU,
  double minimumV,
  double maximumV,
  int joinU,
  int joinV,
  int twistU,
  int twistV,
  int ParametricFunction->GetclockwiseOrdering(),
  int scalarMode)
{
  this->UResolution = uResolution;
  this->VResolution = vResolution;
  this->ParametricFunction->SetMinimumU( minimumU );
  this->ParametricFunction->SetMaximumU( maximumU );
  this->ParametricFunction->SetMinimumV( minimumV );
  this->ParametricFunction->SetMaximumV( maximumV );
  this->ParametricFunction->SetJoinU( joinU );
  this->ParametricFunction->SetJoinV( joinV );
  this->ParametricFunction->SetTwistU( twistU );
  this->ParametricFunction->SetTwistV( twistV );
  this->ParametricFunction->SetClockwiseOrdering( clockwiseOrdering );
  this->ScalarMode = scalarMode;
  if ( ScalarMode < SCALAR_NONE || ScalarMode > SCALAR_USER_DEFINED )
    {
    this->ScalarMode = SCALAR_NONE;
    }
  this->Modified();
}
*/

//----------------------------------------------------------------------------
unsigned long vtkParametricFunctionSource::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long funcMTime;

  if ( this->ParametricFunction != NULL )
    {
    funcMTime = this->ParametricFunction->GetMTime();
    mTime = ( funcMTime > mTime ? funcMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkParametricFunctionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "U Resolution: " << this->UResolution << "\n";
  os << indent << "V Resolution: " << this->VResolution << "\n";
  os << indent << "W Resolution: " << this->WResolution << "\n";

  if ( this->ParametricFunction )
    {
    os << indent << "Parametric Function: " << this->ParametricFunction << "\n";
    }
  else
    {
    os << indent << "No Parametric function defined\n";
    }

  std::string s;
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
  case SCALAR_FUNCTION_DEFINED:
    s = "SCALAR_FUNCTION_DEFINED";
    break;
  default:
    s = "Unknown scalar mode.";
   }
  os << indent << "Scalar Mode: " << s.c_str() << "\n";
  os << indent << "GenerateTextureCoordinates:" << (this->GenerateTextureCoordinates ? "On" : "Off" ) << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}
