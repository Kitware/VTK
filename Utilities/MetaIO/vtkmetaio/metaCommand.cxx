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
#pragma warning(disable:4702)
#endif

#include "metaUtils.h"
#include "metaCommand.h"

#include <cstdio>
#include <cstring>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

#ifdef METAIO_USE_LIBXML2
#include <libxml/xmlwriter.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/tree.h>
#endif


MetaCommand::
MetaCommand()
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
std::string MetaCommand::
ExtractDateFromCVS(std::string date)
{
  std::string newdate;
  for(int i=7;i<(int)date.size()-1;i++)
    {
    newdate += date[i];
    }
  return newdate;
}

void MetaCommand::DisableDeprecatedWarnings()
{
  m_DisableDeprecatedWarnings = true;
}

void MetaCommand::
SetDateFromCVS(std::string cvsDate)
{
  this->SetDate( this->ExtractDateFromCVS( cvsDate ).c_str() );
}

/** Extract the version from a CVS Revision keyword/value pair.  */
std::string MetaCommand::
ExtractVersionFromCVS(std::string version)
{
  std::string newversion;
  for(int i=11;i<(int)version.size()-1;i++)
    {
    newversion += version[i];
    }
  return newversion;
}

void MetaCommand::
SetVersionFromCVS(std::string cvsVersion)
{
  this->SetVersion( this->ExtractVersionFromCVS( cvsVersion ).c_str() );
}


/** */
bool MetaCommand::
SetOption(Option option)
{
  // need to add some tests here to check if the option is not defined yet
  m_OptionVector.push_back(option);
  return true;
}

bool MetaCommand::
SetOption(std::string name,
                            std::string shortTag,
                            bool required,
                            std::string description,
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
  if(!m_DisableDeprecatedWarnings && shortTag.size()>1)
    {
    std::cout << "Warning: as of August 23, 2007 MetaCommand::SetOption()"
              << " is expecting a shortTag of exactly one character."
              << " You should use the SetOptionLongTag(optionName,longTagName)"
              << " if you want to use a longer tag. The longtag will be"
              << " referred to as --LongTag and the short tag as -ShortTag."
              << " Replace -" << shortTag.c_str()
              << " by --" << shortTag.c_str()
              << std::endl;
    }

  Option option;
  option.name = name;
  option.tag = shortTag;
  option.longtag = "";
  option.fields = fields;
  option.required = required;
  option.description = description;
  option.userDefined = false;
  option.complete = false;

  m_OptionVector.push_back(option);
  return true;
}


bool MetaCommand::
SetOption(std::string name,
          std::string shortTag,
          bool required,
          std::string description,
          TypeEnumType type,
          std::string defVal,
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

  if(!m_DisableDeprecatedWarnings && shortTag.size()>1)
    {
    std::cout << "Warning: as of August 23, 2007 MetaCommand::SetOption() "
              << " is expecting a shortTag of exactly one character."
              << " You should use the SetOptionLongTag(optionName,longTagName)"
              << " if you want to use a longer tag. The longtag will be "
              << " referred to as --LongTag and the short tag as -ShortTag "
              << " Replace -" << shortTag.c_str()
              << " by --" << shortTag.c_str()
              << std::endl;
    }

  Option option;
  option.tag = shortTag;
  option.longtag = "";
  option.name = name;
  option.required = required;
  option.description = description;
  option.userDefined = false;
  option.complete = false;

  // Create a field without description as a flag
  Field field;
  if(type == LIST)
    {
    field.name = "NumberOfValues";
    }
  else
    {
    field.name = name;
    }
  field.externaldata = externalData;
  field.type = type;
  field.value = defVal;
  field.userDefined = false;
  field.required = true;
  field.rangeMin = "";
  field.rangeMax = "";
  option.fields.push_back(field);

  m_OptionVector.push_back(option);
  return true;
}


/** Add a field */
bool MetaCommand::
AddField(std::string name,
         std::string description,
         TypeEnumType type,
         DataEnumType externalData,
         std::string rangeMin,
         std::string rangeMax)
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
  field.rangeMin = rangeMin;
  field.rangeMax = rangeMax;
  option.fields.push_back(field);

  option.required = true;
  option.name = name;
  option.description = description;
  option.userDefined = false;
  option.complete = false;

  m_OptionVector.push_back(option);
  return true;
}

/** For backward compatibility */
bool MetaCommand::
AddField(std::string name,
         std::string description,
         TypeEnumType type,
         bool externalData)
{
  if(externalData)
    {
    return this->AddField(name,description,type,DATA_IN);
    }
  else
    {
    return this->AddField(name,description,type,DATA_NONE);
    }
}

/** Collect all the information until the next tag
  * \warning this function works only if the field is of type String */
void MetaCommand::
SetOptionComplete(std::string optionName,
                  bool complete)
{
  OptionVector::iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      (*it).complete = complete;
      return;
      }
    ++it;
    }
 }

