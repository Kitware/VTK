/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfArrayType.cpp                                                   */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <sstream>
#include <utility>
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"

// Supported XdmfArrayTypes
shared_ptr<const XdmfArrayType>
XdmfArrayType::Uninitialized()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("None", 0, XdmfArrayType::Unsigned));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::Int8()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("Char", 1, XdmfArrayType::Signed));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::Int16()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("Short", 2, XdmfArrayType::Signed));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::Int32()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("Int", 4, XdmfArrayType::Signed));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::Int64()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("Int", 8, XdmfArrayType::Signed));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::Float32()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("Float", 4, XdmfArrayType::Float));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::Float64()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("Float", 8, XdmfArrayType::Float));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::UInt8()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("UChar", 1, XdmfArrayType::Unsigned));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::UInt16()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("UShort", 2, XdmfArrayType::Unsigned));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::UInt32()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("UInt", 4, XdmfArrayType::Unsigned));
  return p;
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::String()
{
  static shared_ptr<const XdmfArrayType> p(new XdmfArrayType("String", 0, XdmfArrayType::Unsigned));
  return p;
}

XdmfArrayType::XdmfArrayType(const std::string & name,
                             const unsigned int precision,
                             const Format typeFormat) :
  mName(name),
  mPrecision(precision),
  mTypeFormat(typeFormat)
{
  std::stringstream precisionString;
  precisionString << precision;
  mPrecisionString = precisionString.str();
}

