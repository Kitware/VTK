// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellAttribute.h"

#include "vtkAbstractArray.h"
#include "vtkObjectFactory.h"

#include <vtk_pegtl.h>

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

bool vtkCellAttribute::CellTypeInfo::operator!=(const CellTypeInfo& other) const
{
  if (this->DOFSharing != other.DOFSharing)
  {
    return true;
  }
  if (this->FunctionSpace != other.FunctionSpace)
  {
    return true;
  }
  if (this->Basis != other.Basis)
  {
    return true;
  }
  if (this->Order != other.Order)
  {
    return true;
  }
  if (this->ArraysByRole != other.ArraysByRole)
  {
    return true;
  }
  return false;
}

vtkStandardNewMacro(vtkCellAttribute);

void vtkCellAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << this->GetName().Data() << "\n";
  os << indent << "Id: " << this->Id << "\n";
  os << indent << "Space: " << this->GetSpace().Data() << "\n";
  os << indent << "NumberOfComponents: " << this->GetNumberOfComponents() << "\n";
  os << indent << "Hash: " << std::hex << this->GetHash() << std::dec << "\n";
  os << indent << "AllArrays: (" << this->AllArrays.size() << " cell types)\n";
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();
  for (const auto& arraysByCell : this->AllArrays)
  {
    os << i2 << arraysByCell.first.Data() << ":";
    if (arraysByCell.second.DOFSharing.IsValid())
    {
      os << " continuous (";
      if (arraysByCell.second.DOFSharing.HasData())
      {
        os << arraysByCell.second.DOFSharing.Data() << ")";
      }
      else
      {
        os << std::hex << arraysByCell.second.DOFSharing.GetId() << std::hex << ")";
      }
    }
    else
    {
      os << " discontinuous";
    }
    if (arraysByCell.second.FunctionSpace.IsValid())
    {
      if (arraysByCell.second.FunctionSpace.HasData())
      {
        os << " " << arraysByCell.second.FunctionSpace.Data();
      }
      else
      {
        os << " " << std::hex << arraysByCell.second.FunctionSpace.GetId() << std::dec;
      }
    }
    else
    {
      os << " (none)";
    }
    if (arraysByCell.second.Basis.IsValid())
    {
      if (arraysByCell.second.Basis.HasData())
      {
        os << " " << arraysByCell.second.Basis.Data();
      }
      else
      {
        os << " " << std::hex << arraysByCell.second.Basis.GetId() << std::dec;
      }
    }
    else
    {
      os << " (no scheme)";
    }
    os << arraysByCell.second.Order << "\n";
    for (const auto& arrayEntry : arraysByCell.second.ArraysByRole)
    {
      os << i3 << arrayEntry.first.Data() << ": " << arrayEntry.second->GetName() << "\n";
    }
  }
}

bool vtkCellAttribute::Initialize(vtkStringToken name, vtkStringToken space, int numberOfComponents)
{
  if (this->Name == name && this->Space == space && this->NumberOfComponents == numberOfComponents)
  {
    return false;
  }

  this->Name = name;
  this->Space = space;
  this->NumberOfComponents = numberOfComponents;

  this->AllArrays.clear();
  this->Modified();

  return true;
}

vtkStringToken::Hash vtkCellAttribute::GetHash() const
{
  std::ostringstream str;
  str << this->GetNumberOfComponents() << "-" << this->GetName().Data() << "-"
      << "-" << this->GetSpace().Data();
  vtkStringToken result(str.str());
  return result.GetId();
}

vtkCellAttribute::CellTypeInfo vtkCellAttribute::GetCellTypeInfo(vtkStringToken cellType) const
{
  auto it = this->AllArrays.find(cellType);
  if (it == this->AllArrays.end())
  {
    return {};
  }
  return it->second;
}

vtkAbstractArray* vtkCellAttribute::GetArrayForCellTypeAndRole(
  vtkStringToken cellType, vtkStringToken arrayRole) const
{
  auto it = this->AllArrays.find(cellType);
  if (it == this->AllArrays.end())
  {
    return nullptr;
  }
  auto it2 = it->second.ArraysByRole.find(arrayRole);
  if (it2 == it->second.ArraysByRole.end())
  {
    return nullptr;
  }
  return it2->second;
}

