/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLY.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*

The interface routines for reading and writing PLY polygon files.

Greg Turk, February 1994

---------------------------------------------------------------

A PLY file contains a single polygonal _object_.

An object is composed of lists of _elements_.  Typical elements are
vertices, faces, edges and materials.

Each type of element for a given object has one or more _properties_
associated with the element type.  For instance, a vertex element may
have as properties the floating-point values x,y,z and the three unsigned
chars representing red, green and blue.

---------------------------------------------------------------

Copyright (c) 1994 The Board of Trustees of The Leland Stanford
Junior University.  All rights reserved.

Permission to use, copy, modify and distribute this software and its
documentation for any purpose is hereby granted without fee, provided
that the above copyright notice and this permission notice appear in
all copies of this software and that you do not sell the software.

THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

*/

#include "vtkPLY.h"
#include "vtkByteSwap.h"
#include "vtkHeap.h"
#include "vtkMath.h"
#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>

/* memory allocation */
#define myalloc(mem_size) vtkPLY::my_alloc((mem_size), __LINE__, __FILE__)

// wjs: added to manage memory leak
static vtkHeap* plyHeap = nullptr;
static void plyInitialize()
{
  if (plyHeap == nullptr)
  {
    plyHeap = vtkHeap::New();
  }
}
static void plyCleanUp()
{
  if (plyHeap)
  {
    plyHeap->Delete();
    plyHeap = nullptr;
  }
}
static void* plyAllocateMemory(size_t n)
{
  return plyHeap->AllocateMemory(n);
}

static const char* type_names[] = { "invalid", "char", "short", "int", "int8", "int16", "int32",
  "uchar", "ushort", "uint", "uint8", "uint16", "uint32", "float", "float32", "double", "float64" };

static const int ply_type_size[] = { 0, 1, 2, 4, 1, 2, 4, 1, 2, 4, 1, 2, 4, 4, 4, 8 };

#define NO_OTHER_PROPS (-1)

#define DONT_STORE_PROP 0
#define STORE_PROP 1

#define OTHER_PROP 0
#define NAMED_PROP 1

/*************/
/*  Writing  */
/*************/

/******************************************************************************
Given a file pointer, get ready to write PLY data to the file.

Entry:
  os         - the given output stream
  nelems     - number of elements in object
  elem_names - list of element names
  file_type  - file type, either ascii or binary

Exit:
  returns a pointer to a PlyFile, used to refer to this file, or nullptr if error
******************************************************************************/

PlyFile* vtkPLY::ply_write(std::ostream* os, int nelems, const char** elem_names, int file_type)
{
  int i;
  PlyFile* plyfile;
  PlyElement* elem;

  /* check for nullptr file pointer */
  if (os == nullptr)
    return (nullptr);

  /* create a record for this object */

  plyfile = (PlyFile*)myalloc(sizeof(PlyFile));
  plyfile->file_type = file_type;
  plyfile->num_comments = 0;
  plyfile->num_obj_info = 0;
  plyfile->nelems = nelems;
  plyfile->version = 1.0;
  plyfile->os = os;
  plyfile->is = nullptr;
  plyfile->other_elems = nullptr;

  /* tuck aside the names of the elements */

  plyfile->elems = (PlyElement**)myalloc(sizeof(PlyElement*) * nelems);
  for (i = 0; i < nelems; i++)
  {
    elem = (PlyElement*)myalloc(sizeof(PlyElement));
    plyfile->elems[i] = elem;
    elem->name = strdup(elem_names[i]);
    elem->num = 0;
    elem->nprops = 0;
  }

  /* return pointer to the file descriptor */
  return (plyfile);
}

/******************************************************************************
Open a PLY file for writing.

Entry:
  filename   - name of file to read from
  nelems     - number of elements in object
  elem_names - list of element names
  file_type  - file type, either ascii or binary

Exit:
  version - version number of PLY file
  returns a file identifier, used to refer to this file, or nullptr if error
******************************************************************************/

PlyFile* vtkPLY::ply_open_for_writing(
  const char* filename, int nelems, const char** elem_names, int file_type)
{
  PlyFile* plyfile;
  char* name;
  vtksys::ofstream* ofs = nullptr;

  // memory leaks
  plyInitialize();

  /* tack on the extension .ply, if necessary */
  size_t nameSize = sizeof(char) * (strlen(filename) + 5);
  name = (char*)myalloc(nameSize);
  strncpy(name, filename, nameSize);
  if (strlen(name) < 4 || strcmp(name + strlen(name) - 4, ".ply") != 0)
    strcat(name, ".ply");

  /* open the file for writing */
  ofs = new vtksys::ofstream;

  ofs->open(name, std::istream::out | std::istream::binary);
  free(name); // wjs remove memory leak//
  if (!ofs->is_open())
  {
    delete ofs;
    return (nullptr);
  }

  /* create the actual PlyFile structure */

  plyfile = vtkPLY::ply_write(ofs, nelems, elem_names, file_type);
  if (plyfile == nullptr)
  {
    ofs->close();
    delete ofs;
    return (nullptr);
  }

  /* return pointer to the file descriptor */
  return (plyfile);
}

/******************************************************************************
Open a PLY file for writing.

Entry:
  output     - string to write to
  nelems     - number of elements in object
  elem_names - list of element names
  file_type  - file type, either ascii or binary

Exit:
  version - version number of PLY file
  returns a file identifier, used to refer to this file, or nullptr if error
******************************************************************************/

PlyFile* vtkPLY::ply_open_for_writing_to_string(
  std::string& output, int nelems, const char** elem_names, int file_type)
{
  PlyFile* plyfile;
  // memory leaks
  plyInitialize();
  std::ostringstream* oss = new std::ostringstream;
  oss->str(output);

  /* create the actual PlyFile structure */
  plyfile = vtkPLY::ply_write(oss, nelems, elem_names, file_type);
  if (plyfile == nullptr)
  {
    delete oss;
    return (nullptr);
  }

  /* return pointer to the file descriptor */
  return (plyfile);
}

/******************************************************************************
Describe an element, including its properties and how many will be written
to the file.

Entry:
  plyfile   - file identifier
  elem_name - name of element that information is being specified about
  nelems    - number of elements of this type to be written
  nprops    - number of properties contained in the element
  prop_list - list of properties
******************************************************************************/

void vtkPLY::ply_describe_element(
  PlyFile* plyfile, const char* elem_name, int nelems, int nprops, PlyProperty* prop_list)
{
  int i;
  PlyElement* elem;
  PlyProperty* prop;

  /* look for appropriate element */
  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
  {
    vtkGenericWarningMacro("ply_describe_element: can't find element " << elem_name);
    return;
  }

  elem->num = nelems;

  /* copy the list of properties */

  elem->nprops = nprops;
  elem->props = (PlyProperty**)myalloc(sizeof(PlyProperty*) * nprops);
  elem->store_prop = (char*)myalloc(sizeof(char) * nprops);

  for (i = 0; i < nprops; i++)
  {
    prop = (PlyProperty*)myalloc(sizeof(PlyProperty));
    elem->props[i] = prop;
    elem->store_prop[i] = NAMED_PROP;
    copy_property(prop, &prop_list[i]);
  }
}

/******************************************************************************
Describe a property of an element.

Entry:
  plyfile   - file identifier
  elem_name - name of element that information is being specified about
  prop      - the new property
******************************************************************************/

void vtkPLY::ply_describe_property(PlyFile* plyfile, const char* elem_name, PlyProperty* prop)
{
  PlyElement* elem;
  PlyProperty* elem_prop;

  /* look for appropriate element */
  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
  {
    vtkGenericWarningMacro("ply_describe_property: can't find element " << elem_name);
    return;
  }

  /* create room for new property */

  if (elem->nprops == 0)
  {
    elem->props = (PlyProperty**)myalloc(sizeof(PlyProperty*));
    elem->store_prop = (char*)myalloc(sizeof(char));
    elem->nprops = 1;
  }
  else
  {
    elem->nprops++;
    elem->props = (PlyProperty**)realloc(elem->props, sizeof(PlyProperty*) * elem->nprops);
    elem->store_prop = (char*)realloc(elem->store_prop, sizeof(char) * elem->nprops);
  }

  /* copy the new property */
  elem->other_offset = 0; // added by wjs Purify UMR
  elem_prop = (PlyProperty*)myalloc(sizeof(PlyProperty));
  elem->props[elem->nprops - 1] = elem_prop;
  elem->store_prop[elem->nprops - 1] = NAMED_PROP;
  copy_property(elem_prop, prop);
}

/******************************************************************************
Describe what the "other" properties are that are to be stored, and where
they are in an element.
******************************************************************************/

