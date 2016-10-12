/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTessellatorFilter.cxx
  Language:  C++

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#include "vtkObjectFactory.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetEdgeSubdivisionCriterion.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingTessellator.h"
#include "vtkEdgeSubdivisionCriterion.h"
#include "vtkTessellatorFilter.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkTessellatorFilter);

namespace
{
  void vtkCopyTuples(
    vtkDataSetAttributes* inDSA, vtkIdType inId,
    vtkDataSetAttributes* outDSA, vtkIdType beginId, vtkIdType endId)
  {
    for (vtkIdType cc=beginId; cc < endId; ++cc)
    {
      outDSA->CopyData(inDSA, inId, cc);
    }
  }
}

// ========================================
// vtkCommand subclass for reporting progress of merge filter
class vtkProgressCommand : public vtkCommand
{
public:
  vtkProgressCommand( vtkTessellatorFilter* tf )
  {
    this->Tessellator = tf;
  }
  ~vtkProgressCommand() VTK_OVERRIDE
  {
  }
  void Execute( vtkObject*, unsigned long, void* callData ) VTK_OVERRIDE
  {
    double subprogress = *( static_cast<double*>( callData ) );
    cout << "  ++ <" << ( (subprogress / 2. + 0.5) * 100. ) << ">\n";
    this->Tessellator->UpdateProgress( subprogress / 2. + 0.5 );
  }
protected:
  vtkTessellatorFilter* Tessellator;
};

// ========================================
// convenience routines for paraview
void vtkTessellatorFilter::SetMaximumNumberOfSubdivisions( int N )
{
  if ( this->Tessellator )
  {
    this->Tessellator->SetMaximumNumberOfSubdivisions( N );
  }
}

int vtkTessellatorFilter::GetMaximumNumberOfSubdivisions()
{
  return this->Tessellator ? this->Tessellator->GetMaximumNumberOfSubdivisions() : 0;
}

void vtkTessellatorFilter::SetChordError( double E )
{
  if ( this->Subdivider )
  {
    this->Subdivider->SetChordError2( E > 0. ? E*E : E );
  }
}

double vtkTessellatorFilter::GetChordError()
{
  double tmp = this->Subdivider ? this->Subdivider->GetChordError2() : 0.;
  return tmp > 0. ? sqrt( tmp ) : tmp;
}

// ========================================
// callbacks for simplex output
void vtkTessellatorFilter::AddATetrahedron(
  const double* a, const double* b, const double* c, const double* d,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  vtkTessellatorFilter* self = (vtkTessellatorFilter*) pd;
  self->OutputTetrahedron( a, b, c, d );
}

void vtkTessellatorFilter::OutputTetrahedron(
  const double* a, const double* b, const double* c, const double* d )
{
  vtkIdType cellIds[4];

  cellIds[0] = this->OutputPoints->InsertNextPoint( a );
  cellIds[1] = this->OutputPoints->InsertNextPoint( b );
  cellIds[2] = this->OutputPoints->InsertNextPoint( c );
  cellIds[3] = this->OutputPoints->InsertNextPoint( d );

  this->OutputMesh->InsertNextCell( VTK_TETRA, 4, cellIds );

  const int* off = this->Subdivider->GetFieldOffsets();
  vtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;
  b += 6;
  c += 6;
  d += 6;

  for ( int at=0; at<this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off )
  {
    (*att)->InsertTuple( cellIds[0], a + *off );
    (*att)->InsertTuple( cellIds[1], b + *off );
    (*att)->InsertTuple( cellIds[2], c + *off );
    (*att)->InsertTuple( cellIds[3], d + *off );
  }
}

void vtkTessellatorFilter::AddATriangle(
  const double* a, const double* b, const double* c,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  vtkTessellatorFilter* self = (vtkTessellatorFilter*) pd;
  self->OutputTriangle( a, b, c );
}

void vtkTessellatorFilter::OutputTriangle(
  const double* a, const double* b, const double* c )
{
  vtkIdType cellIds[3];

  cellIds[0] = this->OutputPoints->InsertNextPoint( a );
  cellIds[1] = this->OutputPoints->InsertNextPoint( b );
  cellIds[2] = this->OutputPoints->InsertNextPoint( c );

  this->OutputMesh->InsertNextCell( VTK_TRIANGLE, 3, cellIds );

  const int* off = this->Subdivider->GetFieldOffsets();
  vtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;
  b += 6;
  c += 6;

  for ( int at=0; at<this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off )
  {
    (*att)->InsertTuple( cellIds[0], a + *off );
    (*att)->InsertTuple( cellIds[1], b + *off );
    (*att)->InsertTuple( cellIds[2], c + *off );
  }
}

void vtkTessellatorFilter::AddALine(
  const double* a, const double* b,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  vtkTessellatorFilter* self = (vtkTessellatorFilter*) pd;
  self->OutputLine( a, b );
}

void vtkTessellatorFilter::OutputLine( const double* a, const double* b )
{
  vtkIdType cellIds[2];

  cellIds[0] = this->OutputPoints->InsertNextPoint( a );
  cellIds[1] = this->OutputPoints->InsertNextPoint( b );

  this->OutputMesh->InsertNextCell( VTK_LINE, 2, cellIds );

  const int* off = this->Subdivider->GetFieldOffsets();
  vtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;
  b += 6;

  for ( int at=0; at<this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off )
  {
    (*att)->InsertTuple( cellIds[0], a + *off );
    (*att)->InsertTuple( cellIds[1], b + *off );
  }
}

