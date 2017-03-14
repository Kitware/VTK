#!/usr/bin/python2.2
# -*- coding: latin-1 -*-
#
# Copyright 2003 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the
# U.S. Government. Redistribution and use in source and binary forms, with
# or without modification, are permitted provided that this Notice and any
# statement of authorship are reproduced on all copies.
#

import sys, re, math

QualityThang = 0

# This script writes some of the tessellator code along with
# a few static variables containing configuration information
# for tetrahedral decompositions.
# It is broken into 4 sections:
# - the class definition of vtkTessCase (represent a decomposition)
#   (no output)
# - the declaration of decompositions and permutations
#   (outputs static arrays)
# - the logic that produces test
#   (outputs C++ code that references the static arrays)

# ============================================================================
# A class to organize the list of output tetrahedra
class vtkTessCase:
  """
vtkTessCase holds the tetrahedral decomposition of a simplex.
"""
  CurrentOffset = 0
  AllCases = {}

  def PrintAll( fout ):
    print >> fout, "vtkIdType vtkStreamingTessellator::TetrahedralDecompositions[] = \n{"
    tmp = []
    for k in vtkTessCase.AllCases.keys():
      tmp.append( ( vtkTessCase.AllCases[k].Offset, k ) )
    tmp.sort()
    #for t in tmp:
    #  print t
    for t in tmp:
      vtkTessCase.AllCases[t[1]].Print( fout )
    print >> fout, "};\n"
  PrintAll = staticmethod( PrintAll )

  def GetOffset( label ):
    if label == '':
      return -1
    try:
      val = vtkTessCase.AllCases[label].Offset
    except:
      val = -1
    return val
  GetOffset = staticmethod( GetOffset )

  def __init__( self, label ):
    """Create an empty decomposition with a unique name."""
    #print >> sys.stderr, '  Tetrahedra: %s' % label
    self.Tets = []
    self.Label = label
    self.Offset = vtkTessCase.CurrentOffset
    vtkTessCase.CurrentOffset += 1
    vtkTessCase.AllCases[ self.Label ] = self

  def InsertTet( self, indices ):
    """Add tetrahedra to the decomposition. The argument must be
    a vector of integer numbers whose length is a multiple of 4.
    Every tetrahedron should be followed by an integer bit vector
    calling out any edges that are internal to the subdivision
    with a nonzero bit."""
    self.Tets += indices
    vtkTessCase.CurrentOffset += len(indices)
    if len(indices) % 4:
      raise 'Bad number of indices!'

  def Print( self, fout ):
    """Write the decomposition out to the given file."""
    print >> fout, '// case %s' % self.Label
    print >> fout, '  %2d,' % (len(self.Tets)/4)
    for i in range(len(self.Tets)/4):
      print >> fout, '  %2d, %2d, %2d, %2d,' % tuple(self.Tets[(i*4):(i*4+4)])
    print >> fout, ''
    #self.Offset = vtkTessCase.CurrentOffset
    #vtkTessCase.CurrentOffset += len(self.Tets)+2

# ============================================================================
# Global variables that hold code generation state

currentCase = 'invalid'
currentCaseCtr = -1
currentBits = {}
caseLabel = 0

# ============================================================================
# Constant global variables

genCode = file('vtkStreamingTessellator.cxx','w')
# These are EDGE numbers of the edges on a given face, NOT vertex numbers
FaceEdges = [ [0,1,2], [0,3,4], [1,4,5], [2,5,3] ]
# These are the vertices of a given edge
EdgeVerts = [ [0,1], [1,2], [0,2], [0,3], [1,3], [2,3] ]
# This dictionary maps from vertices to a face index
FaceFromVerts = { (0,1,2):0, (0,1,3):1, (1,2,3):2, (0,2,3):3 }
# The Ruprecht-Müller cases
# Each case is assigned a tuple containing:
# 1. A unique integer (used in switch statements)
# 2. A list of edges to be subdivided. This is the list
#    of edges for the canonical configuration.
RMCases = {\
    '1'  : ( 0, [0]), \
    '2a' : ( 1, [0,1]), \
    '2b' : ( 2, [0,5]), \
    '3a' : ( 3, [0,2,3]), \
    '3b' : ( 4, [0,3,4]), \
    '3c' : ( 5, [0,1,3]), \
    '3d' : ( 6, [0,2,4]), \
    '4a' : ( 7, [2,3,4,5]), \
    '4b' : ( 8, [1,2,3,4]), \
    '5'  : ( 9, [1,2,3,4,5]), \
    '6'  : (10, [0,1,2,3,4,5]) \
    }

re_if = re.compile( '([0-9]+)[ \t]*([<>][=]?|=)[ \t]*([0-9]+)' )
re_switch = re.compile( '(\d+)' )
PermutationIndices = { \
  ()        : ( 0,+1), \
  (0,1,2,3) : ( 0,+1), \
  (1,2,0,3) : ( 1,+1), \
  (2,0,1,3) : ( 2,+1), \
  (0,3,1,2) : ( 3,+1), \
  (3,1,0,2) : ( 4,+1), \
  (1,0,3,2) : ( 5,+1), \
  (1,3,2,0) : ( 6,+1), \
  (3,2,1,0) : ( 7,+1), \
  (2,1,3,0) : ( 8,+1), \
  (2,3,0,1) : ( 9,+1), \
  (3,0,2,1) : (10,+1), \
  (0,2,3,1) : (11,+1), \
  (0,2,1,3) : (12,-1), \
  (2,1,0,3) : (13,-1), \
  (1,0,2,3) : (14,-1), \
  (0,1,3,2) : (15,-1), \
  (1,3,0,2) : (16,-1), \
  (3,0,1,2) : (17,-1), \
  (1,2,3,0) : (18,-1), \
  (2,3,1,0) : (19,-1), \
  (3,1,2,0) : (20,-1), \
  (2,0,3,1) : (21,-1), \
  (0,3,2,1) : (22,-1), \
  (3,2,0,1) : (23,-1), \
}

# ============================================================================
# Utility routines

def GetInverse( op ):
  if op == '<':
    return '>'
  elif op == '>':
    return '<'
  return '?'

def GetBitcodeFromConditional( conditional, indices ):
  #print 'Indices:   %s' % indices
  stuff = re_switch.split( conditional )
  bits = 0
  v = []
  for t in stuff:
    if t == '':
      continue
    v.append( t )
  for i in range(1,len(v)-1,2):
    if v[i] == ',':
      # This conditional requires multiple disjoint comparisons
      # and the ',' just serves as a separator. Skip it.
      continue
    #print '%d: %s %s %s' % ( i, v[i-1], v[i], v[i+1] )
    cind = ( int(v[i-1]), int(v[i+1]) )
    flip = 0
    if not indices.has_key( cind ):
      cind = ( cind[1], cind[0] )
      flip = 1
    if not indices.has_key( cind ):
      # This edge pair is unimportant (inequality contains
      # more constraints than are required to characterize
      # this case). Print warning and skip:
      print '*** WARNING *** Edge comparison %s %s %s unnecessary!' % ( v[i-1], v[i], v[i+1] )
      continue
    if v[i] == ',':
      continue
    elif v[i] == '=':
      m = indices[ cind ]
      if not flip:
        bits += 2**m + 2**(m+1)
      else:
        bits += 2**(m+1) + 2**m
    elif v[i] == '<':
      m = indices[ cind ]
      if not flip:
        bits += 2**m
      else:
        bits += 2**(m+1)
    elif v[i] == '>':
      m = indices[ cind ]
      if not flip:
        bits += 2**(m+1)
      else:
        bits += 2**m
  return bits

def GetVerticesFromPair( edgePair ):
  a = []
  for e in edgePair:
    a += EdgeVerts[ e ]
  top = -1
  for i in range(len(a)):
    if a.count(a[i]) == 2:
      top = i
      break
  if top < 0:
    raise 'Given a bad pair of edges! They\'re not on the same face.'
  v0 = a[top]
  a.remove(v0)
  a.remove(v0)
  a = [ v0, a[0], a[1] ]
  return a

def GetFaceFromVertices( verts ):
  verts.sort()
  return FaceFromVerts[ (verts[0],verts[1],verts[2]) ]

def EdgesToSubdivide( edges ):
  global currentCaseCtr, currentCase, currentBits
  possibleFaces = []
  edgePairs = {}
  #print 'These edges are subdivided: %s' % edges
  for f in range(4):
    edgePair = ()
    for edge in FaceEdges[f]:
      #print '  Is %s in the list?' % edge
      if edge in edges:
        edgePair = edgePair + tuple([ edge ])
    #print 'edgePairs are %s' % str(edgePair)
    if len(edgePair) == 2:
      possibleFaces += [ f ]
      edgePairs[ edgePair ] = 1
  #print 'Possible faces: %s' % possibleFaces
  edgePairs = edgePairs.keys()
  #print 'Edge pairs : %s' % edgePairs
  if len(edgePairs):
    print >> genCode, '      comparisonBits = '
  tmp = 1;
  for p in edgePairs:
    print >> genCode, '        (permlen[%d] <= permlen[%d] ? %d : 0) | (permlen[%d] >= permlen[%d] ? %d : 0) |' % ( p[0], p[1], tmp, p[0], p[1], 2*tmp )
    tmp = tmp * 4
  if len(edgePairs):
    print >> genCode, '        0;'
  tmp = 1
  for p in edgePairs:
    currentBits[ (p[0],p[1]) ] = math.frexp(tmp)[1]-1
    bits = tmp + 2*tmp
    tmp = tmp*4
    [v0,v1,v2] = GetVerticesFromPair( p )
    faceidx = 10 + GetFaceFromVertices( [v0,v1,v2] )
    print >> genCode, '      if ( (comparisonBits & %d) == %d )' % (bits,bits)
    print >> genCode, '        {'
    print >> genCode, '        // Compute face point'
    print >> genCode, '        for ( i=0; i<this->PointDimension[3]; i++ )'
    print >> genCode, '          {'
    print >> genCode, '          permuted[%d][i] = (permuted[%d][i] + permuted[%d][i])*0.375 + permuted[%d][i]/4.;' % ( faceidx, v1, v2, v0 )
    print >> genCode, '          }'
    print >> genCode, '        }'
    faceidx += 1

