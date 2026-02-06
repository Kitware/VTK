/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifdef _MSC_VER
#  pragma warning(disable : 4702)
#endif

#include "metaUtils.h"
#include "metaCommand.h"

#include <cstring>
#include <utility>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

#ifdef METAIO_USE_LIBXML2
#  include <libxml/xmlwriter.h>
#  include <libxml/xmlmemory.h>
#  include <libxml/parser.h>
#  include <libxml/xpath.h>
#  include <libxml/xpathInternals.h>
#  include <libxml/tree.h>
#endif


MetaCommand::MetaCommand()
{
  m_HelpCallBack = nullptr;
  m_OptionVector.clear();
  m_Version = "Not defined";
  m_Date = "Not defined";
  m_Name = "";
  m_Author = "Not defined";
  m_Description = "";
  m_Acknowledgments = "";
  m_Category = "";
  m_ParsedOptionVector.clear();
  m_Verbose = true;
  m_FailOnUnrecognizedOption = false;
  m_GotXMLFlag = false;
  m_DisableDeprecatedWarnings = false;
}


/** Extract the date from a CVS Date keyword/value pair.  */
std::string
MetaCommand::ExtractDateFromCVS(std::string date)
{
  std::string newdate;
  for (int i = 7; i < static_cast<int>(date.size()) - 1; i++)
  {
    newdate += date[i];
  }
  return newdate;
}

void
MetaCommand::DisableDeprecatedWarnings()
{
  m_DisableDeprecatedWarnings = true;
}

void
MetaCommand::SetDateFromCVS(std::string cvsDate)
{
  this->SetDate(this->ExtractDateFromCVS(std::move(cvsDate)).c_str());
}

/** Extract the version from a CVS Revision keyword/value pair.  */
std::string
MetaCommand::ExtractVersionFromCVS(std::string version)
{
  std::string newversion;
  for (int i = 11; i < static_cast<int>(version.size()) - 1; i++)
  {
    newversion += version[i];
  }
  return newversion;
}

void
MetaCommand::SetVersionFromCVS(std::string cvsVersion)
{
  this->SetVersion(this->ExtractVersionFromCVS(std::move(cvsVersion)).c_str());
}


/** */
bool
MetaCommand::SetOption(const Option& option)
{
  // need to add some tests here to check if the option is not defined yet
  m_OptionVector.push_back(option);
  return true;
}

bool
MetaCommand::SetOption(std::string        name,
                       const std::string&        shortTag,
                       bool               required,
                       std::string        description,
                       std::vector<Field> fields)
{
  // need to add some tests here to check if the option is not defined yet
  // Short tag can be empty as long as the long tag is defined.
  // This is checked in the Parse() command
  /*if(tag == "")
    {
    std::cout << "Tag cannot be empty : use AddField() instead."
                        << std::endl;
    return false;
    }*/
  if (!m_DisableDeprecatedWarnings && shortTag.size() > 1)
  {
    std::cout << "Warning: as of August 23, 2007 MetaCommand::SetOption()"
              << " is expecting a shortTag of exactly one character."
              << " You should use the SetOptionLongTag(optionName,longTagName)"
              << " if you want to use a longer tag. The longtag will be"
              << " referred to as --LongTag and the short tag as -ShortTag."
              << " Replace -" << shortTag.c_str() << " by --" << shortTag.c_str() << '\n';
  }

  Option option;
  option.name = std::move(name);
  option.tag = shortTag;
  option.longtag = "";
  option.fields = std::move(fields);
  option.required = required;
  option.description = std::move(description);
  option.userDefined = false;
  option.complete = false;

  m_OptionVector.push_back(option);
  return true;
}


bool
MetaCommand::SetOption(const std::string&  name,
                       const std::string&  shortTag,
                       bool         required,
                       std::string  description,
                       TypeEnumType type,
                       std::string  defVal,
                       DataEnumType externalData)
{
  // need to add some tests here to check if the option is not defined yet
  // Short tag can be empty as long as the long tag is defined.
  // This is checked in the Parse() command
  /*
  if(tag == "")
    {
    std::cout << "Tag cannot be empty : use AddField() instead."
                        << std::endl;
    return false;
    }*/

  if (!m_DisableDeprecatedWarnings && shortTag.size() > 1)
  {
    std::cout << "Warning: as of August 23, 2007 MetaCommand::SetOption() "
              << " is expecting a shortTag of exactly one character."
              << " You should use the SetOptionLongTag(optionName,longTagName)"
              << " if you want to use a longer tag. The longtag will be "
              << " referred to as --LongTag and the short tag as -ShortTag "
              << " Replace -" << shortTag.c_str() << " by --" << shortTag.c_str() << '\n';
  }

  Option option;
  option.tag = shortTag;
  option.longtag = "";
  option.name = name;
  option.required = required;
  option.description = std::move(description);
  option.userDefined = false;
  option.complete = false;

  // Create a field without description as a flag
  Field field;
  if (type == LIST)
  {
    field.name = "NumberOfValues";
  }
  else
  {
    field.name = name;
  }
  field.externaldata = externalData;
  field.type = type;
  field.value = std::move(defVal);
  field.userDefined = false;
  field.required = true;
  field.rangeMin = "";
  field.rangeMax = "";
  option.fields.push_back(field);

  m_OptionVector.push_back(option);
  return true;
}


/** Add a field */
bool
MetaCommand::AddField(const std::string&  name,
                      std::string  description,
                      TypeEnumType type,
                      DataEnumType externalData,
                      std::string  rangeMin,
                      std::string  rangeMax)
{
  // need to add some tests here to check if the option is not defined yet
  Option option;
  option.tag = "";
  option.longtag = "";

  // Create a field without description with the specified type
  Field field;
  field.name = name;
  field.type = type;
  field.required = true;
  field.userDefined = false;
  field.externaldata = externalData;
  field.rangeMin = std::move(rangeMin);
  field.rangeMax = std::move(rangeMax);
  option.fields.push_back(field);

  option.required = true;
  option.name = name;
  option.description = std::move(description);
  option.userDefined = false;
  option.complete = false;

  m_OptionVector.push_back(option);
  return true;
}

/** For backward compatibility */
bool
MetaCommand::AddField(const std::string& name, const std::string& description, TypeEnumType type, bool externalData)
{
  if (externalData)
  {
    return this->AddField(name, description, type, DATA_IN);
  }
  else
  {
    return this->AddField(name, description, type, DATA_NONE);
  }
}

/** Collect all the information until the next tag
 * \warning this function works only if the field is of type String */
void
MetaCommand::SetOptionComplete(const std::string& optionName, bool complete)
{
  auto it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      (*it).complete = complete;
      return;
    }
    ++it;
  }
}

/** Add a field to a given an option */
bool
MetaCommand::AddOptionField(const std::string&  optionName,
                            const std::string&  name,
                            TypeEnumType type,
                            bool         required,
                            const std::string&  defVal,
                            const std::string&  description,
                            DataEnumType externalData)
{
  auto it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      // Create a field without description with the specified type
      Field field;
      field.name = name;
      field.type = type;
      field.required = required;
      field.value = defVal;
      field.description = description;
      field.userDefined = false;
      field.externaldata = externalData;
      field.rangeMin = "";
      field.rangeMax = "";

      // If this is the first field in the list we replace the current field
      if ((*it).fields[0].type == FLAG)
      {
        (*it).fields[0] = field;
      }
      else
      {
        (*it).fields.push_back(field);
      }
      return true;
    }
    ++it;
  }
  return false;
}