/** Add a field to a given an option */
bool MetaCommand::
AddOptionField(std::string optionName,
               std::string name,
               TypeEnumType type,
               bool required,
               std::string defVal,
               std::string description,
               DataEnumType externalData)
{
  OptionVector::iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
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
      if((*it).fields[0].type == FLAG)
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
bool MetaCommand::
SetOptionRange(std::string optionName,
               std::string name,
               std::string rangeMin,
               std::string rangeMax)
{
  OptionVector::iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      std::vector<Field> & fields = (*it).fields;
      std::vector<Field>::iterator itField = fields.begin();
      while(itField != fields.end())
        {
        if((*itField).name == name)
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
bool MetaCommand::
SetOptionEnumerations(std::string optionName,
                      std::string name,
                      std::string optionEnums)

{
  OptionVector::iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      std::vector<Field> & fields = (*it).fields;
      std::vector<Field>::iterator itField = fields.begin();
      while(itField != fields.end())
        {
        if((*itField).name == name)
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
bool MetaCommand::
GetValueAsBool(std::string optionName,
               std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = optionName;
    }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      std::vector<Field>::const_iterator itField = (*it).fields.begin();
      while(itField != (*it).fields.end())
        {
        if((*itField).name == fieldname)
          {
          if((*itField).value == "true"
            || (*itField).value == "1"
            || (*itField).value == "True"
            || (*itField).value == "TRUE"
            )
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
bool MetaCommand::
GetValueAsBool(Option option,
               std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = option.name;
    }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while(itField != option.fields.end())
    {
    if((*itField).name == fieldname)
      {
      if((*itField).value == "true"
         || (*itField).value == "1"
         || (*itField).value == "True"
         || (*itField).value == "TRUE"
        )
        {
        return true;
        }
      return false;
      }
    ++itField;
    }
  return 0;
}

/** Return the value of the option as a float */
float MetaCommand::
GetValueAsFloat(std::string optionName,
                std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = optionName;
    }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      std::vector<Field>::const_iterator itField = (*it).fields.begin();
      while(itField != (*it).fields.end())
        {
        if((*itField).name == fieldname)
          {
          return (float)atof((*itField).value.c_str());
          }
        ++itField;
        }
      }
    ++it;
    }
  return 0;
}

/** Return the value of the option as a float */
float MetaCommand::
GetValueAsFloat(Option option,
                std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = option.name;
    }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while(itField != option.fields.end())
    {
    if((*itField).name == fieldname)
      {
      return (float)atof((*itField).value.c_str());
      }
    ++itField;
    }
  return 0;
}

/** Return the value of the option as a int */
int MetaCommand::
GetValueAsInt(std::string optionName,
              std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = optionName;
    }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      std::vector<Field>::const_iterator itField = (*it).fields.begin();
      while(itField != (*it).fields.end())
        {
        if((*itField).name == fieldname)
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
int MetaCommand::
GetValueAsInt(Option option,
              std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = option.name;
    }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while(itField != option.fields.end())
    {
    if((*itField).name == fieldname)
      {
      return atoi((*itField).value.c_str());
      }
    ++itField;
    }
  return 0;
}

/** Return the value of the option as a string */
std::string MetaCommand::
GetValueAsString(std::string optionName,
                 std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = optionName;
    }

  OptionVector::const_iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      std::vector<Field>::const_iterator itField = (*it).fields.begin();
      while(itField != (*it).fields.end())
        {
        if((*itField).name == fieldname)
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
std::string MetaCommand::
GetValueAsString(Option option,
                 std::string fieldName)
{
  std::string fieldname = fieldName;
  if(fieldName == "")
    {
    fieldname = option.name;
    }

  std::vector<Field>::const_iterator itField = option.fields.begin();
  while(itField != option.fields.end())
    {
    if((*itField).name == fieldname)
      {
      return (*itField).value;
      }
    ++itField;
    }
  return "";
}

/** Return the value of the option as a list of strings */
std::list<std::string> MetaCommand::
GetValueAsList( Option option )
{
  std::list<std::string> results;
  results.clear();
  std::vector<Field>::const_iterator itField = option.fields.begin();
  ++itField;
  while(itField != option.fields.end())
    {
    results.push_back((*itField).value);
    ++itField;
    }
  return results;
}

std::list< std::string > MetaCommand::
GetValueAsList( std::string optionName )
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      return this->GetValueAsList( *it );
      }
    ++it;
    }
  std::list< std::string > empty;
  empty.clear();
  return empty;
}

bool MetaCommand::
GetOptionWasSet(Option option)
{
  if(option.userDefined)
    {
    return true;
    }
  return false;
}

bool MetaCommand::
GetOptionWasSet( std::string optionName)
{
  OptionVector::const_iterator it = m_ParsedOptionVector.begin();
  while(it != m_ParsedOptionVector.end())
    {
    if((*it).name == optionName)
      {
      return true;
      }
    ++it;
    }
  return false;
}

/** List the current options */
void MetaCommand::
ListOptions()
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  int i=0;
  while(it != m_OptionVector.end())
    {
    std::cout << "Option #" << i << std::endl;
    std::cout << "   Name: " <<  (*it).name.c_str()
                        << std::endl;
    if((*it).tag.size() > 0)
      {
      std::cout << "   Tag: " << (*it).tag.c_str()
                          << std::endl;
      }
    if((*it).longtag.size() > 0)
      {
      std::cout << "   LongTag: " << (*it).longtag.c_str()
                          << std::endl;
      }
    std::cout << "   Description: " << (*it).description.c_str()
                        << std::endl;
    if((*it).required)
      {
      std::cout << "   Required: true" << std::endl;
      }
    else
      {
      std::cout << "   Required: false" << std::endl;
      }
    std::cout << "   Number of expected values: "
                        << (*it).fields.size()
                        << std::endl;

    std::vector<Field>::const_iterator itField = (*it).fields.begin();
    while(itField != (*it).fields.end())
      {
      std::cout << "      Field Name: " <<  (*itField).name.c_str()
                          << std::endl;
      std::cout << "      Description: "
                          << (*itField).description.c_str()
                          << std::endl;
      std::cout << "      Type: "
                          << this->TypeToString((*itField).type).c_str()
                          << std::endl;
      std::cout << "      Value: " << (*itField).value.c_str()
                          << std::endl;
      if( (*itField).type == ENUM )
        {
        std::cout << "      Enum list: "
                            << (*itField).rangeMin.c_str()
                            << std::endl;
        }
      else
        {
        std::cout << "      RangeMin: "
                            << (*itField).rangeMin.c_str()
                            << std::endl;
        std::cout << "      RangeMax: "
                            << (*itField).rangeMax.c_str()
                            << std::endl;
        }

      if((*itField).externaldata)
        {
        std::cout << "      External Data: true"
                            << std::endl;
        }
      else
        {
        std::cout << "      External Data: false"
                            << std::endl;
        }

      if((*itField).required)
        {
        std::cout << "      Required: true" << std::endl;
        }
      else
        {
        std::cout << "      Required: false" << std::endl;
        }

      if((*itField).userDefined)
        {
        std::cout << "      User Defined: true" << std::endl;
        }
      else
        {
        std::cout << "      User Defined: false" << std::endl;
        }

      ++itField;
      }
    std::cout << std::endl;
    i++;
    ++it;
    }
  if(m_HelpCallBack != nullptr)
    {
    m_HelpCallBack();
    }
}

/** List the current options in xml format */
void MetaCommand::ListOptionsXML()
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  int i=0;
  while(it != m_OptionVector.end())
    {
    std::cout << "<option>" << std::endl;
    std::cout << "<number>" << i << "</number>"
                        << std::endl;
    std::cout << "<name>" << (*it).name.c_str() << "</name>"
                        << std::endl;
    std::cout << "<tag>" << (*it).tag.c_str() << "</tag>"
                        << std::endl;
    std::cout << "<longtag>" << (*it).longtag.c_str() << "</longtag>"
                        << std::endl;
    std::cout << "<description>" << (*it).description.c_str()
                        << "</description>" << std::endl;
    std::cout << "<required>";
    if((*it).required)
      {
      std::cout << "1</required>" << std::endl;
      }
    else
      {
      std::cout << "0</required>" << std::endl;
      }

    std::cout << "<nvalues>" << (*it).fields.size() << "</nvalues>"
                        << std::endl;

    std::vector<Field>::const_iterator itField = (*it).fields.begin();
    while(itField != (*it).fields.end())
      {
      std::cout << "<field>" << std::endl;
      std::cout << "<name>" << (*itField).name.c_str() << "</name>"
                          << std::endl;
      std::cout << "<description>" << (*itField).description.c_str()
                          << "</description>" << std::endl;
      std::cout << "<type>"
                          << this->TypeToString((*itField).type).c_str()
                          << "</type>" << std::endl;
      std::cout << "<value>" << (*itField).value.c_str()
                          << "</value>" << std::endl;
      std::cout << "<external>";
      if((*itField).externaldata == DATA_IN)
        {
        std::cout << "1</external>" << std::endl;
        }
      else if((*itField).externaldata == DATA_OUT)
        {
        std::cout << "2</external>" << std::endl;
        }
      else
        {
        std::cout << "0</external>" << std::endl;
        }
      std::cout << "<required>";
      if((*itField).required)
        {
        std::cout << "1</required>" << std::endl;
        }
      else
        {
        std::cout << "0</required>" << std::endl;
        }


      std::cout << "</field>" << std::endl;
      ++itField;
      }
    std::cout << "</option>" << std::endl;
    i++;
    ++it;
    }
}