void vtkPLY::ply_describe_other_properties(PlyFile* plyfile, PlyOtherProp* other, int offset)
{
  int i;
  PlyElement* elem;
  PlyProperty* prop;

  /* look for appropriate element */
  elem = find_element(plyfile, other->name);
  if (elem == nullptr)
  {
    vtkGenericWarningMacro("ply_describe_other_properties: can't find element " << other->name);
    return;
  }

  /* create room for other properties */

  if (elem->nprops == 0)
  {
    elem->props = (PlyProperty**)myalloc(sizeof(PlyProperty*) * other->nprops);
    elem->store_prop = (char*)myalloc(sizeof(char) * other->nprops);
    elem->nprops = 0;
  }
  else
  {
    int newsize;
    newsize = elem->nprops + other->nprops;
    elem->props = (PlyProperty**)realloc(elem->props, sizeof(PlyProperty*) * newsize);
    elem->store_prop = (char*)realloc(elem->store_prop, sizeof(char) * newsize);
  }

  /* copy the other properties */

  for (i = 0; i < other->nprops; i++)
  {
    prop = (PlyProperty*)myalloc(sizeof(PlyProperty));
    copy_property(prop, other->props[i]);
    elem->props[elem->nprops] = prop;
    elem->store_prop[elem->nprops] = OTHER_PROP;
    elem->nprops++;
  }

  /* save other info about other properties */
  elem->other_size = other->size;
  elem->other_offset = offset;
}

/******************************************************************************
State how many of a given element will be written.

Entry:
  plyfile   - file identifier
  elem_name - name of element that information is being specified about
  nelems    - number of elements of this type to be written
******************************************************************************/

void vtkPLY::ply_element_count(PlyFile* plyfile, const char* elem_name, int nelems)
{
  PlyElement* elem;

  /* look for appropriate element */
  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
  {
    vtkGenericWarningMacro("ply_element_count: can't find element " << elem_name);
    return;
  }

  elem->num = nelems;
}

/******************************************************************************
Signal that we've described everything a PLY file's header and that the
header should be written to the file.

Entry:
  plyfile - file identifier
******************************************************************************/

void vtkPLY::ply_header_complete(PlyFile* plyfile)
{
  int i, j;
  std::ostream* os = plyfile->os;
  PlyElement* elem;
  PlyProperty* prop;

  *os << "ply\n";

  switch (plyfile->file_type)
  {
    case PLY_ASCII:
      *os << "format ascii 1.0\n";
      break;
    case PLY_BINARY_BE:
      *os << "format binary_big_endian 1.0\n";
      break;
    case PLY_BINARY_LE:
      *os << "format binary_little_endian 1.0\n";
      break;
    default:
      *os << "ply_header_complete: bad file type = " << plyfile->file_type << "\n";
      assert(0);
  }

  /* write out the comments */

  for (i = 0; i < plyfile->num_comments; i++)
    *os << "comment " << plyfile->comments[i] << "\n";

  /* write out object information */

  for (i = 0; i < plyfile->num_obj_info; i++)
    *os << "obj_info " << plyfile->obj_info[i] << "\n";

  /* write out information about each element */

  for (i = 0; i < plyfile->nelems; i++)
  {

    elem = plyfile->elems[i];
    *os << "element " << elem->name << " " << elem->num << "\n";

    /* write out each property */
    for (j = 0; j < elem->nprops; j++)
    {
      prop = elem->props[j];
      if (prop->is_list)
      {
        *os << "property list ";
        write_scalar_type(os, prop->count_external);
        *os << " ";
        write_scalar_type(os, prop->external_type);
        *os << " " << prop->name << "\n";
      }
      else
      {
        *os << "property ";
        write_scalar_type(os, prop->external_type);
        *os << " " << prop->name << "\n";
      }
    }
  }

  *os << "end_header\n";
}

/******************************************************************************
Specify which elements are going to be written.  This should be called
before a call to the routine ply_put_element().

Entry:
  plyfile   - file identifier
  elem_name - name of element we're talking about
******************************************************************************/

void vtkPLY::ply_put_element_setup(PlyFile* plyfile, const char* elem_name)
{
  PlyElement* elem;

  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
  {
    vtkGenericWarningMacro("ply_put_element_setup: can't find element " << elem_name);
    return;
  }

  plyfile->which_elem = elem;
}

/******************************************************************************
Write an element to the file.  This routine assumes that we're
writing the type of element specified in the last call to the routine
ply_put_element_setup().

Entry:
  plyfile  - file identifier
  elem_ptr - pointer to the element
******************************************************************************/

void vtkPLY::ply_put_element(PlyFile* plyfile, void* elem_ptr)
{
  int j, k;
  std::ostream* os = plyfile->os;
  PlyElement* elem;
  PlyProperty* prop;
  char *elem_data, *item;
  char** item_ptr;
  int list_count;
  int item_size;
  int int_val = 0;
  unsigned int uint_val = 0;
  double double_val = 0.0;
  char** other_ptr;

  elem = plyfile->which_elem;
  other_ptr = (char**)(((char*)elem_ptr) + elem->other_offset);

  /* write out either to an ascii or binary file */

  if (plyfile->file_type == PLY_ASCII)
  {

    /* write an ascii file */

    /* write out each property of the element */
    for (j = 0; j < elem->nprops; j++)
    {
      prop = elem->props[j];
      if (elem->store_prop[j] == OTHER_PROP)
        elem_data = *other_ptr;
      else
        elem_data = (char*)elem_ptr;
      if (prop->is_list)
      {
        item = elem_data + prop->count_offset;
        get_stored_item(item, prop->count_internal, &int_val, &uint_val, &double_val);
        write_ascii_item(os, int_val, uint_val, double_val, prop->count_external);
        list_count = uint_val;
        item_ptr = (char**)(elem_data + prop->offset);
        item = item_ptr[0];
        item_size = ply_type_size[prop->internal_type];
        for (k = 0; k < list_count; k++)
        {
          get_stored_item(item, prop->internal_type, &int_val, &uint_val, &double_val);
          write_ascii_item(os, int_val, uint_val, double_val, prop->external_type);
          item += item_size;
        }
      }
      else
      {
        item = elem_data + prop->offset;
        get_stored_item(item, prop->internal_type, &int_val, &uint_val, &double_val);
        write_ascii_item(os, int_val, uint_val, double_val, prop->external_type);
      }
    }

    *os << "\n";
  }
  else
  {

    /* write a binary file */

    /* write out each property of the element */
    for (j = 0; j < elem->nprops; j++)
    {
      prop = elem->props[j];
      if (elem->store_prop[j] == OTHER_PROP)
        elem_data = *other_ptr;
      else
        elem_data = (char*)elem_ptr;
      if (prop->is_list)
      {
        item = elem_data + prop->count_offset;
        // item_size = ply_type_size[prop->count_internal];
        get_stored_item(item, prop->count_internal, &int_val, &uint_val, &double_val);
        write_binary_item(plyfile, int_val, uint_val, double_val, prop->count_external);
        list_count = uint_val;
        item_ptr = (char**)(elem_data + prop->offset);
        item = item_ptr[0];
        item_size = ply_type_size[prop->internal_type];
        for (k = 0; k < list_count; k++)
        {
          get_stored_item(item, prop->internal_type, &int_val, &uint_val, &double_val);
          write_binary_item(plyfile, int_val, uint_val, double_val, prop->external_type);
          item += item_size;
        }
      }
      else
      {
        item = elem_data + prop->offset;
        get_stored_item(item, prop->internal_type, &int_val, &uint_val, &double_val);
        write_binary_item(plyfile, int_val, uint_val, double_val, prop->external_type);
      }
    }
  }
}

/******************************************************************************
Specify a comment that will be written in the header.

Entry:
  plyfile - file identifier
  comment - the comment to be written
******************************************************************************/

void vtkPLY::ply_put_comment(PlyFile* plyfile, const char* comment)
{
  /* (re)allocate space for new comment */
  if (plyfile->num_comments == 0)
  {
    plyfile->comments = (char**)myalloc(sizeof(char*));
  }
  else
  {
    plyfile->comments =
      (char**)realloc(plyfile->comments, sizeof(char*) * (plyfile->num_comments + 1));
  }

  /* add comment to list */
  plyfile->comments[plyfile->num_comments] = strdup(comment);
  plyfile->num_comments++;
}

/******************************************************************************
Specify a piece of object information (arbitrary text) that will be written
in the header.

Entry:
  plyfile  - file identifier
  obj_info - the text information to be written
******************************************************************************/

void vtkPLY::ply_put_obj_info(PlyFile* plyfile, const char* obj_info)
{
  /* (re)allocate space for new info */
  if (plyfile->num_obj_info == 0)
  {
    plyfile->obj_info = (char**)myalloc(sizeof(char*));
  }
  else
  {
    plyfile->obj_info =
      (char**)realloc(plyfile->obj_info, sizeof(char*) * (plyfile->num_obj_info + 1));
  }

  /* add info to list */
  plyfile->obj_info[plyfile->num_obj_info] = strdup(obj_info);
  plyfile->num_obj_info++;
}

