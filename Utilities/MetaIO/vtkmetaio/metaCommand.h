/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METACOMMAND_H
#  define ITKMetaIO_METACOMMAND_H


#  ifdef _MSC_VER
#    pragma warning(disable : 4786)
#    pragma warning(disable : 4251)
#  endif

#  include <cstdlib>
#  include <string>
#  include <vector>
#  include <list>
#  include <map>

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class METAIO_EXPORT MetaCommand
{

public:
  typedef enum
  {
    DATA_NONE,
    DATA_IN,
    DATA_OUT
  } DataEnumType;
  typedef enum
  {
    INT,
    FLOAT,
    CHAR,
    STRING,
    LIST,
    FLAG,
    BOOL,
    IMAGE,
    ENUM,
    FILE
  } TypeEnumType;

  struct Field
  {
    std::string  name;
    std::string  description;
    std::string  value;
    TypeEnumType type;
    DataEnumType externaldata;
    std::string  rangeMin;
    std::string  rangeMax;
    bool         required;
    bool         userDefined;
  };

  struct Option
  {
    std::string        name;
    std::string        description;
    std::string        tag;
    std::string        longtag;
    std::string        label;
    std::vector<Field> fields;
    bool               required;
    bool               userDefined;
    bool               complete;
  };

  struct ParameterGroup
  {
    std::string              name;
    std::string              description;
    std::vector<std::string> options;
    bool                     advanced;
  };

  typedef std::vector<Option>         OptionVector;
  typedef std::vector<ParameterGroup> ParameterGroupVector;

  MetaCommand();
  ~MetaCommand() = default;

  bool
  SetOption(const Option& option);
  bool
  SetOption(std::string name, const std::string& shortTag, bool required, std::string description, std::vector<Field> fields);
  bool
  SetOption(const std::string&  name,
            const std::string&  shortTag,
            bool         required,
            std::string  description,
            TypeEnumType type = TypeEnumType::FLAG,
            std::string  defVal = "",
            DataEnumType externalData = DataEnumType::DATA_NONE);

  /** Fields are added in order */
  bool
  AddField(const std::string&  name,
           std::string  description,
           TypeEnumType type,
           DataEnumType externalData = DATA_NONE,
           std::string  rangeMin = "",
           std::string  rangeMax = "");

  /** For backward compatibility */
  bool
  AddField(const std::string& name, const std::string& description, TypeEnumType type, bool externalData);

  /** Add a field to an option */
  bool
  AddOptionField(const std::string&  optionName,
                 const std::string&  name,
                 TypeEnumType type,
                 bool         required = true,
                 const std::string&  defVal = "",
                 const std::string&  description = "",
                 DataEnumType externalData = DATA_NONE);

  /** Set the range of value as an option */
  bool
  SetOptionRange(const std::string& optionName, const std::string& name, const std::string& rangeMin, const std::string& rangeMax);

  /** Set the list of values that can be used with an option */
  bool
  SetOptionEnumerations(const std::string& optionName, const std::string& name, const std::string& optionEnums);

  /** Set the long tag for the option */
  bool
  SetOptionLongTag(const std::string& optionName, const std::string& longTag);

  /** Set the label for the option */
  bool
  SetOptionLabel(const std::string& optionName, const std::string& label);

  /** Set the group for a field or an option
   *  If the group doesn't exist it is automatically created. */
  bool
  SetParameterGroup(const std::string& optionName,
                    const std::string& groupName,
                    std::string groupDescription = "",
                    bool        advanced = false);

  /** Collect all the information until the next tag
   * \warning this function works only if the field is of type String */
  void
  SetOptionComplete(const std::string& optionName, bool complete);

  /** Get the values given the option name */
  bool
  GetValueAsBool(const std::string& optionName, const std::string& fieldName = "");
  static bool
  GetValueAsBool(Option option, const std::string& fieldName = "");

  float
  GetValueAsFloat(const std::string& optionName, const std::string& fieldName = "");
  static float
  GetValueAsFloat(Option option, const std::string& fieldName = "");

  int
  GetValueAsInt(const std::string& optionName, const std::string& fieldName = "");
  static int
  GetValueAsInt(Option option, const std::string& fieldName = "");

  std::string
  GetValueAsString(const std::string& optionName, const std::string& fieldName = "");
  static std::string
  GetValueAsString(Option option, const std::string& fieldName = "");

  std::list<std::string>
  GetValueAsList(const std::string& optionName);
  static std::list<std::string>
  GetValueAsList(Option option);

  bool
  GetOptionWasSet(const std::string& optionName);
  static bool
  GetOptionWasSet(const Option& option);

  /** List the options */
  void
  ListOptions();
  void
  ListOptionsXML();
  void
  ListOptionsSlicerXML();
  void
  ListOptionsSimplified(bool extended = true);

  Option *
  GetOptionByMinusTag(const std::string& minusTag);
  Option *
  GetOptionByTag(const std::string& tag);

  bool
  OptionExistsByMinusTag(const std::string& minusTag);

  bool
  Parse(int argc, char ** argv);

  /** Given an XML buffer fill in the command line arguments */
  bool
  ParseXML(const char * buffer);

  /** Export the current command line arguments to a Grid Application
   *  Description file */
  bool
  ExportGAD(bool dynamic = false);

  /** Extract the date from cvs date */
  static std::string
  ExtractDateFromCVS(std::string date);
  void
  SetDateFromCVS(std::string cvsDate);

  /** Extract the version from cvs date */
  static std::string
  ExtractVersionFromCVS(std::string version);
  void
  SetVersionFromCVS(std::string cvsVersion);

  /** Set the version of the app */
  std::string
  GetVersion()
  {
    return m_Version;
  }

  void
  SetVersion(const char * version)
  {
    m_Version = version;
  }

  /** Get the name of the application */
  std::string
  GetApplicationName()
  {
    return m_ExecutableName;
  }

  /** Set the date of the app */
  std::string
  GetDate()
  {
    return m_Date;
  }

  void
  SetDate(const char * date)
  {
    m_Date = date;
  }

  void
  SetName(const char * name)
  {
    m_Name = name;
  }

  /** Set the description */
  void
  SetDescription(const char * description)
  {
    m_Description = description;
  }
  std::string
  GetDescription() const
  {
    return m_Description;
  }

  /** Set the author */
  void
  SetAuthor(const char * author)
  {
    m_Author = author;
  }
  std::string
  GetAuthor() const
  {
    return m_Author;
  }

  /** Set the acknowledgments */
  void
  SetAcknowledgments(const char * acknowledgments)
  {
    m_Acknowledgments = acknowledgments;
  }
  std::string
  GetAcknowledgments() const
  {
    return m_Acknowledgments;
  }

  /** Set the category */
  void
  SetCategory(const char * category)
  {
    m_Category = category;
  }
  std::string
  GetCategory() const
  {
    return m_Category;
  }

  long
  GetOptionId(Option * option);

  /** Return the list of options */
  const OptionVector &
  GetOptions()
  {
    return m_OptionVector;
  }

  /** Return the list of parse options */
  const OptionVector &
  GetParsedOptions()
  {
    return m_ParsedOptionVector;
  }

  void
  SetHelpCallBack(void (*newHelpCallBack)())
  {
    m_HelpCallBack = newHelpCallBack;
  }

  static std::string
  TypeToString(TypeEnumType type);
  static TypeEnumType
  StringToType(const char * type);

  void
  SetVerbose(bool verbose)
  {
    m_Verbose = verbose;
  }
  void
  SetParseFailureOnUnrecognizedOption(bool fail)
  {
    m_FailOnUnrecognizedOption = fail;
  }

  /** Return true if we got the --xml */
  bool
  GotXMLFlag() const
  {
    return m_GotXMLFlag;
  }

  /** Disable the deprecated warnings */
  void
  DisableDeprecatedWarnings();

  /** Load arguments from XML file.
   *  The second argument when set to true allows
   *  external classes to use this function to parse XML
   *  arguments. */
  static bool
  LoadArgumentsFromXML(const char * filename, bool createMissingArguments = false);

protected:
  /** Small XML helper */
  static std::string
  GetXML(const char * buffer, const char * desc, unsigned long pos);

  std::string m_Version;
  std::string m_Date;
  std::string m_Name;
  std::string m_Description;
  std::string m_Author;
  std::string m_ExecutableName;
  std::string m_Acknowledgments;
  std::string m_Category;

  ParameterGroupVector m_ParameterGroup;

private:
  void (*m_HelpCallBack)();

  /** Set the value of an option or a field
   *  This is used when importing command line arguments
   *  from XML */
  bool
  SetOptionValue(const char * optionName, const char * name, const char * value, bool createMissingArgument = false);

  OptionVector m_OptionVector;
  OptionVector m_ParsedOptionVector; // We store the parsed option in
                                     //   case we have multiple options

  bool m_Verbose;
  bool m_FailOnUnrecognizedOption;
  bool m_GotXMLFlag;
  bool m_DisableDeprecatedWarnings;

  // Use when write --xml
  void
  WriteXMLOptionToCout(const std::string& optionName, unsigned int & index);

}; // end of class

#  if (METAIO_USE_NAMESPACE)
};
#  endif

#endif