/** Set the range of an option */
bool
MetaCommand::SetOptionRange(const std::string& optionName, const std::string& name, const std::string& rangeMin, const std::string& rangeMax)
{
  auto it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      std::vector<Field> & fields = (*it).fields;
      auto                 itField = fields.begin();
      while (itField != fields.end())
      {
        if ((*itField).name == name)
        {
          (*itField).rangeMin = rangeMin;
          (*itField).rangeMax = rangeMax;
          return true;
        }
        ++itField;
      }
    }
    ++it;
  }
  return false;
}

/** Set the range of an option */
bool
MetaCommand::SetOptionEnumerations(const std::string& optionName, const std::string& name, const std::string& optionEnums)

{
  auto it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      std::vector<Field> & fields = (*it).fields;
      auto                 itField = fields.begin();
      while (itField != fields.end())
      {
        if ((*itField).name == name)
        {
          (*itField).rangeMin = optionEnums;
          return true;
        }
        ++itField;
      }
    }
    ++it;
  }
  return false;
}


/** Return the value of the option as a boolean */
bool
MetaCommand::GetValueAsBool(const std::string& optionName, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = optionName;
  }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      auto itField = (*it).fields.begin();
      while (itField != (*it).fields.end())
      {
        if ((*itField).name == fieldname)
        {
          if ((*itField).value == "true" || (*itField).value == "1" || (*itField).value == "True" ||
              (*itField).value == "TRUE")
          {
            return true;
          }
          return false;
        }
        ++itField;
      }
    }
    ++it;
  }
  return false;
}


/** Return the value of the option as a bool */
bool
MetaCommand::GetValueAsBool(Option option, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = option.name;
  }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while (itField != option.fields.end())
  {
    if ((*itField).name == fieldname)
    {
      if ((*itField).value == "true" || (*itField).value == "1" || (*itField).value == "True" ||
          (*itField).value == "TRUE")
      {
        return true;
      }
      return false;
    }
    ++itField;
  }
  return false;
}

/** Return the value of the option as a float */
float
MetaCommand::GetValueAsFloat(const std::string& optionName, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = optionName;
  }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      auto itField = (*it).fields.begin();
      while (itField != (*it).fields.end())
      {
        if ((*itField).name == fieldname)
        {
          return static_cast<float>(atof((*itField).value.c_str()));
        }
        ++itField;
      }
    }
    ++it;
  }
  return 0;
}

/** Return the value of the option as a float */
float
MetaCommand::GetValueAsFloat(Option option, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = option.name;
  }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while (itField != option.fields.end())
  {
    if ((*itField).name == fieldname)
    {
      return static_cast<float>(atof((*itField).value.c_str()));
    }
    ++itField;
  }
  return 0;
}

/** Return the value of the option as a int */
int
MetaCommand::GetValueAsInt(const std::string& optionName, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = optionName;
  }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      auto itField = (*it).fields.begin();
      while (itField != (*it).fields.end())
      {
        if ((*itField).name == fieldname)
        {
          return atoi((*itField).value.c_str());
        }
        ++itField;
      }
    }
    ++it;
  }
  return 0;
}

/** Return the value of the option as a int */
int
MetaCommand::GetValueAsInt(Option option, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = option.name;
  }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while (itField != option.fields.end())
  {
    if ((*itField).name == fieldname)
    {
      return atoi((*itField).value.c_str());
    }
    ++itField;
  }
  return 0;
}

/** Return the value of the option as a string */
std::string
MetaCommand::GetValueAsString(const std::string& optionName, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = optionName;
  }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      auto itField = (*it).fields.begin();
      while (itField != (*it).fields.end())
      {
        if ((*itField).name == fieldname)
        {
          return (*itField).value;
        }
        ++itField;
      }
    }
    ++it;
  }
  return "";
}

/** Return the value of the option as a string */
std::string
MetaCommand::GetValueAsString(Option option, const std::string& fieldName)
{
  std::string fieldname = fieldName;
  if (fieldName.empty())
  {
    fieldname = option.name;
  }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while (itField != option.fields.end())
  {
    if ((*itField).name == fieldname)
    {
      return (*itField).value;
    }
    ++itField;
  }
  return "";
}

/** Return the value of the option as a list of strings */
std::list<std::string>
MetaCommand::GetValueAsList(Option option)
{
  std::list<std::string> results;
  results.clear();
  std::vector<Field>::const_iterator itField = option.fields.begin();
  ++itField;
  while (itField != option.fields.end())
  {
    results.push_back((*itField).value);
    ++itField;
  }
  return results;
}

std::list<std::string>
MetaCommand::GetValueAsList(const std::string& optionName)
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      return this->GetValueAsList(*it);
    }
    ++it;
  }
  std::list<std::string> empty;
  empty.clear();
  return empty;
}

bool
MetaCommand::GetOptionWasSet(const Option& option)
{
  if (option.userDefined)
  {
    return true;
  }
  return false;
}

bool
MetaCommand::GetOptionWasSet(const std::string& optionName)
{
  OptionVector::const_iterator it = m_ParsedOptionVector.begin();
  while (it != m_ParsedOptionVector.end())
  {
    if ((*it).name == optionName)
    {
      return true;
    }
    ++it;
  }
  return false;
}

/** List the current options */
void
MetaCommand::ListOptions()
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  int                          i = 0;
  while (it != m_OptionVector.end())
  {
    std::cout << "Option #" << i << '\n';
    std::cout << "   Name: " << (*it).name.c_str() << '\n';
    if (!(*it).tag.empty())
    {
      std::cout << "   Tag: " << (*it).tag.c_str() << '\n';
    }
    if (!(*it).longtag.empty())
    {
      std::cout << "   LongTag: " << (*it).longtag.c_str() << '\n';
    }
    std::cout << "   Description: " << (*it).description.c_str() << '\n';
    if ((*it).required)
    {
      std::cout << "   Required: true" << '\n';
    }
    else
    {
      std::cout << "   Required: false" << '\n';
    }
    std::cout << "   Number of expected values: " << (*it).fields.size() << '\n';

    auto itField = (*it).fields.begin();
    while (itField != (*it).fields.end())
    {
      std::cout << "      Field Name: " << (*itField).name.c_str() << '\n';
      std::cout << "      Description: " << (*itField).description.c_str() << '\n';
      std::cout << "      Type: " << MetaCommand::TypeToString((*itField).type).c_str() << '\n';
      std::cout << "      Value: " << (*itField).value.c_str() << '\n';
      if ((*itField).type == ENUM)
      {
        std::cout << "      Enum list: " << (*itField).rangeMin.c_str() << '\n';
      }
      else
      {
        std::cout << "      RangeMin: " << (*itField).rangeMin.c_str() << '\n';
        std::cout << "      RangeMax: " << (*itField).rangeMax.c_str() << '\n';
      }

      if ((*itField).externaldata)
      {
        std::cout << "      External Data: true" << '\n';
      }
      else
      {
        std::cout << "      External Data: false" << '\n';
      }

      if ((*itField).required)
      {
        std::cout << "      Required: true" << '\n';
      }
      else
      {
        std::cout << "      Required: false" << '\n';
      }

      if ((*itField).userDefined)
      {
        std::cout << "      User Defined: true" << '\n';
      }
      else
      {
        std::cout << "      User Defined: false" << '\n';
      }

      ++itField;
    }
    std::cout << '\n';
    i++;
    ++it;
  }
  if (m_HelpCallBack != nullptr)
  {
    m_HelpCallBack();
  }
}