/*************/
/*  Reading  */
/*************/

/******************************************************************************
Given a file pointer, get ready to read PLY data from the file.

Entry:
  is - the given input stream

Exit:
  nelems     - number of elements in object
  elem_names - list of element names
  returns a pointer to a PlyFile, used to refer to this file, or nullptr if error
******************************************************************************/

PlyFile* vtkPLY::ply_read(std::istream* is, int* nelems, char*** elem_names)
{
  int i, j;
  PlyFile* plyfile;
  int nwords;
  char** words;
  char** elist;
  PlyElement* elem;
  char* orig_line;

  /* check for nullptr file pointer */
  if (is == nullptr)
    return (nullptr);

  /* create record for this object */

  plyfile = (PlyFile*)myalloc(sizeof(PlyFile));
  plyfile->nelems = 0;
  plyfile->comments = nullptr;
  plyfile->num_comments = 0;
  plyfile->obj_info = nullptr;
  plyfile->num_obj_info = 0;
  plyfile->is = is;
  plyfile->os = nullptr;
  plyfile->other_elems = nullptr;

  /* read and parse the file's header */

  words = get_words(plyfile->is, &nwords, &orig_line);
  if (!words || !equal_strings(words[0], "ply"))
  {
    free(plyfile);
    if (words)
      free(words);
    return (nullptr);
  }

  while (words)
  {

    /* parse words */

    if (equal_strings(words[0], "format"))
    {
      if (nwords != 3)
      {
        free(plyfile);
        free(words);
        return (nullptr);
      }
      if (equal_strings(words[1], "ascii"))
        plyfile->file_type = PLY_ASCII;
      else if (equal_strings(words[1], "binary_big_endian"))
        plyfile->file_type = PLY_BINARY_BE;
      else if (equal_strings(words[1], "binary_little_endian"))
        plyfile->file_type = PLY_BINARY_LE;
      else
      {
        free(plyfile);
        free(words);
        return (nullptr);
      }
      plyfile->version = atof(words[2]);
    }
    else if (equal_strings(words[0], "element"))
      add_element(plyfile, words, nwords);
    else if (equal_strings(words[0], "property"))
      add_property(plyfile, words, nwords);
    else if (equal_strings(words[0], "comment"))
      add_comment(plyfile, orig_line);
    else if (equal_strings(words[0], "obj_info"))
      add_obj_info(plyfile, orig_line);
    else if (equal_strings(words[0], "end_header"))
    {
      free(words);
      words = nullptr;
      break;
    }

    /* free up words space */
    free(words);

    words = get_words(plyfile->is, &nwords, &orig_line);
  }

  if (plyfile->nelems == 0)
  {
    free(plyfile);
    free(words);
    return (nullptr);
  }

  /* create tags for each property of each element, to be used */
  /* later to say whether or not to store each property for the user */

  for (i = 0; i < plyfile->nelems; i++)
  {
    elem = plyfile->elems[i];
    elem->store_prop = (char*)myalloc(sizeof(char) * elem->nprops);
    for (j = 0; j < elem->nprops; j++)
      elem->store_prop[j] = DONT_STORE_PROP;
    elem->other_offset = NO_OTHER_PROPS; /* no "other" props by default */
  }

  /* set return values about the elements */

  elist = (char**)myalloc(sizeof(char*) * plyfile->nelems);
  for (i = 0; i < plyfile->nelems; i++)
    elist[i] = strdup(plyfile->elems[i]->name);

  *elem_names = elist;
  *nelems = plyfile->nelems;

  /* return a pointer to the file's information */

  return (plyfile);
}

/******************************************************************************
Open a polygon file for reading.

Entry:
  filename - name of file to read from

Exit:
  nelems     - number of elements in object
  elem_names - list of element names
  returns a file identifier, used to refer to this file, or nullptr if error
******************************************************************************/

PlyFile* vtkPLY::ply_open_for_reading(const char* filename, int* nelems, char*** elem_names)
{
  vtksys::ifstream* ifs;
  PlyFile* plyfile;

  // memory leaks
  plyInitialize();

  /* open the file for reading */
  ifs = new vtksys::ifstream;

  ifs->open(filename, std::ios::in | std::ios::binary);
  if (!ifs->is_open())
  {
    delete ifs;
    return (nullptr);
  }

  /* create the PlyFile data structure */

  plyfile = vtkPLY::ply_read(ifs, nelems, elem_names);
  if (plyfile == nullptr)
  {
    ifs->close();
    delete ifs;
    return (nullptr);
  }

  /* return a pointer to the file's information */

  return (plyfile);
}

/******************************************************************************
Open a polygon file for reading.

Entry:
  input - string to read from

Exit:
  nelems     - number of elements in object
  elem_names - list of element names
  returns a file identifier, used to refer to this file, or nullptr if error
******************************************************************************/

PlyFile* vtkPLY::ply_open_for_reading_from_string(
  const std::string& input, int* nelems, char*** elem_names)
{
  std::istringstream* iss;
  PlyFile* plyfile;

  // memory leaks
  plyInitialize();

  /* open the file for reading */
  iss = new std::istringstream;

  iss->str(input);
  /* create the PlyFile data structure */

  plyfile = vtkPLY::ply_read(iss, nelems, elem_names);
  if (plyfile == nullptr)
  {
    delete iss;
    return (nullptr);
  }

  /* return a pointer to the file's information */

  return (plyfile);
}

/******************************************************************************
Get information about a particular element.

Entry:
  plyfile   - file identifier
  elem_name - name of element to get information about

Exit:
  nelems   - number of elements of this type in the file
  nprops   - number of properties
  returns a list of properties, or nullptr if the file doesn't contain that elem
******************************************************************************/

PlyElement* vtkPLY::ply_get_element_description(
  PlyFile* plyfile, char* elem_name, int* nelems, int* nprops)
{
  // wjs: this class has been drastically changed because of memory leaks.
  // the return type is different, and the element properties are not
  // returned.
  PlyElement* elem;

  /* find information about the element */
  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
    return (nullptr);

  *nelems = elem->num;
  *nprops = elem->nprops;

  return elem;
}

/******************************************************************************
Specify which properties of an element are to be returned.  This should be
called before a call to the routine ply_get_element().

Entry:
  plyfile   - file identifier
  elem_name - which element we're talking about
  nprops    - number of properties
  prop_list - list of properties
******************************************************************************/

void vtkPLY::ply_get_element_setup(
  PlyFile* plyfile, const char* elem_name, int nprops, PlyProperty* prop_list)
{
  int i;
  PlyElement* elem;
  PlyProperty* prop;
  int index;

  /* find information about the element */
  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
    return;
  plyfile->which_elem = elem;

  /* deposit the property information into the element's description */
  for (i = 0; i < nprops; i++)
  {

    /* look for actual property */
    prop = find_property(elem, prop_list[i].name, &index);
    if (prop == nullptr)
    {
      fprintf(stderr, "Warning:  Can't find property '%s' in element '%s'\n", prop_list[i].name,
        elem_name);
      continue;
    }

    /* store its description */
    prop->internal_type = prop_list[i].internal_type;
    prop->offset = prop_list[i].offset;
    prop->count_internal = prop_list[i].count_internal;
    prop->count_offset = prop_list[i].count_offset;

    /* specify that the user wants this property */
    elem->store_prop[index] = STORE_PROP;
  }
}

/******************************************************************************
Specify a property of an element that is to be returned.  This should be
called (usually multiple times) before a call to the routine ply_get_element().
This routine should be used in preference to the less flexible old routine
called ply_get_element_setup().

Entry:
  plyfile   - file identifier
  elem_name - which element we're talking about
  prop      - property to add to those that will be returned
******************************************************************************/

void vtkPLY::ply_get_property(PlyFile* plyfile, const char* elem_name, PlyProperty* prop)
{
  PlyElement* elem;
  PlyProperty* prop_ptr;
  int index;

  /* find information about the element */
  elem = find_element(plyfile, elem_name);
  plyfile->which_elem = elem;

  /* deposit the property information into the element's description */

  prop_ptr = find_property(elem, prop->name, &index);
  if (prop_ptr == nullptr)
  {
    fprintf(stderr, "Warning:  Can't find property '%s' in element '%s'\n", prop->name, elem_name);
    return;
  }
  prop_ptr->internal_type = prop->internal_type;
  prop_ptr->offset = prop->offset;
  prop_ptr->count_internal = prop->count_internal;
  prop_ptr->count_offset = prop->count_offset;

  /* specify that the user wants this property */
  elem->store_prop[index] = STORE_PROP;
}

