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

#include <vtkPoints.h>
#include <vtkMath.h>
#include <vtkTetra.h>

#include "vtkMeshQuality.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkCellTypes.h"

vtkCxxRevisionMacro(vtkMeshQuality,"1.12");
vtkStandardNewMacro(vtkMeshQuality);

typedef double (*CellQualityType)( vtkCell* );

double TetVolume( vtkCell* cell );

const char* QualityMeasureNames[] =
{
  "RadiusRatio",
  "AspectRatio",
  "FrobeniusNorm",
  "EdgeRatio"
};

void vtkMeshQuality::PrintSelf( ostream& os, vtkIndent indent )
{
  static const char* onStr = "On";
  static const char* offStr = "Off";

  this->Superclass::PrintSelf( os, indent );

  os << indent << "SaveCellQuality:   " << (this->SaveCellQuality ? onStr : offStr) << endl;
  os << indent << "TriangleQualityMeasure: " << QualityMeasureNames[this->TriangleQualityMeasure] << endl;
  os << indent << "QuadQualityMeasure: " << QualityMeasureNames[this->QuadQualityMeasure] << endl;
  os << indent << "TetQualityMeasure: " << QualityMeasureNames[this->TetQualityMeasure] << endl;
  os << indent << "HexQualityMeasure: " << QualityMeasureNames[this->HexQualityMeasure] << endl;
  os << indent << "Volume: " << (this->Volume ? onStr : offStr) << endl;
  os << indent << "CompatibilityMode: " << (this->CompatibilityMode ? onStr : offStr) << endl;
}

vtkMeshQuality::vtkMeshQuality()
{
  this->SaveCellQuality = 1; // Default is On
  this->TriangleQualityMeasure = VTK_QUALITY_RADIUS_RATIO;
  this->QuadQualityMeasure = VTK_QUALITY_EDGE_RATIO;
  this->TetQualityMeasure = VTK_QUALITY_RADIUS_RATIO;
  this->HexQualityMeasure = VTK_QUALITY_RADIUS_RATIO;
  this->Volume = 0;
  this->CompatibilityMode = 0;
}

vtkMeshQuality::~vtkMeshQuality()
{
  // Nothing yet.
}

