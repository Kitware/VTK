/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLY.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Mike Dresser MD/PhD
             Director of Core Facility for Imaging
             Program in Molecular and Cell Biology
             Oklahoma Medical Research Foundation


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include <stdio.h>
#include <stddef.h>
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
#define PLY_UCHAR      4
#define PLY_USHORT     5
#define PLY_UINT       6
#define PLY_FLOAT      7
#define PLY_DOUBLE     8
#define PLY_END_TYPE   9

#define  PLY_SCALAR  0
#define  PLY_LIST    1

typedef struct PlyProperty {    /* description of a property */

  char *name;                           /* property name */
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

/* memory allocation */
#define myalloc(mem_size) vtkPLY::my_alloc((mem_size), __LINE__, __FILE__)

class VTK_IO_EXPORT vtkPLY
{
public:
  //standard PLY library interface
  static PlyFile *ply_write(FILE *, int, char **, int);
  static PlyFile *ply_open_for_writing(char *, int, char **, int, float *);
  static void ply_describe_element(PlyFile *, char *, int, int, PlyProperty *);
  static void ply_describe_property(PlyFile *, char *, PlyProperty *);
  static void ply_element_count(PlyFile *, char *, int);
  static void ply_header_complete(PlyFile *);
  static void ply_put_element_setup(PlyFile *, char *);
  static void ply_put_element(PlyFile *, void *);
  static void ply_put_comment(PlyFile *, char *);
  static void ply_put_obj_info(PlyFile *, char *);
  static PlyFile *ply_read(FILE *, int *, char ***);
  static PlyFile *ply_open_for_reading( char *, int *, char ***, int *, float *);
  static PlyElement *ply_get_element_description(PlyFile *, char *, int*, int*);
  static void ply_get_element_setup( PlyFile *, char *, int, PlyProperty *);
  static void ply_get_property(PlyFile *, char *, PlyProperty *);
  static PlyOtherProp *ply_get_other_properties(PlyFile *, char *, int);
  static void ply_get_element(PlyFile *, void *);
  static char **ply_get_comments(PlyFile *, int *);
  static char **ply_get_obj_info(PlyFile *, int *);
  static void ply_close(PlyFile *);
  static void ply_get_info(PlyFile *, float *, int *);
  static PlyOtherElems *ply_get_other_element (PlyFile *, char *, int);
  static void ply_describe_other_elements ( PlyFile *, PlyOtherElems *);
  static void ply_put_other_elements (PlyFile *);
  static void ply_free_other_elements (PlyOtherElems *);
  static void ply_describe_other_properties(PlyFile *, PlyOtherProp *, int);

  // These methods are internal to the PLY library in the normal distribution
  // They should be used carefully
  static int equal_strings(char *, char *);
  static PlyElement *find_element(PlyFile *, char *);
  static PlyProperty *find_property(PlyElement *, char *, int *);
  static void write_scalar_type (FILE *, int);
  static char **get_words(FILE *, int *, char **);
  static char **old_get_words(FILE *, int *);
  static void write_binary_item(FILE *, int, unsigned int, double, int);
  static void write_ascii_item(FILE *, int, unsigned int, double, int);
  static double old_write_ascii_item(FILE *, char *, int);
  static void add_element(PlyFile *, char **, int);
  static void add_property(PlyFile *, char **, int);
  static void add_comment(PlyFile *, char *);
  static void add_obj_info(PlyFile *, char *);
  static void copy_property(PlyProperty *, PlyProperty *);
  static void store_item(char *, int, int, unsigned int, double);
  static void get_stored_item( void *, int, int *, unsigned int *, double *);
  static double get_item_value(char *, int);
  static void get_ascii_item(char *, int, int *, unsigned int *, double *);
  static void get_binary_item(FILE *, int, int *, unsigned int *, double *);
  static void ascii_get_element(PlyFile *, char *);
  static void binary_get_element(PlyFile *, char *);
  static char *my_alloc(int, int, char *);
  static int get_prop_type(char *);
  
};

#endif


