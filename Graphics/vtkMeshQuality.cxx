/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeshQuality.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2003-2006 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: dcthomp@sandia.gov,pppebay@sandia.gov

=========================================================================*/
#include "vtkMeshQuality.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkCellTypes.h"
#include "vtkPoints.h"
#include "vtkMath.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"

#include "verdict.h"

vtkCxxRevisionMacro(vtkMeshQuality,"1.42");
vtkStandardNewMacro(vtkMeshQuality);

typedef double (*CellQualityType)( vtkCell*  );

double TetVolume( vtkCell* cell );

const char* QualityMeasureNames[] =
{
  "EdgeRatio",
  "AspectRatio",
  "RadiusRatio",
  "AspectFrobenius",
  "MedAspectFrobenius",
  "MaxAspectFrobenius",
  "MaxEdgeRatios",
  "MinAngle",
  "CollapseRatio",
  "MaxAngle",
  "Condition",
  "ScaledJacobian",
  "Shear",
  "RelativeSizeSquared",
  "Shape",
  "ShapeAndSize",
  "Distortion"
};

double vtkMeshQuality::CurrentTriNormal[3];

static double TriangleArea( vtkCell* cell )
{
  double p0[3],p1[3],p2[3];
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  return vtkTriangle::TriangleArea( p0, p1, p2 );
}

void vtkMeshQuality::PrintSelf(ostream& os, vtkIndent indent )
{
  const char onStr[] = "On";
  const char offStr[] = "Off";

  this->Superclass::PrintSelf( os, indent );

  os << indent << "SaveCellQuality:   "
     << (this->SaveCellQuality ? onStr : offStr) << endl;
  os << indent << "TriangleQualityMeasure: "
     << QualityMeasureNames[this->TriangleQualityMeasure] << endl;
  os << indent << "QuadQualityMeasure: "
     << QualityMeasureNames[this->QuadQualityMeasure] << endl;
  os << indent << "TetQualityMeasure: "
     << QualityMeasureNames[this->TetQualityMeasure] << endl;
  os << indent << "HexQualityMeasure: "
     << QualityMeasureNames[this->HexQualityMeasure] << endl;
  os << indent << "Volume: " 
     << (this->Volume ? onStr : offStr) << endl;
  os << indent << "CompatibilityMode: " 
     << (this->CompatibilityMode ? onStr : offStr) << endl;
}

vtkMeshQuality::vtkMeshQuality()
{
  this->SaveCellQuality = 1; // Default is On
  this->TriangleQualityMeasure = VTK_QUALITY_ASPECT_RATIO;
  this->QuadQualityMeasure = VTK_QUALITY_EDGE_RATIO;
  this->TetQualityMeasure = VTK_QUALITY_ASPECT_RATIO;
  this->HexQualityMeasure = VTK_QUALITY_MAX_ASPECT_FROBENIUS;
  this->Volume = 0;
  this->CompatibilityMode = 0;
}

vtkMeshQuality::~vtkMeshQuality()
{
  // Nothing yet.
}

