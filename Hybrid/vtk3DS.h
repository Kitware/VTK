/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DS.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <ctype.h>

typedef float Vector[3];

/* A generic list type */
#define VTK_LIST_INSERT(root, node) list_insert ((List **)&root, reinterpret_cast<List *>(node))
#define VTK_LIST_FIND(root, name)   list_find   ((List **)&root, name)
#define VTK_LIST_DELETE(root, node) list_delete ((List **)&root, (List *)node)
#define VTK_LIST_KILL(root)         list_kill   ((List **)&root)

#define VTK_LIST_FIELDS  \
    char name[80];   \
    void *next;


typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned int  dword;

typedef struct {
   VTK_LIST_FIELDS
} List;


typedef struct {
    int a, b, c;
} Face;


typedef struct {
    float red, green, blue;
} Colour;


/* Omni light command */
typedef struct {
    VTK_LIST_FIELDS

    Vector pos;            /* Light position */
    Colour col;            /* Light colour */
    vtkLight *aLight;
} OmniLight;


/* Spotlight command */
typedef struct {
    VTK_LIST_FIELDS

    Vector pos;            /* Spotlight position */
    Vector target;         /* Spotlight target location */
    Colour col;            /* Spotlight colour */
    float  hotspot;        /* Hotspot angle (degrees) */
    float  falloff;        /* Falloff angle (degrees) */
    int    shadow_flag;    /* Shadow flag (not used) */
    vtkLight *aLight;
} SpotLight;


/* Camera command */
typedef struct {
    VTK_LIST_FIELDS

    Vector pos;            /* Camera location */
    Vector target;         /* Camera target */
    float  bank;           /* Banking angle (degrees) */
    float  lens;           /* Camera lens size (mm) */
    vtkCamera *aCamera;
} Camera;


/* Material list */
typedef struct {
    VTK_LIST_FIELDS

    int  external;         /* Externally defined material? */
} Material;


/* Object summary */
typedef struct {
    VTK_LIST_FIELDS

    Vector center;         /* Min value of object extents */
    Vector lengths;        /* Max value of object extents */
} Summary;


/* Material property */
typedef struct {
    VTK_LIST_FIELDS

    Colour ambient;
    Colour diffuse;
    Colour specular;
    float  shininess;
    float  transparency;
    float  reflection;
    int    self_illum;
    char   tex_map[40];
    float  tex_strength;
    char   bump_map[40];
    float  bump_strength;
    vtkProperty *aProperty;
} MatProp;



class vtkActor;
class vtkPolyDataMapper;
class vtkPolyDataNormals;
class vtkStripper;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;

/* A mesh object */
typedef struct {
    VTK_LIST_FIELDS

    int  vertices;         /* Number of vertices */
    Vector *vertex;        /* List of object vertices */

    int  faces;            /* Number of faces */
    Face *face;            /* List of object faces */
    Material **mtl;        /* Materials for each face */

    int hidden;            /* Hidden flag */
    int shadow;            /* Shadow flag */
    vtkActor *anActor;
    vtkPolyDataMapper *aMapper;
    vtkPolyDataNormals *aNormals;
    vtkStripper *aStripper;
    vtkPoints *aPoints;
    vtkCellArray *aCellArray;
    vtkPolyData *aPolyData;
 
} Mesh;


typedef struct {
    dword start;
    dword end;
    dword length;
    word  tag;
} Chunk;


typedef struct {
    byte red;
    byte green;
    byte blue;
} Colour_24;



