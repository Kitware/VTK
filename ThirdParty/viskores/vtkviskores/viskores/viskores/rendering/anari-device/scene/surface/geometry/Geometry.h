//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#pragma once

#include "Object.h"
#include "array/Array1D.h"
#include "scene/surface/material/Material.h"

#include <viskores/cont/DataSet.h>
#include <viskores/rendering/Canvas.h>

#include <map>

namespace viskores_device
{

struct Geometry : public Object
{
  Geometry(ViskoresDeviceGlobalState* s);
  ~Geometry() override;
  static Geometry* createInstance(std::string_view subtype, ViskoresDeviceGlobalState* s);

  const viskores::cont::DataSet& getDataSet() const { return this->m_dataSet; }

  // The commitParameters and finalize will load parameters common to all
  // geometry objects and add them to `m_dataSet`.
  void commitParameters() override;
  void finalize() override;

  virtual void render(viskores::rendering::Canvas& canvas,
                      const viskores::rendering::Camera& camera,
                      const viskores::cont::Field& field,
                      const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const = 0;

  // This struct helps manage a set of parameters providing arrays of data that
  // will be attached to fields on a Viskores dataset created by this geometry.
  // The fields are in the form `association.name` where `association` is the
  // string used to describe what type of element each value is attached to
  // (i.e., `vertex`, `primitive`) and `name` is how ANARI references the array
  // (i.e., `color`, `attribute0`). The structure will register itself with the
  // given `Geometry` object so that the geometry will be updated on data
  // change. Use `commitParameters` to pull the arrays from the associated
  // parameters (usually done in a `commitParameters` method of the geometry
  // object). Use `setFields` to add these arrays as fields to a provided
  // `viskores::cont::DataSet` (usually done in a `finalize` method of the
  // geometry object).
  class FieldArrayParameters
  {
    Geometry* m_geometry;
    std::map<std::string, helium::ChangeObserverPtr<Array1D>> m_attributes;
    std::string m_anariAssociation;
    viskores::cont::Field::Association m_viskoresAssociation;

  public:
    void setAttributes(Geometry* self, std::initializer_list<const char*>&& attributes);
    void setAnariAssociation(const std::string& association);
    void setViskoresAssociation(viskores::cont::Field::Association association);

    void commitParameters();
    void setFields(viskores::cont::DataSet& dataSet);

    // Gets a specific named parameter (perhaps because it is special). The
    // string should be the same as that given in the list provided to
    // `setAttributes`. It will not have the association part included in
    // ANARI's name. This method only works after `commitParameters` is called.
    helium::ChangeObserverPtr<Array1D>& getParam(const std::string& attribute);
    const helium::ChangeObserverPtr<Array1D>& getParam(const std::string& attribute) const;
  };

protected:
  viskores::cont::DataSet m_dataSet;
  FieldArrayParameters m_primitiveAttributes;
};

struct UnknownGeometry : public Geometry
{
  UnknownGeometry(ViskoresDeviceGlobalState* s);
  void commitParameters() override;
  void finalize() override;
  bool isValid() const override;
  virtual void render(viskores::rendering::Canvas&,
                      const viskores::rendering::Camera&,
                      const viskores::cont::Field&,
                      const viskores::cont::ArrayHandle<viskores::Vec4f_32>&) const override;
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Geometry*, ANARI_GEOMETRY);