def BeginCase( label ):
  global currentCaseCtr, currentCase, currentBits, caseLabel
  print >> genCode, '    case %d: // Ruprecht-Müller Case %s' % ( RMCases[ label ][0]+1, label )
  currentCase = label
  currentCaseCtr = 0
  currentBits = {}
  EdgesToSubdivide( RMCases[ label ][1] )
  caseLabel = 0
  print >> genCode, '      VTK_TESSELLATOR_INCR_CASE_COUNT(%d);' % RMCases[ currentCase ][0]

def EndCase():
  global currentCaseCtr, currentCase, currentBits, caseLabel
  print >> genCode, '      break;'
  currentCase = 'invalid'
  currentCaseCtr = -1
  currentBits = {}
  caseLabel = -1

def __Unconditional( tets, perm, sign, indent='', label='', alternates=() ):
  global currentCaseCtr, currentCase, currentBits, caseLabel

  if PermutationIndices[ perm ][1] != sign:
    if sign < 0:
      sgnstr = '-'
    else:
      sgnstr = '+'
    raise '%s %s (%d): Permutation(%s) and sign(%s) do not match!' % (currentCase, label, currentCaseCtr, str(perm), sgnstr)

  tc = []
  if label == '':
    label = currentCase + '_%d' % currentCaseCtr
  else:
    label = currentCase +  ', ' + label

  stuff = vtkTessCase( label )
  tc.append( stuff )
  stuff.InsertTet( tets )
  if QualityThang:
    altcount = 97
    for a in alternates:
      if label == '':
        altlabel = currentCase + '_%d%c' % (currentCaseCtr,altcount)
      else:
        altlabel = label + ', %c' % altcount
      stuff = vtkTessCase( altlabel )
      stuff.InsertTet( a )
      altcount += 1
      tc.append( stuff )
  currentCaseCtr += 1

  if len(tc) > 1 and QualityThang:
    altstring = '{ '
    for stuff in tc:
      altstring += str(stuff.Offset) + ', '
    altstring += '-1 }'
    print >> genCode, '      %s{' % indent
    print >> genCode, '      %s  int alternates[] = %s;' % (indent, altstring)
    print >> genCode, '      %s  outputTets.push( vtkStreamingTessellator::TetrahedralDecompositions + this->BestTets( alternates, permuted, %d, %d ) );' % ( indent, PermutationIndices[ perm ][0], sign )
    print >> genCode, '      %s}' % indent
  else:
    print >> genCode, '      %soutputTets.push( vtkStreamingTessellator::TetrahedralDecompositions + %d );' % (indent, tc[0].Offset)
  print >> genCode, '      %soutputPerm.push( vtkStreamingTessellator::PermutationsFromIndex[%d] );' % (indent, PermutationIndices[ perm ][0])
  print >> genCode, '      %soutputSign.push( %d );' % (indent, sign)
  print >> genCode, '      %sVTK_TESSELLATOR_INCR_SUBCASE_COUNT(%d,%d);' % (indent,RMCases[ currentCase ][0],caseLabel)
  caseLabel += 1

def __Permuted( perm, sign, source, indent, label ):
  global currentCaseCtr, currentCase, currentBits, caseLabel
  if PermutationIndices[ perm ][1] != sign:
    if sign < 0:
      sgnstr = '-'
    else:
      sgnstr = '+'
    raise '%s %s (%d): Permutation(%s) and sign(%s) do not match!' % (currentCase, label, currentCaseCtr, str(perm), sgnstr)

  # Create a list of all the possible tessellations
  tc = []
  stuff = vtkTessCase.GetOffset( currentCase + ', ' + source )
  tc.append( stuff )
  if QualityThang:
    altcount = 97
    stuff = vtkTessCase.GetOffset( currentCase + ', ' + source + ', %c' % altcount )
    while stuff > 0:
      tc.append( stuff )
      altcount += 1
      stuff = vtkTessCase.GetOffset( currentCase + ', ' + source + ', %c' % altcount )

  if len(tc) > 1 and QualityThang:
    altstring = str( tc ).replace( '[', '{' ).replace( ']', ', -1 }' )
    print >> genCode, '      %s{' % indent
    print >> genCode, '      %s  int alternates[] = %s;' % (indent, altstring)
    print >> genCode, '      %s  outputTets.push( vtkStreamingTessellator::TetrahedralDecompositions + this->BestTets( alternates, permuted, %d, %d ) );' % ( indent, PermutationIndices[ perm ][0], sign )
    print >> genCode, '      %s}' % indent
  else:
    print >> genCode, '      %soutputTets.push( vtkStreamingTessellator::TetrahedralDecompositions + %d );' % (indent, tc[0])
  print >> genCode, '      %soutputPerm.push( vtkStreamingTessellator::PermutationsFromIndex[%d] );' % (indent, PermutationIndices[ perm ][0])
  print >> genCode, '      %soutputSign.push( %d );' % (indent, sign)
  print >> genCode, '      %sVTK_TESSELLATOR_INCR_SUBCASE_COUNT(%d,%d);' % (indent,RMCases[ currentCase ][0],caseLabel)
  caseLabel += 1

def __BeginSubcase():
  print >> genCode, '      switch (comparisonBits)'
  print >> genCode, '        {'

def __EndSubcase():
  print >> genCode, '        }'

def __SubCase( ctxt, tets, sgn, alternates=() ):
  global currentCaseCtr, currentCase, currentBits, caseLabel
  code = GetBitcodeFromConditional( ctxt, currentBits )
  print >> genCode, '        case %d: // %s' % ( code, ctxt )
  if sgn > 0:
    __Unconditional( tets, (0,1,2,3), +1, '    ', ctxt, alternates )
  else:
    __Unconditional( tets, (1,0,2,3), -1, '    ', ctxt, alternates )
  print >> genCode, '      %sbreak;' % '    '

def __PrmCase( ctxt, csrc, perm, sgn ):
  global currentCaseCtr, currentCase, currentBits, caseLabel
  code = GetBitcodeFromConditional( ctxt, currentBits )
  print >> genCode, '        case %d: // %s' % ( code, ctxt )
  __Permuted( perm, sgn, csrc, '    ', ctxt )
  print >> genCode, '      %sbreak;' % '    '

print >> genCode, \
"""/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/* Do not edit this file! It was generated by hand. Mostly.
 * Edit vtkStreamingTessellatorGenerator.py instead.
 */

#include "vtkObjectFactory.h"

#include "vtkStreamingTessellator.h"
#include "vtkEdgeSubdivisionCriterion.h"
"""

if QualityThang:
  print >> genCode, """
#include "vtkMeshQuality.h"
#include "vtkPoints.h"
#include "vtkTetra.h"

// how's this for avoiding namespace conflicts?! 8-)
static vtkTetra* argyle = 0;
static vtkPoints* goCallTheCops;
"""

print >> genCode, """
#undef UGLY_ASPECT_RATIO_HACK
#undef DBG_MIDPTS

#include <stack>
#include <algorithm>

#ifdef PARAVIEW_DEBUG_TESSELLATOR
#  define VTK_TESSELLATOR_INCR_CASE_COUNT(cs) this->CaseCounts[cs]++
#  define VTK_TESSELLATOR_INCR_SUBCASE_COUNT(cs,sc) this->SubcaseCounts[cs][sc]++
#else // PARAVIEW_DEBUG_TESSELLATOR
#  define VTK_TESSELLATOR_INCR_CASE_COUNT(cs)
#  define VTK_TESSELLATOR_INCR_SUBCASE_COUNT(cs,sc)
#endif // PARAVIEW_DEBUG_TESSELLATOR

static void DefaultFacet3Callback(
  const double* a, const double* b, const double* c, const double* d,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  (void)a;
  (void)b;
  (void)c;
  (void)d;
  (void)pd;
}

static void DefaultFacet2Callback(
  const double* a, const double* b, const double* c,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  (void)a;
  (void)b;
  (void)c;
  (void)pd;
}

static void DefaultFacet1Callback(
  const double* a, const double* b,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  (void)a;
  (void)b;
  (void)pd;
}

static void DefaultFacet0Callback(
  const double* a,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  (void)a;
  (void)pd;
}

vtkStandardNewMacro(vtkStreamingTessellator);

void vtkStreamingTessellator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "PointDimension:       "
     << this->PointDimension[1] << " " << this->PointDimension[2] << " " << this->PointDimension[3] << endl;
  os << indent << "EmbeddingDimension:   "
     << this->EmbeddingDimension[1] << " " << this->EmbeddingDimension[2] << " " << this->EmbeddingDimension[3] << endl;
  os << indent << "PrivateData:          " << this->PrivateData << endl;
  os << indent << "ConstPrivateData:     " << this->ConstPrivateData << endl;
  os << indent << "SubdivisionAlgorithm: " << this->Algorithm << endl;
  os << indent << "VertexCallback:       " << this->Callback0 << endl;
  os << indent << "EdgeCallback:         " << this->Callback1 << endl;
  os << indent << "TriangleCallback:     " << this->Callback2 << endl;
  os << indent << "TetrahedronCallback:  " << this->Callback3 << endl;
#ifdef PARAVIEW_DEBUG_TESSELLATOR
  os << indent << "CaseCounts:           " << this->CaseCounts << endl;
  os << indent << "SubcaseCounts:        " << this->SubcaseCounts << endl;
#endif // PARAVIEW_DEBUG_TESSELLATOR
}

vtkStreamingTessellator::vtkStreamingTessellator()
{
  this->PrivateData = 0;
  this->ConstPrivateData = 0;
  this->Algorithm = 0;
  this->Callback0 = DefaultFacet0Callback;
  this->Callback1 = DefaultFacet1Callback;
  this->Callback2 = DefaultFacet2Callback;
  this->Callback3 = DefaultFacet3Callback;
  this->MaximumNumberOfSubdivisions = 3;
  for ( int i=0; i<4; ++i )
    {
    this->EmbeddingDimension[i] = i;
    this->PointDimension[i] = i+3; // By default, FieldSize = 0
    }"""
if QualityThang:
  print >> genCode, """
  if ( ! argyle )
    {
    argyle = vtkTetra::New();
    goCallTheCops = argyle->GetPoints();
    }"""