int vtkMeshQuality::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *in = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *out = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  CellQualityType TriangleQuality,QuadQuality,TetQuality,HexQuality;
  vtkDoubleArray* quality = 0;
  vtkDoubleArray* volume = 0;
  vtkIdType N = in->GetNumberOfCells();
  double qtrim,qtriM,Eqtri,Eqtri2;
  double qquam,qquaM,Eqqua,Eqqua2;
  double qtetm,qtetM,Eqtet,Eqtet2;
  double qhexm,qhexM,Eqhex,Eqhex2;
  double q;
  double V = 0.;
  vtkIdType ntri = 0;
  vtkIdType nqua = 0;
  vtkIdType ntet = 0;
  vtkIdType nhex = 0;
  vtkCell* cell;
  int progressNumer = 0;
  double progressDenom = 20.;

  this->CellNormals = in->GetCellData()->GetNormals();

  if ( this->CellNormals  )
    v_set_tri_normal_func( (ComputeNormal) vtkMeshQuality::GetCurrentTriangleNormal );
  else
    v_set_tri_normal_func( 0 );

  // Initialize the min and max values, std deviations, etc.
  qtriM = qquaM = qtetM = qhexM = VTK_DOUBLE_MIN;
  qtrim = qquam = qtetm = qhexm = VTK_DOUBLE_MAX;
  Eqtri = Eqtri2 = Eqqua = Eqqua2 = Eqtet = Eqtet2 = Eqhex = Eqhex2 = 0.;

  switch ( this->GetTriangleQualityMeasure() )
    {
    case VTK_QUALITY_EDGE_RATIO:
      TriangleQuality = TriangleEdgeRatio;
      break;
    case VTK_QUALITY_ASPECT_RATIO:
      TriangleQuality = TriangleAspectRatio;
      break;
    case VTK_QUALITY_RADIUS_RATIO:
      TriangleQuality = TriangleRadiusRatio;
      break;
    case VTK_QUALITY_ASPECT_FROBENIUS:
      TriangleQuality = TriangleAspectFrobenius;
      break;
    case VTK_QUALITY_MIN_ANGLE:
      TriangleQuality = TriangleMinAngle;
      break;
    case VTK_QUALITY_MAX_ANGLE:
      TriangleQuality = TriangleMaxAngle;
      break;
    case VTK_QUALITY_CONDITION:
      TriangleQuality = TriangleCondition;
      break;
    case VTK_QUALITY_SCALED_JACOBIAN:
      TriangleQuality = TriangleScaledJacobian;
      break;
    case VTK_QUALITY_RELATIVE_SIZE_SQUARED:
      TriangleQuality = TriangleRelativeSizeSquared;
      break;
    case VTK_QUALITY_SHAPE:
      TriangleQuality = TriangleShape;
      break;
    case VTK_QUALITY_SHAPE_AND_SIZE:
      TriangleQuality = TriangleShapeAndSize;
      break;
    case VTK_QUALITY_DISTORTION:
      TriangleQuality = TriangleDistortion;
      break;
    default:
      vtkWarningMacro( "Bad TriangleQualityMeasure ("
        << this->GetTriangleQualityMeasure() << "), using RadiusRatio instead");
      TriangleQuality = TriangleRadiusRatio;
      break;
    }

  switch ( this->GetQuadQualityMeasure() )
    {
    case VTK_QUALITY_EDGE_RATIO:
      QuadQuality = QuadEdgeRatio;
      break;
    case VTK_QUALITY_ASPECT_RATIO:
      QuadQuality = QuadAspectRatio;
      break;
    case VTK_QUALITY_RADIUS_RATIO:
      QuadQuality = QuadRadiusRatio;
      break;
    case VTK_QUALITY_MED_ASPECT_FROBENIUS:
      QuadQuality = QuadMedAspectFrobenius;
      break;
    case VTK_QUALITY_MAX_ASPECT_FROBENIUS:
      QuadQuality = QuadMaxAspectFrobenius;
      break;
    case VTK_QUALITY_MIN_ANGLE:
      QuadQuality = QuadMinAngle;
      break;
    case VTK_QUALITY_MAX_EDGE_RATIOS:
      QuadQuality = QuadMaxEdgeRatios;
      break;
    case VTK_QUALITY_SKEW:
      QuadQuality = QuadSkew;
      break;
    case VTK_QUALITY_TAPER:
      QuadQuality = QuadTaper;
      break;
    case VTK_QUALITY_WARPAGE:
      QuadQuality = QuadWarpage;
      break;
    case VTK_QUALITY_AREA:
      QuadQuality = QuadArea;
      break;
    case VTK_QUALITY_STRETCH:
      QuadQuality = QuadStretch;
      break;
    //case VTK_QUALITY_MIN_ANGLE:
    case VTK_QUALITY_MAX_ANGLE:
      QuadQuality = QuadMaxAngle;
      break;
    case VTK_QUALITY_ODDY:
      QuadQuality = QuadOddy;
      break;
    case VTK_QUALITY_CONDITION:
      QuadQuality = QuadCondition;
      break;
    case VTK_QUALITY_JACOBIAN:
      QuadQuality = QuadJacobian;
      break;
    case VTK_QUALITY_SCALED_JACOBIAN:
      QuadQuality = QuadScaledJacobian;
      break;
    case VTK_QUALITY_SHEAR:
      QuadQuality = QuadShear;
      break;
    case VTK_QUALITY_SHAPE:
      QuadQuality = QuadShape;
      break;
    case VTK_QUALITY_RELATIVE_SIZE_SQUARED:
      QuadQuality = QuadRelativeSizeSquared;
      break;
    case VTK_QUALITY_SHAPE_AND_SIZE:
      QuadQuality = QuadShapeAndSize;
      break;
    case VTK_QUALITY_SHEAR_AND_SIZE:
      QuadQuality = QuadShearAndSize;
      break;
    case VTK_QUALITY_DISTORTION:
      QuadQuality = QuadDistortion;
      break;
    default:
      vtkWarningMacro( "Bad QuadQualityMeasure ("
        << this->GetQuadQualityMeasure() << "), using EdgeRatio instead");
      QuadQuality = QuadEdgeRatio;
      break;
    }

  switch ( this->GetTetQualityMeasure() )
    {
    case VTK_QUALITY_EDGE_RATIO:
      TetQuality = TetEdgeRatio;
      break;
    case VTK_QUALITY_ASPECT_RATIO:
      TetQuality = TetAspectRatio;
      break;
    case VTK_QUALITY_RADIUS_RATIO:
      TetQuality = TetRadiusRatio;
      break;
    case VTK_QUALITY_ASPECT_FROBENIUS:
      TetQuality = TetAspectFrobenius;
      break;
    case VTK_QUALITY_MIN_ANGLE:
      TetQuality = TetMinAngle;
      break;
    case VTK_QUALITY_COLLAPSE_RATIO:
      TetQuality = TetCollapseRatio;
      break;
    case VTK_QUALITY_ASPECT_GAMMA:
      TetQuality = TetAspectGamma;
      break;
    case VTK_QUALITY_VOLUME:
      TetQuality = TetVolume;
      break;
    case VTK_QUALITY_CONDITION:
      TetQuality = TetCondition;
      break;
    case VTK_QUALITY_JACOBIAN:
      TetQuality = TetJacobian;
      break;
    case VTK_QUALITY_SCALED_JACOBIAN:
      TetQuality = TetScaledJacobian;
      break;
    case VTK_QUALITY_SHAPE:
      TetQuality = TetShape;
      break;
    case VTK_QUALITY_RELATIVE_SIZE_SQUARED:
      TetQuality = TetRelativeSizeSquared;
      break;
    case VTK_QUALITY_SHAPE_AND_SIZE:
      TetQuality = TetShapeandSize;
      break;
    case VTK_QUALITY_DISTORTION:
      TetQuality = TetDistortion;
      break;
    default:
      vtkWarningMacro( "Bad TetQualityMeasure ("
        << this->GetTetQualityMeasure() << "), using RadiusRatio instead");
      TetQuality = TetRadiusRatio;
      break;
    }

  switch ( this->GetHexQualityMeasure() )
    {
    case VTK_QUALITY_EDGE_RATIO:
      HexQuality = HexEdgeRatio;
      break;
    case VTK_QUALITY_MED_ASPECT_FROBENIUS:
      HexQuality = HexMedAspectFrobenius;
      break;
    case VTK_QUALITY_MAX_ASPECT_FROBENIUS:
      HexQuality = HexMaxAspectFrobenius;
      break;
    case VTK_QUALITY_MAX_EDGE_RATIOS:
      HexQuality = HexMaxEdgeRatios;
      break;
    case VTK_QUALITY_SKEW:
      HexQuality = HexSkew;
      break;
    case VTK_QUALITY_TAPER:
      HexQuality = HexTaper;
      break;
    case VTK_QUALITY_VOLUME:
      HexQuality = HexVolume;
      break;
    case VTK_QUALITY_STRETCH:
      HexQuality = HexStretch;
      break;
    case VTK_QUALITY_DIAGONAL:
      HexQuality = HexDiagonal;
      break;
    case VTK_QUALITY_DIMENSION:
      HexQuality = HexDimension;
      break;
    case VTK_QUALITY_ODDY:
      HexQuality = HexOddy;
      break;
    case VTK_QUALITY_CONDITION:
      HexQuality = HexCondition;
      break;
    case VTK_QUALITY_JACOBIAN:
      HexQuality = HexJacobian;
      break;
    case VTK_QUALITY_SCALED_JACOBIAN:
      HexQuality = HexScaledJacobian;
      break;
    case VTK_QUALITY_SHEAR:
      HexQuality = HexShear;
      break;
    case VTK_QUALITY_SHAPE:
      HexQuality = HexShape;
      break;
    case VTK_QUALITY_RELATIVE_SIZE_SQUARED:
      HexQuality = HexRelativeSizeSquared;
      break;
    case VTK_QUALITY_SHAPE_AND_SIZE:
      HexQuality = HexShapeAndSize;
      break;
    case VTK_QUALITY_SHEAR_AND_SIZE:
      HexQuality = HexShearAndSize;
      break;
    case VTK_QUALITY_DISTORTION:
      HexQuality = HexDistortion;
      break;
    default:
      vtkWarningMacro( "Bad HexQualityMeasure ("
        << this->GetTetQualityMeasure() << "), using MaxAspectFrobenius instead");
      HexQuality = HexEdgeRatio;
      break;
    }

  out->ShallowCopy( in );

  if ( this->SaveCellQuality )
    {
    quality = vtkDoubleArray::New();
    if ( this->CompatibilityMode )
      {
      if ( this->Volume )
        {
        quality->SetNumberOfComponents(2);
        }
      else
        {
        quality->SetNumberOfComponents(1);
        }
      }
    else
      {
      quality->SetNumberOfComponents(1);
      }
    quality->SetNumberOfTuples( N );
    quality->SetName( "Quality" );
    out->GetCellData()->AddArray( quality );
    out->GetCellData()->SetActiveAttribute( "Quality", vtkDataSetAttributes::SCALARS );
    quality->Delete();

    if ( ! this->CompatibilityMode )
      {
      if ( this->Volume )
        {
        volume = vtkDoubleArray::New();
        volume->SetNumberOfComponents(1);
        volume->SetNumberOfTuples( N );
        volume->SetName( "Volume" );
        out->GetCellData()->AddArray( volume );
        volume->Delete();
        }
      }
    }

  // These measures require the average area/volume for all cells of the same type in the mesh.
  // Either use the hinted value (computed by a previous vtkMeshQuality filter) or compute it.
  if ( this->GetTriangleQualityMeasure() == VTK_QUALITY_RELATIVE_SIZE_SQUARED ||
       this->GetTriangleQualityMeasure() == VTK_QUALITY_SHAPE_AND_SIZE ||
       this->GetQuadQualityMeasure() == VTK_QUALITY_RELATIVE_SIZE_SQUARED ||
       this->GetQuadQualityMeasure() == VTK_QUALITY_SHAPE_AND_SIZE ||
       this->GetQuadQualityMeasure() == VTK_QUALITY_SHEAR_AND_SIZE ||
       this->GetTetQualityMeasure() == VTK_QUALITY_RELATIVE_SIZE_SQUARED ||
       this->GetTetQualityMeasure() == VTK_QUALITY_SHAPE_AND_SIZE ||
       this->GetHexQualityMeasure() == VTK_QUALITY_RELATIVE_SIZE_SQUARED ||
       this->GetHexQualityMeasure() == VTK_QUALITY_SHAPE_AND_SIZE ||
       this->GetHexQualityMeasure() == VTK_QUALITY_SHEAR_AND_SIZE )
    {
    vtkDataArray* triAreaHint = in->GetFieldData()->GetArray( "TriArea" );
    vtkDataArray* quadAreaHint = in->GetFieldData()->GetArray( "QuadArea" );
    vtkDataArray* tetVolHint = in->GetFieldData()->GetArray( "TetVolume" );
    vtkDataArray* hexVolHint = in->GetFieldData()->GetArray( "HexVolume" );

    double triAreaTuple[5];
    double quadAreaTuple[5];
    double tetVolTuple[5];
    double hexVolTuple[5];

    if ( triAreaHint  &&  triAreaHint->GetNumberOfTuples() > 0 &&  triAreaHint->GetNumberOfComponents() == 5 &&
         quadAreaHint && quadAreaHint->GetNumberOfTuples() > 0 && quadAreaHint->GetNumberOfComponents() == 5 &&
         tetVolHint   &&   tetVolHint->GetNumberOfTuples() > 0 &&   tetVolHint->GetNumberOfComponents() == 5 &&
         hexVolHint   &&   hexVolHint->GetNumberOfTuples() > 0 &&   hexVolHint->GetNumberOfComponents() == 5 )
      {
      triAreaHint->GetTuple( 0, triAreaTuple );
      quadAreaHint->GetTuple( 0, quadAreaTuple );
      tetVolHint->GetTuple( 0, tetVolTuple );
      hexVolHint->GetTuple( 0, hexVolTuple );
      v_set_tri_size( triAreaTuple[1] / triAreaTuple[4] );
      v_set_quad_size( quadAreaTuple[1] / quadAreaTuple[4] );
      v_set_tet_size(  tetVolTuple[1] / tetVolTuple[4] );
      v_set_hex_size(  hexVolTuple[1] / hexVolTuple[4] );
      }
    else
      {
      for ( int i = 0; i < 5; ++i ) {
        triAreaTuple[i]  = 0;
        quadAreaTuple[i] = 0;
        tetVolTuple[i]   = 0;
        hexVolTuple[i]   = 0;
      }
      for ( vtkIdType c = 0; c < N; ++c )
        {
        double a, v; // area and volume
        cell = out->GetCell( c );
        switch ( cell->GetCellType() )
          {
        case VTK_TRIANGLE:
          a = TriangleArea( cell );
          if ( a > triAreaTuple[2] )
            {
            if ( triAreaTuple[0] == triAreaTuple[2] )
              { // min == max => min has not been set
              triAreaTuple[0] = a;
              }
            triAreaTuple[2] = a;
            }
          else if ( a < triAreaTuple[0] )
            {
            triAreaTuple[0] = a;
            }
          triAreaTuple[1] += a;
          triAreaTuple[3] += a * a;
          ntri++;
          break;
        case VTK_QUAD:
          a = QuadArea( cell );
          if ( a > quadAreaTuple[2] )
            {
            if ( quadAreaTuple[0] == quadAreaTuple[2] )
              { // min == max => min has not been set
              quadAreaTuple[0] = a;
              }
            quadAreaTuple[2] = a;
            }
          else if ( a < quadAreaTuple[0] )
            {
            quadAreaTuple[0] = a;
            }
          quadAreaTuple[1] += a;
          quadAreaTuple[3] += a * a;
          nqua++;
          break;
        case VTK_TETRA:
          v = TetVolume( cell );
          if ( v > tetVolTuple[2] )
            {
            if ( tetVolTuple[0] == tetVolTuple[2] )
              { // min == max => min has not been set
              tetVolTuple[0] = v;
              }
            tetVolTuple[2] = v;
            }
          else if ( v < tetVolTuple[0] )
            {
            tetVolTuple[0] = v;
            }
          tetVolTuple[1] += v;
          tetVolTuple[3] += v * v;
          ntet++;
          break;
        case VTK_HEXAHEDRON:
          v = HexVolume( cell );
          if ( v > hexVolTuple[2] )
            {
            if ( hexVolTuple[0] == hexVolTuple[2] )
              { // min == max => min has not been set
              hexVolTuple[0] = v;
              }
            hexVolTuple[2] = v;
            }
          else if ( v < hexVolTuple[0] )
            {
            hexVolTuple[0] = v;
            }
          hexVolTuple[1] += v;
          hexVolTuple[3] += v * v;
          nhex++;
          break;
          }
        }
      triAreaTuple[4]  = ntri;
      quadAreaTuple[4] = nqua;
      tetVolTuple[4]   = ntet;
      hexVolTuple[4]   = nhex;
      v_set_tri_size( triAreaTuple[1] / triAreaTuple[4] );
      v_set_quad_size( quadAreaTuple[1] / quadAreaTuple[4] );
      v_set_tet_size( tetVolTuple[1] / tetVolTuple[4] );
      v_set_hex_size( hexVolTuple[1] / hexVolTuple[4] );
      progressNumer = 20;
      progressDenom = 40.;
      ntri = 0;
      nqua = 0;
      ntet = 0;
      nhex = 0;

      // Save info as field data for downstream filters
      triAreaHint = vtkDoubleArray::New();
      triAreaHint->SetName( "TriArea" );
      triAreaHint->SetNumberOfComponents( 5 );
      triAreaHint->InsertNextTuple( triAreaTuple );
      out->GetFieldData()->AddArray( triAreaHint );
      triAreaHint->Delete();

      quadAreaHint = vtkDoubleArray::New();
      quadAreaHint->SetName( "QuadArea" );
      quadAreaHint->SetNumberOfComponents( 5 );
      quadAreaHint->InsertNextTuple( quadAreaTuple );
      out->GetFieldData()->AddArray( quadAreaHint );
      quadAreaHint->Delete();

      tetVolHint = vtkDoubleArray::New();
      tetVolHint->SetName( "TetVolume" );
      tetVolHint->SetNumberOfComponents( 5 );
      tetVolHint->InsertNextTuple( tetVolTuple );
      out->GetFieldData()->AddArray( tetVolHint );
      tetVolHint->Delete();

      hexVolHint = vtkDoubleArray::New();
      hexVolHint->SetName( "HexVolume" );
      hexVolHint->SetNumberOfComponents( 5 );
      hexVolHint->InsertNextTuple( hexVolTuple );
      out->GetFieldData()->AddArray( hexVolHint );
      hexVolHint->Delete();
      }
    }

  int p;
  vtkIdType c = 0;
  vtkIdType sz = N / 20 + 1;
  vtkIdType inner;
  this->UpdateProgress( progressNumer/progressDenom + 0.01 );
  for ( p = 0; p < 20; ++p )
    {
    for ( inner = 0; (inner < sz && c < N); ++c, ++inner )
      {
      cell = out->GetCell( c );
      V = 0.;
      switch ( cell->GetCellType() )
        {
      case VTK_TRIANGLE:
        if ( this->CellNormals )
          this->CellNormals->GetTuple( c, vtkMeshQuality::CurrentTriNormal );
        q = TriangleQuality( cell );
        if ( q > qtriM )
          {
          if ( qtrim > qtriM )
            {
            qtrim = q;
            }
          qtriM = q;
          }
        else if ( q < qtrim )
          {
          qtrim = q;
          }
        Eqtri += q;
        Eqtri2 += q * q;
        ntri++;
        break;
      case VTK_QUAD:
        q = QuadQuality( cell );
        if ( q > qquaM )
          {
          if ( qquam > qquaM )
            {
            qquam = q;
            }
          qquaM = q;
          }
        else if ( q < qquam )
          {
          qquam = q;
          }
        Eqqua += q;
        Eqqua2 += q * q;
        nqua++;
        break;
      case VTK_TETRA:
        q = TetQuality( cell );
        if ( q > qtetM )
          {
          if ( qtetm > qtetM )
            {
            qtetm = q;
            }
          qtetM = q;
          }
        else if ( q < qtetm )
          {
          qtetm = q;
          }
        Eqtet += q;
        Eqtet2 += q * q;
        ntet++;
        if ( this->Volume )
          {
          V = TetVolume( cell );
          if ( ! this->CompatibilityMode )
            {
            volume->SetTuple1( 0, V );
            }
          }
        break;
      case VTK_HEXAHEDRON:
        q = HexQuality( cell );
        if ( q > qhexM )
          {
          if ( qhexm > qhexM )
            {
            qhexm = q;
            }
          qhexM = q;
          }
        else if ( q < qhexm )
          {
          qhexm = q;
          }
        Eqhex += q;
        Eqhex2 += q * q;
        nhex++;
        break;
      default:
        q = 0.;
        }

      if ( this->SaveCellQuality )
        {
        if ( this->CompatibilityMode && this->Volume )
          {
          quality->SetTuple2( c, V, q );
          }
        else
          {
          quality->SetTuple1( c, q );
          }
        }
      }
    this->UpdateProgress( double(p+1+progressNumer)/progressDenom );
    }

  if ( ntri )
    {
    Eqtri  /= (double) ntri;
    Eqtri2 /= (double) ntri;
    }
  else
    {
    qtrim = Eqtri = qtriM = Eqtri2 = 0.;
    }
  
  if ( nqua )
    {
    Eqqua  /= (double) nqua;
    Eqqua2 /= (double) nqua;
    }
  else
    {
    qquam = Eqqua = qquaM = Eqqua2 = 0.;
    }
  
  if ( ntet )
    {
    Eqtet  /= (double) ntet;
    Eqtet2 /= (double) ntet;
    }
  else
    {
    qtetm = Eqtet = qtetM = Eqtet2 = 0.;
    }

  if ( nhex )
    {
    Eqhex  /= (double) nhex;
    Eqhex2 /= (double) nhex;
    }
  else
    {
    qhexm = Eqhex = qhexM = Eqhex2 = 0.;
    }

  double tuple[5];
  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Triangle Quality" );
  quality->SetNumberOfComponents(5);
  tuple[0] = qtrim;
  tuple[1] = Eqtri;
  tuple[2] = qtriM;
  tuple[3] = Eqtri2;
  tuple[4] = ntri;
  quality->InsertNextTuple( tuple );
  out->GetFieldData()->AddArray( quality );
  quality->Delete();

  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Quadrilateral Quality" );
  quality->SetNumberOfComponents(5);
  tuple[0] = qquam;
  tuple[1] = Eqqua;
  tuple[2] = qquaM;
  tuple[3] = Eqqua2;
  tuple[4] = nqua;
  quality->InsertNextTuple( tuple );
  out->GetFieldData()->AddArray( quality );
  quality->Delete();

  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Tetrahedron Quality" );
  quality->SetNumberOfComponents(5);
  tuple[0] = qtetm;
  tuple[1] = Eqtet;
  tuple[2] = qtetM;
  tuple[3] = Eqtet2;
  tuple[4] = ntet;
  quality->InsertNextTuple( tuple );
  out->GetFieldData()->AddArray( quality );
  quality->Delete();

  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Hexahedron Quality" );
  quality->SetNumberOfComponents(5);
  tuple[0] = qhexm;
  tuple[1] = Eqhex;
  tuple[2] = qhexM;
  tuple[3] = Eqhex2;
  tuple[4] = nhex;
  quality->InsertNextTuple( tuple );
  out->GetFieldData()->AddArray( quality );
  quality->Delete();

  return 1;
}

