// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 * This reader assume the time step column is numeric. A warning is
 * set otherwise. The DetectNumericColumns field is set to on,
 * do not change this field unless you really know what you are
 * doing.
 *
 * @see vtkDelimitedTextReader
 */

#ifndef vtkTemporalDelimitedTextReader_h
#define vtkTemporalDelimitedTextReader_h

#include "vtkDelimitedTextReader.h"

#include "vtkIOInfovisModule.h" // module export
#include "vtkNew.h"             // For ReadTable field

#include <map>    // To store the TimeMap
#include <vector> // To store the TimeMap

VTK_ABI_NAMESPACE_BEGIN
class VTKIOINFOVIS_EXPORT vtkTemporalDelimitedTextReader : public vtkDelimitedTextReader
{
public:
  static vtkTemporalDelimitedTextReader* New();
  vtkTypeMacro(vtkTemporalDelimitedTextReader, vtkDelimitedTextReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the name of the column to use as time indicator.
   * Ignored if TimeColumnId is not equal to -1.
   * If no column has been chosen using either the TimeColumnId or the
   * TimeColumnName the whole input file is outputted.
   * Default to empty string.
   */
  vtkGetMacro(TimeColumnName, std::string);
  void SetTimeColumnName(std::string name);
  ///@}

  ///@{
  /**
   * Get/Set the column to use as time indicator.
   * It the TimeColumnId is equal to -1, the TimeColumnName will be used
   * instead.
   * If no column has been chosen using either the TimeColumnId or the
   * TimeColumnName the whole input file is outputted.
   * Default to -1.
   */
  vtkGetMacro(TimeColumnId, int);
  void SetTimeColumnId(int idx);
  ///@}

  ///@{
  /**
   * Set the RemoveTimeStepColumn flag
   * If this boolean is true, the output will not contain the Time step column.
   * Default to true.
   */
  vtkGetMacro(RemoveTimeStepColumn, bool);
  void SetRemoveTimeStepColumn(bool rts);
  ///@}

  /** Internal fields of this reader use a specific MTime (InternalMTime).
   * This mechanism ensures the actual data is only re-read when necessary.
   * Here, we ensure the GetMTime of this reader stay consistent by returning
   * the latest between the MTime of this reader and the internal one.
   *
   * @see InternalModified
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkTemporalDelimitedTextReader();
  ~vtkTemporalDelimitedTextReader() override = default;

  /**
   * In order to fill the TIME_STEPS and TIME_RANGE keys, this method call the
   * ReadData function that actually read the full input file content (may be
   * slow!). Custom MTime management is used to ensure we do not re-read the
   * input file uselessly.
   */
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * This function checks if a user specified column has been set and check if
   * this input is valid. If an invalid input has been detected, return false.
   * Otherwise, InternalColumnName will be set to the name of the time column
   * or empty if none has been given by the user.
   */
  bool EnforceColumnName();

  /**
   * When parameters specific of this reader are modified, we do not want to
   * re-read the input file. Keep an internal time stamp to track them.
   */
  void InternalModified();

  // Time column fields
  std::string TimeColumnName;
  std::string InternalColumnName;
  vtkIdType TimeColumnId = -1;
  bool RemoveTimeStepColumn = true;
  std::map<double, std::vector<vtkIdType>> TimeMap;

  // Input file content and update
  vtkNew<vtkTable> ReadTable;
  vtkMTimeType LastReadTime = 0;
  vtkTimeStamp InternalMTime;

private:
  vtkTemporalDelimitedTextReader(const vtkTemporalDelimitedTextReader&) = delete;
  void operator=(const vtkTemporalDelimitedTextReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