/** Used by ListOptionsSlicerXML */
void MetaCommand::WriteXMLOptionToCout(std::string optionName,
                                       unsigned int & index)
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if(!strcmp((*it).name.c_str(),optionName.c_str()))
      {
      break;
      }
    ++it;
    }

  std::vector<Field>::const_iterator itField = (*it).fields.begin();

  std::string optionType = "";

  if((*itField).type == MetaCommand::STRING
     && ( (*itField).externaldata == MetaCommand::DATA_IN
     || (*itField).externaldata == MetaCommand::DATA_OUT))
    {
    optionType = "image";
    }
  else if((*itField).type == MetaCommand::FLAG)
    {
    optionType = "boolean";
    }
  else if((*itField).type == MetaCommand::INT)
    {
    optionType = "integer";
    }
  else if((*itField).type == MetaCommand::ENUM)
    {
    optionType = "string-enumeration";
    }
  else
    {
    optionType = this->TypeToString((*itField).type).c_str();
    }

  std::cout << "<" << optionType.c_str()
                      << ">" << std::endl;


  std::cout << "<name>" << (*it).name.c_str() << "</name>"
                      << std::endl;
  // Label is the description for now
  std::string label = (*it).label;
  if(label.size()==0)
    {
    label = (*it).name;
    }

  std::cout << "<label>" << label.c_str() << "</label>"
                      << std::endl;
  std::cout << "<description>" << (*it).description.c_str()
                      << "</description>" << std::endl;
  if((*it).tag.size()>0) // use the single by default flag if any
    {
    std::cout << "<flag>" << (*it).tag.c_str() << "</flag>"
                        << std::endl;
    }
  else if((*it).longtag.size()>0)
    {
    std::cout << "<longflag>" << (*it).longtag.c_str() << "</longflag>"
                        << std::endl;
    }
  else
    {
    std::cout << "<index>" << index << "</index>" << std::endl;
    index++;
    }

  if((*itField).value.size()>0)
    {
    std::cout << "<default>" << (*itField).value.c_str() << "</default>"
                        << std::endl;
    }

  if((*itField).externaldata == MetaCommand::DATA_IN)
    {
    std::cout << "<channel>input</channel>" << std::endl;
    }
  else if((*itField).externaldata == MetaCommand::DATA_OUT)
    {
    std::cout << "<channel>output</channel>" << std::endl;
    }

  if((*itField).type == MetaCommand::ENUM)
    {
    std::vector< std::string > enumVector;
    MET_StringToVector< std::string>( (*itField).rangeMin, enumVector );
    std::vector< std::string >::iterator itenum;
    itenum = enumVector.begin();
    while(itenum != enumVector.end() )
      {
      std::cout << "<element>" << (*itenum).c_str() << "</element>"
                          << std::endl;
      ++itenum;
      }
    }

  // Write out the closing tag
  std::cout << "</" << optionType.c_str()
                      << ">" << std::endl;
}

/** List the current options in Slicer's xml format (www.slicer.org) */
void MetaCommand::ListOptionsSlicerXML()
{
  std::cout << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" <<  std::endl;
  std::cout << "<executable>" <<  std::endl;
  std::cout << "  <category>" << m_Category.c_str() << "</category>" <<  std::endl;
  std::cout << "  <title>" << m_Name.c_str() << "</title>" <<  std::endl;
  std::cout << "  <description>" <<  std::endl;
  std::cout << "  " << m_Description.c_str() <<  std::endl;
  std::cout << "  </description>" <<  std::endl;
  std::cout << "  <version>" << m_Version.c_str() << "</version>" <<  std::endl;
  std::cout << "  <contributor>" << m_Author.c_str() << "</contributor>" <<  std::endl;
  std::cout << "  <documentation-url></documentation-url>" <<  std::endl;
  std::cout << "  <license></license>" <<  std::endl;
  std::cout << "  <acknowledgements>" <<  std::endl;
  std::cout << "  " << m_Acknowledgments.c_str() <<  std::endl;
  std::cout << "  </acknowledgements>" <<  std::endl;

  // Organize by group first
  // Keep a list of options
  unsigned int index=0;
  std::vector<std::string>   GroupedOptionVector;
  ParameterGroupVector::const_iterator itGroup = m_ParameterGroup.begin();
  while(itGroup != m_ParameterGroup.end())
    {
    if( (*itGroup).advanced == true )
      {
      std::cout << " <parameters advanced=\"true\">" <<  std::endl;
      }
    else
      {
      std::cout << " <parameters>" <<  std::endl;
      }
    std::cout << "  <label>" << (*itGroup).name.c_str()
                        <<  "</label>" <<  std::endl;

    if((*itGroup).description.size() == 0)
      {
      std::cout << "  <description>" << (*itGroup).name.c_str()
                          << "</description>" <<  std::endl;
      }
    else
      {
      std::cout << "  <description>" << (*itGroup).description.c_str()
                          << "</description>" <<  std::endl;
      }

    std::vector<std::string>::const_iterator itOption = (*itGroup).options.begin();
    while(itOption != (*itGroup).options.end())
      {
      this->WriteXMLOptionToCout(*itOption,index);
      GroupedOptionVector.push_back(*itOption);
      ++itOption;
      }
    std::cout << " </parameters>" <<  std::endl;
    ++itGroup;
    }

  // Then take the remaining options
  if(m_OptionVector.size()>GroupedOptionVector.size())
    {
    std::cout << " <parameters>" <<  std::endl;
    std::cout << "  <label>IO</label>" <<  std::endl;
    std::cout << "  <description>Input/output parameters</description>" <<  std::endl;

    OptionVector::const_iterator it = m_OptionVector.begin();
    while(it != m_OptionVector.end())
      {
      bool optionIsGrouped = false;
      std::vector<std::string>::const_iterator itGroupedOption = GroupedOptionVector.begin();
      while(itGroupedOption != GroupedOptionVector.end())
        {
        if(!strcmp((*itGroupedOption).c_str(),(*it).name.c_str()))
          {
          optionIsGrouped = true;
          break;
          }
        ++itGroupedOption;
        }

      if(!optionIsGrouped)
        {
        this->WriteXMLOptionToCout((*it).name.c_str(),index);
        }
      ++it;
      } // end loop option

    std::cout << " </parameters>" <<  std::endl;
    } // end m_OptionVector.size()>GroupedOptionVector.size()

std::cout << "</executable>" <<  std::endl;
}


