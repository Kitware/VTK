/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTokenizer.h

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

#ifndef __vtkTokenizer_h
#define __vtkTokenizer_h

#include <vtkTableAlgorithm.h>
#include <vtkUnicodeString.h> //Needed for delimiter specification

// .NAME vtkTokenizer - Converts a document collection into a term collection.
//
// .SECTION Description
// Given an artifact table containing text documents, splits each document
// into its component tokens, producing a feature table containing the results.
//
// Tokenization is performed by splitting input text into tokens based on
// character delimiters.  Delimiters are divided into two categories: "dropped"
// and "kept".  "Dropped" delimiters are discarded from the output, while "kept"
// delimiters are retained in the output as individual tokens.  Initially,
// vtkTokenizer has no delimiters defined, so you must set some delimiters
// before use.
//
// Users can reset and append to the lists of delimiters for each category.
// Delimiters are specified as half-open ranges of Unicode code points.  This
// makes it easy to tokenize logosyllabic scripts such as Chinese, Korean, and
// Japanese by specifying an entire range of logograms as "kept" delimiters, so
// that individual glyphs become tokens.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing zero-to-many "documents", with
//     one document per table row, a vtkIdTypeArray column containing document ids,
//     and a vtkUnicodeStringArray column containing the contents of each document.
//   Input port 1: (optional) A vtkTable containing zero-to-many document ranges
//     to be processed, with one range per table row, a vtkIdTypeArray column
//     containing document ids, a vtkIdTypeArray containing begin offsets, and a
//     vtkIdTypeArray column containing end offsets.  If input port 1 is left
//     unconnected, the filter will automatically process the entire contents of
//     every input document.
//
// Outputs:
//   Output port 0: A vtkTable containing "document", "begin", "end", "type", and
//     "text" columns. 
//
// Use SetInputArrayToProcess(0, ...) to specify the input table column that contains
// document ids (must be a vtkIdTypeArray).  Default: "document"
//
// Use SetInputArrayToProcess(1, ...) to specify the input table column that contains
// document contents (must be a vtkUnicodeStringArray).  Default: "text"
//
// Use SetInputArrayToProcess(2, 1, ...) to specify the input table column that contains
// range document ids (must be a vtkIdTypeArray).  Defaults to "document".
//
// Use SetInputArrayToProcess(3, 1, ...) to specify the input table column that contains
// range begin offsets (must be a vtkIdTypeArray).  Defaults to "begin".
//
// Use SetInputArrayToProcess(4, 1, ...) to specify the input table column that contains
// range end offsets (must be a vtkIdTypeArray).  Defaults to "end".
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkTokenizer :
  public vtkTableAlgorithm
{
public:
  static vtkTokenizer* New();
  vtkTypeMacro(vtkTokenizer, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Defines storage for a half-open range of Unicode characters [begin, end).
  typedef vtkstd::pair<vtkUnicodeString::value_type, vtkUnicodeString::value_type> DelimiterRange;
  // Description:
  // Defines storage for a collection of half-open ranges of Unicode characters.
  typedef vtkstd::vector<DelimiterRange> DelimiterRanges;
 
  // Description:
  // Returns a set of delimiter ranges that match Unicode punctuation codepoints.
  static const DelimiterRanges Punctuation();
  // Description:
  // Returns a set of delimiter ranges that match Unicode whitespace codepoints.
  static const DelimiterRanges Whitespace();
  // Description:
  // Returns a set of delimiter ranges that match logosyllabic languages where characters represent
  // words instead of sounds, such as Chinese, Japanese, and Korean.
  static const DelimiterRanges Logosyllabic();
  
  // Description:
  // Adds the half-open range of Unicode characters [begin, end) to the set of "dropped" delimiters.
  void AddDroppedDelimiters(vtkUnicodeString::value_type begin, vtkUnicodeString::value_type end);
  // Description:
  // Adds a collection of delimiter ranges to the set of "dropped" delimiters.
  void AddDroppedDelimiters(const DelimiterRanges& ranges);

  // Description:
  // Adds the half-open range of Unicode characters [begin, end) to the set of "kept" delimiters.
  void AddKeptDelimiters(vtkUnicodeString::value_type begin, vtkUnicodeString::value_type end);
  // Description:
  // Adds a collection of delimiter ranges to the set of "kept" delimiters.
  void AddKeptDelimiters(const DelimiterRanges& ranges);
//ETX

  // Description:
  // Convenience functions to specify delimiters, mainly intended for use from Python and
  // the ParaView server manager.  C++ developers are strongly encouraged to use
  // AddDroppedDelimiters(...) and AddKeptDelimiters(...) instead.
  void DropPunctuation();
  void DropWhitespace();
  void KeepPunctuation();
  void KeepWhitespace();
  void KeepLogosyllabic();

  // Description:
  // Clears the set of "dropped" delimiters.
  void ClearDroppedDelimiters();
  // Description:
  // Clears the set of "kept" delimiters.
  void ClearKeptDelimiters();

//BTX
protected:
  vtkTokenizer();
  ~vtkTokenizer();

  int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

private:
  vtkTokenizer(const vtkTokenizer &); // Not implemented.
  void operator=(const vtkTokenizer &); // Not implemented.

  class Internals;
  Internals* const Implementation;
//ETX
};

#endif // __vtkTokenizer_h