int vtkMeshQuality::GetCurrentTriangleNormal( double point[3], double normal[3] )
{
  // ignore the location where the normal should be evaluated.
  (void) point;

  // copy the cell normal
  for ( int i = 0; i < 3; ++i )
    normal[i] = vtkMeshQuality::CurrentTriNormal[i];
  return 1;
}

// Triangle quality measures:
// edge ratio, aspect ratio, radius ratio, Frobenius norm, minimal angle

double vtkMeshQuality::TriangleEdgeRatio( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_edge_ratio( 3, pc );
}

double vtkMeshQuality::TriangleAspectRatio( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_aspect_ratio( 3, pc );
}

double vtkMeshQuality::TriangleRadiusRatio( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_radius_ratio( 3, pc );
}

double vtkMeshQuality::TriangleAspectFrobenius( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_aspect_frobenius( 3, pc );
}

double vtkMeshQuality::TriangleMinAngle( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_minimum_angle( 3, pc );
}

double vtkMeshQuality::TriangleMaxAngle( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_maximum_angle( 3, pc );
}

double vtkMeshQuality::TriangleCondition( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_condition( 3, pc );
}

double vtkMeshQuality::TriangleScaledJacobian( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_scaled_jacobian( 3, pc );
}

double vtkMeshQuality::TriangleRelativeSizeSquared( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_relative_size_squared( 3, pc );
}