/******************************************************************************
Read one element from the file.  This routine assumes that we're reading
the type of element specified in the last call to the routine
ply_get_element_setup().

Entry:
  plyfile  - file identifier
  elem_ptr - pointer to location where the element information should be put
******************************************************************************/

void vtkPLY::ply_get_element(PlyFile* plyfile, void* elem_ptr)
{
  if (plyfile->file_type == PLY_ASCII)
    ascii_get_element(plyfile, (char*)elem_ptr);
  else
    binary_get_element(plyfile, (char*)elem_ptr);
}

/******************************************************************************
Extract the comments from the header information of a PLY file.

Entry:
  plyfile - file identifier

Exit:
  num_comments - number of comments returned
  returns a pointer to a list of comments
******************************************************************************/

char** vtkPLY::ply_get_comments(PlyFile* plyfile, int* num_comments)
{
  *num_comments = plyfile->num_comments;
  return (plyfile->comments);
}

/******************************************************************************
Extract the object information (arbitrary text) from the header information
of a PLY file.

Entry:
  plyfile - file identifier

Exit:
  num_obj_info - number of lines of text information returned
  returns a pointer to a list of object info lines
******************************************************************************/

char** vtkPLY::ply_get_obj_info(PlyFile* plyfile, int* num_obj_info)
{
  *num_obj_info = plyfile->num_obj_info;
  return (plyfile->obj_info);
}

/******************************************************************************
Make ready for "other" properties of an element-- those properties that
the user has not explicitly asked for, but that are to be stashed away
in a special structure to be carried along with the element's other
information.

Entry:
  plyfile - file identifier
  elem    - element for which we want to save away other properties
******************************************************************************/

static void setup_other_props(PlyFile*, PlyElement* elem)
{
  int i;
  PlyProperty* prop;
  int size = 0;
  int type_size;

  /* Examine each property in decreasing order of size. */
  /* We do this so that all data types will be aligned by */
  /* word, half-word, or whatever within the structure. */

  for (type_size = 8; type_size > 0; type_size /= 2)
  {

    /* add up the space taken by each property, and save this information */
    /* away in the property descriptor */

    for (i = 0; i < elem->nprops; i++)
    {

      /* don't bother with properties we've been asked to store explicitly */
      if (elem->store_prop[i])
        continue;

      prop = elem->props[i];

      /* internal types will be same as external */
      prop->internal_type = prop->external_type;
      prop->count_internal = prop->count_external;

      /* check list case */
      if (prop->is_list)
      {

        /* pointer to list */
        if (type_size == sizeof(void*))
        {
          prop->offset = size;
          size += sizeof(void*); /* always use size of a pointer here */
        }

        /* count of number of list elements */
        if (type_size == ply_type_size[prop->count_external])
        {
          prop->count_offset = size;
          size += ply_type_size[prop->count_external];
        }
      }
      /* not list */
      else if (type_size == ply_type_size[prop->external_type])
      {
        prop->offset = size;
        size += ply_type_size[prop->external_type];
      }
    }
  }

  /* save the size for the other_props structure */
  elem->other_size = size;
}

/******************************************************************************
Specify that we want the "other" properties of an element to be tucked
away within the user's structure.  The user needn't be concerned for how
these properties are stored.

Entry:
  plyfile   - file identifier
  elem_name - name of element that we want to store other_props in
  offset    - offset to where other_props will be stored inside user's structure

Exit:
  returns pointer to structure containing description of other_props
******************************************************************************/

PlyOtherProp* vtkPLY::ply_get_other_properties(PlyFile* plyfile, const char* elem_name, int offset)
{
  int i;
  PlyElement* elem;
  PlyOtherProp* other;
  PlyProperty* prop;
  int nprops;

  /* find information about the element */
  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
  {
    vtkGenericWarningMacro("ply_get_other_properties: can't find element " << elem_name);
    return (nullptr);
  }

  /* remember that this is the "current" element */
  plyfile->which_elem = elem;

  /* save the offset to where to store the other_props */
  elem->other_offset = offset;

  /* place the appropriate pointers, etc. in the element's property list */
  setup_other_props(plyfile, elem);

  /* create structure for describing other_props */
  other = (PlyOtherProp*)myalloc(sizeof(PlyOtherProp));
  other->name = strdup(elem_name);
#if 0
  if (elem->other_offset == NO_OTHER_PROPS) {
    other->size = 0;
    other->props = nullptr;
    other->nprops = 0;
    return (other);
  }
#endif
  other->size = elem->other_size;
  other->props = (PlyProperty**)myalloc(sizeof(PlyProperty) * elem->nprops);

  /* save descriptions of each "other" property */
  nprops = 0;
  for (i = 0; i < elem->nprops; i++)
  {
    if (elem->store_prop[i])
      continue;
    prop = (PlyProperty*)myalloc(sizeof(PlyProperty));
    copy_property(prop, elem->props[i]);
    other->props[nprops] = prop;
    nprops++;
  }
  other->nprops = nprops;

#if 1
  /* set other_offset pointer appropriately if there are NO other properties */
  if (other->nprops == 0)
  {
    elem->other_offset = NO_OTHER_PROPS;
  }
#endif

  /* return structure */
  return (other);
}

/*************************/
/*  Other Element Stuff  */
/*************************/

/******************************************************************************
Grab all the data for an element that a user does not want to explicitly
read in.

Entry:
  plyfile    - pointer to file
  elem_name  - name of element whose data is to be read in
  elem_count - number of instances of this element stored in the file

Exit:
  returns pointer to ALL the "other" element data for this PLY file
******************************************************************************/

PlyOtherElems* vtkPLY::ply_get_other_element(
  PlyFile* plyfile, const char* elem_name, int elem_count)
{
  int i;
  PlyElement* elem;
  PlyOtherElems* other_elems;
  OtherElem* other;

  /* look for appropriate element */
  elem = find_element(plyfile, elem_name);
  if (elem == nullptr)
  {
    vtkGenericWarningMacro("ply_get_other_element: can't find element " << elem_name);
    return (nullptr);
  }

  /* create room for the new "other" element, initializing the */
  /* other data structure if necessary */

  if (plyfile->other_elems == nullptr)
  {
    plyfile->other_elems = (PlyOtherElems*)myalloc(sizeof(PlyOtherElems));
    other_elems = plyfile->other_elems;
    other_elems->other_list = (OtherElem*)myalloc(sizeof(OtherElem));
    other = &(other_elems->other_list[0]);
    other_elems->num_elems = 1;
  }
  else
  {
    other_elems = plyfile->other_elems;
    other_elems->other_list =
      (OtherElem*)realloc(other_elems->other_list, sizeof(OtherElem) * other_elems->num_elems + 1);
    other = &(other_elems->other_list[other_elems->num_elems]);
    other_elems->num_elems++;
  }

  /* count of element instances in file */
  other->elem_count = elem_count;

  /* save name of element */
  other->elem_name = strdup(elem_name);

  /* create a list to hold all the current elements */
  other->other_data = (OtherData**)malloc(sizeof(OtherData*) * other->elem_count);

  /* set up for getting elements */
  other->other_props = vtkPLY::ply_get_other_properties(
    plyfile, elem_name, static_cast<int>(offsetof(OtherData, other_props)));

  /* grab all these elements */
  for (i = 0; i < other->elem_count; i++)
  {
    /* grab and element from the file */
    other->other_data[i] = (OtherData*)malloc(sizeof(OtherData));
    vtkPLY::ply_get_element(plyfile, (void*)other->other_data[i]);
  }

  /* return pointer to the other elements data */
  return (other_elems);
}

/******************************************************************************
Pass along a pointer to "other" elements that we want to save in a given
PLY file.  These other elements were presumably read from another PLY file.

Entry:
  plyfile     - file pointer in which to store this other element info
  other_elems - info about other elements that we want to store
******************************************************************************/

void vtkPLY::ply_describe_other_elements(PlyFile* plyfile, PlyOtherElems* other_elems)
{
  int i;
  OtherElem* other;

  /* ignore this call if there is no other element */
  if (other_elems == nullptr)
    return;

  /* save pointer to this information */
  plyfile->other_elems = other_elems;

  /* describe the other properties of this element */

  for (i = 0; i < other_elems->num_elems; i++)
  {
    other = &(other_elems->other_list[i]);
    vtkPLY::ply_element_count(plyfile, other->elem_name, other->elem_count);
    vtkPLY::ply_describe_other_properties(
      plyfile, other->other_props, static_cast<int>(offsetof(OtherData, other_props)));
  }
}

/******************************************************************************
Write out the "other" elements specified for this PLY file.

Entry:
  plyfile - pointer to PLY file to write out other elements for
******************************************************************************/

