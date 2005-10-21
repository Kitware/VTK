/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeshQuality.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

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

vtkCxxRevisionMacro(vtkMeshQuality,"1.32");
vtkStandardNewMacro(vtkMeshQuality);

typedef double (*CellQualityType)( vtkCell* );

double TetVolume( vtkCell* cell );

const char* QualityMeasureNames[] =
{
  "EdgeRatio",
  "AspectRatio",
  "RadiusRatio",
  "FrobeniusNorm",
  "MedFrobeniusNorm",
  "MaxFrobeniusNorm",
  "MinAngle"
};

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
  this->HexQualityMeasure = VTK_QUALITY_EDGE_RATIO;
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
    case VTK_QUALITY_FROBENIUS_NORM:
      TriangleQuality = TriangleFrobeniusNorm;
      break;
    case VTK_QUALITY_MIN_ANGLE:
      TriangleQuality = TriangleMinAngle;
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
    case VTK_QUALITY_MED_FROBENIUS_NORM:
      QuadQuality = QuadMedFrobeniusNorm;
      break;
    case VTK_QUALITY_MAX_FROBENIUS_NORM:
      QuadQuality = QuadMaxFrobeniusNorm;
      break;
    case VTK_QUALITY_MIN_ANGLE:
      QuadQuality = QuadMinAngle;
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
    case VTK_QUALITY_FROBENIUS_NORM:
      TetQuality = TetFrobeniusNorm;
      break;
    case VTK_QUALITY_MIN_ANGLE:
      TetQuality = TetMinAngle;
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
    default:
      vtkWarningMacro( "Bad HexQualityMeasure ("
        << this->GetTetQualityMeasure() << "), using EdgeRatio instead");
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

  int p;
  vtkIdType c = 0;
  vtkIdType sz = N / 20 + 1;
  vtkIdType inner;
  this->UpdateProgress( 0.01 );
  for ( p = 0; p < 20; ++p )
    {
    for ( inner = 0; (inner < sz && c < N); ++c, ++inner )
      {
      cell = out->GetCell( c );
      V = 0.;
      switch ( cell->GetCellType() )
        {
      case VTK_TRIANGLE:
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
    this->UpdateProgress( double(p+1)/20. );
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

// Triangle quality measures:
// edge ratio, aspect ratio, radius ratio, Frobenius norm, minimal angle

double vtkMeshQuality::TriangleEdgeRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double a2,b2,c2,m2,M2;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p2[0]-p0[0];
  c[1] = p2[1]-p0[1];
  c[2] = p2[2]-p0[2];
 
  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);

  if ( a2 < b2 )
    {
    if ( b2 < c2 )
      {
        m2 = a2;
        M2 = c2;
      }
    else // b2 <= a2
      {
      if ( a2 < c2 )
        {
        m2 = a2;
        M2 = b2;
        }
      else // c2 <= a2
        {
          m2 = c2;
          M2 = b2;
        }
      }
    }
  else // b2 <= a2
    {
    if ( a2 < c2 )
      {
        m2 = b2;
        M2 = c2;
      }
    else // c2 <= a2
      {
      if ( b2 < c2 )
        {
          m2 = b2;
          M2 = a2;
        }
      else // c2 <= b2
        {
          m2 = c2;
          M2 = a2;
        }
      }
    }

  return sqrt(M2 / m2);
}

double vtkMeshQuality::TriangleAspectRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double a1,b1,c1,hm;
  const double normal_coeff = sqrt(3.) / 6.;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p2[0]-p0[0];
  c[1] = p2[1]-p0[1];
  c[2] = p2[2]-p0[2];
 
  a1 = sqrt(vtkMath::Dot(a,a));
  b1 = sqrt(vtkMath::Dot(b,b));
  c1 = sqrt(vtkMath::Dot(c,c));

  hm = a1 > b1 ? a1 : b1;
  hm = hm > c1 ? hm : c1;

  vtkMath::Cross(a,b,c);

  return normal_coeff * hm * (a1 + b1 + c1) / vtkMath::Norm(c);
}

