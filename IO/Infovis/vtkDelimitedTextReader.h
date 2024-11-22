// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

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
 * vtkStdString columns in the output table.  In addition, it is
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
#include "vtkStdString.h"       // Needed for vtkStdString
#include "vtkTableAlgorithm.h"

#include <memory>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkTextCodec;

class VTKIOINFOVIS_EXPORT vtkDelimitedTextReader : public vtkTableAlgorithm
{
public:
  static vtkDelimitedTextReader* New();
  vtkTypeMacro(vtkDelimitedTextReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specifies the delimited text file to be loaded.
   */
  vtkGetFilePathMacro(FileName);
  vtkSetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the InputString for use when reading from a character array.
   * Optionally include the length for binary strings. Note that a copy
   * of the string is made and stored. If this causes exceedingly large
   * memory consumption, consider using InputArray instead.
   */
  void SetInputString(const char* in);
  vtkGetStringMacro(InputString);
  void SetInputString(const char* in, int len);
  vtkGetMacro(InputStringLength, int);
  void SetInputString(const vtkStdString& input)
  {
    this->SetInputString(input.c_str(), static_cast<int>(input.length()));
  }
  ///@}

  ///@{
  /**
   * Enable reading from an InputString or InputArray instead of the default,
   * a file.
   */
  vtkSetMacro(ReadFromInputString, vtkTypeBool);
  vtkGetMacro(ReadFromInputString, vtkTypeBool);
  vtkBooleanMacro(ReadFromInputString, vtkTypeBool);
  ///@}

  ///@{
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
  ///@}

  ///@{
  /**
   * Specify the character(s) that will be used to separate records.
   * The order of characters in the string does not matter.  Defaults
   * to "\r\n".
   */
  void SetUTF8RecordDelimiters(const char* delimiters);
  const char* GetUTF8RecordDelimiters();
  ///@}

  ///@{
  /**
   * Specify the character(s) that will be used to separate fields.  For
   * example, set this to "," for a comma-separated value file.  Set
   * it to ".:;" for a file where columns can be separated by a
   * period, colon or semicolon.  The order of the characters in the
   * string does not matter.  Defaults to a comma.
   */
  vtkSetStringMacro(FieldDelimiterCharacters);
  vtkGetStringMacro(FieldDelimiterCharacters);
  ///@}

  void SetUTF8FieldDelimiters(const char* delimiters);
  const char* GetUTF8FieldDelimiters();

  ///@{
  /**
   * Get/set the character that will begin and end strings.  Microsoft
   * Excel, for example, will export the following format:

   * "First Field","Second Field","Field, With, Commas","Fourth Field"

   * The third field has a comma in it.  By using a string delimiter,
   * this will be correctly read.  The delimiter defaults to '"'.
   */
  vtkGetMacro(StringDelimiter, char);
  vtkSetMacro(StringDelimiter, char);
  ///@}

  void SetUTF8StringDelimiters(const char* delimiters);
  const char* GetUTF8StringDelimiters();

  ///@{
  /**
   * Set/get whether to use the string delimiter.  Defaults to on.
   */
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);
  vtkBooleanMacro(UseStringDelimiter, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to treat the first line of the file as headers.
   * The default is false (no headers).
   */
  vtkGetMacro(HaveHeaders, bool);
  vtkSetMacro(HaveHeaders, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to merge successive delimiters.  Use this if (for
   * example) your fields are separated by spaces but you don't know
   * exactly how many.
   */
  vtkSetMacro(MergeConsecutiveDelimiters, bool);
  vtkGetMacro(MergeConsecutiveDelimiters, bool);
  vtkBooleanMacro(MergeConsecutiveDelimiters, bool);
  ///@}

  ///@{
  /**
   * Specifies the maximum number of records to read from the file.  Limiting the
   * number of records to read is useful for previewing the contents of a file.
   * Note: see Preview.
   */
  vtkGetMacro(MaxRecords, vtkIdType);
  vtkSetMacro(MaxRecords, vtkIdType);
  ///@}

  ///@{
  /**
   * Specifies the first records to read, so it is possible to skip some header text.
   * Default is 0.
   */
  vtkGetMacro(SkippedRecords, vtkIdType);
  vtkSetMacro(SkippedRecords, vtkIdType);
  ///@}

  ///@{
  /**
   * When set to true, the reader will detect numeric columns and create
   * vtkDoubleArray or vtkIntArray for those instead of vtkStringArray. Default
   * is off.
   * Then, it works as follow:
   *  - uses vtkIntArray
   *  - if data is not an int, try vtkDoubleArray
   *  - if data is not a double, fallback to vtkStringArray
   */
  vtkSetMacro(DetectNumericColumns, bool);
  vtkGetMacro(DetectNumericColumns, bool);
  vtkBooleanMacro(DetectNumericColumns, bool);
  ///@}

  ///@{
  /**
   * When set to true and DetectNumericColumns is also true, forces all
   * numeric columns to vtkDoubleArray even if they contain only
   * integer values. Default is off.
   */
  vtkSetMacro(ForceDouble, bool);
  vtkGetMacro(ForceDouble, bool);
  vtkBooleanMacro(ForceDouble, bool);
  ///@}

  ///@{
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
  ///@}

  ///@{
  /**
   * When DetectNumericColumns is set to true, the reader use this value to populate
   * the vtkIntArray where empty strings are found. Default is 0.
   */
  vtkSetMacro(DefaultIntegerValue, int);
  vtkGetMacro(DefaultIntegerValue, int);
  ///@}

  ///@{
  /**
   * When DetectNumericColumns is set to true, the reader use this value to populate
   * the vtkDoubleArray where empty strings are found. Default is 0.0
   */
  vtkSetMacro(DefaultDoubleValue, double);
  vtkGetMacro(DefaultDoubleValue, double);
  ///@}

  ///@{
  /**
   * The name of the array for generating or assigning pedigree ids
   * (default "id").
   */
  vtkSetStringMacro(PedigreeIdArrayName);
  vtkGetStringMacro(PedigreeIdArrayName);
  ///@}

  ///@{
  /**
   * If on (default), generates pedigree ids automatically.
   * If off, assign one of the arrays to be the pedigree id.
   */
  vtkSetMacro(GeneratePedigreeIds, bool);
  vtkGetMacro(GeneratePedigreeIds, bool);
  vtkBooleanMacro(GeneratePedigreeIds, bool);
  ///@}

  ///@{
  /**
   * If on, assigns pedigree ids to output. Defaults to off.
   */
  vtkSetMacro(OutputPedigreeIds, bool);
  vtkGetMacro(OutputPedigreeIds, bool);
  vtkBooleanMacro(OutputPedigreeIds, bool);
  ///@}

  ///@{
  /**
   * If on, also add in the tab (i.e. \c '\\t') character as a field delimiter.
   * We add this specially since applications may have a more
   * difficult time doing this. Defaults to off.
   */
  vtkSetMacro(AddTabFieldDelimiter, bool);
  vtkGetMacro(AddTabFieldDelimiter, bool);
  vtkBooleanMacro(AddTabFieldDelimiter, bool);
  ///@}

  /**
   * Returns a human-readable description of the most recent error, if any.
   * Otherwise, returns an empty string.  Note that the result is only valid
   * after calling Update().
   */
  vtkStdString GetLastError();

  ///@{
  /**
   * Fallback character for use in the US-ASCII-WITH-FALLBACK
   * character set.  Any characters that have their 8th bit set will
   * be replaced with this code point.  Defaults to 'x'.
   */
  vtkSetMacro(ReplacementCharacter, vtkTypeUInt32);
  vtkGetMacro(ReplacementCharacter, vtkTypeUInt32);
  ///@}

  /**
   * Return the first lines as a single string.
   * Number of read lines is defined by PreviewNumberOfLines
   * This is updated in RequestInformation pass, so one can use
   * it before the actual RequestData.
   */
  vtkGetMacro(Preview, std::string);

  ///@{
  /**
   * Set / Get The number of lines to read for the preview.
   * Default is 0.
   */
  vtkSetMacro(PreviewNumberOfLines, vtkIdType);
  vtkGetMacro(PreviewNumberOfLines, vtkIdType);
  ///@}

  ///@{
  /**
   * Set / Get the list of possible characters used to start comments section.
   * Comment section will start at first matching character.
   * So multi-character (like `//`) is not supported.
   * Default is `#`.
   */
  vtkGetMacro(CommentCharacters, std::string);
  vtkSetMacro(CommentCharacters, std::string);
  ///@}

protected:
  vtkDelimitedTextReader();
  ~vtkDelimitedTextReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Read the content of the input file.
  int ReadData(vtkTable* output_table);

  char* FileName = nullptr;
  vtkTypeBool ReadFromInputString = 0;
  char* InputString = nullptr;
  int InputStringLength = 0;
  char* UnicodeCharacterSet = nullptr;
  vtkIdType SkippedRecords = 0;
  vtkIdType MaxRecords = 0;
  std::string UnicodeRecordDelimiters = "\r\n";
  std::string UnicodeFieldDelimiters = ",";
  std::string UnicodeStringDelimiters = "\"";
  std::string UnicodeWhitespace = " \t\r\n\v\f";
  std::string UnicodeEscapeCharacter = "\\";
  std::string CommentCharacters = "#";
  bool DetectNumericColumns = false;
  bool ForceDouble = false;
  bool TrimWhitespacePriorToNumericConversion = false;
  int DefaultIntegerValue = 0;
  double DefaultDoubleValue = 0.;
  char* FieldDelimiterCharacters = nullptr;
  char StringDelimiter = '"';
  bool UseStringDelimiter = true;
  bool HaveHeaders = false;
  bool MergeConsecutiveDelimiters = false;
  char* PedigreeIdArrayName = nullptr;
  bool GeneratePedigreeIds = true;
  bool OutputPedigreeIds = false;
  bool AddTabFieldDelimiter = false;
  vtkStdString LastError = "";
  vtkTypeUInt32 ReplacementCharacter = 'x';

  std::string Preview;
  vtkIdType PreviewNumberOfLines = 0;

private:
  /**
   * Create and return an ifstream or an isstring stream depending on configuration.
   * Return nullptr if stream cannot be open (e.g. unable to open file)
   */
  std::unique_ptr<std::istream> OpenStream();

  /**
   * Read the BOM and configure a default value for UnicodeCharacterSet,
   * if not set explicitly.
   */
  void ReadBOM(std::istream* stream);

  /**
   * Create a vtkTextCodec for the given stream.
   * Uses UnicodeCharacterSet if given, or try to find an one.
   */
  vtkTextCodec* CreateTextCodec(std::istream* input_stream);

  vtkDelimitedTextReader(const vtkDelimitedTextReader&) = delete;
  void operator=(const vtkDelimitedTextReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
