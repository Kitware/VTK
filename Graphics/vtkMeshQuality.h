/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeshQuality.h
  Language:  C++
  Date:      $Date$ 
  Version:   $Revision$

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: dcthomp@sandia.gov,pppebay@ca.sandia.gov

=========================================================================*/
// .NAME vtkMeshQuality - Calculate measures of quality of a mesh
//
// .SECTION Description
// vtkMeshQuality computes one or more measures of (geometric)
// quality for each 2-D and 3-D cell (triangle, quadrilateral, tetrahedron,
// or hexahedron) of a mesh. These measures of quality are then averaged
// over the entire mesh. The minimum, average, maximum, and variance
// of quality for each type of cell is stored in the output mesh's FieldData.
// The FieldData arrays are named &ldquo;Mesh Triangle Quality,&rdquo
// &ldquo;Mesh Quadrilateral Quality,&rdquo &ldquo;Mesh Tetrahedron Quality,&rdquo
// and &ldquo;Mesh Hexahedron Quality,&rdquo. Each array has a single tuple
// with 5 components. The first 4 components are the quality statistics
// mentioned above; the final value is the number of cells of the given type.
// This final component makes aggregation of statistics for distributed
// mesh data possible.
//
// By default, the per-cell quality is added to the mesh's cell data, in
// an array named &ldquo;Quality.&rdquo; Cell types not supported by
// this filter will have an entry of 0. Use SaveCellQualityOff() to
// store only the final statistics.
//
// This version of the filter overtakes an older version written by
// Leila Baghdadi, Hanif Ladak, and David Steinman at the Imaging Research
// Labs, Robarts Research Institute. That version focused solely on
// tetrahedra. See the CompatibilityModeOn() member for information on
// how to make this filter behave like the previous implementation.
//
// .SECTION Caveats
// While more general than before, this class does not address many
// cell types, including wedges and pyramids in 3D and triangle strips
// and fans in 2D (among others).
//
#ifndef vtkMeshQuality_h
#define vtkMeshQuality_h

#include "vtkDataSetToDataSetFilter.h"

class vtkCell;

#define VTK_QUALITY_RADIUS_RATIO 0
#define VTK_QUALITY_ASPECT_RATIO 1
#define VTK_QUALITY_FROBENIUS_NORM 2
#define VTK_QUALITY_EDGE_RATIO 3