void vtkTessellatorFilter::AddAPoint(
  const double* a,
  vtkEdgeSubdivisionCriterion*, void* pd, const void* )
{
  vtkTessellatorFilter* self = (vtkTessellatorFilter*) pd;
  self->OutputPoint( a );
}

void vtkTessellatorFilter::OutputPoint( const double* a )
{
  vtkIdType cellId;

  cellId = this->OutputPoints->InsertNextPoint( a );
  this->OutputMesh->InsertNextCell( VTK_VERTEX, 1, &cellId );

  const int* off = this->Subdivider->GetFieldOffsets();
  vtkDataArray** att = this->OutputAttributes;

  // Move a, b, & c past the geometric and parametric coordinates to the
  // beginning of the field values.
  a += 6;

  for ( int at=0; at<this->Subdivider->GetNumberOfFields(); ++at, ++att, ++off )
  {
    (*att)->InsertTuple( cellId, a + *off );
  }
}

// ========================================

// constructor/boilerplate members
vtkTessellatorFilter::vtkTessellatorFilter()
  : Tessellator( 0 ), Subdivider( 0 )
{
  this->OutputDimension = 3; // Tesselate elements directly, not boundaries
  this->SetTessellator( vtkStreamingTessellator::New() );
  this->Tessellator->Delete();
  this->SetSubdivider( vtkDataSetEdgeSubdivisionCriterion::New() );
  this->Subdivider->Delete();
  this->MergePoints = 1;
  this->Locator = vtkMergePoints::New();

  this->Tessellator->SetEmbeddingDimension( 1, 3 );
  this->Tessellator->SetEmbeddingDimension( 2, 3 );
}

vtkTessellatorFilter::~vtkTessellatorFilter()
{
  this->SetSubdivider( 0 );
  this->SetTessellator( 0 );
  this->Locator->Delete();
  this->Locator = 0;
}

void vtkTessellatorFilter::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "OutputDimension: " << this->OutputDimension << "\n"
     << indent << "Tessellator: " << this->Tessellator << "\n"
     << indent << "Subdivider: " << this->Subdivider << " (" << this->Subdivider->GetClassName() << ")" << "\n"
     << indent << "MergePoints: " << this->MergePoints << "\n"
     << indent << "Locator: " << this->Locator << "\n";
}

// override for proper Update() behavior
vtkMTimeType vtkTessellatorFilter::GetMTime()
{
  vtkMTimeType mt = this->MTime;
  vtkMTimeType tmp;

  if ( this->Tessellator )
  {
    tmp = this->Tessellator->GetMTime();
    if ( tmp > mt )
      mt = tmp;
  }

  if ( this->Subdivider )
  {
    tmp = this->Subdivider->GetMTime();
    if ( tmp > mt )
      mt = tmp;
  }

  return mt;
}

void vtkTessellatorFilter::SetTessellator( vtkStreamingTessellator* t )
{
  if ( this->Tessellator == t )
  {
    return;
  }

  if ( this->Tessellator )
  {
    this->Tessellator->UnRegister( this );
  }

  this->Tessellator = t;

  if ( this->Tessellator )
  {
    this->Tessellator->Register( this );
    this->Tessellator->SetSubdivisionAlgorithm( this->Subdivider );
  }

  this->Modified();
}

void vtkTessellatorFilter::SetSubdivider( vtkDataSetEdgeSubdivisionCriterion* s )
{
  if ( this->Subdivider == s )
  {
    return;
  }

  if ( this->Subdivider )
  {
    this->Subdivider->UnRegister( this );
  }

  this->Subdivider = s;

  if ( this->Subdivider )
  {
    this->Subdivider->Register( this );
  }

  if ( this->Tessellator )
  {
    this->Tessellator->SetSubdivisionAlgorithm( this->Subdivider );
  }

  this->Modified();
}

void vtkTessellatorFilter::SetFieldCriterion( int s, double err )
{
  if ( this->Subdivider )
  {
    this->Subdivider->SetFieldError2( s, err > 0. ? err*err : -1. );
  }
}

void vtkTessellatorFilter::ResetFieldCriteria()
{
  if ( this->Subdivider )
  {
    this->Subdivider->ResetFieldError2();
  }
}