print >> genCode, """
}

vtkStreamingTessellator::~vtkStreamingTessellator()
{
  if ( this->Algorithm )
    this->Algorithm->UnRegister( this );
}

void vtkStreamingTessellator::SetEmbeddingDimension( int k, int d )
{
  if ( d > 8 )
    {
    vtkErrorMacro( "Embedding dimension may not be > 8. (You asked for " << d << "). Truncating to 8." );
    d = 8;
    }

  if ( k == 0 || k < -1 || k >= 4 )
    {
    vtkWarningMacro( "Invalid argument k=" << k );
    return;
    }

  if ( k < 0 )
    {
    for ( k=0; k<4; k++ )
      if ( this->EmbeddingDimension[k] != d )
        {
        this->PointDimension[k] += d - this->EmbeddingDimension[k] ;
        this->EmbeddingDimension[k] = d;
        this->Modified();
        }
    return;
    }
  if ( this->EmbeddingDimension[k] != d )
    {
    this->PointDimension[k] += d - this->EmbeddingDimension[k] ;
    this->EmbeddingDimension[k] = d;
    this->Modified();
    }
}

void vtkStreamingTessellator::SetFieldSize( int k, int s )
{
  if ( s > vtkStreamingTessellator::MaxFieldSize )
    {
    vtkErrorMacro( "Embedding dimension may not be > " << MaxFieldSize << ". (You asked for " << s << "). Truncating to " << MaxFieldSize );
    s = vtkStreamingTessellator::MaxFieldSize;
    }

  if ( k == 0 || k < -1 || k >= 4 )
    {
    vtkWarningMacro( "Invalid argument k=" << k );
    return;
    }

  if ( k < 0 )
    {
    // Use field size for all facet types (point, line, triangle, tet, ...)
    for ( k=0; k<4; k++ )
      if ( this->PointDimension[k] != s + this->EmbeddingDimension[k] + 3 )
        {
        this->PointDimension[k] = s + this->EmbeddingDimension[k] + 3;
        this->Modified();
        }
    return;
    }

  if ( this->PointDimension[k] != s + this->EmbeddingDimension[k] + 3 )
    {
    this->PointDimension[k] = s + this->EmbeddingDimension[k] + 3;
    this->Modified();
    }
}

void vtkStreamingTessellator::SetMaximumNumberOfSubdivisions( int num_subdiv_in )
{
  if ( this->MaximumNumberOfSubdivisions == num_subdiv_in )
    return;

  if ( num_subdiv_in < 0 )
    {
    vtkErrorMacro( "MaximumNumberOfSubdivisions must be 0 or greater (you requested " << num_subdiv_in << ")" );
    return;
    }

  this->MaximumNumberOfSubdivisions = num_subdiv_in;
  this->Modified();
}

void vtkStreamingTessellator::SetTetrahedronCallback( TetrahedronProcessorFunction f )
{
  this->Callback3 = f ? f : DefaultFacet3Callback;
}

vtkStreamingTessellator::TetrahedronProcessorFunction vtkStreamingTessellator::GetTetrahedronCallback() const
{
  return this->Callback3;
}

void vtkStreamingTessellator::SetTriangleCallback( TriangleProcessorFunction f )
{
  this->Callback2 = f ? f : DefaultFacet2Callback;
}

vtkStreamingTessellator::TriangleProcessorFunction vtkStreamingTessellator::GetTriangleCallback() const
{
  return this->Callback2;
}

void vtkStreamingTessellator::SetEdgeCallback( EdgeProcessorFunction f )
{
  this->Callback1 = f ? f : DefaultFacet1Callback;
}

vtkStreamingTessellator::EdgeProcessorFunction vtkStreamingTessellator::GetEdgeCallback() const
{
  return this->Callback1;
}

void vtkStreamingTessellator::SetVertexCallback( VertexProcessorFunction f )
{
  this->Callback0 = f ? f : DefaultFacet0Callback;
}

vtkStreamingTessellator::VertexProcessorFunction vtkStreamingTessellator::GetVertexCallback() const
{
  return this->Callback0;
}

void vtkStreamingTessellator::SetPrivateData( void* Private )
{
  this->PrivateData = Private;
}

void* vtkStreamingTessellator::GetPrivateData() const
{
  return this->PrivateData;
}

void vtkStreamingTessellator::SetConstPrivateData( const void* ConstPrivate )
{
  this->ConstPrivateData = ConstPrivate;
}

const void* vtkStreamingTessellator::GetConstPrivateData() const
{
  return this->ConstPrivateData;
}

void vtkStreamingTessellator::SetSubdivisionAlgorithm( vtkEdgeSubdivisionCriterion* a )
{
  if ( a != this->Algorithm )
    {
    if ( this->Algorithm )
      this->Algorithm->UnRegister( this );

    this->Algorithm = a;
    this->Modified();

    if ( this->Algorithm )
      this->Algorithm->Register( this );
    }
}

vtkEdgeSubdivisionCriterion* vtkStreamingTessellator::GetSubdivisionAlgorithm()
{
  return this->Algorithm;
}

const vtkEdgeSubdivisionCriterion* vtkStreamingTessellator::GetSubdivisionAlgorithm() const
{
  return this->Algorithm;
}


// Returns true if || a0a1 || < || b0b1 ||
// We use this to test which triangulation has the best
// aspect ratio when there are 2 to choose from.
bool compareHopfCrossStringDist( const double* a0, const double* a1, const double* b0, const double* b1 )
{
  double SqMagA = 0.;
  double SqMagB = 0.;
  for (int i=0; i<3; i++)
    {
    double tmp;
    tmp = a0[i] - a1[i];
    SqMagA += tmp * tmp;
    tmp = b0[i] - b1[i];
    SqMagB += tmp * tmp;
    }
  return SqMagA < SqMagB;
}
"""

if QualityThang:
  print >> genCode, """
int vtkStreamingTessellator::BestTets( int* connOffsets, double** verts, int permOffset, int sgn ) const
{
  int bestOffset = -1;
  double bestQuality = 0.;
  double currQuality;

  while ( *connOffsets >= 0 )
    {
    int nTets = TetrahedralDecompositions[*connOffsets];
    vtkIdType* conn = &TetrahedralDecompositions[*connOffsets +1];
    int v;
    currQuality = 0.;
    for (v=0; v<nTets; ++v)
      {
      goCallTheCops->SetPoint( 0, verts[ vtkStreamingTessellator::PermutationsFromIndex[ permOffset ][ conn[sgn < 0 ? 1:0]] ] );
      goCallTheCops->SetPoint( 1, verts[ vtkStreamingTessellator::PermutationsFromIndex[ permOffset ][ conn[sgn < 0 ? 0:1]] ] );
      goCallTheCops->SetPoint( 2, verts[ vtkStreamingTessellator::PermutationsFromIndex[ permOffset ][ conn[2]] ] );
      goCallTheCops->SetPoint( 3, verts[ vtkStreamingTessellator::PermutationsFromIndex[ permOffset ][ conn[3]] ] );
      currQuality += vtkMeshQuality::TetFrobeniusNorm( argyle );
      conn += 4;
      }
    currQuality /= nTets;
    std::cout << currQuality << " " << *connOffsets << " ";
    if ( bestQuality > currQuality || bestOffset < 0 )
      {
      bestQuality = currQuality;
      bestOffset = *connOffsets;
      }
      ++connOffsets;
    }
    std::cout << "Choose " << bestOffset << "\\n";
  return bestOffset;
}
"""
else:
  print >> genCode, """
int vtkStreamingTessellator::BestTets( int* vtkNotUsed(connOffsets), double** vtkNotUsed(verts), int vtkNotUsed(permOffset), int vtkNotUsed(sgn) ) const
{
  // Re-run vtkStreamingTessellatorGenerator.py with QualityThang=1
  // to get this implemented (along with on-the-fly quality improvement)
  return 1;
}
"""