class VTK_GRAPHICS_EXPORT vtkMeshQuality : public vtkDataSetToDataSetFilter
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkMeshQuality,vtkDataSetToDataSetFilter);
  static vtkMeshQuality* New();

  // Description:
  // This variable controls whether or not cell quality is stored as
  // cell data in the resulting mesh or discarded (leaving only the
  // aggregate quality average of the entire mesh, recorded in the
  // FieldData).
  vtkSetMacro(SaveCellQuality,int);
  vtkGetMacro(SaveCellQuality,int);
  vtkBooleanMacro(SaveCellQuality,int);

  // Description:
  // Set/Get the particular estimator used to measure the quality of triangles.
  // The default is VTK_QUALITY_RADIUS_RATIO and valid values also include
  // VTK_QUALITY_ASPECT_RATIO, VTK_QUALITY_FROBENIUS_NORM, and VTK_QUALITY_EDGE_RATIO.
  vtkSetMacro(TriangleQualityMeasure,int);
  vtkGetMacro(TriangleQualityMeasure,int);
  void SetTriangleQualityMeasureToRadiusRatio() { this->SetTriangleQualityMeasure( VTK_QUALITY_RADIUS_RATIO ); }
  void SetTriangleQualityMeasureToAspectRatio() { this->SetTriangleQualityMeasure( VTK_QUALITY_ASPECT_RATIO ); }
  void SetTriangleQualityMeasureToFrobeniusNorm() { this->SetTriangleQualityMeasure( VTK_QUALITY_FROBENIUS_NORM ); }
  void SetTriangleQualityMeasureToEdgeRatio() { this->SetTriangleQualityMeasure( VTK_QUALITY_EDGE_RATIO ); }

  // Description:
  // Set/Get the particular estimator used to measure the quality of quadrilaterals.
  // The default is VTK_QUALITY_EDGE_RATIO and valid values also include
  // VTK_QUALITY_RADIUS_RATIO and VTK_QUALITY_ASPECT_RATIO.
  // Scope: Except for VTK_QUALITY_EDGE_RATIO, these estimators are intended for planar
  // quadrilaterals only; use at your own risk if you really want to assess non-planar
  // quadrilateral quality with those.
  vtkSetMacro(QuadQualityMeasure,int);
  vtkGetMacro(QuadQualityMeasure,int);
  void SetQuadQualityMeasureToRadiusRatio() { this->SetQuadQualityMeasure( VTK_QUALITY_RADIUS_RATIO ); }
  void SetQuadQualityMeasureToAspectRatio() { this->SetQuadQualityMeasure( VTK_QUALITY_ASPECT_RATIO ); }
  void SetQuadQualityMeasureToEdgeRatio() { this->SetQuadQualityMeasure( VTK_QUALITY_EDGE_RATIO ); }

  // Description:
  // Set/Get the particular estimator used to measure the quality of tetrahedra.
  // The default is VTK_QUALITY_RADIUS_RATIO and valid values also include
  // VTK_QUALITY_ASPECT_RATIO, VTK_QUALITY_FROBENIUS_NORM, and VTK_QUALITY_EDGE_RATIO.
  vtkSetMacro(TetQualityMeasure,int);
  vtkGetMacro(TetQualityMeasure,int);
  void SetTetQualityMeasureToRadiusRatio() { this->SetTetQualityMeasure( VTK_QUALITY_RADIUS_RATIO ); }
  void SetTetQualityMeasureToAspectRatio() { this->SetTetQualityMeasure( VTK_QUALITY_ASPECT_RATIO ); }
  void SetTetQualityMeasureToFrobeniusNorm() { this->SetTetQualityMeasure( VTK_QUALITY_FROBENIUS_NORM ); }
  void SetTetQualityMeasureToEdgeRatio() { this->SetTetQualityMeasure( VTK_QUALITY_EDGE_RATIO ); }

  // Description:
  // Set/Get the particular estimator used to measure the quality of hexahedra.
  // The default is VTK_QUALITY_RADIUS_RATIO and valid values also include
  // VTK_QUALITY_ASPECT_RATIO.
  vtkSetMacro(HexQualityMeasure,int);
  vtkGetMacro(HexQualityMeasure,int);
  void SetHexQualityMeasureToRadiusRatio() { this->SetHexQualityMeasure( VTK_QUALITY_RADIUS_RATIO ); }
  void SetHexQualityMeasureToAspectRatio() { this->SetHexQualityMeasure( VTK_QUALITY_ASPECT_RATIO ); }

  // Description:
  // These are static functions used to calculate the individual quality measures.
  // They assume that you pass the correct type of cell -- no type checking is
  // performed because these methods are called from the inner loop of the Execute()
  // member function.
  static double TriangleRadiusRatio( vtkCell* cell );
  static double TriangleAspectRatio( vtkCell* cell );
  static double TriangleFrobeniusNorm( vtkCell* cell );
  static double TriangleEdgeRatio( vtkCell* cell );
  static double QuadRadiusRatio( vtkCell* cell );
  static double QuadAspectRatio( vtkCell* cell );
  static double QuadFrobeniusNorm( vtkCell* cell );
  static double QuadEdgeRatio( vtkCell* cell );
  static double TetRadiusRatio( vtkCell* cell );
  static double TetAspectRatio( vtkCell* cell );
  static double TetFrobeniusNorm( vtkCell* cell );
  static double TetEdgeRatio( vtkCell* cell );
  static double HexahedronQuality( vtkCell* cell );

  // Description:
  // These methods are deprecated. Use Get/SetSaveCellQuality() instead.
  //
  // Formerly, SetRatio could be used to disable computation
  // of the tetrahedral radius ratio so that volume alone could be computed.
  // Now, cell quality is always computed, but you may decide not
  // to store the result for each cell.
  // This allows average cell quality of a mesh to be
  // calculated without requiring per-cell storage.
  virtual void SetRatio( int r ) { this->SetSaveCellQuality( r ); }
  int GetRatio() { return this->GetSaveCellQuality(); }
  vtkBooleanMacro(Ratio,int);

  // Description:
  // These methods are deprecated. The functionality of computing cell
  // volume is being removed until it can be computed for any 3D cell.
  // (The previous implementation only worked for tetrahedra.)
  //
  // For now, turning on the volume computation will put this
  // filter into "compatibility mode," where tetrahedral cell
  // volume is stored in first component of each output tuple and
  // the radius ratio is stored in the second component. You may
  // also use CompatibilityModeOn()/Off() to enter this mode.
  // In this mode, cells other than tetrahedra will have report
  // a volume of 0.0 (if volume computation is enabled).
  //
  // By default, volume computation is disabled and compatibility
  // mode is off, since it does not make a lot of sense for
  // meshes with non-tetrahedral cells.
  virtual void SetVolume( int cv )
    {
    if ( ! ((cv != 0) ^ (this->Volume != 0)) )
      {
      return;
      }
    this->Modified();
    this->Volume = cv;
    if ( this->Volume )
      {
      this->CompatibilityModeOn();
      }
    }
  int GetVolume()
    {
    return this->Volume;
    }
  vtkBooleanMacro(Volume,int);

  // Description:
  // CompatibilityMode governs whether, when both a quality measure
  // and cell volume are to be stored as cell data, the two values
  // are stored in a single array. When compatibility mode is off
  // (the default), two separate arrays are used -- one labeled
  // "Quality" and the other labeled "Volume".
  // When compatibility mode is on, both values are stored in a
  // single array, with volume as the first component and quality
  // as the second component.
  //
  // Enabling CompatibilityMode changes the default tetrahedral
  // quality measure to VTK_QUALITY_RADIUS_RATIO and turns volume
  // computation on. (This matches the default behavior of the
  // initial implementation of vtkMeshQuality.) You may change
  // quality measure and volume computation without leaving
  // compatibility mode.
  //
  // Disabling compatibility mode does not affect the current
  // volume computation or tetrahedral quality measure settings. 
  //
  // The final caveat to CompatibilityMode is that regardless of
  // its setting, the resulting array will be of type vtkDoubleArray
  // rather than the original vtkFloatArray.
  // This is a safety measure to keep the authors from
  // diving off of the Combinatorial Coding Cliff into
  // Certain Insanity.
  virtual void SetCompatibilityMode( int cm )
    {
    if ( !((cm != 0) ^ (this->CompatibilityMode != 0)) )
      {
      return;
      }
    this->CompatibilityMode = cm;
    this->Modified();
    if ( this->CompatibilityMode )
      {
      this->Volume = 1;
      this->TetQualityMeasure = VTK_QUALITY_RADIUS_RATIO;
      }
    }
  vtkGetMacro(CompatibilityMode,int);
  vtkBooleanMacro(CompatibilityMode,int);

protected:
  vtkMeshQuality();
  virtual ~vtkMeshQuality();

  virtual void Execute();

  int SaveCellQuality;
  int TriangleQualityMeasure;
  int QuadQualityMeasure;
  int TetQualityMeasure;
  int HexQualityMeasure;

  int CompatibilityMode;
  int Volume;

private:
  vtkMeshQuality( const vtkMeshQuality& ); // Not implemented.
  void operator = ( const vtkMeshQuality& ); // Not implemented.
};

#endif // vtkMeshQuality_h