// ========================================
// pipeline procedures
void vtkTessellatorFilter::SetupOutput(
  vtkDataSet* input, vtkUnstructuredGrid* output )
{
  this->OutputMesh = output;

  // avoid doing all the stupid checks on NumberOfOutputs for every
  // triangle/line.
  this->OutputMesh->Reset();
  this->OutputMesh->Allocate(0,0);

  if ( ! (this->OutputPoints = OutputMesh->GetPoints()) )
  {
    this->OutputPoints = vtkPoints::New();
    this->OutputMesh->SetPoints( this->OutputPoints );
    this->OutputPoints->Delete();
  }

  // This returns the id numbers of arrays that are default scalars, vectors,
  // normals, texture coords, and tensors.  These are the fields that will be
  // interpolated and passed on to the output mesh.
  vtkPointData* fields = input->GetPointData();
  vtkDataSetAttributes* outarrays = this->OutputMesh->GetPointData();
  outarrays->Initialize();
  // empty, turn off all attributes, and set CopyAllOn to true.

  this->OutputAttributes = new vtkDataArray* [ fields->GetNumberOfArrays() ];
  this->OutputAttributeIndices = new int [ fields->GetNumberOfArrays() ];

  // OK, we always add normals as the 0-th array so that there's less work to
  // do inside the tight loop (OutputTriangle)
  int attrib = 0;
  for ( int a = 0; a < fields->GetNumberOfArrays(); ++a )
  {
    if ( fields->IsArrayAnAttribute( a ) == vtkDataSetAttributes::NORMALS )
    {
      continue;
    }

    vtkDataArray* array = fields->GetArray( a );
    this->OutputAttributes[ attrib ] = vtkDataArray::CreateDataArray( array->GetDataType() );
    this->OutputAttributes[ attrib ]->SetNumberOfComponents( array->GetNumberOfComponents() );
    this->OutputAttributes[ attrib ]->SetName( array->GetName() );
    this->OutputAttributeIndices[ attrib ] = outarrays->AddArray( this->OutputAttributes[ attrib ] );
    this->OutputAttributes[ attrib ]->Delete(); // output mesh now owns the array
    int attribType;
    if ( (attribType = fields->IsArrayAnAttribute( a )) != -1 )
      outarrays->SetActiveAttribute( this->OutputAttributeIndices[ attrib ], attribType );

    this->Subdivider->PassField( a, array->GetNumberOfComponents(), this->Tessellator );
    ++attrib;
  }

  output->GetCellData()->CopyAllocate(input->GetCellData(), input->GetNumberOfCells());
}

void vtkTessellatorFilter::MergeOutputPoints( vtkUnstructuredGrid* input, vtkUnstructuredGrid* output )
{
  // this method cleverly lifted from ParaView's Servers/Filters/vtkCleanUnstructuredGrid::RequestData()
  if (input->GetNumberOfCells() == 0)
  {
    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.
    output->Allocate(1);
    output->GetPointData()->CopyAllocate(input->GetPointData(), VTK_CELL_SIZE);
    output->GetCellData()->CopyAllocate(input->GetCellData(), 1);
    vtkPoints *pts = vtkPoints::New();
    output->SetPoints(pts);
    pts->Delete();
    return;
  }

  output->GetPointData()->CopyAllocate(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // First, create a new points array that eliminate duplicate points.
  // Also create a mapping from the old point id to the new.
  vtkPoints* newPts = vtkPoints::New();
  vtkIdType num = input->GetNumberOfPoints();
  vtkIdType id;
  vtkIdType newId;
  vtkIdType* ptMap = new vtkIdType[num];
  double pt[3];

  this->Locator->InitPointInsertion(newPts, input->GetBounds(), num);

  vtkIdType progressStep = num / 100;
  if (progressStep == 0)
  {
    progressStep = 1;
  }
  for (id = 0; id < num; ++id)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress( 0.5 * ( 1. + id * 0.8 / num ) );
    }
    input->GetPoint(id, pt);
    if (this->Locator->InsertUniquePoint(pt, newId))
    {
      output->GetPointData()->CopyData(input->GetPointData(),id,newId);
    }
    ptMap[id] = newId;
  }
  output->SetPoints(newPts);
  newPts->Delete();

  // New copy the cells.
  vtkIdList *cellPoints = vtkIdList::New();
  num = input->GetNumberOfCells();
  output->Allocate(num);
  for (id = 0; id < num; ++id)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress(0.9+0.1*((float)id/num));
    }
    input->GetCellPoints(id, cellPoints);
    for (int i=0; i < cellPoints->GetNumberOfIds(); i++)
    {
      int cellPtId = cellPoints->GetId(i);
      newId = ptMap[cellPtId];
      cellPoints->SetId(i, newId);
    }
    output->InsertNextCell(input->GetCellType(id), cellPoints);
  }

  delete [] ptMap;
  cellPoints->Delete();
}

void vtkTessellatorFilter::Teardown()
{
  this->OutputMesh = 0;
  this->OutputPoints = 0;
  delete [] this->OutputAttributes;
  delete [] this->OutputAttributeIndices;
  this->Subdivider->ResetFieldList();
  this->Subdivider->SetMesh(0);
}

// ========================================
// output element topology
static const double extraLinHexParams[12][3] =
{
  { 0.5, 0.0, 0.0 },
  { 1.0, 0.5, 0.0 },
  { 0.5, 1.0, 0.0 },
  { 0.0, 0.5, 0.0 },
  { 0.5, 0.0, 1.0 },
  { 1.0, 0.5, 1.0 },
  { 0.5, 1.0, 1.0 },
  { 0.0, 0.5, 1.0 },
  { 0.0, 0.0, 0.5 },
  { 1.0, 0.0, 0.5 },
  { 1.0, 1.0, 0.5 },
  { 0.0, 1.0, 0.5 },
};

static const double extraQuadHexParams[7][3] =
{
  { 0.5, 0.5, 0.0 },
  { 0.5, 0.5, 1.0 },
  { 0.5, 0.0, 0.5 },
  { 0.5, 1.0, 0.5 },
  { 0.0, 0.5, 0.5 },
  { 1.0, 0.5, 0.5 },
  { 0.5, 0.5, 0.5 }
};