print >> genCode, """
void vtkStreamingTessellator::AdaptivelySample0Facet( double* v0 ) const
{
  Callback0( v0, this->Algorithm, this->PrivateData, this->ConstPrivateData );
}

void vtkStreamingTessellator::AdaptivelySample1Facet( double* v0, double* v1, int maxDepth ) const
{
  int edgeCode = 0;

  double midpt0[11+vtkStreamingTessellator::MaxFieldSize];
  // make valgrind happy
  std::fill(midpt0,midpt0+this->PointDimension[1],0.);

  if ( maxDepth-- > 0 )
    {
      for ( int i=0; i<this->PointDimension[1]; i++ )
        midpt0[i] = (v0[i] + v1[i])/2.;

      if ( this->Algorithm->EvaluateEdge( v0, midpt0, v1, 3+this->EmbeddingDimension[1] ) )
        edgeCode += 1;
    }

  switch (edgeCode) {
    // No edges to subdivide
    case 0:
      Callback1( v0, v1, this->Algorithm, this->PrivateData, this->ConstPrivateData );
      break ;

      // One edge to subdivide
    case 1:
      this->AdaptivelySample1Facet( v0, midpt0, maxDepth );
      this->AdaptivelySample1Facet( midpt0, v1, maxDepth );
      break;
  }
}

void vtkStreamingTessellator::AdaptivelySample2Facet( double* v0, double* v1, double* v2, int maxDepth, int move ) const
{
  int edgeCode = 0;

  double midpt0[11+vtkStreamingTessellator::MaxFieldSize];
  double midpt1[11+vtkStreamingTessellator::MaxFieldSize];
  double midpt2[11+vtkStreamingTessellator::MaxFieldSize];

  // Make valgrind happy
  std::fill(midpt0,midpt0+this->PointDimension[2],0.);
  std::fill(midpt1,midpt1+this->PointDimension[2],0.);
  std::fill(midpt2,midpt2+this->PointDimension[2],0.);

  if ( maxDepth-- > 0 )
    {

    for ( int i=0; i<this->PointDimension[2]; i++ )
      {
      midpt0[i] = (v0[i] + v1[i])/2.;
      midpt1[i] = (v1[i] + v2[i])/2.;
      midpt2[i] = (v2[i] + v0[i])/2.;
      }

    if ( (move & 1) && Algorithm->EvaluateEdge( v0, midpt0, v1, 3+this->EmbeddingDimension[2] ) )
      edgeCode += 1;
    if ( (move & 2) && Algorithm->EvaluateEdge( v1, midpt1, v2, 3+this->EmbeddingDimension[2] ) )
      edgeCode += 2;
    if ( (move & 4) && Algorithm->EvaluateEdge( v2, midpt2, v0, 3+this->EmbeddingDimension[2] ) )
      edgeCode += 4;
#ifdef UGLY_ASPECT_RATIO_HACK
    double dist0=0.;
    double dist1=0.;
    double dist2=0.;
    double tmp;
    for ( int j=0; j<3; ++j )
      {
      tmp = v0[j] - v1[j];
      dist0 += tmp*tmp;
      tmp = v1[j] - v2[j];
      dist1 += tmp*tmp;
      tmp = v2[j] - v0[j];
      dist2 += tmp*tmp;
      }

    if ( edgeCode & 1 ) dist0 /= 2.;
    if ( edgeCode & 2 ) dist1 /= 2.;
    if ( edgeCode & 4 ) dist2 /= 2.;

#define MAR2 2.25
    if ( (!(edgeCode & 1)) && (move&1) && ((dist0/dist1 > MAR2) || (dist0/dist2 > MAR2)) )
      {
      edgeCode += 1;
      move &= 6;
      }
    if ( (!(edgeCode & 2)) && (move&2) && ((dist1/dist0 > MAR2) || (dist1/dist2 > MAR2)) )
      {
      edgeCode += 2;
      move &= 5;
      }
    if ( (!(edgeCode & 4)) && (move&4) && ((dist2/dist1 > MAR2) || (dist2/dist0 > MAR2)) )
      {
      edgeCode += 4;
      move &= 3;
      }
#endif // UGLY_ASPECT_RATIO_HACK
    }

#ifdef DBG_MIDPTS
  if ( maxDepth == 0 )
    {
    fprintf( stderr, "midpoint of v%d (%g %g %g/%g %g %g)-v%d (%g %g %g/%g %g %g) = (%g %g %g/%g %g %g)\\n",
      0, v0[0], v0[1], v0[2], v0[3], v0[4], v0[5],
      1, v1[0], v1[1], v1[2], v1[3], v1[4], v1[5],
         midpt0[0], midpt0[1], midpt0[2], midpt0[3], midpt0[4], midpt0[5]
    );

    fprintf( stderr, "midpoint of v%d (%g %g %g/%g %g %g)-v%d (%g %g %g/%g %g %g) = (%g %g %g/%g %g %g)\\n",
      1, v1[0], v1[1], v1[2], v1[3], v1[4], v1[5],
      2, v2[0], v2[1], v2[2], v2[3], v2[4], v2[5],
         midpt1[0], midpt1[1], midpt1[2], midpt1[3], midpt1[4], midpt1[5]
    );

    fprintf( stderr, "midpoint of v%d (%g %g %g/%g %g %g)-v%d (%g %g %g/%g %g %g) = (%g %g %g/%g %g %g)\\n\\n",
      2, v2[0], v2[1], v2[2], v2[3], v2[4], v2[5],
      0, v0[0], v0[1], v0[2], v0[3], v0[4], v0[5],
         midpt2[0], midpt2[1], midpt2[2], midpt2[3], midpt2[4], midpt2[5]
    );
    }
#endif // DBG_MIDPTS

  switch (edgeCode)
    {
    // No edges to subdivide
  case 0:
    Callback2( v0, v1, v2, this->Algorithm, this->PrivateData, this->ConstPrivateData );
    break ;

    // One edge to subdivide
  case 1:
    this->AdaptivelySample2Facet( v0, midpt0, v2, maxDepth, move | 2 );
    this->AdaptivelySample2Facet( midpt0, v1, v2, maxDepth, move | 4 );
    break;
  case 2:
    this->AdaptivelySample2Facet( v0, v1, midpt1, maxDepth, move | 4 );
    this->AdaptivelySample2Facet( v0, midpt1, v2, maxDepth, move | 1 );
    break;
  case 4:
    this->AdaptivelySample2Facet( v0, v1, midpt2, maxDepth, move | 2 );
    this->AdaptivelySample2Facet( midpt2, v1, v2, maxDepth, move | 1 );
    break;

    // Two edges to subdivide
  case 3:
    this->AdaptivelySample2Facet( midpt0, v1, midpt1, maxDepth, move | 4 );
    if ( compareHopfCrossStringDist( v2, midpt0, v0, midpt1 ) )
      {
      this->AdaptivelySample2Facet( midpt0, midpt1,   v2  , maxDepth, move | 5 );
      this->AdaptivelySample2Facet(   v0,   midpt0,   v2  , maxDepth, move | 2 );
      }
    else
      {
      this->AdaptivelySample2Facet(   v0  , midpt0, midpt1, maxDepth, move | 6 );
      this->AdaptivelySample2Facet(   v0,   midpt1,   v2  , maxDepth, move | 1 );
      }
    break;
  case 5:
    this->AdaptivelySample2Facet( v0, midpt0, midpt2, maxDepth, move | 2 );
    if ( compareHopfCrossStringDist( v2, midpt0, v1, midpt2 ) )
      {
      this->AdaptivelySample2Facet( midpt0,   v1,     v2  , maxDepth, move | 4 );
      this->AdaptivelySample2Facet( midpt2, midpt0,   v2  , maxDepth, move | 3 );
      }
    else
      {
      this->AdaptivelySample2Facet( midpt0,   v1,   midpt2, maxDepth, move | 6 );
      this->AdaptivelySample2Facet( midpt2,   v1,     v2,   maxDepth, move | 1 );
      }
    break;
  case 6:
    this->AdaptivelySample2Facet( midpt2, midpt1, v2, maxDepth, move | 1 );
    if ( compareHopfCrossStringDist( v0, midpt1, v1, midpt2 ) )
      {
      this->AdaptivelySample2Facet(   v0,   midpt1, midpt2, maxDepth, move | 3 );
      this->AdaptivelySample2Facet(   v0,     v1,   midpt1, maxDepth, move | 4 );
      }
    else
      {
      this->AdaptivelySample2Facet(   v0,     v1,   midpt2, maxDepth, move | 2 );
      this->AdaptivelySample2Facet( midpt2,   v1,   midpt1, maxDepth, move | 5 );
      }
    break;

    // Three edges to subdivide
  case 7:
    this->AdaptivelySample2Facet( midpt0, midpt1, midpt2, maxDepth, 7 );
    this->AdaptivelySample2Facet(   v0  , midpt0, midpt2, maxDepth, move | 2 );
    this->AdaptivelySample2Facet( midpt0,   v1  , midpt1, maxDepth, move | 4 );
    this->AdaptivelySample2Facet( midpt2, midpt1,   v2  , maxDepth, move | 1 );
    break;
    }
}

void vtkStreamingTessellator::AdaptivelySample3Facet( double* v0, double* v1, double* v2, double* v3, int maxDepth ) const
{
  int edgeCode = 0;

  double midpt0[11+vtkStreamingTessellator::MaxFieldSize];
  double midpt1[11+vtkStreamingTessellator::MaxFieldSize];
  double midpt2[11+vtkStreamingTessellator::MaxFieldSize];
  double midpt3[11+vtkStreamingTessellator::MaxFieldSize];
  double midpt4[11+vtkStreamingTessellator::MaxFieldSize];
  double midpt5[11+vtkStreamingTessellator::MaxFieldSize];
  double facept0[11+vtkStreamingTessellator::MaxFieldSize];
  double facept1[11+vtkStreamingTessellator::MaxFieldSize];
  double facept2[11+vtkStreamingTessellator::MaxFieldSize];
  double facept3[11+vtkStreamingTessellator::MaxFieldSize];

  // Make valgrind happy
  std::fill(midpt0,midpt0+this->PointDimension[3],0.);
  std::fill(midpt1,midpt1+this->PointDimension[3],0.);
  std::fill(midpt2,midpt2+this->PointDimension[3],0.);
  std::fill(midpt3,midpt3+this->PointDimension[3],0.);
  std::fill(midpt4,midpt4+this->PointDimension[3],0.);
  std::fill(midpt5,midpt5+this->PointDimension[3],0.);

  double edgeLength2[6];
  if ( maxDepth-- > 0 )
    {
    for ( int i=0; i<this->PointDimension[3]; i++ )
      {
      midpt0[i] = (v0[i] + v1[i])/2.;
      midpt1[i] = (v1[i] + v2[i])/2.;
      midpt2[i] = (v2[i] + v0[i])/2.;
      midpt3[i] = (v0[i] + v3[i])/2.;
      midpt4[i] = (v1[i] + v3[i])/2.;
      midpt5[i] = (v2[i] + v3[i])/2.;
      }

    if ( Algorithm->EvaluateEdge( v0, midpt0, v1, 3+this->EmbeddingDimension[3] ) )
      edgeCode |=  1;
    if ( Algorithm->EvaluateEdge( v1, midpt1, v2, 3+this->EmbeddingDimension[3] ) )
      edgeCode |=  2;
    if ( Algorithm->EvaluateEdge( v2, midpt2, v0, 3+this->EmbeddingDimension[3] ) )
      edgeCode |=  4;

    if ( Algorithm->EvaluateEdge( v0, midpt3, v3, 3+this->EmbeddingDimension[3] ) )
      edgeCode |=  8;
    if ( Algorithm->EvaluateEdge( v1, midpt4, v3, 3+this->EmbeddingDimension[3] ) )
      edgeCode |= 16;
    if ( Algorithm->EvaluateEdge( v2, midpt5, v3, 3+this->EmbeddingDimension[3] ) )
      edgeCode |= 32;

    edgeLength2[0] = edgeLength2[1] = edgeLength2[2] = edgeLength2[3]
      = edgeLength2[4] = edgeLength2[5] = 0;
    for ( int c=0; c<3; ++c )
      {
      double tmp;
      tmp = v1[c] - v0[c];
      edgeLength2[0] += tmp*tmp;
      tmp = v2[c] - v1[c];
      edgeLength2[1] += tmp*tmp;
      tmp = v2[c] - v0[c];
      edgeLength2[2] += tmp*tmp;
      tmp = v3[c] - v0[c];
      edgeLength2[3] += tmp*tmp;
      tmp = v3[c] - v1[c];
      edgeLength2[4] += tmp*tmp;
      tmp = v3[c] - v2[c];
      edgeLength2[5] += tmp*tmp;
      }
    }

  if ( edgeCode == 0 )
    {
    // No edges to subdivide
    Callback3( v0, v1, v2, v3, this->Algorithm, this->PrivateData, this->ConstPrivateData );
    }
  else
    {
    // Do the subdivision
    double* vertices[10] =
    {
      v0, v1, v2, v3,     midpt0, midpt1, midpt2, midpt3, midpt4, midpt5
    };

    // Generate tetrahedra that are compatible except when edge
    // lengths are equal on indeterminately subdivided faces.
    double* permuted[14];
    double permlen[6]; // permuted edge lengths
    int C = vtkStreamingTessellator::EdgeCodesToCaseCodesPlusPermutation[ edgeCode ][0];
    int P = vtkStreamingTessellator::EdgeCodesToCaseCodesPlusPermutation[ edgeCode ][1];
    int i;

    // 1. Permute the tetrahedron into our canonical configuration
    for ( i=0; i<4; ++i )
      {
      permuted[i] = vertices[ vtkStreamingTessellator::PermutationsFromIndex[P][i] ];
      }
    for ( i=4; i<10; ++i )
      {
      // permute the edge lengths, too
      permuted[i] = vertices[ vtkStreamingTessellator::PermutationsFromIndex[P][i] ];
      permlen[i-4]  = edgeLength2[ vtkStreamingTessellator::PermutationsFromIndex[P][i] - 4 ];
      }
    // Add our local (heap) storage for face points to the list.
    permuted[10] = facept0;
    permuted[11] = facept1;
    permuted[12] = facept2;
    permuted[13] = facept3;

    int comparisonBits;
    std::stack<vtkIdType*> outputTets;
    std::stack<vtkIdType*> outputPerm;
    std::stack<int>        outputSign;

    // cout << "Case " << C << "  Permutation " << P << endl;
    // 2. Generate tetrahedra based on the configuration.
    //    Note that case 0 is handled above (edgeCode == 0).
    switch (C)
      {
"""

