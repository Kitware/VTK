/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelimitedTextReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/


/**
 * @class   vtkDelimitedTextReader
 * @brief   reads in delimited ascii or unicode text files
 * and outputs a vtkTable data structure.
 *
 *
 * vtkDelimitedTextReader is an interface for pulling in data from a
 * flat, delimited ascii or unicode text file (delimiter can be any character).
 *
 * The behavior of the reader with respect to ascii or unicode input
 * is controlled by the SetUnicodeCharacterSet() method.  By default
 * (without calling SetUnicodeCharacterSet()), the reader will expect
 * to read ascii text and will output vtkStdString columns.  Use the
 * Set and Get methods to set delimiters that do not contain UTF8 in
 * the name when operating the reader in default ascii mode.  If the
 * SetUnicodeCharacterSet() method is called, the reader will output
 * vtkUnicodeString columns in the output table.  In addition, it is
 * necessary to use the Set and Get methods that contain UTF8 in the
 * name to specify delimiters when operating in unicode mode.
 *
 * There is also a special character set US-ASCII-WITH-FALLBACK that
 * will treat the input text as ASCII no matter what.  If and when it
 * encounters a character with its 8th bit set it will replace that
 * character with the code point ReplacementCharacter.  You may use
 * this if you have text that belongs to a code page like LATIN9 or
 * ISO-8859-1 or friends: mostly ASCII but not entirely.  Eventually
 * this class will acquire the ability to read gracefully text from
 * any code page, making this option obsolete.
 *
 * This class emits ProgressEvent for every 100 lines it reads.
 *
 * @par Thanks:
 * Thanks to Andy Wilson, Brian Wylie, Tim Shead, and Thomas Otahal
 * from Sandia National Laboratories for implementing this class.
 *
 *
 * @warning
 * This reader assumes that the first line in the file (whether that's
 * headers or the first document) contains at least as many fields as
 * any other line in the file.
*/

#ifndef vtkDelimitedTextReader_h
#define vtkDelimitedTextReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkTableAlgorithm.h"
#include "vtkUnicodeString.h" // Needed for vtkUnicodeString
#include "vtkStdString.h" // Needed for vtkStdString