/** List the current options in xml format */
void
MetaCommand::ListOptionsXML()
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  int                          i = 0;
  while (it != m_OptionVector.end())
  {
    std::cout << "<option>" << '\n';
    std::cout << "<number>" << i << "</number>" << '\n';
    std::cout << "<name>" << (*it).name.c_str() << "</name>" << '\n';
    std::cout << "<tag>" << (*it).tag.c_str() << "</tag>" << '\n';
    std::cout << "<longtag>" << (*it).longtag.c_str() << "</longtag>" << '\n';
    std::cout << "<description>" << (*it).description.c_str() << "</description>" << '\n';
    std::cout << "<required>";
    if ((*it).required)
    {
      std::cout << "1</required>" << '\n';
    }
    else
    {
      std::cout << "0</required>" << '\n';
    }

    std::cout << "<nvalues>" << (*it).fields.size() << "</nvalues>" << '\n';

    auto itField = (*it).fields.begin();
    while (itField != (*it).fields.end())
    {
      std::cout << "<field>" << '\n';
      std::cout << "<name>" << (*itField).name.c_str() << "</name>" << '\n';
      std::cout << "<description>" << (*itField).description.c_str() << "</description>" << '\n';
      std::cout << "<type>" << MetaCommand::TypeToString((*itField).type).c_str() << "</type>" << '\n';
      std::cout << "<value>" << (*itField).value.c_str() << "</value>" << '\n';
      std::cout << "<external>";
      if ((*itField).externaldata == DATA_IN)
      {
        std::cout << "1</external>" << '\n';
      }
      else if ((*itField).externaldata == DATA_OUT)
      {
        std::cout << "2</external>" << '\n';
      }
      else
      {
        std::cout << "0</external>" << '\n';
      }
      std::cout << "<required>";
      if ((*itField).required)
      {
        std::cout << "1</required>" << '\n';
      }
      else
      {
        std::cout << "0</required>" << '\n';
      }


      std::cout << "</field>" << '\n';
      ++itField;
    }
    std::cout << "</option>" << '\n';
    i++;
    ++it;
  }
}

/** Used by ListOptionsSlicerXML */
void
MetaCommand::WriteXMLOptionToCout(const std::string& optionName, unsigned int & index)
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if (!strcmp((*it).name.c_str(), optionName.c_str()))
    {
      break;
    }
    ++it;
  }

  auto itField = (*it).fields.begin();

  std::string optionType;

  if ((*itField).type == MetaCommand::STRING &&
      ((*itField).externaldata == MetaCommand::DATA_IN || (*itField).externaldata == MetaCommand::DATA_OUT))
  {
    optionType = "image";
  }
  else if ((*itField).type == MetaCommand::FLAG)
  {
    optionType = "boolean";
  }
  else if ((*itField).type == MetaCommand::INT)
  {
    optionType = "integer";
  }
  else if ((*itField).type == MetaCommand::ENUM)
  {
    optionType = "string-enumeration";
  }
  else
  {
    optionType = MetaCommand::TypeToString((*itField).type);
  }

  std::cout << "<" << optionType.c_str() << ">" << '\n';


  std::cout << "<name>" << (*it).name.c_str() << "</name>" << '\n';
  // Label is the description for now
  std::string label = (*it).label;
  if (label.empty())
  {
    label = (*it).name;
  }

  std::cout << "<label>" << label.c_str() << "</label>" << '\n';
  std::cout << "<description>" << (*it).description.c_str() << "</description>" << '\n';
  if (!(*it).tag.empty()) // use the single by default flag if any
  {
    std::cout << "<flag>" << (*it).tag.c_str() << "</flag>" << '\n';
  }
  else if (!(*it).longtag.empty())
  {
    std::cout << "<longflag>" << (*it).longtag.c_str() << "</longflag>" << '\n';
  }
  else
  {
    std::cout << "<index>" << index << "</index>" << '\n';
    index++;
  }

  if (!(*itField).value.empty())
  {
    std::cout << "<default>" << (*itField).value.c_str() << "</default>" << '\n';
  }

  if ((*itField).externaldata == MetaCommand::DATA_IN)
  {
    std::cout << "<channel>input</channel>" << '\n';
  }
  else if ((*itField).externaldata == MetaCommand::DATA_OUT)
  {
    std::cout << "<channel>output</channel>" << '\n';
  }

  if ((*itField).type == MetaCommand::ENUM)
  {
    std::vector<std::string> enumVector;
    MET_StringToVector<std::string>((*itField).rangeMin, enumVector);
    std::vector<std::string>::iterator itenum;
    itenum = enumVector.begin();
    while (itenum != enumVector.end())
    {
      std::cout << "<element>" << (*itenum).c_str() << "</element>" << '\n';
      ++itenum;
    }
  }

  // Write out the closing tag
  std::cout << "</" << optionType.c_str() << ">" << '\n';
}

/** List the current options in Slicer's xml format (www.slicer.org) */
void
MetaCommand::ListOptionsSlicerXML()
{
  std::cout << R"(<?xml version="1.0" encoding="utf-8"?>)" << '\n';
  std::cout << "<executable>" << '\n';
  std::cout << "  <category>" << m_Category.c_str() << "</category>" << '\n';
  std::cout << "  <title>" << m_Name.c_str() << "</title>" << '\n';
  std::cout << "  <description>" << '\n';
  std::cout << "  " << m_Description.c_str() << '\n';
  std::cout << "  </description>" << '\n';
  std::cout << "  <version>" << m_Version.c_str() << "</version>" << '\n';
  std::cout << "  <contributor>" << m_Author.c_str() << "</contributor>" << '\n';
  std::cout << "  <documentation-url></documentation-url>" << '\n';
  std::cout << "  <license></license>" << '\n';
  std::cout << "  <acknowledgements>" << '\n';
  std::cout << "  " << m_Acknowledgments.c_str() << '\n';
  std::cout << "  </acknowledgements>" << '\n';

  // Organize by group first
  // Keep a list of options
  unsigned int                         index = 0;
  std::vector<std::string>             GroupedOptionVector;
  ParameterGroupVector::const_iterator itGroup = m_ParameterGroup.begin();
  while (itGroup != m_ParameterGroup.end())
  {
    if ((*itGroup).advanced == true)
    {
      std::cout << " <parameters advanced=\"true\">" << '\n';
    }
    else
    {
      std::cout << " <parameters>" << '\n';
    }
    std::cout << "  <label>" << (*itGroup).name.c_str() << "</label>" << '\n';

    if ((*itGroup).description.empty())
    {
      std::cout << "  <description>" << (*itGroup).name.c_str() << "</description>" << '\n';
    }
    else
    {
      std::cout << "  <description>" << (*itGroup).description.c_str() << "</description>" << '\n';
    }

    auto itOption = (*itGroup).options.begin();
    while (itOption != (*itGroup).options.end())
    {
      this->WriteXMLOptionToCout(*itOption, index);
      GroupedOptionVector.push_back(*itOption);
      ++itOption;
    }
    std::cout << " </parameters>" << '\n';
    ++itGroup;
  }

  // Then take the remaining options
  if (m_OptionVector.size() > GroupedOptionVector.size())
  {
    std::cout << " <parameters>" << '\n';
    std::cout << "  <label>IO</label>" << '\n';
    std::cout << "  <description>Input/output parameters</description>" << '\n';

    OptionVector::const_iterator it = m_OptionVector.begin();
    while (it != m_OptionVector.end())
    {
      bool                                     optionIsGrouped = false;
      std::vector<std::string>::const_iterator itGroupedOption = GroupedOptionVector.begin();
      while (itGroupedOption != GroupedOptionVector.end())
      {
        if (!strcmp((*itGroupedOption).c_str(), (*it).name.c_str()))
        {
          optionIsGrouped = true;
          break;
        }
        ++itGroupedOption;
      }

      if (!optionIsGrouped)
      {
        this->WriteXMLOptionToCout((*it).name, index);
      }
      ++it;
    } // end loop option

    std::cout << " </parameters>" << '\n';
  } // end m_OptionVector.size()>GroupedOptionVector.size()

  std::cout << "</executable>" << '\n';
}