# ============================================================================
# Code generation
#  Each BeginCase() ... EndCase() marks the start of a Ruprect-Muller case.
#  Inside a Begin/EndCase() pair, you may have
#    - __Unconditional( tetList, permutation, sign ): unconditionally add
#          the tetrahedra specified as a list of integer vertices in tetList
#          to the output. permutation is an optional vertex reordering to
#          be applied to the output vertex indices, and sign is +1 or -1
#          depending on whether permutation is a positive or negative
#          rearrangement of the vertices. The vertex indices used in tetList
#          are
#            0-3 for the corners of the current input tetrahedron
#            4-9 for any mid-edge nodes created when an edge must be
#                subdivided. 4 is always the node between input
#                vertices 0 and 1, 5 between 1 and 2, and so on (see
#                the EdgeVerts variable for a full list).
#            10-13 for any mid-face nodes created to resolve an
#                ambiguity. 10 always corresponds to a node on input
#                face 0, 11 on 1, and so on.
#    - __BeginSubCase() ... __EndSubCase()
#  Inside a subcase pair, you may have
#    - __SubCase( label, tetList, sign ): conditionally add the tetrahedra
#          of tetList to the output based on whether the edge length
#          relationships in label are true. The label is a text string
#          containing relationships between edge lengths. Each relationship
#          is written as an edge number, an equality or inequality symbol, and
#          a second edge number -- i.e., '0>1' or '2<4'. See the EdgeVerts
#          variable for the map between edge number and vertices. If multiple
#          comparisons are required, they may be separated by a comma.
#    - __PrmCase( label, subcaselabel, perm, sign ): conditionally add the
#          tetrahedra corresponding to the __SubCase specified by subcaselabel
#          variable -- but using the specified permutation -- when the edge
#          length relationships in label are met.
#  Note that permutations must be Python tuples not lists. Each permutation is
# a tuple of 4 numbers specifying a new order for the original 4 input vertices.

BeginCase( '1' )
__Unconditional( [ 0, 4, 2, 3,   4, 1, 2, 3 ], (), +1 )
EndCase()

BeginCase( '2a' )
__Unconditional( [ 3, 4, 5, 1 ], (), +1 )
__BeginSubcase()
__SubCase( '0>1', [  0, 4, 2, 3,     4, 5, 2, 3 ], +1 )
__SubCase( '1>0', [  0, 4, 5, 3,     0, 5, 2, 3 ], +1 )
__SubCase( '0=1', [ 10, 3, 0, 4,    10, 3, 4, 5,   10, 3, 5, 2,   10, 3, 2, 0 ], +1 )
__EndSubcase()
EndCase()

BeginCase( '2b' )
__Unconditional( [ 0, 4, 9, 3,   4, 1, 9, 3,   0, 4, 2, 9,    4, 1, 2, 9 ], (), +1 )
EndCase()

BeginCase( '3a' )
__Unconditional( [ 4, 7, 6, 0 ], (), +1 )
__BeginSubcase()
__SubCase( '0>2>3<0', [ 1, 3, 2, 4,   4, 6, 3, 2,   4, 6, 7, 3 ], +1 )
__PrmCase( '2>3>0<2', '0>2>3<0', ( 0, 2, 3, 1 ), +1 )
__PrmCase( '3>0>2<3', '0>2>3<0', ( 0, 3, 1, 2 ), +1 )
__PrmCase( '3>2>0<3', '0>2>3<0', ( 0, 3, 2, 1 ), -1 )
__PrmCase( '2>0>3<2', '0>2>3<0', ( 0, 2, 1, 3 ), -1 )
__PrmCase( '0>3>2<0', '0>2>3<0', ( 0, 1, 3, 2 ), -1 )
# 3a-alpha
__SubCase( '0=2>3<0', [  4, 6, 7, 3,  10, 1, 2, 3, \
                      10, 2, 6, 3,  10, 6, 4, 3, \
                      10, 4, 1, 3 ], +1 )
__PrmCase( '2=3>0<2', '0=2>3<0', ( 0, 2, 3, 1 ), +1 )
__PrmCase( '0=3>2<0', '0=2>3<0', ( 0, 3, 1, 2 ), +1 )
# 3a-beta
__SubCase( '3>0=2<3', [  1, 3, 2, 7,  10, 1, 2, 7, \
                      10, 2, 6, 7,  10, 6, 4, 7, \
                      10, 4, 1, 7 ], +1 )
__PrmCase( '0>2=3<0', '3>0=2<3', ( 0, 2, 3, 1 ), +1 )
__PrmCase( '2>0=3<2', '3>0=2<3', ( 0, 3, 1, 2 ), +1 )
# 3a-gamma
__SubCase( '0=2=3=0', [  2, 6,10,13,   3, 7,13,11, \
                         4, 1,10,11,  11, 6,10, 4, \
                        11, 6,13,10,  11, 6, 7,13, \
                        11, 6, 4, 7,   2,10,11,13, \
                         1,10,11, 2,   2,11, 3,13,  \
                         3, 2, 1,11  ], +1 )
__EndSubcase()
EndCase()

BeginCase( '3b' )
__Unconditional( [ 0, 7, 4, 2,   4, 7, 8, 2,   4, 8, 1, 2,   7, 3, 8, 2 ], (), +1 )
EndCase()

BeginCase( '3c' )
__BeginSubcase()
__SubCase( '0>1,0>3', [ 4, 2, 7, 5,   4, 2, 0, 7, \
                        4, 3, 1, 5,   4, 3, 5, 7, \
                        3, 5, 7, 2 ], +1 )
__SubCase( '1>0,3>0', [ 0, 5, 2, 7,   0, 5, 7, 4, \
                        7, 1, 4, 5,   7, 1, 5, 3, \
                        3, 5, 7, 2 ], +1 )
__SubCase( '0>1,3>0', [ 4, 2, 7, 5,   4, 2, 0, 7, \
                        7, 1, 4, 5,   7, 1, 5, 3, \
                        3, 5, 7, 2 ], +1 )
__SubCase( '1>0,0>3', [ 0, 5, 2, 7,   0, 5, 7, 4, \
                        4, 3, 1, 5,   4, 3, 5, 7, \
                        3, 5, 7, 2 ], +1 )
# 3c-alpha
__SubCase( '0=1,0>3', [ 4, 1, 5, 3,  10, 0 ,4, 7, \
                     10, 2, 0, 7,  10, 7, 4, 3, \
                     10, 2, 7, 3,  10, 5, 2, 3, \
                     10, 4, 5, 3 ], +1 )
__PrmCase( '0=3,0>1', '0=1,0>3', ( 1, 0, 3, 2 ), +1 )
# 3c-beta
__SubCase( '3>0,0=1', [ 7, 1, 5, 3,   7, 5 ,2, 3, \
                     10, 0, 4, 7,  10, 2, 0, 7, \
                     10, 5, 2, 7,  10, 4, 5, 7, \
                      1, 5, 4, 7 ], +1 )
__PrmCase( '1>0,0=3', '3>0,0=1', ( 1, 0, 3, 2 ), +1 )
# 3c-gamma
__SubCase( '0=1,0=3', [ 4, 1, 5,11,  11, 1 ,5, 3, \
                     10, 0, 4, 7,  10, 2, 0, 7, \
                     10, 5, 2, 3,  10, 2, 7, 3, \
                     10, 7, 4,11,  10, 7,11, 3, \
                     10, 4, 5,11,  10,11, 5, 3 ], +1 )
__EndSubcase()
EndCase()

BeginCase( '3d' )
__BeginSubcase()
__SubCase( '0>4,0>2', [ 4, 3, 6, 0,   4, 3, 8, 6, \
                        4, 2, 8, 1,   4, 2, 6, 8, \
                        2, 3, 6, 8 ], +1 )
__SubCase( '4>0,2>0', [ 8, 0, 6, 4,   8, 0, 3, 6, \
                        6, 1, 8, 4,   6, 1, 2, 8, \
                        2, 3, 6, 8 ], +1 )
