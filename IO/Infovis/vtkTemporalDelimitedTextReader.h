/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDelimitedTextReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTemporalDelimitedTextReader
 * @brief   reads a delimited ascii or unicode text files and and output a
 * temporal vtkTable.
 *
 * This reader requires that FieldDelimiterCharacters is set before
 * the pipeline is executed, otherwise it will produce an empty output.
 *
 * A column can be selected as time step indicator using the SetTimeColumnName
 * or SetTimeColumnId functions. If so, for a given time step 's' only the
 * lines where the time step indicator column have the value 's' are present.
 * To control if the time step indicator column should be present in the
 * output, a RemoveTimeStepColumn option is available. If no time step
 * indicator column is given by the user, the whole file it outputted.
 *
 * The FileName, FieldDelimiterCharacters, HaveHeaders,
 * MergeConsecutiveDelimiters and AddTabFieldDelimiter options of the
 * vtkDelimitedTextReader are directly exposed by this reader. If you need to
 * specify other options of the delimited text reader, you can access the
 * internal reader using the GetCSVReader.
 *
 * @see vtkDelimitedTextReader
 */

#ifndef vtkTemporalDelimitedTextReader_h
#define vtkTemporalDelimitedTextReader_h

#include "vtkIOInfovisModule.h"
#include "vtkTableAlgorithm.h"

#include "vtkSetGet.h"       // For field macros
#include "vtkSmartPointer.h" // For the InternalReader field
#include "vtkStringArray.h"  // Fot the GetAllVariableArrayNames

#include <map> // To store the TimeMap

class vtkDelimitedTextReader;

class VTKIOINFOVIS_EXPORT vtkTemporalDelimitedTextReader : public vtkTableAlgorithm
{
public:
  static vtkTemporalDelimitedTextReader* New();
  vtkTypeMacro(vtkTemporalDelimitedTextReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the column to use as time
   * default to empty string: this field is not used to retrieve the time step
   * column.
   * Using this setter also reset the TimeColumnId to -1.
   * If neither TimeColumnName nor TimeColumnId are set, the whole data set is
   * outputted.
   */
  vtkGetMacro(TimeColumnName, std::string);
  void SetTimeColumnName(const std::string name);
  //@}

  //@{
  /**
   * Get/Set the column to use as time
   * default to -1: this field is not used to retrieve the time step column.
   * Using this setter also reset the TimeColumnName to empty string.
   * If neither TimeColumnName nor TimeColumnId are set, the whole data set is
   * outputted.
   */
  vtkGetMacro(TimeColumnId, int);
  void SetTimeColumnId(const int idx);
  //@}

  //@{
  /**
   * Set the RemoveTimeStepColumn flag
   * If this boolean is true, the output will not contain the Time step column.
   */
  vtkGetMacro(RemoveTimeStepColumn, bool);
  vtkSetMacro(RemoveTimeStepColumn, bool);
  vtkBooleanMacro(RemoveTimeStepColumn, bool);
  //@}

  /**
   * Get the internal reader used to parse the CSV.
   * This function may be used to change reader settings not
   * exposed by this reader (like unicode ...)
   * Note, you should not change the DetectNumericColumns parameter of the
   * internal reader.
   */
  vtkGetSmartPointerMacro(InternalReader, vtkDelimitedTextReader);

  //@{
  /**
   * Get/Set the FileName of the input CSV / TCSV file
   */
  vtkGetMacro(FileName, std::string);
  vtkSetMacro(FileName, std::string);
  //@}

  //@{
  /**
   * Get/Set the separator to use when parsing the file
   * default: ","
   */
  vtkSetMacro(FieldDelimiterCharacters, std::string);
  vtkGetMacro(FieldDelimiterCharacters, std::string);
  //@}

  //@{
  /**
   * Get/Set the HaveHeaders flag that drive if the first
   * line of the CSV file should be considered as the name
   * of the column (default false)
   */
  vtkGetMacro(HaveHeaders, bool);
  vtkSetMacro(HaveHeaders, bool);
  vtkBooleanMacro(HaveHeaders, bool);
  //@}

  //@{
  /**
   * Get/Set the MergeConsecutiveDelimiters flag.
   * Default: false;
   */
  vtkGetMacro(MergeConsecutiveDelimiters, bool);
  vtkSetMacro(MergeConsecutiveDelimiters, bool);
  vtkBooleanMacro(MergeConsecutiveDelimiters, bool);
  //@}

  //@{
  /**
   * Get/Set the AddTabFieldDelimiter flag.
   * This add the tab character to the FieldDelimiterCharacters. It is a
   * convenience method for GUI application to set the tab character. Default:
   * false;
   */
  vtkGetMacro(AddTabFieldDelimiter, bool);
  vtkSetMacro(AddTabFieldDelimiter, bool);
  vtkBooleanMacro(AddTabFieldDelimiter, bool);
  //@}

protected:
  vtkTemporalDelimitedTextReader();
  ~vtkTemporalDelimitedTextReader() override = default;

  // In order to fill the TIME_STEPS and TIME_RANGE keys, this method call
  // update on the internal csv reader and so read all the data.
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * This function check if a user specified column
   * has been set and check if this input is valid.
   * If none have been given or an invalid input has
   * been detected, return false. If a valid input
   * was found, TimeColumnName will be set to the
   * corresponding value after the call.
   */
  bool EnforceColumnName(vtkTable* inputTable);

  /**
   * Setup the InternalReader with the given parameters
   * and call Update on it to read the input file.
   */
  void ReadInputFile();

  // Time column fields
  std::string TimeColumnName = "";
  vtkIdType TimeColumnId = -1;
  bool RemoveTimeStepColumn = true;
  vtkNew<vtkStringArray> ColumnNames;
  std::map<double, std::vector<vtkIdType> > TimeMap;

  // Reader fields
  vtkNew<vtkDelimitedTextReader> InternalReader;
  std::string FileName = "";
  std::string FieldDelimiterCharacters = "";
  bool HaveHeaders = false;
  bool MergeConsecutiveDelimiters = false;
  bool AddTabFieldDelimiter = false;

private:
  vtkTemporalDelimitedTextReader(const vtkTemporalDelimitedTextReader&) = delete;
  void operator=(const vtkTemporalDelimitedTextReader&) = delete;
};

#endif
