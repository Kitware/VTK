/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkDataSetEdgeSubdivisionCriterion.h"
#include "vtkStreamingTessellator.h"

#include <vtkstd/algorithm>

#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkDataSet.h"

#if defined(_MSC_VER)
# pragma warning (disable: 4996) /* 'vtkstd::_Copy_opt' was declared deprecated */
#endif

vtkStandardNewMacro(vtkDataSetEdgeSubdivisionCriterion);

vtkDataSetEdgeSubdivisionCriterion::vtkDataSetEdgeSubdivisionCriterion()
{
  this->CurrentMesh = 0;
  this->CurrentCellId = -1;
  this->CurrentCellData = 0;
  this->ChordError2 = 1e-6;
  // We require this->FieldError2 to be a valid address at all times -- it
  // may never be null
  this->FieldError2Capacity = 2;
  this->FieldError2 = new double[this->FieldError2Capacity];
  this->FieldError2Length = 0;
  this->ActiveFieldCriteria = 0;
}

vtkDataSetEdgeSubdivisionCriterion::~vtkDataSetEdgeSubdivisionCriterion()
{
  if ( this->CurrentMesh )
    this->CurrentMesh->UnRegister( this );
  delete [] this->FieldError2;
}

void vtkDataSetEdgeSubdivisionCriterion::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "CurrentCellId: " << this->CurrentCellId << endl;
  os << indent << "CurrentMesh: " << this->CurrentMesh << endl;
  os << indent << "ChordError2: " << this->ChordError2 << endl;
  os << indent << "ActiveFieldCriteria: " << this->ActiveFieldCriteria << endl;
}

void vtkDataSetEdgeSubdivisionCriterion::SetMesh( vtkDataSet* mesh )
{
  if ( mesh == this->CurrentMesh )
    return;

  if ( this->CurrentMesh )
    this->CurrentMesh->UnRegister( this );

  this->CurrentMesh = mesh;
  this->Modified();

  if ( this->CurrentMesh )
    this->CurrentMesh->Register( this );
}

void vtkDataSetEdgeSubdivisionCriterion::SetCellId( vtkIdType cell )
{
  if ( cell == this->CurrentCellId )
    return;

  this->CurrentCellId = cell;

  if ( this->CurrentMesh )
    this->CurrentCellData = this->CurrentMesh->GetCell( this->CurrentCellId );

  this->Modified();
}

double* vtkDataSetEdgeSubdivisionCriterion::EvaluateFields( double* vertex, double* weights, int field_start )
{
  const int* fields = this->GetFieldIds();
  const int* offsets = this->GetFieldOffsets();

  for ( int f=0; f<this->GetNumberOfFields(); ++f )
    {
    // Do the magic of evaluating either:
    //  - the nodal (linear or quadratic) fields here
    //  - the cell (constant or linear) fields here
    // Negative IDs are cell data? I dunno, we need some kinda convention.
    if ( fields[f] < 0 )
      this->EvaluateCellDataField( vertex + field_start + offsets[f], weights, -(1+fields[f]) );
    else
      this->EvaluatePointDataField( vertex + field_start + offsets[f], weights, fields[f] );
    }
  return vertex;
}

void vtkDataSetEdgeSubdivisionCriterion::EvaluatePointDataField( double* result, double* weights, int field )
{
  vtkDataArray* array = this->CurrentMesh->GetPointData()->GetArray( field );
  vtkIdList* ptIds = this->CurrentCellData->GetPointIds();
  int npts = ptIds->GetNumberOfIds();
  int nc = array->GetNumberOfComponents();
  int i, j;
  for ( j=0; j<nc; ++j )
    result[j] = 0.;
  for ( i=0; i<npts; ++i )
    {
    double* tuple = array->GetTuple( ptIds->GetId(i) );
    for ( j=0; j<nc; ++j )
      result[j] += weights[i]*tuple[j];
    }
}

