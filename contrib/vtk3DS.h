/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DS.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#ifndef __VECT_H
#define __VECT_H

#include <ctype.h>

typedef float Vector[3];

#define X 0
#define Y 1
#define Z 2

#endif
/* Internal bounding modes */
#define FALSE 0
#define TRUE 1
#define OFF  0
#define ON   1
#define AUTO 2

#define MAX_LIB  10
#define ASPECT   1.333


#define DEG(x) ((180.0/M_PI)*(x))
#define RAD(x) ((M_PI/180.0)*(x))

#ifndef M_PI
#define	M_PI		3.14159265358979323846
#endif

#ifndef MAXFLOAT
#define MAXFLOAT (1e37)
#endif

/* A generic list type */
#define LIST_INSERT(root, node) list_insert ((List **)&root, (List *)node)
#define LIST_FIND(root, name)   list_find   ((List **)&root, name)
#define LIST_DELETE(root, node) list_delete ((List **)&root, (List *)node)
#define LIST_KILL(root)         list_kill   ((List **)&root)

#define LIST_FIELDS  \
    char name[80];   \
    void *next;


typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

typedef struct {
    LIST_FIELDS
} List;


typedef struct {
    int a, b, c;
} Face;


typedef struct {
    float red, green, blue;
} Colour;


/* Omni light command */
typedef struct {
    LIST_FIELDS

    Vector pos;            /* Light position */
    Colour col;            /* Light colour */
} OmniLight;


/* Spotlight command */
typedef struct {
    LIST_FIELDS

    Vector pos;            /* Spotlight position */
    Vector target;         /* Spotlight target location */
    Colour col;            /* Spotlight colour */
    float  hotspot;        /* Hotspot angle (degrees) */
    float  falloff;        /* Falloff angle (degrees) */
    int    shadow_flag;    /* Shadow flag (not used) */
} SpotLight;


/* Camera command */
typedef struct {
    LIST_FIELDS

    Vector pos;            /* Camera location */
    Vector target;         /* Camera target */
    float  bank;           /* Banking angle (degrees) */
    float  lens;           /* Camera lens size (mm) */
} Camera;


/* Material list */
typedef struct {
    LIST_FIELDS

    int  external;         /* Externally defined material? */
} Material;


/* Object summary */
typedef struct {
    LIST_FIELDS

    Vector center;         /* Min value of object extents */
    Vector lengths;        /* Max value of object extents */
} Summary;


/* Material property */
typedef struct {
    LIST_FIELDS

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



/* A mesh object */
typedef struct {
    LIST_FIELDS

    int  vertices;         /* Number of vertices */
    Vector *vertex;        /* List of object vertices */

    int  faces;            /* Number of faces */
    Face *face;            /* List of object faces */
    Material **mtl;        /* Materials for each face */

    int hidden;            /* Hidden flag */
    int shadow;            /* Shadow flag */
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


/* Default material property */
static MatProp DefaultMaterial =
  { "Default", NULL,
    {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0},
    70.0, // shininess
    0.0,  // transparency
    0.0,  // reflection
    FALSE,// self illumination
    "",   // tex_map
    0.0,  // tex_strength
    "",   // bump_map
    0.0,  // bump_strength
    NULL};// vtkProperty

static Colour Black = {0.0, 0.0, 0.0};

static char   obj_name[80] = "";
static Colour fog_colour = {0.0, 0.0, 0.0};
static Colour col        = {0.0, 0.0, 0.0};
static Colour global_amb = {0.1, 0.1, 0.1};
static Vector pos        = {0.0, 0.0, 0.0};
static Vector target     = {0.0, 0.0, 0.0};
static float  fog_distance = 0.0;
static float  hotspot = -1;
static float  falloff = -1;

static void cleanup_name (char *);
static void list_insert (List **root, List *new_node);
static void *list_find (List **root, char *name);
static void list_delete (List **root, List *node);
static void list_kill (List **root);
static MatProp *create_mprop (void);
static Mesh *create_mesh (char *name, int vertices, int faces);
static void free_mesh_data (Mesh *mesh);
static float findfov (float lens);