bool vtkCellAttribute::SetCellTypeInfo(vtkStringToken cellType, const CellTypeInfo& cellTypeInfo)
{
  auto it = this->AllArrays.find(cellType);
  if (it == this->AllArrays.end() || it->second != cellTypeInfo)
  {
    this->AllArrays[cellType] = cellTypeInfo;
    this->Modified();
    return true;
  }
  return false;
}

bool vtkCellAttribute::SetColormap(vtkScalarsToColors* colormap)
{
  if (colormap == this->Colormap)
  {
    return false;
  }
  this->Colormap = colormap;
  this->Modified();
  return true;
}

void vtkCellAttribute::ShallowCopy(vtkCellAttribute* other, bool copyArrays)
{
  if (!other)
  {
    return;
  }

  this->Name = other->Name;
  this->Space = other->Space;
  this->NumberOfComponents = other->NumberOfComponents;
  // Copy CellTypeInfo for each cell type, but if we
  // are not copying arrays, clear out ArraysByRole.
  this->AllArrays = other->AllArrays;
  if (!copyArrays)
  {
    for (auto& entry : this->AllArrays)
    {
      entry.second.ArraysByRole.clear();
    }
  }

  // Do not copy other->Id! Identifiers must be unique across attributes.

  this->Colormap = other->Colormap;
}

void vtkCellAttribute::DeepCopy(
  vtkCellAttribute* other, const std::map<vtkAbstractArray*, vtkAbstractArray*>& arrayRewrites)
{
  if (!other)
  {
    return;
  }

  this->Name = other->Name;
  this->Space = other->Space;
  this->NumberOfComponents = other->NumberOfComponents;

  // Copy arrays, then rewrite pointers as directed.
  this->AllArrays = other->AllArrays;
  if (!arrayRewrites.empty())
  {
    for (auto& entry : this->AllArrays)
    {
      for (auto& subentry : entry.second.ArraysByRole)
      {
        auto it = arrayRewrites.find(subentry.second.GetPointer());
        if (it != arrayRewrites.end())
        {
          subentry.second = it->second;
        }
      }
    }
  }

  // Do not copy other->Id! Identifiers must be unique across attributes.

  // Clone any colormap
  if (other->Colormap)
  {
    this->Colormap = vtkScalarsToColors::SafeDownCast(
      vtkObjectFactory::CreateInstance(other->Colormap->GetClassName()));
    if (this->Colormap)
    {
      this->Colormap->DeepCopy(other->Colormap);
    }
    else
    {
      vtkErrorMacro("Could not clone the attribute's colormap.");
    }
  }
  else
  {
    this->Colormap = nullptr;
  }
}

