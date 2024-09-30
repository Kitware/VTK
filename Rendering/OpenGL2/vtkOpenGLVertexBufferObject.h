// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkOpenGLVertexBufferObject_h
#define vtkOpenGLVertexBufferObject_h

#include "vtkOpenGLBufferObject.h"
#include "vtkPolyDataMapper.h"         // for ShiftScaleMethodType
#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkWeakPointer.h"            // For vtkWeakPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLVertexBufferObjectCache;

/**
 * @brief OpenGL vertex buffer object
 *
 * OpenGL buffer object to store geometry and/or attribute data on the
 * GPU.
 */

// useful union for stuffing colors into a float
union vtkFourByteUnion
{
  unsigned char c[4];
  short s[2];
  float f;
};

class vtkCamera;
class vtkProp3D;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLVertexBufferObject : public vtkOpenGLBufferObject
{
public:
  static vtkOpenGLVertexBufferObject* New();
  vtkTypeMacro(vtkOpenGLVertexBufferObject, vtkOpenGLBufferObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // set the VBOs data to the provided data array and upload
  // this can use a fast path of just passing the
  // data array pointer to OpenGL if it is suitable
  void UploadDataArray(vtkDataArray* array);

  // append a data array to this VBO, always
  // copies the data from the data array
  void AppendDataArray(vtkDataArray* array);

  // Get the mtime when this VBO was loaded
  vtkGetMacro(UploadTime, vtkTimeStamp);

  using ShiftScaleMethod = vtkPolyDataMapper::ShiftScaleMethodType;

  ///@{
  /**
   * These typed enums are available in class scope for convenience and backward compatibility.
   */
  static constexpr int DISABLE_SHIFT_SCALE = ShiftScaleMethod::DISABLE_SHIFT_SCALE;
  static constexpr int AUTO_SHIFT_SCALE = ShiftScaleMethod::AUTO_SHIFT_SCALE;
  static constexpr int ALWAYS_AUTO_SHIFT_SCALE = ShiftScaleMethod::ALWAYS_AUTO_SHIFT_SCALE;
  static constexpr int MANUAL_SHIFT_SCALE = ShiftScaleMethod::MANUAL_SHIFT_SCALE;
  static constexpr int AUTO_SHIFT = ShiftScaleMethod::AUTO_SHIFT;
  static constexpr int NEAR_PLANE_SHIFT_SCALE = ShiftScaleMethod::NEAR_PLANE_SHIFT_SCALE;
  static constexpr int FOCAL_POINT_SHIFT_SCALE = ShiftScaleMethod::FOCAL_POINT_SHIFT_SCALE;
  ///@}

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
  virtual bool GetCoordShiftAndScaleEnabled();
  virtual int GetCoordShiftAndScaleMethod();
  virtual void SetCoordShiftAndScaleMethod(int meth);
  virtual void SetShift(const std::vector<double>& shift);
  virtual void SetShift(double x, double y, double z);
  virtual void SetScale(const std::vector<double>& scale);
  virtual void SetScale(double x, double y, double z);
  virtual const std::vector<double>& GetShift();
  virtual const std::vector<double>& GetScale();

  // update the shift scale if needed
  void UpdateShiftScale(vtkDataArray* da);

  // Allow all vertex adjustments to be enabled/disabled
  //
  // When smaller objects are positioned on the side of a larger scene,
  // we don't want an individual mapper to try and center all its vertices.
  //
  // Complex scenes need to center the whole scene, not an individual mapper,
  // so allow applications to turn all these shifts off and manage the
  // float imprecision on their own.
  static void SetGlobalCoordShiftAndScaleEnabled(vtkTypeBool val);
  static void GlobalCoordShiftAndScaleEnabledOn() { SetGlobalCoordShiftAndScaleEnabled(1); }
  static void GlobalCoordShiftAndScaleEnabledOff() { SetGlobalCoordShiftAndScaleEnabled(0); }
  static vtkTypeBool GetGlobalCoordShiftAndScaleEnabled();

  // Set/Get the DataType to use for the VBO
  // As a side effect sets the DataTypeSize
  void SetDataType(int v);
  vtkGetMacro(DataType, int);

  // Get the size in bytes of the data type
  vtkGetMacro(DataTypeSize, unsigned int);

  // How many tuples in the VBO
  vtkGetMacro(NumberOfTuples, unsigned int);

  // How many components in the VBO
  vtkGetMacro(NumberOfComponents, unsigned int);

  // Set/Get the VBO stride in bytes
  vtkSetMacro(Stride, unsigned int);
  vtkGetMacro(Stride, unsigned int);

  // Get the underlying VBO array
  std::vector<float>& GetPackedVBO() { return this->PackedVBO; }

  // upload the current PackedVBO
  // only used by mappers that skip the VBOGroup support
  void UploadVBO();

  // VBOs may hold onto the cache, never the other way around
  void SetCache(vtkOpenGLVertexBufferObjectCache* cache);

  // used by mappers that support camera based shift scale
  virtual void SetCamera(vtkCamera* cam);
  virtual void SetProp3D(vtkProp3D* prop3d);

protected:
  vtkOpenGLVertexBufferObject();
  ~vtkOpenGLVertexBufferObject() override;

  std::vector<float> PackedVBO; // the data

  vtkTimeStamp UploadTime;

  unsigned int Stride; // The size of a complete tuple
  unsigned int NumberOfComponents;
  unsigned int NumberOfTuples;
  int DataType;
  unsigned int DataTypeSize;

  int CoordShiftAndScaleMethod;
  bool CoordShiftAndScaleEnabled;
  std::vector<double> Shift;
  std::vector<double> Scale;

  vtkOpenGLVertexBufferObjectCache* Cache;

  vtkWeakPointer<vtkCamera> Camera;
  vtkWeakPointer<vtkProp3D> Prop3D;

private:
  vtkOpenGLVertexBufferObject(const vtkOpenGLVertexBufferObject&) = delete;
  void operator=(const vtkOpenGLVertexBufferObject&) = delete;

  // Initialize static member that controls shifts and scales
  static vtkTypeBool GlobalCoordShiftAndScaleEnabled;
};

VTK_ABI_NAMESPACE_END
#endif