/** Internal small XML parser */
std::string MetaCommand::
GetXML(const char* buffer, const char* desc, unsigned long pos)
{
  std::string begin = "<";
  begin += desc;
  begin += ">";
  std::string end = "</";
  end += desc;
  end += ">";

  std::string buf = buffer;

  long int posb = static_cast<long int>(buf.find(begin,pos));
  if(posb == -1)
    {
    return "";
    }
  long int pose = static_cast<long int>(buf.find(end,posb));
  if(pose == -1)
    {
    return "";
    }

  return buf.substr(posb+begin.size(),pose-posb-begin.size());
}

/** Given an XML buffer fill in the command line arguments */
bool MetaCommand::
ParseXML(const char* buffer)
{
  m_OptionVector.clear();
  std::string buf = this->GetXML(buffer,"option",0);
  long pos = 0;
  while(buf.size() > 0)
    {
    Option option;
    option.userDefined = false;
    option.complete = false;
    option.name = this->GetXML(buf.c_str(),"name",0);
    option.tag = this->GetXML(buf.c_str(),"tag",0);
    option.longtag = this->GetXML(buf.c_str(),"longtag",0);
    option.description = this->GetXML(buf.c_str(),"description",0);
    if(atoi(this->GetXML(buf.c_str(),"required",0).c_str()) == 0)
      {
      option.required = false;
      }
    else
      {
      option.required = true;
      }
    unsigned int n = atoi(this->GetXML(buf.c_str(),"nvalues",0).c_str());

    // Now check the fields
    long posF = static_cast<long>(buf.find("<field>"));
    for(unsigned int i=0;i<n;i++)
      {
      std::string f = this->GetXML(buf.c_str(),"field",posF);
      Field field;
      field.userDefined = false;
      field.name = this->GetXML(f.c_str(),"name",0);
      field.description = this->GetXML(f.c_str(),"description",0);
      field.value = this->GetXML(f.c_str(),"value",0);
      field.type = this->StringToType(this->GetXML(f.c_str(),"type",0).c_str());
      if(atoi(this->GetXML(f.c_str(),"external",0).c_str()) == 0)
        {
        field.externaldata = DATA_NONE;
        }
      else
        {
        if(atoi(this->GetXML(f.c_str(),"external",0).c_str()) == 1)
          {
          field.externaldata = DATA_IN;
          }
        else
          {
          field.externaldata = DATA_OUT;
          }
        }
      if(atoi(this->GetXML(f.c_str(),"required",0).c_str()) == 0)
        {
        field.required = false;
        }
      else
        {
        field.required = true;
        }

      option.fields.push_back(field);
      posF += static_cast<long>(f.size()+8);
      }

    m_OptionVector.push_back(option);

    pos += static_cast<long>(buf.size()+17);
    buf = this->GetXML(buffer,"option",pos);
    }

  return true;
}


/** List the current options */
void MetaCommand::
ListOptionsSimplified(bool extended)
{
  if(extended)
    {
    std::cout << " System tags: " << std::endl
            << "   [ -v ] or [ -h ]"
            << std::endl
            << "      = List options in short format"
            << std::endl
            << "   [ -V ] or [ -H ]"
            << std::endl
            << "      = List options in long format"
            << std::endl
            << "   [ -vxml ] or [ -hxml ] or [ -exportXML ]"
            << std::endl
            << "      = List options in xml format for BatchMake"
            << std::endl
            << "   [ --xml ]"
            << std::endl
            << "      = List options in xml format for Slicer"
            << std::endl
            << "   [ -vgad ] or [ -hgad ] or [ -exportGAD ]"
            << std::endl
            << "      = List options in Grid Application Description format"
            << std::endl
            << "   [ -version ]"
            << std::endl
            << "      = return the version number"
            << std::endl
            << "   [ -date ]"
            << std::endl
            << "      = return the cvs checkout date"
#ifdef METAIO_USE_LIBXML2
            << std::endl
            << "   [ --loadArguments filename ]"
            << "      = load the arguments from an XML file"
#endif
            << std::endl;
     }

  int count = 0;
  int ntags = 0;
  int nfields = 0;
  OptionVector::const_iterator it;
  it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).tag.size() > 0 || (*it).longtag.size() > 0)
      {
      ntags++;
      }
    else
      {
      nfields++;
      }
    ++it;
    }
  while(count<2)
    {
    if(count == 0)
      {
      if(ntags > 0)
        {
        std::cout << " Command tags: "
                            << std::endl;
        }
      else
        {
        count++;
        }
      }
    if(count == 1)
      {
      if(nfields > 0)
        {
        std::cout << " Command fields: "
                            << std::endl;
        }
      else
        {
        count++;
        }
      }
    count++;
    it = m_OptionVector.begin();
    while(it != m_OptionVector.end())
      {
      if( (count == 1 && ( (*it).tag.size() > 0 || (*it).longtag.size() > 0 ))
         || (count == 2 && ( (*it).tag.size() == 0 && (*it).longtag.size() == 0 )) )
        {
        if(!(*it).required)
          {
          std::cout << "   [ ";
          }
        else
          {
          std::cout << "   ";
          }
        if((*it).tag.size() > 0)
          {
          std::cout << "-" << (*it).tag.c_str() << " ";
          }
        if((*it).longtag.size() > 0)
          {
          std::cout << "--" << (*it).longtag.c_str() << " ";
          }
        std::vector<Field>::const_iterator itField =
                                                  (*it).fields.begin();
        while(itField != (*it).fields.end())
          {
          // only display the type if it's not a FLAG
          if((*itField).type != FLAG)
            {
            if((*itField).required)
              {
              std::cout << "< ";
              }
            else
              {
              std::cout << "[ ";
              }

            std::cout << (*itField).name.c_str();

            if((*itField).required)
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

        if(!(*it).required)
          {
          std::cout << "]";
          }
        std::cout << std::endl;

        if((*it).description.size()>0)
          {
          std::cout << "      = " << (*it).description.c_str();
          std::cout << std::endl;
          itField = (*it).fields.begin();
          while(itField != (*it).fields.end())
            {
            if((*itField).description.size() > 0
               || (*itField).value.size() > 0)
              {
              std::cout << "        With: "
                                  << (*itField).name.c_str();
              if((*itField).description.size() > 0)
                {
                std::cout << " = " << (*itField).description.c_str();
                }
              if((*itField).value.size() > 0)
                {
                std::cout << " (Default = "
                                    << (*itField).value.c_str() << ")";
                }
              std::cout << std::endl;
              }
            ++itField;
            }
          }
        }
      ++it;
      }
    }

  if(m_HelpCallBack != nullptr)
    {
    m_HelpCallBack();
    }
}

/** Get the option by "-"+tag
 *  or by "--"+longtag */
bool MetaCommand::
OptionExistsByMinusTag(std::string minusTag)
{
  OptionVector::const_iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    std::string tagToSearch = "-";
    tagToSearch += (*it).tag;
    std::string longtagToSearch = "--";
    longtagToSearch += (*it).longtag;
    std::string longtagToSearchBackwardCompatible = "-";
    longtagToSearchBackwardCompatible += (*it).longtag;
    // WARNING: This is for backward compatibility but a warning
    // is going to be thrown if used so that people can adjust
    if(tagToSearch == minusTag
       || longtagToSearch == minusTag
       || longtagToSearchBackwardCompatible == minusTag
       )
      {
      return true;
      }
    ++it;
    }
  return false;
}