double vtkMeshQuality::TriangleRadiusRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double a1,b1,c1,ab;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p2[0]-p0[0];
  c[1] = p2[1]-p0[1];
  c[2] = p2[2]-p0[2];
 
  a1 = sqrt(vtkMath::Dot(a,a));
  b1 = sqrt(vtkMath::Dot(b,b));
  c1 = sqrt(vtkMath::Dot(c,c));

  vtkMath::Cross(a,b,c);
  ab = vtkMath::Norm(c);

  return .25 * a1 * b1 * c1 * (a1 + b1 + c1) / (ab * ab);
}

double vtkMeshQuality::TriangleFrobeniusNorm( vtkCell* cell )
{
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double t22;
  const double normal_coeff = .5 / sqrt(3.);
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p2[0]-p0[0];
  c[1] = p2[1]-p0[1];
  c[2] = p2[2]-p0[2];
 
  t22  = vtkMath::Dot(a,a);
  t22 += vtkMath::Dot(b,b);
  t22 += vtkMath::Dot(c,c);

  vtkMath::Cross(a,b,c);

  return normal_coeff * t22 / vtkMath::Norm(c);
}

double vtkMeshQuality::TriangleMinAngle( vtkCell* cell )
{
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double a2,b2,c2,alpha,beta,gamma;
  const double normal_coeff = .3183098861837906715377675267450287;

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p2[0]-p0[0];
  c[1] = p2[1]-p0[1];
  c[2] = p2[2]-p0[2];
 
  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);

  alpha = acos(vtkMath::Dot(b,c) / sqrt(b2 * c2));
  beta  = acos(vtkMath::Dot(c,a) / sqrt(c2 * a2));
  gamma = acos(vtkMath::Dot(a,b) / sqrt(a2 * b2));

  alpha = alpha < beta ? alpha : beta;

  return  (alpha < gamma ? alpha : gamma) * 180. * normal_coeff;
}

// Quadrangle quality measures:
// edge ratio, aspect ratio, radius ratio, average Frobenius norm, 
// maximal Frobenius norm, minimal angle
// (only edge ratio and minimal angle are intended for nonplanar quads)

double vtkMeshQuality::QuadEdgeRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3];
  double a2,b2,c2,d2,mab,Mab,mcd,Mcd,m2,M2;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p3[0]-p2[0];
  c[1] = p3[1]-p2[1];
  c[2] = p3[2]-p2[2];
 
  d[0] = p0[0]-p3[0];
  d[1] = p0[1]-p3[1];
  d[2] = p0[2]-p3[2];
 
  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);
  d2 = vtkMath::Dot(d,d);

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
  m2 = mab < mcd ? mab : mcd;
  M2 = Mab > Mcd ? Mab : Mcd;

  return sqrt(M2 / m2);
}

double vtkMeshQuality::QuadAspectRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3],ab[3],cd[3];
  double a1,b1,c1,d1;
  double ma,mb,hm;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p3[0]-p2[0];
  c[1] = p3[1]-p2[1];
  c[2] = p3[2]-p2[2];
 
  d[0] = p0[0]-p3[0];
  d[1] = p0[1]-p3[1];
  d[2] = p0[2]-p3[2];
 
  a1 = sqrt(vtkMath::Dot(a,a));
  b1 = sqrt(vtkMath::Dot(b,b));
  c1 = sqrt(vtkMath::Dot(c,c));
  d1 = sqrt(vtkMath::Dot(d,d));

  ma = a1 > b1 ? a1 : b1;
  mb = c1 > d1 ? c1 : d1;
  hm = ma > mb ? ma : mb;

  vtkMath::Cross(a,b,ab);
  vtkMath::Cross(c,d,cd);

  return .5 * hm * (a1 + b1 + c1 + d1) / (vtkMath::Norm(ab) + vtkMath::Norm(cd));
}