void vtkPLY::ply_put_other_elements(PlyFile* plyfile)
{
  int i, j;
  OtherElem* other;

  /* make sure we have other elements to write */
  if (plyfile->other_elems == nullptr)
    return;

  /* write out the data for each "other" element */

  for (i = 0; i < plyfile->other_elems->num_elems; i++)
  {

    other = &(plyfile->other_elems->other_list[i]);
    vtkPLY::ply_put_element_setup(plyfile, other->elem_name);

    /* write out each instance of the current element */
    for (j = 0; j < other->elem_count; j++)
      vtkPLY::ply_put_element(plyfile, (void*)other->other_data[j]);
  }
}

/******************************************************************************
Free up storage used by an "other" elements data structure.

Entry:
  other_elems - data structure to free up
******************************************************************************/

void vtkPLY::ply_free_other_elements(PlyOtherElems*) {}

/*******************/
/*  Miscellaneous  */
/*******************/

/******************************************************************************
Close a PLY file.

Entry:
  plyfile - identifier of file to close
******************************************************************************/

void vtkPLY::ply_close(PlyFile* plyfile)
{
  // Changed by Will Schroeder. Old stuff leaked like a sieve.

  /* free up memory associated with the PLY file */
  if (plyfile->is)
  {
    vtksys::ifstream* ifs = dynamic_cast<vtksys::ifstream*>(plyfile->is);
    if (ifs)
    {
      ifs->close();
    }
    delete plyfile->is;
  }
  if (plyfile->os)
  {
    vtksys::ofstream* ofs = dynamic_cast<vtksys::ofstream*>(plyfile->os);
    if (ofs)
    {
      ofs->close();
    }
    delete plyfile->os;
  }

  int i, j;
  PlyElement* elem;
  for (i = 0; i < plyfile->nelems; i++)
  {
    elem = plyfile->elems[i];
    free(elem->name);
    for (j = 0; j < elem->nprops; j++)
    {
      free(const_cast<char*>(elem->props[j]->name));
      free(elem->props[j]);
    }
    free(elem->props);
    free(elem->store_prop);
    free(elem);
  }
  free(plyfile->elems);

  for (i = 0; i < plyfile->num_comments; i++)
  {
    free(plyfile->comments[i]);
  }
  free(plyfile->comments);

  for (i = 0; i < plyfile->num_obj_info; i++)
  {
    free(plyfile->obj_info[i]);
  }
  free(plyfile->obj_info);

  free(plyfile);

  // memory leaks
  plyCleanUp();
}

/******************************************************************************
Get version number and file type of a PlyFile.

Entry:
  ply - pointer to PLY file

Exit:
  version - version of the file
  file_type - PLY_ASCII, PLY_BINARY_BE, or PLY_BINARY_LE
******************************************************************************/

void vtkPLY::ply_get_info(PlyFile* ply, float* version, int* file_type)
{
  if (ply == nullptr)
    return;

  *version = ply->version;
  *file_type = ply->file_type;
}

/******************************************************************************
Compare two null-terminated strings.  Returns 1 if they are the same, 0 if not.
******************************************************************************/

bool vtkPLY::equal_strings(const char* s1, const char* s2)
{
  while (*s1 && *s2)
    if (*s1++ != *s2++)
      return false;

  if (*s1 != *s2)
    return false;
  else
    return true;
}

/******************************************************************************
Find an element from the element list of a given PLY object.

Entry:
  plyfile - file id for PLY file
  element - name of element we're looking for

Exit:
  returns the element, or nullptr if not found
******************************************************************************/

PlyElement* vtkPLY::find_element(PlyFile* plyfile, const char* element)
{
  int i;

  for (i = 0; i < plyfile->nelems; i++)
    if (equal_strings(element, plyfile->elems[i]->name))
      return (plyfile->elems[i]);

  return (nullptr);
}

/******************************************************************************
Find a property in the list of properties of a given element.

Entry:
  elem      - pointer to element in which we want to find the property
  prop_name - name of property to find

Exit:
  index - index to position in list
  returns a pointer to the property, or nullptr if not found
******************************************************************************/

PlyProperty* vtkPLY::find_property(PlyElement* elem, const char* prop_name, int* index)
{
  int i;

  for (i = 0; i < elem->nprops; i++)
    if (equal_strings(prop_name, elem->props[i]->name))
    {
      *index = i;
      return (elem->props[i]);
    }

  *index = -1;
  return (nullptr);
}

/******************************************************************************
Read an element from an ascii file.

Entry:
  plyfile  - file identifier
  elem_ptr - pointer to element
******************************************************************************/

bool vtkPLY::ascii_get_element(PlyFile* plyfile, char* elem_ptr)
{
  int j, k;
  PlyElement* elem;
  PlyProperty* prop;
  char** words;
  int nwords;
  int which_word;
  char *elem_data, *item = nullptr;
  char* item_ptr;
  int item_size;
  int int_val = 0;
  unsigned int uint_val = 0;
  double double_val = 0.0;
  int list_count;
  int store_it;
  char** store_array;
  char* orig_line;
  char* other_data = nullptr;
  int other_flag;

  /* the kind of element we're reading currently */
  elem = plyfile->which_elem;

  /* do we need to setup for other_props? */

  if (elem->other_offset != NO_OTHER_PROPS)
  {
    char** ptr;
    other_flag = 1;
    /* make room for other_props */
    other_data = (char*)plyAllocateMemory(elem->other_size);
    /* store pointer in user's structure to the other_props */
    ptr = (char**)(elem_ptr + elem->other_offset);
    *ptr = other_data;
  }
  else
    other_flag = 0;

  /* read in the element */

  words = get_words(plyfile->is, &nwords, &orig_line);
  if (words == nullptr)
  {
    fprintf(stderr, "ply_get_element: unexpected end of file\n");
    assert(0);
    return false;
  }

  which_word = 0;

  for (j = 0; j < elem->nprops; j++)
  {

    prop = elem->props[j];
    store_it = (elem->store_prop[j] | other_flag);

    /* store either in the user's structure or in other_props */
    if (elem->store_prop[j])
      elem_data = elem_ptr;
    else
      elem_data = other_data;

    if (prop->is_list)
    { /* a list */

      /* get and store the number of items in the list */
      get_ascii_item(words[which_word++], prop->count_external, &int_val, &uint_val, &double_val);
      if (store_it)
      {
        item = elem_data + prop->count_offset;
        store_item(item, prop->count_internal, int_val, uint_val, double_val);
      }

      /* allocate space for an array of items and store a ptr to the array */
      list_count = int_val;
      item_size = ply_type_size[prop->internal_type];
      store_array = (char**)(elem_data + prop->offset);

      if (list_count == 0)
      {
        if (store_it)
          *store_array = nullptr;
      }
      else
      {
        if (store_it)
        {
          item_ptr = (char*)myalloc(sizeof(char) * item_size * list_count);
          item = item_ptr;
          *store_array = item_ptr;
        }

        /* read items and store them into the array */
        for (k = 0; k < list_count; k++)
        {
          get_ascii_item(
            words[which_word++], prop->external_type, &int_val, &uint_val, &double_val);
          if (store_it)
          {
            store_item(item, prop->internal_type, int_val, uint_val, double_val);
            item += item_size;
          }
        }
      }
    }
    else
    { /* not a list */
      get_ascii_item(words[which_word++], prop->external_type, &int_val, &uint_val, &double_val);
      if (store_it)
      {
        item = elem_data + prop->offset;
        store_item(item, prop->internal_type, int_val, uint_val, double_val);
      }
    }
  }

  free(words);
  return true;
}

/******************************************************************************
Read an element from a binary file.

Entry:
  plyfile  - file identifier
  elem_ptr - pointer to an element
******************************************************************************/