double vtkMeshQuality::TriangleShape( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_shape( 3, pc );
}

double vtkMeshQuality::TriangleShapeAndSize( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_shape_and_size( 3, pc );
}

double vtkMeshQuality::TriangleDistortion( vtkCell* cell )
{
  double pc[3][3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, pc[0]);
  p->GetPoint(1, pc[1]);
  p->GetPoint(2, pc[2]);

  return v_tri_distortion( 3, pc );
}

// Quadrangle quality measures:
// edge ratio, aspect ratio, radius ratio, average Frobenius norm, 
// maximum Frobenius norm, minimal angle
// (only edge ratio and minimal angle are intended for nonplanar quads)

double vtkMeshQuality::QuadEdgeRatio( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_edge_ratio( 4, pc );
}

double vtkMeshQuality::QuadAspectRatio( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_aspect_ratio( 4, pc );
}

double vtkMeshQuality::QuadRadiusRatio( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_radius_ratio( 4, pc );
}

double vtkMeshQuality::QuadMedAspectFrobenius( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_med_aspect_frobenius( 4, pc );
}

double vtkMeshQuality::QuadMaxAspectFrobenius( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_max_aspect_frobenius( 4, pc );
}

double vtkMeshQuality::QuadMinAngle( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_minimum_angle( 4, pc );
}

