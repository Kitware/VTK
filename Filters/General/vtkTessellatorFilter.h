/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkTessellatorFilter.h
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright 2003 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the
U.S. Government. Redistribution and use in source and binary forms, with
or without modification, are permitted provided that this Notice and any
statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef __vtkTessellatorFilter_h
#define __vtkTessellatorFilter_h

// .NAME vtkTessellatorFilter - approximate nonlinear FEM elements with simplices
// .SECTION Description
// This class approximates nonlinear FEM elements with linear simplices.
//
// <b>Warning</b>: This class is temporary and will go away at some point
// after ParaView 1.4.0.
//
// This filter rifles through all the cells in an input vtkDataSet. It
// tesselates each cell and uses the vtkStreamingTessellator and
// vtkDataSetEdgeSubdivisionCriterion classes to generate simplices that
// approximate the nonlinear mesh using some approximation metric (encoded
// in the particular vtkDataSetEdgeSubdivisionCriterion::EvaluateEdge
// implementation). The simplices are placed into the filter's output
// vtkDataSet object by the callback routines AddATetrahedron,
// AddATriangle, and AddALine, which are registered with the triangulator.
//
// The output mesh will have geometry and any fields specified as
// attributes in the input mesh's point data.  The attribute's copy flags
// are honored, except for normals.
//
// .SECTION Internals
//
// The filter's main member function is RequestData(). This function first
// calls SetupOutput() which allocates arrays and some temporary variables
// for the primitive callbacks (OutputTriangle and OutputLine which are
// called by AddATriangle and AddALine, respectively).  Each cell is given
// an initial tesselation, which results in one or more calls to
// OutputTetrahedron, OutputTriangle or OutputLine to add elements to the
// OutputMesh. Finally, Teardown() is called to free the filter's working
// space.
//
// .SECTION See Also
// vtkDataSetToUnstructuredGridFilter vtkDataSet vtkStreamingTessellator
// vtkDataSetEdgeSubdivisionCriterion

#include "vtkUnstructuredGridAlgorithm.h"

class vtkDataArray;
class vtkDataSet;
class vtkDataSetEdgeSubdivisionCriterion;
class vtkPointLocator;
class vtkPoints;
class vtkStreamingTessellator;
class vtkEdgeSubdivisionCriterion;
class vtkUnstructuredGrid;

class VTK_GRAPHICS_EXPORT vtkTessellatorFilter : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkTessellatorFilter,vtkUnstructuredGridAlgorithm);
  void PrintSelf( ostream& os, vtkIndent indent );

  static vtkTessellatorFilter* New();

  virtual void SetTessellator( vtkStreamingTessellator* );
  vtkGetObjectMacro(Tessellator, vtkStreamingTessellator);

  virtual void SetSubdivider( vtkDataSetEdgeSubdivisionCriterion* );
  vtkGetObjectMacro(Subdivider, vtkDataSetEdgeSubdivisionCriterion);

  virtual unsigned long GetMTime();

  // Description:
  // Set the dimension of the output tessellation.
  // Cells in dimensions higher than the given value will have
  // their boundaries of dimension \a OutputDimension tessellated.
  // For example, if \a OutputDimension is 2, a hexahedron's
  // quadrilateral faces would be tessellated rather than its
  // interior.
  vtkSetClampMacro(OutputDimension,int,1,3);
  vtkGetMacro(OutputDimension,int);
  //BTX
  int GetOutputDimension() const;
  //ETX

  // Description:
  // These are convenience routines for setting properties maintained by the
  // tessellator and subdivider. They are implemented here for ParaView's
  // sake.
  virtual void SetMaximumNumberOfSubdivisions( int num_subdiv_in );
  int GetMaximumNumberOfSubdivisions();
  virtual void SetChordError( double ce );
  double GetChordError();

  // Description:
  // These methods are for the ParaView client.
  virtual void ResetFieldCriteria();
  virtual void SetFieldCriterion( int field, double chord );

  // Description:
  // The adaptive tessellation will output vertices that are not shared
  // among cells, even where they should be. This can be corrected to
  // some extents with a vtkMergeFilter.
  // By default, the filter is off and vertices will not be shared.
  vtkGetMacro(MergePoints,int);
  vtkSetMacro(MergePoints,int);
  vtkBooleanMacro(MergePoints,int);

protected:
  vtkTessellatorFilter();
  ~vtkTessellatorFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  
  // Description:
  // Called by RequestData to set up a multitude of member variables used by
  // the per-primitive output functions (OutputLine, OutputTriangle, and
  // maybe one day... OutputTetrahedron).
  void SetupOutput( vtkDataSet* input, vtkUnstructuredGrid* output );

  // Description:
  // Called by RequestData to merge output points.
  void MergeOutputPoints( vtkUnstructuredGrid* input, vtkUnstructuredGrid* output );

  // Description:
  // Reset the temporary variables used during the filter's RequestData() method.
  void Teardown();

  // Description:
  // Run the filter; produce a polygonal approximation to the grid.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  //BTX
  vtkStreamingTessellator* Tessellator;
  vtkDataSetEdgeSubdivisionCriterion* Subdivider;
  int OutputDimension;
  int MergePoints;
  vtkPointLocator* Locator;

  // Description:
  // These member variables are set by SetupOutput for use inside the
  // callback members OutputLine and OutputTriangle.
  vtkUnstructuredGrid* OutputMesh;
  vtkPoints* OutputPoints;
  vtkDataArray** OutputAttributes;
  int* OutputAttributeIndices;

  static void AddAPoint( const double*, 
                         vtkEdgeSubdivisionCriterion*, 
                         void*, 
                         const void* );
  static void AddALine( const double*, 
                        const double*, 
                        vtkEdgeSubdivisionCriterion*, 
                        void*, 
                        const void* );
  static void AddATriangle( const double*, 
                            const double*, 
                            const double*, 
                            vtkEdgeSubdivisionCriterion*, 
                            void*, 
                            const void* );
  static void AddATetrahedron( const double*, 
                               const double*, 
                               const double*, 
                               const double*, 
                               vtkEdgeSubdivisionCriterion*, 
                               void*, 
                               const void* );
  void OutputPoint( const double* );
  void OutputLine( const double*, const double* );
  void OutputTriangle( const double*, const double*, const double* );
  void OutputTetrahedron( const double*, 
                          const double*, 
                          const double*, 
                          const double* );
  //ETX

private:
  vtkTessellatorFilter( const vtkTessellatorFilter& ); // Not implemented.
  void operator = ( const vtkTessellatorFilter& ); // Not implemented.
};

//BTX
inline int vtkTessellatorFilter::GetOutputDimension() const 
{ 
  return this->OutputDimension; 
}
//ETX

#endif // __vtkTessellatorFilter_h