bool vtkPLY::binary_get_element(PlyFile* plyfile, char* elem_ptr)
{
  int j, k;
  PlyElement* elem;
  PlyProperty* prop;
  char *elem_data, *item = nullptr;
  char* item_ptr;
  int item_size = 0;
  int int_val;
  unsigned int uint_val;
  double double_val;
  int list_count;
  int store_it;
  char** store_array;
  char* other_data = nullptr;
  int other_flag;

  /* the kind of element we're reading currently */
  elem = plyfile->which_elem;

  /* do we need to setup for other_props? */

  if (elem->other_offset != NO_OTHER_PROPS)
  {
    char** ptr;
    other_flag = 1;
    /* make room for other_props */
    other_data = (char*)plyAllocateMemory(elem->other_size);
    /* store pointer in user's structure to the other_props */
    ptr = (char**)(elem_ptr + elem->other_offset);
    *ptr = other_data;
  }
  else
    other_flag = 0;

  /* read in a number of elements */

  for (j = 0; j < elem->nprops; j++)
  {

    prop = elem->props[j];
    store_it = (elem->store_prop[j] | other_flag);

    /* store either in the user's structure or in other_props */
    if (elem->store_prop[j])
      elem_data = elem_ptr;
    else
      elem_data = other_data;

    if (prop->is_list)
    { /* a list */

      /* get and store the number of items in the list */
      if (!get_binary_item(plyfile, prop->count_external, &int_val, &uint_val, &double_val))
      {
        return false;
      }
      if (store_it)
      {
        item = elem_data + prop->count_offset;
        store_item(item, prop->count_internal, int_val, uint_val, double_val);
      }

      /* allocate space for an array of items and store a ptr to the array */
      list_count = int_val;
      /* The "if" was added by Afra Zomorodian 8/22/95
       * so that zipper won't crash reading plies that have additional
       * properties.
       */
      if (store_it)
      {
        item_size = ply_type_size[prop->internal_type];
      }
      store_array = (char**)(elem_data + prop->offset);
      if (list_count == 0)
      {
        if (store_it)
          *store_array = nullptr;
      }
      else
      {
        if (store_it)
        {
          item_ptr = (char*)myalloc(sizeof(char) * item_size * list_count);
          item = item_ptr;
          *store_array = item_ptr;
        }

        /* read items and store them into the array */
        for (k = 0; k < list_count; k++)
        {
          if (!get_binary_item(plyfile, prop->external_type, &int_val, &uint_val, &double_val))
          {
            return false;
          }
          if (store_it)
          {
            store_item(item, prop->internal_type, int_val, uint_val, double_val);
            item += item_size;
          }
        }
      }
    }
    else
    { /* not a list */
      if (!get_binary_item(plyfile, prop->external_type, &int_val, &uint_val, &double_val))
      {
        return false;
      }
      if (store_it)
      {
        item = elem_data + prop->offset;
        store_item(item, prop->internal_type, int_val, uint_val, double_val);
      }
    }
  }
  return true;
}

/******************************************************************************
Write to a file the word that represents a PLY data type.

Entry:
  os   - output stream
  code - code for type
******************************************************************************/

void vtkPLY::write_scalar_type(std::ostream* os, int code)
{
  /* make sure this is a valid code */

  if (code <= PLY_START_TYPE || code >= PLY_END_TYPE)
  {
    fprintf(stderr, "write_scalar_type: bad data code = %d\n", code);
    assert(0);
  }

  /* write the code to a file */

  *os << type_names[code];
}

/******************************************************************************
Get a text line from a file and break it up into words.

IMPORTANT: The calling routine call "free" on the returned pointer once
finished with it.

Entry:
  is - input stream

Exit:
  nwords    - number of words returned
  orig_line - the original line of characters
  returns a list of words from the line, or nullptr if end-of-file
******************************************************************************/

char** vtkPLY::get_words(std::istream* is, int* nwords, char** orig_line)
{
  const int BIG_STRING = 4096;
  char str[BIG_STRING];
  char str_copy[BIG_STRING];
  char** words;
  int max_words = 10;
  int num_words = 0;
  char *ptr, *ptr2;

  /* read in a line */
  is->getline(str, BIG_STRING);
  if (!is->good())
  {
    *nwords = 0;
    *orig_line = nullptr;
    return (nullptr);
  }
  words = (char**)myalloc(sizeof(char*) * max_words);

  char* pos = strstr(str, "vertex_index");
  if (pos != nullptr)
  {
    strcpy(pos, "vertex_indices");
  }

  /* convert line-feed and tabs into spaces */
  /* (this guarantees that there will be a space before the */
  /*  null character at the end of the string) */

  str[BIG_STRING - 2] = ' ';
  str[BIG_STRING - 1] = '\0';

  for (ptr = str, ptr2 = str_copy; *ptr != '\0'; ptr++, ptr2++)
  {
    *ptr2 = *ptr;
    if (*ptr == '\t')
    {
      *ptr = ' ';
      *ptr2 = ' ';
    }
    else if (*ptr == '\r')
    {
      *ptr = ' ';
      *ptr2 = '\0';
    }
    else if (*ptr == '\n')
    {
      *ptr = ' ';
      *ptr2 = '\0';
      break;
    }
  }

  /* Terminate the copy with null character */
  /* so that the returned orig_line is terminated*/
  *ptr2 = '\0';

  /* find the words in the line */

  ptr = str;
  while (*ptr != '\0')
  {

    /* jump over leading spaces */
    while (*ptr == ' ')
      ptr++;

    /* break if we reach the end */
    if (*ptr == '\0')
      break;

    /* save pointer to beginning of word */
    if (num_words >= max_words)
    {
      max_words += 10;
      char** oldwords = words;
      words = (char**)realloc(words, sizeof(char*) * max_words);
      if (!words)
      {
        *nwords = 0;
        *orig_line = nullptr;
        free(oldwords);
        return nullptr;
      }
    }
    words[num_words++] = ptr;

    /* jump over non-spaces */
    while (*ptr != ' ' && *ptr != '\0')
      ptr++;

    /* break if we reach the end */
    if (*ptr == '\0')
      break;

    /* place a null character here to mark the end of the word */
    *ptr++ = '\0';
  }

  /* return the list of words */
  *nwords = num_words;
  *orig_line = str_copy;
  return (words);
}

/******************************************************************************
Return the value of an item, given a pointer to it and its type.

Entry:
  item - pointer to item
  type - data type that "item" points to

Exit:
  returns a double-precision float that contains the value of the item
******************************************************************************/

double vtkPLY::get_item_value(const char* item, int type)
{
  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
    {
      vtkTypeInt8 value;
      memcpy(&value, item, sizeof(value));
      return ((double)value);
    }
    case PLY_UCHAR:
    case PLY_UINT8:
    {
      vtkTypeUInt8 value;
      memcpy(&value, item, sizeof(value));
      return ((double)value);
    }
    case PLY_SHORT:
    case PLY_INT16:
    {
      vtkTypeInt16 value;
      memcpy(&value, item, sizeof(value));
      return ((double)value);
    }
    case PLY_USHORT:
    case PLY_UINT16:
    {
      vtkTypeUInt16 value;
      memcpy(&value, item, sizeof(value));
      return ((double)value);
    }
    case PLY_INT:
    case PLY_INT32:
    {
      vtkTypeInt32 value;
      memcpy(&value, item, sizeof(value));
      return ((double)value);
    }
    case PLY_UINT:
    case PLY_UINT32:
    {
      vtkTypeUInt32 value;
      memcpy(&value, item, sizeof(value));
      return ((double)value);
    }
    case PLY_FLOAT:
    case PLY_FLOAT32:
    {
      vtkTypeFloat32 value;
      memcpy(&value, item, sizeof(value));
      return ((double)value);
    }
    case PLY_DOUBLE:
    case PLY_FLOAT64:
    {
      vtkTypeFloat64 value;
      memcpy(&value, item, sizeof(value));
      return (value);
    }
  }
  fprintf(stderr, "get_item_value: bad type = %d\n", type);
  return 0;
}

/******************************************************************************
Write out an item to a file as raw binary bytes.

Entry:
  os         - output stream
  int_val    - integer version of item
  uint_val   - unsigned integer version of item
  double_val - double-precision float version of item
  type       - data type to write out
******************************************************************************/

void vtkPLY::write_binary_item(
  PlyFile* plyfile, int int_val, unsigned int uint_val, double double_val, int type)
{
  std::ostream* os = plyfile->os;
  vtkTypeUInt8 uchar_val;
  vtkTypeInt8 char_val;
  vtkTypeUInt16 ushort_val;
  vtkTypeInt16 short_val;
  vtkTypeFloat32 float_val;

  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
      char_val = int_val;
      os->write(reinterpret_cast<char*>(&char_val), sizeof(char_val));
      break;
    case PLY_SHORT:
    case PLY_INT16:
      short_val = int_val;
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap2BE(&short_val)
                                          : vtkByteSwap::Swap2LE(&short_val);
      os->write(reinterpret_cast<char*>(&short_val), sizeof(short_val));
      break;
    case PLY_INT:
    case PLY_INT32:
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap4BE(&int_val)
                                          : vtkByteSwap::Swap4LE(&int_val);
      os->write(reinterpret_cast<char*>(&int_val), sizeof(int_val));
      break;
    case PLY_UCHAR:
    case PLY_UINT8:
      uchar_val = uint_val;
      os->write(reinterpret_cast<char*>(&uchar_val), sizeof(uchar_val));
      break;
    case PLY_USHORT:
    case PLY_UINT16:
      ushort_val = uint_val;
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap2BE(&ushort_val)
                                          : vtkByteSwap::Swap2LE(&ushort_val);
      os->write(reinterpret_cast<char*>(&ushort_val), sizeof(ushort_val));
      break;
    case PLY_UINT:
    case PLY_UINT32:
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap4BE(&uint_val)
                                          : vtkByteSwap::Swap4LE(&uint_val);
      os->write(reinterpret_cast<char*>(&uint_val), sizeof(uint_val));
      break;
    case PLY_FLOAT:
    case PLY_FLOAT32:
      float_val = double_val;
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap4BE(&float_val)
                                          : vtkByteSwap::Swap4LE(&float_val);
      os->write(reinterpret_cast<char*>(&float_val), sizeof(float_val));
      break;
    case PLY_DOUBLE:
    case PLY_FLOAT64:
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap8BE(&double_val)
                                          : vtkByteSwap::Swap8LE(&double_val);
      os->write(reinterpret_cast<char*>(&double_val), sizeof(double_val));
      break;
    default:
      fprintf(stderr, "write_binary_item: bad type = %d\n", type);
      assert(0);
  }
}