class VTKIOINFOVIS_EXPORT vtkDelimitedTextReader : public vtkTableAlgorithm
{
public:
  static vtkDelimitedTextReader* New();
  vtkTypeMacro(vtkDelimitedTextReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specifies the delimited text file to be loaded.
   */
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the InputString for use when reading from a character array.
   * Optionally include the length for binary strings. Note that a copy
   * of the string is made and stored. If this causes exceedingly large
   * memory consumption, consider using InputArray instead.
   */
  void SetInputString(const char *in);
  vtkGetStringMacro(InputString);
  void SetInputString(const char *in, int len);
  vtkGetMacro(InputStringLength, int);
  void SetInputString(const vtkStdString& input)
    { this->SetInputString(input.c_str(), static_cast<int>(input.length())); }
  //@}

  //@{
  /**
   * Enable reading from an InputString or InputArray instead of the default,
   * a file.
   */
  vtkSetMacro(ReadFromInputString,int);
  vtkGetMacro(ReadFromInputString,int);
  vtkBooleanMacro(ReadFromInputString,int);
  //@}

  //@{
  /**
   * Specifies the character set used in the input file.  Valid character set
   * names will be drawn from the list maintained by the Internet Assigned Name
   * Authority at

   * http://www.iana.org/assignments/character-sets

   * Where multiple aliases are provided for a character set, the preferred MIME name
   * will be used.  vtkUnicodeDelimitedTextReader currently supports "US-ASCII", "UTF-8",
   * "UTF-16", "UTF-16BE", and "UTF-16LE" character sets.
   */
  vtkGetStringMacro(UnicodeCharacterSet);
  vtkSetStringMacro(UnicodeCharacterSet);
  //@}

  //@{
  /**
   * Specify the character(s) that will be used to separate records.
   * The order of characters in the string does not matter.  Defaults
   * to "\r\n".
   */
  void SetUTF8RecordDelimiters(const char* delimiters);
  const char* GetUTF8RecordDelimiters();
  void SetUnicodeRecordDelimiters(const vtkUnicodeString& delimiters);
  vtkUnicodeString GetUnicodeRecordDelimiters();
  //@}

  //@{
  /**
   * Specify the character(s) that will be used to separate fields.  For
   * example, set this to "," for a comma-separated value file.  Set
   * it to ".:;" for a file where columns can be separated by a
   * period, colon or semicolon.  The order of the characters in the
   * string does not matter.  Defaults to a comma.
   */
  vtkSetStringMacro(FieldDelimiterCharacters);
  vtkGetStringMacro(FieldDelimiterCharacters);
  //@}

  void SetUTF8FieldDelimiters(const char* delimiters);
  const char* GetUTF8FieldDelimiters();
  void SetUnicodeFieldDelimiters(const vtkUnicodeString& delimiters);
  vtkUnicodeString GetUnicodeFieldDelimiters();

  //@{
  /**
   * Get/set the character that will begin and end strings.  Microsoft
   * Excel, for example, will export the following format:

   * "First Field","Second Field","Field, With, Commas","Fourth Field"

   * The third field has a comma in it.  By using a string delimiter,
   * this will be correctly read.  The delimiter defaults to '"'.
   */
  vtkGetMacro(StringDelimiter, char);
  vtkSetMacro(StringDelimiter, char);
  //@}

  void SetUTF8StringDelimiters(const char* delimiters);
  const char* GetUTF8StringDelimiters();
  void SetUnicodeStringDelimiters(const vtkUnicodeString& delimiters);
  vtkUnicodeString GetUnicodeStringDelimiters();

  //@{
  /**
   * Set/get whether to use the string delimiter.  Defaults to on.
   */
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);
  vtkBooleanMacro(UseStringDelimiter, bool);
  //@}

  //@{
  /**
   * Set/get whether to treat the first line of the file as headers.
   * The default is false (no headers).
   */
  vtkGetMacro(HaveHeaders, bool);
  vtkSetMacro(HaveHeaders, bool);
  //@}

  //@{
  /**
   * Set/get whether to merge successive delimiters.  Use this if (for
   * example) your fields are separated by spaces but you don't know
   * exactly how many.
   */
  vtkSetMacro(MergeConsecutiveDelimiters, bool);
  vtkGetMacro(MergeConsecutiveDelimiters, bool);
  vtkBooleanMacro(MergeConsecutiveDelimiters, bool);
  //@}

  //@{
  /**
   * Specifies the maximum number of records to read from the file.  Limiting the
   * number of records to read is useful for previewing the contents of a file.
   */
  vtkGetMacro(MaxRecords, vtkIdType);
  vtkSetMacro(MaxRecords, vtkIdType);
  //@}

  //@{
  /**
   * When set to true, the reader will detect numeric columns and create
   * vtkDoubleArray or vtkIntArray for those instead of vtkStringArray. Default
   * is off.
   */
  vtkSetMacro(DetectNumericColumns, bool);
  vtkGetMacro(DetectNumericColumns, bool);
  vtkBooleanMacro(DetectNumericColumns, bool);
  //@}

  //@{
  /**
   * When set to true and DetectNumericColumns is also true, forces all
   * numeric columns to vtkDoubleArray even if they contain only
   * integer values. Default is off.
   */
  vtkSetMacro(ForceDouble, bool);
  vtkGetMacro(ForceDouble, bool);
  vtkBooleanMacro(ForceDouble, bool);
  //@}