double vtkMeshQuality::QuadMaxEdgeRatios( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_max_edge_ratios( 4, pc );
}

double vtkMeshQuality::QuadSkew( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_skew( 4, pc );
}

double vtkMeshQuality::QuadTaper( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_taper( 4, pc );
}

double vtkMeshQuality::QuadWarpage( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_warpage( 4, pc );
}

double vtkMeshQuality::QuadArea( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_area( 4, pc );
}

double vtkMeshQuality::QuadStretch( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_stretch( 4, pc );
}

#if 0
// FIXME
double vtkMeshQuality::QuadMinAngle( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_minimum_angle( 4, pc );
}
#endif // 0

double vtkMeshQuality::QuadMaxAngle( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_maximum_angle( 4, pc );
}

double vtkMeshQuality::QuadOddy( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_oddy( 4, pc );
}

double vtkMeshQuality::QuadCondition( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_condition( 4, pc );
}

double vtkMeshQuality::QuadJacobian( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_jacobian( 4, pc );
}

double vtkMeshQuality::QuadScaledJacobian( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_scaled_jacobian( 4, pc );
}

double vtkMeshQuality::QuadShear( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_shear( 4, pc );
}

double vtkMeshQuality::QuadShape( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_shape( 4, pc );
}

double vtkMeshQuality::QuadRelativeSizeSquared( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_relative_size_squared( 4, pc );
}

double vtkMeshQuality::QuadShapeAndSize( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_shape_and_size( 4, pc );
}

double vtkMeshQuality::QuadShearAndSize( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_shear_and_size( 4, pc );
}

double vtkMeshQuality::QuadDistortion( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_quad_distortion( 4, pc );
}


