/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGlyph3D - copy oriented and scaled glyph geometry to every input point
// .SECTION Description
// vtkGlyph3D is a filter that copies a geometric representation (called
// a glyph) to every point in the input dataset. The glyph is defined with
// polygonal data from a source filter input. The glyph may be oriented
// along the input vectors or normals, and it may be scaled according to
// scalar data or vector magnitude. More than one glyph may be used by
// creating a table of source objects, each defining a different glyph. If a
// table of glyphs is defined, then the table can be indexed into by using
// either scalar value or vector magnitude.
//
// To use this object you'll have to provide an input dataset and a source
// to define the glyph. Then decide whether you want to scale the glyph and
// how to scale the glyph (using scalar value or vector magnitude). Next
// decide whether you want to orient the glyph, and whether to use the
// vector data or normal data to orient it. Finally, decide whether to use a
// table of glyphs, or just a single glyph. If you use a table of glyphs,
// you'll have to decide whether to index into it with scalar value or with
// vector magnitude.
//
// .SECTION Caveats
// The scaling of the glyphs is controlled by the ScaleFactor ivar multiplied
// by the scalar value at each point (if VTK_SCALE_BY_SCALAR is set), or
// multiplied by the vector magnitude (if VTK_SCALE_BY_VECTOR is set),
// Alternatively (if VTK_SCALE_BY_VECTORCOMPONENTS is set), the scaling
// may be specified for x,y,z using the vector components. The
// scale factor can be further controlled by enabling clamping using the
// Clamping ivar. If clamping is enabled, the scale is normalized by the
// Range ivar, and then multiplied by the scale factor. The normalization
// process includes clamping the scale value between (0,1).
//
// Typically this object operates on input data with scalar and/or vector
// data. However, scalar and/or vector aren't necessary, and it can be used
// to copy data from a single source to each point. In this case the scale
// factor can be used to uniformly scale the glyphs.
//
// The object uses "vector" data to scale glyphs, orient glyphs, and/or index
// into a table of glyphs. You can choose to use either the vector or normal
// data at each input point. Use the method SetVectorModeToUseVector() to use
// the vector input data, and SetVectorModeToUseNormal() to use the
// normal input data.
//
// If you do use a table of glyphs, make sure to set the Range ivar to make
// sure the index into the glyph table is computed correctly.
//
// You can turn off scaling of the glyphs completely by using the Scaling
// ivar. You can also turn off scaling due to data (either vector or scalar)
// by using the SetScaleModeToDataScalingOff() method.
//
// You can set what arrays to use for the scalars, vectors, normals, and
// color scalars by using the SetInputArrayToProcess methods in
// vtkAlgorithm. The first array is scalars, the next vectors, the next
// normals and finally color scalars.

// .SECTION See Also
// vtkTensorGlyph

#ifndef vtkGlyph3D_h
#define vtkGlyph3D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_SCALE_BY_SCALAR 0
#define VTK_SCALE_BY_VECTOR 1
#define VTK_SCALE_BY_VECTORCOMPONENTS 2
#define VTK_DATA_SCALING_OFF 3

#define VTK_COLOR_BY_SCALE  0
#define VTK_COLOR_BY_SCALAR 1
#define VTK_COLOR_BY_VECTOR 2

#define VTK_USE_VECTOR 0
#define VTK_USE_NORMAL 1
#define VTK_VECTOR_ROTATION_OFF 2

#define VTK_INDEXING_OFF 0
#define VTK_INDEXING_BY_SCALAR 1
#define VTK_INDEXING_BY_VECTOR 2

class vtkTransform;