static const double extraQuadQuadParams[1][3] =
{
  { 0.5, 0.5, 0.0 }
};

static vtkIdType linEdgeEdges[][2] =
{
  {0,1}
};

static vtkIdType quadEdgeEdges[][2] =
{
  {0,2},
  {2,1}
};

static vtkIdType cubicLinEdges[][2] =
{
  {0,2},
  {2,3},
  {3,1}
};

static vtkIdType linTriTris[][3] =
{
  {0,1,2}
};

static vtkIdType linTriEdges[][2] =
{
  {0,1},
  {1,2},
  {2,0}
};

static vtkIdType quadTriTris[][3] =
{
  {0,3,5},
  {5,3,1},
  {5,1,4},
  {4,2,5}
};

static vtkIdType biQuadTriTris[][3] =
{
  {0,3,6},
  {3,1,6},
  {6,1,4},
  {6,4,2},
  {6,2,5},
  {0,6,5}
};

static vtkIdType biQuadTriEdges[][2] =
{
  {0,3},
  {3,1},
  {1,4},
  {4,2},
  {2,5},
  {5,0}
};

static vtkIdType quadTriEdges[][2] =
{
  {0,3},
  {3,1},
  {1,4},
  {4,2},
  {2,5},
  {5,0}
};

static vtkIdType linQuadTris[][3] =
{
  {0,1,2},
  {0,2,3}
};

static vtkIdType linQuadEdges[][2] =
{
  {0,1},
  {1,2},
  {2,3},
  {3,0}
};

static vtkIdType quadQuadTris[][3] =
{
  {0,4,7},
  {7,4,8},
  {7,8,3},
  {3,8,6},
  {4,1,5},
  {8,4,5},
  {8,5,2},
  {2,6,8}
};

static vtkIdType quadQuadEdges[][2] =
{
  {0,4},
  {4,1},
  {1,5},
  {5,2},
  {2,6},
  {6,3},
  {3,7},
  {7,0}
};

static vtkIdType linWedgeTetrahedra[][4] =
{
  {3,2,1,0},
  {1,2,3,4},
  {2,3,4,5}
};

static vtkIdType linWedgeTris[][3] =
{
  {0,2,1},
  {3,4,5},
  {3,4,5},
  {3,4,5},
  {0,1,3},
  {3,1,4},
  {1,2,4},
  {4,2,5},
  {2,0,5},
  {5,0,3}
};

static vtkIdType linWedgeEdges[][2] =
{
  {0,1},
  {1,2},
  {2,0},
  {3,4},
  {4,5},
  {5,3},
  {0,3},
  {1,4},
  {2,5}
};

static vtkIdType linPyrTetrahedra[][4] =
{
  {0,1,2,4},
  {0,2,3,4}
};

static vtkIdType linPyrTris[][3] =
{
  {0,1,2},
  {0,2,3},
  {0,1,4},
  {1,2,4},
  {2,3,4},
  {3,0,4}
};

static vtkIdType linPyrEdges[][2] =
{
  {0,1},
  {1,2},
  {2,3},
  {3,0},
  {0,4},
  {1,4},
  {2,4},
  {3,4}
};

static vtkIdType linTetTetrahedra[][4] =
{
  {0,1,2,3}
};

static vtkIdType linTetTris[][3] =
{
  {0,2,1},
  {0,1,3},
  {1,2,3},
  {2,0,3}
};

static vtkIdType linTetEdges[][2] =
{
  {0,1},
  {1,2},
  {2,0},
  {0,3},
  {1,3},
  {2,3}
};

static vtkIdType quadTetTetrahedra[][4] =
{
  {4,7,6,0},
  {5,6,9,2},
  {7,8,9,3},
  {4,5,8,1},
  {6,8,7,4},
  {6,8,4,5},
  {6,8,5,9},
  {6,8,9,7}
};

static vtkIdType quadTetTris[][3] =
{
  {0,6,4},
  {4,6,5},
  {5,6,2},
  {4,5,1},

  {0,4,7},
  {7,4,8},
  {8,4,1},
  {7,8,3},

  {1,5,8},
  {8,5,9},
  {9,5,2},
  {8,9,3},

  {2,6,9},
  {9,6,7},
  {7,6,0},
  {9,7,3}
};

static vtkIdType quadTetEdges[][2] =
{
  {0,4},
  {4,1},
  {1,5},
  {5,2},
  {2,6},
  {6,0},
  {0,7},
  {7,3},
  {1,8},
  {8,3},
  {2,9},
  {9,3}
};

/* Each face should look like this:
 *             +-+-+
 *             |\|/|
 *             +-+-+
 *             |/|\|
 *             +-+-+
 * This tessellation is required for
 * neighboring hexes to have compatible
 * boundaries.
 */