// Volume of a tetrahedron, for compatibility with the original vtkMeshQuality

double TetVolume( vtkCell* cell )
{
  double x0[3];
  double x1[3];
  double x2[3];
  double x3[3];
  cell->Points->GetPoint( 0, x0 );
  cell->Points->GetPoint( 1, x1 );
  cell->Points->GetPoint( 2, x2 );
  cell->Points->GetPoint( 3, x3 );
  return vtkTetra::ComputeVolume( x0, x1, x2, x3 );
}

// Tetrahedron quality measures:
// edge ratio, aspect ratio, radius ratio, Frobenius norm, minimal angle

double vtkMeshQuality::TetEdgeRatio( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_edge_ratio( 4, pc );
}

double vtkMeshQuality::TetAspectRatio( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_aspect_ratio( 4, pc );
}

double vtkMeshQuality::TetRadiusRatio( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_radius_ratio( 4, pc );
}

double vtkMeshQuality::TetAspectBeta( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_aspect_beta( 4, pc );
}

double vtkMeshQuality::TetAspectFrobenius( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_aspect_frobenius( 4, pc );
}

double vtkMeshQuality::TetMinAngle( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_minimum_angle( 4, pc );
}

double vtkMeshQuality::TetCollapseRatio( vtkCell* cell )
{
  double p0[3], p1[3], p2[3], p3[3];
  double e01[3], e02[3], e03[3], e12[3], e13[3], e23[3];
  double l[6];
  double N[3];
  double h, magN;
  double cr;
  double crMin;
  int i;

  vtkPoints *p = cell->GetPoints();
  p->GetPoint( 0, p0 );
  p->GetPoint( 1, p1 );
  p->GetPoint( 2, p2 );
  p->GetPoint( 3, p3 );

  // Compute edge vectors and their lengths
  l[0] = l[1] = l[2] = l[3] = l[4] = l[5] = 0.;
  for ( i = 0; i < 3; ++i )
    {
    e01[i] = p1[i] - p0[i];  l[0] += e01[i] * e01[i];
    e02[i] = p2[i] - p0[i];  l[1] += e02[i] * e02[i];
    e03[i] = p3[i] - p0[i];  l[2] += e03[i] * e03[i];

    e12[i] = p2[i] - p1[i];  l[3] += e12[i] * e12[i];
    e13[i] = p3[i] - p1[i];  l[4] += e13[i] * e13[i];

    e23[i] = p3[i] - p2[i];  l[5] += e23[i] * e23[i];
    }
  for ( i = 0; i < 6; ++i )
    {
    l[i] = sqrt( l[i] );
    }

  // Find longest edge for each bounding triangle of tetrahedron
  double l012 = l[4] > l[0] ? l[4] : l[0]; l012 = l[1] > l012 ? l[1] : l012;
  double l031 = l[0] > l[2] ? l[0] : l[2]; l031 = l[3] > l031 ? l[3] : l031;
  double l023 = l[2] > l[1] ? l[2] : l[1]; l023 = l[5] > l023 ? l[5] : l023;
  double l132 = l[4] > l[3] ? l[4] : l[3]; l132 = l[5] > l132 ? l[5] : l132;

  // Compute collapse ratio for each vertex/triangle pair
  vtkMath::Cross(e01, e02, N);
  magN = vtkMath::Dot(N, N);
  h = vtkMath::Dot(e03, N) / sqrt( magN ); // height of vertex 3 above 0-1-2
  crMin = h / l012;                        // ratio of height to longest edge of 0-1-2

  vtkMath::Cross(e03, e01, N);
  magN = vtkMath::Dot(N, N);
  h = vtkMath::Dot(e02, N) / sqrt( magN ); // height of vertex 2 above 0-3-1
  cr = h / l031;                           // ratio of height to longest edge of 0-3-1
  if ( cr < crMin ) crMin = cr;

  vtkMath::Cross(e02, e03, N);
  magN = vtkMath::Dot(N, N);
  h = vtkMath::Dot(e01, N) / sqrt( magN ); // height of vertex 1 above 0-2-3
  cr = h / l023;                           // ratio of height to longest edge of 0-2-3
  if ( cr < crMin ) crMin = cr;

  vtkMath::Cross(e12, e13, N);
  magN = vtkMath::Dot(N, N);
  h = vtkMath::Dot(e01, N) / sqrt( magN ); // height of vertex 0 above 1-3-2
  cr = h / l132;                           // ratio of height to longest edge of 1-3-2
  if ( cr < crMin ) crMin = cr;

  return crMin;
}

double vtkMeshQuality::TetAspectGamma( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints *p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_aspect_gamma( 4, pc );
}

double vtkMeshQuality::TetVolume( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_volume( 4, pc );
}

double vtkMeshQuality::TetCondition( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_condition( 4, pc );
}

double vtkMeshQuality::TetJacobian( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_jacobian( 4, pc );
}

double vtkMeshQuality::TetScaledJacobian( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_scaled_jacobian( 4, pc );
}

double vtkMeshQuality::TetShape( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_shape( 4, pc );
}

double vtkMeshQuality::TetRelativeSizeSquared( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_relative_size_squared( 4, pc );
}

double vtkMeshQuality::TetShapeandSize( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_shape_and_size( 4, pc );
}

double vtkMeshQuality::TetDistortion( vtkCell* cell )
{
  double pc[4][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 4; ++i )
    p->GetPoint( i, pc[i] );

  return v_tet_distortion( 4, pc );
}


// Hexahedron quality measure:
// edge ratio, average Frobenius norm, maximum Frobenius norm