/** Internal small XML parser */
std::string
MetaCommand::GetXML(const char * buffer, const char * desc, unsigned long pos)
{
  std::string begin = "<";
  begin += desc;
  begin += ">";
  std::string end = "</";
  end += desc;
  end += ">";

  std::string buf = buffer;

  auto posb = static_cast<long int>(buf.find(begin, pos));
  if (posb == -1)
  {
    return "";
  }
  auto pose = static_cast<long int>(buf.find(end, static_cast<unsigned long>(posb)));
  if (pose == -1)
  {
    return "";
  }

  return buf.substr(posb + begin.size(), pose - posb - begin.size());
}

/** Given an XML buffer fill in the command line arguments */
bool
MetaCommand::ParseXML(const char * buffer)
{
  m_OptionVector.clear();
  std::string buf = this->GetXML(buffer, "option", 0);
  long        pos = 0;
  while (!buf.empty())
  {
    Option option;
    option.userDefined = false;
    option.complete = false;
    option.name = this->GetXML(buf.c_str(), "name", 0);
    option.tag = this->GetXML(buf.c_str(), "tag", 0);
    option.longtag = this->GetXML(buf.c_str(), "longtag", 0);
    option.description = this->GetXML(buf.c_str(), "description", 0);
    if (atoi(this->GetXML(buf.c_str(), "required", 0).c_str()) == 0)
    {
      option.required = false;
    }
    else
    {
      option.required = true;
    }
    unsigned int n = static_cast<unsigned int>(atoi(this->GetXML(buf.c_str(), "nvalues", 0).c_str()));

    // Now check the fields
    long posF = static_cast<long>(buf.find("<field>"));
    for (unsigned int i = 0; i < n; i++)
    {
      std::string f = this->GetXML(buf.c_str(), "field", static_cast<unsigned long>(posF));
      Field       field;
      field.userDefined = false;
      field.name = this->GetXML(f.c_str(), "name", 0);
      field.description = this->GetXML(f.c_str(), "description", 0);
      field.value = this->GetXML(f.c_str(), "value", 0);
      field.type = MetaCommand::StringToType(this->GetXML(f.c_str(), "type", 0).c_str());
      if (atoi(this->GetXML(f.c_str(), "external", 0).c_str()) == 0)
      {
        field.externaldata = DATA_NONE;
      }
      else
      {
        if (atoi(this->GetXML(f.c_str(), "external", 0).c_str()) == 1)
        {
          field.externaldata = DATA_IN;
        }
        else
        {
          field.externaldata = DATA_OUT;
        }
      }
      if (atoi(this->GetXML(f.c_str(), "required", 0).c_str()) == 0)
      {
        field.required = false;
      }
      else
      {
        field.required = true;
      }

      option.fields.push_back(field);
      posF += static_cast<long>(f.size() + 8);
    }

    m_OptionVector.push_back(option);

    pos += static_cast<long>(buf.size() + 17);
    buf = this->GetXML(buffer, "option", static_cast<unsigned long>(pos));
  }

  return true;
}


/** List the current options */
void
MetaCommand::ListOptionsSimplified(bool extended)
{
  if (extended)
  {
    std::cout << " System tags: " << '\n'
              << "   [ -v ] or [ -h ]" << '\n'
              << "      = List options in short format" << '\n'
              << "   [ -V ] or [ -H ]" << '\n'
              << "      = List options in long format" << '\n'
              << "   [ -vxml ] or [ -hxml ] or [ -exportXML ]" << '\n'
              << "      = List options in xml format for BatchMake" << '\n'
              << "   [ --xml ]" << '\n'
              << "      = List options in xml format for Slicer" << '\n'
              << "   [ -vgad ] or [ -hgad ] or [ -exportGAD ]" << '\n'
              << "      = List options in Grid Application Description format" << '\n'
              << "   [ -version ]" << '\n'
              << "      = return the version number" << '\n'
              << "   [ -date ]" << '\n'
              << "      = return the cvs checkout date"
#ifdef METAIO_USE_LIBXML2
              << std::endl
              << "   [ --loadArguments filename ]"
              << "      = load the arguments from an XML file"
#endif
              << '\n';
  }

  int                          count = 0;
  int                          ntags = 0;
  int                          nfields = 0;
  OptionVector::const_iterator it;
  it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if (!(*it).tag.empty() || !(*it).longtag.empty())
    {
      ntags++;
    }
    else
    {
      nfields++;
    }
    ++it;
  }
  while (count < 2)
  {
    if (count == 0)
    {
      if (ntags > 0)
      {
        std::cout << " Command tags: " << '\n';
      }
      else
      {
        count++;
      }
    }
    if (count == 1)
    {
      if (nfields > 0)
      {
        std::cout << " Command fields: " << '\n';
      }
      else
      {
        count++;
      }
    }
    count++;
    it = m_OptionVector.begin();
    while (it != m_OptionVector.end())
    {
      if ((count == 1 && (!(*it).tag.empty() || !(*it).longtag.empty())) ||
          (count == 2 && ((*it).tag.empty() && (*it).longtag.empty())))
      {
        if (!(*it).required)
        {
          std::cout << "   [ ";
        }
        else
        {
          std::cout << "   ";
        }
        if (!(*it).tag.empty())
        {
          std::cout << "-" << (*it).tag.c_str() << " ";
        }
        if (!(*it).longtag.empty())
        {
          std::cout << "--" << (*it).longtag.c_str() << " ";
        }
        auto itField = (*it).fields.begin();
        while (itField != (*it).fields.end())
        {
          // only display the type if it's not a FLAG
          if ((*itField).type != FLAG)
          {
            if ((*itField).required)
            {
              std::cout << "< ";
            }
            else
            {
              std::cout << "[ ";
            }

            std::cout << (*itField).name.c_str();

            if ((*itField).required)
            {
              std::cout << " > ";
            }
            else
            {
              std::cout << " ] ";
            }
          }
          ++itField;
        }

        if (!(*it).required)
        {
          std::cout << "]";
        }
        std::cout << '\n';

        if (!(*it).description.empty())
        {
          std::cout << "      = " << (*it).description.c_str();
          std::cout << '\n';
          itField = (*it).fields.begin();
          while (itField != (*it).fields.end())
          {
            if (!(*itField).description.empty() || !(*itField).value.empty())
            {
              std::cout << "        With: " << (*itField).name.c_str();
              if (!(*itField).description.empty())
              {
                std::cout << " = " << (*itField).description.c_str();
              }
              if (!(*itField).value.empty())
              {
                std::cout << " (Default = " << (*itField).value.c_str() << ")";
              }
              std::cout << '\n';
            }
            ++itField;
          }
        }
      }
      ++it;
    }
  }

  if (m_HelpCallBack != nullptr)
  {
    m_HelpCallBack();
  }
}