XdmfArrayType::~XdmfArrayType()
{
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::New(const std::map<std::string, std::string> & itemProperties)
{
  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("DataType");
  if(type == itemProperties.end()) {
    type = itemProperties.find("NumberType");
  }
  if(type == itemProperties.end()) {
    // to support old xdmf defaults, return Float32()
    return Float32();
  }
  const std::string & typeVal = type->second;

  std::map<std::string, std::string>::const_iterator precision =
    itemProperties.find("Precision");
  const unsigned int precisionVal = 
    (precision == itemProperties.end()) ? 0 : atoi(precision->second.c_str()); 

  if(typeVal.compare("Float") == 0) {
    if(precisionVal == 8) {
      return Float64();
    }
    return Float32();
  }
  else if(typeVal.compare("Int") == 0) {
    if(precisionVal == 8) {
      return Int64();
    }
    return Int32();
  }
  else if(typeVal.compare("String") == 0) {
    return String();
  }
  else if(typeVal.compare("Char") == 0) {
    return Int8();
  }
  else if(typeVal.compare("Short") == 0) {
    return Int16();
  }
  else if(typeVal.compare("UChar") == 0) {
    return UInt8();
  }
  else if(typeVal.compare("UShort") == 0) {
    return UInt16();
  }
  else if(typeVal.compare("UInt") == 0) {
    return UInt32();
  }
  else if(typeVal.compare("None") == 0) {
    return Uninitialized();
  }

  XdmfError::message(XdmfError::FATAL,
                     "Type not one of accepted values: " + typeVal +
                     " in XdmfArrayType::New");
  
  // unreachable
  return shared_ptr<const XdmfArrayType>();
}

shared_ptr<const XdmfArrayType>
XdmfArrayType::comparePrecision(shared_ptr<const XdmfArrayType> type1,
                                shared_ptr<const XdmfArrayType> type2)
{
  std::string type1Name = type1->getName();
  std::string type2Name = type2->getName();

  if (type2Name.compare(type1Name) == 0) {
    if (type1->getElementSize() >= type2->getElementSize()) {
      return type1;
    }
    else {
      return type2;
    }
  }

  bool firstIsSigned = false;
  if (type1Name.compare("UChar") != 0 &&
      type1Name.compare("UShort") != 0 &&
      type1Name.compare("UInt") != 0) {
    firstIsSigned = true;
  }

  bool secondIsSigned = false;
  if (type2Name.compare("UChar") != 0 &&
      type2Name.compare("UShort") != 0 &&
      type2Name.compare("UInt") != 0) {
    secondIsSigned = true;
  }

  std::map<std::string, int> controlmap;
  controlmap["Char"] = 1;
  controlmap["UChar"] = 2;
  controlmap["Short"] = 3;
  controlmap["UShort"] = 4;
  controlmap["Int"] = 5;
  controlmap["UInt"] = 6;
  controlmap["Float"] = 7;
  controlmap["String"] = 8;

  int control = controlmap[type1Name];


  // In this switch the starting location is determined by
  // the first type and then the algorithm cascades
  // until it finds the second type
  switch (control) {
    case 1:
      // Char
    case 2:
      // UChar
      if (type2Name.compare("Char") == 0 ||
          type2Name.compare("UChar") == 0) {
        // This statement would be called in the case
        // where there is a mixed type of Char and UChar
        // The resulting type should be a Short
        return Int16();
      }
    case 3:
      // Short
      if (type2Name.compare("Char") == 0 ||
          type2Name.compare("UChar") == 0 ||
          type2Name.compare("Short") == 0) {
        // This will be called for any combination of
        // Char/UChar and Short
        // In all of these cases the result shoule be a Short
        return Int16();
      }
    case 4:
      // UShort
      if (type2Name.compare("Char") == 0 ||
          type2Name.compare("Short") == 0) {
        // When mixing UShort with a signed type that has a lower precision
        // the resulting type should be an int
        return Int32();
      }
      else if (type2Name.compare("UChar") == 0 ||
               type2Name.compare("UShort") == 0) {
        // When mixing UShort with an unsigned type that has a lower precision
        // a Ushort should be the resulting type
        if (!firstIsSigned) {
          return UInt16();
        }
        else {
          return Int32();
        }
      }
    case 5:
      // Int
      if (type2Name.compare("Int") != 0 &&
          type2Name.compare("UInt") != 0 &&
          type2Name.compare("Float") != 0 &&
          type2Name.compare("String") != 0) {
        // When mixing an Int with a type of lower precision
        // the resulting type should match the Int's precision
        if (type1->getElementSize() == 4) {
          return Int32();
        }
        else {
          return Int64();
        }
      }
      if (type2Name.compare("Int") == 0) {
        if (type2->getElementSize() == 4) {
          return Int32();
        }
        else {
          return Int64();
        }
      }
    case 6:
      // UInt
      if (type2Name.compare("UInt") != 0 &&
          type2Name.compare("Int") != 0 &&
          type2Name.compare("Float") != 0 &&
          type2Name.compare("String") != 0) {
        // When mixing UInt with another non-floating-point type
        // the result should be either long or unsigned int
        // depending on the if the mixed type is signed or not
        if (!secondIsSigned) {
          return UInt32();
        }
        else {
          return Int64();
        }
      }
      else if (type2Name.compare("UInt") == 0) {
        if (firstIsSigned) {
          return Int64();
        }
        else {
          return UInt32();
        }
      }
      else if (type2Name.compare("Int") == 0) {
        return Int64();
      }
    case 7:
      // Float
      if (type2Name.compare("String") != 0 &&
          type2Name.compare("Float") != 0 &&
          type2Name.compare("UInt") != 0) {
        // String is the only type that has priority over a float
        // This case occurs when type1 is a float
        return type1;
      }
      else if (type2Name.compare("UInt") == 0) {
        return Float64();
      }
      else if (type2Name.compare("Float") == 0) {
        // Since there was a check earlier to see if the type names matched
        // This is the case when type2 is a float
        if (type1Name.compare("UInt") == 0) {
          return Float64();
        }
	else {
          return type2;
        }
      }
    case 8:
      // String
      // String has priority over everything
      return String();
    default:
      break;
  }
  // Double is the default value
  // Should all of the above manage to fail to return a value
  return Float64();
}

unsigned int
XdmfArrayType::getElementSize() const
{
  return mPrecision;
}

std::string
XdmfArrayType::getName() const
{
  return mName;
}

bool
XdmfArrayType::getIsFloat() const
{
  if (mTypeFormat == XdmfArrayType::Float) {
    return true;
  }
  else {
    return false;
  }
}

bool
XdmfArrayType::getIsSigned() const
{
  if (mTypeFormat == XdmfArrayType::Float ||
      mTypeFormat == XdmfArrayType::Signed) {
    return true;
  }
  else {
    return false;
  }
}

void
XdmfArrayType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("DataType", mName));
  collectedProperties.insert(std::make_pair("Precision", mPrecisionString));
}
