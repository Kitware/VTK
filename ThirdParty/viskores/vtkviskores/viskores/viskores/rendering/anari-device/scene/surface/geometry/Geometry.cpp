//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Geometry.h"
// subtypes
#include "Cylinder.h"
#include "Quad.h"
#include "Sphere.h"
#include "Triangle.h"

#include "array/ArrayConversion.h"

#include <viskores/cont/FieldRangeCompute.h>

namespace viskores_device
{

Geometry::Geometry(ViskoresDeviceGlobalState* s)
  : Object(ANARI_GEOMETRY, s)
{
  this->m_primitiveAttributes.setAttributes(
    this, { "color", "attribute0", "attribute1", "attribute2", "attribute3" });
  this->m_primitiveAttributes.setAnariAssociation("primitive");
  this->m_primitiveAttributes.setViskoresAssociation(viskores::cont::Field::Association::Cells);
}

Geometry::~Geometry() = default;

Geometry* Geometry::createInstance(std::string_view subtype, ViskoresDeviceGlobalState* s)
{
  // std::cout << "Creating geometry of type " << subtype << "\n";
  if (subtype == "triangle")
    return new Triangle(s);
  else if (subtype == "cylinder")
    return new Cylinder(s);
  else if (subtype == "quad")
    return new Quad(s);
  else if (subtype == "sphere")
    return new Sphere(s);
  else
    return new UnknownGeometry(s);
}

void Geometry::commitParameters()
{
  this->m_primitiveAttributes.commitParameters();
}

void Geometry::finalize()
{
  this->m_primitiveAttributes.setFields(this->m_dataSet);
}

void Geometry::FieldArrayParameters::setAttributes(Geometry* self,
                                                   std::initializer_list<const char*>&& attributes)
{
  this->m_geometry = self;
  for (auto&& attrib : attributes)
    this->m_attributes.emplace(attrib, self);
}

void Geometry::FieldArrayParameters::setAnariAssociation(const std::string& association)
{
  this->m_anariAssociation = association;
}

void Geometry::FieldArrayParameters::setViskoresAssociation(
  viskores::cont::Field::Association association)
{
  this->m_viskoresAssociation = association;
}

void Geometry::FieldArrayParameters::commitParameters()
{
  for (auto& iter : this->m_attributes)
  {
    iter.second =
      this->m_geometry->getParamObject<Array1D>(this->m_anariAssociation + "." + iter.first);
  }
}

void Geometry::FieldArrayParameters::setFields(viskores::cont::DataSet& dataSet)
{
  for (auto& iter : this->m_attributes)
  {
    if (iter.second)
      dataSet.AddField(iter.first, this->m_viskoresAssociation, iter.second->dataAsViskoresArray());
  }
}

helium::ChangeObserverPtr<Array1D>& Geometry::FieldArrayParameters::getParam(
  const std::string& attribute)
{
  return this->m_attributes.find(attribute)->second;
}

const helium::ChangeObserverPtr<Array1D>& Geometry::FieldArrayParameters::getParam(
  const std::string& attribute) const
{
  return this->m_attributes.find(attribute)->second;
}

UnknownGeometry::UnknownGeometry(ViskoresDeviceGlobalState* s)
  : Geometry(s)
{
}

void UnknownGeometry::commitParameters()
{
  // invalid
}

void UnknownGeometry::finalize()
{
  // invalid
}

bool UnknownGeometry::isValid() const
{
  return false;
}

void UnknownGeometry::render(viskores::rendering::Canvas&,
                             const viskores::rendering::Camera&,
                             const viskores::cont::Field&,
                             const viskores::cont::ArrayHandle<viskores::Vec4f_32>&) const
{
  // invalid
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Geometry*);