/** Get the option by "-"+tag
 *  or by "--"+longtag */
bool
MetaCommand::OptionExistsByMinusTag(const std::string& minusTag)
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    std::string tagToSearch = "-";
    tagToSearch += (*it).tag;
    std::string longtagToSearch = "--";
    longtagToSearch += (*it).longtag;
    std::string longtagToSearchBackwardCompatible = "-";
    longtagToSearchBackwardCompatible += (*it).longtag;
    // WARNING: This is for backward compatibility but a warning
    // is going to be thrown if used so that people can adjust
    if (tagToSearch == minusTag || longtagToSearch == minusTag || longtagToSearchBackwardCompatible == minusTag)
    {
      return true;
    }
    ++it;
  }
  return false;
}

/** Get the option by "-"+tag
 *  or by "--"+longtag */
MetaCommand::Option *
MetaCommand::GetOptionByMinusTag(const std::string& minusTag)
{
  auto it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    std::string tagToSearch = "-";
    tagToSearch += (*it).tag;
    std::string longtagToSearch = "--";
    longtagToSearch += (*it).longtag;
    std::string longtagToSearchBackwardCompatible = "-";
    longtagToSearchBackwardCompatible += (*it).longtag;

    // WARNING: This is for backward compatibility but a warning
    // is going to be thrown if used so that people can adjust
    if (tagToSearch == minusTag || longtagToSearch == minusTag || longtagToSearchBackwardCompatible == minusTag)
    {
      return &(*it);
    }
    ++it;
  }
  return nullptr;
}

/** Get the option by tag */
MetaCommand::Option *
MetaCommand::GetOptionByTag(const std::string& tag)
{
  auto it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).tag == tag || (*it).longtag == tag)
    {
      return &(*it);
    }
    ++it;
  }
  return nullptr;
}

/** Return the option id. i.e the position in the vector */
long
MetaCommand::GetOptionId(Option * option)
{
  auto          it = m_OptionVector.begin();
  unsigned long i = 0;
  while (it != m_OptionVector.end())
  {
    if (&(*it) == option)
    {
      return i;
    }
    i++;
    ++it;
  }
  return -1;
}

/** Export the current command line arguments to a Grid Application
 *  Description file */
bool
MetaCommand::ExportGAD(bool dynamic)
{
  std::cout << "Exporting GAD file...";

  OptionVector options = m_OptionVector;
  if (dynamic)
  {
    options = m_ParsedOptionVector;
  }

  if (m_Name.empty())
  {
    std::cout << "Set the name of the application using SetName()" << '\n';
    return false;
  }

  std::string filename = m_Name;
  filename += ".gad.xml";

  METAIO_STREAM::ofstream file;
#ifdef __sgi
  file.open(filename.c_str(), std::ios::out);
#else
  file.open(filename.c_str(), std::ios::binary | std::ios::out);
#endif
  if (!file.rdbuf()->is_open())
  {
    std::cout << "Cannot open file for writing: " << filename.c_str() << '\n';
    return false;
  }

  file << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << '\n';
  file << "<gridApplication" << '\n';
  file << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << '\n';
  file << "xsi:noNamespaceSchemaLocation=\"grid-application-description.xsd\"" << '\n';
  file << "name=\"" << m_Name.c_str() << "\"" << '\n';
  file << "description=\"" << m_Description.c_str() << "\">" << '\n';
  file << R"(<applicationComponent name="Client" remoteExecution="true">)" << '\n';
  file << "<componentActionList>" << '\n';
  file << '\n';

  unsigned int order = 1;
  // Write out the input data to be transferred
  OptionVector::const_iterator it = options.begin();
  while (it != options.end())
  {
    auto itFields = (*it).fields.begin();
    while (itFields != (*it).fields.end())
    {
      if ((*itFields).externaldata == DATA_IN)
      {
        file << R"( <componentAction type="DataRelocation" order=")" << order << "\">" << '\n';
        file << R"(  <parameter name="Name" value=")" << (*itFields).name.c_str() << "\"/>" << '\n';
        file << R"(  <parameter name="Host" value="hostname"/>)" << '\n';
        file << R"(  <parameter name="Description" value=")" << (*itFields).description.c_str() << "\"/>" << '\n';
        file << R"(  <parameter name="Direction" value="In"/>)" << '\n';
        file << R"(  <parameter name="Protocol" value="gsiftp"/>)" << '\n';
        file << R"(  <parameter name="SourceDataPath" value=")" << (*itFields).value.c_str() << "\"/>" << '\n';

        std::string datapath = (*itFields).value;
        auto        slash = static_cast<long int>(datapath.find_last_of('/'));
        if (slash > 0)
        {
          datapath = datapath.substr(static_cast<unsigned long>(slash + 1), datapath.size() - slash - 1);
        }
        slash = static_cast<long int>(datapath.find_last_of('\\'));
        if (slash > 0)
        {
          datapath = datapath.substr(static_cast<unsigned long>(slash + 1), datapath.size() - slash - 1);
        }
        file << R"(  <parameter name="DestDataPath" value=")" << datapath.c_str() << "\"/>" << '\n';
        file << " </componentAction>" << '\n';
        file << '\n';
        order++;
      }
      ++itFields;
    }
    ++it;
  }

  file << R"( <componentAction type="JobSubmission" order=")" << order << "\">" << '\n';
  file << R"(  <parameter name="Executable" value=")" << m_ExecutableName.c_str() << "\"/>" << '\n';
  file << R"(  <parameter name="Arguments"  value=")";
  // Write out the command line arguments
  it = options.begin();
  while (it != options.end())
  {
    if (it != options.begin())
    {
      file << " ";
    }
    file << "{" << (*it).name.c_str() << "}";
    ++it;
  }
  file << "\"/>" << '\n';
  // Write out the arguments that are not data
  it = options.begin();
  while (it != options.end())
  {
    // Find if this is a non data field
    bool isData = false;
    auto itFields = (*it).fields.begin();
    while (itFields != (*it).fields.end())
    {
      if ((*itFields).externaldata != DATA_NONE)
      {
        isData = true;
        break;
      }
      ++itFields;
    }

    if (isData)
    {
      ++it;
      continue;
    }

    file << "   <group name=\"" << (*it).name.c_str();
    file << "\" syntax=\"";

    if (!(*it).tag.empty())
    {
      file << "-" << (*it).tag.c_str() << " ";
    }

    itFields = (*it).fields.begin();
    while (itFields != (*it).fields.end())
    {
      if (itFields != (*it).fields.begin())
      {
        file << " ";
      }
      file << "{" << (*it).name.c_str() << (*itFields).name.c_str() << "}";
      ++itFields;
    }
    file << "\"";

    if (!(*it).required)
    {
      file << " optional=\"true\"";

      // Add if the option was selected
      if ((*it).userDefined)
      {
        file << " selected=\"true\"";
      }
      else
      {
        file << " selected=\"false\"";
      }
    }

    file << ">" << '\n';

    // Now writes the value of the arguments
    itFields = (*it).fields.begin();
    while (itFields != (*it).fields.end())
    {
      file << "    <argument name=\"" << (*it).name.c_str() << (*itFields).name.c_str();
      file << "\" value=\"" << (*itFields).value.c_str();
      file << "\" type=\"" << MetaCommand::TypeToString((*itFields).type).c_str();
      file << "\"";

      if (!(*itFields).rangeMin.empty())
      {
        file << " rangeMin=\"" << (*itFields).rangeMin.c_str() << "\"";
      }

      if (!(*itFields).rangeMax.empty())
      {
        file << " rangeMax=\"" << (*itFields).rangeMax.c_str() << "\"";
      }
      file << "/>" << '\n';
      ++itFields;
    }
    file << "  </group>" << '\n';
    ++it;
  }
  file << " </componentAction>" << '\n';
  order++;
  file << '\n';
  // Write out the input data to be transferred
  it = options.begin();
  while (it != options.end())
  {
    auto itFields = (*it).fields.begin();
    while (itFields != (*it).fields.end())
    {
      if ((*itFields).externaldata == DATA_OUT)
      {
        file << R"( <componentAction type="DataRelocation" order=")" << order << "\">" << '\n';
        file << R"(  <parameter name="Name" Value=")" << (*itFields).name.c_str() << "\"/>" << '\n';
        file << R"(  <parameter name="Host" Value="hostname"/>)" << '\n';
        file << R"(  <parameter name="Description" value=")" << (*itFields).description.c_str() << "\"/>" << '\n';
        file << R"(  <parameter name="Direction" value="Out"/>)" << '\n';
        file << R"(  <parameter name="Protocol" value="gsiftp"/>)" << '\n';
        std::string datapath = (*itFields).value;
        auto        slash = static_cast<long int>(datapath.find_last_of('/'));
        if (slash > 0)
        {
          datapath = datapath.substr(static_cast<unsigned long>(slash + 1), datapath.size() - slash - 1);
        }
        slash = static_cast<long int>(datapath.find_last_of('\\'));
        if (slash > 0)
        {
          datapath = datapath.substr(static_cast<unsigned long>(slash + 1), datapath.size() - slash - 1);
        }
        file << R"(  <parameter name="SourceDataPath" value=")" << datapath.c_str() << "\"/>" << '\n';
        file << R"(  <parameter name="DestDataPath" value=")" << (*itFields).value.c_str() << "\"/>" << '\n';
        file << " </componentAction>" << '\n';
        file << '\n';
        order++;
      }
      ++itFields;
    }
    ++it;
  }
  file << "    </componentActionList>" << '\n';
  file << "  </applicationComponent>" << '\n';
  file << "</gridApplication>" << '\n';

  file.close();

  std::cout << "done" << '\n';
  return true;
}


