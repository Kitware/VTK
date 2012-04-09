/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLY.h

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

// .NAME vtkPLY - a modified version of the PLY 1.1 library
// .SECTION Description
// vtkPLY is a modified version of the PLY 1.1 library. The library
// has been modified by wrapping in a class (to minimize global symbols);
// to take advantage of functionality generally not available through the
// PLY library API; and to correct problems with the PLY library.
//
// The original distribution was taken from the Stanford University PLY
// file format release 1.1 (see http://graphics.stanford.edu/data/3Dscanrep/).

// .SECTION See Also
// vtkPLYWriter vtkPLYReader

#ifndef __vtkPLY_h
#define __vtkPLY_h

#include "vtkIOPLYModule.h" // For export macro
#include "vtkObject.h"

#define PLY_ASCII      1        /* ascii PLY file */
#define PLY_BINARY_BE  2        /* binary PLY file, big endian */
#define PLY_BINARY_LE  3        /* binary PLY file, little endian */

#define PLY_OKAY    0           /* ply routine worked okay */
#define PLY_ERROR  -1           /* error in ply routine */

/* scalar data types supported by PLY format */

#define PLY_START_TYPE 0
#define PLY_CHAR       1
#define PLY_SHORT      2
#define PLY_INT        3
#define PLY_INT32      4
#define PLY_UCHAR      5
#define PLY_USHORT     6
#define PLY_UINT       7
#define PLY_UINT8      8
#define PLY_FLOAT      9
#define PLY_FLOAT32    10
#define PLY_DOUBLE     11
#define PLY_END_TYPE   12

#define  PLY_SCALAR  0
#define  PLY_LIST    1

typedef struct PlyProperty {    /* description of a property */

  const char *name;                           /* property name */
  int external_type;                    /* file's data type */
  int internal_type;                    /* program's data type */
  int offset;                           /* offset bytes of prop in a struct */

  int is_list;                          /* 1 = list, 0 = scalar */
  int count_external;                   /* file's count type */
  int count_internal;                   /* program's count type */
  int count_offset;                     /* offset byte for list count */

} PlyProperty;

typedef struct PlyElement {     /* description of an element */
  char *name;                   /* element name */
  int num;                      /* number of elements in this object */
  int size;                     /* size of element (bytes) or -1 if variable */
  int nprops;                   /* number of properties for this element */
  PlyProperty **props;          /* list of properties in the file */
  char *store_prop;             /* flags: property wanted by user? */
  int other_offset;             /* offset to un-asked-for props, or -1 if none*/
  int other_size;               /* size of other_props structure */
} PlyElement;

typedef struct PlyOtherProp {   /* describes other properties in an element */
  char *name;                   /* element name */
  int size;                     /* size of other_props */
  int nprops;                   /* number of properties in other_props */
  PlyProperty **props;          /* list of properties in other_props */
} PlyOtherProp;

typedef struct OtherData { /* for storing other_props for an other element */
  void *other_props;
} OtherData;

typedef struct OtherElem {     /* data for one "other" element */
  char *elem_name;             /* names of other elements */
  int elem_count;              /* count of instances of each element */
  OtherData **other_data;      /* actual property data for the elements */
  PlyOtherProp *other_props;   /* description of the property data */
} OtherElem;

typedef struct PlyOtherElems {  /* "other" elements, not interpreted by user */
  int num_elems;                /* number of other elements */
  OtherElem *other_list;        /* list of data for other elements */
} PlyOtherElems;

typedef struct PlyFile {        /* description of PLY file */
  FILE *fp;                     /* file pointer */
  int file_type;                /* ascii or binary */
  float version;                /* version number of file */
  int nelems;                   /* number of elements of object */
  PlyElement **elems;           /* list of elements */
  int num_comments;             /* number of comments */
  char **comments;              /* list of comments */
  int num_obj_info;             /* number of items of object information */
  char **obj_info;              /* list of object info items */
  PlyElement *which_elem;       /* which element we're currently writing */
  PlyOtherElems *other_elems;   /* "other" elements from a PLY file */
} PlyFile;

class VTKIOPLY_EXPORT vtkPLY
{
public:
  //standard PLY library interface
  static PlyFile *ply_write(FILE *, int, const char **, int);
  static PlyFile *ply_open_for_writing(const char *, int, const char **, int, float *);
  static void ply_describe_element(PlyFile *, const char *, int, int, PlyProperty *);
  static void ply_describe_property(PlyFile *, const char *, PlyProperty *);
  static void ply_element_count(PlyFile *, const char *, int);
  static void ply_header_complete(PlyFile *);
  static void ply_put_element_setup(PlyFile *, const char *);
  static void ply_put_element(PlyFile *, void *);
  static void ply_put_comment(PlyFile *, const char *);
  static void ply_put_obj_info(PlyFile *, const char *);
  static PlyFile *ply_read(FILE *, int *, char ***);
  static PlyFile *ply_open_for_reading( const char *, int *, char ***, int *, float *);
  static PlyElement *ply_get_element_description(PlyFile *, char *, int*, int*);
  static void ply_get_element_setup( PlyFile *, const char *, int, PlyProperty *);
  static void ply_get_property(PlyFile *, const char *, PlyProperty *);
  static PlyOtherProp *ply_get_other_properties(PlyFile *, const char *, int);
  static void ply_get_element(PlyFile *, void *);
  static char **ply_get_comments(PlyFile *, int *);
  static char **ply_get_obj_info(PlyFile *, int *);
  static void ply_close(PlyFile *);
  static void ply_get_info(PlyFile *, float *, int *);
  static PlyOtherElems *ply_get_other_element (PlyFile *, const char *, int);
  static void ply_describe_other_elements ( PlyFile *, PlyOtherElems *);
  static void ply_put_other_elements (PlyFile *);
  static void ply_free_other_elements (PlyOtherElems *);
  static void ply_describe_other_properties(PlyFile *, PlyOtherProp *, int);

  // These methods are internal to the PLY library in the normal distribution
  // They should be used carefully
  static bool equal_strings(const char *, const char *);
  static PlyElement *find_element(PlyFile *, const char *);
  static PlyProperty *find_property(PlyElement *, const char *, int *);
  static void write_scalar_type (FILE *, int);
  static char **get_words(FILE *, int *, char **);
  static char **old_get_words(FILE *, int *);
  static void write_binary_item(PlyFile *, int, unsigned int, double, int);
  static void write_ascii_item(FILE *, int, unsigned int, double, int);
  static double old_write_ascii_item(FILE *, char *, int);
  static void add_element(PlyFile *, char **, int);
  static void add_property(PlyFile *, char **, int);
  static void add_comment(PlyFile *, char *);
  static void add_obj_info(PlyFile *, char *);
  static void copy_property(PlyProperty *, const PlyProperty *);
  static void store_item(char *, int, int, unsigned int, double);
  static void get_stored_item( const void *, int, int *, unsigned int *, double *);
  static double get_item_value(const char *, int);
  static void get_ascii_item(const char *, int, int *, unsigned int *, double *);
  static void get_binary_item(PlyFile *, int, int *, unsigned int *, double *);
  static void ascii_get_element(PlyFile *, char *);
  static void binary_get_element(PlyFile *, char *);
  static char *my_alloc(size_t, int, const char *);
  static int get_prop_type(const char *);

};

#endif


// VTK-HeaderTest-Exclude: vtkPLY.h