  //@{
  /**
   * When DetectNumericColumns is set to true, whether to trim whitespace from
   * strings prior to conversion to a numeric.
   * Default is false to preserve backward compatibility.

   * vtkVariant handles whitespace inconsistently, so trim it before we try to
   * convert it.  For example:

   * vtkVariant("  2.0").ToDouble() == 2.0 <-- leading whitespace is not a problem
   * vtkVariant("  2.0  ").ToDouble() == NaN <-- trailing whitespace is a problem
   * vtkVariant("  infinity  ").ToDouble() == NaN <-- any whitespace is a problem

   * In these cases, trimming the whitespace gives us the result we expect:
   * 2.0 and INF respectively.
   */
  vtkSetMacro(TrimWhitespacePriorToNumericConversion, bool);
  vtkGetMacro(TrimWhitespacePriorToNumericConversion, bool);
  vtkBooleanMacro(TrimWhitespacePriorToNumericConversion, bool);
  //@}

  //@{
  /**
   * When DetectNumericColumns is set to true, the reader use this value to populate
   * the vtkIntArray where empty strings are found. Default is 0.
   */
  vtkSetMacro(DefaultIntegerValue, int);
  vtkGetMacro(DefaultIntegerValue, int);
  //@}

  //@{
  /**
   * When DetectNumericColumns is set to true, the reader use this value to populate
   * the vtkDoubleArray where empty strings are found. Default is 0.0
   */
  vtkSetMacro(DefaultDoubleValue, double);
  vtkGetMacro(DefaultDoubleValue, double);
  //@}

  //@{
  /**
   * The name of the array for generating or assigning pedigree ids
   * (default "id").
   */
  vtkSetStringMacro(PedigreeIdArrayName);
  vtkGetStringMacro(PedigreeIdArrayName);
  //@}

  //@{
  /**
   * If on (default), generates pedigree ids automatically.
   * If off, assign one of the arrays to be the pedigree id.
   */
  vtkSetMacro(GeneratePedigreeIds, bool);
  vtkGetMacro(GeneratePedigreeIds, bool);
  vtkBooleanMacro(GeneratePedigreeIds, bool);
  //@}

  //@{
  /**
   * If on, assigns pedigree ids to output. Defaults to off.
   */
  vtkSetMacro(OutputPedigreeIds, bool);
  vtkGetMacro(OutputPedigreeIds, bool);
  vtkBooleanMacro(OutputPedigreeIds, bool);
  //@}

  /**
   * Returns a human-readable description of the most recent error, if any.
   * Otherwise, returns an empty string.  Note that the result is only valid
   * after calling Update().
   */
  vtkStdString GetLastError();

  //@{
  /**
   * Fallback character for use in the US-ASCII-WITH-FALLBACK
   * character set.  Any characters that have their 8th bit set will
   * be replaced with this code point.  Defaults to 'x'.
   */
  vtkSetMacro(ReplacementCharacter, vtkTypeUInt32);
  vtkGetMacro(ReplacementCharacter, vtkTypeUInt32);
  //@}

protected:
  vtkDelimitedTextReader();
  ~vtkDelimitedTextReader() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  char* FileName;
  int ReadFromInputString;
  char *InputString;
  int InputStringLength;
  char* UnicodeCharacterSet;
  vtkIdType MaxRecords;
  vtkUnicodeString UnicodeRecordDelimiters;
  vtkUnicodeString UnicodeFieldDelimiters;
  vtkUnicodeString UnicodeStringDelimiters;
  vtkUnicodeString UnicodeWhitespace;
  vtkUnicodeString UnicodeEscapeCharacter;
  bool DetectNumericColumns;
  bool ForceDouble;
  bool TrimWhitespacePriorToNumericConversion;
  int DefaultIntegerValue;
  double DefaultDoubleValue;
  char* FieldDelimiterCharacters;
  char StringDelimiter;
  bool UseStringDelimiter;
  bool HaveHeaders;
  bool UnicodeOutputArrays;
  bool MergeConsecutiveDelimiters;
  char* PedigreeIdArrayName;
  bool GeneratePedigreeIds;
  bool OutputPedigreeIds;
  vtkStdString LastError;
  vtkTypeUInt32 ReplacementCharacter;

private:
  vtkDelimitedTextReader(const vtkDelimitedTextReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDelimitedTextReader&) VTK_DELETE_FUNCTION;

};

#endif