/** Parse the command line */
bool
MetaCommand::Parse(int argc, char ** const argv)
{
  m_GotXMLFlag = false;
  m_ExecutableName = argv[0];

  auto slash = static_cast<long int>(m_ExecutableName.find_last_of('/'));
  if (slash > 0)
  {
    m_ExecutableName = m_ExecutableName.substr(static_cast<unsigned long>(slash + 1), m_ExecutableName.size() - slash - 1);
  }
  slash = static_cast<long int>(m_ExecutableName.find_last_of('\\'));
  if (slash > 0)
  {
    m_ExecutableName = m_ExecutableName.substr(static_cast<unsigned long>(slash + 1), m_ExecutableName.size() - slash - 1);
  }

  // Fill in the results
  m_ParsedOptionVector.clear();
  bool        inArgument = false;
  std::string tag;

  unsigned long currentField = 0;  // current field position
  long          currentOption = 0; // id of the option to fill
  unsigned int  valuesRemaining = 0;
  unsigned int  optionalValuesRemaining = 0;
  bool          isComplete = false; // check if the option should be parse until
                                    // the next tag is found
  std::string completeString;

  bool exportGAD = false;
  for (unsigned int i = 1; i < static_cast<unsigned int>(argc); i++)
  {
    if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "-H"))
    {
      std::cout << "Usage : " << argv[0] << '\n';
      this->ListOptions();
      return true;
    }
    // List the options if using -v
    if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-h"))
    {
      std::cout << "Usage : " << argv[0] << '\n';
      this->ListOptionsSimplified();
      return true;
    }
    // List the options if using -v
    if (!strcmp(argv[i], "--loadArguments"))
    {
      if ((i + 1) >= static_cast<unsigned int>(argc))
      {
        std::cout << "--loadArguments expected a filename as argument" << '\n';
        return false;
      }
      MetaCommand::LoadArgumentsFromXML(argv[i + 1]);
      i++;
      continue;
    }
    if (!strcmp(argv[i], "-vxml") || !strcmp(argv[i], "-hxml") || !strcmp(argv[i], "-exportXML") ||
        !strcmp(argv[i], "--vxml") || !strcmp(argv[i], "--hxml") || !strcmp(argv[i], "--exportXML"))
    {
      this->ListOptionsXML();
      continue;
    }
    if (!strcmp(argv[i], "--xml"))
    {
      this->ListOptionsSlicerXML();
      m_GotXMLFlag = true;
      return false;
    }
    if (!strcmp(argv[i], "-version"))
    {
      std::cout << "Version: " << m_Version.c_str() << '\n';
      continue;
    }
    if (!strcmp(argv[i], "-date"))
    {
      std::cout << "Date: " << m_Date.c_str() << '\n';
      continue;
    }
    if (!strcmp(argv[i], "-exportGAD") || !strcmp(argv[i], "-vgad") || !strcmp(argv[i], "-hgad"))
    {
      this->ExportGAD();
      exportGAD = true;
      continue;
    }

    // If this is a tag
    if (argv[i][0] == '-' && (atof(argv[i]) == 0) && (strlen(argv[i]) > 1))
    {
      // if we have a tag before the expected values we throw an exception
      if (valuesRemaining != 0)
      {
        if (!isComplete)
        {
          if (optionalValuesRemaining > 0)
          {
            valuesRemaining = 0;
            m_OptionVector[currentOption].userDefined = true;
            m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
          }
          else
          {
            std::cout << "Found tag " << argv[i] << " before end of value list!" << '\n';
            return false;
          }
        }
        else
        {
          m_OptionVector[currentOption].fields[0].value = completeString;
          m_OptionVector[currentOption].fields[0].userDefined = true;
          m_OptionVector[currentOption].userDefined = true;
          m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
          valuesRemaining = 0;
        }
      }
      inArgument = false;
      // New tag so we add the previous values to the tag
      tag = argv[i];

      // Check if the tag is in the list
      if (this->OptionExistsByMinusTag(tag))
      {
        inArgument = true;

        // We check the number of mandatory and optional values for
        // this tag
        std::vector<Field>::const_iterator fIt = this->GetOptionByMinusTag(tag)->fields.begin();
        while (fIt != this->GetOptionByMinusTag(tag)->fields.end())
        {
          if (!(*fIt).required)
          {
            optionalValuesRemaining++;
          }
          valuesRemaining++;
          ++fIt;
        }
        currentOption = this->GetOptionId(this->GetOptionByMinusTag(tag));

        if (currentOption < 0)
        {
          std::cout << "Error processing tag " << tag.c_str() << ".  Tag exists but cannot find its Id." << '\n';
        }
        else
        {
          isComplete = m_OptionVector[currentOption].complete;

          if (m_OptionVector[currentOption].fields[0].type == FLAG)
          {
            // the tag exists by default
            m_OptionVector[currentOption].fields[0].value = "true";
            valuesRemaining = 0;
            optionalValuesRemaining = 0;
            inArgument = false;
          }
          else if (m_OptionVector[currentOption].fields[0].type == LIST)
          {
            inArgument = true;
            auto valuesInList = static_cast<unsigned int>(atoi(argv[++i]));
            m_OptionVector[currentOption].fields[0].value = argv[i];
            valuesRemaining += valuesInList - 1;
            char optName[255];
            for (unsigned int j = 0; j < valuesInList; j++)
            {
              snprintf(optName, sizeof(optName), "%03u", j);
              this->AddOptionField(m_OptionVector[currentOption].name, optName, STRING);
            }
          }
        }
      }
      else
      {
        if (m_Verbose)
        {
          std::cout << "The tag " << tag.c_str() << " is not a valid argument : skipping this tag" << '\n';
        }
        if (m_FailOnUnrecognizedOption)
        {
          return false;
        }
      }
      if (inArgument)
      {
        i++;
      }
    }
    else if (!inArgument) // If this is a field
    {
      // Look for the field to add
      auto          it = m_OptionVector.begin();
      unsigned long pos = 0;
      bool          found = false;
      while (it != m_OptionVector.end())
      {
        if ((pos >= currentField) && ((*it).tag.empty() && (*it).longtag.empty()))
        {
          currentOption = pos;
          valuesRemaining = static_cast<unsigned int>((*it).fields.size());
          found = true;
          break;
        }
        pos++;
        ++it;
      }

      if (!found && m_Verbose)
      {
        std::cout << "Too many arguments specified in your command line! "
                  << "Skipping extra argument: " << argv[i] << '\n';
      }

      inArgument = true;
      currentField = static_cast<unsigned long>(currentOption + 1);
    }

    // We collect the values
    if (isComplete && static_cast<int>(i) < argc)
    {
      if (completeString.empty())
      {
        completeString = argv[i];
      }
      else
      {
        completeString += " ";
        completeString += argv[i];
      }
    }
    else if (inArgument && i < static_cast<unsigned int>(argc) && (valuesRemaining > 0))
    {
      // We check that the current value is not a tag.
      // This might be the case when we have optional fields
      if (this->OptionExistsByMinusTag(argv[i]) && optionalValuesRemaining > 0)
      {
        valuesRemaining = 0;
        optionalValuesRemaining = 0;
        i--; // the outter loop will take care of incrementing it.
      }

      else if (currentOption >= 0 && currentOption < static_cast<int>(m_OptionVector.size()))
      {
        auto s = m_OptionVector[currentOption].fields.size();

        // We change the value only if this is not a tag
        if (this->OptionExistsByMinusTag(argv[i]))
        {
          std::cout << "Option " << m_OptionVector[currentOption].name.c_str()
                    << " expect a value and got tag: " << argv[i] << '\n';
          this->ListOptionsSimplified(false);
          return false;
        }

        m_OptionVector[currentOption].fields[s - (valuesRemaining)].value = argv[i];

        m_OptionVector[currentOption].fields[s - (valuesRemaining)].userDefined = true;

        if (!m_OptionVector[currentOption].fields[s - (valuesRemaining)].required)
        {
          optionalValuesRemaining--;
        }

        valuesRemaining--;
      }
      else if (valuesRemaining > 0)
      {
        valuesRemaining--;
      }
    }
    else if (valuesRemaining == optionalValuesRemaining && i == static_cast<unsigned int>(argc) && (optionalValuesRemaining > 0))
    // if this is the last argument and all the remaining values are optionals
    {
      if (this->OptionExistsByMinusTag(argv[i - 1]))
      {
        valuesRemaining = 0;
        optionalValuesRemaining = 0;
      }
    }

    if (valuesRemaining == 0)
    {
      inArgument = false;
      m_OptionVector[currentOption].userDefined = true;
      m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
    }

  } // end loop command line arguments

  if (isComplete) // If we are still in the isComplete mode we add the option
  {
    m_OptionVector[currentOption].fields[0].value = completeString;
    m_OptionVector[currentOption].fields[0].userDefined = true;
    m_OptionVector[currentOption].userDefined = true;
    m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
    valuesRemaining = 0;
  }

  if (optionalValuesRemaining > 0 && optionalValuesRemaining == valuesRemaining)
  {
    valuesRemaining = 0;
    m_OptionVector[currentOption].userDefined = true;
    m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
  }

  if (valuesRemaining > 0)
  {
    std::cout << "Not enough parameters for " << m_OptionVector[currentOption].name.c_str() << '\n';
    std::cout << "Usage: " << argv[0] << '\n';
    this->ListOptionsSimplified(false);
    return false;
  }

  // Check if the options with required arguments are defined
  auto it = m_OptionVector.begin();
  bool requiredAndNotDefined = false;
  while (it != m_OptionVector.end())
  {
    if ((*it).required)
    {
      // First check if the option is actually defined
      if (!(*it).userDefined)
      {
        std::cout << "Option " << (*it).name.c_str() << " is required but not defined" << '\n';
        requiredAndNotDefined = true;
        ++it;
        continue;
      }

      // Check if the values are defined
      std::vector<Field>::const_iterator itFields = (*it).fields.begin();
      bool                               defined = true;
      while (itFields != (*it).fields.end())
      {
        if ((*itFields).value.empty())
        {
          defined = false;
        }
        ++itFields;
      }

      if (!defined)
      {
        if (!(*it).tag.empty() || !(*it).longtag.empty())
        {
          std::cout << "Field " << (*it).tag.c_str() << " is required but not defined" << '\n';
        }
        else
        {
          std::cout << "Field " << (*it).name.c_str() << " is required but not defined" << '\n';
        }
        requiredAndNotDefined = true;
      }
    }
    ++it;
  }

  if (requiredAndNotDefined)
  {
    // std::cout << "Command: " << argv[0] << std::endl;
    this->ListOptionsSimplified(false);
    return false;
  }

  // Check if the values are in range (if the range is defined)
  auto itParsed = m_ParsedOptionVector.begin();
  bool valueInRange = true;
  while (itParsed != m_ParsedOptionVector.end())
  {
    std::vector<Field>::const_iterator itFields = (*itParsed).fields.begin();
    while (itFields != (*itParsed).fields.end())
    {
      // Check only if this is a number
      if (((*itFields).type == INT || (*itFields).type == FLOAT || (*itFields).type == CHAR) &&
          (!(*itFields).value.empty()))
      {
        // Check the range min
        if (((!(*itFields).rangeMin.empty()) &&
             (atof((*itFields).rangeMin.c_str()) > atof((*itFields).value.c_str()))) ||
            ((!(*itFields).rangeMax.empty()) && (atof((*itFields).rangeMax.c_str()) < atof((*itFields).value.c_str()))))
        {
          std::cout << (*itParsed).name.c_str() << "." << (*itFields).name.c_str() << " : Value ("
                    << (*itFields).value.c_str() << ") "
                    << "is not in the range [" << (*itFields).rangeMin.c_str() << "," << (*itFields).rangeMax.c_str()
                    << "]" << '\n';
          valueInRange = false;
        }
      }
      ++itFields;
    }
    ++itParsed;
  }

  if (!valueInRange)
  {
    return false;
  }

  // If everything is ok
  if (exportGAD)
  {
    this->ExportGAD(true);
    return false; // prevent from running the application
  }

  return true;
}