/** Get the option by "-"+tag
 *  or by "--"+longtag */
MetaCommand::Option * MetaCommand::
GetOptionByMinusTag(std::string minusTag)
{
  OptionVector::iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    std::string tagToSearch = "-";
    tagToSearch += (*it).tag;
    std::string longtagToSearch = "--";
    longtagToSearch += (*it).longtag;
    std::string longtagToSearchBackwardCompatible = "-";
    longtagToSearchBackwardCompatible += (*it).longtag;

    // WARNING: This is for backward compatibility but a warning
    // is going to be thrown if used so that people can adjust
    if(tagToSearch == minusTag
       || longtagToSearch == minusTag
       || longtagToSearchBackwardCompatible == minusTag
      )
      {
      return &(*it);
      }
    ++it;
    }
  return nullptr;
}

/** Get the option by tag */
MetaCommand::Option * MetaCommand::
GetOptionByTag(std::string tag)
{
  OptionVector::iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).tag == tag || (*it).longtag == tag)
      {
      return &(*it);
      }
    ++it;
    }
  return nullptr;
}

/** Return the option id. i.e the position in the vector */
long MetaCommand::
GetOptionId(Option* option)
{
  OptionVector::iterator it = m_OptionVector.begin();
  unsigned long i = 0;
  while(it != m_OptionVector.end())
    {
    if(&(*it) == option)
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
bool MetaCommand::
ExportGAD(bool dynamic)
{
  std::cout << "Exporting GAD file...";

  OptionVector options = m_OptionVector;
  if(dynamic)
    {
    options = m_ParsedOptionVector;
    }

  if(m_Name=="")
    {
    std::cout << "Set the name of the application using SetName()"
                        << std::endl;
    return false;
    }

  std::string filename = m_Name;
  filename += ".gad.xml";

  std::ofstream file;
#ifdef __sgi
  file.open(filename.c_str(), std::ios::out);
#else
  file.open(filename.c_str(), std::ios::binary
                              | std::ios::out);
#endif
  if(!file.rdbuf()->is_open())
    {
    std::cout << "Cannot open file for writing: "
                        << filename.c_str() <<  std::endl;
    return false;
    }

  file << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
  file << "<gridApplication" << std::endl;
  file << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
       << std::endl;
  file << "xsi:noNamespaceSchemaLocation=\"grid-application-description.xsd\""
       << std::endl;
  file << "name=\"" << m_Name.c_str() << "\"" << std::endl;
  file << "description=\"" << m_Description.c_str() << "\">"
       << std::endl;
  file << "<applicationComponent name=\"Client\" remoteExecution=\"true\">"
       << std::endl;
  file << "<componentActionList>" << std::endl;
  file << std::endl;

  unsigned int order = 1;
  // Write out the input data to be transferred
  OptionVector::const_iterator it = options.begin();
  while(it != options.end())
    {
    std::vector<Field>::const_iterator itFields = (*it).fields.begin();
    while(itFields != (*it).fields.end())
      {
      if((*itFields).externaldata == DATA_IN)
        {
        file << " <componentAction type=\"DataRelocation\" order=\"" << order
             << "\">" << std::endl;
        file << "  <parameter name=\"Name\" value=\""
             << (*itFields).name.c_str()
             <<"\"/>" << std::endl;
        file << "  <parameter name=\"Host\" value=\"hostname\"/>"
             << std::endl;
        file << "  <parameter name=\"Description\" value=\""
             << (*itFields).description.c_str() << "\"/>"
             << std::endl;
        file << "  <parameter name=\"Direction\" value=\"In\"/>"
             << std::endl;
        file << "  <parameter name=\"Protocol\" value=\"gsiftp\"/>"
             << std::endl;
        file << "  <parameter name=\"SourceDataPath\" value=\""
             << (*itFields).value.c_str() << "\"/>" << std::endl;

        std::string datapath = (*itFields).value;
        long int slash = static_cast<long int>(datapath.find_last_of('/'));
        if(slash>0)
          {
          datapath = datapath.substr(slash+1,datapath.size()-slash-1);
          }
        slash = static_cast<long int>(datapath.find_last_of('\\'));
        if(slash>0)
          {
          datapath = datapath.substr(slash+1,datapath.size()-slash-1);
          }
        file << "  <parameter name=\"DestDataPath\" value=\""
             << datapath.c_str() << "\"/>" << std::endl;
        file << " </componentAction>" << std::endl;
        file << std::endl;
        order++;
        }
      ++itFields;
      }
    ++it;
    }

  file << " <componentAction type=\"JobSubmission\" order=\"" << order << "\">"
       << std::endl;
  file << "  <parameter name=\"Executable\" value=\""
       << m_ExecutableName.c_str() << "\"/>" << std::endl;
  file << "  <parameter name=\"Arguments\"  value=\"";
  // Write out the command line arguments
  it = options.begin();
  while(it != options.end())
    {
    if(it != options.begin())
      {
      file << " ";
      }
    file << "{" << (*it).name.c_str() << "}";
    ++it;
    }
  file << "\"/>" << std::endl;
  // Write out the arguments that are not data
  it = options.begin();
  while(it != options.end())
    {
    // Find if this is a non data field
    bool isData = false;
    std::vector<Field>::const_iterator itFields = (*it).fields.begin();
    while(itFields != (*it).fields.end())
      {
      if((*itFields).externaldata != DATA_NONE)
        {
        isData = true;
        break;
        }
      ++itFields;
      }

    if(isData)
      {
      ++it;
      continue;
      }

    file << "   <group name=\"" << (*it).name.c_str();
    file << "\" syntax=\"";

    if((*it).tag.size()>0)
      {
      file << "-" << (*it).tag.c_str() << " ";
      }

    itFields = (*it).fields.begin();
    while(itFields != (*it).fields.end())
      {
      if(itFields != (*it).fields.begin())
        {
        file << " ";
        }
      file << "{" << (*it).name.c_str() << (*itFields).name.c_str() << "}";
      ++itFields;
      }
    file << "\"";

    if(!(*it).required)
      {
      file << " optional=\"true\"";

      // Add if the option was selected
      if((*it).userDefined)
        {
        file << " selected=\"true\"";
        }
      else
        {
        file << " selected=\"false\"";
        }
      }

    file << ">" << std::endl;

    // Now writes the value of the arguments
    itFields = (*it).fields.begin();
    while(itFields != (*it).fields.end())
      {
      file << "    <argument name=\"" << (*it).name.c_str()
           << (*itFields).name.c_str();
      file << "\" value=\"" << (*itFields).value.c_str();
      file << "\" type=\"" << this->TypeToString((*itFields).type).c_str();
      file << "\"";

      if((*itFields).rangeMin != "")
        {
        file << " rangeMin=\"" << (*itFields).rangeMin.c_str() << "\"";
        }

      if((*itFields).rangeMax != "")
        {
        file << " rangeMax=\"" << (*itFields).rangeMax.c_str() << "\"";
        }
      file << "/>" << std::endl;
      ++itFields;
      }
    file << "  </group>" << std::endl;
    ++it;
    }
  file << " </componentAction>" << std::endl;
  order++;
  file << std::endl;
  // Write out the input data to be transferred
  it = options.begin();
  while(it != options.end())
    {
    std::vector<Field>::const_iterator itFields = (*it).fields.begin();
    while(itFields != (*it).fields.end())
      {
      if((*itFields).externaldata == DATA_OUT)
        {
        file << " <componentAction type=\"DataRelocation\" order=\"" << order
             << "\">" << std::endl;
        file << "  <parameter name=\"Name\" Value=\""
             << (*itFields).name.c_str()
             <<"\"/>" << std::endl;
        file << "  <parameter name=\"Host\" Value=\"hostname\"/>"
             << std::endl;
        file << "  <parameter name=\"Description\" value=\""
             << (*itFields).description.c_str() << "\"/>"
             << std::endl;
        file << "  <parameter name=\"Direction\" value=\"Out\"/>"
             << std::endl;
        file << "  <parameter name=\"Protocol\" value=\"gsiftp\"/>"
             << std::endl;
        std::string datapath = (*itFields).value;
        long int slash = static_cast<long int>(datapath.find_last_of('/'));
        if(slash>0)
          {
          datapath = datapath.substr(slash+1,datapath.size()-slash-1);
          }
        slash = static_cast<long int>(datapath.find_last_of('\\'));
        if(slash>0)
          {
          datapath = datapath.substr(slash+1,datapath.size()-slash-1);
          }
        file << "  <parameter name=\"SourceDataPath\" value=\""
             << datapath.c_str() << "\"/>" << std::endl;
        file << "  <parameter name=\"DestDataPath\" value=\""
             << (*itFields).value.c_str() << "\"/>" << std::endl;
        file << " </componentAction>" << std::endl;
        file << std::endl;
        order++;
        }
      ++itFields;
      }
    ++it;
    }
  file << "    </componentActionList>" << std::endl;
  file << "  </applicationComponent>" << std::endl;
  file << "</gridApplication>" << std::endl;

  file.close();

  std::cout << "done" << std::endl;
  return true;
}


/** Parse the command line */
bool MetaCommand::Parse(int argc, char* argv[])
{
  m_GotXMLFlag = false;
  m_ExecutableName = argv[0];

  long int slash = static_cast<long int>(m_ExecutableName.find_last_of('/'));
  if(slash>0)
    {
    m_ExecutableName = m_ExecutableName.substr(slash+1,
                                               m_ExecutableName.size()-slash-1);
    }
  slash = static_cast<long int>(m_ExecutableName.find_last_of('\\'));
  if(slash>0)
    {
    m_ExecutableName = m_ExecutableName.substr(slash+1,
                                               m_ExecutableName.size()-slash-1);
    }

  // Fill in the results
  m_ParsedOptionVector.clear();
  bool inArgument = false;
  std::string tag = "";
  std::string args;

  unsigned long currentField = 0; // current field position
  long currentOption = 0; // id of the option to fill
  unsigned int valuesRemaining=0;
  unsigned int optionalValuesRemaining=0;
  bool isComplete = false; // check if the option should be parse until
                           // the next tag is found
  std::string completeString = "";

  bool exportGAD = false;
  for(unsigned int i=1;i<(unsigned int)argc;i++)
    {
    if(!strcmp(argv[i],"-V") || !strcmp(argv[i],"-H"))
      {
      std::cout << "Usage : " << argv[0] << std::endl;
      this->ListOptions();
      return true;
      }
    // List the options if using -v
    if(!strcmp(argv[i],"-v") || !strcmp(argv[i],"-h"))
      {
      std::cout << "Usage : " << argv[0] << std::endl;
      this->ListOptionsSimplified();
      return true;
      }
    // List the options if using -v
    if(!strcmp(argv[i],"--loadArguments"))
      {
      if((i+1)>=(unsigned int)argc)
        {
        std::cout << "--loadArguments expected a filename as argument"
                            << std::endl;
        return false;
        }
      this->LoadArgumentsFromXML(argv[i+1]);
      i++;
      continue;
      }
    if(!strcmp(argv[i],"-vxml")
       || !strcmp(argv[i],"-hxml")
       || !strcmp(argv[i],"-exportXML")
       || !strcmp(argv[i],"--vxml")
       || !strcmp(argv[i],"--hxml")
       || !strcmp(argv[i],"--exportXML"))
      {
      this->ListOptionsXML();
      continue;
      }
    if(!strcmp(argv[i],"--xml") )
      {
      this->ListOptionsSlicerXML();
      m_GotXMLFlag = true;
      return false;
      }
    if(!strcmp(argv[i],"-version"))
      {
      std::cout << "Version: " << m_Version.c_str()
                          << std::endl;
      continue;
      }
    if(!strcmp(argv[i],"-date"))
      {
      std::cout << "Date: " << m_Date.c_str()
                          << std::endl;
      continue;
      }
    if(!strcmp(argv[i],"-exportGAD")
       || !strcmp(argv[i],"-vgad")
       || !strcmp(argv[i],"-hgad"))
      {
      this->ExportGAD();
      exportGAD = true;
      continue;
      }

    // If this is a tag
    if(argv[i][0] == '-' && (atof(argv[i])==0) && (strlen(argv[i])>1))
      {
      // if we have a tag before the expected values we throw an exception
      if(valuesRemaining!=0)
        {
        if(!isComplete)
          {
          if(optionalValuesRemaining >0)
            {
            valuesRemaining = 0;
            m_OptionVector[currentOption].userDefined = true;
            m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
            }
          else
            {
            std::cout << "Found tag " << argv[i]
                              << " before end of value list!"
                              << std::endl;
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
      if(this->OptionExistsByMinusTag(tag))
        {
        inArgument = true;

        // We check the number of mandatory and optional values for
        // this tag
        std::vector<Field>::const_iterator fIt =
                                 this->GetOptionByMinusTag(tag)->fields.begin();
        while(fIt != this->GetOptionByMinusTag(tag)->fields.end())
          {
          if(!(*fIt).required)
            {
            optionalValuesRemaining++;
            }
          valuesRemaining++;
          ++fIt;
          }
        currentOption = this->GetOptionId(this->GetOptionByMinusTag(tag));

        if(currentOption < 0)
          {
          std::cout << "Error processing tag " << tag.c_str()
                              << ".  Tag exists but cannot find its Id."
                              << std::endl;
          }
        else
          {
          isComplete = m_OptionVector[currentOption].complete;

          if(m_OptionVector[currentOption].fields[0].type == FLAG)
            {
            // the tag exists by default
            m_OptionVector[currentOption].fields[0].value = "true";
            valuesRemaining = 0;
            optionalValuesRemaining = 0;
            inArgument = false;
            }
          else if(m_OptionVector[currentOption].fields[0].type == LIST)
            {
            inArgument = true;
            unsigned int valuesInList = (int)atoi(argv[++i]);
            m_OptionVector[currentOption].fields[0].value = argv[i];
            valuesRemaining += valuesInList-1;
            char optName[255];
            for(unsigned int j=0; j<valuesInList; j++)
              {
              snprintf(optName, sizeof(optName), "%03u", j);
              this->AddOptionField( m_OptionVector[currentOption].name,
                                    optName, STRING );
              }
            }
          args = "";
          }
        }
      else
        {
        if(m_Verbose)
          {
          std::cout << "The tag " << tag.c_str()
                              << " is not a valid argument : skipping this tag"
                              << std::endl;
          }
        if(m_FailOnUnrecognizedOption)
          {
          return false;
          }
        }
      if(inArgument)
        {
        i++;
        }
      }
    else if(!inArgument) // If this is a field
      {
      // Look for the field to add
      OptionVector::iterator it = m_OptionVector.begin();
      unsigned long pos = 0;
      bool found = false;
      while(it != m_OptionVector.end())
        {
        if((pos >= currentField) && ((*it).tag=="" && (*it).longtag==""))
          {
          currentOption = pos;
          valuesRemaining = static_cast<unsigned int>((*it).fields.size());
          found = true;
          break;
          }
        pos++;
        ++it;
        }

      if(!found && m_Verbose)
        {
        std::cout
                  << "Too many arguments specified in your command line! "
                  << "Skipping extra argument: " << argv[i]
                  << std::endl;
        }

      inArgument=true;
      currentField=currentOption+1;
      }

    // We collect the values
    if(isComplete && (int)i<argc)
      {
      if(completeString.size()==0)
        {
        completeString = argv[i];
        }
      else
        {
        completeString += " ";
        completeString += argv[i];
        }
      }
    else if(inArgument && i<(unsigned int)argc && (valuesRemaining>0))
      {
      // We check that the current value is not a tag.
      // This might be the case when we have optional fields
      if(this->OptionExistsByMinusTag(argv[i]) && optionalValuesRemaining>0)
        {
        valuesRemaining = 0;
        optionalValuesRemaining = 0;
        i--; // the outter loop will take care of incrementing it.
        }

      else if(currentOption >=0 && currentOption <(int)(m_OptionVector.size()))
        {
        unsigned long s = static_cast<unsigned long>(
          m_OptionVector[currentOption].fields.size());

        // We change the value only if this is not a tag
        if(this->OptionExistsByMinusTag(argv[i]))
          {
          std::cout << "Option "
                              << m_OptionVector[currentOption].name.c_str()
                              << " expect a value and got tag: " << argv[i]
                              << std::endl;
          this->ListOptionsSimplified(false);
          return false;
          }

        m_OptionVector[currentOption].fields[s-(valuesRemaining)].value = argv[i];

        m_OptionVector[currentOption].fields[s-(valuesRemaining)].userDefined =
                                                                           true;

       if(!m_OptionVector[currentOption].fields[s-(valuesRemaining)].required)
         {
         optionalValuesRemaining--;
         }

        valuesRemaining--;
        }
      else if(valuesRemaining>0)
        {
        valuesRemaining--;
        }
      }
    else if(valuesRemaining==optionalValuesRemaining
            && i==(unsigned int)argc && (optionalValuesRemaining>0))
    // if this is the last argument and all the remaining values are optionals
      {
      if(this->OptionExistsByMinusTag(argv[i-1]) )
        {
        valuesRemaining = 0;
        optionalValuesRemaining = 0;
        }
      }

    if(valuesRemaining == 0)
      {
      inArgument = false;
      m_OptionVector[currentOption].userDefined = true;
      m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
      }

    } // end loop command line arguments

  if(isComplete) // If we are still in the isComplete mode we add the option
    {
    m_OptionVector[currentOption].fields[0].value = completeString;
    m_OptionVector[currentOption].fields[0].userDefined = true;
    m_OptionVector[currentOption].userDefined = true;
    m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
    valuesRemaining = 0;
    }

  if(optionalValuesRemaining >0 && optionalValuesRemaining==valuesRemaining)
    {
    valuesRemaining = 0;
    m_OptionVector[currentOption].userDefined = true;
    m_ParsedOptionVector.push_back(m_OptionVector[currentOption]);
    }

  if(valuesRemaining>0)
    {
    std::cout << "Not enough parameters for "
                        << m_OptionVector[currentOption].name.c_str()
                        << std::endl;
    std::cout << "Usage: " << argv[0] << std::endl;
    this->ListOptionsSimplified(false);
    return false;
    }

  // Check if the options with required arguments are defined
  OptionVector::iterator it = m_OptionVector.begin();
  bool requiredAndNotDefined = false;
  while(it != m_OptionVector.end())
    {
    if((*it).required)
      {
      // First check if the option is actually defined
      if(!(*it).userDefined)
        {
        std::cout << "Option " << (*it).name.c_str()
                            << " is required but not defined"
                            << std::endl;
        requiredAndNotDefined = true;
        ++it;
        continue;
        }

      // Check if the values are defined
      std::vector<Field>::const_iterator itFields = (*it).fields.begin();
      bool defined = true;
      while(itFields != (*it).fields.end())
        {
        if((*itFields).value == "")
          {
          defined = false;
          }
        ++itFields;
        }

      if(!defined)
        {
        if((*it).tag.size()>0 || (*it).longtag.size()>0)
          {
          std::cout << "Field " << (*it).tag.c_str()
                              << " is required but not defined"
                              << std::endl;
          }
        else
          {
          std::cout << "Field " << (*it).name.c_str()
                              << " is required but not defined"
                              << std::endl;
          }
        requiredAndNotDefined = true;
        }
      }
    ++it;
    }

  if(requiredAndNotDefined)
    {
    //std::cout << "Command: " << argv[0] << std::endl;
    this->ListOptionsSimplified(false);
    return false;
    }

  // Check if the values are in range (if the range is defined)
  OptionVector::iterator itParsed = m_ParsedOptionVector.begin();
  bool valueInRange = true;
  while(itParsed != m_ParsedOptionVector.end())
    {
    std::vector<Field>::const_iterator itFields = (*itParsed).fields
                                                                    .begin();
    while(itFields != (*itParsed).fields.end())
      {
      // Check only if this is a number
      if(((*itFields).type == INT ||
        (*itFields).type == FLOAT ||
        (*itFields).type == CHAR)
        && ((*itFields).value != "")
        )
        {
        // Check the range min
        if(
          (((*itFields).rangeMin != "")
          && (atof((*itFields).rangeMin.c_str())
              > atof((*itFields).value.c_str())))
          ||
          (((*itFields).rangeMax != "")
          && (atof((*itFields).rangeMax.c_str())
              < atof((*itFields).value.c_str())))
          )
          {
          std::cout << (*itParsed).name.c_str()
                    << "." << (*itFields).name.c_str()
                    << " : Value (" << (*itFields).value.c_str() << ") "
                    << "is not in the range [" << (*itFields).rangeMin.c_str()
                    << "," << (*itFields).rangeMax.c_str()
                    << "]" << std::endl;
          valueInRange = false;
          }
        }
      ++itFields;
      }
    ++itParsed;
    }

  if(!valueInRange)
    {
    return false;
    }

  // If everything is ok
  if(exportGAD)
    {
    this->ExportGAD(true);
    return false; // prevent from running the application
    }

  return true;
}

/** Return the string representation of a type */
std::string MetaCommand::TypeToString(TypeEnumType type)
{
  switch(type)
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
MetaCommand::TypeEnumType MetaCommand::StringToType(const char* type)
{
  if(!strcmp(type,"int"))
    {
    return INT;
    }
  else if(!strcmp(type,"float"))
    {
    return FLOAT;
    }
  else if(!strcmp(type,"string"))
    {
    return STRING;
    }
  else if(!strcmp(type,"list"))
    {
    return LIST;
    }
  else if(!strcmp(type,"flag"))
    {
    return FLAG;
    }
  else if(!strcmp(type,"bool"))
    {
    return BOOL;
    }
  else if(!strcmp(type,"image"))
    {
    return IMAGE;
    }
  else if(!strcmp(type,"enum"))
    {
    return ENUM;
    }
  else if(!strcmp(type,"file"))
    {
    return FILE;
    }

  return INT; // by default

}


/** Set the long flag for the option */
bool MetaCommand::SetOptionLongTag(std::string optionName,
                                    std::string longTag)
{
  OptionVector::iterator itOption = m_OptionVector.begin();
  while(itOption != m_OptionVector.end())
    {
    if(!strcmp((*itOption).name.c_str(),optionName.c_str()))
      {
      (*itOption).longtag = longTag;
      return true;
      }
    ++itOption;
    }

  return false;
}

/** Set the label for the option */
bool MetaCommand::SetOptionLabel(std::string optionName,
                                 std::string label)
{
  OptionVector::iterator itOption = m_OptionVector.begin();
  while(itOption != m_OptionVector.end())
    {
    if(!strcmp((*itOption).name.c_str(),optionName.c_str()))
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
bool MetaCommand::SetParameterGroup(std::string optionName,
                                    std::string groupName,
                                    std::string groupDescription,
                                    bool advanced
                                    )
{
  // Check if the group exists
  ParameterGroup* group = nullptr;
  ParameterGroupVector::iterator it = m_ParameterGroup.begin();
  while(it != m_ParameterGroup.end())
    {
    if(!strcmp((*it).name.c_str(),groupName.c_str()))
      {
      group = &(*it);
      }
    ++it;
    }

  bool optionExists = false;
  unsigned int index = 0;
  OptionVector::const_iterator itOption = m_OptionVector.begin();
  while(itOption != m_OptionVector.end())
    {
    if(!strcmp((*itOption).name.c_str(),optionName.c_str()))
      {
      optionExists = true;
      break;
      }
    index++;
    ++itOption;
    }

  if(!optionExists)
    {
    std::cout << "The option " << optionName.c_str()
                        << " doesn't exist" << std::endl;
    return false;
    }

  if(!group)
    {
    ParameterGroup pgroup;
    pgroup.name = groupName;
    pgroup.description = groupDescription;
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
bool MetaCommand::LoadArgumentsFromXML(const char* filename,
                                       bool createMissingArguments)
{
#ifdef METAIO_USE_LIBXML2
  xmlDocPtr doc;
  xmlNodePtr cur;
  doc = xmlParseFile(filename);

  if (doc == nullptr )
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
  if (xmlStrcmp(cur->name, (const xmlChar *) "MetaCommand"))
    {
    std::cerr << "document of the wrong type. Root node should be MetaCommand" << std::endl;
    xmlFreeDoc(doc);
    return false;
    }
  xmlCleanupParser();

  // Simple parsing (two levels hierarchy)
  cur = cur->children;
  while(cur)
    {
    xmlNodePtr child = cur->children;
    if(child)
      {
      xmlNodePtr subargument = child->next;
      while(subargument)
        {
        xmlNodePtr subargumentChild = subargument->children;
        if(subargumentChild && subargumentChild->content)
          {
          this->SetOptionValue((const char*)cur->name,
                               (const char*)subargument->name,
                               (const char*)subargumentChild->content,
                               createMissingArguments
                               );
          }
        subargument = subargument->next;
        }

      if(child->content)
        {
        this->SetOptionValue((const char*)cur->name,
                             (const char*)cur->name,
                             (const char*)child->content,
                             createMissingArguments);
        }
      }
    cur = cur->next;
    }
  xmlFreeDoc(doc);
#else
  std::cout << "LoadArguments(" << filename << ") requires libxml2" << std::endl;
  if(createMissingArguments)
    {
    }

#endif
  return true;
}

/** Set the value of an option or a field
 *  This is used when importing command line arguments
 *  from XML */
bool MetaCommand::SetOptionValue(const char* optionName,
                                 const char* name,
                                 const char* value,
                                 bool createMissingArgument)
{
  OptionVector::iterator it = m_OptionVector.begin();
  while(it != m_OptionVector.end())
    {
    if((*it).name == optionName)
      {
      (*it).userDefined = true;
      std::vector<Field> & fields = (*it).fields;
      std::vector<Field>::iterator itField = fields.begin();
      while(itField != fields.end())
        {
        if((*itField).name == name)
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

  if(createMissingArgument)
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