static vtkIdType quadHexTetrahedra[][4] =
{
  { 0, 8,20,26},
  { 8, 1,20,26},
  { 1, 9,20,26},
  { 9, 2,20,26},
  { 2,10,20,26},
  {10, 3,20,26},
  { 3,11,20,26},
  {11, 0,20,26},

  { 4,15,21,26},
  {15, 7,21,26},
  { 7,14,21,26},
  {14, 6,21,26},
  { 6,13,21,26},
  {13, 5,21,26},
  { 5,12,21,26},
  {12, 4,21,26},

  { 0,16,22,26},
  {16, 4,22,26},
  { 4,12,22,26},
  {12, 5,22,26},
  { 5,17,22,26},
  {17, 1,22,26},
  { 1, 8,22,26},
  { 8, 0,22,26},

  { 3,10,23,26},
  {10, 2,23,26},
  { 2,18,23,26},
  {18, 6,23,26},
  { 6,14,23,26},
  {14, 7,23,26},
  { 7,19,23,26},
  {19, 3,23,26},

  { 0,11,24,26},
  {11, 3,24,26},
  { 3,19,24,26},
  {19, 7,24,26},
  { 7,15,24,26},
  {15, 4,24,26},
  { 4,16,24,26},
  {16, 0,24,26},

  { 1,17,25,26},
  {17, 5,25,26},
  { 5,13,25,26},
  {13, 6,25,26},
  { 6,18,25,26},
  {18, 2,25,26},
  { 2, 9,25,26},
  { 9, 1,25,26}
};

static vtkIdType quadHexTris[][3] =
{
  { 0, 8,20},
  { 8, 1,20},
  { 1, 9,20},
  { 9, 2,20},
  { 2,10,20},
  {10, 3,20},
  { 3,11,20},
  {11, 0,20},

  { 4,15,21},
  {15, 7,21},
  { 7,14,21},
  {14, 6,21},
  { 6,13,21},
  {13, 5,21},
  { 5,12,21},
  {12, 4,21},

  { 0,16,22},
  {16, 4,22},
  { 4,12,22},
  {12, 5,22},
  { 5,17,22},
  {17, 1,22},
  { 1, 8,22},
  { 8, 0,22},

  { 3,10,23},
  {10, 2,23},
  { 2,18,23},
  {18, 6,23},
  { 6,14,23},
  {14, 7,23},
  { 7,19,23},
  {19, 3,23},

  { 0,11,24},
  {11, 3,24},
  { 3,19,24},
  {19, 7,24},
  { 7,15,24},
  {15, 4,24},
  { 4,16,24},
  {16, 0,24},

  { 1,17,25},
  {17, 5,25},
  { 5,13,25},
  {13, 6,25},
  { 6,18,25},
  {18, 2,25},
  { 2, 9,25},
  { 9, 1,25}
};

static vtkIdType quadHexEdges[][2] =
{
  { 0, 8},
  { 8, 1},
  { 1, 9},
  { 9, 2},
  { 2,10},
  {10, 3},
  { 3,11},
  {11, 0},
  { 4,15},
  {15, 7},
  { 7,14},
  {14, 6},
  { 6,13},
  {13, 5},
  { 5,12},
  {12, 4},
  { 0,16},
  {16, 4},
  { 5,17},
  {17, 1},
  { 2,18},
  {18, 6},
  { 7,19},
  {19, 3}
};


static vtkIdType quadVoxTetrahedra[][4] =
{
  { 0, 8,20,26},
  { 8, 1,20,26},
  { 1, 9,20,26},
  { 9, 3,20,26},
  { 3,10,20,26},
  {10, 2,20,26},
  { 2,11,20,26},
  {11, 0,20,26},

  { 4,15,21,26},
  {15, 6,21,26},
  { 6,14,21,26},
  {14, 7,21,26},
  { 7,13,21,26},
  {13, 5,21,26},
  { 5,12,21,26},
  {12, 4,21,26},

  { 0,16,22,26},
  {16, 4,22,26},
  { 4,12,22,26},
  {12, 5,22,26},
  { 5,17,22,26},
  {17, 1,22,26},
  { 1, 8,22,26},
  { 8, 0,22,26},

  { 2,10,23,26},
  {10, 3,23,26},
  { 3,18,23,26},
  {18, 7,23,26},
  { 7,14,23,26},
  {14, 6,23,26},
  { 6,19,23,26},
  {19, 2,23,26},

  { 0,11,24,26},
  {11, 2,24,26},
  { 2,19,24,26},
  {19, 6,24,26},
  { 6,15,24,26},
  {15, 4,24,26},
  { 4,16,24,26},
  {16, 0,24,26},

  { 1,17,25,26},
  {17, 5,25,26},
  { 5,13,25,26},
  {13, 7,25,26},
  { 7,18,25,26},
  {18, 3,25,26},
  { 3, 9,25,26},
  { 9, 1,25,26}
};

static vtkIdType quadVoxTris[][3] =
{
  { 0, 8,20},
  { 8, 1,20},
  { 1, 9,20},
  { 9, 3,20},
  { 3,10,20},
  {10, 2,20},
  { 2,11,20},
  {11, 0,20},

  { 4,15,21},
  {15, 6,21},
  { 6,14,21},
  {14, 7,21},
  { 7,13,21},
  {13, 5,21},
  { 5,12,21},
  {12, 4,21},

  { 0,16,22},
  {16, 4,22},
  { 4,12,22},
  {12, 5,22},
  { 5,17,22},
  {17, 1,22},
  { 1, 8,22},
  { 8, 0,22},

  { 2,10,23},
  {10, 3,23},
  { 3,18,23},
  {18, 7,23},
  { 7,14,23},
  {14, 6,23},
  { 6,19,23},
  {19, 2,23},

  { 0,11,24},
  {11, 2,24},
  { 2,19,24},
  {19, 6,24},
  { 6,15,24},
  {15, 4,24},
  { 4,16,24},
  {16, 0,24},

  { 1,17,25},
  {17, 5,25},
  { 5,13,25},
  {13, 7,25},
  { 7,18,25},
  {18, 3,25},
  { 3, 9,25},
  { 9, 1,25}
};

