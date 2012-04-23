/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DS.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <ctype.h>

class vtkLight;
class vtkCamera;
class vtkProperty;

typedef float vtk3DSVector[3];

/* A generic list type */
#define VTK_LIST_INSERT(root, node) list_insert ((vtk3DSList **)&root, reinterpret_cast<vtk3DSList *>(node))
#define VTK_LIST_FIND(root, name)   list_find   ((vtk3DSList **)&root, name)
#define VTK_LIST_DELETE(root, node) list_delete ((vtk3DSList **)&root, (vtk3DSList *)node)
#define VTK_LIST_KILL(root)         list_kill   ((vtk3DSList **)&root)

#define VTK_LIST_FIELDS  \
    char name[80];   \
    void *next;


typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned int  dword;

typedef struct {
   VTK_LIST_FIELDS
} vtk3DSList;


typedef struct {
    int a, b, c;
} vtk3DSFace;


typedef struct {
    float red, green, blue;
} vtk3DSColour;


/* Omni light command */
typedef struct {
    VTK_LIST_FIELDS

    vtk3DSVector pos;            /* Light position */
    vtk3DSColour col;            /* Light colour */
    vtkLight *aLight;
} vtk3DSOmniLight;


/* Spotlight command */
typedef struct {
    VTK_LIST_FIELDS

    vtk3DSVector pos;            /* Spotlight position */
    vtk3DSVector target;         /* Spotlight target location */
    vtk3DSColour col;            /* Spotlight colour */
    float  hotspot;        /* Hotspot angle (degrees) */
    float  falloff;        /* Falloff angle (degrees) */
    int    shadow_flag;    /* Shadow flag (not used) */
    vtkLight *aLight;
} vtk3DSSpotLight;


/* Camera command */
typedef struct {
    VTK_LIST_FIELDS

    vtk3DSVector pos;            /* Camera location */
    vtk3DSVector target;         /* Camera target */
    float  bank;           /* Banking angle (degrees) */
    float  lens;           /* Camera lens size (mm) */
    vtkCamera *aCamera;
} vtk3DSCamera;


/* Material list */
typedef struct {
    VTK_LIST_FIELDS

    int  external;         /* Externally defined material? */
} vtk3DSMaterial;


/* Object summary */
typedef struct {
    VTK_LIST_FIELDS

    vtk3DSVector center;         /* Min value of object extents */
    vtk3DSVector lengths;        /* Max value of object extents */
} vtk3DSSummary;


/* Material property */
typedef struct {
    VTK_LIST_FIELDS

    vtk3DSColour ambient;
    vtk3DSColour diffuse;
    vtk3DSColour specular;
    float  shininess;
    float  transparency;
    float  reflection;
    int    self_illum;
    char   tex_map[40];
    float  tex_strength;
    char   bump_map[40];
    float  bump_strength;
    vtkProperty *aProperty;
} vtk3DSMatProp;



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
    vtk3DSVector *vertex;        /* List of object vertices */

    int  faces;            /* Number of faces */
    vtk3DSFace *face;            /* List of object faces */
    vtk3DSMaterial **mtl;        /* Materials for each face */

    int hidden;            /* Hidden flag */
    int shadow;            /* Shadow flag */
    vtkActor *anActor;
    vtkPolyDataMapper *aMapper;
    vtkPolyDataNormals *aNormals;
    vtkStripper *aStripper;
    vtkPoints *aPoints;
    vtkCellArray *aCellArray;
    vtkPolyData *aPolyData;

} vtk3DSMesh;


typedef struct {
    dword start;
    dword end;
    dword length;
    word  tag;
} vtk3DSChunk;


typedef struct {
    byte red;
    byte green;
    byte blue;
} vtk3DSColour_24;



// VTK-HeaderTest-Exclude: vtk3DS.h