void vtkDataSetEdgeSubdivisionCriterion::EvaluateCellDataField( double* result, double* vtkNotUsed(weights), int field )
{
  // FIXME
  // VTK's CellData really assumes that there will only be one value per cell (i.e., we
  // will only ever store a function constant over the entire cell). Things like
  // the discontinuous galerkin method produce data that is cell-specific but
  // not constant over the cell. There's no real way to represent this in VTK,
  // so at the moment, this code punts and assumes cell-constant data.
  vtkDataArray* array = this->CurrentMesh->GetCellData()->GetArray( field );
  int nc = array->GetNumberOfComponents();
  int j;
  double* tuple = array->GetTuple( this->CurrentCellId );
  for ( j=0; j<nc; ++j )
    result[j] = tuple[j];
}

bool vtkDataSetEdgeSubdivisionCriterion::EvaluateEdge( const double* p0, double* midpt, const double* p1, int field_start )
{
  static double weights[27];
  static int dummySubId=-1;
  double realMidPt[ 3 ];

  this->CurrentCellData->EvaluateLocation( dummySubId, midpt + 3, realMidPt, weights );
  double chord2 = 0.;
  double tmp;
  int c;
  for ( c = 0; c < 3; ++c )
    {
    tmp = midpt[c] - realMidPt[c];
    chord2 += tmp * tmp;
    }

  bool rval = chord2 > this->ChordError2;
  if ( rval )
    {
    for ( c = 0; c < 3; ++c )
      midpt[c] = realMidPt[c];

    this->EvaluateFields( midpt, weights, field_start );
    return true;
    }

  int active = this->GetActiveFieldCriteria();
  if ( active )
    {
    double real_pf[6+vtkStreamingTessellator::MaxFieldSize];
    vtkstd::copy( midpt, midpt + field_start, real_pf );
    this->EvaluateFields( real_pf, weights, field_start );

    rval = this->FixedFieldErrorEval( p0, midpt, real_pf, p1, field_start, active, this->FieldError2 );
#if 0
    cout << (rval ? "*" : " ")
      <<    "p0 " <<      p0[13] << ", " <<      p0[14] << ", " <<      p0[15]
      << "   md " <<   midpt[13] << ", " <<   midpt[14] << ", " <<   midpt[15]
      << "   cm " << real_pf[13] << ", " << real_pf[14] << ", " << real_pf[15]
      << "   p1 " <<      p1[13] << ", " <<      p1[14] << ", " <<      p1[15] << endl;
#endif
    if ( rval )
      {
      vtkstd::copy( real_pf+field_start, real_pf+field_start+this->FieldOffsets[this->NumberOfFields], midpt+field_start );
      }
    }

  return rval;
}

void vtkDataSetEdgeSubdivisionCriterion::SetFieldError2( int s, double err )
{
  if ( s < this->FieldError2Length )
    {
    if  ( this->FieldError2[s] == err )
      return; // no change
    }
  else
    {
    if ( err <= 0. )
      return; // no need to allocate more memory to store an unused value
    }

  if ( this->FieldError2Capacity <= s )
    {
    int nc = this->FieldError2Capacity;
    while ( nc <= s )
      nc <<= 1;
    double* tmp = new double[ nc ];
    for ( int i=0; i<this->FieldError2Length; ++i )
      tmp[i] = this->FieldError2[i];
    delete [] this->FieldError2;
    this->FieldError2 = tmp;
    this->FieldError2Capacity = nc;
    }
  for ( int j=this->FieldError2Length; j<s; ++j )
    this->FieldError2[j] = -1.;
  this->FieldError2Length = this->FieldError2Length > s ? this->FieldError2Length : s + 1;

  if ( s < int(sizeof(int)*8)  && s >= 0 )
    {
    if ( err > 0. )
      this->ActiveFieldCriteria = this->ActiveFieldCriteria | (1<<s);
    else
      this->ActiveFieldCriteria = this->ActiveFieldCriteria & ~(1<<s);
    }

  this->FieldError2[s] = err;
  this->Modified();
}

double vtkDataSetEdgeSubdivisionCriterion::GetFieldError2( int s ) const
{
  if ( s >= this->FieldError2Length || s < 0 )
    return -1;
  return this->FieldError2[s];
}

void vtkDataSetEdgeSubdivisionCriterion::ResetFieldError2()
{
  this->FieldError2Length = 0;
  this->ActiveFieldCriteria = 0;
}

