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
  typedef enum {INT,FLOAT,CHAR,STRING,LIST,FLAG,BOOL} TypeEnumType;

  struct Field{
    METAIO_STL::string  name;
    METAIO_STL::string  description;
    METAIO_STL::string  value;
    TypeEnumType type;
    DataEnumType externaldata;
    METAIO_STL::string  rangeMin;
    METAIO_STL::string  rangeMax;
    bool         required;
    bool         userDefined;
    };

  struct Option{
    METAIO_STL::string        name;
    METAIO_STL::string        description;
    METAIO_STL::string        tag;
    METAIO_STL::vector<Field> fields;
    bool               required;
    bool               userDefined;
    bool               complete;
  };

  typedef METAIO_STL::vector<Option>                OptionVector; 
  
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
                bool externalData
                )
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


  /** Collect all the information until the next tag 
   * \warning this function works only if the field is of type String */
  void SetOptionComplete(METAIO_STL::string optionName,
                         bool complete);  

  /** Get the values given the option name */
  bool GetValueAsBool(METAIO_STL::string optionName,METAIO_STL::string fieldName="");
  bool GetValueAsBool(Option option,METAIO_STL::string fieldName="");

  float GetValueAsFloat(METAIO_STL::string optionName,METAIO_STL::string fieldName="");
  float GetValueAsFloat(Option option,METAIO_STL::string fieldName="");

  int GetValueAsInt(METAIO_STL::string optionName,METAIO_STL::string fieldName="");
  int GetValueAsInt(Option option,METAIO_STL::string fieldName="");

  METAIO_STL::string GetValueAsString(METAIO_STL::string optionName,METAIO_STL::string fieldName="");
  METAIO_STL::string GetValueAsString(Option option,METAIO_STL::string fieldName="");

  METAIO_STL::list< METAIO_STL::string > GetValueAsList(METAIO_STL::string optionName);
  METAIO_STL::list< METAIO_STL::string > GetValueAsList(Option option);

  bool GetOptionWasSet(METAIO_STL::string optionName);
  bool GetOptionWasSet(Option option);

  /** List the options */
  void ListOptions();
  void ListOptionsXML();
  void ListOptionsSimplified();

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

  void SetDescription(const char* description) 
    { m_Description=description; }
  METAIO_STL::string GetDescription() const
    {return m_Description;}

  void SetAuthor(const char* author) 
    { m_Author=author; }
  METAIO_STL::string GetAuthor() const
    {return m_Author;}

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
protected:

  /** Small XML helper */
  METAIO_STL::string GetXML(const char* buffer,const char* desc,unsigned long pos);

  METAIO_STL::string m_Version;
  METAIO_STL::string m_Date;
  METAIO_STL::string m_Name;
  METAIO_STL::string m_Description;
  METAIO_STL::string m_Author;
  METAIO_STL::string m_ExecutableName;

private:

  void         (* m_HelpCallBack)(void);

  OptionVector m_OptionVector;
  OptionVector m_ParsedOptionVector; // We store the parsed option in
                                     //   case we have multiple options

  bool         m_Verbose;
  bool         m_FailOnUnrecognizedOption;
}; // end of class

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif 
