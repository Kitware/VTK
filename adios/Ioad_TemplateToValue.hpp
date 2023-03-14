// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioad_TemplateToValue_hpp
#define IOSS_Ioad_TemplateToValue_hpp

#include "Ioss_Field.h" // for Field, etc

namespace Ioad {

  template <typename T> constexpr Ioss::Field::BasicType template_to_basic_type() noexcept
  {
    return Ioss::Field::BasicType::INVALID;
  }

  template <> constexpr Ioss::Field::BasicType template_to_basic_type<double>() noexcept
  {
    return Ioss::Field::BasicType::DOUBLE;
  }

  template <> constexpr Ioss::Field::BasicType template_to_basic_type<int32_t>() noexcept
  {
    return Ioss::Field::BasicType::INT32;
  }

  template <> constexpr Ioss::Field::BasicType template_to_basic_type<int64_t>() noexcept
  {
    return Ioss::Field::BasicType::INT64;
  }

  template <> constexpr Ioss::Field::BasicType template_to_basic_type<Complex>() noexcept
  {
    return Ioss::Field::BasicType::COMPLEX;
  }

  template <> constexpr Ioss::Field::BasicType template_to_basic_type<std::string>() noexcept
  {
    return Ioss::Field::BasicType::STRING;
  }

  template <> constexpr Ioss::Field::BasicType template_to_basic_type<char>() noexcept
  {
    return Ioss::Field::BasicType::CHARACTER;
  }

  template <> constexpr char const *get_entity_type<Ioss::SideBlock>() noexcept
  {
    return "SideBlock";
  }

  template <> constexpr char const *get_entity_type<Ioss::SideSet>() noexcept { return "SideSet"; }

  template <> constexpr char const *get_entity_type<Ioss::NodeBlock>() noexcept
  {
    return "NodeBlock";
  }

  template <> constexpr char const *get_entity_type<Ioss::EdgeBlock>() noexcept
  {
    return "EdgeBlock";
  }

  template <> constexpr char const *get_entity_type<Ioss::FaceBlock>() noexcept
  {
    return "FaceBlock";
  }

  template <> constexpr char const *get_entity_type<Ioss::ElementBlock>() noexcept
  {
    return "ElementBlock";
  }

  template <> constexpr char const *get_entity_type<Ioss::NodeSet>() noexcept { return "NodeSet"; }

  template <> constexpr char const *get_entity_type<Ioss::EdgeSet>() noexcept { return "EdgeSet"; }

  template <> constexpr char const *get_entity_type<Ioss::FaceSet>() noexcept { return "FaceSet"; }

  template <> constexpr char const *get_entity_type<Ioss::ElementSet>() noexcept
  {
    return "ElementSet";
  }

  template <> constexpr char const *get_entity_type<Ioss::CommSet>() noexcept { return "CommSet"; }

  template <class T> inline std::string GetType() noexcept { return "compound"; }
  template <> inline std::string        GetType<void>() noexcept { return "unknown"; }

  template <> inline std::string GetType<std::string>() noexcept { return "string"; }

  template <> inline std::string GetType<char>() noexcept { return "char"; }
  template <> inline std::string GetType<signed char>() noexcept { return "signed char"; }
  template <> inline std::string GetType<unsigned char>() noexcept { return "unsigned char"; }
  template <> inline std::string GetType<short>() noexcept { return "short"; }
  template <> inline std::string GetType<unsigned short>() noexcept { return "unsigned short"; }
  template <> inline std::string GetType<int>() noexcept { return "int"; }
  template <> inline std::string GetType<unsigned int>() noexcept { return "unsigned int"; }
  template <> inline std::string GetType<long int>() noexcept { return "long int"; }
  template <> inline std::string GetType<unsigned long int>() noexcept
  {
    return "unsigned long int";
  }
  template <> inline std::string GetType<long long int>() noexcept { return "long long int"; }
  template <> inline std::string GetType<unsigned long long int>() noexcept
  {
    return "unsigned long long int";
  }
  template <> inline std::string GetType<float>() noexcept { return "float"; }
  template <> inline std::string GetType<double>() noexcept { return "double"; }
  template <> inline std::string GetType<long double>() noexcept { return "long double"; }
  template <> inline std::string GetType<std::complex<float>>() noexcept { return "float complex"; }
  template <> inline std::string GetType<std::complex<double>>() noexcept
  {
    return "double complex";
  }

} // namespace Ioad

#endif