static vtkIdType quadVoxEdges[][2] =
{
  { 0, 8},
  { 8, 1},
  { 1, 9},
  { 9, 3},
  { 3,10},
  {10, 2},
  { 2,11},
  {11, 0},
  { 4,15},
  {15, 6},
  { 6,14},
  {14, 7},
  { 7,13},
  {13, 5},
  { 5,12},
  {12, 4},
  { 0,16},
  {16, 4},
  { 5,17},
  {17, 1},
  { 3,18},
  {18, 7},
  { 6,19},
  {19, 2}
};




// This is used by the Execute() method to avoid printing out one
// "Not Supported" error message per cell. Instead, we print one
// per Execute().
static int vtkNotSupportedErrorPrinted = 0;
static int vtkTessellatorHasPolys = 0;

// ========================================
// the meat of the class: execution!
int vtkTessellatorFilter::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  static double weights[27];
  int dummySubId=-1;
  int p;

  vtkNotSupportedErrorPrinted = 0;

  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* mesh = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSmartPointer<vtkUnstructuredGrid> tmpOut;
  if ( this->MergePoints )
  {
    tmpOut = vtkSmartPointer<vtkUnstructuredGrid>::New();
  }
  else
  {
    tmpOut = output;
  }

  this->SetupOutput( mesh, tmpOut );

  this->Subdivider->SetMesh( mesh );
  this->Tessellator->SetVertexCallback( AddAPoint );
  this->Tessellator->SetEdgeCallback( AddALine );
  this->Tessellator->SetTriangleCallback( AddATriangle );
  this->Tessellator->SetTetrahedronCallback( AddATetrahedron );
  this->Tessellator->SetPrivateData( this );

  vtkIdType cell = 0;
  int nprim = 0;
  vtkIdType* outconn = 0;
  double pts[27][11 + vtkStreamingTessellator::MaxFieldSize];
  int c;
  vtkIdType numCells = mesh->GetNumberOfCells();
  int progress = 0;
  int progMax = this->MergePoints ? 50 : 100;
  vtkIdType deltaProg = numCells / progMax + 1; // the extra + 1 means we always reach the end
  vtkIdType progCells = 0;

  vtkTessellatorHasPolys = 0; // print error message once per invocation, if needed
  for ( progress = 0; progress < progMax; ++progress )
  {
    progCells += deltaProg;
    for ( ; (cell < progCells) && (cell < numCells); ++cell )
    {
      const vtkIdType nextOutCellId = this->OutputMesh->GetNumberOfCells();

      this->Subdivider->SetCellId( cell );

      vtkCell* cp = this->Subdivider->GetCell(); // We set the cell ID, get the vtkCell pointer
      int np = cp->GetCellType();
      double* pcoord = cp->GetParametricCoords();
      if (
        !pcoord ||
        np == VTK_POLYGON || np == VTK_TRIANGLE_STRIP || np == VTK_CONVEX_POINT_SET ||
        np == VTK_POLY_LINE || np == VTK_POLY_VERTEX || np == VTK_POLYHEDRON ||
        np == VTK_QUADRATIC_POLYGON)
      {
        if ( ! vtkTessellatorHasPolys )
        {
          vtkWarningMacro(
            "Input dataset has cells without parameterizations "
            "(VTK_POLYGON,VTK_POLY_LINE,VTK_POLY_VERTEX,VTK_TRIANGLE_STRIP,VTK_CONVEX_POINT_SET,VTK_QUADRATIC_POLYGON). "
            "They will be ignored. Use vtkTriangleFilter, vtkTetrahedralize, etc. to parameterize them first." );
          vtkTessellatorHasPolys = 1;
        }
        continue;
      }
      double* gcoord;
      vtkDataArray* field;
      for ( p = 0; p < cp->GetNumberOfPoints(); ++p )
      {
        gcoord = cp->Points->GetPoint( p );
        for ( c = 0; c < 3; ++c, ++gcoord, ++pcoord )
        {
          pts[p][c  ] = *gcoord;
          pts[p][c+3] = *pcoord;
        }
        // fill in field data
        const int* offsets = this->Subdivider->GetFieldOffsets();
        for ( int f = 0; f < this->Subdivider->GetNumberOfFields(); ++f )
        {
          field = mesh->GetPointData()->GetArray( this->Subdivider->GetFieldIds()[ f ] );
          double* tuple = field->GetTuple( cp->GetPointId( p ) );
          for ( c = 0; c < field->GetNumberOfComponents(); ++c )
          {
            pts[p][ 6 + offsets[f] + c ] = tuple[c];
          }
        }
      }
      int dim = this->OutputDimension;
      // Tessellate each cell:
      switch ( cp->GetCellType() )
      {
      case VTK_VERTEX:
        dim = 0;
        outconn = 0;
        nprim = 1;
        break;
      case VTK_LINE:
        dim = 1;
        outconn = &linEdgeEdges[0][0];
        nprim = sizeof(linEdgeEdges)/sizeof(linEdgeEdges[0]);
        break;
      case VTK_TRIANGLE:
        if ( dim > 1 )
        {
          dim = 2;
          outconn = &linTriTris[0][0];
          nprim = sizeof(linTriTris)/sizeof(linTriTris[0]);
        }
        else
        {
          outconn = &linTriEdges[0][0];
          nprim = sizeof(linTriEdges)/sizeof(linTriEdges[0]);
        }
        break;
      case VTK_QUAD:
        if ( dim > 1 )
        {
          dim = 2;
          outconn = &linQuadTris[0][0];
          nprim = sizeof(linQuadTris)/sizeof(linQuadTris[0]);
        }
        else
        {
          outconn = &linQuadEdges[0][0];
          nprim = sizeof(linQuadEdges)/sizeof(linQuadEdges[0]);
        }
        break;
      case VTK_TETRA:
        if ( dim == 3 )
        {
          outconn = &linTetTetrahedra[0][0];
          nprim = sizeof(linTetTetrahedra)/sizeof(linTetTetrahedra[0]);
        }
        else if ( dim == 2 )
        {
          outconn = &linTetTris[0][0];
          nprim = sizeof(linTetTris)/sizeof(linTetTris[0]);
        }
        else
        {
          outconn = &linTetEdges[0][0];
          nprim = sizeof(linTetEdges)/sizeof(linTetEdges[0]);
        }
        break;
      case VTK_WEDGE:
        if ( dim == 3 )
        {
          outconn = &linWedgeTetrahedra[0][0];
          nprim = sizeof(linWedgeTetrahedra)/sizeof(linWedgeTetrahedra[0]);
        }
        else if ( dim ==2 )
        {
          outconn = &linWedgeTris[0][0];
          nprim = sizeof(linWedgeTris)/sizeof(linWedgeTris[0]);
        }
        else
        {
          outconn = &linWedgeEdges[0][0];
          nprim = sizeof(linWedgeEdges)/sizeof(linWedgeEdges[0]);
        }
        break;
      case VTK_PYRAMID:
        if ( dim == 3 )
        {
          outconn = &linPyrTetrahedra[0][0];
          nprim = sizeof(linPyrTetrahedra)/sizeof(linPyrTetrahedra[0]);
        }
        else if ( dim ==2 )
        {
          outconn = &linPyrTris[0][0];
          nprim = sizeof(linPyrTris)/sizeof(linPyrTris[0]);
        }
        else
        {
          outconn = &linPyrEdges[0][0];
          nprim = sizeof(linPyrEdges)/sizeof(linPyrEdges[0]);
        }
        break;
      case VTK_QUADRATIC_EDGE:
        dim = 1;
        outconn = &quadEdgeEdges[0][0];
        nprim = sizeof(quadEdgeEdges)/sizeof(quadEdgeEdges[0]);
        break;
      case VTK_CUBIC_LINE:
        dim = 1;
        outconn = &cubicLinEdges[0][0];
        nprim = sizeof(cubicLinEdges)/sizeof(cubicLinEdges[0]);
        break;
      case VTK_QUADRATIC_TRIANGLE:
        if ( dim > 1 )
        {
          dim = 2;
          outconn = &quadTriTris[0][0];
          nprim = sizeof(quadTriTris)/sizeof(quadTriTris[0]);
        }
        else
        {
          outconn = &quadTriEdges[0][0];
          nprim = sizeof(quadTriEdges)/sizeof(quadTriEdges[0]);
        }
        break;
      case VTK_BIQUADRATIC_TRIANGLE:
        if ( dim > 1 )
        {
          dim = 2;
          outconn = &biQuadTriTris[0][0];
          nprim = sizeof(biQuadTriTris)/sizeof(biQuadTriTris[0]);
        }
        else
        {
          outconn = &biQuadTriEdges[0][0];
          nprim = sizeof(biQuadTriEdges)/sizeof(biQuadTriEdges[0]);
        }
        break;
      case VTK_BIQUADRATIC_QUAD:
      case VTK_QUADRATIC_QUAD:
        for ( c = 0; c < 3; ++c )
        {
          pts[8][c+3] = extraQuadQuadParams[0][c];
        }
        cp->EvaluateLocation( dummySubId, pts[8] + 3, pts[8], weights );
        this->Subdivider->EvaluateFields( pts[8], weights, 6 );
        if ( dim > 1 )
        {
          dim = 2;
          outconn = &quadQuadTris[0][0];
          nprim = sizeof(quadQuadTris)/sizeof(quadQuadTris[0]);
        }
        else
        {
          outconn = &quadQuadEdges[0][0];
          nprim = sizeof(quadQuadEdges)/sizeof(quadQuadEdges[0]);
        }
        break;
      case VTK_QUADRATIC_TETRA:
        if ( dim == 3 )
        {
          outconn = &quadTetTetrahedra[0][0];
          nprim = sizeof(quadTetTetrahedra)/sizeof(quadTetTetrahedra[0]);
        }
        else if ( dim ==2 )
        {
          outconn = &quadTetTris[0][0];
          nprim = sizeof(quadTetTris)/sizeof(quadTetTris[0]);
        }
        else
        {
          outconn = &quadTetEdges[0][0];
          nprim = sizeof(quadTetEdges)/sizeof(quadTetEdges[0]);
        }
        break;
      case VTK_HEXAHEDRON:
        // we sample 19 extra points to guarantee a compatible tetrahedralization
        for ( p = 8; p < 20; ++p )
        {
          dummySubId=-1;
          for ( int y = 0; y < 3; ++y )
          {
            pts[p][y+3] = extraLinHexParams[p-8][y];
          }
          cp->EvaluateLocation( dummySubId, pts[p] + 3, pts[p], weights );
          this->Subdivider->EvaluateFields( pts[p], weights, 6 );
        }
        VTK_FALLTHROUGH;
      case VTK_QUADRATIC_HEXAHEDRON:
        for ( p = 20; p < 27; ++p )
        {
          dummySubId=-1;
          for ( int x = 0; x < 3; ++x )
          {
            pts[p][x+3] = extraQuadHexParams[p-20][x];
          }
          cp->EvaluateLocation( dummySubId, pts[p] + 3, pts[p], weights );
          this->Subdivider->EvaluateFields( pts[p], weights, 6 );
        }
        if ( dim == 3 )
        {
          outconn = &quadHexTetrahedra[0][0];
          nprim = sizeof(quadHexTetrahedra)/sizeof(quadHexTetrahedra[0]);
        }
        else if ( dim == 2 )
        {
          outconn = &quadHexTris[0][0];
          nprim = sizeof(quadHexTris)/sizeof(quadHexTris[0]);
        }
        else
        {
          outconn = &quadHexEdges[0][0];
          nprim = sizeof(quadHexEdges)/sizeof(quadHexEdges[0]);
        }
        break;
      case VTK_VOXEL:
        // we sample 19 extra points to guarantee a compatible tetrahedralization
        for ( p = 8; p < 20; ++p )
        {
          dummySubId=-1;
          for ( int y = 0; y < 3; ++y )
          {
            pts[p][y+3] = extraLinHexParams[p-8][y];
          }
          cp->EvaluateLocation( dummySubId, pts[p] + 3, pts[p], weights );
          this->Subdivider->EvaluateFields( pts[p], weights, 6 );
        }
        for ( p = 20; p < 27; ++p )
        {
          dummySubId=-1;
          for ( int x = 0; x < 3; ++x )
          {
            pts[p][x+3] = extraQuadHexParams[p-20][x];
          }
          cp->EvaluateLocation( dummySubId, pts[p] + 3, pts[p], weights );
          this->Subdivider->EvaluateFields( pts[p], weights, 6 );
        }
        if ( dim == 3 )
        {
          outconn = &quadVoxTetrahedra[0][0];
          nprim = sizeof(quadVoxTetrahedra)/sizeof(quadVoxTetrahedra[0]);
        }
        else if ( dim == 2 )
        {
          outconn = &quadVoxTris[0][0];
          nprim = sizeof(quadVoxTris)/sizeof(quadVoxTris[0]);
        }
        else
        {
          outconn = &quadVoxEdges[0][0];
          nprim = sizeof(quadVoxEdges)/sizeof(quadVoxEdges[0]);
        }
        break;
      case VTK_PIXEL:
        dim = -1;
        if ( ! vtkNotSupportedErrorPrinted )
        {
          vtkNotSupportedErrorPrinted = 1;
          vtkWarningMacro( "Oops, pixels are not supported" );
        }
        break;
      default:
        dim = -1;
        if ( ! vtkNotSupportedErrorPrinted )
        {
          vtkNotSupportedErrorPrinted = 1;
          vtkWarningMacro( "Oops, some cell type (" << cp->GetCellType()
            << ") not supported" );
        }
      }

      // OK, now output the primitives
      int tet, tri, edg;
      switch ( dim )
      {
      case 3:
        for ( tet=0; tet<nprim; ++tet, outconn += 4 )
        {
          this->Tessellator->AdaptivelySample3Facet( pts[outconn[0]],
            pts[outconn[1]],
            pts[outconn[2]],
            pts[outconn[3]] );
        }
        break;
      case 2:
        for ( tri=0; tri<nprim; ++tri, outconn += 3 )
        {
          this->Tessellator->AdaptivelySample2Facet( pts[outconn[0]],
            pts[outconn[1]],
            pts[outconn[2]] );
        }
        break;
      case 1:
        for ( edg=0; edg<nprim; ++edg, outconn += 2 )
        {
          this->Tessellator->AdaptivelySample1Facet( pts[outconn[0]],
            pts[outconn[1]] );
        }
        break;
      case 0:
        this->Tessellator->AdaptivelySample0Facet( pts[0] );
        break;
      default:
        // do nothing
        break;
      }

      // Copy cell data.
      vtkCopyTuples(mesh->GetCellData(), cell,
        this->OutputMesh->GetCellData(),
        nextOutCellId, this->OutputMesh->GetNumberOfCells());
    }
    this->UpdateProgress( (double)( progress / 100. ) );
  }

  if ( this->MergePoints )
  {
    this->MergeOutputPoints( tmpOut, output );
  }
  output->Squeeze();
  this->Teardown();

  return 1;
}


//----------------------------------------------------------------------------
int vtkTessellatorFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
