/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMimeTypes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#include <vtkForceMimeTypeStrategy.h>
#include <vtkMimeTypes.h>
#include <vtkSmartPointer.h>

#include <vtkstd/stdexcept>
#include <vtksys/ios/sstream>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtkstd::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

int TestMimeTypes(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkMimeTypes> mime_types = vtkSmartPointer<vtkMimeTypes>::New();

    test_expression(mime_types->Lookup("file:///home/bob/a.foo") == "");
    test_expression(mime_types->Lookup("file:///home/bob/b.txt") == "text/plain");
    test_expression(mime_types->Lookup("file:///home/bob/c.doc") == "application/msword");
    test_expression(mime_types->Lookup("file:///home/bob/d.pdf") == "application/pdf");

    vtkSmartPointer<vtkForceMimeTypeStrategy> force_mime_type = vtkSmartPointer<vtkForceMimeTypeStrategy>::New();
    mime_types->PrependStrategy(force_mime_type);

    test_expression(mime_types->Lookup("file:///home/bob/a.foo") == "text/plain");
    test_expression(mime_types->Lookup("file:///home/bob/b.txt") == "text/plain");
    test_expression(mime_types->Lookup("file:///home/bob/c.doc") == "text/plain");
    test_expression(mime_types->Lookup("file:///home/bob/d.pdf") == "text/plain");

    force_mime_type->SetMimeType("foo/bar");

    test_expression(mime_types->Lookup("file:///home/bob/a.foo") == "foo/bar");
    test_expression(mime_types->Lookup("file:///home/bob/b.txt") == "foo/bar");
    test_expression(mime_types->Lookup("file:///home/bob/c.doc") == "foo/bar");
    test_expression(mime_types->Lookup("file:///home/bob/d.pdf") == "foo/bar");

    test_expression(mime_types->Match("*/*", ""));
    test_expression(!mime_types->Match("*/*", "bleh"));
    test_expression(mime_types->Match("*/*", "text/plain"));
    test_expression(mime_types->Match("text/*", "text/plain"));
    test_expression(!mime_types->Match("text/*", "application/pdf"));
    test_expression(mime_types->Match("foo/bar", "foo/bar"));
    test_expression(!mime_types->Match("foo/bar", "foo/baz"));
 
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