double vtkMeshQuality::HexEdgeRatio( vtkCell* cell)
{
  double p0[3],p1[3],p2[3],p3[3];
  double p4[3],p5[3],p6[3],p7[3];

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);
  p->GetPoint(4, p4);
  p->GetPoint(5, p5);
  p->GetPoint(6, p6);
  p->GetPoint(7, p7);

  double a[3],b[3],c[3],d[3],e[3],f[3];
  double g[3],h[3],i[3],j[3],k[3],l[3];

  a[0] = p1[0] - p0[0];
  a[1] = p1[1] - p0[1];
  a[2] = p1[2] - p0[2];
 
  b[0] = p2[0] - p1[0];
  b[1] = p2[1] - p1[1];
  b[2] = p2[2] - p1[2];
 
  c[0] = p3[0] - p2[0];
  c[1] = p3[1] - p2[1];
  c[2] = p3[2] - p2[2];
 
  d[0] = p0[0] - p3[0];
  d[1] = p0[1] - p3[1];
  d[2] = p0[2] - p3[2];
 
  e[0] = p4[0] - p0[0];
  e[1] = p4[1] - p0[1];
  e[2] = p4[2] - p0[2];
 
  f[0] = p5[0] - p1[0];
  f[1] = p5[1] - p1[1];
  f[2] = p5[2] - p1[2];

  g[0] = p6[0] - p2[0];
  g[1] = p6[1] - p2[1];
  g[2] = p6[2] - p2[2];

  h[0] = p7[0] - p3[0];
  h[1] = p7[1] - p3[1];
  h[2] = p7[2] - p3[2];

  i[0] = p5[0] - p4[0];
  i[1] = p5[1] - p4[1];
  i[2] = p5[2] - p4[2];
 
  j[0] = p6[0] - p5[0];
  j[1] = p6[1] - p5[1];
  j[2] = p6[2] - p5[2];

  k[0] = p7[0] - p6[0];
  k[1] = p7[1] - p6[1];
  k[2] = p7[2] - p6[2];

  l[0] = p4[0] - p7[0];
  l[1] = p4[1] - p7[1];
  l[2] = p4[2] - p7[2];

  double a2 = vtkMath::Dot(a,a);
  double b2 = vtkMath::Dot(b,b);
  double c2 = vtkMath::Dot(c,c);
  double d2 = vtkMath::Dot(d,d);
  double e2 = vtkMath::Dot(e,e);
  double f2 = vtkMath::Dot(f,f);
  double g2 = vtkMath::Dot(g,g);
  double h2 = vtkMath::Dot(h,h);
  double i2 = vtkMath::Dot(i,i);
  double j2 = vtkMath::Dot(j,j);
  double k2 = vtkMath::Dot(k,k);
  double l2 = vtkMath::Dot(l,l);

  double mab,mcd,mef,Mab,Mcd,Mef;
  double mgh,mij,mkl,Mgh,Mij,Mkl;

  if ( a2 < b2 )
    {
      mab = a2;
      Mab = b2;
    }
  else // b2 <= a2
    {
      mab = b2;
      Mab = a2;
    }
  if ( c2 < d2 )
    {
      mcd = c2;
      Mcd = d2;
    }
  else // d2 <= c2
    {
      mcd = d2;
      Mcd = c2;
    }
  if ( e2 < f2 )
    {
      mef = e2;
      Mef = f2;
    }
  else // f2 <= e2
    {
      mef = f2;
      Mef = e2;
    }
  if ( g2 < h2 )
    {
      mgh = g2;
      Mgh = h2;
    }
  else // h2 <= g2
    {
      mgh = h2;
      Mgh = g2;
    }
  if ( i2 < j2 )
    {
      mij = i2;
      Mij = j2;
    }
  else // j2 <= i2
    {
      mij = j2;
      Mij = i2;
    }
  if ( k2 < l2 )
    {
      mkl = k2;
      Mkl = l2;
    }
  else // l2 <= k2
    {
      mkl = l2;
      Mkl = k2;
    }

  double m2,M2;
 
  m2 = mab < mcd ? mab : mcd;
  m2 = m2  < mef ? m2  : mef;
  m2 = m2  < mgh ? m2  : mgh;
  m2 = m2  < mij ? m2  : mij;
  m2 = m2  < mkl ? m2  : mkl;
  M2 = Mab > Mcd ? Mab : Mcd;
  M2 = M2  > Mef ? M2  : Mef;
  M2 = M2  > Mgh ? M2  : Mgh;
  M2 = M2  > Mij ? M2  : Mij;
  M2 = M2  > Mkl ? M2  : Mkl;

  return sqrt( M2 / m2 );
}

double cubicCornerAspectFrobenius( double* u, double* v, double* w )
{
  const double normal_coeff = 1. / 9.;
  
  double A[3][3];
  int i;
  for ( i = 0; i < 3; ++ i )
    {
    A[0][i] = u[i];
    A[1][i] = v[i];
    A[2][i] = w[i];
    }

  double U[3][3];
  double VT[3][3];
  double s[3];
  vtkMath::SingularValueDecomposition3x3( A, U, s, VT );

  double a2 = s[0] * s[0];
  double b2 = s[1] * s[1];
  double c2 = s[2] * s[2];

  return sqrt( ( a2 + b2 + c2 ) * ( 1. / a2 + 1. / b2 + 1. / c2 ) * normal_coeff );
}

double vtkMeshQuality::HexMedAspectFrobenius( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double p4[3],p5[3],p6[3],p7[3];
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);
  p->GetPoint(4, p4);
  p->GetPoint(5, p5);
  p->GetPoint(6, p6);
  p->GetPoint(7, p7);

  double e01[3],e12[3],e23[3],e30[3],e04[3],e15[3];
  double e26[3],e37[3],e45[3],e56[3],e67[3],e74[3];

  e01[0] = p1[0] - p0[0];
  e01[1] = p1[1] - p0[1];
  e01[2] = p1[2] - p0[2];
 
  e12[0] = p2[0] - p1[0];
  e12[1] = p2[1] - p1[1];
  e12[2] = p2[2] - p1[2];
 
  e23[0] = p3[0] - p2[0];
  e23[1] = p3[1] - p2[1];
  e23[2] = p3[2] - p2[2];
 
  e30[0] = p0[0] - p3[0];
  e30[1] = p0[1] - p3[1];
  e30[2] = p0[2] - p3[2];
 
  e04[0] = p4[0] - p0[0];
  e04[1] = p4[1] - p0[1];
  e04[2] = p4[2] - p0[2];
 
  e15[0] = p5[0] - p1[0];
  e15[1] = p5[1] - p1[1];
  e15[2] = p5[2] - p1[2];

  e26[0] = p6[0] - p2[0];
  e26[1] = p6[1] - p2[1];
  e26[2] = p6[2] - p2[2];

  e37[0] = p7[0] - p3[0];
  e37[1] = p7[1] - p3[1];
  e37[2] = p7[2] - p3[2];

  e45[0] = p5[0] - p4[0];
  e45[1] = p5[1] - p4[1];
  e45[2] = p5[2] - p4[2];
 
  e56[0] = p6[0] - p5[0];
  e56[1] = p6[1] - p5[1];
  e56[2] = p6[2] - p5[2];

  e67[0] = p7[0] - p6[0];
  e67[1] = p7[1] - p6[1];
  e67[2] = p7[2] - p6[2];

  e74[0] = p4[0] - p7[0];
  e74[1] = p4[1] - p7[1];
  e74[2] = p4[2] - p7[2];

  double qsum = cubicCornerAspectFrobenius( e01, e04, e30 ); 

  qsum += cubicCornerAspectFrobenius( e01, e12, e15 ); 
  qsum += cubicCornerAspectFrobenius( e12, e23, e26 ); 
  qsum += cubicCornerAspectFrobenius( e23, e30, e37 ); 
  qsum += cubicCornerAspectFrobenius( e74, e45, e04 ); 
  qsum += cubicCornerAspectFrobenius( e45, e56, e15 ); 
  qsum += cubicCornerAspectFrobenius( e56, e67, e26 ); 
  qsum += cubicCornerAspectFrobenius( e67, e74, e37 ); 

  return .125 * qsum;
}