/******************************************************************************
Write out an item to a file as ascii characters.

Entry:
  os         - output stream
  int_val    - integer version of item
  uint_val   - unsigned integer version of item
  double_val - double-precision float version of item
  type       - data type to write out
******************************************************************************/

void vtkPLY::write_ascii_item(
  std::ostream* os, int int_val, unsigned int uint_val, double double_val, int type)
{
  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
    case PLY_SHORT:
    case PLY_INT16:
    case PLY_INT:
    case PLY_INT32:
      *os << int_val << " ";
      break;
    case PLY_UCHAR:
    case PLY_UINT8:
    case PLY_USHORT:
    case PLY_UINT16:
    case PLY_UINT:
    case PLY_UINT32:
      *os << uint_val << " ";
      break;
    case PLY_FLOAT:
    case PLY_FLOAT32:
    case PLY_DOUBLE:
    case PLY_FLOAT64:
      // Use needed precision.
      *os << std::setprecision(std::numeric_limits<double>::max_digits10) << double_val << " ";
      break;
    default:
      fprintf(stderr, "write_ascii_item: bad type = %d\n", type);
      assert(0);
  }
}

/******************************************************************************
Write out an item to a file as ascii characters.

Entry:
  os   - output stream
  item - pointer to item to write
  type - data type that "item" points to

Exit:
  returns a double-precision float that contains the value of the written item
******************************************************************************/

double vtkPLY::old_write_ascii_item(std::ostream* os, char* item, int type)
{
  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
    {
      vtkTypeInt8 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return static_cast<double>(value);
    }
    case PLY_UCHAR:
    case PLY_UINT8:
    {
      vtkTypeUInt8 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return static_cast<double>(value);
    }
    case PLY_SHORT:
    case PLY_INT16:
    {
      vtkTypeInt16 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return static_cast<double>(value);
    }
    case PLY_USHORT:
    case PLY_UINT16:
    {
      vtkTypeUInt16 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return static_cast<double>(value);
    }
    case PLY_INT:
    case PLY_INT32:
    {
      vtkTypeInt32 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return static_cast<double>(value);
    }
    case PLY_UINT:
    case PLY_UINT32:
    {
      vtkTypeUInt32 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return static_cast<double>(value);
    }
    case PLY_FLOAT:
    case PLY_FLOAT32:
    {
      vtkTypeFloat32 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return value;
    }
    case PLY_DOUBLE:
    case PLY_FLOAT64:
    {
      vtkTypeFloat64 value;
      memcpy(&value, item, sizeof(value));
      *os << value << " ";
      return value;
    }
  }
  fprintf(stderr, "old_write_ascii_item: bad type = %d\n", type);
  return 0.0;
}

/******************************************************************************
Get the value of an item that is in memory, and place the result
into an integer, an unsigned integer and a double.

Entry:
  ptr  - pointer to the item
  type - data type supposedly in the item

Exit:
  int_val    - integer value
  uint_val   - unsigned integer value
  double_val - double-precision floating point value
******************************************************************************/

void vtkPLY::get_stored_item(
  const void* ptr, int type, int* int_val, unsigned int* uint_val, double* double_val)
{
  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
    {
      vtkTypeInt8 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    case PLY_UCHAR:
    case PLY_UINT8:
    {
      vtkTypeUInt8 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    case PLY_SHORT:
    case PLY_INT16:
    {
      vtkTypeInt16 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    case PLY_USHORT:
    case PLY_UINT16:
    {
      vtkTypeUInt16 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    case PLY_INT:
    case PLY_INT32:
    {
      vtkTypeInt32 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    case PLY_UINT:
    case PLY_UINT32:
    {
      vtkTypeUInt32 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    case PLY_FLOAT:
    case PLY_FLOAT32:
    {
      vtkTypeFloat32 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    case PLY_DOUBLE:
    case PLY_FLOAT64:
    {
      vtkTypeFloat64 value;
      memcpy(&value, ptr, sizeof(value));
      *uint_val = static_cast<unsigned int>(value);
      *int_val = static_cast<int>(value);
      *double_val = static_cast<double>(value);
      break;
    }
    default:
      fprintf(stderr, "get_stored_item: bad type = %d\n", type);
      assert(0);
  }
}

/******************************************************************************
Get the value of an item from a binary file, and place the result
into an integer, an unsigned integer and a double.

Entry:
  os   - output stream
  type - data type supposedly in the word

Exit:
  int_val    - integer value
  uint_val   - unsigned integer value
  double_val - double-precision floating point value
******************************************************************************/

bool vtkPLY::get_binary_item(
  PlyFile* plyfile, int type, int* int_val, unsigned int* uint_val, double* double_val)
{
  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
    {
      vtkTypeInt8 value = 0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading char.");
        return false;
      }

      // Here value can always fit in int, unsigned int, and double.
      *int_val = static_cast<int>(value);
      *uint_val = static_cast<unsigned int>(value);
      *double_val = static_cast<double>(value);
    }
    break;
    case PLY_UCHAR:
    case PLY_UINT8:
    {
      vtkTypeUInt8 value = 0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading uchar or uint8.");
        return false;
      }

      // Here value can always fit in int, unsigned int, and double.
      *int_val = static_cast<int>(value);
      *uint_val = static_cast<unsigned int>(value);
      *double_val = static_cast<double>(value);
    }
    break;
    case PLY_SHORT:
    case PLY_INT16:
    {
      vtkTypeInt16 value = 0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading short.");
        return false;
      }
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap2BE(&value)
                                          : vtkByteSwap::Swap2LE(&value);

      // Here value can always fit in int, unsigned int, and double.
      *int_val = static_cast<int>(value);
      *uint_val = static_cast<unsigned int>(value);
      *double_val = static_cast<double>(value);
    }
    break;
    case PLY_USHORT:
    case PLY_UINT16:
    {
      vtkTypeUInt16 value = 0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading ushort.");
        return false;
      }
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap2BE(&value)
                                          : vtkByteSwap::Swap2LE(&value);

      // Here value can always fit in int, unsigned int, and double.
      *int_val = static_cast<int>(value);
      *uint_val = static_cast<unsigned int>(value);
      *double_val = static_cast<double>(value);
    }
    break;
    case PLY_INT:
    case PLY_INT32:
    {
      vtkTypeInt32 value = 0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading int or int32.");
        return false;
      }
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap4BE(&value)
                                          : vtkByteSwap::Swap4LE(&value);

      // Here value can always fit in int, unsigned int, and double.
      *int_val = static_cast<int>(value);
      *uint_val = static_cast<unsigned int>(value);
      *double_val = static_cast<double>(value);
    }
    break;
    case PLY_UINT:
    case PLY_UINT32:
    {
      vtkTypeUInt32 value = 0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading uint");
        return false;
      }
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap4BE(&value)
                                          : vtkByteSwap::Swap4LE(&value);

      // Here value can always fit in int, unsigned int, and double.
      *int_val = static_cast<int>(value);
      *uint_val = static_cast<unsigned int>(value);
      *double_val = static_cast<double>(value);
    }
    break;
    case PLY_FLOAT:
    case PLY_FLOAT32:
    {
      vtkTypeFloat32 value = 0.0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading float of float32.");
        return false;
      }
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap4BE(&value)
                                          : vtkByteSwap::Swap4LE(&value);

      // INT32_MIN (-2^31) is a power of 2 and thus exactly representable as float.
      // INT32_MAX (2^31 - 1) is not exactly representable as float; closest smaller integer is 2^31
      // - 128. UINT32_MAX (2^32 - 1) is not exactly representable as float; closest smaller integer
      // is 2^32 - 256.
      *int_val = static_cast<int>(vtkMath::ClampValue(value, (float)VTK_INT_MIN, 2147483520.0f));
      *uint_val = static_cast<unsigned int>(vtkMath::ClampValue(value, 0.0f, 4294967040.0f));
      *double_val = static_cast<double>(value);
    }
    break;
    case PLY_DOUBLE:
    case PLY_FLOAT64:
    {
      vtkTypeFloat64 value = 0.0;
      plyfile->is->read(reinterpret_cast<char*>(&value), sizeof(value));
      if (!plyfile->is->good())
      {
        vtkGenericWarningMacro("PLY error reading file."
          << " Premature EOF while reading double.");
        return false;
      }
      plyfile->file_type == PLY_BINARY_BE ? vtkByteSwap::Swap8BE(&value)
                                          : vtkByteSwap::Swap8LE(&value);

      // Here we can just clamp and cast, all int32s can be exactly represented as doubles.
      *int_val =
        static_cast<int>(vtkMath::ClampValue(value, (double)VTK_INT_MIN, (double)VTK_INT_MAX));
      *uint_val =
        static_cast<unsigned int>(vtkMath::ClampValue(value, 0.0, (double)VTK_UNSIGNED_INT_MAX));
      *double_val = value;
    }
    break;
    default:
      fprintf(stderr, "get_binary_item: bad type = %d\n", type);
      assert(0);
      return false;
  }
  return true;
}

