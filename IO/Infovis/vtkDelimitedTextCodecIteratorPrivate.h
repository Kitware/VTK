// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkDelimitedTextCodecIteratorPrivate
 * @brief Implements vtkTextCodec::OutputIterator to
 * fill a vtkTable from text input.
 *
 * vtkDelimitedTextCodecIteratorPrivate parses an input text
 * to generate a vtkTable. It supports Delimiters configuration.
 * Column type can be detected to output numeric arrays
 * instead of string arrays.
 */

#ifndef vtkDelimitedTextCodecIterator_h
#define vtkDelimitedTextCodecIterator_h

#include "vtkTextCodec.h"

#include <set> //for set

VTK_ABI_NAMESPACE_BEGIN

class vtkAbstractArray;
class vtkDoubleArray;
class vtkIntArray;
class vtkStringArray;
class vtkTable;

class vtkDelimitedTextCodecIteratorPrivate : public vtkTextCodec::OutputIterator
{
public:
  vtkDelimitedTextCodecIteratorPrivate(vtkIdType startRecords, vtkIdType maxRecords,
    const std::string& recordDelimiters, const std::string& fieldDelimiters,
    const std::string& stringDelimiters, const std::string& whitespace, const std::string& comments,
    const std::string& escape, bool haveHeaders, bool mergConsDelimiters, bool useStringDelimiter,
    bool detectNumericColumns, bool forceDouble, int defaultInt, double defaultDouble,
    vtkTable* outputTable);

  ~vtkDelimitedTextCodecIteratorPrivate() override;

  /**
   * Handle windows files that do not have a carriage return line feed on the last line of the file
   */
  void ReachedEndOfInput();

  /**
   * Entry point to parse text
   */
  vtkDelimitedTextCodecIteratorPrivate& operator=(const vtkTypeUInt32& value) override;

private:
  void operator=(const vtkDelimitedTextCodecIteratorPrivate&) = delete;
  vtkDelimitedTextCodecIteratorPrivate(const vtkDelimitedTextCodecIteratorPrivate&) = delete;

  template <typename T>
  static vtkSmartPointer<vtkStringArray> ToStringArray(T* array);

  /**
   * Convert an int Array to a double one.
   * Copy input content to output.
   */
  static vtkSmartPointer<vtkDoubleArray> ToDoubleArray(vtkIntArray* array);

  /**
   * Append value in an Int array.
   * Convert array if needed.
   * Return the new array if constructed, or nullptr.
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkIntArray* array, vtkIdType index, const std::string& str);

  /**
   * Append value in a Double array.
   * Convert array if needed.
   * Return the new array if constructed, or nullptr.
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkDoubleArray* array, vtkIdType index, const std::string& str);

  /**
   * Append value in a string array.
   * Always return nullptr as it never convert the array to a new type.
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkStringArray* array, vtkIdType index, const std::string& str);

  /**
   * Append value to array.
   * Forwards to the dedicated method specialized for each array type.
   * Convert the array as needed to another type. If converted, return the new array.
   * By default, return nullptr (array was not converted).
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkAbstractArray* array, vtkIdType index, const std::string& str);

  /**
   * Create new array with the good basic type.
   * Use header to set array name or construct a default name.
   */
  void CreateColumn();

  /**
   * Insert value in a column.
   * Create the column as needed.
   * Convert the column type if needed.
   */
  void InsertField();

  /**
   * Utility struct to count records and get associated information.
   * A "Record" is usually a line (see RecordDelimiters)
   */
  struct RecordsCounter
  {
    RecordsCounter(bool has, vtkIdType max, vtkIdType start)
      : HasMax(has)
      , Max(max)
      , Start(start)
    {
    }

    bool HasMax = false;
    vtkIdType Max = 0;
    vtkIdType Start = 0;
    vtkIdType Skipped = 0;
    vtkIdType Current = 0;

    // Return true if max records was reached
    bool MaxReached();

    // Return true if current records is acceptable, based on its index
    bool AcceptingField();

    // Return true if this is the first acceptable records
    bool FirstAccepted();

    // Increment current index
    void Next();

    // Mark current record as skipped.
    void Skip();

    // Return the current number of read record.
    vtkIdType GetNumberOfAcceptedRecords();
  };

  RecordsCounter RecordsCount;

  std::set<vtkTypeUInt32> RecordDelimiters;
  std::set<vtkTypeUInt32> FieldDelimiters;
  std::set<vtkTypeUInt32> StringDelimiters;
  std::set<vtkTypeUInt32> Whitespace;
  std::set<vtkTypeUInt32> CommentChar = { '#' };
  std::set<vtkTypeUInt32> EscapeDelimiter;

  bool HaveHeaders = false;
  bool WhiteSpaceOnlyString = true;
  vtkTable* OutputTable = nullptr;
  vtkIdType CurrentFieldIndex = 0;
  std::string CurrentField;
  bool RecordAdjacent = true;
  bool MergeConsDelims = false;
  bool ProcessEscapeSequence = false;
  bool UseStringDelimiter = true;
  bool DetectNumericColumns = false;
  bool ForceDouble = false;
  int DefaultIntegerValue = 0;
  double DefaultDoubleValue = 0.;
  vtkTypeUInt32 WithinString = 0;
  bool WithinComment = false;
};

VTK_ABI_NAMESPACE_END
#endif
/* VTK-HeaderTest-Exclude: INCLUDES:CLASSES */
