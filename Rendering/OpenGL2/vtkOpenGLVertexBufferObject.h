/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkOpenGLVertexBufferObject_h
#define vtkOpenGLVertexBufferObject_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkOpenGLBufferObject.h"


/**
 * @brief OpenGL vertex buffer object
 *
 * OpenGL buffer object to store geometry and/or attribute data on the
 * GPU.
 */


// useful union for stuffing colors into a float
union vtkucfloat
{
  unsigned char c[4];
  float f;
};

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLVertexBufferObject :
  public vtkOpenGLBufferObject
{
public:
  static vtkOpenGLVertexBufferObject *New();
  vtkTypeMacro(vtkOpenGLVertexBufferObject, vtkOpenGLBufferObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Take the points, and pack them into this VBO. This currently
  // takes whatever the input type might be and packs them into a VBO using
  // floats for the vertices and normals, and unsigned char for the colors (if
  // the array is non-null).
  void CreateVBO(vtkPoints *points, unsigned int numPoints,
      vtkDataArray *normals,
      vtkDataArray *tcoords,
      unsigned char *colors, int colorComponents);

  void AppendVBO(vtkPoints *points, unsigned int numPoints,
      vtkDataArray *normals,
      vtkDataArray *tcoords,
      unsigned char *colors, int colorComponents);

  /**\brief Methods for VBO coordinate shift+scale-computation.
    *
    * By default, shift and scale vectors are enabled
    * whenever CreateVBO is called with points whose
    * bounds are many bbox-lengths away from the origin.
    *
    * Shifting and scaling may be completely disabled,
    * or manually specified, or left at the default.
    *
    * Manual specification is for the case when you
    * will be calling AppendVBO instead of just CreateVBO
    * and know better bounds than the what CreateVBO
    * might produce.
    *
    * The automatic method tells CreatVBO to compute shift and
    * scale vectors that remap the points to the unit cube.
    */
  enum ShiftScaleMethod {
    DISABLE_SHIFT_SCALE,  //!< Do not shift/scale point coordinates. Ever!
    AUTO_SHIFT_SCALE,     //!< The default, automatic computation.
    MANUAL_SHIFT_SCALE    //!< Manual shift/scale provided (for use with AppendVBO)
  };

  // Description:
  // Get the shift and scale vectors computed by CreateVBO;
  // or set the values CreateVBO and AppendVBO will use.
  // Note that the "Set" methods **must** be called before the
  // first time that CreateVBO or AppendVBO is invoked and
  // should never be called afterwards.
  //
  // The CoordShiftAndScaleMethod describes how the shift
  // and scale vectors are obtained (or that they should never
  // be used).
  // The GetCoordShiftAndScaleEnabled() method returns true if
  // a shift and scale are currently being applied (or false if not).
  //
  // The "Get" methods are used by the mapper to modify the world
  // and camera transformation matrices to match the scaling applied
  // to coordinates in the VBO.
  // CreateVBO only applies a shift and scale when the midpoint
  // of the point bounding-box is distant from the origin by a
  // factor of 10,000 or more relative to the size of the box
  // along any axis.
  //
  // For example, if the x coordinates of the points range from
  // 200,000 to 200,001 then the factor is
  // 200,000.5 / (200,001 - 200,000) = 2x10^5, which is larger
  // than 10,000 -- so the coordinates will be shifted and scaled.
  //
  // This is important as many OpenGL drivers use reduced precision
  // to hold point coordinates.
  //
  // These methods are used by the mapper to determine the
  // additional transform (if any) to apply to the rendering transform.
  vtkGetMacro(CoordShiftAndScaleEnabled,bool);
  vtkGetMacro(CoordShiftAndScaleMethod,ShiftScaleMethod);
  vtkGetVector3Macro(CoordShift,double);
  vtkGetVector3Macro(CoordScale,double);
  virtual void SetCoordShiftAndScaleMethod(ShiftScaleMethod meth);
  virtual void SetCoordShift(double x, double y, double z);
  virtual void SetCoordShift(const double s[3]);
  virtual void SetCoordScale(double sx, double sy, double sz);
  virtual void SetCoordScale(const double s[3]);

  // Sizes/offsets are all in bytes as OpenGL API expects them.
  size_t VertexCount; // Number of vertices in the VBO
  int Stride;       // The size of a complete vertex + attributes
  int VertexOffset; // Offset of the vertex
  int NormalOffset; // Offset of the normal
  int TCoordOffset; // Offset of the texture coordinates
  int TCoordComponents; // Number of texture dimensions
  int ColorOffset;  // Offset of the color
  int ColorComponents; // Number of color components
  std::vector<float> PackedVBO; // the data

protected:
  vtkOpenGLVertexBufferObject();
  ~vtkOpenGLVertexBufferObject();

  double CoordShift[3];
  double CoordScale[3];
  ShiftScaleMethod CoordShiftAndScaleMethod;
  bool CoordShiftAndScaleEnabled;

private:
  vtkOpenGLVertexBufferObject(const vtkOpenGLVertexBufferObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLVertexBufferObject&) VTK_DELETE_FUNCTION;
};

#endif