namespace
{
using namespace tao::pegtl;

struct SpaceBase
  : plus<tao::pegtl::utf8::not_one<0xb9, 0xb1, 0xb2, 0xb3, 0x2070, 0x2071, 0x2074, 0x2075, 0x2076,
      0x2077, 0x2078, 0x2079, 0x207a, 0x207b>>
{
};

struct SpaceExp0 : TAO_PEGTL_ISTRING("⁰")
{
};
// NB: These strings may appear the same, but one is unicode code-point 0xb1
//     while the other is 0x2071:
struct SpaceExp1 : sor<TAO_PEGTL_ISTRING("¹"), TAO_PEGTL_ISTRING("ⁱ")>
{
};
struct SpaceExp2 : TAO_PEGTL_ISTRING("²")
{
};
struct SpaceExp3 : TAO_PEGTL_ISTRING("³")
{
};
struct SpaceExp4 : TAO_PEGTL_ISTRING("⁴")
{
};
struct SpaceExp5 : TAO_PEGTL_ISTRING("⁵")
{
};
struct SpaceExp6 : TAO_PEGTL_ISTRING("⁶")
{
};
struct SpaceExp7 : TAO_PEGTL_ISTRING("⁷")
{
};
struct SpaceExp8 : TAO_PEGTL_ISTRING("⁸")
{
};
struct SpaceExp9 : TAO_PEGTL_ISTRING("⁹")
{
};
struct SpaceHSPlus : TAO_PEGTL_ISTRING("⁺")
{
};
struct SpaceHSMinus : TAO_PEGTL_ISTRING("⁻")
{
};

#if 0
struct SpaceExp : plus<tao::pegtl::utf8::one<
  0xb9, 0xb1, 0xb2, 0xb3,
  0x2070, 0x2071, 0x2074, 0x2075, 0x2076, 0x2077, 0x2078, 0x2079>
{ };
#else
struct SpaceExp
  : plus<sor<SpaceExp0, SpaceExp1, SpaceExp2, SpaceExp3, SpaceExp4, SpaceExp5, SpaceExp6, SpaceExp7,
      SpaceExp8, SpaceExp9>>
{
};
#endif

struct SpaceHalfspace : tao::pegtl::utf8::one<0x207a, 0x207b>
{
};

struct SpaceNoExponent : seq<SpaceBase, SpaceHalfspace>
{
};

struct SpaceGrammar
  : must<sor<seq<SpaceBase, SpaceExp, SpaceHalfspace>, seq<SpaceBase, SpaceExp>, SpaceNoExponent>,
      eof>
{
};

template <typename Token>
struct SpaceAction : nothing<Token>
{
};

template <>
struct SpaceAction<SpaceBase>
{
  template <typename Input>
  static void apply(const Input& in, std::string& base, double& exp, int& halfspace)
  {
    (void)exp;
    (void)halfspace;
    base = in.string();
  }
};

#if 0
#define VTK_SPACE_EXP_ACTION(Rule, Number)                                                         \
  template <>                                                                                      \
  struct SpaceAction<Rule>                                                                         \
  {                                                                                                \
    template <typename Input>                                                                      \
    static void apply(const Input& in, std::string& base, double& exp, int& halfspace)             \
    {                                                                                              \
      exp = 10.0 * exp + Number;                                                                   \
    }                                                                                              \
  };

VTK_SPACE_EXP_ACTION(SpaceExp0, 0);
VTK_SPACE_EXP_ACTION(SpaceExp1, 1);
VTK_SPACE_EXP_ACTION(SpaceExp2, 2);
VTK_SPACE_EXP_ACTION(SpaceExp3, 3);
VTK_SPACE_EXP_ACTION(SpaceExp4, 4);
VTK_SPACE_EXP_ACTION(SpaceExp5, 5);
VTK_SPACE_EXP_ACTION(SpaceExp6, 6);
VTK_SPACE_EXP_ACTION(SpaceExp7, 7);
VTK_SPACE_EXP_ACTION(SpaceExp8, 8);
VTK_SPACE_EXP_ACTION(SpaceExp9, 9);
#else
bool consume(const std::string& source, std::size_t& loc, const std::string& match)
{
  if (source.size() - loc < match.size())
  {
    return false;
  }
  if (source.substr(loc, match.size()) == match)
  {
    loc += match.size();
    return true;
  }
  return false;
}

template <>
struct SpaceAction<SpaceExp>
{
  template <typename Input>
  static void apply(const Input& in, std::string& base, double& exp, int& halfspace)
  {
    (void)base;
    (void)halfspace;
    static std::string zero("⁰");
    static std::string oneA("\xc2\xb9");     // ¹
    static std::string oneB("\xe2\x81\xb1"); // ⁱ
    static std::string two("²");
    static std::string three("³");
    static std::string four("⁴");
    static std::string five("⁵");
    static std::string six("⁶");
    static std::string seven("⁷");
    static std::string eight("⁸");
    static std::string nine("⁹");
    exp = 0.0;
    std::string source = in.string();
    std::size_t loc = 0;
    while (loc < source.size())
    {
      int digit = -1;
      if (consume(source, loc, zero))
      {
        digit = 0;
      }
      else if (consume(source, loc, oneA))
      {
        digit = 1;
      }
      else if (consume(source, loc, oneB))
      {
        digit = 1;
      }
      else if (consume(source, loc, two))
      {
        digit = 2;
      }
      else if (consume(source, loc, three))
      {
        digit = 3;
      }
      else if (consume(source, loc, four))
      {
        digit = 4;
      }
      else if (consume(source, loc, five))
      {
        digit = 5;
      }
      else if (consume(source, loc, six))
      {
        digit = 6;
      }
      else if (consume(source, loc, seven))
      {
        digit = 7;
      }
      else if (consume(source, loc, eight))
      {
        digit = 8;
      }
      else if (consume(source, loc, nine))
      {
        digit = 9;
      }
      if (digit < 0)
      {
        exp = 0.0;
        vtkGenericWarningMacro("Bad token at " << loc << " of '" << in.string() << "'.");
        return;
      }
      exp = 10.0 * exp + digit;
    }
  }
};

#endif

template <>
struct SpaceAction<SpaceHalfspace>
{
  template <typename Input>
  static void apply(const Input& in, std::string& base, double& exp, int& halfspace)
  {
    (void)base;
    (void)exp;
    halfspace = (in.string() == "⁺" ? 1 : -1);
  }
};

template <>
struct SpaceAction<SpaceNoExponent>
{
  template <typename Input>
  static void apply(const Input& in, std::string& base, double& exp, int& halfspace)
  {
    (void)in;
    (void)base;
    (void)halfspace;
    // We matched the rule that only includes a halfspace.
    // The exponent should be assumed to be 1.0.
    exp = 1.0;
  }
};

} // anonymous namespace