/** Return the string representation of a type */
std::string
MetaCommand::TypeToString(TypeEnumType type)
{
  switch (type)
  {
    case INT:
      return "int";
    case FLOAT:
      return "float";
    case STRING:
      return "string";
    case LIST:
      return "list";
    case FLAG:
      return "flag";
    case BOOL:
      return "boolean";
    case IMAGE:
      return "image";
    case FILE:
      return "file";
    case ENUM:
      return "enum";
    case CHAR:
    default:
      return "not defined";
  }
}


/** Return a type given a string */
MetaCommand::TypeEnumType
MetaCommand::StringToType(const char * type)
{
  if (!strcmp(type, "int"))
  {
    return INT;
  }
  else if (!strcmp(type, "float"))
  {
    return FLOAT;
  }
  else if (!strcmp(type, "string"))
  {
    return STRING;
  }
  else if (!strcmp(type, "list"))
  {
    return LIST;
  }
  else if (!strcmp(type, "flag"))
  {
    return FLAG;
  }
  else if (!strcmp(type, "bool"))
  {
    return BOOL;
  }
  else if (!strcmp(type, "image"))
  {
    return IMAGE;
  }
  else if (!strcmp(type, "enum"))
  {
    return ENUM;
  }
  else if (!strcmp(type, "file"))
  {
    return FILE;
  }

  return INT; // by default
}


