// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef OMFElement_h
#define OMFElement_h

#include "vtkSmartPointer.h"

#include "vtk_jsoncpp_fwd.h"

#include <memory>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSet;
class vtkPartitionedDataSetCollection;
class vtkTexture;
VTK_ABI_NAMESPACE_END

namespace omf
{
VTK_ABI_NAMESPACE_BEGIN

struct OMFFile;

class ProjectElement
{
public:
  ProjectElement(const std::string& uid, double globalOrigin[3]);
  virtual ~ProjectElement() = default;

  virtual void ProcessJSON(std::shared_ptr<OMFFile>& file, const Json::Value& element,
    vtkPartitionedDataSet* output, bool writeOutTextures, bool columnMajorOrdering);

protected:
  std::string UID;
  double GlobalOrigin[3] = { 0, 0, 0 };

  virtual void ProcessGeometry(std::shared_ptr<OMFFile>& file, const std::string& geometryUID,
    vtkPartitionedDataSet* output) = 0;
  virtual void ProcessDataFields(
    std::shared_ptr<OMFFile>& file, const Json::Value& dataJSON, vtkPartitionedDataSet* output);
  virtual void ProcessColumnOrdering(std::shared_ptr<OMFFile>& /*file*/,
    const Json::Value& /*dataJSON*/, vtkPartitionedDataSet* /*output*/){};
  virtual void ProcessTextures(std::shared_ptr<OMFFile>& file, const Json::Value& textureJSON,
    vtkPartitionedDataSet* output, const std::string& elementName);
};

class PointSetElement : public ProjectElement
{
public:
  PointSetElement(const std::string& uid, double globalOrigin[3])
    : ProjectElement(uid, globalOrigin)
  {
  }

protected:
  void ProcessGeometry(std::shared_ptr<OMFFile>& file, const std::string& geometryUID,
    vtkPartitionedDataSet* output) override;
};

class LineSetElement : public ProjectElement
{
public:
  LineSetElement(const std::string& uid, double globalOrigin[3])
    : ProjectElement(uid, globalOrigin)
  {
  }

protected:
  void ProcessGeometry(std::shared_ptr<OMFFile>& file, const std::string& geometryUID,
    vtkPartitionedDataSet* output) override;
};

class SurfaceElement : public ProjectElement
{
public:
  SurfaceElement(const std::string& uid, double globalOrigin[3])
    : ProjectElement(uid, globalOrigin)
  {
  }

protected:
  void ProcessGeometry(std::shared_ptr<OMFFile>& file, const std::string& geometryUID,
    vtkPartitionedDataSet* output) override;
};

class VolumeElement : public ProjectElement
{
public:
  VolumeElement(const std::string& uid, double globalOrigin[3])
    : ProjectElement(uid, globalOrigin)
  {
  }

protected:
  void ProcessGeometry(std::shared_ptr<OMFFile>& file, const std::string& geometryUID,
    vtkPartitionedDataSet* output) override;
  void ProcessColumnOrdering(std::shared_ptr<OMFFile>& file, const Json::Value& dataJSON,
    vtkPartitionedDataSet* output) override;
  size_t Dimensions[3];
};

VTK_ABI_NAMESPACE_END
} // end namespace omf

#endif // OMFElement_h