void vtkMeshQuality::Execute()
{
  CellQualityType TriangleQuality,QuadQuality,TetQuality;
  vtkDoubleArray* quality = 0;
  vtkDoubleArray* volume = 0;
  vtkDataSet* in = this->GetInput();
  vtkDataSet* out = this->GetOutput();
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
  vtkIdType c;
  vtkCell* cell;

  // Initialize the min and max values, std deviations, etc.
  qtriM = qquaM = qtetM = qhexM = VTK_DOUBLE_MIN;
  qtrim = qquam = qtetm = qhexm = VTK_DOUBLE_MAX;
  Eqtri = Eqtri2 = Eqqua = Eqqua2 = Eqtet = Eqtet2 = Eqhex = Eqhex2 = 0.;

  switch ( this->GetTriangleQualityMeasure() )
    {
    case VTK_QUALITY_RADIUS_RATIO:
      TriangleQuality = TriangleRadiusRatio;
      break;
    case VTK_QUALITY_ASPECT_RATIO:
      TriangleQuality = TriangleAspectRatio;
      break;
    case VTK_QUALITY_FROBENIUS_NORM:
      TriangleQuality = TriangleFrobeniusNorm;
      break;
    case VTK_QUALITY_EDGE_RATIO:
      TriangleQuality = TriangleEdgeRatio;
      break;
    default:
      TriangleQuality = NULL;
      break;
    }

  switch ( this->GetQuadQualityMeasure() )
    {
    case VTK_QUALITY_RADIUS_RATIO:
      QuadQuality = QuadRadiusRatio;
      break;
    case VTK_QUALITY_ASPECT_RATIO:
      QuadQuality = QuadAspectRatio;
      break;
    case VTK_QUALITY_FROBENIUS_NORM:
      QuadQuality = QuadFrobeniusNorm;
      break;
    case VTK_QUALITY_EDGE_RATIO:
      QuadQuality = QuadEdgeRatio;
      break;
    default:
      QuadQuality = NULL;
      break;
    }

  switch ( this->GetTetQualityMeasure() )
    {
    case VTK_QUALITY_RADIUS_RATIO:
      TetQuality = TetRadiusRatio;
      break;
    case VTK_QUALITY_ASPECT_RATIO:
      TetQuality = TetAspectRatio;
      break;
    case VTK_QUALITY_FROBENIUS_NORM:
      TetQuality = TetFrobeniusNorm;
      break;
    case VTK_QUALITY_EDGE_RATIO:
      TetQuality = TetEdgeRatio;
      break;
    default:
      TetQuality = NULL;
      break;
    }

  out->ShallowCopy( in );

  if ( this->SaveCellQuality )
    {
    quality = vtkDoubleArray::New();
    quality->SetNumberOfTuples( N );
    quality->SetName( "Quality" );
    out->GetCellData()->AddArray( quality );
    out->GetCellData()->SetActiveAttribute( "Quality", vtkDataSetAttributes::SCALARS );
    quality->Delete();
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
      if ( this->Volume )
        {
        volume = vtkDoubleArray::New();
        volume->SetNumberOfComponents(1);
        volume->SetNumberOfTuples( N );
        volume->SetName( "Volume" );
        out->GetCellData()->AddArray( volume );
        }
      }
    }

  for ( c = 0; c < N; ++c )
    {
    cell = out->GetCell( c );
    V = 0.;
    switch ( cell->GetCellType() )
      {
    case VTK_TRIANGLE:
      q = TriangleQuality( cell );
      if ( q > qtriM )
        {
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
      q = HexahedronQuality( cell );
      if ( q > qhexM )
        {
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

  if ( ntri )
    {
      Eqtri  /= (double) ntri;
      Eqtri2 /= (double) ntri;
      if ( ntri == 1 )
        {
        if ( qtrim == VTK_DOUBLE_MAX )
          {
          qtrim = qtriM;
          }
        else
          {
          qtriM = qtrim;
          }
        }
    }
  else
    {
    qtrim = Eqtri = qtriM = Eqtri2 = 0.;
    }
  
  if ( nqua )
    {
      Eqqua  /= (double) nqua;
      Eqqua2 /= (double) nqua;
      if ( nqua == 1 )
        {
        if ( qquam == VTK_DOUBLE_MAX )
          {
          qquam = qquaM;
          }
        else
          {
          qquaM = qquam;
          }
        }
     }
  else
    {
    qquam = Eqqua = qquaM = Eqqua2 = 0.;
    }
  
  if ( ntet )
    {
      Eqtet  /= (double) ntet;
      Eqtet2 /= (double) ntet;
      if ( ntet == 1 )
        {
        if ( qtetm == VTK_DOUBLE_MAX )
          {
          qtetm = qtetM;
          }
        else
          {
          qtetM = qtetm;
          }
        }
     }
  else
    {
    qtetm = Eqtet = qtetM = Eqtet2 = 0.;
    }
  
  if ( nhex )
    {
      Eqhex  /= (double) nhex;
      Eqhex2 /= (double) nhex;
      if ( nhex == 1 )
        {
        if ( qhexm == VTK_DOUBLE_MAX )
          {
          qhexm = qhexM;
          }
        else
          {
          qhexM = qhexm;
          }
        }
     }
  else
    {
    qhexm = Eqhex = qhexM = Eqhex2 = 0.;
    }
  
  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Triangle Quality" );
  quality->SetNumberOfComponents(4);
  quality->InsertNextTuple4( qtrim, Eqtri, qtriM, Eqtri2 );
  out->GetFieldData()->AddArray( quality );

  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Quadrilateral Quality" );
  quality->SetNumberOfComponents(4);
  quality->InsertNextTuple4( qquam, Eqqua, qquaM, Eqqua2 );
  out->GetFieldData()->AddArray( quality );

  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Tetrahedron Quality" );
  quality->SetNumberOfComponents(4);
  quality->InsertNextTuple4( qtetm, Eqtet, qtetM, Eqtet2 );
  out->GetFieldData()->AddArray( quality );

  quality = vtkDoubleArray::New();
  quality->SetName( "Mesh Hexahedron Quality" );
  quality->SetNumberOfComponents(4);
  quality->InsertNextTuple4( qhexm, Eqhex, qhexM, Eqhex2 );
  out->GetFieldData()->AddArray( quality );
}

double vtkMeshQuality::TriangleRadiusRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double a1,b1,c1,ab;
  
  p = cell->GetPoints();
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

double vtkMeshQuality::TriangleAspectRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double a1,b1,c1,hm;
  static double normal_coeff=sqrt(3.) / 6.;
  
  p = cell->GetPoints();
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

double vtkMeshQuality::TriangleFrobeniusNorm( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double t22;
  static double normal_coeff=.5 / sqrt(3.);
  
  p = cell->GetPoints();
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

double vtkMeshQuality::TriangleEdgeRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3];
  double a[3],b[3],c[3];
  double a2,b2,c2,m2,M2;
  
  p = cell->GetPoints();
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

double vtkMeshQuality::QuadRadiusRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3],m[3],n[3];
  double ab[3],bc[3],cd[3],da[3];
  double a2,b2,c2,d2,m2,n2,h2;
  double t0,t1,t2,t3;
  static double normal_coeff=1. / (2.*sqrt(2.));
  
  p = cell->GetPoints();
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

double vtkMeshQuality::QuadAspectRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3],ab[3],cd[3];
  double a1,b1,c1,d1;
  double ma,mb,hm;
  static double normal_coeff=.5;
  
  p = cell->GetPoints();
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

  return normal_coeff * hm * (a1 + b1 + c1 + d1) / (vtkMath::Norm(ab) + vtkMath::Norm(cd));
}

double vtkMeshQuality::QuadFrobeniusNorm( vtkCell* vtkNotUsed(cell) )
{
  return 1.;
}

double vtkMeshQuality::QuadEdgeRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3],p3[3];
  double a[3],b[3],c[3],d[3];
  double a2,b2,c2,d2,mab,Mab,mcd,Mcd,m2,M2;
  
  p = cell->GetPoints();
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