/** Set the long flag for the option */
bool
MetaCommand::SetOptionLongTag(const std::string& optionName, const std::string& longTag)
{
  auto itOption = m_OptionVector.begin();
  while (itOption != m_OptionVector.end())
  {
    if (!strcmp((*itOption).name.c_str(), optionName.c_str()))
    {
      (*itOption).longtag = longTag;
      return true;
    }
    ++itOption;
  }

  return false;
}

/** Set the label for the option */
bool
MetaCommand::SetOptionLabel(const std::string& optionName, const std::string& label)
{
  auto itOption = m_OptionVector.begin();
  while (itOption != m_OptionVector.end())
  {
    if (!strcmp((*itOption).name.c_str(), optionName.c_str()))
    {
      (*itOption).label = label;
      return true;
    }
    ++itOption;
  }

  return false;
}

/** Set the group for a field or an option
 *  If the group doesn't exist it is automatically created. */
bool
MetaCommand::SetParameterGroup(const std::string& optionName,
                               const std::string& groupName,
                               std::string groupDescription,
                               bool        advanced)
{
  // Check if the group exists
  ParameterGroup * group = nullptr;
  auto             it = m_ParameterGroup.begin();
  while (it != m_ParameterGroup.end())
  {
    if (!strcmp((*it).name.c_str(), groupName.c_str()))
    {
      group = &(*it);
    }
    ++it;
  }

  bool                         optionExists = false;
  unsigned int                 index = 0;
  OptionVector::const_iterator itOption = m_OptionVector.begin();
  while (itOption != m_OptionVector.end())
  {
    if (!strcmp((*itOption).name.c_str(), optionName.c_str()))
    {
      optionExists = true;
      break;
    }
    index++;
    ++itOption;
  }

  if (!optionExists)
  {
    std::cout << "The option " << optionName.c_str() << " doesn't exist" << '\n';
    return false;
  }

  if (!group)
  {
    ParameterGroup pgroup;
    pgroup.name = groupName;
    pgroup.description = std::move(groupDescription);
    pgroup.advanced = advanced;
    pgroup.options.push_back(optionName);
    m_ParameterGroup.push_back(pgroup);
  }
  else
  {
    group->options.push_back(optionName);
  }

  return true;
}

/** Load arguments from XML file */
bool
MetaCommand::LoadArgumentsFromXML(const char * filename, bool createMissingArguments)
{
#ifdef METAIO_USE_LIBXML2
  xmlDocPtr  doc;
  xmlNodePtr cur;
  doc = xmlParseFile(filename);

  if (doc == nullptr)
  {
    std::cerr << "Cannot parse XML file" << std::endl;
    return false;
  }

  cur = xmlDocGetRootElement(doc);

  if (cur == nullptr)
  {
    std::cerr << "XML document is empty" << std::endl;
    xmlFreeDoc(doc);
    return false;
  }
  if (xmlStrcmp(cur->name, (const xmlChar *)"MetaCommand"))
  {
    std::cerr << "document of the wrong type. Root node should be MetaCommand" << std::endl;
    xmlFreeDoc(doc);
    return false;
  }
  xmlCleanupParser();

  // Simple parsing (two levels hierarchy)
  cur = cur->children;
  while (cur)
  {
    xmlNodePtr child = cur->children;
    if (child)
    {
      xmlNodePtr subargument = child->next;
      while (subargument)
      {
        xmlNodePtr subargumentChild = subargument->children;
        if (subargumentChild && subargumentChild->content)
        {
          this->SetOptionValue((const char *)cur->name,
                               (const char *)subargument->name,
                               (const char *)subargumentChild->content,
                               createMissingArguments);
        }
        subargument = subargument->next;
      }

      if (child->content)
      {
        this->SetOptionValue(
          (const char *)cur->name, (const char *)cur->name, (const char *)child->content, createMissingArguments);
      }
    }
    cur = cur->next;
  }
  xmlFreeDoc(doc);
#else
  std::cout << "LoadArguments(" << filename << ") requires libxml2" << '\n';
  if (createMissingArguments)
  {
  }

#endif
  return true;
}

/** Set the value of an option or a field
 *  This is used when importing command line arguments
 *  from XML */
bool
MetaCommand::SetOptionValue(const char * optionName, const char * name, const char * value, bool createMissingArgument)
{
  auto it = m_OptionVector.begin();
  while (it != m_OptionVector.end())
  {
    if ((*it).name == optionName)
    {
      (*it).userDefined = true;
      std::vector<Field> & fields = (*it).fields;
      auto                 itField = fields.begin();
      while (itField != fields.end())
      {
        if ((*itField).name == name)
        {
          (*itField).userDefined = true;
          (*itField).value = value;
          return true;
        }
        ++itField;
      }
    }
    ++it;
  }

  if (createMissingArgument)
  {
    Option option;
    option.tag = "";
    option.longtag = optionName;
    option.name = optionName;
    option.required = false;
    option.description = "";
    option.userDefined = true;
    option.complete = false;

    Field field;
    field.name = name;
    field.externaldata = DATA_NONE;
    field.type = STRING;
    field.value = value;
    field.userDefined = true;
    field.required = false;
    field.rangeMin = "";
    field.rangeMax = "";
    option.fields.push_back(field);
    m_OptionVector.push_back(option);
  }

  return false;
}


#if (METAIO_USE_NAMESPACE)
};
#endif