double vtkMeshQuality::QuadRadiusRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3],m[3],n[3];
  double ab[3],bc[3],cd[3],da[3];
  double a2,b2,c2,d2,m2,n2,h2;
  double t0,t1,t2,t3;
  const double normal_coeff = 1. / (2.*sqrt(2.));
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p3[0]-p2[0];
  c[1] = p3[1]-p2[1];
  c[2] = p3[2]-p2[2];
 
  d[0] = p0[0]-p3[0];
  d[1] = p0[1]-p3[1];
  d[2] = p0[2]-p3[2];
 
  m[0] = p2[0]-p0[0];
  m[1] = p2[1]-p0[1];
  m[2] = p2[2]-p0[2];
 
  n[0] = p3[0]-p1[0];
  n[1] = p3[1]-p1[1];
  n[2] = p3[2]-p1[2];
 
  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);
  d2 = vtkMath::Dot(d,d);
  m2 = vtkMath::Dot(m,m);
  n2 = vtkMath::Dot(n,n);

  t0 = a2 > b2 ? a2 : b2;
  t1 = c2 > d2 ? c2 : d2;
  t2 = m2 > n2 ? m2 : n2;
  h2 = t0 > t1 ? t0 : t1;
  h2 = h2 > t2 ? h2 : t2;

  vtkMath::Cross(a,b,ab);
  vtkMath::Cross(b,c,bc);
  vtkMath::Cross(c,d,cd);
  vtkMath::Cross(d,a,da);

  t0 = vtkMath::Norm(da);
  t1 = vtkMath::Norm(ab);
  t2 = vtkMath::Norm(bc);
  t3 = vtkMath::Norm(cd);

  t0 = t0 < t1 ? t0 : t1;
  t2 = t2 < t3 ? t2 : t3;
  t0 = t0 < t2 ? t0 : t2;

  return normal_coeff * sqrt((a2 + b2 + c2 + d2) * h2) / t0;
}

double vtkMeshQuality::QuadMedFrobeniusNorm( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3],ab[3],bc[3],cd[3],da[3];
  double a2,b2,c2,d2;
  double kappa2;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p3[0]-p2[0];
  c[1] = p3[1]-p2[1];
  c[2] = p3[2]-p2[2];
 
  d[0] = p0[0]-p3[0];
  d[1] = p0[1]-p3[1];
  d[2] = p0[2]-p3[2];
 
  vtkMath::Cross(a,b,ab);
  vtkMath::Cross(b,c,bc);
  vtkMath::Cross(c,d,cd);
  vtkMath::Cross(d,a,da);

  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);
  d2 = vtkMath::Dot(d,d);

  kappa2  = (a2 + b2) / vtkMath::Norm(ab);
  kappa2 += (b2 + c2) / vtkMath::Norm(bc);
  kappa2 += (c2 + d2) / vtkMath::Norm(cd);
  kappa2 += (d2 + a2) / vtkMath::Norm(da);

  return .125 * kappa2;
}

double vtkMeshQuality::QuadMaxFrobeniusNorm( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3],ab[3],bc[3],cd[3],da[3];
  double a2,b2,c2,d2;
  double kmax,kcur;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p3[0]-p2[0];
  c[1] = p3[1]-p2[1];
  c[2] = p3[2]-p2[2];
 
  d[0] = p0[0]-p3[0];
  d[1] = p0[1]-p3[1];
  d[2] = p0[2]-p3[2];
 
  vtkMath::Cross(a,b,ab);
  vtkMath::Cross(b,c,bc);
  vtkMath::Cross(c,d,cd);
  vtkMath::Cross(d,a,da);

  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);
  d2 = vtkMath::Dot(d,d);

  kmax = (a2 + b2) / vtkMath::Norm(ab);

  kcur = (b2 + c2) / vtkMath::Norm(bc);
  kmax = kmax > kcur ? kmax : kcur;

  kcur = (c2 + d2) / vtkMath::Norm(cd);
  kmax = kmax > kcur ? kmax : kcur;

  kcur = (d2 + a2) / vtkMath::Norm(da);
  kmax = kmax > kcur ? kmax : kcur;

  return .5 * kmax;
}

