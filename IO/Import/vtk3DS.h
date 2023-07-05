// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtk3DS_h
#define vtk3DS_h

#include "vtkABINamespace.h"

#include <ctype.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkLight;
class vtkCamera;
class vtkProperty;

typedef float vtk3DSVector[3];

/* A generic list type */
#define VTK_LIST_INSERT(root, node)                                                                \
  list_insert((vtk3DSList**)&root, reinterpret_cast<vtk3DSList*>(node))
#define VTK_LIST_FIND(root, name) list_find((vtk3DSList**)&root, name)
#define VTK_LIST_DELETE(root, node) list_delete((vtk3DSList**)&root, (vtk3DSList*)node)
#define VTK_LIST_KILL(root) list_kill((vtk3DSList**)&root)

#define VTK_LIST_FIELDS                                                                            \
  char name[80];                                                                                   \
  void* next;

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

struct vtk3DSList_t
{
  VTK_LIST_FIELDS
};
using vtk3DSList = struct vtk3DSList_t;

struct vtk3DSFace_t
{
  int a, b, c;
};
using vtk3DSFace = struct vtk3DSFace_t;

struct vtk3DSColour_t
{
  float red, green, blue;
};
using vtk3DSColour = struct vtk3DSColour_t;

/* Omni light command */
struct vtk3DSOmniLight_t
{
  VTK_LIST_FIELDS

  vtk3DSVector pos; /* Light position */
  vtk3DSColour col; /* Light colour */
  vtkLight* aLight;
};
using vtk3DSOmniLight = struct vtk3DSOmniLight_t;

/* Spotlight command */
struct vtk3DSSpotLight_t
{
  VTK_LIST_FIELDS

  vtk3DSVector pos;    /* Spotlight position */
  vtk3DSVector target; /* Spotlight target location */
  vtk3DSColour col;    /* Spotlight colour */
  float hotspot;       /* Hotspot angle (degrees) */
  float falloff;       /* Falloff angle (degrees) */
  int shadow_flag;     /* Shadow flag (not used) */
  vtkLight* aLight;
};
using vtk3DSSpotLight = struct vtk3DSSpotLight_t;

/* Camera command */
struct vtk3DSCamera_t
{
  VTK_LIST_FIELDS

  vtk3DSVector pos;    /* Camera location */
  vtk3DSVector target; /* Camera target */
  float bank;          /* Banking angle (degrees) */
  float lens;          /* Camera lens size (mm) */
  vtkCamera* aCamera;
};
using vtk3DSCamera = struct vtk3DSCamera_t;

/* Material list */
struct vtk3DSMaterial_t
{
  VTK_LIST_FIELDS

  int external; /* Externally defined material? */
};
using vtk3DSMaterial = struct vtk3DSMaterial_t;

/* Object summary */
struct vtk3DSSummary_t
{
  VTK_LIST_FIELDS

  vtk3DSVector center;  /* Min value of object extents */
  vtk3DSVector lengths; /* Max value of object extents */
};
using vtk3DSSummary = struct vtk3DSSummary_t;

/* Material property */
struct vtk3DSMatProp_t
{
  VTK_LIST_FIELDS

  vtk3DSColour ambient;
  vtk3DSColour diffuse;
  vtk3DSColour specular;
  float shininess;
  float transparency;
  float reflection;
  int self_illum;
  char tex_map[40];
  float tex_strength;
  char bump_map[40];
  float bump_strength;
  vtkProperty* aProperty;
};
using vtk3DSMatProp = struct vtk3DSMatProp_t;

class vtkActor;
class vtkPolyDataMapper;
class vtkPolyDataNormals;
class vtkStripper;
class vtkPoints;
class vtkCellArray;
class vtkPolyData;

/* A mesh object */
struct vtk3DSMesh_t
{
  VTK_LIST_FIELDS

  int vertices;         /* Number of vertices */
  vtk3DSVector* vertex; /* List of object vertices */

  int faces;            /* Number of faces */
  vtk3DSFace* face;     /* List of object faces */
  vtk3DSMaterial** mtl; /* Materials for each face */

  int hidden; /* Hidden flag */
  int shadow; /* Shadow flag */
  vtkActor* anActor;
  vtkPolyDataMapper* aMapper;
  vtkPolyDataNormals* aNormals;
  vtkStripper* aStripper;
  vtkPoints* aPoints;
  vtkCellArray* aCellArray;
  vtkPolyData* aPolyData;
};
using vtk3DSMesh = struct vtk3DSMesh_t;

struct vtk3DSChunk_t
{
  dword start;
  dword end;
  dword length;
  word tag;
};
using vtk3DSChunk = struct vtk3DSChunk_t;

struct vtk3DSColour_t_24
{
  byte red;
  byte green;
  byte blue;
};
using vtk3DSColour_24 = struct vtk3DSColour_t_24;

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtk3DS.h
