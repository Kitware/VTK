/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

// .SECTION See Also
// vtkTensorGlyph

#ifndef __vtkGlyph3D_h
#define __vtkGlyph3D_h

#include "vtkDataSetToPolyDataFilter.h"

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

class VTK_EXPORT vtkGlyph3D : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeMacro(vtkGlyph3D,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct object with scaling on, scaling mode is by scalar value, 
  // scale factor = 1.0, the range is (0,1), orient geometry is on, and
  // orientation is by vector. Clamping and indexing are turned off. No
  // initial sources are defined.
  static vtkGlyph3D *New();

  // Description:
  // Get the number of source objects used to define the glyph
  // table. Specify the number of sources before defining a table of glyphs.
  void SetNumberOfSources(int num);
  int GetNumberOfSources();

  // Description:
  // Set the source to use for he glyph.
  void SetSource(vtkPolyData *pd) {this->SetSource(0,pd);};

  // Description:
  // Specify a source object at a specified table location.
  void SetSource(int id, vtkPolyData *pd);

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
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Specify range to map scalar values into.
  vtkSetVector2Macro(Range,float);
  vtkGetVectorMacro(Range,float,2);

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
  // the table of glyphs is used.
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
protected:
  vtkGlyph3D();
  ~vtkGlyph3D();
  vtkGlyph3D(const vtkGlyph3D&);
  void operator=(const vtkGlyph3D&);

  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *output);

  int NumberOfSources; // Number of source objects
  vtkPolyData **Source; // Geometry to copy to each point
  int Scaling; // Determine whether scaling of geometry is performed
  int ScaleMode; // Scale by scalar value or vector magnitude
  int ColorMode; // new scalars based on scale, scalar or vector
  float ScaleFactor; // Scale factor to use to scale geometry
  float Range[2]; // Range to use to perform scalar scaling
  int Orient; // boolean controls whether to "orient" data
  int VectorMode; // Orient/scale via normal or via vector data
  int Clamping; // whether to clamp scale factor
  int IndexMode; // what to use to index into glyph table
  int GeneratePointIds; // produce input points ids for each output point
  char *PointIdsName;

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