bool vtkCellAttribute::DecodeSpace(
  const std::string& space, std::string& base, double& exp, int& halfspace, bool quiet)
{
  (void)base;
#if 0
  // Consume characters from \a space until we find one of the following
  // "exponent" UTF-8 code-points.
  bool condCPC2 = false;
  bool condCPE2 = false;
  bool condCP81 = false;
  bool haveExp = false;
  std::size_t ii = 0;
  base.clear();
  for (const char& codePoint : space)
  {
    if (condCPC2)
    {
      if (codePoint == 0xb9 || codePoint == 0xb2 || codePoint == 0xb3)
      {
        --ii; // Do not consume first code point of exponent.
        haveExp = true;
        break;
      }
      condCPC2 = false;
      base.push_back(0xc2);
      base.push_back(codePoint);
    }
    else if (condCPE2)
    {
      if (codePoint == 0x81)
      {
        condCP81 = true;
      }
      else
      {
        condCPE2 = false;
        base.push_back(0xe2);
        base.push_back(codePoint);
      }
    }
    else if (condCP81)
    {
      if (
        (codePoint >= 0xb0 && codePoint <= 0xb1) ||
        (codePoint >= 0xb4 && codePoint <= 0xbf))
      {
        ii -= 2;
        haveExp = true;
        break;
      }
      else
      {
        codeCPE2 = false;
        condCP81 = false;
        base.push_back(0xe2);
        base.push_back(0x81);
        base.push_back(codePoint);
      }
    }
    else
    {
      if (codePoint == 0xc2)
      {
        condCPC2 = true;
      }
      else if (codePoint == 0xe2)
      {
        condCPE2 = true;
      }
      else
      {
        base.push_back(codePoint);
      }
    }
    ++ii;
  }
  if (!haveExp)
  {
    exp = 0;
    halfspace = 0;
    // No exponent was found and we consumed the whole string.
    return false;
  }
#endif

  exp = 0.0;
  halfspace = 0;
  tao::pegtl::string_input<> in(space, "constructRule");
  try
  {
    tao::pegtl::parse<SpaceGrammar, SpaceAction>(in, base, exp, halfspace);
  }
  catch (tao::pegtl::parse_error& err)
  {
    if (!quiet)
    {
      const auto p = err.positions.front();
      vtkGenericWarningMacro("Attribute Space: " << err.what() << "\n"
                                                 << in.line_at(p) << "\n"
                                                 << std::string(p.byte_in_line, ' ') << "^\n");
    }
    return false;
  }
  return true;
}

std::string vtkCellAttribute::EncodeSpace(const std::string& base, unsigned int exp, int halfspace)
{
  std::ostringstream result;
  result << base;
  if (exp == 0)
  {
    result << "⁰";
  }
  else if (exp == 1)
  {
    // No need to write the exponent.
  }
  else
  {
    std::vector<unsigned int> expDigits;
    static const char* digits[] = { "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹" };
    while (exp)
    {
      int digit = exp % 10;
      expDigits.push_back(digit);
      exp = (exp - digit) / 10;
    }
    for (auto rit = expDigits.rbegin(); rit != expDigits.rend(); ++rit)
    {
      result << digits[*rit];
    }
  }
  if (halfspace != 0)
  {
    result << (halfspace < 0 ? "⁻" : "⁺");
  }
  return result.str();
}

VTK_ABI_NAMESPACE_END