double vtkMeshQuality::QuadMinAngle( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3];
  double a2,b2,c2,d2,alpha,beta,gamma,delta;
  const double normal_coeff = .3183098861837906715377675267450287;

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p3[0]-p2[0];
  c[1] = p3[1]-p2[1];
  c[2] = p3[2]-p2[2];
 
  d[0] = p0[0]-p3[0];
  d[1] = p0[1]-p3[1];
  d[2] = p0[2]-p3[2];
 
  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);
  d2 = vtkMath::Dot(d,d);

  alpha = acos(vtkMath::Dot(b,c) / sqrt(b2 * c2));
  beta  = acos(vtkMath::Dot(c,d) / sqrt(c2 * d2));
  gamma = acos(vtkMath::Dot(d,a) / sqrt(d2 * a2));
  delta = acos(vtkMath::Dot(a,b) / sqrt(a2 * b2));

  alpha = alpha < beta  ? alpha : beta;
  gamma = gamma < delta ? gamma : delta;

  return  (alpha < gamma ? alpha : gamma) * 180. * normal_coeff;
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
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3],e[3],f[3];
  double a2,b2,c2,d2,e2,f2;
  double m2,M2,mab,mcd,mef,Mab,Mcd,Mef;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p2[0]-p0[0];
  c[1] = p2[1]-p0[1];
  c[2] = p2[2]-p0[2];
 
  d[0] = p3[0]-p0[0];
  d[1] = p3[1]-p0[1];
  d[2] = p3[2]-p0[2];
 
  e[0] = p3[0]-p1[0];
  e[1] = p3[1]-p1[1];
  e[2] = p3[2]-p1[2];
 
  f[0] = p3[0]-p2[0];
  f[1] = p3[1]-p2[1];
  f[2] = p3[2]-p2[2];

  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);
  d2 = vtkMath::Dot(d,d);
  e2 = vtkMath::Dot(e,e);
  f2 = vtkMath::Dot(f,f);

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

  m2 = mab < mcd ? mab : mcd;
  m2 = m2  < mef ? m2  : mef;
  M2 = Mab > Mcd ? Mab : Mcd;
  M2 = M2  > Mef ? M2  : Mef;

  return sqrt(M2 / m2);
}

double vtkMeshQuality::TetAspectRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double ab[3],bc[3],ac[3],ad[3],bd[3],cd[3];
  double t0,t1,t2,t3,t4,t5;
  double ma,mb,mc,hm;
  const double normal_coeff = sqrt(6.) / 12.;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  ab[0] = p1[0]-p0[0];
  ab[1] = p1[1]-p0[1];
  ab[2] = p1[2]-p0[2];

  bc[0] = p2[0]-p1[0];
  bc[1] = p2[1]-p1[1];
  bc[2] = p2[2]-p1[2];
 
  ac[0] = p2[0]-p0[0];
  ac[1] = p2[1]-p0[1];
  ac[2] = p2[2]-p0[2];
 
  ad[0] = p3[0]-p0[0];
  ad[1] = p3[1]-p0[1];
  ad[2] = p3[2]-p0[2];
 
  bd[0] = p3[0]-p1[0];
  bd[1] = p3[1]-p1[1];
  bd[2] = p3[2]-p1[2];
 
  cd[0] = p3[0]-p2[0];
  cd[1] = p3[1]-p2[1];
  cd[2] = p3[2]-p2[2];

  t0 = vtkMath::Dot(ab,ab);
  t1 = vtkMath::Dot(bc,bc);
  t2 = vtkMath::Dot(ac,ac);
  t3 = vtkMath::Dot(ad,ad);
  t4 = vtkMath::Dot(bd,bd);
  t5 = vtkMath::Dot(cd,cd);

  ma = t0 > t1 ? t0 : t1;
  mb = t2 > t3 ? t2 : t3;
  mc = t4 > t5 ? t4 : t5;
  hm = ma > mb ? ma : mb;
  hm = hm > mc ? sqrt(hm) : sqrt(mc);

  vtkMath::Cross(ab,bc,bd);
  t0 = vtkMath::Norm(bd);
  vtkMath::Cross(ab,ad,bd);
  t1 = vtkMath::Norm(bd);
  vtkMath::Cross(ac,ad,bd);
  t2 = vtkMath::Norm(bd);
  vtkMath::Cross(bc,cd,bd);
  t3 = vtkMath::Norm(bd);

  t4 = fabs(vtkMath::Determinant3x3(ab,ac,ad));
 
  return normal_coeff * hm * (t0 + t1 + t2 + t3) / t4;
}

