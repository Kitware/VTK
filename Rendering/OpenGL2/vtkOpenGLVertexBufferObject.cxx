/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkObjectFactory.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkPoints.h"

#include "vtk_glew.h"

vtkStandardNewMacro(vtkOpenGLVertexBufferObject);

vtkOpenGLVertexBufferObject::vtkOpenGLVertexBufferObject()
{
  this->Cache = nullptr;
  this->Stride = 0;
  this->NumberOfComponents = 0;
  this->NumberOfTuples = 0;
  this->DataType = 0;
  this->DataTypeSize = 0;
  this->SetType(vtkOpenGLBufferObject::ArrayBuffer);
  this->CoordShiftAndScaleMethod = DISABLE_SHIFT_SCALE;
  this->CoordShiftAndScaleEnabled = false;
}

vtkOpenGLVertexBufferObject::~vtkOpenGLVertexBufferObject()
{
  if (this->Cache)
  {
    this->Cache->RemoveVBO(this);
    this->Cache->Delete();
    this->Cache = nullptr;
  }
}

vtkCxxSetObjectMacro(vtkOpenGLVertexBufferObject, Cache, vtkOpenGLVertexBufferObjectCache);

void vtkOpenGLVertexBufferObject::SetCoordShiftAndScaleMethod(ShiftScaleMethod meth)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting CoordShiftAndScaleMethod to "
                << meth);
  if (this->CoordShiftAndScaleMethod != meth)
  {
    if (!this->PackedVBO.empty())
    {
      vtkErrorMacro("SetCoordShiftAndScaleMethod() called with non-empty VBO! Ignoring.");
      return;
    }

    this->CoordShiftAndScaleMethod = meth;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObject::SetShift(const std::vector<double>& shift)
{
  if (!this->PackedVBO.empty())
  {
    vtkErrorMacro("SetShift() called with non-empty VBO! Ignoring.");
    return;
  }
  if (shift == this->Shift)
  {
    return;
  }

  this->Modified();
  this->Shift.clear();
  this->CoordShiftAndScaleEnabled = false;
  for (unsigned int i = 0; i < shift.size(); ++i)
  {
    this->Shift.push_back(shift.at(i));
    if (this->Shift.at(i) != 0.0)
    {
      this->CoordShiftAndScaleEnabled = true;
    }
  }
  for (unsigned int i = 0; i < this->Scale.size(); ++i)
  {
    if (this->Scale.at(i) != 1.0)
    {
      this->CoordShiftAndScaleEnabled = true;
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObject::SetScale(const std::vector<double>& scale)
{
  if (!this->PackedVBO.empty())
  {
    vtkErrorMacro("SetScale() called with non-empty VBO! Ignoring.");
    return;
  }
  if (scale == this->Scale)
  {
    return;
  }

  this->Modified();
  this->Scale.clear();
  this->CoordShiftAndScaleEnabled = false;
  for (unsigned int i = 0; i < scale.size(); ++i)
  {
    this->Scale.push_back(scale.at(i));
    if (this->Scale.at(i) != 1.0)
    {
      this->CoordShiftAndScaleEnabled = true;
    }
  }
  for (unsigned int i = 0; i < this->Shift.size(); ++i)
  {
    if (this->Shift.at(i) != 0.0)
    {
      this->CoordShiftAndScaleEnabled = true;
      return;
    }
  }
}

//-----------------------------------------------------------------------------
const std::vector<double>& vtkOpenGLVertexBufferObject::GetShift()
{
  return this->Shift;
}

//-----------------------------------------------------------------------------
const std::vector<double>& vtkOpenGLVertexBufferObject::GetScale()
{
  return this->Scale;
}

namespace
{

template <typename destType>
class vtkAppendVBOWorker
{
public:
  vtkAppendVBOWorker(vtkOpenGLVertexBufferObject* vbo, unsigned int offset,
    const std::vector<double>& shift, const std::vector<double>& scale)
    : VBO(vbo)
    , Offset(offset)
    , Shift(shift)
    , Scale(scale)
  {
  }

  vtkOpenGLVertexBufferObject* VBO;
  unsigned int Offset;
  const std::vector<double>& Shift;
  const std::vector<double>& Scale;

  // faster path
  template <typename ValueType>
  void operator()(vtkAOSDataArrayTemplate<ValueType>* src);

  // generic path
  template <typename DataArray>
  void operator()(DataArray* array);

  vtkAppendVBOWorker<destType>& operator=(const vtkAppendVBOWorker&) = delete;
};

template <typename destType>
template <typename ValueType>
void vtkAppendVBOWorker<destType>::operator()(vtkAOSDataArrayTemplate<ValueType>* src)
{
  // Check if shift&scale
  if (this->VBO->GetCoordShiftAndScaleEnabled() &&
    (this->Shift.empty() || this->Scale.empty() || (this->Shift.size() != this->Scale.size())))
  {
    return; // fixme: should handle error here?
  }

  destType* VBOit = reinterpret_cast<destType*>(&this->VBO->GetPackedVBO()[this->Offset]);

  ValueType* input = src->Begin();
  unsigned int numComps = this->VBO->GetNumberOfComponents();
  unsigned int numTuples = src->GetNumberOfTuples();

  // compute extra padding required
  int bytesNeeded = this->VBO->GetDataTypeSize() * this->VBO->GetNumberOfComponents();
  int extraComponents = ((4 - (bytesNeeded % 4)) % 4) / this->VBO->GetDataTypeSize();

  // If not shift & scale
  if (!this->VBO->GetCoordShiftAndScaleEnabled())
  {
    // if no padding and no type conversion then memcpy
    if (extraComponents == 0 && src->GetDataType() == this->VBO->GetDataType())
    {
      memcpy(VBOit, input, this->VBO->GetDataTypeSize() * numComps * numTuples);
    }
    else
    {
      for (unsigned int i = 0; i < numTuples; ++i)
      {
        for (unsigned int j = 0; j < numComps; j++)
        {
          *(VBOit++) = *(input++);
        }
        VBOit += extraComponents;
      }
    }
  }
  else
  {
    for (unsigned int i = 0; i < numTuples; ++i)
    {
      for (unsigned int j = 0; j < numComps; j++)
      {
        *(VBOit++) = (*(input++) - this->Shift.at(j)) * this->Scale.at(j);
      }
      VBOit += extraComponents;
    }
  } // end if shift*scale
}

template <typename destType>
template <typename DataArray>
void vtkAppendVBOWorker<destType>::operator()(DataArray* array)
{
  // Check if shift&scale
  if (this->VBO->GetCoordShiftAndScaleEnabled() &&
    (this->Shift.empty() || this->Scale.empty() || (this->Shift.size() != this->Scale.size())))
  {
    return; // fixme: should handle error here?
  }

  destType* VBOit = reinterpret_cast<destType*>(&this->VBO->GetPackedVBO()[this->Offset]);

  const auto dataRange = vtk::DataArrayTupleRange(array);

  // compute extra padding required
  int bytesNeeded = this->VBO->GetDataTypeSize() * this->VBO->GetNumberOfComponents();
  int extraComponents = ((4 - (bytesNeeded % 4)) % 4) / this->VBO->GetDataTypeSize();

  // If not shift & scale
  if (!this->VBO->GetCoordShiftAndScaleEnabled())
  {
    for (const auto tuple : dataRange)
    {
      VBOit = std::copy(tuple.cbegin(), tuple.cend(), VBOit);
      VBOit += extraComponents;
    }
  }
  else
  {
    for (const auto tuple : dataRange)
    {
      for (int j = 0; j < tuple.size(); ++j)
      {
        *(VBOit++) = (tuple[j] - this->Shift[j]) * this->Scale[j];
      }
      VBOit += extraComponents;
    }
  } // end if shift*scale
}

} // end anon namespace

void vtkOpenGLVertexBufferObject::SetDataType(int v)
{
  if (this->DataType == v)
  {
    return;
  }

  this->DataType = v;
  this->DataTypeSize = vtkAbstractArray::GetDataTypeSize(this->DataType);

  this->Modified();
}

void vtkOpenGLVertexBufferObject::UploadDataArray(vtkDataArray* array)
{
  if (array == nullptr || array->GetNumberOfTuples() == 0)
  {
    return;
  }

  this->NumberOfComponents = array->GetNumberOfComponents();

  // Set stride (size of a tuple in bytes on the VBO) based on the data
  int bytesNeeded = this->NumberOfComponents * this->DataTypeSize;
  int extraComponents =
    (this->DataTypeSize > 0) ? ((4 - (bytesNeeded % 4)) % 4) / this->DataTypeSize : 0;
  this->Stride = (this->NumberOfComponents + extraComponents) * this->DataTypeSize;

  // Can we use the fast path?
  // have to compute auto shift scale first to know if we can use
  // the fast path
  bool useSS = false;
  if (this->GetCoordShiftAndScaleMethod() == vtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE)
  {
    // first compute the diagonal size and distance from origin for this data
    // we use squared values to avoid sqrt calls
    double diag2 = 0.0;
    double dist2 = 0.0;
    for (int i = 0; i < array->GetNumberOfComponents(); ++i)
    {
      double range[2];
      array->GetRange(range, i);
      double delta = range[1] - range[0];
      diag2 += (delta * delta);
      double dshift = 0.5 * (range[1] + range[0]);
      dist2 += (dshift * dshift);
    }
    // if the data is far from the origin relative to it's size
    // or if the size itself is huge when not far from the origin
    // or if it is a point, but far from the origin
    if ((diag2 > 0 && (fabs(dist2) / diag2 > 1.0e6 || fabs(log10(diag2)) > 3.0)) ||
      (diag2 == 0 && dist2 > 1.0e6))
    {
      useSS = true;
    }
    else if (this->CoordShiftAndScaleEnabled)
    {
      // make sure to reset if we go far away and come back.
      this->CoordShiftAndScaleEnabled = false;
      this->Shift.clear();
      this->Scale.clear();
    }
  }
  if (useSS ||
    this->GetCoordShiftAndScaleMethod() == vtkOpenGLVertexBufferObject::ALWAYS_AUTO_SHIFT_SCALE)
  {
    std::vector<double> shift;
    std::vector<double> scale;
    for (int i = 0; i < array->GetNumberOfComponents(); ++i)
    {
      double range[2];
      array->GetRange(range, i);
      shift.push_back(0.5 * (range[1] + range[0]));
      double delta = range[1] - range[0];
      if (delta > 0)
      {
        scale.push_back(1.0 / delta);
      }
      else
      {
        scale.push_back(1.0);
      }
    }
    this->SetShift(shift);
    this->SetScale(scale);
  }

  // can we use the fast path and just upload the raw array?
  if (!this->GetCoordShiftAndScaleEnabled() && this->DataType == array->GetDataType() &&
    extraComponents == 0)
  {
    this->NumberOfTuples = array->GetNumberOfTuples();
    this->PackedVBO.resize(0);
    this->Upload(reinterpret_cast<float*>(array->GetVoidPointer(0)),
      this->NumberOfTuples * this->Stride / sizeof(float), vtkOpenGLBufferObject::ArrayBuffer);
    this->UploadTime.Modified();
  }
  // otherwise use a worker to build the array to upload
  else
  {
    this->NumberOfTuples = array->GetNumberOfTuples();

    // Resize VBO to fit new array
    this->PackedVBO.resize(this->NumberOfTuples * this->Stride / sizeof(float));

    // Dispatch based on the array data type
    typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes> Dispatcher;
    bool result = true;
    switch (this->DataType)
    {
      case VTK_FLOAT:
      {
        vtkAppendVBOWorker<float> worker(this, 0, this->GetShift(), this->GetScale());
        // result = Dispatcher::Execute(array, worker);
        if (!Dispatcher::Execute(array, worker))
        {
          worker(array);
        }
        break;
      }
      case VTK_UNSIGNED_CHAR:
      {
        vtkAppendVBOWorker<unsigned char> worker(this, 0, this->GetShift(), this->GetScale());
        // result = Dispatcher::Execute(array, worker);
        if (!Dispatcher::Execute(array, worker))
        {
          worker(array);
        }
        break;
      }
    }

    if (!result)
    {
      vtkErrorMacro(<< "Error filling VBO.");
    }

    this->Modified();
    this->UploadVBO();
  }
}

void vtkOpenGLVertexBufferObject::AppendDataArray(vtkDataArray* array)
{
  if (array == nullptr || array->GetNumberOfTuples() == 0)
  {
    return;
  }

  if (this->NumberOfTuples == 0)
  {
    // Set stride (size of a tuple in bytes on the VBO) based on the data
    this->NumberOfComponents = array->GetNumberOfComponents();
    int bytesNeeded = this->NumberOfComponents * this->DataTypeSize;
    int extraComponents =
      (this->DataTypeSize > 0) ? ((4 - (bytesNeeded % 4)) % 4) / this->DataTypeSize : 0;
    this->Stride = (this->NumberOfComponents + extraComponents) * this->DataTypeSize;
  }
  else if (static_cast<int>(this->NumberOfComponents) != array->GetNumberOfComponents())
  {
    vtkErrorMacro("Attempt to append an array to a VBO with a different number of components");
  }

  int offset = this->NumberOfTuples * this->Stride / sizeof(float);

  // compute auto Shift & Scale on first block
  if (offset == 0)
  {
    bool useSS = false;
    if (this->GetCoordShiftAndScaleMethod() == vtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE)
    {
      for (int i = 0; i < array->GetNumberOfComponents(); ++i)
      {
        double range[2];
        array->GetRange(range, i);
        double dshift = 0.5 * (range[1] + range[0]);
        double delta = range[1] - range[0];
        if (delta > 0 && (fabs(dshift) / delta > 1.0e3 || fabs(log10(delta)) > 3.0))
        {
          useSS = true;
          break;
        }
      }
    }
    if (useSS ||
      this->GetCoordShiftAndScaleMethod() == vtkOpenGLVertexBufferObject::ALWAYS_AUTO_SHIFT_SCALE)
    {
      std::vector<double> shift;
      std::vector<double> scale;
      for (int i = 0; i < array->GetNumberOfComponents(); ++i)
      {
        double range[2];
        array->GetRange(range, i);
        shift.push_back(0.5 * (range[1] + range[0]));
        double delta = range[1] - range[0];
        if (delta > 0)
        {
          scale.push_back(1.0 / delta);
        }
        else
        {
          scale.push_back(1.0);
        }
      }
      this->SetShift(shift);
      this->SetScale(scale);
    }
  }

  this->NumberOfTuples += array->GetNumberOfTuples();

  // Resize VBO to fit new array
  this->PackedVBO.resize(this->NumberOfTuples * this->Stride / sizeof(float));

  // Dispatch based on the array data type
  typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes> Dispatcher;
  bool result = true;
  switch (this->DataType)
  {
    case VTK_FLOAT:
    {
      vtkAppendVBOWorker<float> worker(this, offset, this->GetShift(), this->GetScale());
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      break;
    }
    case VTK_UNSIGNED_CHAR:
    {
      vtkAppendVBOWorker<unsigned char> worker(this, offset, this->GetShift(), this->GetScale());
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      break;
    }
  }

  if (!result)
  {
    vtkErrorMacro(<< "Error filling VBO.");
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObject::UploadVBO()
{
  this->Upload(this->PackedVBO, vtkOpenGLBufferObject::ArrayBuffer);
  this->PackedVBO.resize(0);
  this->UploadTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of Components: " << this->NumberOfComponents << "\n";
  os << indent << "Data Type Size: " << this->DataTypeSize << "\n";
  os << indent << "Stride: " << this->Stride << "\n";
  os << indent << "Number of Values (floats): " << this->PackedVBO.size() << "\n";
}