double vtkMeshQuality::TetRadiusRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3],p3[3];
  double incenter[3],circumcenter[3];
  
  p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  return sqrt(vtkTetra::Circumsphere(p0,p1,p2,p3,circumcenter)) /
    (3. * vtkTetra::Insphere(p0,p1,p2,p3,incenter));
}

double vtkMeshQuality::TetAspectRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3],p3[3];
  double u[3],v[3],w[3],x[3],y[3],z[3];
  double t0,t1,t2,t3,t4,t5;
  double ma,mb,mc,hm;
  static double normal_coeff=sqrt(6.) / 12.;
  
  p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  u[0] = p1[0]-p0[0];
  u[1] = p1[1]-p0[1];
  u[2] = p1[2]-p0[2];
  
  v[0] = p2[0]-p1[0];
  v[1] = p2[1]-p1[1];
  v[2] = p2[2]-p1[2];
  
  w[0] = p2[0]-p0[0];
  w[1] = p2[1]-p0[1];
  w[2] = p2[2]-p0[2];
  
  x[0] = p3[0]-p0[0];
  x[1] = p3[1]-p0[1];
  x[2] = p3[2]-p0[2];
  
  y[0] = p3[0]-p1[0];
  y[1] = p3[1]-p1[1];
  y[2] = p3[2]-p1[2];
  
  z[0] = p3[0]-p2[0];
  z[1] = p3[1]-p2[1];
  z[2] = p3[2]-p2[2];

  t0 = vtkMath::Dot(u,u);
  t1 = vtkMath::Dot(v,v);
  t2 = vtkMath::Dot(w,w);
  t3 = vtkMath::Dot(x,x);
  t4 = vtkMath::Dot(y,y);
  t5 = vtkMath::Dot(z,z);

  ma = t0 > t1 ? t0 : t1;
  mb = t2 > t3 ? t2 : t3;
  mc = t4 > t5 ? t4 : t5;
  hm = ma > mb ? ma : mb;
  hm = hm > mc ? sqrt(hm) : sqrt(mc);

  vtkMath::Cross(u,v,y);
  t0 = vtkMath::Norm(y);
  vtkMath::Cross(u,x,y);
  t1 = vtkMath::Norm(y);
  vtkMath::Cross(w,x,y);
  t2 = vtkMath::Norm(y);
  vtkMath::Cross(v,z,y);
  t3 = vtkMath::Norm(y);

  t4 = fabs(vtkMath::Determinant3x3(u,w,x));
  
  return normal_coeff * hm * (t0 + t1 + t2 + t3) / t4;
}

double vtkMeshQuality::TetFrobeniusNorm( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3],p3[3];
  double u[3],v[3],w[3];
  double den;
  static double normal_coeff=sqrt(2.);
  
  p = cell->GetPoints();
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

  den  = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
  den += v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  den += w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
  den *= 1.5;
  den -= v[0] * u[0] + v[1] * u[1] + v[2] * u[2];
  den -= w[0] * u[0] + w[1] * u[1] + w[2] * u[2];
  den -= w[0] * v[0] + w[1] * v[1] + w[2] * v[2];

  return den / (3. * pow(normal_coeff * vtkMath::Determinant3x3(u,v,w),2./3.));
}

double vtkMeshQuality::TetEdgeRatio( vtkCell* cell )
{
  vtkPoints* p;
  double p0[3],p1[3],p2[3],p3[3];
  double u[3],v[3],w[3],x[3],y[3],z[3];
  double a2,b2,c2,d2,e2,f2;
  double m2,M2,mab,mcd,mef,Mab,Mcd,Mef;
  
  p = cell->GetPoints();
  p->GetPoint(0, p0);
  p->GetPoint(1, p1);
  p->GetPoint(2, p2);
  p->GetPoint(3, p3);

  u[0] = p1[0]-p0[0];
  u[1] = p1[1]-p0[1];
  u[2] = p1[2]-p0[2];
  
  v[0] = p2[0]-p1[0];
  v[1] = p2[1]-p1[1];
  v[2] = p2[2]-p1[2];
  
  w[0] = p2[0]-p0[0];
  w[1] = p2[1]-p0[1];
  w[2] = p2[2]-p0[2];
  
  x[0] = p3[0]-p0[0];
  x[1] = p3[1]-p0[1];
  x[2] = p3[2]-p0[2];
  
  y[0] = p3[0]-p1[0];
  y[1] = p3[1]-p1[1];
  y[2] = p3[2]-p1[2];
  
  z[0] = p3[0]-p2[0];
  z[1] = p3[1]-p2[1];
  z[2] = p3[2]-p2[2];

  a2 = vtkMath::Dot(u,u);
  b2 = vtkMath::Dot(v,v);
  c2 = vtkMath::Dot(w,w);
  d2 = vtkMath::Dot(x,x);
  e2 = vtkMath::Dot(y,y);
  f2 = vtkMath::Dot(z,z);

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

double vtkMeshQuality::HexahedronQuality( vtkCell* vtkNotUsed(cell) )
{
  return 1.;
}