__SubCase( '0>4,2>0', [ 4, 3, 6, 0,   4, 3, 8, 6, \
                        6, 1, 8, 4,   6, 1, 2, 8, \
                        2, 3, 6, 8 ], +1 )
__SubCase( '4>0,0>2', [ 8, 0, 6, 4,   8, 0, 3, 6, \
                        4, 2, 8, 1,   4, 2, 6, 8, \
                        2, 3, 6, 8 ], +1 )
# 3d-alpha
__SubCase( '0=4,0>2', [ 4, 1, 2, 8,  11, 4 ,0, 6, \
                     11, 0, 3, 6,  11, 2, 4, 6, \
                     11, 3, 2, 6,  11, 3, 8, 2, \
                     11, 8, 4, 2 ], +1 )
__PrmCase( '0=2,0>4', '0=4,0>2', ( 1, 0, 3, 2 ), +1 )
# 3d-beta
__SubCase( '2>0,0=4', [ 6, 2, 8, 1,   6, 8 ,2, 3, \
                     11, 4, 0, 6,  11, 0, 3, 6, \
                      8,11, 3, 6,   8, 4,11, 6, \
                      1, 6, 4, 8 ], +1 )
__PrmCase( '4>0,0=2', '2>0,0=4', ( 1, 0, 3, 2 ), +1 )
# 3d-gamma
__SubCase( '0=4,0=2', [ 4, 1,10, 8,  10, 2 ,8, 1, \
                     11, 4, 0, 6,  11, 0, 3, 6, \
                     11, 3, 8, 2,  11, 3, 2, 6, \
                     11,10, 4, 6,  11,10, 6, 2, \
                      8, 4,11,10,  11,10, 2, 8 ], +1 )
__EndSubcase()
EndCase()

BeginCase( '4a' )
__Unconditional( [ 7, 8, 9, 3,   7, 9, 8, 6 ], (), +1 )
__BeginSubcase()
__SubCase( '5>4>3', [ 8, 0, 6, 1,   8, 0, 7, 6, \
                      9, 1, 6, 2,   9, 1, 8, 6 ], +1 )
__PrmCase( '3>4>5', '5>4>3', ( 2, 1, 0, 3 ), -1 )
__SubCase( '3<4>5', [ 8, 0, 6, 1,   8, 0, 7, 6, \
                      8, 2, 6, 9,   8, 2, 1, 6 ], +1 )
__SubCase( '3>4<5', [ 6, 9, 8, 1,   6, 9, 1, 2, \
                      6, 7, 0, 1,   6, 7, 1, 8 ], +1 )
# 4a-alpha
__SubCase( '3=4>5', [ 6, 7, 0,11,   6, 0 ,1,11, \
                      6, 7,11, 8,   6,11, 1, 8, \
                      1, 2, 6, 8,   2, 6, 8, 9 ], +1 )
__PrmCase( '4=5,4>3', '3=4>5', ( 2, 1, 0, 3 ), -1 )
# 4a-beta
__SubCase( '5>4,3=4', [ 6, 7, 0,11,   6, 0 ,1,11, \
                      6, 7,11, 8,   6,11, 1, 8, \
                      1, 2, 6, 9,   1, 6, 8, 9 ], +1 )
__PrmCase( '3>4=5', '5>4,3=4', ( 2, 1, 0, 3 ), -1 )
# 4a-gamma
__SubCase( '3=4=5', [ 6, 7, 0,11,   6, 0 ,1,11, \
                      6, 7,11, 8,   6,11, 1, 8, \
                      6, 1, 2,12,   6, 2, 9,12, \
                      6, 9, 8,12,   6, 8, 1,12 ], +1 )
__EndSubcase()
EndCase()

BeginCase( '4b' )
__BeginSubcase()
# Unambiguous cases
# 4bu-alpha: diagonal 6-8 required
__SubCase( '2>1,3>2,4>3,4>1', [  6, 8, 1, 5,    6, 8, 0, 1, \
                                 6, 8, 7, 0,    6, 8, 2, 7, \
                                 7, 8, 2, 3,    6, 8, 5, 2 ], +1 )
__PrmCase( '1>2,3>2,3>4,4>1', '2>1,3>2,4>3,4>1', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '1>2,2>3,3>4,1>4', '2>1,3>2,4>3,4>1', ( 0, 1, 3, 2 ), -1 )
__PrmCase( '2>1,2>3,4>3,1>4', '2>1,3>2,4>3,4>1', ( 1, 0, 3, 2 ), +1 )
__PrmCase( '1>2,2>3,4>3,4>1', '2>1,3>2,4>3,4>1', ( 2, 3, 0, 1 ), +1 )
__PrmCase( '2>1,2>3,3>4,4>1', '2>1,3>2,4>3,4>1', ( 3, 2, 1, 0 ), +1 )
__PrmCase( '2>1,3>2,3>4,1>4', '2>1,3>2,4>3,4>1', ( 2, 3, 1, 0 ), -1 )
__PrmCase( '1>2,3>2,4>3,1>4', '2>1,3>2,4>3,4>1', ( 3, 2, 0, 1 ), -1 )
# 4bu-beta: diagonal arbitrary, 6-8 used, 5-7 alternate
__SubCase( '2>1,3>2,3>4,4>1', [  6, 8, 1, 5,    6, 8, 7, 1, \
                                 6, 7, 0, 1,    8, 7, 3, 2, \
                                 6, 8, 5, 2,    6, 8, 2, 7 ], +1, \
                            ( [  7, 8, 1, 5,    6, 5, 7, 1, \
                                 6, 7, 0, 1,    8, 7, 3, 2, \
                                 7, 8, 5, 2,    6, 5, 2, 7 ], ) \
                              )
__PrmCase( '1>2,3>2,4>3,4>1', '2>1,3>2,3>4,4>1', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '1>2,2>3,4>3,1>4', '2>1,3>2,3>4,4>1', ( 1, 0, 3, 2 ), +1 )
__PrmCase( '2>1,2>3,3>4,1>4', '2>1,3>2,3>4,4>1', ( 0, 1, 3, 2 ), -1 )
# These represent distinct permutations but they result in the same
# edge comparisons being made, so they generate identical case statements.
#__PrmCase( '1>2,3>2,4>3,4>1', '2>1,3>2,3>4,4>1', ( 3, 2, 0, 1 ), -1 )
#__PrmCase( '2>1,3>2,3>4,4>1', '2>1,3>2,3>4,4>1', ( 3, 2, 1, 0 ), +1 )
#__PrmCase( '2>1,2>3,3>4,1>4', '2>1,3>2,3>4,4>1', ( 2, 3, 1, 0 ), -1 )
#__PrmCase( '1>2,2>3,4>3,1>4', '2>1,3>2,3>4,4>1', ( 2, 3, 0, 1 ), +1 )
# 4bu-gamma: diagonal 5-7 required
__SubCase( '2>1,2>3,4>3,4>1', [  6, 8, 5, 2,    6, 8, 2, 3, \
                                 6, 8, 3, 7,    6, 8, 7, 0, \
                                 6, 8, 0, 1,    6, 8, 1, 5 ], +1 )
#__PrmCase( '2>1,2>3,4>3,4>1', '2>1,2>3,4>3,4>1', ( 2, 3, 0, 1 ), +1 )
__PrmCase( '1>2,3>2,3>4,1>4', '2>1,2>3,4>3,4>1', ( 0, 1, 3, 2 ), -1 )
#__PrmCase( '1>2,3>2,3>4,1>4', '2>1,2>3,4>3,4>1', ( 1, 0, 2, 3 ), -1 )
#__PrmCase( '1>2,3>2,3>4,1>4', '2>1,2>3,4>3,4>1', ( 2, 3, 1, 0 ), -1 )
#__PrmCase( '2>1,2>3,4>3,4>1', '2>1,2>3,4>3,4>1', ( 3, 2, 1, 0 ), +1 )
#__PrmCase( '2>1,2>3,4>3,4>1', '2>1,2>3,4>3,4>1', ( 2, 3, 0, 1 ), +1 )
#__PrmCase( '1>2,3>2,3>4,1>4', '2>1,2>3,4>3,4>1', ( 3, 2, 0, 1 ), -1 )
# Ambiguous cases
# 4b-alpha
__SubCase( '1=2,3>2,3>4,4>1', [ 10, 6, 0, 7,   10, 1, 5, 8, \
                                10, 0, 1, 7,   10, 7, 1, 8, \
                                 6, 7,10, 8,    6,10, 5, 8, \
                                 6, 2, 7, 8,    6, 5, 2, 8, \
                                 7, 8, 2, 3 ], +1 )
__PrmCase( '1=2,3>2,4>3,4>1', '1=2,3>2,3>4,4>1', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '2>1,2>3,3=4,1>4', '1=2,3>2,3>4,4>1', ( 0, 1, 3, 2 ), -1 )
__PrmCase( '1>2,2>3,3=4,1>4', '1=2,3>2,3>4,4>1', ( 1, 0, 3, 2 ), +1 )
__PrmCase( '1>2,2=3,4>3,1>4', '1=2,3>2,3>4,4>1', ( 2, 3, 0, 1 ), +1 )
__PrmCase( '2>1,3>2,3>4,1=4', '1=2,3>2,3>4,4>1', ( 3, 2, 1, 0 ), +1 )
__PrmCase( '2>1,2>3,3>4,1=4', '1=2,3>2,3>4,4>1', ( 2, 3, 1, 0 ), -1 )
__PrmCase( '1>2,2=3,4>3,4>1', '1=2,3>2,3>4,4>1', ( 3, 2, 0, 1 ), -1 )
# 4b-beta
__SubCase( '1=2,2>3,4>3,1>4', [ 10, 6, 0, 7,   10, 1, 5, 8, \
                                10, 0, 1, 8,   10, 7, 0, 8, \
                                 6, 7,10, 8,    6,10, 5, 8, \
                                 6, 3, 7, 8,    6, 5, 3, 8, \
                                 6, 5, 2, 3 ], +1 )