double vtkMeshQuality::HexMaxAspectFrobenius( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double p4[3],p5[3],p6[3],p7[3];
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);
  p->GetPoint(4, p4);
  p->GetPoint(5, p5);
  p->GetPoint(6, p6);
  p->GetPoint(7, p7);

  double e01[3],e12[3],e23[3],e30[3],e04[3],e15[3];
  double e26[3],e37[3],e45[3],e56[3],e67[3],e74[3];

  e01[0] = p1[0] - p0[0];
  e01[1] = p1[1] - p0[1];
  e01[2] = p1[2] - p0[2];
 
  e12[0] = p2[0] - p1[0];
  e12[1] = p2[1] - p1[1];
  e12[2] = p2[2] - p1[2];
 
  e23[0] = p3[0] - p2[0];
  e23[1] = p3[1] - p2[1];
  e23[2] = p3[2] - p2[2];
 
  e30[0] = p0[0] - p3[0];
  e30[1] = p0[1] - p3[1];
  e30[2] = p0[2] - p3[2];
 
  e04[0] = p4[0] - p0[0];
  e04[1] = p4[1] - p0[1];
  e04[2] = p4[2] - p0[2];
 
  e15[0] = p5[0] - p1[0];
  e15[1] = p5[1] - p1[1];
  e15[2] = p5[2] - p1[2];

  e26[0] = p6[0] - p2[0];
  e26[1] = p6[1] - p2[1];
  e26[2] = p6[2] - p2[2];

  e37[0] = p7[0] - p3[0];
  e37[1] = p7[1] - p3[1];
  e37[2] = p7[2] - p3[2];

  e45[0] = p5[0] - p4[0];
  e45[1] = p5[1] - p4[1];
  e45[2] = p5[2] - p4[2];
 
  e56[0] = p6[0] - p5[0];
  e56[1] = p6[1] - p5[1];
  e56[2] = p6[2] - p5[2];

  e67[0] = p7[0] - p6[0];
  e67[1] = p7[1] - p6[1];
  e67[2] = p7[2] - p6[2];

  e74[0] = p4[0] - p7[0];
  e74[1] = p4[1] - p7[1];
  e74[2] = p4[2] - p7[2];

  double qmax = cubicCornerAspectFrobenius( e01, e04, e30 ); 

  double qcur = cubicCornerAspectFrobenius( e01, e12, e15 ); 
  if ( qcur > qmax ) qmax = qcur;

  qcur = cubicCornerAspectFrobenius( e12, e23, e26 ); 
  if ( qcur > qmax ) qmax = qcur;

  qcur = cubicCornerAspectFrobenius( e23, e30, e37 ); 
  if ( qcur > qmax ) qmax = qcur;

  qcur = cubicCornerAspectFrobenius( e74, e45, e04 ); 
  if ( qcur > qmax ) qmax = qcur;

  qcur = cubicCornerAspectFrobenius( e45, e56, e15 ); 
  if ( qcur > qmax ) qmax = qcur;

  qcur = cubicCornerAspectFrobenius( e56, e67, e26 ); 
  if ( qcur > qmax ) qmax = qcur;

  qcur = cubicCornerAspectFrobenius( e67, e74, e37 ); 
  if ( qcur > qmax ) qmax = qcur;

  return qmax;
}

double vtkMeshQuality::HexMaxEdgeRatios( vtkCell* cell )
{
  double pc[8][3];

  vtkPoints* p = cell->GetPoints();
  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_max_edge_ratios( 8, pc );
}

double vtkMeshQuality::HexSkew( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_skew( 8, pc );
}

double vtkMeshQuality::HexTaper( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_taper( 8, pc );
}

double vtkMeshQuality::HexVolume( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_volume( 8, pc );
}

double vtkMeshQuality::HexStretch( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_stretch( 8, pc );
}

double vtkMeshQuality::HexDiagonal( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_diagonal( 8, pc );
}

double vtkMeshQuality::HexDimension( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_dimension( 8, pc );
}

double vtkMeshQuality::HexOddy( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_oddy( 8, pc );
}

double vtkMeshQuality::HexCondition( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_condition( 8, pc );
}

double vtkMeshQuality::HexJacobian( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_jacobian( 8, pc );
}

double vtkMeshQuality::HexScaledJacobian( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_scaled_jacobian( 8, pc );
}

double vtkMeshQuality::HexShear( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_shear( 8, pc );
}

double vtkMeshQuality::HexShape( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_shape( 8, pc );
}

double vtkMeshQuality::HexRelativeSizeSquared( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_relative_size_squared( 8, pc );
}

double vtkMeshQuality::HexShapeAndSize( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_shape_and_size( 8, pc );
}

double vtkMeshQuality::HexShearAndSize( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_shear_and_size( 8, pc );
}

double vtkMeshQuality::HexDistortion( vtkCell* cell )
{
  double pc[8][3];
  vtkPoints* p = cell->GetPoints();

  for ( int i = 0; i < 8; ++i )
    p->GetPoint( i, pc[i] );

  return v_hex_distortion( 8, pc );
}