class VTKFILTERSCORE_EXPORT vtkGlyph3D : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkGlyph3D,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct object with scaling on, scaling mode is by scalar value,
  // scale factor = 1.0, the range is (0,1), orient geometry is on, and
  // orientation is by vector. Clamping and indexing are turned off. No
  // initial sources are defined.
  static vtkGlyph3D *New();

  // Description:
  // Set the source to use for the glyph.
  // Note that this method does not connect the pipeline. The algorithm will
  // work on the input data as it is without updating the producer of the data.
  // See SetSourceConnection for connecting the pipeline.
  void SetSourceData(vtkPolyData *pd) {this->SetSourceData(0,pd);};

  // Description:
  // Specify a source object at a specified table location.
  // Note that this method does not connect the pipeline. The algorithm will
  // work on the input data as it is without updating the producer of the data.
  // See SetSourceConnection for connecting the pipeline.
  void SetSourceData(int id, vtkPolyData *pd);

  // Description:
  // Specify a source object at a specified table location. New style.
  // Source connection is stored in port 1. This method is equivalent
  // to SetInputConnection(1, id, outputPort).
  void SetSourceConnection(int id, vtkAlgorithmOutput* algOutput);
  void SetSourceConnection(vtkAlgorithmOutput* algOutput)
    {
      this->SetSourceConnection(0, algOutput);
    }

  // Description:
  // Get a pointer to a source object at a specified table location.
  vtkPolyData *GetSource(int id=0);

  // Description:
  // Turn on/off scaling of source geometry.
  vtkSetMacro(Scaling,int);
  vtkBooleanMacro(Scaling,int);
  vtkGetMacro(Scaling,int);

  // Description:
  // Either scale by scalar or by vector/normal magnitude.
  vtkSetMacro(ScaleMode,int);
  vtkGetMacro(ScaleMode,int);
  void SetScaleModeToScaleByScalar()
    {this->SetScaleMode(VTK_SCALE_BY_SCALAR);};
  void SetScaleModeToScaleByVector()
    {this->SetScaleMode(VTK_SCALE_BY_VECTOR);};
  void SetScaleModeToScaleByVectorComponents()
    {this->SetScaleMode(VTK_SCALE_BY_VECTORCOMPONENTS);};
  void SetScaleModeToDataScalingOff()
    {this->SetScaleMode(VTK_DATA_SCALING_OFF);};
  const char *GetScaleModeAsString();

  // Description:
  // Either color by scale, scalar or by vector/normal magnitude.
  vtkSetMacro(ColorMode,int);
  vtkGetMacro(ColorMode,int);
  void SetColorModeToColorByScale()
    {this->SetColorMode(VTK_COLOR_BY_SCALE);};
  void SetColorModeToColorByScalar()
    {this->SetColorMode(VTK_COLOR_BY_SCALAR);};
  void SetColorModeToColorByVector()
    {this->SetColorMode(VTK_COLOR_BY_VECTOR);};
  const char *GetColorModeAsString();

  // Description:
  // Specify scale factor to scale object by.
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);

  // Description:
  // Specify range to map scalar values into.
  vtkSetVector2Macro(Range,double);
  vtkGetVectorMacro(Range,double,2);

  // Description:
  // Turn on/off orienting of input geometry along vector/normal.
  vtkSetMacro(Orient,int);
  vtkBooleanMacro(Orient,int);
  vtkGetMacro(Orient,int);

  // Description:
  // Turn on/off clamping of "scalar" values to range. (Scalar value may be
  //  vector magnitude if ScaleByVector() is enabled.)
  vtkSetMacro(Clamping,int);
  vtkBooleanMacro(Clamping,int);
  vtkGetMacro(Clamping,int);

  // Description:
  // Specify whether to use vector or normal to perform vector operations.
  vtkSetMacro(VectorMode,int);
  vtkGetMacro(VectorMode,int);
  void SetVectorModeToUseVector() {this->SetVectorMode(VTK_USE_VECTOR);};
  void SetVectorModeToUseNormal() {this->SetVectorMode(VTK_USE_NORMAL);};
  void SetVectorModeToVectorRotationOff()
    {this->SetVectorMode(VTK_VECTOR_ROTATION_OFF);};
  const char *GetVectorModeAsString();

  // Description:
  // Index into table of sources by scalar, by vector/normal magnitude, or
  // no indexing. If indexing is turned off, then the first source glyph in
  // the table of glyphs is used. Note that indexing mode will only use the
  // InputScalarsSelection array and not the InputColorScalarsSelection
  // as the scalar source if an array is specified.
  vtkSetMacro(IndexMode,int);
  vtkGetMacro(IndexMode,int);
  void SetIndexModeToScalar() {this->SetIndexMode(VTK_INDEXING_BY_SCALAR);};
  void SetIndexModeToVector() {this->SetIndexMode(VTK_INDEXING_BY_VECTOR);};
  void SetIndexModeToOff() {this->SetIndexMode(VTK_INDEXING_OFF);};
  const char *GetIndexModeAsString();

  // Description:
  // Enable/disable the generation of point ids as part of the output. The
  // point ids are the id of the input generating point. The point ids are
  // stored in the output point field data and named "InputPointIds". Point
  // generation is useful for debugging and pick operations.
  vtkSetMacro(GeneratePointIds,int);
  vtkGetMacro(GeneratePointIds,int);
  vtkBooleanMacro(GeneratePointIds,int);

  // Description:
  // Set/Get the name of the PointIds array if generated. By default the Ids
  // are named "InputPointIds", but this can be changed with this function.
  vtkSetStringMacro(PointIdsName);
  vtkGetStringMacro(PointIdsName);

  // Description:
  // Enable/disable the generation of cell data as part of the output.
  // The cell data at each cell will match the point data of the input
  // at the glyphed point.
  vtkSetMacro(FillCellData,int);
  vtkGetMacro(FillCellData,int);
  vtkBooleanMacro(FillCellData,int);

  // Description:
  // This can be overwritten by subclass to return 0 when a point is
  // blanked. Default implementation is to always return 1;
  virtual int IsPointVisible(vtkDataSet*, vtkIdType) {return 1;};

  // Description:
  // When set, this is use to transform the source polydata before using it to
  // generate the glyph. This is useful if one wanted to reorient the source,
  // for example.
  void SetSourceTransform(vtkTransform*);
  vtkGetObjectMacro(SourceTransform, vtkTransform);

  // Description:
  // Overridden to include SourceTransform's MTime.
  virtual unsigned long GetMTime();

