/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVariantSerialization.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */

#include "vtkSmartPointer.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkVariantBoostSerialization.h"

#include <sstream>
#include <string.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "vtk_utf8.h"

int TestVariantSerialization(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;

  // Build a vtkVariantArray with a variety of values in it.
  vtkSmartPointer<vtkVariantArray> sourceArray = vtkSmartPointer<vtkVariantArray>::New();
  sourceArray->SetName("Values");
  sourceArray->SetNumberOfTuples(7);
  sourceArray->SetValue(0, vtkVariant('V'));
  sourceArray->SetValue(1, 3.14f);
  sourceArray->SetValue(2, 2.71);
  sourceArray->SetValue(3, "Test string");
  sourceArray->SetValue(4, 17);
  sourceArray->SetValue(5, 42l);

  // This is the first two words from the Iliad in Greek -- approximately 'Mnviv aeide'
  const vtkTypeUInt16 greek_text_utf16[] = { 0x039C, 0x03B7, 0x03BD, 0x03B9, 0x03BD, ' ', 0x03B1,
    0x03B5, 0x03B9, 0x03B4, 0x03B5, 0 };

  vtkStdString text;
  utf8::utf16to8(
    std::begin(greek_text_utf16), std::end(greek_text_utf16), std::back_inserter(text));
  sourceArray->SetValue(6, text);

  // Serialize the array
  std::ostringstream out_stream;
  boost::archive::text_oarchive out(out_stream);
  out << static_cast<const vtkVariantArray&>(*sourceArray);

  // De-serialize the array
  vtkSmartPointer<vtkVariantArray> sinkArray = vtkSmartPointer<vtkVariantArray>::New();
  std::istringstream in_stream(out_stream.str());
  boost::archive::text_iarchive in(in_stream);
  in >> *sinkArray;

  // Check that the arrays are the same
  if (strcmp(sourceArray->GetName(), sinkArray->GetName()) != 0)
  {
    cerr << "Sink array has name \"" << sinkArray->GetName() << "\", should be \""
         << sourceArray->GetName() << "\".\n";
    ++errors;
  }

  if (sourceArray->GetNumberOfTuples() != sinkArray->GetNumberOfTuples())
  {
    cerr << "Sink array has " << sinkArray->GetNumberOfTuples() << " of elements, should be "
         << sourceArray->GetNumberOfTuples() << ".\n";
    ++errors;
    return errors;
  }

  for (vtkIdType i = 0; i < sourceArray->GetNumberOfTuples(); ++i)
  {
    if (sourceArray->GetValue(i).GetType() != sinkArray->GetValue(i).GetType())
    {
      cerr << "Sink array value at index " << i << " has type " << sinkArray->GetValue(i).GetType()
           << ", should be" << sourceArray->GetValue(i).GetType() << ".\n";
      ++errors;
      return errors;
    }
  }

#define VTK_VARIANT_ARRAY_DATA_CHECK(Index, Function, Kind)                                        \
  do                                                                                               \
  {                                                                                                \
    if (sourceArray->GetValue(Index).Function() != sinkArray->GetValue(Index).Function())          \
    {                                                                                              \
      cerr << Kind << " mismatch: \"" << sourceArray->GetValue(Index).Function() << "\" vs. \""    \
           << sinkArray->GetValue(Index).Function() << "\".\n";                                    \
      ++errors;                                                                                    \
    }                                                                                              \
  } while (false)

  VTK_VARIANT_ARRAY_DATA_CHECK(0, ToChar, "Character");
  VTK_VARIANT_ARRAY_DATA_CHECK(1, ToFloat, "Float");
  VTK_VARIANT_ARRAY_DATA_CHECK(2, ToDouble, "Double");
  const char* source = sourceArray->GetValue(3).ToString().c_str();
  const char* sink = sinkArray->GetValue(3).ToString().c_str();
  if (strcmp(source, sink) != 0)
  {
    cerr << "String mismatch: \"" << source << "\" vs. \"" << sink << "\".\n";
    ++errors;
  }
  VTK_VARIANT_ARRAY_DATA_CHECK(4, ToInt, "Int");
  VTK_VARIANT_ARRAY_DATA_CHECK(5, ToLong, "Long");
  source = sourceArray->GetValue(6).ToString().c_str();
  sink = sinkArray->GetValue(6).ToString().c_str();
  if (strcmp(source, sink) != 0)
  {
    cerr << "String mismatch: \"" << source << "\" vs. \"" << sink << "\".\n";
    ++errors;
  }
#undef VTK_VARIANT_ARRAY_DATA_CHECK

  return errors;
}