/******************************************************************************
Extract the value of an item from an ascii word, and place the result
into an integer, an unsigned integer and a double.

Entry:
  word - word to extract value from
  type - data type supposedly in the word

Exit:
  int_val    - integer value
  uint_val   - unsigned integer value
  double_val - double-precision floating point value
******************************************************************************/

void vtkPLY::get_ascii_item(
  const char* word, int type, int* int_val, unsigned int* uint_val, double* double_val)
{
  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
    case PLY_UCHAR:
    case PLY_UINT8:
    case PLY_SHORT:
    case PLY_INT16:
    case PLY_USHORT:
    case PLY_UINT16:
    case PLY_INT:
    case PLY_INT32:
      *int_val = atoi(word);
      *uint_val = *int_val;
      *double_val = *int_val;
      break;

    case PLY_UINT:
    case PLY_UINT32:
      *uint_val = strtoul(word, nullptr, 10);
      *int_val = *uint_val;
      *double_val = *uint_val;
      break;

    case PLY_FLOAT:
    case PLY_FLOAT32:
    case PLY_DOUBLE:
    case PLY_FLOAT64:
      *double_val = atof(word);
      *int_val = (int)*double_val;
      *uint_val = (unsigned int)*double_val;
      break;

    default:
      fprintf(stderr, "get_ascii_item: bad type = %d\n", type);
      assert(0);
  }
}

/******************************************************************************
Store a value into a place being pointed to, guided by a data type.

Entry:
  item       - place to store value
  type       - data type
  int_val    - integer version of value
  uint_val   - unsigned integer version of value
  double_val - double version of value

Exit:
  item - pointer to stored value
******************************************************************************/

void vtkPLY::store_item(char* item, int type, int int_val, unsigned int uint_val, double double_val)
{
  switch (type)
  {
    case PLY_CHAR:
    case PLY_INT8:
    {
      vtkTypeInt8 value = static_cast<vtkTypeInt8>(int_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    case PLY_UCHAR:
    case PLY_UINT8:
    {
      vtkTypeUInt8 value = static_cast<vtkTypeUInt8>(uint_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    case PLY_SHORT:
    case PLY_INT16:
    {
      vtkTypeInt16 value = static_cast<vtkTypeInt16>(int_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    case PLY_USHORT:
    case PLY_UINT16:
    {
      vtkTypeUInt16 value = static_cast<vtkTypeUInt16>(uint_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    case PLY_INT:
    case PLY_INT32:
    {
      vtkTypeInt32 value = static_cast<vtkTypeInt32>(int_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    case PLY_UINT:
    case PLY_UINT32:
    {
      vtkTypeUInt32 value = static_cast<vtkTypeUInt32>(uint_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    case PLY_FLOAT:
    case PLY_FLOAT32:
    {
      vtkTypeFloat32 value = static_cast<vtkTypeFloat32>(double_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    case PLY_DOUBLE:
    case PLY_FLOAT64:
    {
      vtkTypeFloat64 value = static_cast<vtkTypeFloat64>(double_val);
      memcpy(item, &value, sizeof(value));
      break;
    }
    default:
      fprintf(stderr, "store_item: bad type = %d\n", type);
      assert(0);
  }
}

/******************************************************************************
Add an element to a PLY file descriptor.

Entry:
  plyfile - PLY file descriptor
  words   - list of words describing the element
  nwords  - number of words in the list
******************************************************************************/

void vtkPLY::add_element(PlyFile* plyfile, char** words, int)
{
  PlyElement* elem;

  /* create the new element */
  elem = (PlyElement*)myalloc(sizeof(PlyElement));
  elem->name = strdup(words[1]);
  elem->num = atoi(words[2]);
  elem->nprops = 0;

  /* make room for new element in the object's list of elements */
  if (plyfile->nelems == 0)
    plyfile->elems = (PlyElement**)myalloc(sizeof(PlyElement*));
  else
    plyfile->elems =
      (PlyElement**)realloc(plyfile->elems, sizeof(PlyElement*) * (plyfile->nelems + 1));

  /* add the new element to the object's list */
  plyfile->elems[plyfile->nelems] = elem;
  plyfile->nelems++;
}

/******************************************************************************
Return the type of a property, given the name of the property.

Entry:
  name - name of property type

Exit:
  returns integer code for property, or 0 if not found
******************************************************************************/

int vtkPLY::get_prop_type(const char* type_name)
{
  int i;

  for (i = PLY_START_TYPE + 1; i < PLY_END_TYPE; i++)
    if (equal_strings(type_name, type_names[i]))
      return (i);

  /* if we get here, we didn't find the type */
  return (0);
}

/******************************************************************************
Add a property to a PLY file descriptor.

Entry:
  plyfile - PLY file descriptor
  words   - list of words describing the property
  nwords  - number of words in the list
******************************************************************************/

void vtkPLY::add_property(PlyFile* plyfile, char** words, int)
{
  PlyProperty* prop;
  PlyElement* elem;

  /* create the new property */

  prop = (PlyProperty*)myalloc(sizeof(PlyProperty));

  if (equal_strings(words[1], "list"))
  { /* is a list */
    prop->count_external = get_prop_type(words[2]);
    prop->external_type = get_prop_type(words[3]);
    prop->name = strdup(words[4]);
    prop->is_list = 1;
  }
  else
  { /* not a list */
    prop->external_type = get_prop_type(words[1]);
    prop->name = strdup(words[2]);
    prop->is_list = 0;
  }

  /* add this property to the list of properties of the current element */

  elem = plyfile->elems[plyfile->nelems - 1];

  if (elem->nprops == 0)
    elem->props = (PlyProperty**)myalloc(sizeof(PlyProperty*));
  else
    elem->props = (PlyProperty**)realloc(elem->props, sizeof(PlyProperty*) * (elem->nprops + 1));

  elem->props[elem->nprops] = prop;
  elem->nprops++;
}

/******************************************************************************
Add a comment to a PLY file descriptor.

Entry:
  plyfile - PLY file descriptor
  line    - line containing comment
******************************************************************************/

void vtkPLY::add_comment(PlyFile* plyfile, char* line)
{
  int i;

  /* skip over "comment" and leading spaces and tabs */
  i = 7;
  while (line[i] == ' ' || line[i] == '\t')
    i++;

  vtkPLY::ply_put_comment(plyfile, &line[i]);
}

/******************************************************************************
Add a some object information to a PLY file descriptor.

Entry:
  plyfile - PLY file descriptor
  line    - line containing text info
******************************************************************************/

void vtkPLY::add_obj_info(PlyFile* plyfile, char* line)
{
  int i;

  /* skip over "obj_info" and leading spaces and tabs */
  i = 8;
  while (line[i] == ' ' || line[i] == '\t')
    i++;

  vtkPLY::ply_put_obj_info(plyfile, &line[i]);
}

/******************************************************************************
Copy a property.
******************************************************************************/

void vtkPLY::copy_property(PlyProperty* dest, const PlyProperty* src)
{
  dest->name = strdup(src->name);
  dest->external_type = src->external_type;
  dest->internal_type = src->internal_type;
  dest->offset = src->offset;

  dest->is_list = src->is_list;
  dest->count_external = src->count_external;
  dest->count_internal = src->count_internal;
  dest->count_offset = src->count_offset;
}

/******************************************************************************
Allocate some memory.

Entry:
  size  - amount of memory requested (in bytes)
  lnum  - line number from which memory was requested
  fname - file name from which memory was requested
******************************************************************************/

void* vtkPLY::my_alloc(size_t size, int lnum, const char* fname)
{
  void* ptr = malloc(size);

  if (ptr == nullptr)
  {
    fprintf(stderr, "Memory allocation bombed on line %d in %s\n", lnum, fname);
  }

  return (ptr);
}
