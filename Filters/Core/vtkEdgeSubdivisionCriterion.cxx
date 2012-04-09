/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkEdgeSubdivisionCriterion.h"
#include "vtkStreamingTessellator.h"
#include "vtkDataSetAttributes.h"
#include "vtkMatrix4x4.h"


void vtkEdgeSubdivisionCriterion::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

vtkEdgeSubdivisionCriterion::vtkEdgeSubdivisionCriterion()
{
  this->FieldIds     = new int [ vtkStreamingTessellator::MaxFieldSize ];
  this->FieldOffsets = new int [ vtkStreamingTessellator::MaxFieldSize + 1 ];
  this->FieldOffsets[0] = 0;
  this->NumberOfFields = 0;
}

vtkEdgeSubdivisionCriterion::~vtkEdgeSubdivisionCriterion()
{
  delete[] this->FieldIds;
  delete[] this->FieldOffsets;
};

void vtkEdgeSubdivisionCriterion::ResetFieldList()
{
  this->NumberOfFields = 0;
}

int vtkEdgeSubdivisionCriterion::PassField( int sourceId, int sourceSize, vtkStreamingTessellator* t )
{
  if ( sourceSize + this->FieldOffsets[ this->NumberOfFields ] > vtkStreamingTessellator::MaxFieldSize )
    {
    vtkErrorMacro( "PassField source size (" << sourceSize << ") was too large for vtkStreamingTessellator" );
    }

  int off = this->GetOutputField( sourceId );
  if ( off == -1 )
    {
    this->FieldIds[ this->NumberOfFields ] = sourceId;
    off = this->FieldOffsets[ this->NumberOfFields ];
    t->SetFieldSize( -1, this->FieldOffsets[ ++this->NumberOfFields ] = off + sourceSize );
    this->Modified();
    }
  else
    {
    off = this->FieldOffsets[ off ];
    vtkWarningMacro( "Field " << sourceId << " is already being passed as offset " << off << "." );
    }

  return off;
}

bool vtkEdgeSubdivisionCriterion::DontPassField( int sourceId, vtkStreamingTessellator* t )
{
  int id = this->GetOutputField( sourceId );
  if ( id == -1 )
    return false;

  int sz = this->FieldOffsets[id+1] - this->FieldOffsets[id];
  for ( int i=id+1; i<this->GetNumberOfFields(); ++i )
    {
    this->FieldIds[i-1] = this->FieldIds[i];
    this->FieldOffsets[i] = this->FieldOffsets[i+1] - sz;
    }
  t->SetFieldSize( -1, this->FieldOffsets[ this->GetNumberOfFields() ] );
  this->Modified();

  return true;
}

int vtkEdgeSubdivisionCriterion::GetOutputField( int sourceId ) const
{
  for ( int i=0; i<this->NumberOfFields; ++i )
    if ( this->FieldIds[i] == sourceId )
      return i;

  return -1;
}

bool vtkEdgeSubdivisionCriterion::ViewDependentEval( 
  const double* p0, double* p1, double* real_p1, 
  const double* p2, int , 
  vtkMatrix4x4* Transform, const double* PixelSize, 
  double AllowableChordError ) const
{
  double real_p1t[4];
  double intr_p1t[4];

  Transform->MultiplyPoint( real_p1, real_p1t );
  Transform->MultiplyPoint( p1, intr_p1t );
  double eprod = fabs(AllowableChordError*real_p1t[3]*intr_p1t[3]);
  /*
     fprintf( stderr, "eprod=%g, compare to <%g,%g>\n", eprod,
     fabs(real_p1t[0]*intr_p1t[3]-intr_p1t[0]*real_p1t[3])/PixelSize[0],
     fabs(real_p1t[1]*intr_p1t[3]-intr_p1t[1]*real_p1t[3])/PixelSize[1] );
   */
  if ( (real_p1t[0] > real_p1t[3]) || (real_p1t[0] < -real_p1t[3]) || 
      (real_p1t[1] > real_p1t[3]) || (real_p1t[1] < -real_p1t[3]) ) {
    double p0t[4];
    double p2t[4];
    for (int i=0; i<3; i++) {
      p0t[i] = p0[i];
      p2t[i] = p2[i];
    }
    p0t[3] = p2t[3] = 1.;
    Transform->MultiplyPoint( p0t, p0t );
    Transform->MultiplyPoint( p2t, p2t );
    int p0Code=0, p2Code=0;
#define ENDPOINT_CODE(code,pt) \
    if ( pt[0] > pt[3] ) \
      code += 1; \
    else if ( pt[0] < -pt[3] ) \
      code += 2; \
        if ( pt[1] > pt[3] ) \
          code += 4; \
        else if ( pt[1] < -pt[3] ) \
          code += 8;

    ENDPOINT_CODE(p0Code,p0t);
    ENDPOINT_CODE(p2Code,p2t);
    if ( p0Code & p2Code ) {
      return false ;
    }
  }
  if ( fabs(real_p1t[0]*intr_p1t[3]-intr_p1t[0]*real_p1t[3])/PixelSize[0] > eprod ||
      fabs(real_p1t[1]*intr_p1t[3]-intr_p1t[1]*real_p1t[3])/PixelSize[1] > eprod ) {
    // copy the properly interpolated point into the result
    for ( int c=0; c<3; ++c )
      p1[c] = real_p1[c];
    return true ; // need to subdivide
  }

  return false ; // no need to subdivide
}

bool vtkEdgeSubdivisionCriterion::FixedFieldErrorEval( const double*, double* p1, double* real_pf, const double*, int field_start, int criteria, double* AllowableL2Error2 ) const
{
  int id = 0;
  double mag;
  while ( criteria )
    {
    if ( ! (criteria & 1) )
      {
      criteria >>= 1;
      ++id;
      continue;
      }

    mag = 0.;
    int fsz = this->FieldOffsets[id+1] - this->FieldOffsets[id];
    for ( int c=0; c<fsz; ++c )
      {
      double tmp = real_pf[c+field_start] - p1[c+field_start];
      mag += tmp*tmp;
      }
    if ( mag > AllowableL2Error2[id] )
      {
      return true;
      }
    criteria >>= 1;
    ++id;
    }

  return false;
}