__PrmCase( '1=2,2>3,3>4,1>4', '1=2,2>3,4>3,1>4', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '1>2,3>2,3=4,4>1', '1=2,2>3,4>3,1>4', ( 0, 1, 3, 2 ), -1 )
__PrmCase( '2>1,3>2,3=4,4>1', '1=2,2>3,4>3,1>4', ( 1, 0, 3, 2 ), +1 )
__PrmCase( '2>1,2=3,3>4,4>1', '1=2,2>3,4>3,1>4', ( 2, 3, 0, 1 ), +1 )
__PrmCase( '1>2,2>3,4>3,1=4', '1=2,2>3,4>3,1>4', ( 3, 2, 1, 0 ), +1 )
__PrmCase( '1>2,3>2,4>3,1=4', '1=2,2>3,4>3,1>4', ( 2, 3, 1, 0 ), -1 )
__PrmCase( '2>1,2=3,3>4,1>4', '1=2,2>3,4>3,1>4', ( 3, 2, 0, 1 ), -1 )
# 4b-gamma
__SubCase( '1=2,2>3,4>3,4>1', [ 10, 6, 0, 7,   10, 1, 5, 8, \
                                10, 0, 1, 8,   10, 7, 0, 8, \
                                 6, 7,10, 8,    6,10, 5, 8, \
                                 6, 3, 7, 8,    6, 5, 2, 8, \
                                 6, 2, 3, 8 ], +1 )
__PrmCase( '1=2,3>2,3>4,1>4', '1=2,2>3,4>3,4>1', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '1>2,3>2,3=4,1>4', '1=2,2>3,4>3,4>1', ( 0, 1, 3, 2 ), -1 )
__PrmCase( '2>1,2>3,3=4,4>1', '1=2,2>3,4>3,4>1', ( 1, 0, 3, 2 ), +1 )
__PrmCase( '2>1,2=3,4>3,4>1', '1=2,2>3,4>3,4>1', ( 2, 3, 0, 1 ), +1 )
__PrmCase( '2>1,2>3,4>3,1=4', '1=2,2>3,4>3,4>1', ( 3, 2, 1, 0 ), +1 )
__PrmCase( '1>2,3>2,3>4,1=4', '1=2,2>3,4>3,4>1', ( 2, 3, 1, 0 ), -1 )
__PrmCase( '1>2,2=3,3>4,1>4', '1=2,2>3,4>3,4>1', ( 3, 2, 0, 1 ), -1 )
# 4b-delta
__SubCase( '1=2,3>2,3=4,4>1', [ 10, 6, 0, 7,   10, 1, 5, 8, \
                        10, 0, 1,11,   10,11, 1, 8, \
                        10, 0,11, 7,   10, 7,11, 8, \
                         6, 7,10, 8,    6,10, 5, 8, \
                         6, 2, 7, 8,    6, 5, 2, 8, \
                         7, 8, 2, 3 ], +1 )
__PrmCase( '1=2>3=4,1>4', '1=2,3>2,3=4,4>1', ( 0, 1, 3, 2 ), -1 )
__PrmCase( '1>2,2=3,4>3,1=4', '1=2,3>2,3=4,4>1', ( 2, 3, 0, 1 ), +1 )
__PrmCase( '2>1,2=3,3>4,1=4', '1=2,3>2,3=4,4>1', ( 3, 2, 1, 0 ), +1 )
# 4b-epsilon
__SubCase( '4>1=2=3,4>3', [ 10, 6, 0, 7,   10, 1, 5, 8, \
                        10, 0, 1, 8,   10, 7, 0, 8, \
                        13, 6, 2, 5,   13, 3, 7, 8, \
                        13, 2, 3, 8,   13, 2, 8, 5, \
                         6, 7,10, 8,    6,10, 5, 8, \
                         6,13, 7, 8,    6, 5,13, 8], +1 )
__PrmCase( '1=2,3>2,3>4,1=4', '4>1=2=3,4>3', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '1>2=3=4,1>4', '4>1=2=3,4>3', ( 0, 1, 3, 2 ), -1 )
__PrmCase( '2>1,2>3,3=4,1=4', '4>1=2=3,4>3', ( 1, 0, 3, 2 ), +1 )
# 4b-zeta
__SubCase( '1=2=3>4,1>4', [ 10, 6, 0, 7,   10, 1, 5, 8, \
                        10, 0, 1, 7,   10, 7, 1, 8, \
                        13, 6, 2, 5,   13, 3, 7, 8, \
                        13, 2, 3, 5,   13, 3, 8, 5, \
                         6, 7,10, 8,    6,10, 5, 8, \
                         6,13, 7, 8,    6, 5,13, 8], +1 )
__PrmCase( '1=2>3,4>3,1=4', '1=2=3>4,1>4', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '2>1,2=3=4>1', '1=2=3>4,1>4', ( 0, 1, 3, 2 ), -1 )
__PrmCase( '1>2,3>2,3=4=1', '1=2=3>4,1>4', ( 1, 0, 3, 2 ), +1 )
# 4b-eta
__SubCase( '1=2=3=4=1', [ 10, 6, 0, 7,   10, 1, 5, 8, \
                          10, 0, 1,11,   10,11, 1, 8, \
                          10, 0,11, 7,   10, 7,11, 8, \
                          13, 6, 2, 5,   13, 3, 7, 8, \
                          13, 2, 3,12,   13, 2,12, 5, \
                          13,12, 3, 8,   13,12, 5, 8, \
                           6, 7,10, 8,    6,10, 5, 8, \
                           6, 5,13, 8,    6,13, 7, 8 ], +1 )
__EndSubcase()
EndCase()

BeginCase( '5' )
__Unconditional( [ 7, 8, 9, 3,   6, 5, 2, 9 ], (), +1 )
__BeginSubcase()
__SubCase( '1>2,3>4', [  5, 7, 1, 8,   5, 7, 0, 1, \
                         5, 7, 6, 0,   5, 7, 9, 6, \
                         5, 7, 8, 9 ], +1 )
__PrmCase( '2>1,4>3', '1>2,3>4', ( 1, 0, 2, 3 ), -1 )
__SubCase( '1>2,4>3', [  0, 5, 6, 7,   0, 5, 7, 8, \
                         0, 5, 8, 1,   5, 7, 9, 6, \
                         5, 7, 8, 9 ], +1, \
                    ( [  0, 5, 6, 8,   0, 6, 7, 8, \
                         0, 5, 8, 1,   5, 8, 9, 6, \
                         6, 7, 8, 9 ], ) \
                         )
__PrmCase( '2>1,3>4', '1>2,4>3', ( 1, 0, 2, 3 ), -1 )
# 5-alpha
__SubCase( '1=2,3>4', [ 10, 6, 0, 7,  10, 1, 5, 8, \
                        10, 0, 1, 7,  10, 7, 1, 8, \
                        10, 8, 5, 9,  10, 6, 7, 9, \
                        10, 7, 8, 9,  10, 5, 6, 9 ], +1, \
                    ( [ 10, 6, 0, 7,  10, 1, 5, 8, \
                        10, 0, 1, 7,  10, 7, 1, 8, \
                         7, 8, 5, 9,  10, 6, 7, 5, \
                        10, 7, 8, 5,   5, 9, 6, 7 ], \
                      [ 10, 6, 0, 7,  10, 1, 5, 8, \
                        10, 0, 1, 7,  10, 7, 1, 8, \
                         6, 8, 5, 9,  10, 6, 7, 8, \
                        10, 6, 8, 5,   8, 9, 6, 7 ], ) \
                        )
__PrmCase( '1=2,4>3', '1=2,3>4', ( 1, 0, 2, 3 ), -1 )
__PrmCase( '3=4,1>2', '1=2,3>4', ( 1, 0, 3, 2 ), +1 )
__PrmCase( '3=4,2>1', '1=2,3>4', ( 0, 1, 3, 2 ), -1 )
# 5-beta
__SubCase( '1=2,3=4', [ 10, 6, 0, 7,  10, 1, 5, 8, \
                        10, 0, 1,11,  10,11, 1, 8, \
                        10, 0,11, 7,  10, 7,11, 8, \
                        10, 8, 5, 9,  10, 6, 7, 9, \
                        10, 7, 8, 9,  10, 5, 6, 9 ], +1 )
__EndSubcase()
EndCase()

BeginCase( '6' )
__Unconditional( [ 7, 8, 9, 3,   6, 5, 2, 9, \
                   4, 1, 5, 8,   0, 4, 6, 7 ], (), +1 )
# diagonal 6-8, 5-7, or 4-9
__Unconditional( [ 6, 4, 5, 8,   6, 5, 9, 8, \
                   6, 9, 7, 8,   6, 7, 4, 8 ], (), +1, '', '', \
                 ([ 5, 8, 9, 7,   5, 9, 6, 7,   \
                    5, 6, 4, 7,   5, 4, 8, 7 ], \
                  [ 4, 5, 6, 9,   4, 6, 7, 9,   \
                    4, 7, 8, 9,   4, 8, 5, 9 ]) \
                   )
EndCase()

print >> genCode, \
"""      }

    vtkIdType* tets;
    vtkIdType  ntets;
    vtkIdType* perm;
    int        sgn;
#ifdef PARAVIEW_DEBUG_TESSELLATOR
    if ( outputTets.empty() )
    {
    cout << "Argh! Case " << C << " Perm " << P << " has no output!" << endl;
    }
#endif // PARAVIEW_DEBUG_TESSELLATOR
    while ( ! outputTets.empty() )
      {
      tets = outputTets.top();
      ntets = *tets;
      tets++;
      perm = outputPerm.top();
      sgn = outputSign.top();

      outputTets.pop();
      outputPerm.pop();
      outputSign.pop();

      int t;
      if ( sgn > 0 )
        {
        for ( t = 0; t < ntets; ++t )
          {
          this->AdaptivelySample3Facet(
            permuted[ perm[ tets[0] ] ], permuted[ perm[ tets[1] ] ],
            permuted[ perm[ tets[2] ] ], permuted[ perm[ tets[3] ] ],
            maxDepth );
          tets += 4;
          }
        }
      else
        {
        // we have an inverted tet... reverse the first 2 vertices
        // so the orientation is positive.
        for ( t = 0; t < ntets; ++t )
          {
          this->AdaptivelySample3Facet(
            permuted[ perm[ tets[1] ] ], permuted[ perm[ tets[0] ] ],
            permuted[ perm[ tets[2] ] ], permuted[ perm[ tets[3] ] ],
            maxDepth );
          tets += 4;
          }
        }
      }
    }
}
"""

