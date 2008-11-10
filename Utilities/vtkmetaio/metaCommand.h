/*=========================================================================

  Program:   MetaIO
  Module:    metaCommand.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METACOMMAND_H
#define ITKMetaIO_METACOMMAND_H


#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4251 )
#endif

#include <stdlib.h>
#include <string>
#include <vector>
#include <list>
#include <map>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT MetaCommand
{

public:

  typedef enum {DATA_NONE,DATA_IN,DATA_OUT} DataEnumType;
  typedef enum {INT,FLOAT,CHAR,STRING,LIST,FLAG,BOOL,IMAGE,ENUM,FILE} TypeEnumType;

  struct Field{
    METAIO_STL::string  name;
    METAIO_STL::string  description;
    METAIO_STL::string  value;
    TypeEnumType        type;
    DataEnumType        externaldata;
    METAIO_STL::string  rangeMin;
    METAIO_STL::string  rangeMax;
    bool                required;
    bool                userDefined;
    };

  struct Option{
    METAIO_STL::string        name;
    METAIO_STL::string        description;
    METAIO_STL::string        tag;
    METAIO_STL::string        longtag;
    METAIO_STL::string        label;
    METAIO_STL::vector<Field> fields;
    bool                      required;
    bool                      userDefined;
    bool                      complete;
  };

  struct ParameterGroup{
    METAIO_STL::string                     name;
    METAIO_STL::string                     description;
    METAIO_STL::vector<METAIO_STL::string> options;
    bool                                   advanced;
    };

  typedef METAIO_STL::vector<Option>             OptionVector;
  typedef METAIO_STL::vector<ParameterGroup>     ParameterGroupVector;
  
  MetaCommand();
  ~MetaCommand() {}

  bool SetOption(Option option);
  bool SetOption(METAIO_STL::string name,
                 METAIO_STL::string tag,
                 bool required,
                 METAIO_STL::string description,
                 METAIO_STL::vector<Field> fields);
  bool SetOption(METAIO_STL::string name,
                 METAIO_STL::string tag,
                 bool required,
                 METAIO_STL::string description,
                 TypeEnumType type = FLAG,
                 METAIO_STL::string defVal = "",
                 DataEnumType externalData = DATA_NONE);

  /** Fields are added in order */
  bool AddField(METAIO_STL::string name,
                METAIO_STL::string description,
                TypeEnumType type,
                DataEnumType externalData = DATA_NONE,
                METAIO_STL::string rangeMin = "",
                METAIO_STL::string rangeMax = ""
                );

  /** For backward compatibility */
  bool AddField(METAIO_STL::string name,
                METAIO_STL::string description,
                TypeEnumType type,
                bool externalData );
  
  /** Add a field to an option */
  bool AddOptionField(METAIO_STL::string optionName,
                      METAIO_STL::string name,
                      TypeEnumType type,
                      bool required=true,
                      METAIO_STL::string defVal = "",
                      METAIO_STL::string description = "",
                      DataEnumType externalData = DATA_NONE);
  
  /** Set the range of value as an option */
  bool SetOptionRange(METAIO_STL::string optionName,
                      METAIO_STL::string name,
                      METAIO_STL::string rangeMin,
                      METAIO_STL::string rangeMax);

  /** Set the list of values that can be used with an option */
  bool SetOptionEnumerations(METAIO_STL::string optionName,
                             METAIO_STL::string name,
                             METAIO_STL::string optionEnums);

  /** Set the long tag for the option */
  bool SetOptionLongTag(METAIO_STL::string optionName,
                        METAIO_STL::string longTag);

  /** Set the label for the option */
  bool SetOptionLabel(METAIO_STL::string optionName,
                      METAIO_STL::string label);

  /** Set the group for a field or an option
   *  If the group doesn't exist it is automatically created. */
  bool SetParameterGroup(METAIO_STL::string optionName,
                         METAIO_STL::string groupName,
                         METAIO_STL::string groupDescription="",
                         bool advanced=false);

  /** Collect all the information until the next tag 
   * \warning this function works only if the field is of type String */
  void SetOptionComplete(METAIO_STL::string optionName,
                         bool complete);  

  /** Get the values given the option name */
  bool GetValueAsBool(METAIO_STL::string optionName,
                      METAIO_STL::string fieldName="");
  bool GetValueAsBool(Option option,
                      METAIO_STL::string fieldName="");

  float GetValueAsFloat(METAIO_STL::string optionName,
                        METAIO_STL::string fieldName="");
  float GetValueAsFloat(Option option,
                        METAIO_STL::string fieldName="");

  int GetValueAsInt(METAIO_STL::string optionName,
                    METAIO_STL::string fieldName="");
  int GetValueAsInt(Option option,
                    METAIO_STL::string fieldName="");

  METAIO_STL::string GetValueAsString(METAIO_STL::string optionName,
                                      METAIO_STL::string fieldName="");
  METAIO_STL::string GetValueAsString(Option option,
                                      METAIO_STL::string fieldName="");

  METAIO_STL::list< METAIO_STL::string > GetValueAsList(
                                            METAIO_STL::string optionName);
  METAIO_STL::list< METAIO_STL::string > GetValueAsList(Option option);

  bool GetOptionWasSet(METAIO_STL::string optionName);
  bool GetOptionWasSet(Option option);

  /** List the options */
  void ListOptions();
  void ListOptionsXML();
  void ListOptionsSlicerXML();
  void ListOptionsSimplified(bool extended=true);

  Option * GetOptionByMinusTag(METAIO_STL::string minusTag);
  Option * GetOptionByTag(METAIO_STL::string minusTag);

  bool OptionExistsByMinusTag(METAIO_STL::string minusTag);

  bool Parse(int argc, char* argv[]);
  
  /** Given an XML buffer fill in the command line arguments */
  bool ParseXML(const char* buffer);

  /** Export the current command line arguments to a Grid Application
   *  Description file */
  bool ExportGAD(bool dynamic=false);

  /** Extract the date from cvs date */
  METAIO_STL::string ExtractDateFromCVS(METAIO_STL::string date);
  void               SetDateFromCVS(METAIO_STL::string date);

  /** Extract the version from cvs date */
  METAIO_STL::string ExtractVersionFromCVS(METAIO_STL::string version);
  void               SetVersionFromCVS(METAIO_STL::string version);

  /** Set the version of the app */
  METAIO_STL::string GetVersion() 
    { return m_Version; }

  void SetVersion(const char* version) 
    { m_Version=version; }
  
  /** Get the name of the application */
  METAIO_STL::string GetApplicationName() 
    { return m_ExecutableName; }

  /** Set the date of the app */
  METAIO_STL::string GetDate() 
    { return m_Date; }

  void SetDate(const char* date) 
    { m_Date=date; }

  void SetName(const char* name) 
    { m_Name=name; }

  /** Set the description */
  void SetDescription(const char* description) 
    { m_Description=description; }
  METAIO_STL::string GetDescription() const
    {return m_Description;}

  /** Set the author */
  void SetAuthor(const char* author) 
    { m_Author=author; }
  METAIO_STL::string GetAuthor() const
    {return m_Author;}

  /** Set the acknowledgments */
  void SetAcknowledgments(const char* acknowledgments) 
    { m_Acknowledgments=acknowledgments; }
  METAIO_STL::string GetAcknowledgments() const
    {return m_Acknowledgments;} 
  
  /** Set the category */
  void SetCategory(const char* category) 
    { m_Category=category; }
  METAIO_STL::string GetCategory() const
    {return m_Category;}

  long GetOptionId(Option* option);

  /** Return the list of options */
  const OptionVector & GetOptions() 
    { return m_OptionVector; }

  /** Return the list of parse options */
  const OptionVector & GetParsedOptions() 
    { return m_ParsedOptionVector; }

  void SetHelpCallBack(void (* newHelpCallBack)(void))
    { m_HelpCallBack = newHelpCallBack; }
  
  METAIO_STL::string TypeToString(TypeEnumType type);
  TypeEnumType StringToType(const char* type);

  void SetVerbose(bool verbose) {m_Verbose = verbose;}
  void SetParseFailureOnUnrecognizedOption(bool fail) 
  { m_FailOnUnrecognizedOption = fail; }

  /** Return true if we got the --xml */
  bool GotXMLFlag()
    {
    return m_GotXMLFlag;
    }

  /** Disable the deprecated warnings */
  void DisableDeprecatedWarnings();
  
  /** Load arguments from XML file. 
   *  The second argument when set to true allows
   *  external classes to use this function to parse XML
   *  arguments. */
  bool LoadArgumentsFromXML(const char* filename,
                            bool createMissingArguments=false);

protected:

  /** Small XML helper */
  METAIO_STL::string GetXML(const char* buffer,
                            const char* desc,
                            unsigned long pos);

  METAIO_STL::string m_Version;
  METAIO_STL::string m_Date;
  METAIO_STL::string m_Name;
  METAIO_STL::string m_Description;
  METAIO_STL::string m_Author;
  METAIO_STL::string m_ExecutableName;
  METAIO_STL::string m_Acknowledgments;
  METAIO_STL::string m_Category;

  ParameterGroupVector m_ParameterGroup;

private:

  void         (* m_HelpCallBack)(void);

  /** Set the value of an option or a field
   *  This is used when importing command line arguments
   *  from XML */
  bool SetOptionValue(const char* optionName,
                      const char* name, 
                      const char* value,
                      bool createMissingArgument=false);

  OptionVector m_OptionVector;
  OptionVector m_ParsedOptionVector; // We store the parsed option in
                                     //   case we have multiple options

  bool         m_Verbose;
  bool         m_FailOnUnrecognizedOption;
  bool         m_GotXMLFlag;
  bool         m_DisableDeprecatedWarnings;

  // Use when write --xml
  void WriteXMLOptionToCout(METAIO_STL::string optionName,unsigned int& index);

}; // end of class

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif 