protected:
  vtkGlyph3D();
  ~vtkGlyph3D();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);

  vtkPolyData* GetSource(int idx, vtkInformationVector *sourceInfo);

  // Description:
  // Method called in RequestData() to do the actual data processing. This will
  // glyph the \c input, filling up the \c output based on the filter
  // parameters.
  virtual bool Execute(vtkDataSet* input,
    vtkInformationVector* sourceVector,
    vtkPolyData* output, int requestedGhostLevel);

  vtkPolyData **Source; // Geometry to copy to each point
  int Scaling; // Determine whether scaling of geometry is performed
  int ScaleMode; // Scale by scalar value or vector magnitude
  int ColorMode; // new scalars based on scale, scalar or vector
  double ScaleFactor; // Scale factor to use to scale geometry
  double Range[2]; // Range to use to perform scalar scaling
  int Orient; // boolean controls whether to "orient" data
  int VectorMode; // Orient/scale via normal or via vector data
  int Clamping; // whether to clamp scale factor
  int IndexMode; // what to use to index into glyph table
  int GeneratePointIds; // produce input points ids for each output point
  int FillCellData; // whether to fill output cell data
  char *PointIdsName;
  vtkTransform* SourceTransform;

private:
  vtkGlyph3D(const vtkGlyph3D&);  // Not implemented.
  void operator=(const vtkGlyph3D&);  // Not implemented.
};

// Description:
// Return the method of scaling as a descriptive character string.
inline const char *vtkGlyph3D::GetScaleModeAsString(void)
{
  if ( this->ScaleMode == VTK_SCALE_BY_SCALAR )
    {
    return "ScaleByScalar";
    }
  else if ( this->ScaleMode == VTK_SCALE_BY_VECTOR )
    {
    return "ScaleByVector";
    }
  else
    {
    return "DataScalingOff";
    }
}

// Description:
// Return the method of coloring as a descriptive character string.
inline const char *vtkGlyph3D::GetColorModeAsString(void)
{
  if ( this->ColorMode == VTK_COLOR_BY_SCALAR )
    {
    return "ColorByScalar";
    }
  else if ( this->ColorMode == VTK_COLOR_BY_VECTOR )
    {
    return "ColorByVector";
    }
  else
    {
    return "ColorByScale";
    }
}

// Description:
// Return the vector mode as a character string.
inline const char *vtkGlyph3D::GetVectorModeAsString(void)
{
  if ( this->VectorMode == VTK_USE_VECTOR)
    {
    return "UseVector";
    }
  else if ( this->VectorMode == VTK_USE_NORMAL)
    {
    return "UseNormal";
    }
  else
    {
    return "VectorRotationOff";
    }
}

// Description:
// Return the index mode as a character string.
inline const char *vtkGlyph3D::GetIndexModeAsString(void)
{
  if ( this->IndexMode == VTK_INDEXING_OFF)
    {
    return "IndexingOff";
    }
  else if ( this->IndexMode == VTK_INDEXING_BY_SCALAR)
    {
    return "IndexingByScalar";
    }
  else
    {
    return "IndexingByVector";
    }
}

#endif