double vtkMeshQuality::TetRadiusRatio( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double ab[3],bc[3],ac[3],ad[3],bd[3],cd[3],u[3];
  double abc,abd,acd,bcd,a,b,c,det;
  const double normal_coeff = 1. / 12.;

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  ab[0] = p1[0]-p0[0];
  ab[1] = p1[1]-p0[1];
  ab[2] = p1[2]-p0[2];
 
  bc[0] = p2[0]-p1[0];
  bc[1] = p2[1]-p1[1];
  bc[2] = p2[2]-p1[2];
 
  ac[0] = p2[0]-p0[0];
  ac[1] = p2[1]-p0[1];
  ac[2] = p2[2]-p0[2];
 
  ad[0] = p3[0]-p0[0];
  ad[1] = p3[1]-p0[1];
  ad[2] = p3[2]-p0[2];
 
  bd[0] = p3[0]-p1[0];
  bd[1] = p3[1]-p1[1];
  bd[2] = p3[2]-p1[2];
 
  cd[0] = p3[0]-p2[0];
  cd[1] = p3[1]-p2[1];
  cd[2] = p3[2]-p2[2];

  a = sqrt(vtkMath::Dot(ab,ab) * vtkMath::Dot(cd,cd));
  b = sqrt(vtkMath::Dot(ac,ac) * vtkMath::Dot(bd,bd));
  c = sqrt(vtkMath::Dot(ad,ad) * vtkMath::Dot(bc,bc));

  vtkMath::Cross(ab,bc,u);
  abc = vtkMath::Norm(u);
  vtkMath::Cross(ab,ad,u);
  abd = vtkMath::Norm(u);
  vtkMath::Cross(ac,ad,u);
  acd = vtkMath::Norm(u);
  vtkMath::Cross(bc,cd,u);
  bcd = vtkMath::Norm(u);

  det = vtkMath::Determinant3x3(ab,ac,ad);

  return normal_coeff * sqrt((a+b+c) * (a+b-c) * (a+c-b) * (b+c-a)) * (abc + abd + acd + bcd) \
    / (det * det);
}

double vtkMeshQuality::TetFrobeniusNorm( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double u[3],v[3],w[3];
  double numerator,radicand;
  const double normal_exp = 1./3.;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  u[0] = p1[0]-p0[0];
  u[1] = p1[1]-p0[1];
  u[2] = p1[2]-p0[2];
 
  v[0] = p2[0]-p0[0];
  v[1] = p2[1]-p0[1];
  v[2] = p2[2]-p0[2];
 
  w[0] = p3[0]-p0[0];
  w[1] = p3[1]-p0[1];
  w[2] = p3[2]-p0[2];

  numerator  = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
  numerator += v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  numerator += w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
  numerator *= 1.5;
  numerator -= v[0] * u[0] + v[1] * u[1] + v[2] * u[2];
  numerator -= w[0] * u[0] + w[1] * u[1] + w[2] * u[2];
  numerator -= w[0] * v[0] + w[1] * v[1] + w[2] * v[2];

  radicand = vtkMath::Determinant3x3(u,v,w);
  radicand *= radicand;
  radicand *= 2.;

  return numerator / (3. * pow(radicand,normal_exp));
}

double vtkMeshQuality::TetMinAngle( vtkCell* cell )
{
  double p0[3],p1[3],p2[3],p3[3];
  double ab[3],bc[3],ad[3],cd[3];
  double abc[3],abd[3],acd[3],bcd[3];
  double nabc,nabd,nacd,nbcd;
  double alpha,beta,gamma,delta,epsilon,zeta;
  const double normal_coeff = .3183098861837906715377675267450287;

  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  ab[0] = p1[0]-p0[0];
  ab[1] = p1[1]-p0[1];
  ab[2] = p1[2]-p0[2];
 
  bc[0] = p2[0]-p1[0];
  bc[1] = p2[1]-p1[1];
  bc[2] = p2[2]-p1[2];
 
  ad[0] = p3[0]-p0[0];
  ad[1] = p3[1]-p0[1];
  ad[2] = p3[2]-p0[2];
 
  cd[0] = p3[0]-p2[0];
  cd[1] = p3[1]-p2[1];
  cd[2] = p3[2]-p2[2];

  vtkMath::Cross(ab,bc,abc);
  nabc = vtkMath::Norm(abc);
  vtkMath::Cross(ab,ad,abd);
  nabd = vtkMath::Norm(abd);
  vtkMath::Cross(ad,cd,acd);
  nacd = vtkMath::Norm(acd);
  vtkMath::Cross(bc,cd,bcd);
  nbcd = vtkMath::Norm(bcd);

  alpha   = acos(vtkMath::Dot(abc,abd) / (nabc * nabd));
  beta    = acos(vtkMath::Dot(abc,acd) / (nabc * nacd));
  gamma   = acos(vtkMath::Dot(abc,bcd) / (nabc * nbcd));
  delta   = acos(vtkMath::Dot(abd,acd) / (nabd * nacd));
  epsilon = acos(vtkMath::Dot(abd,bcd) / (nabd * nbcd));
  zeta    = acos(vtkMath::Dot(acd,bcd) / (nacd * nbcd));

  alpha = alpha < beta    ? alpha : beta;
  alpha = alpha < gamma   ? alpha : gamma;
  alpha = alpha < delta   ? alpha : delta;
  alpha = alpha < epsilon ? alpha : epsilon;
  
  return  (alpha < zeta ? alpha : zeta) * 180. * normal_coeff;
}