print >> genCode, \
"""
/*
 * The array below is indexed by the edge code for a tetrahedron.
 * Looking up a row with a tet's edge code will return C and P.
 * C is a configuration number and P is a permutation index.
 *
 * C is based on the case number from Ruprecht and
 * Müller's (1998) paper on adaptive tetrahedra. (The case
 * numbers are shown to the left of the row in the column
 * labeled case. The only difference is that we introduce
 * a case 3d which is part of case 3c in the paper.)
 *
 * P is an index into the vtkStreamingTessellator::PermutationsFromIndex array below,
 * and is used to transform the current tetrahedron into
 * the canonical configuration associated with C.
 *
 * The 6-digit binary number to the left (which is shown in
 * the horribly UNconventional LSB->MSB order) is the edge
 * code for the row. The 6 digits correspond to the 6 edges
 * of the tetrahedron; a '0' implies no subdivision while
 * a '1' implies subdivision should occur. The ordering of
 * the bits is
 *
 * Edge 0-1, Edge 1-2, Edge 2-0, Edge 0-3, Edge 1-3, Edge 2-3,
 *
 * where the numbers are vertices of the tetrahedron 0-1-2-3.
 * Note that Tet 0-1-2-3 must be positive (i.e., the plane
 * specified by Triangle 0-1-2 must have a normal pointing
 * towards vertex 3, and Triangle 0-1-2's normal must be
 * calculated using the cross-product (Edge 0-1) x (Edge 0-2)).
 *
 * ===========
 * References:
 * (Ruprect and Müller, 1998) A Scheme for Edge-based Adaptive
 *   Tetrahedron Subdivision, Mathematical Visualization (eds.
 *   Hege and Polthier), pp. 61--70. Springer-Verlag. 1998.
 */
int vtkStreamingTessellator::EdgeCodesToCaseCodesPlusPermutation[64][2] =
{
  /*      code case      C    P */
  /* 000000  0  0  */ {  0,   0 },
  /* 100000  1  1  */ {  1,   0 },
  /* 010000  2  1  */ {  1,   1 },
  /* 110000  3  2a */ {  2,   0 },
  /* 001000  4  1  */ {  1,   2 },
  /* 101000  5  2a */ {  2,   2 },
  /* 011000  6  2a */ {  2,   1 },
  /* 111000  7  3b */ {  5,  11 },
  /* 000100  8  1  */ {  1,  10 },
  /* 100100  9  2a */ {  2,   5 },
  /* 010100 10  2b */ {  3,   1 },
  /* 110100 11  3c */ {  6,   0 },
  /* 001100 12  2a */ {  2,  10 },
  /* 101100 13  3a */ {  4,   0 },
  /* 011100 14  3d */ {  7,   2 },
  /* 111100 15  4a */ {  8,   6 },
  /* 000010 16  1  */ {  1,   6 },
  /* 100010 17  2a */ {  2,   4 },
  /* 010010 18  2a */ {  2,   8 },
  /* 110010 19  3a */ {  4,   1 },
  /* 001010 20  2b */ {  3,   2 },
  /* 101010 21  3d */ {  7,   0 },
  /* 011010 22  3c */ {  6,   1 },
  /* 111010 23  4a */ {  8,   9 },
  /* 000110 24  2a */ {  2,   3 },
  /* 100110 25  3b */ {  5,   0 },
  /* 010110 26  3d */ {  7,   4 },
  /* 110110 27  4a */ {  8,  11 },
  /* 001110 28  3c */ {  6,  10 },
  /* 101110 29  4a */ {  8,   7 },
  /* 011110 30  4b */ {  9,   0 },
  /* 111110 31  5  */ { 10,   7 },
  /* 000001 32  1  */ {  1,   7 },
  /* 100001 33  2b */ {  3,   0 },
  /* 010001 34  2a */ {  2,   7 },
  /* 110001 35  3d */ {  7,   1 },
  /* 001001 36  2a */ {  2,  11 },
  /* 101001 37  3c */ {  6,   2 },
  /* 011001 38  3a */ {  4,   2 },
  /* 111001 39  4a */ {  8,   3 },
  /* 000101 40  2a */ {  2,   9 },
  /* 100101 41  3d */ {  7,  10 },
  /* 010101 42  3c */ {  6,   7 },
  /* 110101 43  4b */ {  9,   2 },
  /* 001101 44  3b */ {  5,   7 },
  /* 101101 45  4a */ {  8,   8 },
  /* 011101 46  4a */ {  8,   4 },
  /* 111101 47  5  */ { 10,   6 },
  /* 000011 48  2a */ {  2,   6 },
  /* 100011 49  3c */ {  6,   4 },
  /* 010011 50  3b */ {  5,   1 },
  /* 110011 51  4a */ {  8,  10 },
  /* 001011 52  3d */ {  7,   7 },
  /* 101011 53  4b */ {  9,   1 },
  /* 011011 54  4a */ {  8,   5 },
  /* 111011 55  5  */ { 10,  10 },
  /* 000111 56  3a */ {  4,  10 },
  /* 100111 57  4a */ {  8,   1 },
  /* 010111 58  4a */ {  8,   2 },
  /* 110111 59  5  */ { 10,   2 },
  /* 001111 60  4a */ {  8,   0 },
  /* 101111 61  5  */ { 10,   1 },
  /* 011111 62  5  */ { 10,   0 },
  /* 111111 63  6  */ { 11,   0 },
};


/* Does this mean anything? If so, then you are either
 * superstitious or much more clever than I (or both?).
 */
/* permutation index, P:  0  1  2  3  4  5  6  7  8  9 10 11 */
/* number of references: 12  9  9  3  4  2  5  6  2  3  7  2 */


/*
 * The array below is a list of all the _positive_
 * permutations of Tetrahedron 0-1-2-3. Given a
 * permutation index, it returns a row of 14 values:
 * these are the vertex numbers of the permuted
 * tetrahedron. The first 4 values are the permuted
 * corner indices, the next 6 values are the
 * permuted edge midpoint indices, and the final
 * entries reference mid-face points inserted
 * to maintain a compatible tetrahedralization.
 *
 * There are 24 entries, 6 for each of the 4 faces of
 * the tetrahedron.
 */
vtkIdType vtkStreamingTessellator::PermutationsFromIndex[24][14] =
{
  /* corners      midpoints          face points   */
  /* POSITIVE ARRANGEMENTS                         */
  { 0, 1, 2, 3,   4, 5, 6, 7, 8, 9,  10, 11, 12, 13 }, /* Face 0-1-2 */
  { 1, 2, 0, 3,   5, 6, 4, 8, 9, 7,  10, 12, 13, 11 },
  { 2, 0, 1, 3,   6, 4, 5, 9, 7, 8,  10, 13, 11, 12 },

  { 0, 3, 1, 2,   7, 8, 4, 6, 9, 5,  11, 13, 12, 10 }, /* Face 0-3-1 */
  { 3, 1, 0, 2,   8, 4, 7, 9, 5, 6,  11, 12, 10, 13 },
  { 1, 0, 3, 2,   4, 7, 8, 5, 6, 9,  11, 10, 13, 12 },

  { 1, 3, 2, 0,   8, 9, 5, 4, 7, 6,  12, 11, 13, 10 }, /* Face 1-3-2 */
  { 3, 2, 1, 0,   9, 5, 8, 7, 6, 4,  12, 13, 10, 11 },
  { 2, 1, 3, 0,   5, 8, 9, 6, 4, 7,  12, 10, 11, 13 },

  { 2, 3, 0, 1,   9, 7, 6, 5, 8, 4,  13, 12, 11, 10 }, /* Face 2-3-0 */
  { 3, 0, 2, 1,   7, 6, 9, 8, 4, 5,  13, 11, 10, 12 },
  { 0, 2, 3, 1,   6, 9, 7, 4, 5, 8,  13, 10, 12, 11 },

  /* NEGATIVE ARRANGEMENTS                         */
  { 0, 2, 1, 3,   6, 5, 4, 7, 9, 8,  10, 13, 12, 11 }, /* Face 0-1-2 */
  { 2, 1, 0, 3,   5, 4, 6, 9, 8, 7,  10, 12, 11, 13 },
  { 1, 0, 2, 3,   4, 6, 5, 8, 7, 9,  10, 11, 13, 12 },

  { 0, 1, 3, 2,   4, 8, 7, 6, 5, 9,  11, 10, 12, 13 }, /* Face 0-3-1 */
  { 1, 3, 0, 2,   8, 7, 4, 5, 9, 6,  11, 12, 13, 10 },
  { 3, 0, 1, 2,   7, 4, 8, 9, 6, 5,  11, 13, 10, 12 },

  { 1, 2, 3, 0,   5, 9, 8, 4, 6, 7,  12, 10, 13, 11 }, /* Face 1-3-2 */
  { 2, 3, 1, 0,   9, 8, 5, 6, 7, 4,  12, 13, 11, 10 },
  { 3, 1, 2, 0,   8, 5, 9, 7, 4, 6,  12, 11, 10, 13 },

  { 2, 0, 3, 1,   6, 7, 9, 5, 4, 8,  13, 10, 11, 12 }, /* Face 2-3-0 */
  { 0, 3, 2, 1,   7, 9, 6, 4, 8, 5,  13, 11, 12, 10 },
  { 3, 2, 0, 1,   9, 6, 7, 8, 5, 4,  13, 12, 10, 11 }
};

/*
 * Below is a list of output tetrahedra. The array is
 * generated by vtkStreamingCaseGenerator.py
 * which also generates the code that references it.
 * Each set of tetrahedra begins with a single integer
 * that is the number of tetrahedra for that particular
 * case. It is followed by 5 integers for each output
 * tetrahedron; the first four numbers on each row are
 * indices of the output tetrahedron. The final number
 * is a bit vector specifying which edges of the
 * tetrahedron are internal to the parent tetrahedron
 * being decomposed.
 *
 * Multiple lists of output tetrahedra may be
 * combined to create the tessellation of a single
 * input tetrahedron.
 */
"""
# ============================================================================
# Now that we have all our output tetrahedra defined, write
# out the list of all tetrahedra.
vtkTessCase.PrintAll( genCode )