// Hexahedron quality measure:
// edge ratio

double vtkMeshQuality::HexEdgeRatio( vtkCell* cell)
{
  double p0[3],p1[3],p2[3],p3[3];
  double p4[3],p5[3],p6[3],p7[3];
  double a[3],b[3],c[3],d[3],e[3],f[3];
  double g[3],h[3],i[3],j[3],k[3],l[3];
  double a2,b2,c2,d2,e2,f2;
  double g2,h2,i2,j2,k2,l2;
  double mab,mcd,mef,Mab,Mcd,Mef;
  double mgh,mij,mkl,Mgh,Mij,Mkl;
  double m2,M2;
 
  vtkPoints *p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);
  p->GetPoint(4, p4);
  p->GetPoint(5, p5);
  p->GetPoint(6, p6);
  p->GetPoint(7, p7);

  a[0] = p1[0]-p0[0];
  a[1] = p1[1]-p0[1];
  a[2] = p1[2]-p0[2];
 
  b[0] = p2[0]-p1[0];
  b[1] = p2[1]-p1[1];
  b[2] = p2[2]-p1[2];
 
  c[0] = p3[0]-p2[0];
  c[1] = p3[1]-p2[1];
  c[2] = p3[2]-p2[2];
 
  d[0] = p0[0]-p3[0];
  d[1] = p0[1]-p3[1];
  d[2] = p0[2]-p3[2];
 
  e[0] = p4[0]-p0[0];
  e[1] = p4[1]-p0[1];
  e[2] = p4[2]-p0[2];
 
  f[0] = p5[0]-p1[0];
  f[1] = p5[1]-p1[1];
  f[2] = p5[2]-p1[2];

  g[0] = p6[0]-p2[0];
  g[1] = p6[1]-p2[1];
  g[2] = p6[2]-p2[2];

  h[0] = p7[0]-p3[0];
  h[1] = p7[1]-p3[1];
  h[2] = p7[2]-p3[2];

  i[0] = p5[0]-p4[0];
  i[1] = p5[1]-p4[1];
  i[2] = p5[2]-p4[2];
 
  j[0] = p6[0]-p5[0];
  j[1] = p6[1]-p5[1];
  j[2] = p6[2]-p5[2];

  k[0] = p7[0]-p6[0];
  k[1] = p7[1]-p6[1];
  k[2] = p7[2]-p6[2];

  l[0] = p4[0]-p7[0];
  l[1] = p4[1]-p7[1];
  l[2] = p4[2]-p7[2];

  a2 = vtkMath::Dot(a,a);
  b2 = vtkMath::Dot(b,b);
  c2 = vtkMath::Dot(c,c);
  d2 = vtkMath::Dot(d,d);
  e2 = vtkMath::Dot(e,e);
  f2 = vtkMath::Dot(f,f);
  g2 = vtkMath::Dot(g,g);
  h2 = vtkMath::Dot(h,h);
  i2 = vtkMath::Dot(i,i);
  j2 = vtkMath::Dot(j,j);
  k2 = vtkMath::Dot(k,k);
  l2 = vtkMath::Dot(l,l);

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

  return sqrt(M2 / m2);
}

