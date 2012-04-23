#ifndef __VTK_SYSTEM_INCLUDES__INSIDE
#  define __VTK_SYSTEM_INCLUDES__INSIDE
#  include "vtkWin32Header.h"
#  undef __VTK_SYSTEM_INCLUDES__INSIDE
#endif

#if defined (__digital__) && defined (__unix__) || defined(__IBMCPP__) || defined(__sun)
#define HAVE_ALLOCA_H 1
#endif


#ifdef __GNUC__
#undef alloca
#define alloca __builtin_alloca
#else /* not __GNUC__ */
#if HAVE_ALLOCA_H
#include <alloca.h>
#else /* not HAVE_ALLOCA_H */
#ifdef _AIX
#pragma alloca
#else /* not _AIX */
#ifndef alloca
char *alloca ();
#endif
#include <malloc.h>
#endif /* not _AIX */
#endif /* not HAVE_ALLOCA_H */
#endif /* not __GNUC__ */

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRMLImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* ======================================================================

   Importer based on BNF Yacc and Lex parser definition from:

        **************************************************
        * VRML 2.0 Parser
        * Copyright (C) 1996 Silicon Graphics, Inc.
        *
        * Author(s) :    Gavin Bell
        *                Daniel Woods (first port)
        **************************************************

  Ported to VTK By:     Thomas D. Citriniti
                        Rensselaer Polytechnic Institute
                        citrit@rpi.edu

=======================================================================*/
#include "vtkVRMLImporter.h"

#include "vtkActor.h"
#include "vtkByteSwap.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLight.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"
#include "vtkSystemIncludes.h"
#include "vtkTransform.h"
#include "vtkVRML.h"

#ifdef _MSC_VER
#pragma warning( disable : 4005 )
#endif

class vtkVRMLImporterInternal {
public:
  vtkVRMLImporterInternal() : Heap(1) {}
//BTX
#ifdef _MSC_VER
#pragma warning( disable : 4251 )
#endif

  vtkVRMLVectorType<vtkObject*> Heap;

#ifdef _MSC_VER
#pragma warning( default : 4251 )
#endif

//ETX
};

// Heap to manage memory leaks
vtkHeap *vtkVRMLAllocator::Heap=NULL;

void  vtkVRMLAllocator::Initialize()
{
  if ( Heap == NULL )
    {
    Heap = vtkHeap::New();
    }
}
void* vtkVRMLAllocator::AllocateMemory(size_t n)
{
  return Heap->AllocateMemory(n);
}

void vtkVRMLAllocator::CleanUp()
{
  if ( Heap )
    {
    Heap->Delete();
    Heap = NULL;
    }
}
char* vtkVRMLAllocator::StrDup(const char *str)
{
  return Heap->StringDup(str);
}


// Provide isatty prototype for Cygwin.
#ifdef __CYGWIN__
#include <unistd.h>
#endif

static int memyyInput_i = 0;
static int memyyInput_j = 0;

// Used during the parsing
static int creatingDEF = 0;
static char *curDEFName;

// Used by the lex input to get characters. Needed to read in memory structure
static void memyyInput(char *buf, int &result, int max_size);
static void defyyInput(char *buf, int &result, int max_size);


/**************************************************
 * VRML 2.0 Parser
 * Copyright (C) 1996 Silicon Graphics, Inc.
 *
 * Author(s)    : Gavin Bell
 *                Daniel Woods (first port)
 **************************************************
 */

//
// The VrmlNodeType class is responsible for storing information about node
// or prototype types.
//

#ifndef _VRML_NODE_TYPE_
#define _VRML_NODE_TYPE_

#ifdef USE_STD_NAMESPACE
using namespace std;
#endif

// used to hold the VRML DEF names and assoc vtkObjects
class vtkVRMLUseStruct {
public:
  vtkVRMLUseStruct( char *n, vtkObject *o) { defName = n; defObject = o; }
  char            *defName;
  vtkObject       *defObject;

  void* operator new(size_t n)
    {
      return vtkVRMLAllocator::AllocateMemory(n);
    }

  void operator delete(void *vtkNotUsed(ptr)) {}
};



class VrmlNodeType {
public:
  // Constructor.  Takes name of new type (e.g. "Transform" or "Box")
  // Copies the string given as name.
  VrmlNodeType(const char *nm);

  // Destructor exists mainly to deallocate storage for name
  ~VrmlNodeType();

  // Namespace management functions.  PROTO definitions add node types
  // to the namespace.  PROTO implementations are a separate node
  // namespace, and require that any nested PROTOs NOT be available
  // outside the PROTO implementation.
  // addToNameSpace will print an error to stderr if the given type
  // is already defined.
  static void addToNameSpace(VrmlNodeType *);
  static void pushNameSpace();
  static void popNameSpace();

  // Find a node type, given its name.  Returns NULL if type is not defined.
  static const VrmlNodeType *find(const char *nm);

  // Routines for adding/getting eventIns/Outs/fields
  void addEventIn(const char *name, int type);
  void addEventOut(const char *name, int type);
  void addField(const char *name, int type);
  void addExposedField(const char *name, int type);

  int hasEventIn(const char *name) const;
  int hasEventOut(const char *name) const;
  int hasField(const char *name) const;
  int hasExposedField(const char *name) const;

  const char *getName() const { return name; }

  void* operator new(size_t n)
    {
      return vtkVRMLAllocator::AllocateMemory(n);
    }

  void operator delete(void *vtkNotUsed(ptr)) {}

  struct NameTypeRec {
    char *name;
    int type;

    void* operator new(size_t n)
      {
        return vtkVRMLAllocator::AllocateMemory(n);
      }

    void operator delete(void *vtkNotUsed(ptr)) {}

  };

// This is used to keep track of which field in which type of node is being
// parsed.  Field are nested (nodes are contained inside MFNode/SFNode fields)
// so a stack of these is needed:
  struct FieldRec
  {
    const VrmlNodeType *nodeType;
    const char *fieldName;
  };

  // Node types are stored in this data structure:
  static vtkVRMLVectorType<VrmlNodeType*>* typeList;
  static vtkVRMLVectorType<vtkVRMLUseStruct *>* useList;
  static vtkVRMLVectorType<FieldRec*>* currentField;

private:
  void add(vtkVRMLVectorType<NameTypeRec*> &,const char *,int);
  int has(const vtkVRMLVectorType<NameTypeRec*> &,const char *) const;

  char *name;

  vtkVRMLVectorType<NameTypeRec*> eventIns;
  vtkVRMLVectorType<NameTypeRec*> eventOuts;
  vtkVRMLVectorType<NameTypeRec*> fields;
};

#endif
//
// The VrmlNodeType class is responsible for storing information about node
// or prototype types.
//

#include <assert.h>

//
// Static list of node types.
//
vtkVRMLVectorType<VrmlNodeType*>* VrmlNodeType::typeList;
vtkVRMLVectorType<vtkVRMLUseStruct *>* VrmlNodeType::useList;
vtkVRMLVectorType<VrmlNodeType::FieldRec*>* VrmlNodeType::currentField;

VrmlNodeType::VrmlNodeType(const char *nm)
{
  assert(nm != NULL);
  name = static_cast<char*>(
    vtkVRMLAllocator::AllocateMemory((strlen(nm)+1)*sizeof(char)));
  strcpy(name, nm);
}

VrmlNodeType::~VrmlNodeType()
{
  // Free strings duplicated when fields/eventIns/eventOuts added:

  int i;
  for (i = 0;i < eventIns.Count(); i++)
    {
    NameTypeRec *r = eventIns[i];
//    free(r->name);
    delete r;
    }
  for (i = 0;i < eventOuts.Count(); i++)
    {
    NameTypeRec *r = eventOuts[i];
//    free(r->name);
    delete r;
    }
  for (i = 0;i < fields.Count(); i++)
    {
    NameTypeRec *r = fields[i];
//    free(r->name);
    delete r;
    }
}

void
VrmlNodeType::addToNameSpace(VrmlNodeType *_type)
{
  if (find(_type->getName()) != NULL)
    {
    cerr << "PROTO " << _type->getName() << " already defined\n";
    return;
    }
  *typeList += _type;
}

//
// One list is used to store all the node types.  Nested namespaces are
// separated by NULL elements.
// This isn't terribly efficient, but it is nice and simple.
//
void
VrmlNodeType::pushNameSpace()
{
  *typeList += (VrmlNodeType *) NULL;
}

void
VrmlNodeType::popNameSpace()
{
  // Remove everything up to and including the next NULL marker:
  for (int i = 0;i < typeList->Count(); i++)
    {
    VrmlNodeType *nodeType = typeList->Pop();

    if (nodeType == NULL)
      {
      break;
      }
    else
      {
      // NOTE:  Instead of just deleting the VrmlNodeTypes, you will
      // probably want to reference count or garbage collect them, since
      // any nodes created as part of the PROTO implementation will
      // probably point back to their VrmlNodeType structure.
      delete nodeType;
      }
    }
}

const VrmlNodeType *
VrmlNodeType::find(const char *_name)
{
  // Look through the type stack:
  for (int i = 0;i < typeList->Count(); i++)
    {
    const VrmlNodeType *nt = (*typeList)[i];
    if (nt != NULL && strcmp(nt->getName(),_name) == 0)
      {
      return nt;
      }
    }
  return NULL;
}

void
VrmlNodeType::addEventIn(const char *nodeName, int type)
{
  add(eventIns, nodeName, type);
};
void
VrmlNodeType::addEventOut(const char *nodeName, int type)
{
  add(eventOuts, nodeName, type);
};
void
VrmlNodeType::addField(const char *nodeName, int type)
{
  add(fields, nodeName, type);
};
void
VrmlNodeType::addExposedField(const char *nodeName, int type)
{
  char tmp[1000];
  add(fields, nodeName, type);
  sprintf(tmp, "set_%s", nodeName);
  add(eventIns, tmp, type);
  sprintf(tmp, "%s_changed", nodeName);
  add(eventOuts, tmp, type);
};

void
VrmlNodeType::add(vtkVRMLVectorType<NameTypeRec*> &recs, const char *nodeName, int type)
{
  NameTypeRec *r = new NameTypeRec;
  r->name = vtkVRMLAllocator::StrDup(nodeName); //strdup(nodeName);
  r->type = type;
  recs += r;
}

int
VrmlNodeType::hasEventIn(const char *nodeName) const
{
  return has(eventIns, nodeName);
}
int
VrmlNodeType::hasEventOut(const char *nodeName) const
{
  return has(eventOuts, nodeName);
}
int
VrmlNodeType::hasField(const char *nodeName) const
{
  return has(fields, nodeName);
}
int
VrmlNodeType::hasExposedField(const char *nodeName) const
{
  // Must have field "name", eventIn "set_name", and eventOut
  // "name_changed", all with same type:
  char tmp[1000];
  int type;
  if ( (type = has(fields, nodeName)) == 0) return 0;

  sprintf(tmp, "set_%s\n", nodeName);
  if (type != has(eventIns, nodeName)) return 0;

  sprintf(tmp, "%s_changed", nodeName);
  if (type != has(eventOuts, nodeName)) return 0;

  return type;
}
int
VrmlNodeType::has(const vtkVRMLVectorType<NameTypeRec*> &recs, const char *nodeName) const
{
  for (int i = 0;i < recs.Count(); i++)
    {
    NameTypeRec *n = recs.Get(i);
    if (strcmp(n->name, nodeName) == 0)
      return n->type;
    }
  return 0;
}
// Here comes the parser and lexer.

// Begin of Auto-generated Parser Code


/*  A Bison parser, made from parser.y with Bison version GNU Bison version 1.24
  */

#define YYBISON 1  /* Identify Bison output.  */

#define IDENTIFIER      258
#define DEF     259
#define USE     260
#define PROTO   261
#define EXTERNPROTO     262
#define TO      263
#define IS      264
#define ROUTE   265
#define SFN_NULL        266
#define EVENTIN 267
#define EVENTOUT        268
#define FIELD   269
#define EXPOSEDFIELD    270
#define SFBOOL  271
#define SFCOLOR 272
#define SFFLOAT 273
#define SFIMAGE 274
#define SFINT32 275
#define SFNODE  276
#define SFROTATION      277
#define SFSTRING        278
#define SFTIME  279
#define SFVEC2F 280
#define SFVEC3F 281
#define MFCOLOR 282
#define MFFLOAT 283
#define MFINT32 284
#define MFROTATION      285
#define MFSTRING        286
#define MFVEC2F 287
#define MFVEC3F 288
#define MFNODE  289



//
// Parser for VRML 2.0 files.
// This is a minimal parser that does NOT generate an in-memory scene graph.
//

// The original parser was developed on a Windows 95 PC with
// Borland's C++ 5.0 development tools.  This was then ported
// to a Windows 95 PC with Microsoft's MSDEV C++ 4.0 development
// tools.  The port introduced the ifdef's for
//    USING_BORLAND_CPP_5          : since this provides a "std namespace",
//    TWO_ARGUMENTS_FOR_STL_STACK  : STL is a moving target.  The stack template
//                                     class takes either one or two arguments.


#define YYDEBUG 1

#include <stdlib.h>

#ifdef USE_STD_NAMESPACE
using namespace std;
#endif
#undef bool


// Currently-being-define proto.  Prototypes may be nested, so a stack
// is needed:

static vtkVRMLVectorType<VrmlNodeType*> *CurrentProtoStack = NULL;


// This is used when the parser knows what kind of token it expects
// to get next-- used when parsing field values (whose types are declared
// and read by the parser) and at certain other places:
extern int expectToken;

// Current line number (set by lexer)
extern int currentLineNumber;

// Some helper routines defined below:
static void beginProto(const char *);
static void endProto();

static int addField(const char *type, const char *name);
static int addEventIn(const char *type, const char *name);
static int addEventOut(const char *type, const char *name);
static int addExposedField(const char *type, const char *name);
static int add(void (VrmlNodeType::*)(const char *, int), const char *,
               const char *);
static int fieldType(const char *type);
static void inScript();
static void expect(int type);

void yyerror(const char *);
int  yylex(vtkVRMLImporter* self);


typedef union {
  char                    *string;

  /* Other types that will be needed by a true VRML implementation
   * (but are not defined by this parser due to the complexity):
   * Node *node;
   * list<Node *> *nodeList;
   */

  float           sffloat;
  vtkPoints       *vec3f;
  vtkFloatArray   *vec2f;
  vtkIdTypeArray  *mfint32;
  int             sfint;
} YYSTYPE;

#ifndef YYLTYPE
typedef
struct yyltype
{
  int timestamp;
  int first_line;
  int first_column;
  int last_line;
  int last_column;
  char *text;
}
yyltype;

#define YYLTYPE yyltype
#endif

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif

#define YYFINAL         128
#define YYFLAG          -32768
#define YYNTBASE        40

#define YYTRANSLATE(x) ((unsigned)(x) <= 289 ? yytranslate[x] : 68)

static const char yytranslate[] = {     0,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,    39,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        35,     2,    36,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,    37,     2,    38,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
                                        2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
                                        6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
                                        16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
                                        26,    27,    28,    29,    30,    31,    32,    33,    34
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
                                    0,     2,     3,     6,     8,    10,    12,    14,    15,    16,
                                    22,    25,    27,    29,    30,    40,    41,    42,    51,    52,
                                    55,    59,    63,    64,    70,    71,    77,    78,    81,    85,
                                    89,    93,    97,   106,   107,   113,   114,   117,   118,   122,
                                    124,   126,   130,   134,   135,   141,   147,   153,   155,   157,
                                    159,   161,   163,   165,   167,   169,   171,   173,   175,   177,
                                    179,   181,   183,   185,   187,   190,   193,   196,   199,   203,
                                    205,   206
};

static const short yyrhs[] = {    41,
                                  0,     0,    41,    42,     0,    43,     0,    46,     0,    58,
                                  0,    59,     0,     0,     0,     4,    44,     3,    45,    59,
                                  0,     5,     3,     0,    47,     0,    49,     0,     0,     6,
                                  3,    48,    35,    52,    36,    37,    41,    38,     0,     0,
                                  0,     7,     3,    50,    35,    56,    36,    51,    65,     0,
                                  0,    52,    53,     0,    12,     3,     3,     0,    13,     3,
                                  3,     0,     0,    14,     3,     3,    54,    65,     0,     0,
                                  15,     3,     3,    55,    65,     0,     0,    56,    57,     0,
                                  12,     3,     3,     0,    13,     3,     3,     0,    14,     3,
                                  3,     0,    15,     3,     3,     0,    10,     3,    39,     3,
                                  8,     3,    39,     3,     0,     0,     3,    60,    37,    61,
                                  38,     0,     0,    61,    62,     0,     0,     3,    63,    65,
                                  0,    58,     0,    46,     0,    12,     3,     3,     0,    13,
                                  3,     3,     0,     0,    14,     3,     3,    64,    65,     0,
                                  12,     3,     3,     9,     3,     0,    13,     3,     3,     9,
                                  3,     0,    16,     0,    17,     0,    27,     0,    18,     0,
                                  28,     0,    19,     0,    20,     0,    29,     0,    22,     0,
                                  30,     0,    23,     0,    31,     0,    24,     0,    25,     0,
                                  32,     0,    26,     0,    33,     0,    21,    43,     0,    21,
                                  11,     0,    34,    66,     0,     9,     3,     0,    35,    67,
                                  36,     0,    43,     0,     0,    67,    43,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
                                 106,   109,   111,   114,   116,   117,   120,   122,   123,   124,
                                 125,   128,   130,   133,   135,   139,   141,   143,   145,   147,
                                 150,   153,   155,   157,   158,   160,   163,   165,   168,   171,
                                 173,   175,   179,   184,   186,   189,   191,   194,   196,   197,
                                 198,   201,   202,   203,   206,   207,   209,   213,   215,   216,
                                 217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
                                 227,   228,   229,   230,   232,   233,   234,   235,   238,   240,
                                 243,   245
};

static const char * const yytname[] = {   "$","error","$undefined.","IDENTIFIER",
                                          "DEF","USE","PROTO","EXTERNPROTO","TO","IS","ROUTE","SFN_NULL","EVENTIN","EVENTOUT",
                                          "FIELD","EXPOSEDFIELD","SFBOOL","SFCOLOR","SFFLOAT","SFIMAGE","SFINT32","SFNODE",
                                          "SFROTATION","SFSTRING","SFTIME","SFVEC2F","SFVEC3F","MFCOLOR","MFFLOAT","MFINT32",
                                          "MFROTATION","MFSTRING","MFVEC2F","MFVEC3F","MFNODE","'['","']'","'{'","'}'",
                                          "'.'","vrmlscene","declarations","declaration","nodeDeclaration","@1","@2","protoDeclaration",
                                          "proto","@3","externproto","@4","@5","interfaceDeclarations","interfaceDeclaration",
                                          "@6","@7","externInterfaceDeclarations","externInterfaceDeclaration","routeDeclaration",
                                          "node","@8","nodeGuts","nodeGut","@9","@10","fieldValue","mfnodeValue","nodes",
                                          ""
};
#endif

static const short yyr1[] = {     0,
                                  40,    41,    41,    42,    42,    42,    43,    44,    45,    43,
                                  43,    46,    46,    48,    47,    50,    51,    49,    52,    52,
                                  53,    53,    54,    53,    55,    53,    56,    56,    57,    57,
                                  57,    57,    58,    60,    59,    61,    61,    63,    62,    62,
                                  62,    62,    62,    64,    62,    62,    62,    65,    65,    65,
                                  65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
                                  65,    65,    65,    65,    65,    65,    65,    65,    66,    66,
                                  67,    67
};

static const short yyr2[] = {     0,
                                  1,     0,     2,     1,     1,     1,     1,     0,     0,     5,
                                  2,     1,     1,     0,     9,     0,     0,     8,     0,     2,
                                  3,     3,     0,     5,     0,     5,     0,     2,     3,     3,
                                  3,     3,     8,     0,     5,     0,     2,     0,     3,     1,
                                  1,     3,     3,     0,     5,     5,     5,     1,     1,     1,
                                  1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
                                  1,     1,     1,     1,     2,     2,     2,     2,     3,     1,
                                  0,     2
};

static const short yydefact[] = {     2,
                                      1,    34,     8,     0,     0,     0,     0,     3,     4,     5,
                                      12,    13,     6,     7,     0,     0,    11,    14,    16,     0,
                                      36,     9,     0,     0,     0,     0,     0,    19,    27,     0,
                                      38,     0,     0,     0,    35,    41,    40,    37,    10,     0,
                                      0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
                                      0,    20,     0,     0,     0,     0,    17,    28,     0,     0,
                                      48,    49,    51,    53,    54,     0,    56,    58,    60,    61,
                                      63,    50,    52,    55,    57,    59,    62,    64,     0,    39,
                                      42,    43,    44,     0,     0,     0,     0,     2,     0,     0,
                                      0,     0,     0,     0,    68,    66,    65,    71,    70,    67,
                                      0,     0,     0,    21,    22,    23,    25,     0,    29,    30,
                                      31,    32,    18,    33,     0,    46,    47,    45,     0,     0,
                                      15,    69,    72,    24,    26,     0,     0,     0
};

static const short yydefgoto[] = {   126,
                                     1,     8,     9,    16,    27,    10,    11,    23,    12,    24,
                                     93,    40,    52,   119,   120,    41,    58,    13,    14,    15,
                                     26,    38,    43,   103,    80,   100,   115
};

static const short yypact[] = {-32768,
                               79,-32768,-32768,    -1,     0,     3,     4,-32768,-32768,-32768,
                               -32768,-32768,-32768,-32768,   -28,    11,-32768,-32768,-32768,   -18,
                               -32768,-32768,    -6,     5,    32,    -2,    38,-32768,-32768,    35,
                               -32768,    39,    41,    45,-32768,-32768,-32768,-32768,-32768,    19,
                               66,    48,    43,    50,    51,    54,    84,    85,    87,    88,
                               55,-32768,    90,    91,    92,    93,-32768,-32768,    58,    95,
                               -32768,-32768,-32768,-32768,-32768,    34,-32768,-32768,-32768,-32768,
                               -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    23,-32768,
                               94,    96,-32768,    97,    98,   101,   103,-32768,   104,   105,
                               106,   107,    43,   108,-32768,-32768,-32768,-32768,-32768,-32768,
                               109,   110,    43,-32768,-32768,-32768,-32768,    12,-32768,-32768,
                               -32768,-32768,-32768,-32768,    20,-32768,-32768,-32768,    43,    43,
                               -32768,-32768,-32768,-32768,-32768,    99,   114,-32768
};

static const short yypgoto[] = {-32768,
                                27,-32768,   -66,-32768,-32768,   100,-32768,-32768,-32768,-32768,
                                -32768,-32768,-32768,-32768,-32768,-32768,-32768,   102,    89,-32768,
                                -32768,-32768,-32768,-32768,   -73,-32768,-32768
};


#define YYLAST          128


static const short yytable[] = {    97,
                                    31,    17,    18,     5,     6,    19,    20,     7,    21,    32,
                                    33,    34,    99,    22,     2,     3,     4,     5,     6,   113,
                                    25,     7,     2,     3,     4,     2,     3,     4,    28,   118,
                                    47,    48,    49,    50,    30,    35,     2,     3,     4,    29,
                                    2,    44,    42,    45,    96,   124,   125,    46,   123,   121,
                                    59,    60,    81,    82,    51,   122,    83,    98,    61,    62,
                                    63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
                                    73,    74,    75,    76,    77,    78,    79,    53,    54,    55,
                                    56,     2,     3,     4,     5,     6,    84,    85,     7,    86,
                                    87,    88,    89,    90,    91,    92,    94,    95,   127,   104,
                                    105,    57,   101,   106,   102,   107,   109,   110,   111,   112,
                                    114,   116,   117,   128,   108,    39,     0,     0,     0,     0,
                                    0,     0,     0,     0,     0,    36,     0,    37
};

static const short yycheck[] = {    66,
                                    3,     3,     3,     6,     7,     3,     3,    10,    37,    12,
                                    13,    14,    79,     3,     3,     4,     5,     6,     7,    93,
                                    39,    10,     3,     4,     5,     3,     4,     5,    35,   103,
                                    12,    13,    14,    15,     3,    38,     3,     4,     5,    35,
                                    3,     3,     8,     3,    11,   119,   120,     3,   115,    38,
                                    3,     9,     3,     3,    36,    36,     3,    35,    16,    17,
                                    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
                                    28,    29,    30,    31,    32,    33,    34,    12,    13,    14,
                                    15,     3,     4,     5,     6,     7,     3,     3,    10,     3,
                                    3,    37,     3,     3,     3,     3,    39,     3,     0,     3,
                                    3,    36,     9,     3,     9,     3,     3,     3,     3,     3,
                                    3,     3,     3,     0,    88,    27,    -1,    -1,    -1,    -1,
                                    -1,    -1,    -1,    -1,    -1,    26,    -1,    26
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */


/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

extern int yylex(vtkVRMLImporter* self);
extern void yyerror();

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
#pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#include <alloca.h>
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */


/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         -2
#define YYEOF           0
#define YYACCEPT        return(0)
#define YYABORT         return(1)
#define YYERROR         goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL          goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do                                                              \
  if (yychar == YYEMPTY && yylen == 1)                          \
    { yychar = (token), yylval = (value);                       \
      yychar1 = YYTRANSLATE (yychar);                           \
      YYPOPSTACK;                                               \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    { yyerror ("syntax error: cannot back up"); YYERROR; }      \
while (0)

#define YYTERROR        1
#define YYERRCODE       256

#ifndef YYPURE
#define YYLEX           yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX           yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX           yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX           yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX           yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int     yychar;                 /*  the lookahead symbol                */
YYSTYPE yylval;                 /*  the semantic value of the           */
                                /*  lookahead symbol                    */

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;                 /*  location data for the lookahead     */
                                /*  symbol                              */
#endif

int yynerrs;                    /*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;                    /*  nonzero means print parse trace     */
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks       */

#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (vtkVRMLImporter*);
#endif

#if __GNUC__ > 1                /* GNU C and GNU C++ define this.  */
#define __yy_memcpy(FROM,TO,COUNT)      __builtin_memcpy(TO,FROM,COUNT)
#else                           /* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (from, to, count)
  char *from;
char *to;
int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif



int
yyparse(vtkVRMLImporter* self)
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;      /*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;              /*  lookahead token as an internal (translated) token number */

  short yyssa[YYINITDEPTH];     /*  the state stack                     */
  YYSTYPE yyvsa[YYINITDEPTH];   /*  the semantic value stack            */

  short *yyss = yyssa;          /*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;        /*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];   /*  the location stack                  */
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;                /*  the variable used to return         */
                                /*  semantic values from the action     */
                                /*  routines                            */

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;             /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

  /* Push a new state, which is found in  yystate  .  */
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.  */
  yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
    /* Give user a chance to reallocate the stack */
    /* Use copies of these so that the &'s don't force the real ones into memory. */
    YYSTYPE *yyvs1 = yyvs;
    short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
    YYLTYPE *yyls1 = yyls;
#endif

    /* Get the current used size of the three stacks, in elements.  */
    int size = yyssp - yyss + 1;

#ifdef yyoverflow
    /* Each stack pointer address is followed by the size of
       the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
    /* This used to be a conditional around just the two extra args,
       but that might be undefined if yyoverflow is a macro.  */
    yyoverflow("parser stack overflow",
               &yyss1, size * sizeof (*yyssp),
               &yyvs1, size * sizeof (*yyvsp),
               &yyls1, size * sizeof (*yylsp),
               &yystacksize);
#else
    yyoverflow("parser stack overflow",
               &yyss1, size * sizeof (*yyssp),
               &yyvs1, size * sizeof (*yyvsp),
               &yystacksize);
#endif

    yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
    yyls = yyls1;
#endif
#else /* no yyoverflow */
    /* Extend the stack our own way.  */
    if (yystacksize >= YYMAXDEPTH)
      {
      yyerror("parser stack overflow");
      return 2;
      }
    yystacksize *= 2;
    if (yystacksize > YYMAXDEPTH)
      yystacksize = YYMAXDEPTH;
    yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
    __yy_memcpy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
    yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
    __yy_memcpy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
    yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
    __yy_memcpy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

    yyssp = yyss + size - 1;
    yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
    yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
    if (yydebug)
      fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

    if (yyssp >= yyss + yystacksize - 1)
      YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
  yybackup:

  /* Do appropriate processing given the current state.  */
  /* Read a lookahead token if we need one and don't already have one.  */
  /* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
    if (yydebug)
      fprintf(stderr, "Reading a token: ");
#endif
    yychar = yylex(self);
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)              /* This means end of input. */
    {
    yychar1 = 0;
    yychar = YYEOF;           /* Don't call YYLEX any more */

#if YYDEBUG != 0
    if (yydebug)
      fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
    yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
    if (yydebug)
      {
      fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
      /* Give the individual parser a way to print the precise meaning
         of a token, for further debugging info.  */
#ifdef YYPRINT
      YYPRINT (stderr, yychar, yylval);
#endif
      fprintf (stderr, ")\n");
      }
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
     New state is final state => don't bother to shift,
     just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
    if (yyn == YYFLAG)
      goto yyerrlab;
    yyn = -yyn;
    goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

  /* Do the default action for the current state.  */
  yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

  /* Do a reduction.  yyn is the number of a rule to reduce with.  */
  yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */
  else
    yyval.sfint = 0;

#if YYDEBUG != 0
  if (yydebug)
    {
    int i;

    fprintf (stderr, "Reducing via rule %d (line %d), ",
             yyn, yyrline[yyn]);

    /* Print the symbols being reduced, and their result.  */
    for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
      fprintf (stderr, "%s ", yytname[yyrhs[i]]);
    fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  // Note: several free() methods are commented out due to the use of
  // vtkVRMLAllocator.
  switch (yyn) {

  case 8:
  { creatingDEF = 1; ;
  break;}
  case 9:
  { curDEFName = yyvsp[0].string; ;
  break;}
  case 10:
  { creatingDEF = 0; ;
  break;}
  case 11:
  { self->useNode(yyvsp[0].string);//free(yyvsp[0].string); ;
  break;}
  case 14:
  { beginProto(yyvsp[0].string); ;
  break;}
  case 15:
  { endProto();  //free(yyvsp[-7].string);;
  break;}
  case 16:
  { beginProto(yyvsp[0].string); ;
  break;}
  case 17:
  { expect(MFSTRING); ;
  break;}
  case 18:
  { endProto();  //free(yyvsp[-6].string); ;
  break;}
  case 21:
  { addEventIn(yyvsp[-1].string, yyvsp[0].string);
  //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 22:
  { addEventOut(yyvsp[-1].string, yyvsp[0].string);
  //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 23:
  { int type = addField(yyvsp[-1].string, yyvsp[0].string);
  expect(type); ;
  break;}
  case 24:
  { //free(yyvsp[-3].string); free(yyvsp[-2].string); ;
  break;}
  case 25:
  { int type = addExposedField(yyvsp[-1].string, yyvsp[0].string);
  expect(type); ;
  break;}
  case 26:
  { //free(yyvsp[-3].string); free(yyvsp[-2].string); ;
  break;}
  case 29:
  { addEventIn(yyvsp[-1].string, yyvsp[0].string);
  //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 30:
  { addEventOut(yyvsp[-1].string, yyvsp[0].string);
  //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 31:
  { addField(yyvsp[-1].string, yyvsp[0].string);
  //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 32:
  { addExposedField(yyvsp[-1].string, yyvsp[0].string);
  //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 33:
  { //free(yyvsp[-6].string); free(yyvsp[-4].string); free(yyvsp[-2].string); free(yyvsp[0].string); ;
  break;}
  case 34:
  { self->enterNode(yyvsp[0].string); ;
  break;}
  case 35:
  { self->exitNode(); //free(yyvsp[-4].string);;
  break;}
  case 38:
  { self->enterField(yyvsp[0].string); ;
  break;}
  case 39:
  { self->exitField(); //free(yyvsp[-2].string); ;
  break;}
  case 42:
  { inScript(); //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 43:
  { inScript(); //free(yyvsp[-1].string); free(yyvsp[0].string); ;
  break;}
  case 44:
  { inScript();
  int type = fieldType(yyvsp[-1].string);
  expect(type); ;
  break;}
  case 45:
  { //free(yyvsp[-3].string); free(yyvsp[-2].string); ;
  break;}
  case 46:
  { inScript(); //free(yyvsp[-3].string); free(yyvsp[-2].string); free(yyvsp[0].string); ;
  break;}
  case 47:
  { inScript(); //free(yyvsp[-3].string); free(yyvsp[-2].string); free(yyvsp[0].string); ;
  break;}
  case 49:
  {;
  break;}
  case 50:
  {     break;}
  case 55:
  {     break;}
  case 63:
  { ;
  break;}
  case 64:
  {     break;}
  case 68:
  { //free(yyvsp[0].string); ;
  break;}
  }
  /* the action file gets copied in in place of this dollarsign */


  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
    short *ssp1 = yyss - 1;
    fprintf (stderr, "state stack now");
    while (ssp1 != yyssp)
      fprintf (stderr, " %d", *++ssp1);
    fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
    yylsp->first_line = yylloc.first_line;
    yylsp->first_column = yylloc.first_column;
    yylsp->last_line = (yylsp-1)->last_line;
    yylsp->last_column = (yylsp-1)->last_column;
    yylsp->text = 0;
    }
  else
    {
    yylsp->last_line = (yylsp+yylen-1)->last_line;
    yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

  yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
    ++yynerrs;

#ifdef YYERROR_VERBOSE
    yyn = yypact[yystate];

    if (yyn > YYFLAG && yyn < YYLAST)
      {
      int size = 0;
      char *msg;
      int x, count;

      count = 0;
      /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
      for (x = (yyn < 0 ? -yyn : 0);
           x < (sizeof(yytname) / sizeof(char *)); x++)
        if (yycheck[x + yyn] == x)
          size += strlen(yytname[x]) + 15, count++;
      msg = (char *) malloc(size + 15);
      if (msg != 0)
        {
        strcpy(msg, "parse error");

        if (count < 5)
          {
          count = 0;
          for (x = (yyn < 0 ? -yyn : 0);
               x < (sizeof(yytname) / sizeof(char *)); x++)
            if (yycheck[x + yyn] == x)
              {
              strcat(msg, count == 0 ? ", expecting `" : " or `");
              strcat(msg, yytname[x]);
              strcat(msg, "'");
              count++;
              }
          }
        yyerror(msg);
        free(msg);
        }
      else
        yyerror ("parse error; also virtual memory exceeded");
      }
    else
#endif /* YYERROR_VERBOSE */
      yyerror("parse error");
    }

  goto yyerrlab1;
  yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
    /* if just tried and failed to reuse lookahead token after an error, discard it.  */

    /* return failure if at end of input */
    if (yychar == YYEOF)
      YYABORT;

#if YYDEBUG != 0
    if (yydebug)
      fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

    yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;              /* Each real token shifted decrements this */

  goto yyerrhandle;

  yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

  yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
    short *ssp1 = yyss - 1;
    fprintf (stderr, "Error: state stack now");
    while (ssp1 != yyssp)
      fprintf (stderr, " %d", *++ssp1);
    fprintf (stderr, "\n");
    }
#endif

  yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
    if (yyn == YYFLAG)
      goto yyerrpop;
    yyn = -yyn;
    goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}


void
yyerror(const char *msg)
{
  cerr << "Error near line " << currentLineNumber << ": " << msg << "\n";
  expect(0);
}

static void
beginProto(const char *protoName)
{
  // Any protos in the implementation are in a local namespace:
  VrmlNodeType::pushNameSpace();

  VrmlNodeType *t = new VrmlNodeType(protoName);
  *CurrentProtoStack += t;
}

static void
endProto()
{
  // Make any protos defined in implementation unavailable:
  VrmlNodeType::popNameSpace();

  // Add this proto definition:
  if (CurrentProtoStack->Count() == 0)
    {
    cerr << "Error: Empty PROTO stack!\n";
    }
  else
    {
    VrmlNodeType *t = CurrentProtoStack->Top();
    CurrentProtoStack->Pop();
    VrmlNodeType::addToNameSpace(t);
    }
}

static int
addField(const char *type, const char *name)
{
  return add(&VrmlNodeType::addField, type, name);
}

static int
addEventIn(const char *type, const char *name)
{
  return add(&VrmlNodeType::addEventIn, type, name);
}
static int
addEventOut(const char *type, const char *name)
{
  return add(&VrmlNodeType::addEventOut, type, name);
}
static int
addExposedField(const char *type, const char *name)
{
  return add(&VrmlNodeType::addExposedField, type, name);
}

static int
add(void (VrmlNodeType::*func)(const char *, int),
    const char *typeString, const char *name)
{
  int type = fieldType(typeString);

  if (type == 0)
    {
    cerr << "Error: invalid field type: " << type << "\n";
    }

  // Need to add support for Script nodes:
  // if (inScript) ... ???

  if (CurrentProtoStack->Count() == 0)
    {
    cerr << "Error: declaration outside of prototype\n";
    return 0;
    }
  VrmlNodeType *t = CurrentProtoStack->Top();
  (t->*func)(name, type);

  return type;
}

static int
fieldType(const char *type)
{
  if (strcmp(type, "SFBool") == 0) return SFBOOL;
  if (strcmp(type, "SFColor") == 0) return SFCOLOR;
  if (strcmp(type, "SFFloat") == 0) return SFFLOAT;
  if (strcmp(type, "SFImage") == 0) return SFIMAGE;
  if (strcmp(type, "SFInt32") == 0) return SFINT32;
  if (strcmp(type, "SFNode") == 0) return SFNODE;
  if (strcmp(type, "SFRotation") == 0) return SFROTATION;
  if (strcmp(type, "SFString") == 0) return SFSTRING;
  if (strcmp(type, "SFTime") == 0) return SFTIME;
  if (strcmp(type, "SFVec2f") == 0) return SFVEC2F;
  if (strcmp(type, "SFVec3f") == 0) return SFVEC3F;
  if (strcmp(type, "MFColor") == 0) return MFCOLOR;
  if (strcmp(type, "MFFloat") == 0) return MFFLOAT;
  if (strcmp(type, "MFInt32") == 0) return MFINT32;
  if (strcmp(type, "MFNode") == 0) return MFNODE;
  if (strcmp(type, "MFRotation") == 0) return MFROTATION;
  if (strcmp(type, "MFString") == 0) return MFSTRING;
  if (strcmp(type, "MFVec2f") == 0) return MFVEC2F;
  if (strcmp(type, "MFVec3f") == 0) return MFVEC3F;

  cerr << "Illegal field type: " << type << "\n";

  return 0;
}

static void
inScript()
{
  VrmlNodeType::FieldRec *fr = VrmlNodeType::currentField->Top();
  if (fr->nodeType == NULL ||
      strcmp(fr->nodeType->getName(), "Script") != 0)
    {
    yyerror("interface declaration outside of Script or prototype");
    }
}


static void
expect(int type)
{
  expectToken = type;
}

// End of Auto-generated Parser Code
// Begin of Auto-generated Lexer Code

/* A lexical scanner generated by flex */


#define FLEX_SCANNER

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/* cfront 1.2 defines "c_plusplus" instead of "__cplusplus" */
#ifdef c_plusplus
#ifndef __cplusplus
#define __cplusplus
#endif
#endif


#ifdef __cplusplus

/* Use prototypes in function declarations. */
#define YY_USE_PROTOS

/* The "const" storage-class-modifier is valid. */
#define YY_USE_CONST

#else   /* ! __cplusplus */

#ifdef __STDC__

#define YY_USE_PROTOS
#define YY_USE_CONST

#endif  /* __STDC__ */
#endif  /* ! __cplusplus */


#ifdef __TURBOC__
#define YY_USE_CONST
#endif


#ifndef YY_USE_CONST
#ifndef const
#define const
#endif
#endif


#ifdef YY_USE_PROTOS
#define YY_PROTO(proto) proto
#else
#define YY_PROTO(proto) ()
#endif

/* Returned upon end-of-file. */
#define YY_NULL 0

/* Promotes a possibly negative, possibly signed char to an unsigned
 * integer for use as an array index.  If the signed char is negative,
 * we want to instead treat it as an 8-bit unsigned char, hence the
 * double cast.
 */
#define YY_SC_TO_UI(c) ((unsigned int) (unsigned char) c)

/* Enter a start condition.  This macro really ought to take a parameter,
 * but we do it the disgusting crufty way forced on us by the ()-less
 * definition of BEGIN.
 */
#define BEGIN yy_start = 1 + 2 *

/* Translate the current start state into a value that can be later handed
 * to BEGIN to return to the state.
 */
#define YY_START ((yy_start - 1) / 2)

/* Action number for EOF rule of a given start state. */
#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)

/* Special action meaning "start processing a new file".  Now included
 * only for backward compatibility with previous versions of flex.
 */
#define YY_NEW_FILE yyrestart( yyin )

#define YY_END_OF_BUFFER_CHAR 0

/* Size of default input buffer. */
#define YY_BUF_SIZE 16384

typedef struct yy_buffer_state *YY_BUFFER_STATE;

extern int yyleng;
extern FILE *yyin, *yyout;

#ifdef __cplusplus
extern "C" {
#endif
  extern int yywrap YY_PROTO(( void ));
#ifdef __cplusplus
}
#endif

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

/* The funky do-while in the following #define is used to turn the definition
 * int a single C statement (which needs a semi-colon terminator).  This
 * avoids problems with code like:
 *
 *      if ( condition_holds )
 *              yyless( 5 );
 *      else
 *              do_something_else();
 *
 * Prior to using the do-while the compiler would get upset at the
 * "else" because it interpreted the "if" statement as being all
 * done when it reached the ';' after the yyless() call.
 */

/* Return all but the first 'n' matched characters back to the input stream. */

#define yyless(n) \
        do \
                { \
                /* Undo effects of setting up yytext. */ \
                *yy_cp = yy_hold_char; \
                yy_c_buf_p = yy_cp = yy_bp + n - YY_MORE_ADJ; \
                YY_DO_BEFORE_ACTION; /* set up yytext again */ \
                } \
        while ( 0 )


struct yy_buffer_state
{
  FILE *yy_input_file;

  char *yy_ch_buf;                /* input buffer */
  char *yy_buf_pos;               /* current position in input buffer */

  /* Size of input buffer in bytes, not including room for EOB
   * characters.
   */
  int yy_buf_size;

  /* Number of characters read into yy_ch_buf, not including EOB
   * characters.
   */
  int yy_n_chars;

  /* Whether this is an "interactive" input source; if so, and
   * if we're using stdio for input, then we want to use getc()
   * instead of fread(), to make sure we stop fetching input after
   * each newline.
   */
  int yy_is_interactive;

  /* Whether to try to fill the input buffer when we reach the
   * end of it.
   */
  int yy_fill_buffer;

  int yy_buffer_status;
#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
  /* When an EOF's been seen but there's still some text to process
   * then we mark the buffer as YY_EOF_PENDING, to indicate that we
   * shouldn't try reading from the input source any more.  We might
   * still have a bunch of tokens to match, though, because of
   * possible backing-up.
   *
   * When we actually see the EOF, we change the status to "new"
   * (via yyrestart()), so that the user can continue scanning by
   * just pointing yyin at a new input file.
   */
#define YY_BUFFER_EOF_PENDING 2
};

static YY_BUFFER_STATE yy_current_buffer = 0;

/* We provide macros for accessing buffer states in case in the
 * future we want to put the buffer states in a more general
 * "scanner state".
 */
#define YY_CURRENT_BUFFER yy_current_buffer


/* yy_hold_char holds the character lost when yytext is formed. */
static char yy_hold_char;

static int yy_n_chars;          /* number of characters read into yy_ch_buf */


int yyleng;

/* Points to current character in buffer. */
static char *yy_c_buf_p = (char *) 0;
static int yy_init = 1;         /* whether we need to initialize */
static int yy_start = 0;        /* start state number */

/* Flag which is used to allow yywrap()'s to do buffer switches
 * instead of setting up a fresh yyin.  A bit of a hack ...
 */
static int yy_did_buffer_switch_on_eof;

void yyrestart YY_PROTO(( FILE *input_file ));
void yy_switch_to_buffer YY_PROTO(( YY_BUFFER_STATE new_buffer ));
void yy_load_buffer_state YY_PROTO(( void ));
YY_BUFFER_STATE yy_create_buffer YY_PROTO(( FILE *file, int size ));
void yy_delete_buffer YY_PROTO(( YY_BUFFER_STATE b ));
void yy_init_buffer YY_PROTO(( YY_BUFFER_STATE b, FILE *file ));

static void *yy_flex_alloc YY_PROTO(( unsigned int ));
static void *yy_flex_realloc YY_PROTO(( void *, unsigned int ));
static void yy_flex_free YY_PROTO(( void * ));

#define yy_new_buffer yy_create_buffer

#define INITIAL 0
#define NODE 1
#define SFB 2
#define SFC 3
#define SFF 4
#define SFIMG 5
#define SFI 6
#define SFR 7
#define SFS 8
#define SFT 9
#define SFV2 10
#define SFV3 11
#define MFC 12
#define MFF 13
#define MFI 14
#define MFR 15
#define MFS 16
#define MFV2 17
#define MFV3 18
#define IN_SFS 19
#define IN_MFS 20
#define IN_SFIMG 21
typedef unsigned char YY_CHAR;
typedef int yy_state_type;

#define FLEX_DEBUG
FILE *yyin = (FILE *) 0, *yyout = (FILE *) 0;
extern char *yytext;
#define yytext_ptr yytext

#ifndef yytext_ptr
static void yy_flex_strncpy YY_PROTO(( char *, const char *, int ));
#endif

static yy_state_type yy_get_previous_state YY_PROTO(( void ));
static yy_state_type yy_try_NUL_trans YY_PROTO(( yy_state_type current_state ));
static int yy_get_next_buffer YY_PROTO(( void ));
static void yy_fatal_error YY_PROTO(( const char msg[] ));

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up yytext.
 */
#define YY_DO_BEFORE_ACTION \
        yytext_ptr = yy_bp; \
        yyleng = yy_cp - yy_bp; \
        yy_hold_char = *yy_cp; \
        *yy_cp = '\0'; \
        yy_c_buf_p = yy_cp;

#define YY_END_OF_BUFFER 50
static const short int yy_accept[949] =
{   0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   50,   48,   46,   47,   46,   14,
    46,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   48,   48,   48,   48,   48,   48,   48,   48,   25,
    48,   48,   48,   48,   23,   23,   48,   48,   48,   38,
    36,   38,   38,   48,   48,   35,   48,   48,   48,   48,
    48,   48,   48,   48,   48,   48,   19,   20,   48,   48,

    26,   17,   48,   24,   24,   18,   48,   48,   48,   39,
    37,   39,   39,   48,   48,   48,   48,   48,   48,   41,
    41,   42,   41,   41,   43,   48,   45,   45,   46,   47,
    46,   47,   47,   46,   46,   46,   46,   14,   14,   14,
    7,   14,   14,   14,    6,   14,   14,   14,   14,    0,
    15,    0,    0,    0,    0,    0,    0,    0,    0,   25,
    25,    0,    0,    0,    0,    0,   23,   23,    0,    0,
    0,    0,    0,    0,   38,   38,   38,   15,    0,   35,
    35,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

    16,    0,   26,   26,    0,   24,   24,    0,    0,    0,
    0,    0,    0,   39,   39,   39,   16,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   41,
    41,   41,   41,   41,   41,   40,   45,   45,   47,   47,
    47,   46,    4,   14,   14,   14,   14,    5,   14,   14,
    14,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   25,    0,   44,   44,    0,
    0,    0,    0,   44,   44,    0,   23,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   35,
    0,    0,   27,    0,    0,    0,    0,    0,   27,    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   26,   24,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   28,
    0,    0,    0,    0,    0,   28,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   45,
    46,   14,    9,   14,   14,   14,   14,   14,    0,   21,
    0,    0,    0,    0,    0,    0,    0,    0,   44,   44,
    0,    0,    0,    0,    0,    0,    0,    0,   27,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   28,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   46,   14,    2,    8,
    14,   14,   12,   22,    0,    0,   33,    0,    0,    0,
    0,    0,   33,    0,    0,    0,    0,   33,    0,    0,
    0,   33,    0,    0,    0,    0,    0,   33,    0,    0,
    0,   44,   44,   44,   44,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   27,    0,    0,   27,    0,    0,   29,

    0,    0,    0,    0,    0,   29,    0,    0,    0,    0,
    29,    0,    0,    0,   29,    0,    0,    0,    0,    0,
    29,    0,    0,    0,    0,    0,   34,    0,    0,    0,
    0,    0,   34,    0,    0,    0,    0,   34,    0,    0,
    0,   34,    0,    0,    0,    0,    0,   34,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   28,
    0,    0,   28,    0,    0,   30,    0,    0,    0,    0,
    0,   30,    0,    0,    0,    0,   30,    0,    0,    0,

    30,    0,    0,    0,    0,    0,   30,    0,    0,    0,
    46,   14,   14,   14,   14,   33,    0,    0,   33,    0,
    33,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   29,    0,    0,   29,    0,   29,    0,    0,
    34,    0,    0,   34,    0,   34,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   30,    0,    0,
    30,    0,   30,    0,    0,   46,   14,   10,   14,   14,
    0,   33,    0,    0,   33,    0,    0,   33,    0,   33,

    0,    0,   33,    0,    0,   31,    0,    0,    0,    0,
    0,   31,    0,    0,    0,    0,   31,    0,    0,   31,
    0,    0,    0,    0,    0,   31,    0,    0,    0,    0,
    0,   31,    0,    0,    0,   31,    0,    0,    0,    0,
    0,   31,    0,    0,    0,    0,    0,    0,    0,   31,
    0,    0,    0,    0,   29,    0,    0,   29,    0,    0,
    29,    0,   29,    0,    0,   29,    0,   34,    0,    0,
    34,    0,    0,   34,    0,   34,    0,    0,   34,    0,
    0,   32,    0,    0,    0,    0,    0,   32,    0,    0,
    0,    0,   32,    0,    0,   32,    0,    0,    0,    0,

    0,   32,    0,    0,    0,    0,    0,   32,    0,    0,
    0,   32,    0,    0,    0,    0,    0,   32,    0,    0,
    0,    0,    0,    0,    0,   32,    0,    0,    0,    0,
    30,    0,    0,   30,    0,    0,   30,    0,   30,    0,
    0,   30,   46,   14,   11,   14,   31,    0,    0,   31,
    0,   31,    0,    0,   31,    0,   31,    0,    0,   31,
    0,   32,    0,    0,   32,    0,   32,    0,    0,   32,
    0,   32,    0,    0,   32,    0,   46,   14,   14,    0,
    31,    0,    0,   31,    0,    0,   31,    0,   31,    0,
    0,   31,    0,    0,   31,    0,   31,    0,    0,   31,

    0,    0,   31,    0,   32,    0,    0,   32,    0,    0,
    32,    0,   32,    0,    0,   32,    0,    0,   32,    0,
    32,    0,    0,   32,    0,    0,   32,   46,   14,   14,
    46,    3,   14,   46,   13,   46,   46,   46,   46,   46,
    1,   46,    1,    1,    1,    1,    1,    0
} ;

static const int yy_ec[256] =
{   0,
    1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
    1,    1,    2,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    4,    5,    6,    7,    5,    5,    5,    1,    5,
    5,    5,    8,    2,    9,   10,    5,   11,   12,   13,
    12,   12,   12,   12,   12,   14,   12,    5,    5,    5,
    5,    5,    5,    5,   15,   16,   16,   17,   18,   19,
    5,    5,   20,    5,    5,   21,   22,   23,   24,   25,
    5,   26,   27,   28,   29,   30,    5,   31,    5,    5,
    32,   33,   34,    5,    5,    5,   16,   16,   16,   35,

    36,   37,    5,    5,   38,    5,    5,   39,    5,   40,
    41,   42,    5,    5,   43,   44,   45,   46,    5,   47,
    5,    5,    1,    5,    1,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,

    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5
} ;

static const int yy_meta[48] =
{   0,
    1,    2,    3,    2,    4,    5,    6,    4,    4,    1,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    4,    8,    1,    8,    7,    7,    7,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4
} ;

static const short int yy_base[1090] =
{   0,
    0,    6,   13,    0,   59,   65,   92,    0,  104,  110,
    116,  122,  128,  134,  140,  146,  159,  165,  164,  177,
    183,  189,  195,  201,  207,  233,  259,  285,  311,  337,
    363,  389,  422,  455,  481,  507,  533,  559,  222,  228,
    256,  274,  324,  350,  787, 7663,   73,   79,  280,    0,
    247,  768,  753,  751,  745,  726,  725,  719,  715,   18,
    685,  702,  687,  682,   78,   96,  592,  170,  289,  368,
    300,  627,   51,  315,  380,  328,  342,  354,  673,    0,
    7663,  410,  677,  420,  376,  427,  436,  394,  708,  454,
    458,  743,  466,  470,  778,  676, 7663, 7663,  486,  491,

    496, 7663,  511,  517,  522, 7663,  544,  538,  813,    0,
    7663,  579,  665,  549,  563,  848,  604,  576,  883,    0,
    605, 7663,  617,  672, 7663,  631,  635,  639,  652,  302,
    658,  665,  666,  686,  692,  698,  725,    0,  652,  636,
    0,  636,  613,  607,    0,  617,  597,  590,  591,  605,
    7663,  596,  722,    0,  727,  918,  931,  751,  755,  792,
    759,  821,    0,  944,  957,  805,  786,    0,  825,    0,
    830,  970,  983,  856,    0,  869,  875,    0,  834,  900,
    987,  891,  895,    0,  996, 1022, 1035, 1003, 1007,    0,
    1040, 1053, 1066, 1073, 1077,    0, 1082, 1095, 1108, 1115,

    7663, 1119, 1124, 1132, 1140, 1144,    0, 1150,    0, 1159,
    1172, 1185, 1192,    0, 1205, 1211,    0, 1208,    0, 1213,
    1244, 1257, 1220, 1261, 1274, 1283, 1309, 1322, 1290,    0,
    1335, 1341, 1347, 1353, 1359, 7663, 1294,    0, 1234, 1365,
    1366, 1372,    0,  604,  580,  572,  569,    0,  552,  537,
    501,  511,  519, 1370, 1374, 1393, 1428, 1441,    0,    0,
    0, 1410, 1378, 1454, 1407, 1411, 1458, 1462, 1466, 1508,
    1521, 1534, 1547, 1560,  191, 1479,    0, 1477, 1483, 1606,
    1641, 1654,    0,    0,    0, 1575, 1487, 1667, 1491, 1572,
    1584, 1576, 1623, 1680,    0,    0,    0,    0,  230, 1588,

    1693, 1615, 1592, 1706, 1741,    0,    0,    0,    0, 1723,
    1720, 1758, 1725, 1762, 1775, 1810,    0,    0,    0,    0,
    1792, 1789, 1827, 1793, 1831,    0, 1836, 1840, 1853, 1888,
    0,    0,    0,    0, 1870, 1867, 1905, 1872, 1909, 1914,
    1949,    0,    0,    0,    0,  238, 1922, 1962, 1927, 1931,
    1975, 2010,    0,    0,    0,    0, 1992, 1935, 2027,    0,
    2040,  500,    0,  488,  493,  444,  443,  450,  456, 7663,
    1990, 2060, 2073, 2037, 2086, 2099, 2112, 2044,    0, 2116,
    1995, 2162, 2175, 2128, 2188, 2201, 2214, 2227, 2132, 2146,
    2240, 2253, 2288, 2301, 2261, 2314, 2327, 2340, 2353, 2366,

    2401, 2414, 2268, 2427, 2440, 2453, 2466, 2479, 2514, 2527,
    2374, 2540, 2553, 2566, 2579, 2383, 2487, 2592, 2605, 2640,
    2653, 2494, 2666, 2679, 2692, 2705, 2281,  437,    0,    0,
    50,  386,    0, 7663, 2379, 2498, 2622, 2718, 2731,    0,
    0,    0,  239, 2613, 2744, 2757, 2770, 2783, 2818, 2792,
    2617, 2831, 2866,    0,    0,    0,    0,  272, 2796, 2800,
    2845,    0, 2881,  205,  285, 2840, 2804, 2927, 2962, 2975,
    0,    0,    0, 2944, 2941, 2988, 3001, 3014, 3027, 3062,
    2946, 3035, 3075, 3110,    0,    0,    0,    0, 3092, 3039,
    3043, 3127, 3047, 3089, 3162, 3175,  400, 3094, 3135, 3144,

    3188, 3201,    0,    0,    0,  417, 3139, 3214, 3227, 3240,
    3253, 3288, 3262, 3266, 3301, 3336,    0,    0,    0,    0,
    437, 3270, 3274, 3315, 3310, 3341, 3346, 3381, 3394,    0,
    0,    0,  548, 3354, 3407, 3420, 3433, 3446, 3481, 3359,
    3363, 3494, 3529,    0,    0,    0,    0,  562, 3367, 3454,
    3463, 3503, 3458, 3542, 3577, 3590,    0,    0,    0, 3559,
    3507, 3603, 3616, 3629, 3642, 3677, 3512, 3556, 3690, 3725,
    0,    0,    0,    0, 3659, 3560, 3656, 3738, 3660, 3698,
    3773, 3786,  645, 3703, 3707, 3755, 3799, 3812,    0,    0,
    0,  661, 3711, 3825, 3838, 3851, 3864, 3899, 3747, 3872,

    3912, 3947,    0,    0,    0,    0,  680, 3876, 3880, 3926,
    3925,  390,  370,  351,  323, 3952, 3960, 3987, 4000, 4035,
    4048, 3967, 4083, 4096, 4131, 4144, 4008, 4157, 4170, 4183,
    4196, 4209, 4244, 4257, 4270, 4283, 4296, 4331, 4015, 4344,
    4357, 4370, 4065, 4056, 4383, 4396, 4431, 4444, 4104, 4479,
    4113, 4217, 4492, 4505, 4540, 4553, 4224, 4588, 4601, 4636,
    4649, 4304, 4662, 4675, 4688, 4701, 4714, 4749, 4762, 4775,
    4788, 4801, 4836, 4311, 4849, 4862, 4875, 4413, 4404, 4888,
    4901, 4936, 4949, 4452, 4984, 2858,  304,    0,  274,  296,
    3884, 3971, 4997, 5010,  688, 5023, 5036, 4522, 4019, 5049,

    5062, 5075,  689, 4061, 4108, 4461, 5088, 5101,    0,    0,
    0,  695, 4228, 5114, 5127, 5140, 5153, 4316, 4408, 5188,
    5223, 5236,    0,    0,    0,  712, 4456, 4519, 4570, 5249,
    5262, 5275, 5310, 4524, 4561, 5323, 5358,    0,    0,    0,
    0,  740, 4565, 4609, 4618, 4613, 5371, 5384, 5397, 5410,
    4722, 4726, 4731, 4809, 4813, 5445, 5458,  757, 5471, 5484,
    4918, 4817, 5497, 5510, 5523,  765, 4821, 4915, 5536, 5549,
    838, 5562, 5575, 4966, 4919, 5588, 5601, 5614,  862, 4964,
    4968, 5167, 5627, 5640,    0,    0,    0,  931, 5161, 5653,
    5666, 5679, 5692, 5197, 5201, 5727, 5762, 5775,    0,    0,

    0,  986, 5205, 5209, 5292, 5788, 5801, 5814, 5849, 5284,
    5331, 5862, 5897,    0,    0,    0,    0, 1022, 5335, 5339,
    5427, 5343, 5910, 5923, 5936, 5949, 5418, 5422, 5706, 5700,
    5735, 5984, 5997, 1023, 6010, 6023, 5831, 5739, 6036, 6049,
    6062, 1025, 5752,  268,    0,  250, 5879, 5828, 6075, 6088,
    6123, 6136, 5870, 6171, 6184, 6219, 6232, 5957, 6267, 6280,
    6315, 5966, 6096, 6103, 5833, 6144, 5875, 6151, 6192, 5962,
    6199, 6108, 6240, 6247, 6156, 6288, 6301,  236,  204, 6203,
    6251, 6328,    0, 1065,    3,  277, 6341, 6340, 6376, 6389,
    0, 1123,  376,  411, 6402, 6344, 6437, 6450,    0, 1129,

    435,  443, 6463, 6349, 6353, 6357, 6361, 6401, 6410, 6414,
    6464, 6418, 6499, 6422, 6472, 6478, 6504, 6508, 6513, 6521,
    6525, 6529, 6533, 6539, 6547, 6551, 6556, 3328,  213,  194,
    6569,    0,  188, 6575,    0, 6491, 3939, 5180, 5719, 6307,
    6579, 6586, 6592, 6593, 6599, 6600, 6606, 7663, 6619, 6627,
    6635, 6643, 6651, 6656, 6663, 6671, 6679, 6686, 6694, 6702,
    6710, 6718, 6726, 6733,  163, 6740, 6748, 6756, 6764,  157,
    6772, 6780, 6788, 6796, 6804,  104, 6812, 6820, 6825, 6832,
    6840, 6847,   93, 6854,   90, 6862, 6870,   66, 6878, 6886,
    6894, 6902, 6910, 6918, 6926, 6934, 6942, 6950, 6958, 6966,

    6974, 6982, 6990, 6998, 7006, 7011, 7018, 7026, 7034,   64,
    7042, 7050, 7058, 7066, 7074, 7082, 7090, 7098, 7106, 7114,
    7122, 7130, 7138, 7146, 7154, 7162, 7170, 7178, 7183, 7190,
    7198, 7206, 7214, 7222, 7230, 7238, 7246, 7254, 7262, 7270,
    7278, 7286, 7294, 7302, 7310, 7318, 7326, 7334, 7342, 7350,
    7358, 7366, 7374, 7382, 7387, 7394, 7402, 7410, 7418, 7426,
    7434, 7442, 7450, 7458, 7466, 7474, 7482, 7490, 7498, 7506,
    7514, 7522, 7530, 7538, 7546, 7554, 7562, 7570, 7578, 7586,
    7591, 7598, 7606, 7614, 7622, 7630, 7638, 7646, 7654
} ;

static const short int yy_def[1090] =
{   0,
    949,  949,  948,    3,  949,  949,  949,    7,    7,    7,
    7,    7,    7,    7,    7,    7,  950,  950,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,  951,  951,    7,    7,    7,    7,  952,  952,
    952,  952,    7,    7,  948,  948,  948,  948,  953,  954,
    953,  954,  954,  954,  954,  954,  954,  954,  954,  954,
    954,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,   72,  948,  948,  948,  948,  948,  948,  955,
    948,  956,  955,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,

    948,  948,  948,  948,  948,  948,  948,  948,  948,  957,
    948,  958,  957,  948,  948,  948,  948,  948,  948,  959,
    959,  948,  960,  959,  948,  948,  948,  948,  948,  948,
    953,  948,  961,  953,  953,  953,  953,  954,  954,  954,
    954,  954,  954,  954,  954,  954,  954,  954,  954,  948,
    948,  948,  948,   67,   67,  948,  962,  948,  948,  948,
    948,  948,   73,  948,  963,  964,  948,  965,  948,   79,
    79,  948,  966,  948,  955,  956,  956,  955,  948,  948,
    948,  948,  948,   89,   89,  948,  967,  948,  948,   92,
    92,  948,  968,  948,  948,   95,   95,  948,  969,  948,

    948,  948,  948,  948,  948,  948,  970,  948,  109,  109,
    948,  971,  948,  957,  958,  958,  957,  948,  116,  116,
    948,  972,  948,  948,  948,  225,  948,  973,  948,  974,
    974,  975,  975,  975,  975,  948,  948,  976,  977,  977,
    977,  978,  979,  979,  979,  979,  979,  979,  979,  979,
    979,  948,  948,  948,  948,  948,  980,  980,  258,  258,
    258,  258,  948,  948,  948,  948,  948,  948,  948,  981,
    981,  981,  981,  981,  274,  982,  983,  948,  948,  948,
    984,  984,  282,  282,  282,  282,  948,  948,  948,  948,
    948,  948,  948,  967,  294,  294,  294,  294,  294,  948,

    948,  948,  948,  948,  968,  305,  305,  305,  305,  305,
    948,  948,  948,  948,  948,  969,  316,  316,  316,  316,
    316,  948,  948,  948,  948,  985,  948,  948,  948,  971,
    330,  330,  330,  330,  330,  948,  948,  948,  948,  948,
    972,  341,  341,  341,  341,  341,  948,  948,  948,  948,
    948,  973,  352,  352,  352,  352,  352,  948,  948,  976,
    978,  979,  979,  979,  979,  979,  979,  979,  948,  948,
    256,  948,  986,  948,  258,  948,  987,  258,  988,  274,
    280,  948,  989,  948,  282,  948,  990,  385,  948,  948,
    991,  948,  948,  992,  948,  993,  948,  994,  993,  948,

    948,  995,  948,  996,  948,  997,  996,  948,  948,  998,
    948,  999,  948, 1000,  999,  948,  948, 1001,  948,  948,
    1002,  948, 1003,  948, 1004, 1003, 1005, 1006, 1006, 1006,
    1006, 1006, 1006,  948,  948,  948,  948, 1007, 1007,  439,
    439,  439,  439,  948,  948, 1008, 1008,  447, 1009,  948,
    948,  948, 1009,  453,  453,  453,  453,  453,  447,  447,
    448, 1010,  948,  463,  463,  948,  948,  948, 1011, 1011,
    470,  470,  470,  470,  948,  948, 1012, 1012,  478, 1013,
    948,  948,  948, 1013,  484,  484,  484,  484,  484,  478,
    478,  478,  948,  948, 1014, 1014,  496,  948,  948,  948,

    1015, 1015,  502,  502,  502,  502,  948,  948, 1016, 1016,
    510, 1017,  948,  948,  948, 1017,  516,  516,  516,  516,
    516,  510,  510,  511,  948,  948,  948, 1018, 1018,  529,
    529,  529,  529,  948,  948, 1019, 1019,  537, 1020,  948,
    948,  948, 1020,  543,  543,  543,  543,  543,  537,  537,
    538,  948,  948,  948, 1021, 1021,  556,  556,  556,  556,
    948,  948, 1022, 1022,  564, 1023,  948,  948,  948, 1023,
    570,  570,  570,  570,  570,  564,  564,  564,  948,  948,
    1024, 1024,  582,  948,  948,  948, 1025, 1025,  588,  588,
    588,  588,  948,  948, 1026, 1026,  596, 1027,  948,  948,

    948, 1027,  602,  602,  602,  602,  602,  596,  596,  597,
    1028, 1029, 1029, 1029, 1029,  948,  948, 1030, 1031, 1031,
    948,  948, 1032,  948,  948, 1033,  948, 1034,  948, 1035,
    1034, 1036, 1036,  948, 1037, 1036,  948, 1035,  948, 1038,
    1037, 1038,  948,  948, 1039, 1040, 1040,  948,  948, 1041,
    948,  948, 1042, 1043, 1043,  948,  948, 1044,  948,  948,
    1045,  948, 1046,  948, 1047, 1046, 1048, 1048,  948, 1049,
    1048,  948, 1047,  948, 1050, 1049, 1050,  948,  948, 1051,
    1052, 1052,  948,  948, 1053, 1054, 1055, 1055, 1055, 1055,
    948,  948, 1056, 1056,  694, 1057, 1057,  697,  948,  948,

    1058, 1058,  702,  948,  948,  948, 1059, 1059,  708,  708,
    708,  708,  948,  948, 1060, 1060,  716,  948,  948,  948,
    1061, 1061,  722,  722,  722,  722,  716,  716,  717, 1062,
    1062,  731, 1063,  948,  948,  948, 1063,  737,  737,  737,
    737,  737,  731,  731,  732,  948,  948, 1064, 1064,  749,
    749,  749,  750,  948,  948, 1065, 1065,  757, 1066, 1066,
    760,  948,  948, 1067, 1067,  765,  948,  948, 1068, 1068,
    770, 1069, 1069,  773,  948,  948, 1070, 1070,  778,  948,
    948,  948, 1071, 1071,  784,  784,  784,  784,  948,  948,
    1072, 1072,  792,  948,  948,  948, 1073, 1073,  798,  798,

    798,  798,  792,  792,  793, 1074, 1074,  807, 1075,  948,
    948,  948, 1075,  813,  813,  813,  813,  813,  807,  807,
    808,  948,  948, 1076, 1076,  825,  825,  825,  826,  948,
    948, 1077, 1077,  833, 1078, 1078,  836,  948,  948, 1079,
    1079,  841, 1080, 1081, 1081, 1081,  948,  948, 1082, 1083,
    1083,  948,  948, 1084, 1085, 1085,  948,  948, 1086, 1087,
    1087,  948,  948,  784,  793,  792,  796,  948,  798,  808,
    807,  812,  948,  813,  826,  825, 1080, 1081, 1081,  948,
    948, 1082,  882,  882,  851,  851,  851,  948,  948, 1084,
    890,  890,  856,  856,  856,  948,  948, 1086,  898,  898,

    861,  861,  861,  948,  948,  784,  784,  784,  792,  792,
    793,  948,  948,  798,  798,  798,  807,  807,  808,  948,
    823,  813,  813,  813,  825,  825,  826, 1080, 1081, 1081,
    1080, 1081, 1081, 1080, 1081, 1080, 1080, 1088, 1088, 1088,
    948, 1088,  948, 1089, 1089, 1089, 1089,    0,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,

    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948
} ;

static const short int yy_nxt[7711] =
{   0,
    948,   47,   48,   47,  948,  948,   49,   47,   48,   47,
    469,  472,   49,   46,   47,   48,   47,   50,   46,   51,
    46,   46,   46,   46,   46,   46,   46,   50,   50,   52,
    53,   50,   54,   50,   50,   55,   50,   56,   57,   50,
    58,   59,   50,   50,   46,   46,   46,   50,   60,   61,
    50,   50,   50,   50,   50,   50,   50,   50,   50,   50,
    47,   48,   47,  147,  148,   51,   47,   48,   47,  613,
    462,   51,  462,  614,  129,  130,  129,   62,   63,  131,
    132,  948,  132,   62,   63,  133,   64,  153,  154,  154,
    154,  154,   64,   47,   48,   47,  326,  948,   51,  277,

    65,   66,   67,   67,   67,   67,  155,  155,  155,  155,
    360,   63,   68,   69,   70,   70,   70,   70,   68,   69,
    70,   70,   70,   70,   71,   46,   72,   73,   73,   73,
    71,   46,   72,   73,   73,   73,   74,   46,   75,   76,
    76,   76,   74,   46,   75,   76,   76,   76,   77,   78,
    79,   79,   79,   79,   77,   78,   79,   79,   79,   79,
    47,   48,   47,  326,   81,   82,   47,   48,   47,  277,
    81,   82,   84,   85,   86,   86,   86,   86,   83,  159,
    160,  160,  160,  160,   83,   84,   85,   86,   86,   86,
    86,   87,   88,   89,   89,   89,   89,   87,   88,   89,

    89,   89,   89,   90,   91,   92,   92,   92,   92,   90,
    91,   92,   92,   92,   92,   93,   94,   95,   95,   95,
    95,  270,  935,  121,   48,  121,   96,  122,  123,  121,
    48,  121,  933,  122,  123,  270,  932,  270,   97,  930,
    98,   93,   94,   95,   95,   95,   95,  391,  135,  130,
    135,  270,   96,  136,  124,  418,  618,  121,   48,  121,
    124,  125,  123,  929,   97,  391,   98,   99,  100,  101,
    101,  101,  101,  418,  618,  121,   48,  121,   96,  125,
    123,  135,  130,  135,  469,  472,  136,  879,  124,  623,
    102,  878,   98,   99,  100,  101,  101,  101,  101,  161,

    161,  161,  161,  132,   96,  132,  124,  623,  133,  137,
    163,  163,  163,  163,  846,  270,  102,  845,   98,  103,
    46,  104,  105,  105,  105,  167,  167,  167,  167,  844,
    96,  270,  126,   46,  127,  128,  128,  128,  167,  167,
    167,  167,  106,   46,   98,  103,   46,  104,  105,  105,
    105,  169,  170,  170,  170,  170,   96,  690,  126,   46,
    127,  128,  128,  128,  171,  171,  171,  171,  106,   46,
    98,  107,  108,  109,  109,  109,  109,  159,  160,  160,
    160,  160,   96,  281,  284,  162,  181,  181,  181,  181,
    167,  167,  167,  167,  102,  689,   98,  107,  108,  109,

    109,  109,  109,  162,  185,  185,  185,  185,   96,  688,
    168,  135,  130,  135,  687,  134,  177,  391,  281,  284,
    102,  615,   98,   47,   48,   47,  168,  111,  112,  179,
    180,  180,  180,  180,  645,  391,  179,  180,  180,  180,
    180,  113,  484,  487,  182,  183,  184,  184,  184,  184,
    484,  487,  645,  102,  650,   98,   47,   48,   47,  612,
    111,  112,  182,  189,  190,  190,  190,  190,  191,  191,
    191,  191,  650,  434,  113,  195,  196,  196,  196,  196,
    197,  197,  197,  197,  433,  432,  102,  431,   98,  114,
    115,  116,  116,  116,  116,  202,  203,  203,  203,  203,

    96,  204,  204,  204,  204,  202,  203,  203,  203,  203,
    430,  429,  102,  205,   98,  114,  115,  116,  116,  116,
    116,  206,  206,  206,  206,  428,   96,  206,  206,  206,
    206,  205,  206,  206,  206,  206,  370,  369,  102,  368,
    98,  117,  118,  119,  119,  119,  119,  207,  210,  210,
    210,  210,   96,  208,  209,  209,  209,  209,  218,  219,
    219,  219,  219,  207,   97,  653,   98,  117,  118,  119,
    119,  119,  119,  220,  220,  220,  220,  367,   96,  658,
    135,  130,  135,  653,  134,  216,  226,  226,  226,  226,
    97,  366,   98,  156,  156,  156,  365,  658,  157,  364,

    363,  153,  154,  154,  154,  154,  231,  130,  231,  158,
    134,  232,  134,  224,  225,  225,  225,  225,  234,  130,
    234,  362,  134,  235,  253,  252,  251,  158,  164,  164,
    164,  250,  249,  165,  248,  247,  246,  163,  163,  163,
    163,  237,  237,  237,  237,  237,  237,  237,  237,  237,
    237,  237,  237,  129,  130,  129,  245,  166,  131,  135,
    130,  135,  418,  244,  136,  238,  132,  240,  132,  240,
    243,  133,  241,  166,  172,  172,  172,  236,  680,  173,
    418,  238,  169,  170,  170,  170,  170,  135,  130,  135,
    174,  217,  136,  135,  130,  135,  680,  685,  136,  135,

    130,  135,  201,  178,  136,  618,  623,  152,  174,  186,
    186,  186,  849,  151,  187,  685,  150,  183,  184,  184,
    184,  184,  149,  618,  623,  188,  135,  130,  135,  854,
    849,  136,  155,  155,  155,  155,  948,  155,  155,  155,
    155,  146,  145,  188,  192,  192,  192,  854,  144,  193,
    242,  143,  189,  190,  190,  190,  190,  859,  263,  263,
    194,  264,  264,  264,  264,  161,  161,  161,  161,  161,
    161,  161,  161,  142,  645,  859,  162,  141,  194,  198,
    198,  198,  650,  140,  199,  139,  948,  195,  196,  196,
    196,  196,  645,  948,  162,  200,  167,  167,  167,  167,

    650,  159,  160,  160,  160,  160,  164,  164,  164,  162,
    948,  165,  948,  200,  211,  211,  211,  948,  948,  212,
    948,  948,  208,  209,  209,  209,  209,  162,  265,  265,
    213,  266,  266,  266,  266,  171,  171,  171,  171,  948,
    171,  171,  171,  171,  181,  181,  181,  181,  213,  221,
    221,  221,  948,  948,  222,  653,  948,  218,  219,  219,
    219,  219,  948,  287,  287,  223,  288,  288,  288,  288,
    135,  130,  135,  653,  134,  177,  135,  130,  135,  658,
    134,  177,  948,  223,  227,  227,  227,  948,  948,  228,
    948,  948,  224,  225,  225,  225,  225,  658,  289,  289,

    229,  290,  290,  290,  290,  185,  185,  185,  185,  179,
    180,  180,  180,  180,  948,  948,  948,  182,  229,  156,
    156,  156,  948,  948,  157,  948,  254,  255,  256,  256,
    256,  256,  258,  156,  258,  182,  948,  259,  948,  260,
    261,  262,  262,  262,  262,  164,  164,  164,  864,  948,
    165,  948,  267,  948,  268,  269,  269,  269,  271,  164,
    271,  948,  948,  272,  948,  273,  864,  274,  275,  275,
    275,  172,  172,  172,  948,  948,  173,  948,  278,  279,
    280,  280,  280,  280,  282,  172,  282,  948,  948,  283,
    948,  284,  285,  286,  286,  286,  286,  181,  181,  181,

    181,  948,  948,  869,  182,  948,  185,  185,  185,  185,
    300,  300,  948,  301,  301,  301,  301,  191,  191,  191,
    191,  869,  182,  186,  186,  186,  948,  948,  187,  948,
    291,  292,  293,  293,  293,  293,  295,  186,  295,  874,
    680,  296,  685,  297,  298,  299,  299,  299,  299,  948,
    191,  191,  191,  191,  192,  192,  192,  874,  680,  193,
    685,  302,  303,  304,  304,  304,  304,  306,  192,  306,
    948,  948,  307,  948,  308,  309,  310,  310,  310,  310,
    311,  311,  849,  312,  312,  312,  312,  197,  197,  197,
    197,  948,  197,  197,  197,  197,  198,  198,  198,  948,

    849,  199,  948,  313,  314,  315,  315,  315,  315,  317,
    198,  317,  948,  948,  318,  948,  319,  320,  321,  321,
    321,  321,  322,  322,  948,  323,  323,  323,  323,  204,
    204,  204,  204,  202,  203,  203,  203,  203,  948,  948,
    854,  205,  204,  204,  204,  204,  859,  324,  324,  205,
    325,  325,  325,  325,  206,  206,  206,  206,  854,  205,
    210,  210,  210,  210,  859,  948,  948,  205,  948,  210,
    210,  210,  210,  211,  211,  211,  948,  948,  212,  948,
    327,  328,  329,  329,  329,  329,  331,  211,  331,  948,
    948,  332,  948,  333,  334,  335,  335,  335,  335,  336,

    336,  948,  337,  337,  337,  337,  135,  130,  135,  948,
    134,  216,  135,  130,  135,  948,  134,  216,  220,  220,
    220,  220,  948,  220,  220,  220,  220,  347,  347,  948,
    348,  348,  348,  348,  948,  240,  134,  240,  134,  948,
    241,  948,  134,  948,  134,  221,  221,  221,  948,  948,
    222,  948,  338,  339,  340,  340,  340,  340,  342,  221,
    342,  948,  948,  343,  948,  344,  345,  346,  346,  346,
    346,  226,  226,  226,  226,  227,  227,  227,  948,  948,
    228,  948,  948,  224,  225,  225,  225,  225,  948,  948,
    948,  229,  948,  226,  226,  226,  226,  358,  358,  948,

    359,  359,  359,  359,  237,  237,  237,  237,  948,  229,
    227,  227,  227,  948,  948,  228,  948,  349,  350,  351,
    351,  351,  351,  353,  227,  353,  948,  948,  354,  948,
    355,  356,  357,  357,  357,  357,  231,  130,  231,  948,
    948,  232,  234,  130,  234,  948,  134,  235,  234,  130,
    234,  948,  134,  235,  234,  130,  234,  948,  134,  235,
    234,  130,  234,  948,  134,  235,  240,  240,  240,  240,
    948,  241,  241,  135,  130,  135,  948,  948,  136,  255,
    256,  256,  256,  256,  371,  371,  371,  371,  264,  264,
    264,  264,  948,  361,  372,  372,  372,  948,  948,  373,

    948,  948,  255,  256,  256,  256,  256,  948,  948,  948,
    374,  375,  376,  375,  948,  948,  377,  266,  266,  266,
    266,  266,  266,  266,  266,  948,  948,  378,  374,  258,
    156,  258,  948,  948,  259,  948,  260,  261,  262,  262,
    262,  262,  258,  156,  258,  378,  948,  259,  948,  260,
    261,  262,  262,  262,  262,  156,  156,  156,  948,  948,
    157,  948,  948,  948,  264,  264,  264,  264,  269,  269,
    269,  269,  269,  269,  269,  269,  269,  269,  269,  269,
    164,  164,  164,  948,  948,  165,  279,  280,  280,  280,
    280,  948,  379,  381,  381,  381,  381,  288,  288,  288,

    288,  290,  290,  290,  290,  948,  948,  948,  379,  271,
    164,  271,  948,  948,  272,  948,  273,  948,  274,  275,
    275,  275,  271,  164,  271,  948,  948,  272,  948,  273,
    948,  274,  275,  275,  275,  271,  164,  271,  948,  948,
    272,  948,  273,  948,  274,  275,  275,  275,  271,  164,
    271,  948,  948,  272,  948,  273,  948,  274,  275,  275,
    275,  271,  164,  271,  948,  948,  272,  948,  273,  948,
    274,  275,  275,  275,  948,  948,  385,  386,  385,  948,
    948,  387,  290,  290,  290,  290,  389,  389,  389,  389,
    380,  948,  388,  292,  293,  293,  293,  293,  301,  301,

    301,  301,  392,  392,  392,  392,  380,  382,  382,  382,
    388,  948,  383,  948,  948,  279,  280,  280,  280,  280,
    948,  948,  948,  384,  303,  304,  304,  304,  304,  948,
    948,  948,  292,  293,  293,  293,  293,  948,  948,  948,
    390,  384,  282,  172,  282,  948,  948,  283,  948,  284,
    285,  286,  286,  286,  286,  282,  172,  282,  390,  948,
    283,  948,  284,  285,  286,  286,  286,  286,  172,  172,
    172,  948,  948,  173,  948,  948,  948,  288,  288,  288,
    288,  295,  186,  295,  948,  948,  296,  948,  297,  298,
    299,  299,  299,  299,  186,  186,  186,  948,  948,  187,

    948,  948,  948,  301,  301,  301,  301,  393,  393,  393,
    948,  948,  394,  948,  948,  303,  304,  304,  304,  304,
    948,  948,  948,  395,  396,  397,  396,  948,  948,  398,
    312,  312,  312,  312,  314,  315,  315,  315,  315,  948,
    399,  395,  306,  192,  306,  948,  948,  307,  948,  308,
    309,  310,  310,  310,  310,  948,  948,  948,  399,  192,
    192,  192,  948,  948,  193,  948,  948,  948,  312,  312,
    312,  312,  400,  400,  400,  400,  401,  401,  401,  948,
    948,  402,  948,  948,  314,  315,  315,  315,  315,  948,
    948,  948,  403,  404,  405,  404,  948,  948,  406,  323,

    323,  323,  323,  325,  325,  325,  325,  948,  948,  407,
    403,  317,  198,  317,  948,  948,  318,  948,  319,  320,
    321,  321,  321,  321,  948,  948,  948,  407,  198,  198,
    198,  948,  948,  199,  948,  948,  948,  323,  323,  323,
    323,  325,  325,  325,  325,  328,  329,  329,  329,  329,
    408,  408,  408,  408,  409,  409,  409,  948,  948,  410,
    948,  948,  328,  329,  329,  329,  329,  948,  948,  948,
    411,  412,  413,  412,  948,  948,  414,  337,  337,  337,
    337,  339,  340,  340,  340,  340,  948,  415,  411,  331,
    211,  331,  948,  948,  332,  948,  333,  334,  335,  335,

    335,  335,  948,  948,  948,  415,  211,  211,  211,  948,
    948,  212,  948,  948,  948,  337,  337,  337,  337,  416,
    416,  416,  416,  339,  340,  340,  340,  340,  948,  948,
    948,  417,  348,  348,  348,  348,  350,  351,  351,  351,
    351,  419,  419,  419,  419,  359,  359,  359,  359,  417,
    342,  221,  342,  948,  948,  343,  948,  344,  345,  346,
    346,  346,  346,  221,  221,  221,  948,  948,  222,  948,
    948,  948,  348,  348,  348,  348,  420,  420,  420,  948,
    948,  421,  948,  948,  350,  351,  351,  351,  351,  948,
    948,  948,  422,  423,  424,  423,  948,  948,  425,  948,

    371,  371,  371,  371,  948,  381,  381,  381,  381,  426,
    422,  353,  227,  353,  948,  948,  354,  948,  355,  356,
    357,  357,  357,  357,  948,  948,  948,  426,  227,  227,
    227,  948,  948,  228,  948,  948,  948,  359,  359,  359,
    359,  135,  130,  135,  444,  444,  136,  445,  445,  445,
    445,  459,  460,  948,  461,  461,  461,  461,  948,  948,
    427,  372,  372,  372,  948,  948,  373,  948,  435,  436,
    437,  437,  437,  437,  439,  372,  439,  948,  948,  440,
    948,  441,  442,  443,  443,  443,  443,  375,  376,  375,
    948,  948,  377,  948,  446,  447,  448,  448,  448,  448,

    376,  376,  376,  948,  948,  449,  948,  450,  451,  452,
    452,  452,  452,  454,  376,  454,  948,  948,  455,  948,
    456,  457,  458,  458,  458,  458,  463,  464,  464,  464,
    465,  465,  465,  465,  465,  475,  475,  948,  476,  476,
    476,  476,  389,  389,  389,  389,  270,  948,  948,  390,
    465,  465,  465,  493,  493,  948,  494,  494,  494,  494,
    948,  948,  270,  382,  382,  382,  948,  390,  383,  948,
    466,  467,  468,  468,  468,  468,  470,  382,  470,  948,
    948,  471,  948,  472,  473,  474,  474,  474,  474,  385,
    386,  385,  948,  948,  387,  948,  477,  478,  479,  479,

    479,  479,  386,  386,  386,  948,  948,  480,  948,  481,
    482,  483,  483,  483,  483,  485,  386,  485,  948,  948,
    486,  948,  487,  488,  489,  489,  489,  489,  282,  172,
    282,  948,  948,  283,  490,  491,  285,  492,  492,  492,
    492,  295,  186,  295,  948,  948,  296,  495,  496,  298,
    497,  497,  497,  497,  393,  393,  393,  948,  948,  394,
    948,  948,  948,  392,  392,  392,  392,  948,  507,  507,
    395,  508,  508,  508,  508,  534,  534,  948,  535,  535,
    535,  535,  135,  130,  611,  948,  948,  136,  395,  393,
    393,  393,  948,  948,  394,  948,  498,  499,  500,  500,

    500,  500,  502,  393,  502,  948,  948,  503,  948,  504,
    505,  506,  506,  506,  506,  396,  397,  396,  948,  948,
    398,  948,  509,  510,  511,  511,  511,  511,  397,  397,
    397,  948,  948,  512,  948,  513,  514,  515,  515,  515,
    515,  517,  397,  517,  948,  948,  518,  948,  519,  520,
    521,  521,  521,  521,  306,  192,  306,  948,  948,  307,
    522,  523,  309,  524,  524,  524,  524,  401,  401,  401,
    948,  948,  402,  948,  948,  948,  400,  400,  400,  400,
    948,  561,  561,  403,  562,  562,  562,  562,  436,  437,
    437,  437,  437,  416,  416,  416,  416,  948,  948,  948,

    417,  403,  401,  401,  401,  948,  948,  402,  948,  525,
    526,  527,  527,  527,  527,  529,  401,  529,  417,  948,
    530,  948,  531,  532,  533,  533,  533,  533,  404,  405,
    404,  948,  948,  406,  948,  536,  537,  538,  538,  538,
    538,  405,  405,  405,  948,  948,  539,  948,  540,  541,
    542,  542,  542,  542,  544,  405,  544,  948,  948,  545,
    948,  546,  547,  548,  548,  548,  548,  317,  198,  317,
    948,  948,  318,  549,  550,  320,  551,  551,  551,  551,
    409,  409,  409,  948,  948,  410,  948,  948,  948,  408,
    408,  408,  408,  948,  579,  579,  411,  580,  580,  580,

    580,  593,  593,  948,  594,  594,  594,  594,  616,  616,
    616,  616,  948,  948,  411,  409,  409,  409,  948,  948,
    410,  948,  552,  553,  554,  554,  554,  554,  556,  409,
    556,  948,  948,  557,  948,  558,  559,  560,  560,  560,
    560,  412,  413,  412,  948,  948,  414,  948,  563,  564,
    565,  565,  565,  565,  413,  413,  413,  948,  948,  566,
    948,  567,  568,  569,  569,  569,  569,  571,  413,  571,
    948,  948,  572,  948,  573,  574,  575,  575,  575,  575,
    331,  211,  331,  948,  948,  332,  576,  577,  334,  578,
    578,  578,  578,  342,  221,  342,  948,  948,  343,  581,

    582,  345,  583,  583,  583,  583,  420,  420,  420,  948,
    948,  421,  948,  948,  948,  419,  419,  419,  419,  948,
    948,  948,  422,  445,  445,  445,  445,  621,  621,  621,
    621,  436,  437,  437,  437,  437,  948,  948,  948,  617,
    422,  420,  420,  420,  948,  948,  421,  948,  584,  585,
    586,  586,  586,  586,  588,  420,  588,  617,  948,  589,
    948,  590,  591,  592,  592,  592,  592,  423,  424,  423,
    948,  948,  425,  948,  595,  596,  597,  597,  597,  597,
    424,  424,  424,  948,  948,  598,  948,  599,  600,  601,
    601,  601,  601,  603,  424,  603,  948,  948,  604,  948,

    605,  606,  607,  607,  607,  607,  353,  227,  353,  948,
    948,  354,  608,  609,  356,  610,  610,  610,  610,  439,
    372,  439,  948,  948,  440,  948,  441,  442,  443,  443,
    443,  443,  439,  372,  439,  948,  948,  440,  948,  441,
    442,  443,  443,  443,  443,  372,  372,  372,  948,  948,
    373,  948,  948,  948,  445,  445,  445,  445,  258,  156,
    258,  948,  948,  259,  948,  260,  447,  448,  448,  448,
    448,  258,  156,  258,  948,  948,  259,  948,  260,  261,
    619,  619,  619,  619,  375,  376,  375,  948,  948,  377,
    948,  948,  447,  448,  448,  448,  448,  948,  948,  948,

    620,  451,  452,  452,  452,  452,  461,  461,  461,  461,
    461,  461,  461,  461,  624,  624,  624,  624,  620,  454,
    376,  454,  948,  948,  455,  948,  456,  457,  458,  458,
    458,  458,  372,  372,  372,  948,  948,  373,  948,  948,
    451,  452,  452,  452,  452,  948,  948,  948,  622,  467,
    468,  468,  468,  468,  261,  461,  461,  461,  461,  135,
    130,  135,  378,  948,  136,  948,  622,  454,  376,  454,
    843,  948,  455,  948,  456,  457,  458,  458,  458,  458,
    378,  270,  271,  164,  271,  270,  270,  272,  270,  273,
    270,  463,  464,  464,  464,  465,  465,  465,  465,  465,

    270,  270,  270,  270,  270,  270,  270,  270,  270,  270,
    270,  380,  270,  270,  270,  465,  465,  465,  270,  270,
    270,  270,  270,  270,  270,  270,  270,  380,  625,  625,
    625,  948,  948,  626,  948,  948,  467,  468,  468,  468,
    468,  948,  948,  948,  627,  628,  629,  628,  948,  948,
    630,  476,  476,  476,  476,  482,  483,  483,  483,  483,
    948,  631,  627,  470,  382,  470,  948,  948,  471,  948,
    472,  473,  474,  474,  474,  474,  470,  382,  470,  631,
    948,  471,  948,  472,  473,  474,  474,  474,  474,  382,
    382,  382,  948,  948,  383,  948,  948,  948,  476,  476,

    476,  476,  282,  172,  282,  948,  948,  283,  948,  284,
    478,  479,  479,  479,  479,  282,  172,  282,  948,  948,
    283,  948,  284,  285,  632,  632,  632,  632,  633,  634,
    633,  948,  948,  635,  948,  948,  478,  479,  479,  479,
    479,  948,  948,  948,  636,  637,  637,  637,  637,  492,
    492,  492,  492,  492,  492,  492,  492,  494,  494,  494,
    494,  948,  636,  485,  386,  485,  948,  948,  486,  948,
    487,  488,  489,  489,  489,  489,  629,  629,  629,  948,
    948,  638,  948,  948,  482,  483,  483,  483,  483,  948,
    948,  948,  639,  640,  634,  640,  948,  948,  641,  494,

    494,  494,  494,  499,  500,  500,  500,  500,  948,  642,
    639,  485,  386,  485,  948,  948,  486,  948,  487,  488,
    489,  489,  489,  489,  948,  948,  948,  642,  385,  386,
    385,  948,  948,  387,  948,  948,  948,  492,  492,  492,
    492,  948,  948,  948,  388,  643,  643,  643,  643,  508,
    508,  508,  508,  499,  500,  500,  500,  500,  948,  948,
    948,  644,  388,  295,  186,  295,  948,  948,  296,  948,
    297,  298,  497,  497,  497,  497,  295,  186,  295,  644,
    948,  296,  948,  297,  298,  497,  497,  497,  497,  502,
    393,  502,  948,  948,  503,  948,  504,  505,  506,  506,

    506,  506,  502,  393,  502,  948,  948,  503,  948,  504,
    505,  506,  506,  506,  506,  393,  393,  393,  948,  948,
    394,  948,  948,  948,  508,  508,  508,  508,  306,  192,
    306,  948,  948,  307,  948,  308,  510,  511,  511,  511,
    511,  306,  192,  306,  948,  948,  307,  948,  308,  309,
    646,  646,  646,  646,  396,  397,  396,  948,  948,  398,
    948,  948,  510,  511,  511,  511,  511,  948,  948,  948,
    647,  514,  515,  515,  515,  515,  648,  648,  648,  648,
    524,  524,  524,  524,  524,  524,  524,  524,  647,  517,
    397,  517,  948,  948,  518,  948,  519,  520,  521,  521,

    521,  521,  393,  393,  393,  948,  948,  394,  948,  948,
    514,  515,  515,  515,  515,  948,  948,  948,  649,  526,
    527,  527,  527,  527,  309,  524,  524,  524,  524,  135,
    130,  931,  399,  948,  136,  948,  649,  517,  397,  517,
    948,  948,  518,  948,  519,  520,  521,  521,  521,  521,
    399,  651,  651,  651,  651,  526,  527,  527,  527,  527,
    948,  948,  948,  652,  535,  535,  535,  535,  541,  542,
    542,  542,  542,  656,  656,  656,  656,  551,  551,  551,
    551,  652,  529,  401,  529,  948,  948,  530,  948,  531,
    532,  533,  533,  533,  533,  529,  401,  529,  948,  948,

    530,  948,  531,  532,  533,  533,  533,  533,  401,  401,
    401,  948,  948,  402,  948,  948,  948,  535,  535,  535,
    535,  317,  198,  317,  948,  948,  318,  948,  319,  537,
    538,  538,  538,  538,  317,  198,  317,  948,  948,  318,
    948,  319,  320,  654,  654,  654,  654,  404,  405,  404,
    948,  948,  406,  948,  948,  537,  538,  538,  538,  538,
    948,  948,  948,  655,  551,  551,  551,  551,  659,  659,
    659,  659,  320,  551,  551,  551,  551,  948,  948,  948,
    407,  655,  544,  405,  544,  948,  948,  545,  948,  546,
    547,  548,  548,  548,  548,  401,  401,  401,  407,  948,

    402,  948,  948,  541,  542,  542,  542,  542,  948,  948,
    948,  657,  553,  554,  554,  554,  554,  562,  562,  562,
    562,  568,  569,  569,  569,  569,  948,  948,  948,  657,
    544,  405,  544,  948,  948,  545,  948,  546,  547,  548,
    548,  548,  548,  660,  660,  660,  948,  948,  661,  948,
    948,  553,  554,  554,  554,  554,  948,  948,  948,  662,
    663,  664,  663,  948,  948,  665,  672,  672,  672,  672,
    578,  578,  578,  578,  948,  948,  666,  662,  556,  409,
    556,  948,  948,  557,  948,  558,  559,  560,  560,  560,
    560,  556,  409,  556,  666,  948,  557,  948,  558,  559,

    560,  560,  560,  560,  409,  409,  409,  948,  948,  410,
    948,  948,  948,  562,  562,  562,  562,  331,  211,  331,
    948,  948,  332,  948,  333,  564,  565,  565,  565,  565,
    331,  211,  331,  948,  948,  332,  948,  333,  334,  667,
    667,  667,  667,  668,  669,  668,  948,  948,  670,  948,
    948,  564,  565,  565,  565,  565,  948,  948,  948,  671,
    675,  669,  675,  948,  948,  676,  578,  578,  578,  578,
    580,  580,  580,  580,  948,  948,  677,  671,  571,  413,
    571,  948,  948,  572,  948,  573,  574,  575,  575,  575,
    575,  664,  664,  664,  677,  948,  673,  948,  948,  568,

    569,  569,  569,  569,  948,  948,  948,  674,  580,  580,
    580,  580,  585,  586,  586,  586,  586,  678,  678,  678,
    678,  594,  594,  594,  594,  674,  571,  413,  571,  948,
    948,  572,  948,  573,  574,  575,  575,  575,  575,  412,
    413,  412,  948,  948,  414,  948,  948,  948,  578,  578,
    578,  578,  948,  948,  948,  415,  600,  601,  601,  601,
    601,  948,  948,  948,  585,  586,  586,  586,  586,  948,
    948,  948,  679,  415,  342,  221,  342,  948,  948,  343,
    948,  344,  345,  583,  583,  583,  583,  342,  221,  342,
    679,  948,  343,  948,  344,  345,  583,  583,  583,  583,

    588,  420,  588,  948,  948,  589,  948,  590,  591,  592,
    592,  592,  592,  588,  420,  588,  948,  948,  589,  948,
    590,  591,  592,  592,  592,  592,  420,  420,  420,  948,
    948,  421,  948,  948,  948,  594,  594,  594,  594,  353,
    227,  353,  948,  948,  354,  948,  355,  596,  597,  597,
    597,  597,  353,  227,  353,  948,  948,  354,  948,  355,
    356,  681,  681,  681,  681,  423,  424,  423,  948,  948,
    425,  948,  948,  596,  597,  597,  597,  597,  948,  948,
    948,  682,  683,  683,  683,  683,  610,  610,  610,  610,
    610,  610,  610,  610,  692,  692,  692,  692,  948,  682,

    603,  424,  603,  948,  948,  604,  948,  605,  606,  607,
    607,  607,  607,  420,  420,  420,  948,  948,  421,  948,
    948,  600,  601,  601,  601,  601,  135,  130,  135,  684,
    948,  136,  948,  948,  948,  356,  610,  610,  610,  610,
    135,  130,  135,  426,  948,  136,  948,  684,  603,  424,
    603,  948,  938,  604,  686,  605,  606,  607,  607,  607,
    607,  426,  616,  616,  616,  616,  948,  691,  691,  617,
    692,  692,  692,  692,  699,  699,  948,  700,  700,  700,
    700,  692,  692,  692,  692,  948,  948,  617,  439,  372,
    439,  948,  948,  440,  693,  694,  442,  695,  695,  695,

    695,  375,  376,  375,  948,  948,  377,  948,  260,  261,
    619,  619,  619,  619,  948,  713,  713,  620,  714,  714,
    714,  714,  746,  746,  948,  747,  747,  747,  747,  700,
    700,  700,  700,  948,  948,  620,  258,  156,  258,  948,
    948,  259,  696,  697,  261,  698,  698,  698,  698,  372,
    372,  372,  948,  948,  373,  948,  948,  948,  621,  621,
    621,  621,  948,  754,  754,  622,  755,  755,  755,  755,
    705,  706,  706,  706,  706,  643,  643,  643,  643,  948,
    948,  948,  644,  622,  454,  376,  454,  948,  948,  455,
    701,  702,  457,  703,  703,  703,  703,  625,  625,  625,

    644,  948,  626,  948,  948,  948,  624,  624,  624,  624,
    948,  762,  762,  627,  763,  763,  763,  763,  847,  847,
    847,  847,  948,  651,  651,  651,  651,  948,  948,  948,
    652,  627,  625,  625,  625,  948,  948,  626,  948,  704,
    705,  706,  706,  706,  706,  708,  625,  708,  652,  948,
    709,  948,  710,  711,  712,  712,  712,  712,  628,  629,
    628,  948,  948,  630,  948,  715,  716,  717,  717,  717,
    717,  629,  629,  629,  948,  948,  638,  948,  718,  719,
    720,  720,  720,  720,  722,  629,  722,  948,  948,  723,
    948,  724,  725,  726,  726,  726,  726,  470,  382,  470,

    948,  948,  471,  727,  728,  473,  729,  729,  729,  729,
    633,  634,  633,  948,  948,  635,  948,  284,  285,  632,
    632,  632,  632,  948,  767,  767,  636,  768,  768,  768,
    768,  775,  775,  948,  776,  776,  776,  776,  714,  714,
    714,  714,  948,  948,  636,  633,  634,  633,  948,  948,
    635,  948,  730,  731,  732,  732,  732,  732,  634,  634,
    634,  948,  948,  733,  948,  734,  735,  736,  736,  736,
    736,  738,  634,  738,  948,  948,  739,  948,  740,  741,
    742,  742,  742,  742,  282,  172,  282,  948,  948,  283,
    743,  744,  285,  745,  745,  745,  745,  629,  629,  629,

    948,  948,  638,  948,  948,  948,  637,  637,  637,  637,
    948,  789,  789,  639,  790,  790,  790,  790,  822,  822,
    948,  823,  823,  823,  823,  719,  720,  720,  720,  720,
    948,  639,  722,  629,  722,  948,  948,  723,  948,  724,
    725,  726,  726,  726,  726,  640,  634,  640,  948,  948,
    641,  948,  748,  749,  750,  750,  750,  750,  738,  634,
    738,  948,  948,  739,  948,  740,  741,  742,  742,  742,
    742,  485,  386,  485,  948,  948,  486,  751,  752,  488,
    753,  753,  753,  753,  502,  393,  502,  948,  948,  503,
    756,  757,  505,  758,  758,  758,  758,  396,  397,  396,

    948,  948,  398,  948,  308,  309,  646,  646,  646,  646,
    948,  830,  830,  647,  831,  831,  831,  831,  852,  852,
    852,  852,  948,  678,  678,  678,  678,  948,  948,  948,
    679,  647,  306,  192,  306,  948,  948,  307,  759,  760,
    309,  761,  761,  761,  761,  393,  393,  393,  679,  948,
    394,  948,  948,  948,  648,  648,  648,  648,  948,  838,
    838,  649,  839,  839,  839,  839,  729,  729,  729,  729,
    705,  706,  706,  706,  706,  948,  948,  948,  848,  649,
    517,  397,  517,  948,  948,  518,  764,  765,  520,  766,
    766,  766,  766,  529,  401,  529,  848,  948,  530,  769,

    770,  532,  771,  771,  771,  771,  404,  405,  404,  948,
    948,  406,  948,  319,  320,  654,  654,  654,  654,  948,
    948,  948,  655,  375,  376,  375,  948,  948,  377,  729,
    729,  729,  729,  735,  736,  736,  736,  736,  948,  378,
    655,  317,  198,  317,  948,  948,  318,  772,  773,  320,
    774,  774,  774,  774,  401,  401,  401,  378,  948,  402,
    948,  948,  948,  656,  656,  656,  656,  948,  948,  948,
    657,  857,  857,  857,  857,  745,  745,  745,  745,  473,
    729,  729,  729,  729,  948,  948,  948,  631,  657,  544,
    405,  544,  948,  948,  545,  777,  778,  547,  779,  779,

    779,  779,  660,  660,  660,  631,  948,  661,  948,  948,
    948,  659,  659,  659,  659,  948,  948,  948,  662,  745,
    745,  745,  745,  747,  747,  747,  747,  285,  745,  745,
    745,  745,  948,  948,  948,  388,  662,  660,  660,  660,
    948,  948,  661,  948,  780,  781,  782,  782,  782,  782,
    784,  660,  784,  388,  948,  785,  948,  786,  787,  788,
    788,  788,  788,  663,  664,  663,  948,  948,  665,  948,
    791,  792,  793,  793,  793,  793,  664,  664,  664,  948,
    948,  673,  948,  794,  795,  796,  796,  796,  796,  798,
    664,  798,  948,  948,  799,  948,  800,  801,  802,  802,

    802,  802,  556,  409,  556,  948,  948,  557,  803,  804,
    559,  805,  805,  805,  805,  668,  669,  668,  948,  948,
    670,  948,  333,  334,  667,  667,  667,  667,  948,  948,
    948,  671,  753,  753,  753,  753,  753,  753,  753,  753,
    488,  753,  753,  753,  753,  948,  948,  948,  642,  671,
    668,  669,  668,  948,  948,  670,  948,  806,  807,  808,
    808,  808,  808,  669,  669,  669,  642,  948,  809,  948,
    810,  811,  812,  812,  812,  812,  814,  669,  814,  948,
    948,  815,  948,  816,  817,  818,  818,  818,  818,  331,
    211,  331,  948,  948,  332,  819,  820,  334,  821,  821,

    821,  821,  664,  664,  664,  948,  948,  673,  948,  948,
    948,  672,  672,  672,  672,  948,  948,  948,  674,  755,
    755,  755,  755,  755,  755,  755,  755,  763,  763,  763,
    763,  768,  768,  768,  768,  948,  674,  798,  664,  798,
    948,  948,  799,  948,  800,  801,  802,  802,  802,  802,
    675,  669,  675,  948,  948,  676,  948,  824,  825,  826,
    826,  826,  826,  814,  669,  814,  948,  948,  815,  948,
    816,  817,  818,  818,  818,  818,  571,  413,  571,  948,
    948,  572,  827,  828,  574,  829,  829,  829,  829,  588,
    420,  588,  948,  948,  589,  832,  833,  591,  834,  834,

    834,  834,  423,  424,  423,  948,  948,  425,  948,  355,
    356,  681,  681,  681,  681,  948,  948,  948,  682,  396,
    397,  396,  948,  948,  398,  768,  768,  768,  768,  776,
    776,  776,  776,  948,  948,  399,  682,  353,  227,  353,
    948,  948,  354,  835,  836,  356,  837,  837,  837,  837,
    420,  420,  420,  399,  948,  421,  948,  948,  948,  683,
    683,  683,  683,  948,  948,  948,  684,  404,  405,  404,
    948,  948,  406,  781,  782,  782,  782,  782,  862,  862,
    862,  862,  948,  407,  684,  603,  424,  603,  948,  948,
    604,  840,  841,  606,  842,  842,  842,  842,  439,  372,

    439,  407,  948,  440,  948,  441,  442,  695,  695,  695,
    695,  439,  372,  439,  948,  948,  440,  948,  441,  442,
    695,  695,  695,  695,  258,  156,  258,  948,  948,  259,
    948,  260,  261,  698,  698,  698,  698,  258,  156,  258,
    948,  948,  259,  948,  260,  261,  698,  698,  698,  698,
    372,  372,  372,  948,  948,  373,  948,  948,  948,  700,
    700,  700,  700,  454,  376,  454,  948,  948,  455,  948,
    456,  457,  703,  703,  703,  703,  454,  376,  454,  948,
    948,  455,  948,  456,  457,  703,  703,  703,  703,  708,
    625,  708,  948,  948,  709,  948,  710,  711,  712,  712,

    712,  712,  708,  625,  708,  948,  948,  709,  948,  710,
    711,  712,  712,  712,  712,  625,  625,  625,  948,  948,
    626,  948,  948,  948,  714,  714,  714,  714,  470,  382,
    470,  948,  948,  471,  948,  472,  716,  717,  717,  717,
    717,  470,  382,  470,  948,  948,  471,  948,  472,  473,
    850,  850,  850,  850,  628,  629,  628,  948,  948,  630,
    948,  948,  716,  717,  717,  717,  717,  948,  948,  948,
    851,  790,  790,  790,  790,  948,  781,  782,  782,  782,
    782,  940,  941,  940,  863,  948,  942,  948,  851,  625,
    625,  625,  948,  948,  626,  948,  948,  719,  720,  720,

    720,  720,  863,  948,  948,  853,  795,  796,  796,  796,
    796,  867,  867,  867,  867,  805,  805,  805,  805,  805,
    805,  805,  805,  853,  722,  629,  722,  948,  948,  723,
    948,  724,  725,  726,  726,  726,  726,  722,  629,  722,
    948,  948,  723,  948,  724,  725,  726,  726,  726,  726,
    282,  172,  282,  948,  948,  283,  948,  284,  731,  732,
    732,  732,  732,  282,  172,  282,  948,  948,  283,  948,
    284,  285,  855,  855,  855,  855,  633,  634,  633,  948,
    948,  635,  948,  948,  731,  732,  732,  732,  732,  948,
    948,  948,  856,  811,  812,  812,  812,  812,  948,  948,

    948,  559,  805,  805,  805,  805,  948,  948,  948,  666,
    856,  738,  634,  738,  948,  948,  739,  948,  740,  741,
    742,  742,  742,  742,  629,  629,  629,  666,  948,  638,
    948,  948,  735,  736,  736,  736,  736,  948,  948,  948,
    858,  872,  872,  872,  872,  821,  821,  821,  821,  821,
    821,  821,  821,  823,  823,  823,  823,  948,  858,  738,
    634,  738,  948,  948,  739,  948,  740,  741,  742,  742,
    742,  742,  629,  629,  629,  948,  948,  638,  948,  948,
    948,  747,  747,  747,  747,  485,  386,  485,  948,  948,
    486,  948,  487,  749,  750,  750,  750,  750,  485,  386,

    485,  948,  948,  486,  948,  487,  488,  860,  860,  860,
    860,  640,  634,  640,  948,  948,  641,  948,  948,  749,
    750,  750,  750,  750,  948,  948,  948,  861,  829,  829,
    829,  829,  829,  829,  829,  829,  334,  821,  821,  821,
    821,  948,  948,  948,  415,  861,  502,  393,  502,  948,
    948,  503,  948,  504,  505,  758,  758,  758,  758,  502,
    393,  502,  415,  948,  503,  948,  504,  505,  758,  758,
    758,  758,  306,  192,  306,  948,  948,  307,  948,  308,
    309,  761,  761,  761,  761,  306,  192,  306,  948,  948,
    307,  948,  308,  309,  761,  761,  761,  761,  393,  393,

    393,  948,  948,  394,  948,  948,  948,  763,  763,  763,
    763,  517,  397,  517,  948,  948,  518,  948,  519,  520,
    766,  766,  766,  766,  517,  397,  517,  948,  948,  518,
    948,  519,  520,  766,  766,  766,  766,  529,  401,  529,
    948,  948,  530,  948,  531,  532,  771,  771,  771,  771,
    529,  401,  529,  948,  948,  530,  948,  531,  532,  771,
    771,  771,  771,  317,  198,  317,  948,  948,  318,  948,
    319,  320,  774,  774,  774,  774,  317,  198,  317,  948,
    948,  318,  948,  319,  320,  774,  774,  774,  774,  401,
    401,  401,  948,  948,  402,  948,  948,  948,  776,  776,

    776,  776,  544,  405,  544,  948,  948,  545,  948,  546,
    547,  779,  779,  779,  779,  544,  405,  544,  948,  948,
    545,  948,  546,  547,  779,  779,  779,  779,  784,  660,
    784,  948,  948,  785,  948,  786,  787,  788,  788,  788,
    788,  784,  660,  784,  948,  948,  785,  948,  786,  787,
    788,  788,  788,  788,  660,  660,  660,  948,  948,  661,
    948,  948,  948,  790,  790,  790,  790,  556,  409,  556,
    948,  948,  557,  948,  558,  792,  793,  793,  793,  793,
    556,  409,  556,  948,  948,  557,  948,  558,  559,  865,
    865,  865,  865,  663,  664,  663,  948,  948,  665,  948,

    948,  792,  793,  793,  793,  793,  948,  948,  948,  866,
    831,  831,  831,  831,  948,  574,  829,  829,  829,  829,
    940,  941,  940,  677,  948,  942,  948,  866,  660,  660,
    660,  948,  948,  661,  948,  948,  795,  796,  796,  796,
    796,  677,  948,  948,  868,  831,  831,  831,  831,  839,
    839,  839,  839,  135,  130,  135,  948,  948,  136,  948,
    948,  877,  868,  798,  664,  798,  948,  948,  799,  948,
    800,  801,  802,  802,  802,  802,  798,  664,  798,  948,
    948,  799,  948,  800,  801,  802,  802,  802,  802,  331,
    211,  331,  948,  948,  332,  948,  333,  807,  808,  808,

    808,  808,  331,  211,  331,  948,  948,  332,  948,  333,
    334,  870,  870,  870,  870,  668,  669,  668,  948,  948,
    670,  948,  948,  807,  808,  808,  808,  808,  948,  948,
    948,  871,  423,  424,  423,  880,  880,  425,  881,  881,
    881,  881,  559,  865,  865,  865,  865,  948,  426,  871,
    814,  669,  814,  948,  948,  815,  948,  816,  817,  818,
    818,  818,  818,  664,  664,  664,  426,  948,  673,  948,
    948,  811,  812,  812,  812,  812,  948,  888,  888,  873,
    889,  889,  889,  889,  948,  867,  867,  867,  867,  847,
    847,  847,  847,  948,  948,  948,  848,  873,  814,  669,

    814,  948,  948,  815,  948,  816,  817,  818,  818,  818,
    818,  664,  664,  664,  848,  948,  673,  948,  948,  948,
    823,  823,  823,  823,  571,  413,  571,  948,  948,  572,
    948,  573,  825,  826,  826,  826,  826,  571,  413,  571,
    948,  948,  572,  948,  573,  574,  875,  875,  875,  875,
    675,  669,  675,  948,  948,  676,  948,  948,  825,  826,
    826,  826,  826,  948,  896,  896,  876,  897,  897,  897,
    897,  334,  870,  870,  870,  870,  862,  862,  862,  862,
    948,  948,  948,  863,  876,  588,  420,  588,  948,  948,
    589,  948,  590,  591,  834,  834,  834,  834,  588,  420,

    588,  863,  948,  589,  948,  590,  591,  834,  834,  834,
    834,  353,  227,  353,  948,  948,  354,  948,  355,  356,
    837,  837,  837,  837,  353,  227,  353,  948,  948,  354,
    948,  355,  356,  837,  837,  837,  837,  420,  420,  420,
    948,  948,  421,  948,  948,  948,  839,  839,  839,  839,
    603,  424,  603,  948,  948,  604,  948,  605,  606,  842,
    842,  842,  842,  603,  424,  603,  948,  948,  604,  948,
    605,  606,  842,  842,  842,  842,  708,  625,  708,  948,
    948,  709,  882,  883,  711,  884,  884,  884,  884,  628,
    629,  628,  948,  948,  630,  948,  472,  473,  850,  850,

    850,  850,  948,  904,  904,  851,  905,  905,  905,  905,
    906,  907,  948,  908,  908,  908,  908,  948,  872,  872,
    872,  872,  948,  851,  470,  382,  470,  948,  948,  471,
    885,  886,  473,  887,  887,  887,  887,  625,  625,  625,
    948,  948,  626,  948,  948,  948,  852,  852,  852,  852,
    948,  909,  910,  853,  911,  911,  911,  911,  912,  912,
    948,  913,  913,  913,  913,  574,  875,  875,  875,  875,
    948,  853,  722,  629,  722,  948,  948,  723,  890,  891,
    725,  892,  892,  892,  892,  633,  634,  633,  948,  948,
    635,  948,  284,  285,  855,  855,  855,  855,  948,  914,

    915,  856,  916,  916,  916,  916,  917,  918,  948,  919,
    919,  919,  919,  881,  881,  881,  881,  948,  948,  856,
    282,  172,  282,  948,  948,  283,  893,  894,  285,  895,
    895,  895,  895,  629,  629,  629,  948,  948,  638,  948,
    948,  948,  857,  857,  857,  857,  948,  920,  920,  858,
    921,  921,  921,  921,  922,  923,  948,  924,  924,  924,
    924,  881,  881,  881,  881,  948,  948,  858,  738,  634,
    738,  948,  948,  739,  898,  899,  741,  900,  900,  900,
    900,  640,  634,  640,  948,  948,  641,  948,  487,  488,
    860,  860,  860,  860,  948,  925,  926,  861,  927,  927,

    927,  927,  135,  130,  135,  948,  948,  136,  940,  941,
    940,  928,  948,  942,  948,  861,  485,  386,  485,  948,
    948,  486,  901,  902,  488,  903,  903,  903,  903,  708,
    625,  708,  948,  948,  709,  948,  710,  711,  884,  884,
    884,  884,  628,  629,  628,  948,  948,  630,  469,  472,
    889,  889,  889,  889,  897,  897,  897,  897,  631,  905,
    905,  905,  905,  905,  905,  905,  905,  908,  908,  908,
    908,  908,  908,  908,  908,  948,  631,  625,  625,  625,
    948,  948,  626,  948,  948,  948,  889,  889,  889,  889,
    722,  629,  722,  948,  948,  723,  948,  724,  725,  892,

    892,  892,  892,  633,  634,  633,  948,  948,  635,  281,
    284,  908,  908,  908,  908,  948,  948,  948,  864,  388,
    911,  911,  911,  911,  911,  911,  911,  911,  913,  913,
    913,  913,  916,  916,  916,  916,  864,  388,  629,  629,
    629,  948,  948,  638,  948,  948,  948,  897,  897,  897,
    897,  738,  634,  738,  948,  948,  739,  948,  740,  741,
    900,  900,  900,  900,  640,  634,  640,  948,  948,  641,
    484,  487,  948,  559,  911,  911,  911,  911,  948,  948,
    642,  666,  916,  916,  916,  916,  948,  948,  916,  916,
    916,  916,  135,  130,  135,  869,  948,  136,  642,  666,

    660,  660,  660,  948,  948,  661,  948,  948,  948,  913,
    913,  913,  913,  869,  919,  919,  919,  919,  919,  919,
    919,  919,  334,  919,  919,  919,  919,  937,  948,  948,
    415,  921,  921,  921,  921,  921,  921,  921,  921,  924,
    924,  924,  924,  924,  924,  924,  924,  948,  415,  924,
    924,  924,  924,  948,  948,  948,  874,  927,  927,  927,
    927,  927,  927,  927,  927,  574,  927,  927,  927,  927,
    135,  130,  135,  677,  874,  136,  135,  130,  135,  948,
    943,  136,  943,  948,  948,  944,  948,  940,  941,  940,
    948,  677,  942,  943,  946,  943,  946,  948,  944,  947,

    946,  946,  946,  946,  948,  947,  947,  946,  948,  946,
    948,  948,  947,  934,  948,  948,  948,  948,  936,   46,
    46,   46,   46,   46,   46,   46,   46,   80,   80,   80,
    80,   80,   80,   80,   80,  110,  110,  110,  110,  110,
    110,  110,  110,  120,  120,  120,  120,  120,  120,  120,
    120,  134,  134,  134,  134,  134,  134,  134,  134,  138,
    948,  948,  138,  175,  948,  948,  175,  948,  175,  175,
    175,  176,  176,  176,  176,  176,  176,  176,  176,  214,
    948,  948,  214,  948,  214,  214,  215,  215,  215,  215,
    215,  215,  215,  215,  230,  230,  948,  230,  948,  230,

    230,  230,  233,  233,  233,  233,  233,  233,  233,  233,
    239,  239,  948,  239,  239,  239,  239,  239,  257,  257,
    257,  257,  257,  257,  257,  257,  270,  270,  270,  270,
    270,  270,  270,  270,  276,  276,  948,  948,  276,  276,
    281,  281,  281,  281,  281,  281,  281,  281,  294,  294,
    294,  294,  294,  294,  294,  294,  305,  305,  305,  305,
    305,  305,  305,  305,  316,  316,  316,  316,  316,  316,
    316,  316,  330,  330,  330,  330,  330,  330,  330,  330,
    341,  341,  341,  341,  341,  341,  341,  341,  352,  352,
    352,  352,  352,  352,  352,  352,  230,  230,  948,  230,

    948,  230,  230,  230,  233,  233,  233,  233,  233,  233,
    233,  233,  239,  239,  948,  239,  239,  239,  239,  239,
    134,  134,  134,  134,  134,  134,  134,  134,  138,  948,
    948,  138,  257,  257,  257,  257,  257,  257,  257,  257,
    270,  270,  270,  270,  270,  270,  270,  270,  276,  276,
    948,  948,  276,  276,  281,  281,  281,  281,  281,  281,
    281,  281,  438,  438,  438,  438,  438,  438,  438,  438,
    453,  453,  453,  453,  453,  453,  453,  453,  469,  469,
    469,  469,  469,  469,  469,  469,  484,  484,  484,  484,
    484,  484,  484,  484,  294,  294,  294,  294,  294,  294,

    294,  294,  501,  501,  501,  501,  501,  501,  501,  501,
    305,  305,  305,  305,  305,  305,  305,  305,  516,  516,
    516,  516,  516,  516,  516,  516,  528,  528,  528,  528,
    528,  528,  528,  528,  316,  316,  316,  316,  316,  316,
    316,  316,  543,  543,  543,  543,  543,  543,  543,  543,
    555,  555,  555,  555,  555,  555,  555,  555,  330,  330,
    330,  330,  330,  330,  330,  330,  570,  570,  570,  570,
    570,  570,  570,  570,  341,  341,  341,  341,  341,  341,
    341,  341,  587,  587,  587,  587,  587,  587,  587,  587,
    352,  352,  352,  352,  352,  352,  352,  352,  602,  602,

    602,  602,  602,  602,  602,  602,  134,  134,  134,  134,
    134,  134,  134,  134,  138,  948,  948,  138,  438,  438,
    438,  438,  438,  438,  438,  438,  257,  257,  257,  257,
    257,  257,  257,  257,  453,  453,  453,  453,  453,  453,
    453,  453,  469,  469,  469,  469,  469,  469,  469,  469,
    281,  281,  281,  281,  281,  281,  281,  281,  484,  484,
    484,  484,  484,  484,  484,  484,  294,  294,  294,  294,
    294,  294,  294,  294,  501,  501,  501,  501,  501,  501,
    501,  501,  305,  305,  305,  305,  305,  305,  305,  305,
    516,  516,  516,  516,  516,  516,  516,  516,  528,  528,

    528,  528,  528,  528,  528,  528,  316,  316,  316,  316,
    316,  316,  316,  316,  543,  543,  543,  543,  543,  543,
    543,  543,  555,  555,  555,  555,  555,  555,  555,  555,
    330,  330,  330,  330,  330,  330,  330,  330,  570,  570,
    570,  570,  570,  570,  570,  570,  341,  341,  341,  341,
    341,  341,  341,  341,  587,  587,  587,  587,  587,  587,
    587,  587,  352,  352,  352,  352,  352,  352,  352,  352,
    602,  602,  602,  602,  602,  602,  602,  602,  134,  134,
    134,  134,  134,  134,  134,  134,  138,  948,  948,  138,
    438,  438,  438,  438,  438,  438,  438,  438,  257,  257,

    257,  257,  257,  257,  257,  257,  453,  453,  453,  453,
    453,  453,  453,  453,  707,  707,  707,  707,  707,  707,
    707,  707,  469,  469,  469,  469,  469,  469,  469,  469,
    721,  721,  721,  721,  721,  721,  721,  721,  281,  281,
    281,  281,  281,  281,  281,  281,  737,  737,  737,  737,
    737,  737,  737,  737,  484,  484,  484,  484,  484,  484,
    484,  484,  501,  501,  501,  501,  501,  501,  501,  501,
    305,  305,  305,  305,  305,  305,  305,  305,  516,  516,
    516,  516,  516,  516,  516,  516,  528,  528,  528,  528,
    528,  528,  528,  528,  316,  316,  316,  316,  316,  316,

    316,  316,  543,  543,  543,  543,  543,  543,  543,  543,
    783,  783,  783,  783,  783,  783,  783,  783,  555,  555,
    555,  555,  555,  555,  555,  555,  797,  797,  797,  797,
    797,  797,  797,  797,  330,  330,  330,  330,  330,  330,
    330,  330,  813,  813,  813,  813,  813,  813,  813,  813,
    570,  570,  570,  570,  570,  570,  570,  570,  587,  587,
    587,  587,  587,  587,  587,  587,  352,  352,  352,  352,
    352,  352,  352,  352,  602,  602,  602,  602,  602,  602,
    602,  602,  134,  134,  134,  134,  134,  134,  134,  134,
    138,  948,  948,  138,  438,  438,  438,  438,  438,  438,

    438,  438,  257,  257,  257,  257,  257,  257,  257,  257,
    453,  453,  453,  453,  453,  453,  453,  453,  707,  707,
    707,  707,  707,  707,  707,  707,  469,  469,  469,  469,
    469,  469,  469,  469,  721,  721,  721,  721,  721,  721,
    721,  721,  281,  281,  281,  281,  281,  281,  281,  281,
    737,  737,  737,  737,  737,  737,  737,  737,  484,  484,
    484,  484,  484,  484,  484,  484,  501,  501,  501,  501,
    501,  501,  501,  501,  305,  305,  305,  305,  305,  305,
    305,  305,  516,  516,  516,  516,  516,  516,  516,  516,
    528,  528,  528,  528,  528,  528,  528,  528,  316,  316,

    316,  316,  316,  316,  316,  316,  543,  543,  543,  543,
    543,  543,  543,  543,  783,  783,  783,  783,  783,  783,
    783,  783,  555,  555,  555,  555,  555,  555,  555,  555,
    797,  797,  797,  797,  797,  797,  797,  797,  330,  330,
    330,  330,  330,  330,  330,  330,  813,  813,  813,  813,
    813,  813,  813,  813,  570,  570,  570,  570,  570,  570,
    570,  570,  587,  587,  587,  587,  587,  587,  587,  587,
    352,  352,  352,  352,  352,  352,  352,  352,  602,  602,
    602,  602,  602,  602,  602,  602,  134,  134,  134,  134,
    134,  134,  134,  134,  138,  948,  948,  138,  707,  707,

    707,  707,  707,  707,  707,  707,  469,  469,  469,  469,
    469,  469,  469,  469,  721,  721,  721,  721,  721,  721,
    721,  721,  281,  281,  281,  281,  281,  281,  281,  281,
    737,  737,  737,  737,  737,  737,  737,  737,  484,  484,
    484,  484,  484,  484,  484,  484,  939,  939,  939,  939,
    939,  939,  939,  939,  945,  945,  948,  945,  945,  945,
    945,  945,   45,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,

    948,  948,  948,  948,  948,  948,  948,  948,  948,  948
} ;

static const short int yy_chk[7711] =
{   0,
    0,    1,    1,    1,    0,    0,    1,    2,    2,    2,
    885,  885,    2,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    5,    5,    5,   60,   60,    5,    6,    6,    6,  431,
    1010,    6,  988,  431,   47,   47,   47,    5,    5,   47,
    48,   73,   48,    6,    6,   48,    5,   65,   65,   65,
    65,   65,    6,    7,    7,    7,  985,   73,    7,  983,

    7,    7,    7,    7,    7,    7,   66,   66,   66,   66,
    976,    7,    9,    9,    9,    9,    9,    9,   10,   10,
    10,   10,   10,   10,   11,   11,   11,   11,   11,   11,
    12,   12,   12,   12,   12,   12,   13,   13,   13,   13,
    13,   13,   14,   14,   14,   14,   14,   14,   15,   15,
    15,   15,   15,   15,   16,   16,   16,   16,   16,   16,
    17,   17,   17,  970,   17,   17,   18,   18,   18,  965,
    18,   18,   19,   19,   19,   19,   19,   19,   17,   68,
    68,   68,   68,   68,   18,   20,   20,   20,   20,   20,
    20,   21,   21,   21,   21,   21,   21,   22,   22,   22,

    22,   22,   22,   23,   23,   23,   23,   23,   23,   24,
    24,   24,   24,   24,   24,   25,   25,   25,   25,   25,
    25,  275,  933,   39,   39,   39,   25,   39,   39,   40,
    40,   40,  930,   40,   40,  464,  929,  275,   25,  879,
    25,   26,   26,   26,   26,   26,   26,  299,   51,   51,
    51,  464,   26,   51,   39,  346,  443,   41,   41,   41,
    40,   41,   41,  878,   26,  299,   26,   27,   27,   27,
    27,   27,   27,  346,  443,   42,   42,   42,   27,   42,
    42,   49,   49,   49,  886,  886,   49,  846,   41,  458,
    27,  844,   27,   28,   28,   28,   28,   28,   28,   69,

    69,   69,   69,  130,   28,  130,   42,  458,  130,   49,
    71,   71,   71,   71,  690,  465,   28,  689,   28,   29,
    29,   29,   29,   29,   29,   74,   74,   74,   74,  687,
    29,  465,   43,   43,   43,   43,   43,   43,   76,   76,
    76,   76,   29,   43,   29,   30,   30,   30,   30,   30,
    30,   77,   77,   77,   77,   77,   30,  615,   44,   44,
    44,   44,   44,   44,   78,   78,   78,   78,   30,   44,
    30,   31,   31,   31,   31,   31,   31,   70,   70,   70,
    70,   70,   31,  893,  893,   70,   85,   85,   85,   85,
    75,   75,   75,   75,   31,  614,   31,   32,   32,   32,

    32,   32,   32,   70,   88,   88,   88,   88,   32,  613,
    75,   82,   82,   82,  612,   82,   82,  497,  894,  894,
    32,  432,   32,   33,   33,   33,   75,   33,   33,   84,
    84,   84,   84,   84,  506,  497,   86,   86,   86,   86,
    86,   33,  901,  901,   86,   87,   87,   87,   87,   87,
    902,  902,  506,   33,  521,   33,   34,   34,   34,  428,
    34,   34,   86,   90,   90,   90,   90,   90,   91,   91,
    91,   91,  521,  369,   34,   93,   93,   93,   93,   93,
    94,   94,   94,   94,  368,  367,   34,  366,   34,   35,
    35,   35,   35,   35,   35,   99,   99,   99,   99,   99,

    35,  100,  100,  100,  100,  101,  101,  101,  101,  101,
    365,  364,   35,  101,   35,   36,   36,   36,   36,   36,
    36,  103,  103,  103,  103,  362,   36,  104,  104,  104,
    104,  101,  105,  105,  105,  105,  253,  252,   36,  251,
    36,   37,   37,   37,   37,   37,   37,  104,  108,  108,
    108,  108,   37,  107,  107,  107,  107,  107,  114,  114,
    114,  114,  114,  104,   37,  533,   37,   38,   38,   38,
    38,   38,   38,  115,  115,  115,  115,  250,   38,  548,
    112,  112,  112,  533,  112,  112,  118,  118,  118,  118,
    38,  249,   38,   67,   67,   67,  247,  548,   67,  246,

    245,   67,   67,   67,   67,   67,  121,  121,  121,   67,
    112,  121,  112,  117,  117,  117,  117,  117,  123,  123,
    123,  244,  123,  123,  152,  150,  149,   67,   72,   72,
    72,  148,  147,   72,  146,  144,  143,   72,   72,   72,
    72,  126,  126,  126,  126,  127,  127,  127,  127,  128,
    128,  128,  128,  129,  129,  129,  142,   72,  129,  131,
    131,  131,  583,  140,  131,  127,  132,  133,  132,  133,
    139,  132,  133,   72,   79,   79,   79,  124,  592,   79,
    583,  127,   79,   79,   79,   79,   79,  134,  134,  134,
    79,  113,  134,  135,  135,  135,  592,  607,  135,  136,

    136,  136,   96,   83,  136,  695,  703,   64,   79,   89,
    89,   89,  712,   63,   89,  607,   62,   89,   89,   89,
    89,   89,   61,  695,  703,   89,  137,  137,  137,  726,
    712,  137,  153,  153,  153,  153,  155,  155,  155,  155,
    155,   59,   58,   89,   92,   92,   92,  726,   57,   92,
    137,   56,   92,   92,   92,   92,   92,  742,  158,  158,
    92,  158,  158,  158,  158,  159,  159,  159,  159,  161,
    161,  161,  161,   55,  758,  742,  161,   54,   92,   95,
    95,   95,  766,   53,   95,   52,   45,   95,   95,   95,
    95,   95,  758,    0,  161,   95,  167,  167,  167,  167,

    766,  160,  160,  160,  160,  160,  166,  166,  166,  160,
    0,  166,    0,   95,  109,  109,  109,    0,    0,  109,
    0,    0,  109,  109,  109,  109,  109,  160,  162,  162,
    109,  162,  162,  162,  162,  169,  169,  169,  169,  171,
    171,  171,  171,  171,  179,  179,  179,  179,  109,  116,
    116,  116,    0,    0,  116,  771,    0,  116,  116,  116,
    116,  116,    0,  174,  174,  116,  174,  174,  174,  174,
    176,  176,  176,  771,  176,  176,  177,  177,  177,  779,
    177,  177,    0,  116,  119,  119,  119,    0,    0,  119,
    0,    0,  119,  119,  119,  119,  119,  779,  182,  182,

    119,  182,  182,  182,  182,  183,  183,  183,  183,  180,
    180,  180,  180,  180,    0,    0,    0,  180,  119,  156,
    156,  156,    0,    0,  156,    0,  156,  156,  156,  156,
    156,  156,  157,  157,  157,  180,    0,  157,    0,  157,
    157,  157,  157,  157,  157,  164,  164,  164,  788,    0,
    164,    0,  164,    0,  164,  164,  164,  164,  165,  165,
    165,    0,    0,  165,    0,  165,  788,  165,  165,  165,
    165,  172,  172,  172,    0,    0,  172,    0,  172,  172,
    172,  172,  172,  172,  173,  173,  173,    0,    0,  173,
    0,  173,  173,  173,  173,  173,  173,  181,  181,  181,

    181,    0,    0,  802,  181,  185,  185,  185,  185,  185,
    188,  188,    0,  188,  188,  188,  188,  189,  189,  189,
    189,  802,  181,  186,  186,  186,    0,    0,  186,    0,
    186,  186,  186,  186,  186,  186,  187,  187,  187,  818,
    834,  187,  842,  187,  187,  187,  187,  187,  187,  191,
    191,  191,  191,  191,  192,  192,  192,  818,  834,  192,
    842,  192,  192,  192,  192,  192,  192,  193,  193,  193,
    0,    0,  193,    0,  193,  193,  193,  193,  193,  193,
    194,  194,  884,  194,  194,  194,  194,  195,  195,  195,
    195,  197,  197,  197,  197,  197,  198,  198,  198,    0,

    884,  198,    0,  198,  198,  198,  198,  198,  198,  199,
    199,  199,    0,    0,  199,    0,  199,  199,  199,  199,
    199,  199,  200,  200,    0,  200,  200,  200,  200,  202,
    202,  202,  202,  203,  203,  203,  203,  203,    0,    0,
    892,  203,  204,  204,  204,  204,  900,  205,  205,  204,
    205,  205,  205,  205,  206,  206,  206,  206,  892,  203,
    208,  208,  208,  208,  900,    0,    0,  204,  210,  210,
    210,  210,  210,  211,  211,  211,    0,    0,  211,    0,
    211,  211,  211,  211,  211,  211,  212,  212,  212,    0,
    0,  212,    0,  212,  212,  212,  212,  212,  212,  213,

    213,    0,  213,  213,  213,  213,  215,  215,  215,    0,
    215,  215,  216,  216,  216,    0,  216,  216,  218,  218,
    218,  218,  220,  220,  220,  220,  220,  223,  223,    0,
    223,  223,  223,  223,    0,  239,  215,  239,  215,    0,
    239,    0,  216,    0,  216,  221,  221,  221,    0,    0,
    221,    0,  221,  221,  221,  221,  221,  221,  222,  222,
    222,    0,    0,  222,    0,  222,  222,  222,  222,  222,
    222,  224,  224,  224,  224,  225,  225,  225,    0,    0,
    225,    0,    0,  225,  225,  225,  225,  225,    0,    0,
    0,  225,  226,  226,  226,  226,  226,  229,  229,    0,

    229,  229,  229,  229,  237,  237,  237,  237,    0,  225,
    227,  227,  227,    0,    0,  227,    0,  227,  227,  227,
    227,  227,  227,  228,  228,  228,    0,    0,  228,    0,
    228,  228,  228,  228,  228,  228,  231,  231,  231,    0,
    0,  231,  232,  232,  232,    0,  232,  232,  233,  233,
    233,    0,  233,  233,  234,  234,  234,    0,  234,  234,
    235,  235,  235,    0,  235,  235,  240,  241,  240,  241,
    0,  240,  241,  242,  242,  242,    0,    0,  242,  254,
    254,  254,  254,  254,  255,  255,  255,  255,  263,  263,
    263,  263,    0,  242,  256,  256,  256,    0,    0,  256,

    0,    0,  256,  256,  256,  256,  256,    0,    0,    0,
    256,  262,  262,  262,    0,    0,  262,  265,  265,  265,
    265,  266,  266,  266,  266,    0,    0,  262,  256,  257,
    257,  257,    0,    0,  257,    0,  257,  257,  257,  257,
    257,  257,  258,  258,  258,  262,    0,  258,    0,  258,
    258,  258,  258,  258,  258,  264,  264,  264,    0,    0,
    264,    0,    0,    0,  264,  264,  264,  264,  267,  267,
    267,  267,  268,  268,  268,  268,  269,  269,  269,  269,
    276,  276,  276,    0,    0,  276,  278,  278,  278,  278,
    278,    0,  268,  279,  279,  279,  279,  287,  287,  287,

    287,  289,  289,  289,  289,    0,    0,    0,  268,  270,
    270,  270,    0,    0,  270,    0,  270,    0,  270,  270,
    270,  270,  271,  271,  271,    0,    0,  271,    0,  271,
    0,  271,  271,  271,  271,  272,  272,  272,    0,    0,
    272,    0,  272,    0,  272,  272,  272,  272,  273,  273,
    273,    0,    0,  273,    0,  273,    0,  273,  273,  273,
    273,  274,  274,  274,    0,    0,  274,    0,  274,    0,
    274,  274,  274,  274,    0,    0,  286,  286,  286,    0,
    0,  286,  290,  290,  290,  290,  292,  292,  292,  292,
    274,    0,  286,  291,  291,  291,  291,  291,  300,  300,

    300,  300,  303,  303,  303,  303,  274,  280,  280,  280,
    286,    0,  280,    0,    0,  280,  280,  280,  280,  280,
    0,    0,    0,  280,  302,  302,  302,  302,  302,    0,
    0,    0,  293,  293,  293,  293,  293,    0,    0,    0,
    293,  280,  281,  281,  281,    0,    0,  281,    0,  281,
    281,  281,  281,  281,  281,  282,  282,  282,  293,    0,
    282,    0,  282,  282,  282,  282,  282,  282,  288,  288,
    288,    0,    0,  288,    0,    0,    0,  288,  288,  288,
    288,  294,  294,  294,    0,    0,  294,    0,  294,  294,
    294,  294,  294,  294,  301,  301,  301,    0,    0,  301,

    0,    0,    0,  301,  301,  301,  301,  304,  304,  304,
    0,    0,  304,    0,    0,  304,  304,  304,  304,  304,
    0,    0,    0,  304,  310,  310,  310,    0,    0,  310,
    311,  311,  311,  311,  313,  313,  313,  313,  313,    0,
    310,  304,  305,  305,  305,    0,    0,  305,    0,  305,
    305,  305,  305,  305,  305,    0,    0,    0,  310,  312,
    312,  312,    0,    0,  312,    0,    0,    0,  312,  312,
    312,  312,  314,  314,  314,  314,  315,  315,  315,    0,
    0,  315,    0,    0,  315,  315,  315,  315,  315,    0,
    0,    0,  315,  321,  321,  321,    0,    0,  321,  322,

    322,  322,  322,  324,  324,  324,  324,    0,    0,  321,
    315,  316,  316,  316,    0,    0,  316,    0,  316,  316,
    316,  316,  316,  316,    0,    0,    0,  321,  323,  323,
    323,    0,    0,  323,    0,    0,    0,  323,  323,  323,
    323,  325,  325,  325,  325,  327,  327,  327,  327,  327,
    328,  328,  328,  328,  329,  329,  329,    0,    0,  329,
    0,    0,  329,  329,  329,  329,  329,    0,    0,    0,
    329,  335,  335,  335,    0,    0,  335,  336,  336,  336,
    336,  338,  338,  338,  338,  338,    0,  335,  329,  330,
    330,  330,    0,    0,  330,    0,  330,  330,  330,  330,

    330,  330,    0,    0,    0,  335,  337,  337,  337,    0,
    0,  337,    0,    0,    0,  337,  337,  337,  337,  339,
    339,  339,  339,  340,  340,  340,  340,  340,    0,    0,
    0,  340,  347,  347,  347,  347,  349,  349,  349,  349,
    349,  350,  350,  350,  350,  358,  358,  358,  358,  340,
    341,  341,  341,    0,    0,  341,    0,  341,  341,  341,
    341,  341,  341,  348,  348,  348,    0,    0,  348,    0,
    0,    0,  348,  348,  348,  348,  351,  351,  351,    0,
    0,  351,    0,    0,  351,  351,  351,  351,  351,    0,
    0,    0,  351,  357,  357,  357,    0,    0,  357,  371,

    371,  371,  371,  371,  381,  381,  381,  381,  381,  357,
    351,  352,  352,  352,    0,    0,  352,    0,  352,  352,
    352,  352,  352,  352,    0,    0,    0,  357,  359,  359,
    359,    0,    0,  359,    0,    0,    0,  359,  359,  359,
    359,  361,  361,  361,  374,  374,  361,  374,  374,  374,
    374,  378,  378,    0,  378,  378,  378,  378,    0,    0,
    361,  372,  372,  372,    0,    0,  372,    0,  372,  372,
    372,  372,  372,  372,  373,  373,  373,    0,    0,  373,
    0,  373,  373,  373,  373,  373,  373,  375,  375,  375,
    0,    0,  375,    0,  375,  375,  375,  375,  375,  375,

    376,  376,  376,    0,    0,  376,    0,  376,  376,  376,
    376,  376,  376,  377,  377,  377,    0,    0,  377,    0,
    377,  377,  377,  377,  377,  377,  380,  380,  380,  380,
    380,  380,  380,  380,  380,  384,  384,    0,  384,  384,
    384,  384,  389,  389,  389,  389,  380,    0,    0,  389,
    380,  380,  380,  390,  390,    0,  390,  390,  390,  390,
    0,    0,  380,  382,  382,  382,    0,  389,  382,    0,
    382,  382,  382,  382,  382,  382,  383,  383,  383,    0,
    0,  383,    0,  383,  383,  383,  383,  383,  383,  385,
    385,  385,    0,    0,  385,    0,  385,  385,  385,  385,

    385,  385,  386,  386,  386,    0,    0,  386,    0,  386,
    386,  386,  386,  386,  386,  387,  387,  387,    0,    0,
    387,    0,  387,  387,  387,  387,  387,  387,  388,  388,
    388,    0,    0,  388,  388,  388,  388,  388,  388,  388,
    388,  391,  391,  391,    0,    0,  391,  391,  391,  391,
    391,  391,  391,  391,  392,  392,  392,    0,    0,  392,
    0,    0,    0,  392,  392,  392,  392,    0,  395,  395,
    392,  395,  395,  395,  395,  403,  403,    0,  403,  403,
    403,  403,  427,  427,  427,    0,    0,  427,  392,  393,
    393,  393,    0,    0,  393,    0,  393,  393,  393,  393,

    393,  393,  394,  394,  394,    0,    0,  394,    0,  394,
    394,  394,  394,  394,  394,  396,  396,  396,    0,    0,
    396,    0,  396,  396,  396,  396,  396,  396,  397,  397,
    397,    0,    0,  397,    0,  397,  397,  397,  397,  397,
    397,  398,  398,  398,    0,    0,  398,    0,  398,  398,
    398,  398,  398,  398,  399,  399,  399,    0,    0,  399,
    399,  399,  399,  399,  399,  399,  399,  400,  400,  400,
    0,    0,  400,    0,    0,    0,  400,  400,  400,  400,
    0,  411,  411,  400,  411,  411,  411,  411,  435,  435,
    435,  435,  435,  416,  416,  416,  416,    0,    0,    0,

    416,  400,  401,  401,  401,    0,    0,  401,    0,  401,
    401,  401,  401,  401,  401,  402,  402,  402,  416,    0,
    402,    0,  402,  402,  402,  402,  402,  402,  404,  404,
    404,    0,    0,  404,    0,  404,  404,  404,  404,  404,
    404,  405,  405,  405,    0,    0,  405,    0,  405,  405,
    405,  405,  405,  405,  406,  406,  406,    0,    0,  406,
    0,  406,  406,  406,  406,  406,  406,  407,  407,  407,
    0,    0,  407,  407,  407,  407,  407,  407,  407,  407,
    408,  408,  408,    0,    0,  408,    0,    0,    0,  408,
    408,  408,  408,    0,  417,  417,  408,  417,  417,  417,

    417,  422,  422,    0,  422,  422,  422,  422,  436,  436,
    436,  436,    0,    0,  408,  409,  409,  409,    0,    0,
    409,    0,  409,  409,  409,  409,  409,  409,  410,  410,
    410,    0,    0,  410,    0,  410,  410,  410,  410,  410,
    410,  412,  412,  412,    0,    0,  412,    0,  412,  412,
    412,  412,  412,  412,  413,  413,  413,    0,    0,  413,
    0,  413,  413,  413,  413,  413,  413,  414,  414,  414,
    0,    0,  414,    0,  414,  414,  414,  414,  414,  414,
    415,  415,  415,    0,    0,  415,  415,  415,  415,  415,
    415,  415,  415,  418,  418,  418,    0,    0,  418,  418,

    418,  418,  418,  418,  418,  418,  419,  419,  419,    0,
    0,  419,    0,    0,    0,  419,  419,  419,  419,    0,
    0,    0,  419,  444,  444,  444,  444,  451,  451,  451,
    451,  437,  437,  437,  437,  437,    0,    0,    0,  437,
    419,  420,  420,  420,    0,    0,  420,    0,  420,  420,
    420,  420,  420,  420,  421,  421,  421,  437,    0,  421,
    0,  421,  421,  421,  421,  421,  421,  423,  423,  423,
    0,    0,  423,    0,  423,  423,  423,  423,  423,  423,
    424,  424,  424,    0,    0,  424,    0,  424,  424,  424,
    424,  424,  424,  425,  425,  425,    0,    0,  425,    0,

    425,  425,  425,  425,  425,  425,  426,  426,  426,    0,
    0,  426,  426,  426,  426,  426,  426,  426,  426,  438,
    438,  438,    0,    0,  438,    0,  438,  438,  438,  438,
    438,  438,  439,  439,  439,    0,    0,  439,    0,  439,
    439,  439,  439,  439,  439,  445,  445,  445,    0,    0,
    445,    0,    0,    0,  445,  445,  445,  445,  446,  446,
    446,    0,    0,  446,    0,  446,  446,  446,  446,  446,
    446,  447,  447,  447,    0,    0,  447,    0,  447,  447,
    447,  447,  447,  447,  448,  448,  448,    0,    0,  448,
    0,    0,  448,  448,  448,  448,  448,    0,    0,    0,

    448,  450,  450,  450,  450,  450,  459,  459,  459,  459,
    460,  460,  460,  460,  467,  467,  467,  467,  448,  449,
    449,  449,    0,    0,  449,    0,  449,  449,  449,  449,
    449,  449,  452,  452,  452,    0,    0,  452,    0,    0,
    452,  452,  452,  452,  452,    0,    0,    0,  452,  466,
    466,  466,  466,  466,  461,  461,  461,  461,  461,  686,
    686,  686,  461,    0,  686,    0,  452,  453,  453,  453,
    686,    0,  453,    0,  453,  453,  453,  453,  453,  453,
    461,  463,  463,  463,  463,  463,  463,  463,  463,  463,
    463,  463,  463,  463,  463,  463,  463,  463,  463,  463,

    463,  463,  463,  463,  463,  463,  463,  463,  463,  463,
    463,  463,  463,  463,  463,  463,  463,  463,  463,  463,
    463,  463,  463,  463,  463,  463,  463,  463,  468,  468,
    468,    0,    0,  468,    0,    0,  468,  468,  468,  468,
    468,    0,    0,    0,  468,  474,  474,  474,    0,    0,
    474,  475,  475,  475,  475,  481,  481,  481,  481,  481,
    0,  474,  468,  469,  469,  469,    0,    0,  469,    0,
    469,  469,  469,  469,  469,  469,  470,  470,  470,  474,
    0,  470,    0,  470,  470,  470,  470,  470,  470,  476,
    476,  476,    0,    0,  476,    0,    0,    0,  476,  476,

    476,  476,  477,  477,  477,    0,    0,  477,    0,  477,
    477,  477,  477,  477,  477,  478,  478,  478,    0,    0,
    478,    0,  478,  478,  478,  478,  478,  478,  479,  479,
    479,    0,    0,  479,    0,    0,  479,  479,  479,  479,
    479,    0,    0,    0,  479,  482,  482,  482,  482,  490,
    490,  490,  490,  491,  491,  491,  491,  493,  493,  493,
    493,    0,  479,  480,  480,  480,    0,    0,  480,    0,
    480,  480,  480,  480,  480,  480,  483,  483,  483,    0,
    0,  483,    0,    0,  483,  483,  483,  483,  483,    0,
    0,    0,  483,  489,  489,  489,    0,    0,  489,  494,

    494,  494,  494,  498,  498,  498,  498,  498,    0,  489,
    483,  484,  484,  484,    0,    0,  484,    0,  484,  484,
    484,  484,  484,  484,    0,    0,    0,  489,  492,  492,
    492,    0,    0,  492,    0,    0,    0,  492,  492,  492,
    492,    0,    0,    0,  492,  499,  499,  499,  499,  507,
    507,  507,  507,  500,  500,  500,  500,  500,    0,    0,
    0,  500,  492,  495,  495,  495,    0,    0,  495,    0,
    495,  495,  495,  495,  495,  495,  496,  496,  496,  500,
    0,  496,    0,  496,  496,  496,  496,  496,  496,  501,
    501,  501,    0,    0,  501,    0,  501,  501,  501,  501,

    501,  501,  502,  502,  502,    0,    0,  502,    0,  502,
    502,  502,  502,  502,  502,  508,  508,  508,    0,    0,
    508,    0,    0,    0,  508,  508,  508,  508,  509,  509,
    509,    0,    0,  509,    0,  509,  509,  509,  509,  509,
    509,  510,  510,  510,    0,    0,  510,    0,  510,  510,
    510,  510,  510,  510,  511,  511,  511,    0,    0,  511,
    0,    0,  511,  511,  511,  511,  511,    0,    0,    0,
    511,  513,  513,  513,  513,  513,  514,  514,  514,  514,
    522,  522,  522,  522,  523,  523,  523,  523,  511,  512,
    512,  512,    0,    0,  512,    0,  512,  512,  512,  512,

    512,  512,  515,  515,  515,    0,    0,  515,    0,    0,
    515,  515,  515,  515,  515,    0,    0,    0,  515,  525,
    525,  525,  525,  525,  524,  524,  524,  524,  524,  928,
    928,  928,  524,    0,  928,    0,  515,  516,  516,  516,
    0,    0,  516,    0,  516,  516,  516,  516,  516,  516,
    524,  526,  526,  526,  526,  527,  527,  527,  527,  527,
    0,    0,    0,  527,  534,  534,  534,  534,  540,  540,
    540,  540,  540,  541,  541,  541,  541,  549,  549,  549,
    549,  527,  528,  528,  528,    0,    0,  528,    0,  528,
    528,  528,  528,  528,  528,  529,  529,  529,    0,    0,

    529,    0,  529,  529,  529,  529,  529,  529,  535,  535,
    535,    0,    0,  535,    0,    0,    0,  535,  535,  535,
    535,  536,  536,  536,    0,    0,  536,    0,  536,  536,
    536,  536,  536,  536,  537,  537,  537,    0,    0,  537,
    0,  537,  537,  537,  537,  537,  537,  538,  538,  538,
    0,    0,  538,    0,    0,  538,  538,  538,  538,  538,
    0,    0,    0,  538,  550,  550,  550,  550,  553,  553,
    553,  553,  551,  551,  551,  551,  551,    0,    0,    0,
    551,  538,  539,  539,  539,    0,    0,  539,    0,  539,
    539,  539,  539,  539,  539,  542,  542,  542,  551,    0,

    542,    0,    0,  542,  542,  542,  542,  542,    0,    0,
    0,  542,  552,  552,  552,  552,  552,  561,  561,  561,
    561,  567,  567,  567,  567,  567,    0,    0,    0,  542,
    543,  543,  543,    0,    0,  543,    0,  543,  543,  543,
    543,  543,  543,  554,  554,  554,    0,    0,  554,    0,
    0,  554,  554,  554,  554,  554,    0,    0,    0,  554,
    560,  560,  560,    0,    0,  560,  568,  568,  568,  568,
    576,  576,  576,  576,    0,    0,  560,  554,  555,  555,
    555,    0,    0,  555,    0,  555,  555,  555,  555,  555,
    555,  556,  556,  556,  560,    0,  556,    0,  556,  556,

    556,  556,  556,  556,  562,  562,  562,    0,    0,  562,
    0,    0,    0,  562,  562,  562,  562,  563,  563,  563,
    0,    0,  563,    0,  563,  563,  563,  563,  563,  563,
    564,  564,  564,    0,    0,  564,    0,  564,  564,  564,
    564,  564,  564,  565,  565,  565,    0,    0,  565,    0,
    0,  565,  565,  565,  565,  565,    0,    0,    0,  565,
    575,  575,  575,    0,    0,  575,  577,  577,  577,  577,
    579,  579,  579,  579,    0,    0,  575,  565,  566,  566,
    566,    0,    0,  566,    0,  566,  566,  566,  566,  566,
    566,  569,  569,  569,  575,    0,  569,    0,    0,  569,

    569,  569,  569,  569,    0,    0,    0,  569,  580,  580,
    580,  580,  584,  584,  584,  584,  584,  585,  585,  585,
    585,  593,  593,  593,  593,  569,  570,  570,  570,    0,
    0,  570,    0,  570,  570,  570,  570,  570,  570,  578,
    578,  578,    0,    0,  578,    0,    0,    0,  578,  578,
    578,  578,    0,    0,    0,  578,  599,  599,  599,  599,
    599,    0,    0,    0,  586,  586,  586,  586,  586,    0,
    0,    0,  586,  578,  581,  581,  581,    0,    0,  581,
    0,  581,  581,  581,  581,  581,  581,  582,  582,  582,
    586,    0,  582,    0,  582,  582,  582,  582,  582,  582,

    587,  587,  587,    0,    0,  587,    0,  587,  587,  587,
    587,  587,  587,  588,  588,  588,    0,    0,  588,    0,
    588,  588,  588,  588,  588,  588,  594,  594,  594,    0,
    0,  594,    0,    0,    0,  594,  594,  594,  594,  595,
    595,  595,    0,    0,  595,    0,  595,  595,  595,  595,
    595,  595,  596,  596,  596,    0,    0,  596,    0,  596,
    596,  596,  596,  596,  596,  597,  597,  597,    0,    0,
    597,    0,    0,  597,  597,  597,  597,  597,    0,    0,
    0,  597,  600,  600,  600,  600,  608,  608,  608,  608,
    609,  609,  609,  609,  691,  691,  691,  691,    0,  597,

    598,  598,  598,    0,    0,  598,    0,  598,  598,  598,
    598,  598,  598,  601,  601,  601,    0,    0,  601,    0,
    0,  601,  601,  601,  601,  601,  611,  611,  611,  601,
    0,  611,    0,    0,    0,  610,  610,  610,  610,  610,
    937,  937,  937,  610,    0,  937,    0,  601,  602,  602,
    602,    0,  937,  602,  611,  602,  602,  602,  602,  602,
    602,  610,  616,  616,  616,  616,    0,  617,  617,  616,
    617,  617,  617,  617,  622,  622,    0,  622,  622,  622,
    622,  692,  692,  692,  692,    0,    0,  616,  618,  618,
    618,    0,    0,  618,  618,  618,  618,  618,  618,  618,

    618,  619,  619,  619,    0,    0,  619,    0,  619,  619,
    619,  619,  619,  619,    0,  627,  627,  619,  627,  627,
    627,  627,  639,  639,    0,  639,  639,  639,  639,  699,
    699,  699,  699,    0,    0,  619,  620,  620,  620,    0,
    0,  620,  620,  620,  620,  620,  620,  620,  620,  621,
    621,  621,    0,    0,  621,    0,    0,    0,  621,  621,
    621,  621,    0,  644,  644,  621,  644,  644,  644,  644,
    704,  704,  704,  704,  704,  643,  643,  643,  643,    0,
    0,    0,  643,  621,  623,  623,  623,    0,    0,  623,
    623,  623,  623,  623,  623,  623,  623,  624,  624,  624,

    643,    0,  624,    0,    0,    0,  624,  624,  624,  624,
    0,  649,  649,  624,  649,  649,  649,  649,  705,  705,
    705,  705,    0,  651,  651,  651,  651,    0,    0,    0,
    651,  624,  625,  625,  625,    0,    0,  625,    0,  625,
    625,  625,  625,  625,  625,  626,  626,  626,  651,    0,
    626,    0,  626,  626,  626,  626,  626,  626,  628,  628,
    628,    0,    0,  628,    0,  628,  628,  628,  628,  628,
    628,  629,  629,  629,    0,    0,  629,    0,  629,  629,
    629,  629,  629,  629,  630,  630,  630,    0,    0,  630,
    0,  630,  630,  630,  630,  630,  630,  631,  631,  631,

    0,    0,  631,  631,  631,  631,  631,  631,  631,  631,
    632,  632,  632,    0,    0,  632,    0,  632,  632,  632,
    632,  632,  632,    0,  652,  652,  632,  652,  652,  652,
    652,  657,  657,    0,  657,  657,  657,  657,  713,  713,
    713,  713,    0,    0,  632,  633,  633,  633,    0,    0,
    633,    0,  633,  633,  633,  633,  633,  633,  634,  634,
    634,    0,    0,  634,    0,  634,  634,  634,  634,  634,
    634,  635,  635,  635,    0,    0,  635,    0,  635,  635,
    635,  635,  635,  635,  636,  636,  636,    0,    0,  636,
    636,  636,  636,  636,  636,  636,  636,  637,  637,  637,

    0,    0,  637,    0,    0,    0,  637,  637,  637,  637,
    0,  662,  662,  637,  662,  662,  662,  662,  674,  674,
    0,  674,  674,  674,  674,  718,  718,  718,  718,  718,
    0,  637,  638,  638,  638,    0,    0,  638,    0,  638,
    638,  638,  638,  638,  638,  640,  640,  640,    0,    0,
    640,    0,  640,  640,  640,  640,  640,  640,  641,  641,
    641,    0,    0,  641,    0,  641,  641,  641,  641,  641,
    641,  642,  642,  642,    0,    0,  642,  642,  642,  642,
    642,  642,  642,  642,  645,  645,  645,    0,    0,  645,
    645,  645,  645,  645,  645,  645,  645,  646,  646,  646,

    0,    0,  646,    0,  646,  646,  646,  646,  646,  646,
    0,  679,  679,  646,  679,  679,  679,  679,  719,  719,
    719,  719,    0,  678,  678,  678,  678,    0,    0,    0,
    678,  646,  647,  647,  647,    0,    0,  647,  647,  647,
    647,  647,  647,  647,  647,  648,  648,  648,  678,    0,
    648,    0,    0,    0,  648,  648,  648,  648,    0,  684,
    684,  648,  684,  684,  684,  684,  727,  727,  727,  727,
    706,  706,  706,  706,  706,    0,    0,    0,  706,  648,
    650,  650,  650,    0,    0,  650,  650,  650,  650,  650,
    650,  650,  650,  653,  653,  653,  706,    0,  653,  653,

    653,  653,  653,  653,  653,  653,  654,  654,  654,    0,
    0,  654,    0,  654,  654,  654,  654,  654,  654,    0,
    0,    0,  654,  698,  698,  698,    0,    0,  698,  728,
    728,  728,  728,  734,  734,  734,  734,  734,    0,  698,
    654,  655,  655,  655,    0,    0,  655,  655,  655,  655,
    655,  655,  655,  655,  656,  656,  656,  698,    0,  656,
    0,    0,    0,  656,  656,  656,  656,    0,    0,    0,
    656,  735,  735,  735,  735,  743,  743,  743,  743,  729,
    729,  729,  729,  729,    0,    0,    0,  729,  656,  658,
    658,  658,    0,    0,  658,  658,  658,  658,  658,  658,

    658,  658,  659,  659,  659,  729,    0,  659,    0,    0,
    0,  659,  659,  659,  659,    0,    0,    0,  659,  744,
    744,  744,  744,  746,  746,  746,  746,  745,  745,  745,
    745,  745,    0,    0,    0,  745,  659,  660,  660,  660,
    0,    0,  660,    0,  660,  660,  660,  660,  660,  660,
    661,  661,  661,  745,    0,  661,    0,  661,  661,  661,
    661,  661,  661,  663,  663,  663,    0,    0,  663,    0,
    663,  663,  663,  663,  663,  663,  664,  664,  664,    0,
    0,  664,    0,  664,  664,  664,  664,  664,  664,  665,
    665,  665,    0,    0,  665,    0,  665,  665,  665,  665,

    665,  665,  666,  666,  666,    0,    0,  666,  666,  666,
    666,  666,  666,  666,  666,  667,  667,  667,    0,    0,
    667,    0,  667,  667,  667,  667,  667,  667,    0,    0,
    0,  667,  751,  751,  751,  751,  752,  752,  752,  752,
    753,  753,  753,  753,  753,    0,    0,    0,  753,  667,
    668,  668,  668,    0,    0,  668,    0,  668,  668,  668,
    668,  668,  668,  669,  669,  669,  753,    0,  669,    0,
    669,  669,  669,  669,  669,  669,  670,  670,  670,    0,
    0,  670,    0,  670,  670,  670,  670,  670,  670,  671,
    671,  671,    0,    0,  671,  671,  671,  671,  671,  671,

    671,  671,  672,  672,  672,    0,    0,  672,    0,    0,
    0,  672,  672,  672,  672,    0,    0,    0,  672,  754,
    754,  754,  754,  755,  755,  755,  755,  762,  762,  762,
    762,  767,  767,  767,  767,    0,  672,  673,  673,  673,
    0,    0,  673,    0,  673,  673,  673,  673,  673,  673,
    675,  675,  675,    0,    0,  675,    0,  675,  675,  675,
    675,  675,  675,  676,  676,  676,    0,    0,  676,    0,
    676,  676,  676,  676,  676,  676,  677,  677,  677,    0,
    0,  677,  677,  677,  677,  677,  677,  677,  677,  680,
    680,  680,    0,    0,  680,  680,  680,  680,  680,  680,

    680,  680,  681,  681,  681,    0,    0,  681,    0,  681,
    681,  681,  681,  681,  681,    0,    0,    0,  681,  761,
    761,  761,    0,    0,  761,  768,  768,  768,  768,  775,
    775,  775,  775,    0,    0,  761,  681,  682,  682,  682,
    0,    0,  682,  682,  682,  682,  682,  682,  682,  682,
    683,  683,  683,  761,    0,  683,    0,    0,    0,  683,
    683,  683,  683,    0,    0,    0,  683,  774,  774,  774,
    0,    0,  774,  780,  780,  780,  780,  780,  781,  781,
    781,  781,    0,  774,  683,  685,  685,  685,    0,    0,
    685,  685,  685,  685,  685,  685,  685,  685,  693,  693,

    693,  774,    0,  693,    0,  693,  693,  693,  693,  693,
    693,  694,  694,  694,    0,    0,  694,    0,  694,  694,
    694,  694,  694,  694,  696,  696,  696,    0,    0,  696,
    0,  696,  696,  696,  696,  696,  696,  697,  697,  697,
    0,    0,  697,    0,  697,  697,  697,  697,  697,  697,
    700,  700,  700,    0,    0,  700,    0,    0,    0,  700,
    700,  700,  700,  701,  701,  701,    0,    0,  701,    0,
    701,  701,  701,  701,  701,  701,  702,  702,  702,    0,
    0,  702,    0,  702,  702,  702,  702,  702,  702,  707,
    707,  707,    0,    0,  707,    0,  707,  707,  707,  707,

    707,  707,  708,  708,  708,    0,    0,  708,    0,  708,
    708,  708,  708,  708,  708,  714,  714,  714,    0,    0,
    714,    0,    0,    0,  714,  714,  714,  714,  715,  715,
    715,    0,    0,  715,    0,  715,  715,  715,  715,  715,
    715,  716,  716,  716,    0,    0,  716,    0,  716,  716,
    716,  716,  716,  716,  717,  717,  717,    0,    0,  717,
    0,    0,  717,  717,  717,  717,  717,    0,    0,    0,
    717,  789,  789,  789,  789,    0,  782,  782,  782,  782,
    782,  938,  938,  938,  782,    0,  938,    0,  717,  720,
    720,  720,    0,    0,  720,    0,    0,  720,  720,  720,

    720,  720,  782,    0,    0,  720,  794,  794,  794,  794,
    794,  795,  795,  795,  795,  803,  803,  803,  803,  804,
    804,  804,  804,  720,  721,  721,  721,    0,    0,  721,
    0,  721,  721,  721,  721,  721,  721,  722,  722,  722,
    0,    0,  722,    0,  722,  722,  722,  722,  722,  722,
    730,  730,  730,    0,    0,  730,    0,  730,  730,  730,
    730,  730,  730,  731,  731,  731,    0,    0,  731,    0,
    731,  731,  731,  731,  731,  731,  732,  732,  732,    0,
    0,  732,    0,    0,  732,  732,  732,  732,  732,    0,
    0,    0,  732,  810,  810,  810,  810,  810,    0,    0,

    0,  805,  805,  805,  805,  805,    0,    0,    0,  805,
    732,  733,  733,  733,    0,    0,  733,    0,  733,  733,
    733,  733,  733,  733,  736,  736,  736,  805,    0,  736,
    0,    0,  736,  736,  736,  736,  736,    0,    0,    0,
    736,  811,  811,  811,  811,  819,  819,  819,  819,  820,
    820,  820,  820,  822,  822,  822,  822,    0,  736,  737,
    737,  737,    0,    0,  737,    0,  737,  737,  737,  737,
    737,  737,  747,  747,  747,    0,    0,  747,    0,    0,
    0,  747,  747,  747,  747,  748,  748,  748,    0,    0,
    748,    0,  748,  748,  748,  748,  748,  748,  749,  749,

    749,    0,    0,  749,    0,  749,  749,  749,  749,  749,
    749,  750,  750,  750,    0,    0,  750,    0,    0,  750,
    750,  750,  750,  750,    0,    0,    0,  750,  827,  827,
    827,  827,  828,  828,  828,  828,  821,  821,  821,  821,
    821,    0,    0,    0,  821,  750,  756,  756,  756,    0,
    0,  756,    0,  756,  756,  756,  756,  756,  756,  757,
    757,  757,  821,    0,  757,    0,  757,  757,  757,  757,
    757,  757,  759,  759,  759,    0,    0,  759,    0,  759,
    759,  759,  759,  759,  759,  760,  760,  760,    0,    0,
    760,    0,  760,  760,  760,  760,  760,  760,  763,  763,

    763,    0,    0,  763,    0,    0,    0,  763,  763,  763,
    763,  764,  764,  764,    0,    0,  764,    0,  764,  764,
    764,  764,  764,  764,  765,  765,  765,    0,    0,  765,
    0,  765,  765,  765,  765,  765,  765,  769,  769,  769,
    0,    0,  769,    0,  769,  769,  769,  769,  769,  769,
    770,  770,  770,    0,    0,  770,    0,  770,  770,  770,
    770,  770,  770,  772,  772,  772,    0,    0,  772,    0,
    772,  772,  772,  772,  772,  772,  773,  773,  773,    0,
    0,  773,    0,  773,  773,  773,  773,  773,  773,  776,
    776,  776,    0,    0,  776,    0,    0,    0,  776,  776,

    776,  776,  777,  777,  777,    0,    0,  777,    0,  777,
    777,  777,  777,  777,  777,  778,  778,  778,    0,    0,
    778,    0,  778,  778,  778,  778,  778,  778,  783,  783,
    783,    0,    0,  783,    0,  783,  783,  783,  783,  783,
    783,  784,  784,  784,    0,    0,  784,    0,  784,  784,
    784,  784,  784,  784,  790,  790,  790,    0,    0,  790,
    0,    0,    0,  790,  790,  790,  790,  791,  791,  791,
    0,    0,  791,    0,  791,  791,  791,  791,  791,  791,
    792,  792,  792,    0,    0,  792,    0,  792,  792,  792,
    792,  792,  792,  793,  793,  793,    0,    0,  793,    0,

    0,  793,  793,  793,  793,  793,    0,    0,    0,  793,
    830,  830,  830,  830,    0,  829,  829,  829,  829,  829,
    939,  939,  939,  829,    0,  939,    0,  793,  796,  796,
    796,    0,    0,  796,    0,    0,  796,  796,  796,  796,
    796,  829,    0,    0,  796,  831,  831,  831,  831,  838,
    838,  838,  838,  843,  843,  843,    0,    0,  843,    0,
    0,  843,  796,  797,  797,  797,    0,    0,  797,    0,
    797,  797,  797,  797,  797,  797,  798,  798,  798,    0,
    0,  798,    0,  798,  798,  798,  798,  798,  798,  806,
    806,  806,    0,    0,  806,    0,  806,  806,  806,  806,

    806,  806,  807,  807,  807,    0,    0,  807,    0,  807,
    807,  807,  807,  807,  807,  808,  808,  808,    0,    0,
    808,    0,    0,  808,  808,  808,  808,  808,    0,    0,
    0,  808,  837,  837,  837,  848,  848,  837,  848,  848,
    848,  848,  865,  865,  865,  865,  865,    0,  837,  808,
    809,  809,  809,    0,    0,  809,    0,  809,  809,  809,
    809,  809,  809,  812,  812,  812,  837,    0,  812,    0,
    0,  812,  812,  812,  812,  812,    0,  853,  853,  812,
    853,  853,  853,  853,  867,  867,  867,  867,  867,  847,
    847,  847,  847,    0,    0,    0,  847,  812,  813,  813,

    813,    0,    0,  813,    0,  813,  813,  813,  813,  813,
    813,  823,  823,  823,  847,    0,  823,    0,    0,    0,
    823,  823,  823,  823,  824,  824,  824,    0,    0,  824,
    0,  824,  824,  824,  824,  824,  824,  825,  825,  825,
    0,    0,  825,    0,  825,  825,  825,  825,  825,  825,
    826,  826,  826,    0,    0,  826,    0,    0,  826,  826,
    826,  826,  826,    0,  858,  858,  826,  858,  858,  858,
    858,  870,  870,  870,  870,  870,  862,  862,  862,  862,
    0,    0,    0,  862,  826,  832,  832,  832,    0,    0,
    832,    0,  832,  832,  832,  832,  832,  832,  833,  833,

    833,  862,    0,  833,    0,  833,  833,  833,  833,  833,
    833,  835,  835,  835,    0,    0,  835,    0,  835,  835,
    835,  835,  835,  835,  836,  836,  836,    0,    0,  836,
    0,  836,  836,  836,  836,  836,  836,  839,  839,  839,
    0,    0,  839,    0,    0,    0,  839,  839,  839,  839,
    840,  840,  840,    0,    0,  840,    0,  840,  840,  840,
    840,  840,  840,  841,  841,  841,    0,    0,  841,    0,
    841,  841,  841,  841,  841,  841,  849,  849,  849,    0,
    0,  849,  849,  849,  849,  849,  849,  849,  849,  850,
    850,  850,    0,    0,  850,    0,  850,  850,  850,  850,

    850,  850,    0,  863,  863,  850,  863,  863,  863,  863,
    864,  864,    0,  864,  864,  864,  864,  872,  872,  872,
    872,  872,    0,  850,  851,  851,  851,    0,    0,  851,
    851,  851,  851,  851,  851,  851,  851,  852,  852,  852,
    0,    0,  852,    0,    0,    0,  852,  852,  852,  852,
    0,  866,  866,  852,  866,  866,  866,  866,  868,  868,
    0,  868,  868,  868,  868,  875,  875,  875,  875,  875,
    0,  852,  854,  854,  854,    0,    0,  854,  854,  854,
    854,  854,  854,  854,  854,  855,  855,  855,    0,    0,
    855,    0,  855,  855,  855,  855,  855,  855,    0,  869,

    869,  855,  869,  869,  869,  869,  871,  871,    0,  871,
    871,  871,  871,  880,  880,  880,  880,    0,    0,  855,
    856,  856,  856,    0,    0,  856,  856,  856,  856,  856,
    856,  856,  856,  857,  857,  857,    0,    0,  857,    0,
    0,    0,  857,  857,  857,  857,    0,  873,  873,  857,
    873,  873,  873,  873,  874,  874,    0,  874,  874,  874,
    874,  881,  881,  881,  881,    0,    0,  857,  859,  859,
    859,    0,    0,  859,  859,  859,  859,  859,  859,  859,
    859,  860,  860,  860,    0,    0,  860,    0,  860,  860,
    860,  860,  860,  860,    0,  876,  876,  860,  876,  876,

    876,  876,  877,  877,  877,    0,    0,  877,  940,  940,
    940,  877,    0,  940,    0,  860,  861,  861,  861,    0,
    0,  861,  861,  861,  861,  861,  861,  861,  861,  882,
    882,  882,    0,    0,  882,    0,  882,  882,  882,  882,
    882,  882,  887,  887,  887,    0,    0,  887,  887,  887,
    888,  888,  888,  888,  896,  896,  896,  896,  887,  904,
    904,  904,  904,  905,  905,  905,  905,  906,  906,  906,
    906,  907,  907,  907,  907,    0,  887,  889,  889,  889,
    0,    0,  889,    0,    0,    0,  889,  889,  889,  889,
    890,  890,  890,    0,    0,  890,    0,  890,  890,  890,

    890,  890,  890,  895,  895,  895,    0,    0,  895,  895,
    895,  908,  908,  908,  908,    0,    0,    0,  908,  895,
    909,  909,  909,  909,  910,  910,  910,  910,  912,  912,
    912,  912,  914,  914,  914,  914,  908,  895,  897,  897,
    897,    0,    0,  897,    0,    0,    0,  897,  897,  897,
    897,  898,  898,  898,    0,    0,  898,    0,  898,  898,
    898,  898,  898,  898,  903,  903,  903,    0,    0,  903,
    903,  903,    0,  911,  911,  911,  911,  911,    0,    0,
    903,  911,  915,  915,  915,  915,    0,    0,  916,  916,
    916,  916,  936,  936,  936,  916,    0,  936,  903,  911,

    913,  913,  913,    0,    0,  913,    0,    0,    0,  913,
    913,  913,  913,  916,  917,  917,  917,  917,  918,  918,
    918,  918,  919,  919,  919,  919,  919,  936,    0,    0,
    919,  920,  920,  920,  920,  921,  921,  921,  921,  922,
    922,  922,  922,  923,  923,  923,  923,    0,  919,  924,
    924,  924,  924,    0,    0,    0,  924,  925,  925,  925,
    925,  926,  926,  926,  926,  927,  927,  927,  927,  927,
    931,  931,  931,  927,  924,  931,  934,  934,  934,    0,
    941,  934,  941,    0,    0,  941,    0,  942,  942,  942,
    0,  927,  942,  943,  944,  943,  944,    0,  943,  944,

    945,  946,  945,  946,    0,  945,  946,  947,    0,  947,
    0,    0,  947,  931,    0,    0,    0,    0,  934,  949,
    949,  949,  949,  949,  949,  949,  949,  950,  950,  950,
    950,  950,  950,  950,  950,  951,  951,  951,  951,  951,
    951,  951,  951,  952,  952,  952,  952,  952,  952,  952,
    952,  953,  953,  953,  953,  953,  953,  953,  953,  954,
    0,    0,  954,  955,    0,    0,  955,    0,  955,  955,
    955,  956,  956,  956,  956,  956,  956,  956,  956,  957,
    0,    0,  957,    0,  957,  957,  958,  958,  958,  958,
    958,  958,  958,  958,  959,  959,    0,  959,    0,  959,

    959,  959,  960,  960,  960,  960,  960,  960,  960,  960,
    961,  961,    0,  961,  961,  961,  961,  961,  962,  962,
    962,  962,  962,  962,  962,  962,  963,  963,  963,  963,
    963,  963,  963,  963,  964,  964,    0,    0,  964,  964,
    966,  966,  966,  966,  966,  966,  966,  966,  967,  967,
    967,  967,  967,  967,  967,  967,  968,  968,  968,  968,
    968,  968,  968,  968,  969,  969,  969,  969,  969,  969,
    969,  969,  971,  971,  971,  971,  971,  971,  971,  971,
    972,  972,  972,  972,  972,  972,  972,  972,  973,  973,
    973,  973,  973,  973,  973,  973,  974,  974,    0,  974,

    0,  974,  974,  974,  975,  975,  975,  975,  975,  975,
    975,  975,  977,  977,    0,  977,  977,  977,  977,  977,
    978,  978,  978,  978,  978,  978,  978,  978,  979,    0,
    0,  979,  980,  980,  980,  980,  980,  980,  980,  980,
    981,  981,  981,  981,  981,  981,  981,  981,  982,  982,
    0,    0,  982,  982,  984,  984,  984,  984,  984,  984,
    984,  984,  986,  986,  986,  986,  986,  986,  986,  986,
    987,  987,  987,  987,  987,  987,  987,  987,  989,  989,
    989,  989,  989,  989,  989,  989,  990,  990,  990,  990,
    990,  990,  990,  990,  991,  991,  991,  991,  991,  991,

    991,  991,  992,  992,  992,  992,  992,  992,  992,  992,
    993,  993,  993,  993,  993,  993,  993,  993,  994,  994,
    994,  994,  994,  994,  994,  994,  995,  995,  995,  995,
    995,  995,  995,  995,  996,  996,  996,  996,  996,  996,
    996,  996,  997,  997,  997,  997,  997,  997,  997,  997,
    998,  998,  998,  998,  998,  998,  998,  998,  999,  999,
    999,  999,  999,  999,  999,  999, 1000, 1000, 1000, 1000,
    1000, 1000, 1000, 1000, 1001, 1001, 1001, 1001, 1001, 1001,
    1001, 1001, 1002, 1002, 1002, 1002, 1002, 1002, 1002, 1002,
    1003, 1003, 1003, 1003, 1003, 1003, 1003, 1003, 1004, 1004,

    1004, 1004, 1004, 1004, 1004, 1004, 1005, 1005, 1005, 1005,
    1005, 1005, 1005, 1005, 1006,    0,    0, 1006, 1007, 1007,
    1007, 1007, 1007, 1007, 1007, 1007, 1008, 1008, 1008, 1008,
    1008, 1008, 1008, 1008, 1009, 1009, 1009, 1009, 1009, 1009,
    1009, 1009, 1011, 1011, 1011, 1011, 1011, 1011, 1011, 1011,
    1012, 1012, 1012, 1012, 1012, 1012, 1012, 1012, 1013, 1013,
    1013, 1013, 1013, 1013, 1013, 1013, 1014, 1014, 1014, 1014,
    1014, 1014, 1014, 1014, 1015, 1015, 1015, 1015, 1015, 1015,
    1015, 1015, 1016, 1016, 1016, 1016, 1016, 1016, 1016, 1016,
    1017, 1017, 1017, 1017, 1017, 1017, 1017, 1017, 1018, 1018,

    1018, 1018, 1018, 1018, 1018, 1018, 1019, 1019, 1019, 1019,
    1019, 1019, 1019, 1019, 1020, 1020, 1020, 1020, 1020, 1020,
    1020, 1020, 1021, 1021, 1021, 1021, 1021, 1021, 1021, 1021,
    1022, 1022, 1022, 1022, 1022, 1022, 1022, 1022, 1023, 1023,
    1023, 1023, 1023, 1023, 1023, 1023, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024, 1025, 1025, 1025, 1025, 1025, 1025,
    1025, 1025, 1026, 1026, 1026, 1026, 1026, 1026, 1026, 1026,
    1027, 1027, 1027, 1027, 1027, 1027, 1027, 1027, 1028, 1028,
    1028, 1028, 1028, 1028, 1028, 1028, 1029,    0,    0, 1029,
    1030, 1030, 1030, 1030, 1030, 1030, 1030, 1030, 1031, 1031,

    1031, 1031, 1031, 1031, 1031, 1031, 1032, 1032, 1032, 1032,
    1032, 1032, 1032, 1032, 1033, 1033, 1033, 1033, 1033, 1033,
    1033, 1033, 1034, 1034, 1034, 1034, 1034, 1034, 1034, 1034,
    1035, 1035, 1035, 1035, 1035, 1035, 1035, 1035, 1036, 1036,
    1036, 1036, 1036, 1036, 1036, 1036, 1037, 1037, 1037, 1037,
    1037, 1037, 1037, 1037, 1038, 1038, 1038, 1038, 1038, 1038,
    1038, 1038, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1041, 1041,
    1041, 1041, 1041, 1041, 1041, 1041, 1042, 1042, 1042, 1042,
    1042, 1042, 1042, 1042, 1043, 1043, 1043, 1043, 1043, 1043,

    1043, 1043, 1044, 1044, 1044, 1044, 1044, 1044, 1044, 1044,
    1045, 1045, 1045, 1045, 1045, 1045, 1045, 1045, 1046, 1046,
    1046, 1046, 1046, 1046, 1046, 1046, 1047, 1047, 1047, 1047,
    1047, 1047, 1047, 1047, 1048, 1048, 1048, 1048, 1048, 1048,
    1048, 1048, 1049, 1049, 1049, 1049, 1049, 1049, 1049, 1049,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1051, 1051,
    1051, 1051, 1051, 1051, 1051, 1051, 1052, 1052, 1052, 1052,
    1052, 1052, 1052, 1052, 1053, 1053, 1053, 1053, 1053, 1053,
    1053, 1053, 1054, 1054, 1054, 1054, 1054, 1054, 1054, 1054,
    1055,    0,    0, 1055, 1056, 1056, 1056, 1056, 1056, 1056,

    1056, 1056, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    1058, 1058, 1058, 1058, 1058, 1058, 1058, 1058, 1059, 1059,
    1059, 1059, 1059, 1059, 1059, 1059, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1062, 1062, 1062, 1062, 1062, 1062, 1062, 1062,
    1063, 1063, 1063, 1063, 1063, 1063, 1063, 1063, 1064, 1064,
    1064, 1064, 1064, 1064, 1064, 1064, 1065, 1065, 1065, 1065,
    1065, 1065, 1065, 1065, 1066, 1066, 1066, 1066, 1066, 1066,
    1066, 1066, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1069, 1069,

    1069, 1069, 1069, 1069, 1069, 1069, 1070, 1070, 1070, 1070,
    1070, 1070, 1070, 1070, 1071, 1071, 1071, 1071, 1071, 1071,
    1071, 1071, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072,
    1073, 1073, 1073, 1073, 1073, 1073, 1073, 1073, 1074, 1074,
    1074, 1074, 1074, 1074, 1074, 1074, 1075, 1075, 1075, 1075,
    1075, 1075, 1075, 1075, 1076, 1076, 1076, 1076, 1076, 1076,
    1076, 1076, 1077, 1077, 1077, 1077, 1077, 1077, 1077, 1077,
    1078, 1078, 1078, 1078, 1078, 1078, 1078, 1078, 1079, 1079,
    1079, 1079, 1079, 1079, 1079, 1079, 1080, 1080, 1080, 1080,
    1080, 1080, 1080, 1080, 1081,    0,    0, 1081, 1082, 1082,

    1082, 1082, 1082, 1082, 1082, 1082, 1083, 1083, 1083, 1083,
    1083, 1083, 1083, 1083, 1084, 1084, 1084, 1084, 1084, 1084,
    1084, 1084, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1086, 1086, 1086, 1086, 1086, 1086, 1086, 1086, 1087, 1087,
    1087, 1087, 1087, 1087, 1087, 1087, 1088, 1088, 1088, 1088,
    1088, 1088, 1088, 1088, 1089, 1089,    0, 1089, 1089, 1089,
    1089, 1089,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,
    948,  948,  948,  948,  948,  948,  948,  948,  948,  948,

    948,  948,  948,  948,  948,  948,  948,  948,  948,  948
} ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

extern int yy_flex_debug;
int yy_flex_debug = 1;

static const short int yy_rule_linenum[49] =
{   0,
    122,  127,  128,  129,  130,  131,  132,  133,  134,  135,
    136,  137,  138,  141,  145,  149,  157,  160,  164,  169,
    177,  178,  180,  184,  195,  199,  205,  206,  211,  220,
    234,  235,  240,  249,  260,  263,  264,  266,  270,  274,
    279,  281,  282,  289,  295,  302,  305,  309
} ;

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
char *yytext;
/**************************************************
 * VRML 2.0 Parser
 * Copyright (C) 1996 Silicon Graphics, Inc.
 *
 * Author(s)    : Gavin Bell
 *                Daniel Woods (first port)
 **************************************************
 */
//#include "tokens.h"
#include <string.h>

// used to reset the lexer input after initialization of VRML nodes
void (*theyyInput)(char *, int &, int);

// We define the YY_INPUT so we an change the input source later
#define YY_INPUT(buf, result, max_size) (*theyyInput)(buf, result, max_size);

/* Current line number */
int currentLineNumber = 1;
void yyResetLineNumber() { currentLineNumber = 1; }

extern void yyerror(const char *);

        /* The YACC parser sets this to a token to direct the lexer */
    /* in cases where just syntax isn't enough: */
int expectToken = 0;

/* True when parsing a multiple-valued field: */
static int parsing_mf = 0;

/* These are used when parsing SFImage fields: */
static int sfImageIntsParsed = 0;
static int sfImageIntsExpected = 0;

#ifdef __cplusplus
extern "C"
#endif
int yywrap() { BEGIN INITIAL; return 1; }

/* Normal state:  parsing nodes.  The initial start state is used */
/* only to recognize the VRML header. */
/* Start tokens for all of the field types, */
/* except for MFNode and SFNode, which are almost completely handled */
/* by the parser: */
/* Big hairy expression for floating point numbers: */
/* Ints are decimal or hex (0x##): */
/* Whitespace.  Using this pattern can screw up currentLineNumber, */
/* so it is only used wherever it is really convenient and it is */
/* extremely unlikely that the user will put in a carriage return */
/* (example: between the floats in an SFVec3f) */
/* And the same pattern without the newline */
/* Legal characters to start an identifier */
/* Legal other characters in an identifier */

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifdef YY_MALLOC_DECL
YY_MALLOC_DECL
#else
#if __STDC__
#ifndef __cplusplus
#include <stdlib.h>
#endif
#else
/* Just try to get by without declaring the routines.  This will fail
 * miserably on non-ANSI systems for which sizeof(size_t) != sizeof(int)
 * or sizeof(void*) != sizeof(int).
 */
#endif
#endif

/* Amount of stuff to slurp up with each read. */
#ifndef YY_READ_BUF_SIZE
#define YY_READ_BUF_SIZE 8192
#endif

/* Copy whatever the last rule matched to the standard output. */

#ifndef ECHO
/* This used to be an fputs(), but since the string might contain NUL's,
 * we now use fwrite().
 */
#define ECHO (void) fwrite( yytext, yyleng, 1, yyout )
#endif

/* Gets input and stuffs it into "buf".  number of characters read, or YY_NULL,
 * is returned in "result".
 */
#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
        if ( yy_current_buffer->yy_is_interactive ) \
                { \
                int c = getc( yyin ); \
                result = c == EOF ? 0 : 1; \
                buf[0] = (char) c; \
                } \
        else if ( ((result = fread( buf, 1, max_size, yyin )) == 0) \
                  && ferror( yyin ) ) \
                YY_FATAL_ERROR( "input in flex scanner failed" );
#endif

/* No semi-colon after return; correct usage is to write "yyterminate();" -
 * we don't want an extra ';' after the "return" because that will cause
 * some compilers to complain about unreachable statements.
 */
#ifndef yyterminate
#define yyterminate() return YY_NULL
#endif

/* Number of entries by which start-condition stack grows. */
#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif

/* Report a fatal error. */
#ifndef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) yy_fatal_error( msg )
#endif

/* Default declaration of generated scanner - a define so the user can
 * easily add parameters.
 */
#ifndef YY_DECL
#define YY_DECL int yylex ( vtkVRMLImporter* self )
#endif

/* Code executed at the beginning of each rule, after yytext and yyleng
 * have been set up.
 */
#ifndef YY_USER_ACTION
#define YY_USER_ACTION
#endif

/* Code executed at the end of each rule. */
#ifndef YY_BREAK
#define YY_BREAK break;
#endif

vtkStandardNewMacro(vtkVRMLImporter);

vtkPoints* vtkVRMLImporter::PointsNew()
{
  vtkPoints* pts = vtkPoints::New();
  this->Internal->Heap.Push(pts);
  return pts;
}

vtkFloatArray* vtkVRMLImporter::FloatArrayNew()
{
  vtkFloatArray* array = vtkFloatArray::New();
  this->Internal->Heap.Push(array);
  return array;
}

vtkIdTypeArray* vtkVRMLImporter::IdTypeArrayNew()
{
  vtkIdTypeArray* array = vtkIdTypeArray::New();
  this->Internal->Heap.Push(array);
  return array;
}

void vtkVRMLImporter::DeleteObject(vtkObject* obj)
{
  for(int i=0; i<this->Internal->Heap.Count(); i++)
    {
    if (obj == this->Internal->Heap[i])
      {
      this->Internal->Heap[i] = 0;
      }
    }
  obj->Delete();
}

int yylex ( vtkVRMLImporter* self )
{
  register yy_state_type yy_current_state;
  register char *yy_cp, *yy_bp;
  register int yy_act;




  /* Switch into a new start state if the parser */
  /* just told us that we've read a field name */
  /* and should expect a field value (or IS) */
  if (expectToken != 0) {
  if (yy_flex_debug)
    fprintf(stderr,"LEX--> Start State %d\n", expectToken);

  /*
   * Annoying.  This big switch is necessary because
   * LEX wants to assign particular numbers to start
   * tokens, and YACC wants to define all the tokens
   * used, too.  Sigh.
   */
  switch(expectToken) {
  case SFBOOL: BEGIN SFB; break;
  case SFCOLOR: BEGIN SFC; break;
  case SFFLOAT: BEGIN SFF; break;
  case SFIMAGE: BEGIN SFIMG; break;
  case SFINT32: BEGIN SFI; break;
  case SFROTATION: BEGIN SFR; break;
  case SFSTRING: BEGIN SFS; break;
  case SFTIME: BEGIN SFT; break;
  case SFVEC2F: BEGIN SFV2; break;
  case SFVEC3F: BEGIN SFV3; break;
  case MFCOLOR: BEGIN MFC; break;
  case MFFLOAT: BEGIN MFF; break;
  case MFINT32: BEGIN MFI; break;
  case MFROTATION: BEGIN MFR; break;
  case MFSTRING: BEGIN MFS; break;
  case MFVEC2F: BEGIN MFV2; break;
  case MFVEC3F: BEGIN MFV3; break;

    /* SFNode and MFNode are special.  Here the lexer just returns */
    /* "marker tokens" so the parser knows what type of field is */
    /* being parsed; unlike the other fields, parsing of SFNode/MFNode */
    /* field happens in the parser. */
  case MFNODE: expectToken = 0; return MFNODE;
  case SFNODE: expectToken = 0; return SFNODE;

  default: yyerror("ACK: Bad expectToken"); break;
  }
  }


  /* This is more complicated than they really need to be because */
  /* I was ambitious and made the whitespace-matching rule aggressive */

  if ( yy_init )
    {
#ifdef YY_USER_INIT
    YY_USER_INIT;
#endif

    if ( ! yy_start )
      yy_start = 1;   /* first start state */

    if ( ! yyin )
      yyin = stdin;

    if ( ! yyout )
      yyout = stdout;

    if ( yy_current_buffer )
      yy_init_buffer( yy_current_buffer, yyin );
    else
      yy_current_buffer =
        yy_create_buffer( yyin, YY_BUF_SIZE );

    yy_load_buffer_state();

    yy_init = 0;
    }

  while ( 1 )             /* loops until end-of-file is reached */
    {
    yy_cp = yy_c_buf_p;

    /* Support of yytext. */
    *yy_cp = yy_hold_char;

    /* yy_bp points to the position in yy_ch_buf of the start of
     * the current run.
     */
    yy_bp = yy_cp;

    yy_current_state = yy_start;
      yy_match:
    do
      {
      register YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)];
      if ( yy_accept[yy_current_state] )
        {
        yy_last_accepting_state = yy_current_state;
        yy_last_accepting_cpos = yy_cp;
        }
      while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
        {
        yy_current_state = (int) yy_def[yy_current_state];
        if ( yy_current_state >= 949 )
          yy_c = yy_meta[(unsigned int) yy_c];
        }
      yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
      ++yy_cp;
      }
    while ( yy_base[yy_current_state] != 7663 );

      yy_find_action:
    yy_act = yy_accept[yy_current_state];

    YY_DO_BEFORE_ACTION;


      do_action:      /* This label is used only to access EOF actions. */

    if ( yy_flex_debug )
      {
      if ( yy_act == 0 )
        fprintf( stderr, "--scanner backing up\n" );
      else if ( yy_act < 49 )
        fprintf( stderr, "--accepting rule at line %d (\"%s\")\n",
                 yy_rule_linenum[yy_act], yytext );
      else if ( yy_act == 49 )
        fprintf( stderr, "--accepting default rule (\"%s\")\n",
                 yytext );
      else if ( yy_act == 50 )
        fprintf( stderr, "--(end of buffer or a NUL)\n" );
      else
        fprintf( stderr, "--EOF (start condition %d)\n", YY_START );
      }

    switch ( yy_act )
      { /* beginning of action switch */
      case 0: /* must back up */
        /* undo the effects of YY_DO_BEFORE_ACTION */
        *yy_cp = yy_hold_char;
        yy_cp = yy_last_accepting_cpos;
        yy_current_state = yy_last_accepting_state;
        goto yy_find_action;

      case 1:
        YY_USER_ACTION
          { BEGIN NODE; }
        YY_BREAK
          /* The lexer is in the NODE state when parsing nodes, either */
          /* top-level nodes in the .wrl file, in a prototype implementation, */
          /* or when parsing the contents of SFNode or MFNode fields. */
          case 2:
            YY_USER_ACTION
          { return PROTO; }
      case 3:
        YY_USER_ACTION
          { return EXTERNPROTO; }
      case 4:
        YY_USER_ACTION
          { return DEF; }
      case 5:
        YY_USER_ACTION
          { return USE; }
      case 6:
        YY_USER_ACTION
          { return TO; }
      case 7:
        YY_USER_ACTION
          { return IS; }
      case 8:
        YY_USER_ACTION
          { return ROUTE; }
      case 9:
        YY_USER_ACTION
          { return SFN_NULL; }
      case 10:
        YY_USER_ACTION
          { return EVENTIN; }
      case 11:
        YY_USER_ACTION
          { return EVENTOUT; }
      case 12:
        YY_USER_ACTION
          { return FIELD; }
      case 13:
        YY_USER_ACTION
          { return EXPOSEDFIELD; }
        /* Legal identifiers: */
      case 14:
        YY_USER_ACTION
          {
          yylval.string = vtkVRMLAllocator::StrDup(yytext);
          return IDENTIFIER; }
        /* All fields may have an IS declaration: */
      case 15:
        YY_USER_ACTION
          { BEGIN NODE;
          expectToken = 0;
          yyless(0);
          }
        YY_BREAK
      case 16:
            YY_USER_ACTION
          { BEGIN NODE;
          expectToken = 0;
          yyless(0); /* put back the IS */
          }
        YY_BREAK
          /* All MF field types other than MFNode are completely parsed here */
          /* in the lexer, and one token is returned to the parser.  They all */
          /* share the same rules for open and closing brackets: */
      case 17:
            YY_USER_ACTION
          { if (parsing_mf) yyerror("Double [");
          parsing_mf = 1;
          yylval.vec2f = self->FloatArrayNew();
          yylval.vec2f->SetNumberOfComponents(2);
          }
        YY_BREAK
      case 18:
            YY_USER_ACTION
          { if (parsing_mf) yyerror("Double [");
          parsing_mf = 1;
          yylval.mfint32 = self->IdTypeArrayNew();
          }
        YY_BREAK
      case 19:
            YY_USER_ACTION
          { if (parsing_mf) yyerror("Double [");
          parsing_mf = 1;
          yylval.vec3f = self->PointsNew();
          }
        YY_BREAK
      case 20:
            YY_USER_ACTION
          { if (!parsing_mf) yyerror("Unmatched ]");
          int fieldType = expectToken;
          BEGIN NODE;
          parsing_mf = 0;
          expectToken = 0;
          return fieldType;
          }
      case 21:
        YY_USER_ACTION
          { BEGIN NODE; expectToken = 0; yylval.sfint = 1; return SFBOOL; }
      case 22:
        YY_USER_ACTION
          { BEGIN NODE; expectToken = 0; yylval.sfint = 0; return SFBOOL; }
      case 23:
        YY_USER_ACTION
          { BEGIN NODE; expectToken = 0;
          yylval.sfint = atoi(yytext);
          return SFINT32;
          }
      case 24:
        YY_USER_ACTION
          { if (parsing_mf) {
          int num;
          num = atoi(yytext);
          yylval.mfint32->InsertNextValue(num);
          }
          else {
          BEGIN NODE; expectToken = 0; return MFINT32;
          }
          }
        YY_BREAK
          /* All the floating-point types are pretty similar: */
          case 25:
            YY_USER_ACTION
          { BEGIN NODE; expectToken = 0; float num;
          sscanf(yytext, "%f", &num);
          yylval.sffloat = num;
          return SFFLOAT; }
      case 26:
        YY_USER_ACTION
          { if (parsing_mf) ; /* Add to array... */
          else {
          /* No open bracket means a single value: */
          BEGIN NODE; expectToken = 0; return MFFLOAT;
          }
          }
        YY_BREAK
          case 27:
            YY_USER_ACTION
          { BEGIN NODE; expectToken = 0; return SFVEC2F; }
      case 28:
        YY_USER_ACTION
        {
        if (parsing_mf)
          {
          // .. add to array...
          float num[2];
          num[0] = atof(strtok(yytext, " "));
          num[1] = atof(strtok(NULL, " "));
          // equivalent to: sscanf(yytext, "%f %f", &num[0], &num[1]);
          yylval.vec2f->InsertNextTuple(num);
          }
        else
          {
          BEGIN NODE; expectToken = 0;
          return MFVEC2F;
          }
        }
         YY_BREAK
      case 29:
            YY_USER_ACTION
          {   BEGIN NODE; expectToken = 0;
          float num[3];
          yylval.vec3f = self->PointsNew();
          num[0] = atof(strtok(yytext, " "));
          num[1] = atof(strtok(NULL, " "));
          num[2] = atof(strtok(NULL, " "));
          //sscanf(yytext, "%f %f %f", &num[0], &num[1], &num[2]);
          yylval.vec3f->InsertPoint(0, num);
          return SFVEC3F; }
      case 30:
        YY_USER_ACTION
          { if (parsing_mf) { /*  .. add to array... */
          float num[3];
          num[0] = atof(strtok(yytext, " "));
          num[1] = atof(strtok(NULL, " "));
          num[2] = atof(strtok(NULL, " "));
          //sscanf(yytext, "%f %f %f", &num[0], &num[1], &num[2]);
          yylval.vec3f->InsertNextPoint(num);
          //return MFVEC3F;
          }
          else {
          BEGIN NODE; expectToken = 0;
          return MFVEC3F;
          }
          }
        YY_BREAK
      case 31:
            YY_USER_ACTION
          { BEGIN NODE; expectToken = 0; return SFROTATION; }
      case 32:
        YY_USER_ACTION
          { if (parsing_mf) ; /* .. add to array... */
          else {
          BEGIN NODE; expectToken = 0; return MFROTATION;
          }
          }
        YY_BREAK
      case 33:
            YY_USER_ACTION
          { BEGIN NODE; expectToken = 0;
          float num[3];
          yylval.vec3f = self->PointsNew();
          num[0] = atof(strtok(yytext, " "));
          num[1] = atof(strtok(NULL, " "));
          num[2] = atof(strtok(NULL, " "));
          //sscanf(yytext, "%f %f %f", &num[0], &num[1], &num[2]);
          yylval.vec3f->InsertPoint(0, num);
          return SFCOLOR; }
      case 34:
        YY_USER_ACTION
          { if (parsing_mf) { /*  .. add to array... */
          float num[3];
          num[0] = atof(strtok(yytext, " "));
          num[1] = atof(strtok(NULL, " "));
          num[2] = atof(strtok(NULL, " "));
          yylval.vec3f->InsertNextPoint(num);
          }
          else {
          BEGIN NODE; expectToken = 0; return MFCOLOR;
          }
          }
        YY_BREAK
      case 35:
            YY_USER_ACTION
          { BEGIN NODE; expectToken = 0; return SFTIME; }
        /* SFString/MFString */
      case 36:
        YY_USER_ACTION
          { BEGIN IN_SFS; }
        YY_BREAK
          case 37:
            YY_USER_ACTION
          { BEGIN IN_MFS; }
        YY_BREAK
          /* Anything besides open-quote (or whitespace) is an error: */
      case 38:
            YY_USER_ACTION
          { yyerror("String missing open-quote");
          BEGIN NODE; expectToken = 0; return SFSTRING;
          }
        /* Expect open-quote, open-bracket, or whitespace: */
      case 39:
        YY_USER_ACTION
          { yyerror("String missing open-quote");
          BEGIN NODE; expectToken = 0; return MFSTRING;
          }
        /* Backslashed-quotes are OK: */
      case 40:
        YY_USER_ACTION
          ;
        YY_BREAK
          /* Gobble up anything besides quotes and newlines. */
          /* Newlines are legal in strings, but we exclude them here so */
          /* that line number are counted correctly by the catch-all newline */
          /* rule that applies to everything. */
      case 41:
            YY_USER_ACTION
          ;
        YY_BREAK
          /* Quote ends the string: */
      case 42:
            YY_USER_ACTION
          { BEGIN NODE; expectToken = 0; return SFSTRING; }
      case 43:
        YY_USER_ACTION
          { if (parsing_mf) BEGIN MFS; /* ... add to array ... */
          else {
          BEGIN NODE; expectToken = 0; return MFSTRING;
          }
          }
        YY_BREAK
          /* SFImage: width height numComponents then width*height integers: */
      case 44:
            YY_USER_ACTION
          { int w, h;
          sscanf(yytext, "%d %d", &w, &h);
          sfImageIntsExpected = 1+w*h;
          sfImageIntsParsed = 0;
          BEGIN IN_SFIMG;
          }
        YY_BREAK
      case 45:
            YY_USER_ACTION
          { ++sfImageIntsParsed;
          if (sfImageIntsParsed == sfImageIntsExpected) {
          BEGIN NODE; expectToken = 0; return SFIMAGE;
          }
          }
        YY_BREAK
          /* Whitespace and catch-all rules apply to all start states: */
      case 46:
            YY_USER_ACTION
          ;
        YY_BREAK
          /* This is also whitespace, but we'll keep track of line number */
          /* to report in errors: */
      case 47:
            YY_USER_ACTION
          { ++currentLineNumber; }
        YY_BREAK
          /* This catch-all rule catches anything not covered by any of */
          /* the above: */
          case 48:
            YY_USER_ACTION
          { return yytext[0]; }
      case 49:
        YY_USER_ACTION
          YY_FATAL_ERROR( "flex scanner jammed" );
        YY_BREAK
      case YY_STATE_EOF(INITIAL):
      case YY_STATE_EOF(NODE):
      case YY_STATE_EOF(SFB):
      case YY_STATE_EOF(SFC):
      case YY_STATE_EOF(SFF):
      case YY_STATE_EOF(SFIMG):
      case YY_STATE_EOF(SFI):
      case YY_STATE_EOF(SFR):
      case YY_STATE_EOF(SFS):
      case YY_STATE_EOF(SFT):
      case YY_STATE_EOF(SFV2):
      case YY_STATE_EOF(SFV3):
      case YY_STATE_EOF(MFC):
      case YY_STATE_EOF(MFF):
      case YY_STATE_EOF(MFI):
      case YY_STATE_EOF(MFR):
      case YY_STATE_EOF(MFS):
      case YY_STATE_EOF(MFV2):
      case YY_STATE_EOF(MFV3):
      case YY_STATE_EOF(IN_SFS):
      case YY_STATE_EOF(IN_MFS):
      case YY_STATE_EOF(IN_SFIMG):
        yyterminate();

      case YY_END_OF_BUFFER:
      {
      /* Amount of text matched not including the EOB char. */
      int yy_amount_of_matched_text = yy_cp - yytext_ptr - 1;

      /* Undo the effects of YY_DO_BEFORE_ACTION. */
      *yy_cp = yy_hold_char;

      if ( yy_current_buffer->yy_buffer_status == YY_BUFFER_NEW )
        {
        /* We're scanning a new file or input source.  It's
         * possible that this happened because the user
         * just pointed yyin at a new source and called
         * yylex().  If so, then we have to assure
         * consistency between yy_current_buffer and our
         * globals.  Here is the right place to do so, because
         * this is the first action (other than possibly a
         * back-up) that will match for the new input source.
         */
        yy_n_chars = yy_current_buffer->yy_n_chars;
        yy_current_buffer->yy_input_file = yyin;
        yy_current_buffer->yy_buffer_status = YY_BUFFER_NORMAL;
        }

      /* Note that here we test for yy_c_buf_p "<=" to the position
       * of the first EOB in the buffer, since yy_c_buf_p will
       * already have been incremented past the NUL character
       * (since all states make transitions on EOB to the
       * end-of-buffer state).  Contrast this with the test
       * in input().
       */
      if ( yy_c_buf_p <= &yy_current_buffer->yy_ch_buf[yy_n_chars] )
        { /* This was really a NUL. */
        yy_state_type yy_next_state;

        yy_c_buf_p = yytext_ptr + yy_amount_of_matched_text;

        yy_current_state = yy_get_previous_state();

        /* Okay, we're now positioned to make the NUL
         * transition.  We couldn't have
         * yy_get_previous_state() go ahead and do it
         * for us because it doesn't know how to deal
         * with the possibility of jamming (and we don't
         * want to build jamming into it because then it
         * will run more slowly).
         */

        yy_next_state = yy_try_NUL_trans( yy_current_state );

        yy_bp = yytext_ptr + YY_MORE_ADJ;

        if ( yy_next_state )
          {
                                /* Consume the NUL. */
          yy_cp = ++yy_c_buf_p;
          yy_current_state = yy_next_state;
          goto yy_match;
          }

        else
          {
          yy_cp = yy_c_buf_p;
          goto yy_find_action;
          }
        }

      else switch ( yy_get_next_buffer() )
        {
        case EOB_ACT_END_OF_FILE:
        {
        yy_did_buffer_switch_on_eof = 0;

        if ( yywrap() )
          {
          /* Note: because we've taken care in
           * yy_get_next_buffer() to have set up
           * yytext, we can now set up
           * yy_c_buf_p so that if some total
           * hoser (like flex itself) wants to
           * call the scanner after we return the
           * YY_NULL, it'll still work - another
           * YY_NULL will get returned.
           */
          yy_c_buf_p = yytext_ptr + YY_MORE_ADJ;

          yy_act = YY_STATE_EOF(YY_START);
          goto do_action;
          }

        else
          {
          if ( ! yy_did_buffer_switch_on_eof )
            YY_NEW_FILE;
          }
        break;
        }

        case EOB_ACT_CONTINUE_SCAN:
          yy_c_buf_p =
            yytext_ptr + yy_amount_of_matched_text;

          yy_current_state = yy_get_previous_state();

          yy_cp = yy_c_buf_p;
          yy_bp = yytext_ptr + YY_MORE_ADJ;
          goto yy_match;

        case EOB_ACT_LAST_MATCH:
          yy_c_buf_p =
            &yy_current_buffer->yy_ch_buf[yy_n_chars];

          yy_current_state = yy_get_previous_state();

          yy_cp = yy_c_buf_p;
          yy_bp = yytext_ptr + YY_MORE_ADJ;
          goto yy_find_action;
        }
      break;
      }

      default:
        YY_FATAL_ERROR(
          "fatal flex scanner internal error--no action found" );
      } /* end of action switch */
    } /* end of scanning one token */
} /* end of yylex */


/* yy_get_next_buffer - try to read in a new buffer
 *
 * Returns a code representing an action:
 *      EOB_ACT_LAST_MATCH -
 *      EOB_ACT_CONTINUE_SCAN - continue scanning from current position
 *      EOB_ACT_END_OF_FILE - end of file
 */

static int yy_get_next_buffer()
{
  register char *dest = yy_current_buffer->yy_ch_buf;
  register char *source = yytext_ptr - 1; /* copy prev. char, too */
  register int number_to_move, i;
  int ret_val;

  if ( yy_c_buf_p > &yy_current_buffer->yy_ch_buf[yy_n_chars + 1] )
    YY_FATAL_ERROR(
      "fatal flex scanner internal error--end of buffer missed" );

  if ( yy_current_buffer->yy_fill_buffer == 0 )
    { /* Don't try to fill the buffer, so this is an EOF. */
    if ( yy_c_buf_p - yytext_ptr - YY_MORE_ADJ == 1 )
      {
      /* We matched a singled characater, the EOB, so
       * treat this as a final EOF.
       */
      return EOB_ACT_END_OF_FILE;
      }

    else
      {
      /* We matched some text prior to the EOB, first
       * process it.
       */
      return EOB_ACT_LAST_MATCH;
      }
    }

  /* Try to read more data. */

  /* First move last chars to start of buffer. */
  number_to_move = yy_c_buf_p - yytext_ptr;

  for ( i = 0; i < number_to_move; ++i )
    *(dest++) = *(source++);

  if ( yy_current_buffer->yy_buffer_status == YY_BUFFER_EOF_PENDING )
    /* don't do the read, it's not guaranteed to return an EOF,
     * just force an EOF
     */
    yy_n_chars = 0;

  else
    {
    int num_to_read =
      yy_current_buffer->yy_buf_size - number_to_move - 1;

    while ( num_to_read <= 0 )
      { /* Not enough room in the buffer - grow it. */
#ifdef YY_USES_REJECT
      YY_FATAL_ERROR(
        "input buffer overflow, can't enlarge buffer because scanner uses REJECT" );
#else

      /* just a shorter name for the current buffer */
      YY_BUFFER_STATE b = yy_current_buffer;

      int yy_c_buf_p_offset = yy_c_buf_p - b->yy_ch_buf;

      b->yy_buf_size *= 2;
      b->yy_ch_buf = (char *)
        yy_flex_realloc( (void *) b->yy_ch_buf,
                         b->yy_buf_size );

      if ( ! b->yy_ch_buf )
        YY_FATAL_ERROR(
          "fatal error - scanner input buffer overflow" );

      yy_c_buf_p = &b->yy_ch_buf[yy_c_buf_p_offset];

      num_to_read = yy_current_buffer->yy_buf_size -
        number_to_move - 1;
#endif
      }

    if ( num_to_read > YY_READ_BUF_SIZE )
      num_to_read = YY_READ_BUF_SIZE;

    /* Read in more data. */
    YY_INPUT( (&yy_current_buffer->yy_ch_buf[number_to_move]),
              yy_n_chars, num_to_read );
    }

  if ( yy_n_chars == 0 )
    {
    if ( number_to_move - YY_MORE_ADJ == 1 )
      {
      ret_val = EOB_ACT_END_OF_FILE;
      yyrestart( yyin );
      }

    else
      {
      ret_val = EOB_ACT_LAST_MATCH;
      yy_current_buffer->yy_buffer_status =
        YY_BUFFER_EOF_PENDING;
      }
    }

  else
    ret_val = EOB_ACT_CONTINUE_SCAN;

  yy_n_chars += number_to_move;
  yy_current_buffer->yy_ch_buf[yy_n_chars] = YY_END_OF_BUFFER_CHAR;
  yy_current_buffer->yy_ch_buf[yy_n_chars + 1] = YY_END_OF_BUFFER_CHAR;

  /* yytext begins at the second character in yy_ch_buf; the first
   * character is the one which preceded it before reading in the latest
   * buffer; it needs to be kept around in case it's a newline, so
   * yy_get_previous_state() will have with '^' rules active.
   */

  yytext_ptr = &yy_current_buffer->yy_ch_buf[1];

  return ret_val;
}


/* yy_get_previous_state - get the state just before the EOB char was reached */

static yy_state_type yy_get_previous_state()
{
  register yy_state_type yy_current_state;
  register char *yy_cp;

  yy_current_state = yy_start;

  for ( yy_cp = yytext_ptr + YY_MORE_ADJ; yy_cp < yy_c_buf_p; ++yy_cp )
    {
    register YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 1);
    if ( yy_accept[yy_current_state] )
      {
      yy_last_accepting_state = yy_current_state;
      yy_last_accepting_cpos = yy_cp;
      }
    while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
      {
      yy_current_state = (int) yy_def[yy_current_state];
      if ( yy_current_state >= 949 )
        yy_c = yy_meta[(unsigned int) yy_c];
      }
    yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
    }

  return yy_current_state;
}


/* yy_try_NUL_trans - try to make a transition on the NUL character
 *
 * synopsis
 *      next_state = yy_try_NUL_trans( current_state );
 */

#ifdef YY_USE_PROTOS
static yy_state_type yy_try_NUL_trans( yy_state_type yy_current_state )
#else
  static yy_state_type yy_try_NUL_trans( yy_current_state )
  yy_state_type yy_current_state;
#endif
{
  register int yy_is_jam;
  register char *yy_cp = yy_c_buf_p;

  register YY_CHAR yy_c = 1;
  if ( yy_accept[yy_current_state] )
    {
    yy_last_accepting_state = yy_current_state;
    yy_last_accepting_cpos = yy_cp;
    }
  while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
    {
    yy_current_state = (int) yy_def[yy_current_state];
    if ( yy_current_state >= 949 )
      yy_c = yy_meta[(unsigned int) yy_c];
    }
  yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
  yy_is_jam = (yy_current_state == 948);

  return yy_is_jam ? 0 : yy_current_state;
}



#ifdef YY_USE_PROTOS
void yyrestart( FILE *input_file )
#else
  void yyrestart( input_file )
  FILE *input_file;
#endif
{
  if ( ! yy_current_buffer )
    yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE );

  yy_init_buffer( yy_current_buffer, input_file );
  yy_load_buffer_state();
}


#ifdef YY_USE_PROTOS
void yy_switch_to_buffer( YY_BUFFER_STATE new_buffer )
#else
  void yy_switch_to_buffer( new_buffer )
  YY_BUFFER_STATE new_buffer;
#endif
{
  if ( yy_current_buffer == new_buffer )
    return;

  if ( yy_current_buffer )
    {
    /* Flush out information for old buffer. */
    *yy_c_buf_p = yy_hold_char;
    yy_current_buffer->yy_buf_pos = yy_c_buf_p;
    yy_current_buffer->yy_n_chars = yy_n_chars;
    }

  yy_current_buffer = new_buffer;
  yy_load_buffer_state();

  /* We don't actually know whether we did this switch during
   * EOF (yywrap()) processing, but the only time this flag
   * is looked at is after yywrap() is called, so it's safe
   * to go ahead and always set it.
   */
  yy_did_buffer_switch_on_eof = 1;
}


#ifdef YY_USE_PROTOS
void yy_load_buffer_state( void )
#else
  void yy_load_buffer_state()
#endif
{
  yy_n_chars = yy_current_buffer->yy_n_chars;
  yytext_ptr = yy_c_buf_p = yy_current_buffer->yy_buf_pos;
  yyin = yy_current_buffer->yy_input_file;
  yy_hold_char = *yy_c_buf_p;
}


#ifdef YY_USE_PROTOS
YY_BUFFER_STATE yy_create_buffer( FILE *file, int size )
#else
  YY_BUFFER_STATE yy_create_buffer( file, size )
  FILE *file;
int size;
#endif
{
  YY_BUFFER_STATE b;

  b = (YY_BUFFER_STATE) yy_flex_alloc( sizeof( struct yy_buffer_state ) );

  if ( ! b )
    YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

  b->yy_buf_size = size;

  /* yy_ch_buf has to be 2 characters longer than the size given because
   * we need to put in 2 end-of-buffer characters.
   */
  b->yy_ch_buf = (char *) yy_flex_alloc( b->yy_buf_size + 2 );

  if ( ! b->yy_ch_buf )
    YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

  yy_init_buffer( b, file );

  return b;
}


#ifdef YY_USE_PROTOS
void yy_delete_buffer( YY_BUFFER_STATE b )
#else
  void yy_delete_buffer( b )
  YY_BUFFER_STATE b;
#endif
{
  if ( b == yy_current_buffer )
    yy_current_buffer = (YY_BUFFER_STATE) 0;

  yy_flex_free( (void *) b->yy_ch_buf );
  yy_flex_free( (void *) b );
}


#ifdef YY_USE_PROTOS
void yy_init_buffer( YY_BUFFER_STATE b, FILE *file )
#else
  void yy_init_buffer( b, file )
  YY_BUFFER_STATE b;
FILE *file;
#endif
{
  b->yy_input_file = file;

  /* We put in the '\n' and start reading from [1] so that an
   * initial match-at-newline will be true.
   */

  b->yy_ch_buf[0] = '\n';
  b->yy_n_chars = 1;

  /* We always need two end-of-buffer characters.  The first causes
   * a transition to the end-of-buffer state.  The second causes
   * a jam in that state.
   */
  b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;
  b->yy_ch_buf[2] = YY_END_OF_BUFFER_CHAR;

  b->yy_buf_pos = &b->yy_ch_buf[1];

  b->yy_is_interactive = file ? isatty( fileno(file) ) : 0;

  b->yy_fill_buffer = 1;

  b->yy_buffer_status = YY_BUFFER_NEW;
}


#ifdef YY_USE_PROTOS
static void yy_fatal_error( const char msg[] )
#else
  static void yy_fatal_error( msg )
  char msg[];
#endif
{
  (void) fprintf( stderr, "%s\n", msg );
  exit( 1 );
}



/* Redefine yyless() so it works in section 3 code. */

#undef yyless
#define yyless(n) \
        do \
                { \
                /* Undo effects of setting up yytext. */ \
                yytext[yyleng] = yy_hold_char; \
                yy_c_buf_p = yytext + n - YY_MORE_ADJ; \
                yy_hold_char = *yy_c_buf_p; \
                *yy_c_buf_p = '\0'; \
                yyleng = n; \
                } \
        while ( 0 )


/* Internal utility routines. */

#ifndef yytext_ptr
#ifdef YY_USE_PROTOS
static void yy_flex_strncpy( char *s1, const char *s2, int n )
#else
  static void yy_flex_strncpy( s1, s2, n )
  char *s1;
const char *s2;
int n;
#endif
{
  register int i;
  for ( i = 0; i < n; ++i )
    s1[i] = s2[i];
}
#endif


#ifdef YY_USE_PROTOS
static void *yy_flex_alloc( unsigned int size )
#else
  static void *yy_flex_alloc( size )
  unsigned int size;
#endif
{
  return (void *) malloc( size );
}

#ifdef YY_USE_PROTOS
static void *yy_flex_realloc( void *ptr, unsigned int size )
#else
  static void *yy_flex_realloc( ptr, size )
  void *ptr;
unsigned int size;
#endif
{
  return (void *) realloc( ptr, size );
}

#ifdef YY_USE_PROTOS
static void yy_flex_free( void *ptr )
#else
  static void yy_flex_free( ptr )
  void *ptr;
#endif
{
  free( ptr );
}
// End of Auto-generated Lexer Code


vtkVRMLImporter::vtkVRMLImporter ()
{
  this->Internal = new vtkVRMLImporterInternal;
  this->CurrentActor = NULL;
  this->CurrentLight = NULL;
  this->CurrentProperty = NULL;
  this->CurrentCamera = NULL;
  this->CurrentSource = NULL;
  this->CurrentPoints = NULL;
  this->CurrentScalars = NULL;
  this->CurrentNormals = NULL;
  this->CurrentNormalCells = NULL;
  this->CurrentTCoords = NULL;
  this->CurrentTCoordCells = NULL;
  this->CurrentMapper = NULL;
  this->CurrentLut = NULL;
  this->CurrentTransform = vtkTransform::New();
  this->FileName = NULL;
  this->FileFD = NULL;
}

// Open an import file. Returns zero if error.
int vtkVRMLImporter::OpenImportFile ()
{
  vtkDebugMacro(<< "Opening import file");

  if ( !this->FileName )
    {
    vtkErrorMacro(<< "No file specified!");
    return 0;
    }
  this->FileFD = fopen (this->FileName, "r");
  if (this->FileFD == NULL)
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    return 0;
    }
  return 1;
}

int vtkVRMLImporter::ImportBegin ()
{

  memyyInput_i = 0;
  memyyInput_j = 0;

  vtkVRMLAllocator::Initialize();
  VrmlNodeType::typeList = new vtkVRMLVectorType<VrmlNodeType*>;
  VrmlNodeType::typeList->Init();

  VrmlNodeType::useList = new vtkVRMLVectorType<vtkVRMLUseStruct *>;
  VrmlNodeType::useList->Init();

  VrmlNodeType::currentField = new vtkVRMLVectorType<VrmlNodeType::FieldRec *>;
  VrmlNodeType::currentField->Init();

  if (!this->OpenImportFile())
    {
    return 0;
    }

  // This is acrually where it all takes place, Since VRML is a SG
  // And is state based, I need to create actors, cameras, and lights
  // as I go. The ImportXXXXXX rotuines are not used.
  CurrentProtoStack = new vtkVRMLVectorType<VrmlNodeType*>;

  // Lets redefine the YY_INPUT macro on Flex and get chars from memory
  theyyInput = memyyInput;
  // Crank up the yacc parser...
  yydebug = 0;
  yy_flex_debug = 0;
  /*FILE *standardNodes = fopen("standardNodes.wrl", "r");
    if (standardNodes == NULL) {
    cerr << "Error, couldn't open standardNodes.wrl file";
    return 0;
    }
    yyin = standardNodes;*/
  yyparse(this);
  yyin = NULL;
  yyResetLineNumber();
  //fclose(standardNodes);

  // Not sure why I have to do this but its not working when
  // When I use the FileFD file pointer...
  // File existence already checked.
  yyin = fopen(this->FileName, "r");

  // reset the lex input routine
  theyyInput = defyyInput;

  // For this little test application, pushing and popping the node
  // namespace isn't really necessary.  But each VRML .wrl file is
  // a separate namespace for PROTOs (except for things predefined
  // in the spec), and pushing/popping the namespace when reading each
  // file is a good habit to get into:
  VrmlNodeType::pushNameSpace();
  yyparse(this);
  VrmlNodeType::popNameSpace();

  fclose(yyin);
  yyin = NULL;

  delete CurrentProtoStack;

  // In case there was a ViewPoint introduced it usually happens prior
  // to any actors being placed in the scene, need to reset the camera
  //this->Renderer->UpdateActors();
  return 1;
}

void vtkVRMLImporter::ImportEnd ()
{
  delete VrmlNodeType::typeList;
  VrmlNodeType::typeList = 0;

  delete VrmlNodeType::currentField;
  VrmlNodeType::currentField = 0;

  vtkDebugMacro(<<"Closing import file");
  if ( this->FileFD != NULL )
    {
    fclose (this->FileFD);
    }
  this->FileFD = NULL;
}


vtkVRMLImporter::~vtkVRMLImporter()
{
  if (this->CurrentActor)
    {
    this->CurrentActor->Delete();
    }
  if (this->CurrentLight)
    {
    this->CurrentLight->Delete();
    }
  if (this->CurrentProperty)
    {
    this->CurrentProperty->Delete();
    }
  if (this->CurrentCamera)
    {
    this->CurrentCamera->Delete();
    }
  if (this->CurrentSource)
    {
    this->CurrentSource->Delete();
    }
  if (this->CurrentPoints)
    {
    this->CurrentPoints->Delete();
    }
  if (this->CurrentNormals)
    {
    this->CurrentNormals->Delete();
    }
  if (this->CurrentTCoords)
    {
    this->CurrentTCoords->Delete();
    }
  if (this->CurrentTCoordCells)
    {
    this->CurrentTCoordCells->Delete();
    }
  if (this->CurrentNormalCells)
    {
    this->CurrentNormalCells->Delete();
    }
  if (this->CurrentScalars)
    {
    this->CurrentScalars->Delete();
    }
  if (this->CurrentMapper)
    {
    this->CurrentMapper->Delete();
    }
  if (this->CurrentLut)
    {
    this->CurrentLut->Delete();
    }
  this->CurrentTransform->Delete();
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  while(this->Internal->Heap.Count() > 0)
    {
    vtkObject* obj = this->Internal->Heap.Pop();
    if (obj)
      {
      obj->Delete();
      }
    }
  delete this->Internal;

  // According to Tom Citriniti the useList must not be deleted until the
  // instance is destroyed. The importer was crashing when users asked for a
  // DEF node from within the VRML file. This DEF mechanism allows you to
  // name a node inside the VRML file and refer to it from other nodes or
  // from scripts that can be associated with the VRML file. A vector of
  // these is created in the importer and has to live until the class is
  // deleted.
  delete VrmlNodeType::useList;
  VrmlNodeType::useList = 0;
  vtkVRMLAllocator::CleanUp();
}

void vtkVRMLImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << "Defined names in File:" << endl;
  if (VrmlNodeType::useList)
    {
      for (int i = 0;i < VrmlNodeType::useList->Count();i++)
        {
        os << "\tName: " << (*VrmlNodeType::useList)[i]->defName
           << " is a " << (*VrmlNodeType::useList)[i]->defObject->GetClassName()
           << endl;
        }
    }
}

// Yacc/lex routines to add stuff to the renderer.

void
vtkVRMLImporter::enterNode(const char *nodeType)
{
  vtkActor *actor;
  vtkPolyDataMapper *pmap;

  const VrmlNodeType *t = VrmlNodeType::find(nodeType);
  if (t == NULL)
    {
    char tmp[1000];
    sprintf(tmp, "Unknown node type '%s'", nodeType);
    yyerror(tmp);
    exit(99);
    }
  VrmlNodeType::FieldRec *fr = new VrmlNodeType::FieldRec;
  fr->nodeType = t;
  fr->fieldName = NULL;
  *VrmlNodeType::currentField += fr;
  if (strcmp(fr->nodeType->getName(), "Appearance") == 0)
    {
    if (this->CurrentProperty)
      {
      this->CurrentProperty->Delete();
      }
    this->CurrentProperty = vtkProperty::New();
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName,
                                                     this->CurrentProperty);
      creatingDEF = 0;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "Box") == 0)
    {
    pmap = vtkPolyDataMapper::New();
    vtkCubeSource *cube= vtkCubeSource::New();
    pmap->SetInputConnection(cube->GetOutputPort());
    this->CurrentActor->SetMapper(pmap);
    pmap->Delete();
    if (this->CurrentProperty)
      {
      this->CurrentActor->SetProperty(this->CurrentProperty);
      }
    if (this->CurrentSource)
      {
      this->CurrentSource->Delete();
      }
    this->CurrentSource = cube;
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, pmap);
      creatingDEF = 0;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "Cone") == 0)
    {
    pmap = vtkPolyDataMapper::New();
    vtkConeSource *cone= vtkConeSource::New();
    cone->SetResolution(12);
    pmap->SetInputConnection(cone->GetOutputPort());
    this->CurrentActor->SetMapper(pmap);
    pmap->Delete();
    if (this->CurrentProperty)
      {
      this->CurrentActor->SetProperty(this->CurrentProperty);
      }
    if (this->CurrentSource)
      {
      this->CurrentSource->Delete();
      }
    this->CurrentSource = cone;
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, pmap);
      creatingDEF = 0;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "Cylinder") == 0)
    {
    pmap = vtkPolyDataMapper::New();
    vtkCylinderSource *cyl= vtkCylinderSource::New();
    cyl->SetResolution(12);
    pmap->SetInputConnection(cyl->GetOutputPort());
    this->CurrentActor->SetMapper(pmap);
    pmap->Delete();
    if (this->CurrentProperty)
      {
      this->CurrentActor->SetProperty(this->CurrentProperty);
      }
    if (this->CurrentSource)
      {
      this->CurrentSource->Delete();
      }
    this->CurrentSource = cyl;
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, pmap);
      creatingDEF = 0;

      }
    }
  else if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
    {
    if (this->CurrentLight)
      {
      this->CurrentLight->Delete();
      }
    this->CurrentLight = vtkLight::New();
    this->Renderer->AddLight(this->CurrentLight);
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName,
                                                     this->CurrentLight);
      creatingDEF = 0;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "IndexedFaceSet") == 0 ||
           strcmp(fr->nodeType->getName(), "IndexedLineSet") == 0 ||
           strcmp(fr->nodeType->getName(), "PointSet") == 0)
    {
    pmap = vtkPolyDataMapper::New();
    pmap->SetScalarVisibility(0);
    this->CurrentActor->SetMapper(pmap);
    if (this->CurrentProperty)
      {
      this->CurrentActor->SetProperty(this->CurrentProperty);
      }
    if (this->CurrentMapper)
      {
      this->CurrentMapper->Delete();
      }
    this->CurrentMapper = pmap;
    if (this->CurrentScalars)
      {
      this->CurrentScalars->Delete();
      }
    this->CurrentScalars = vtkFloatArray::New();
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, pmap);
      creatingDEF = 0;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "Shape") == 0)
    {
    actor = vtkActor::New();
    if (this->CurrentProperty)
      {
      actor->SetProperty(this->CurrentProperty);
      }
    actor->SetOrientation(this->CurrentTransform->GetOrientation());
    actor->SetPosition(this->CurrentTransform->GetPosition());
    actor->SetScale(this->CurrentTransform->GetScale());
    if (this->CurrentActor)
      {
      this->CurrentActor->Delete();
      }
    this->CurrentActor = actor;
    // Add actor to renderer
    this->Renderer->AddActor(actor);
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, actor);
      creatingDEF= 0;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "Sphere") == 0)
    {
    pmap = vtkPolyDataMapper::New();
    vtkSphereSource *sphere = vtkSphereSource::New();
    pmap->SetInputConnection(sphere->GetOutputPort());
    if (this->CurrentSource)
      {
      this->CurrentSource->Delete();
      }
    this->CurrentSource = sphere;
    this->CurrentActor->SetMapper(pmap);
    pmap->Delete();
    if (this->CurrentProperty)
      {
      this->CurrentActor->SetProperty(this->CurrentProperty);
      }
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, pmap);
      creatingDEF= 0;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "Transform") == 0)
    {
    this->CurrentTransform->Push();
    }
}

void
vtkVRMLImporter::exitNode()
{
  VrmlNodeType::FieldRec *fr = VrmlNodeType::currentField->Top();
  assert(fr != NULL);
  VrmlNodeType::currentField->Pop();

  // Exiting this means we need to setup the color mode and
  // normals and other fun stuff.
  if (strcmp(fr->nodeType->getName(), "IndexedFaceSet") == 0 ||
      strcmp(fr->nodeType->getName(), "IndexedLineSet") == 0 ||
      strcmp(fr->nodeType->getName(), "PointSet") == 0)
    {
    // if tcoords exactly correspond with vertices (or there aren't any)
    // then can map straight through as usual
    // if not then must rejig using face-correspondence
    // (VRML supports per-face tcoords)
    // a similar scheme is implemented in vtkOBJReader

    int tcoords_correspond; // (boolean)
    if ( (this->CurrentTCoords==NULL || this->CurrentTCoordCells==NULL) && (this->CurrentNormals==NULL || this->CurrentNormalCells==NULL) )
        tcoords_correspond=1; // there aren't any, can proceed
    else if (this->CurrentTCoords && this->CurrentTCoords->GetNumberOfTuples()!=this->CurrentPoints->GetNumberOfPoints())
        tcoords_correspond=0; // false, must rejig
    else if (this->CurrentNormals && this->CurrentNormals->GetNumberOfTuples()!=this->CurrentPoints->GetNumberOfPoints())
        tcoords_correspond=0; // false, must rejig
    else
      {
      // the number of polygon faces and texture faces must be equal.
      // if they are not then something is wrong
      if (this->CurrentTCoordCells && this->CurrentTCoordCells->GetNumberOfCells() !=
          this->CurrentMapper->GetInput()->GetPolys()->GetNumberOfCells())
        {
        vtkErrorMacro(<<"Number of faces does not match texture faces, output may not be correct")
        tcoords_correspond=1; // don't rejig
        }
      else if (this->CurrentNormalCells && this->CurrentNormalCells->GetNumberOfCells() !=
          this->CurrentMapper->GetInput()->GetPolys()->GetNumberOfCells())
        {
        vtkErrorMacro(<<"Number of faces does not match normal faces, output may not be correct")
        tcoords_correspond=1; // don't rejig
        }
      else
        {
        // count of tcoords and points is the same, must run through indices to see if they
        // correspond by index point-for-point
        tcoords_correspond=1; // assume true until found otherwise
        if (this->CurrentTCoords && this->CurrentTCoordCells)
          {
          vtkIdType DUMMY_WARNING_PREVENTION_MECHANISM;
          vtkIdType n_pts=-1,*pts = &DUMMY_WARNING_PREVENTION_MECHANISM;
          vtkIdType n_tcoord_pts=-1,*tcoord_pts = &DUMMY_WARNING_PREVENTION_MECHANISM;
          this->CurrentMapper->GetInput()->GetPolys()->InitTraversal();
          this->CurrentTCoordCells->InitTraversal();
          int i,j;
          for (i=0;i<this->CurrentTCoordCells->GetNumberOfCells();i++)
            {
            this->CurrentMapper->GetInput()->GetPolys()->GetNextCell(n_pts,pts);
            this->CurrentTCoordCells->GetNextCell(n_tcoord_pts,tcoord_pts);
            if (n_pts!=n_tcoord_pts)
              {
              vtkErrorMacro(<<"Face size differs to texture face size, output may not be correct")
              break;
              }
            for (j=0;j<n_pts;j++)
              {
              if (pts[j]!=tcoord_pts[j])
                {
                tcoords_correspond=0; // have found an exception
                break;
                }
              }
            }
          }

        if (this->CurrentNormals && this->CurrentNormalCells)
          {
          vtkIdType DUMMY_WARNING_PREVENTION_MECHANISM;
          vtkIdType n_pts=-1,*pts = &DUMMY_WARNING_PREVENTION_MECHANISM;
          vtkIdType n_normal_pts=-1,*normal_pts = &DUMMY_WARNING_PREVENTION_MECHANISM;
          this->CurrentMapper->GetInput()->GetPolys()->InitTraversal();
          this->CurrentNormalCells->InitTraversal();
          int i,j;
          for (i=0;i<this->CurrentNormalCells->GetNumberOfCells();i++)
            {
            this->CurrentMapper->GetInput()->GetPolys()->GetNextCell(n_pts,pts);
            this->CurrentNormalCells->GetNextCell(n_normal_pts,normal_pts);
            if (n_pts!=n_normal_pts)
              {
              vtkErrorMacro(<<"Face size differs to normal face size, output may not be correct")
              break;
              }
            for (j=0;j<n_pts;j++)
              {
              if (pts[j]!=normal_pts[j])
                {
                tcoords_correspond=0; // have found an exception
                break;
                }
              }
            }
          }

        }
      }

    if (tcoords_correspond) // no rejigging necessary
      {
      ((vtkPolyData *)this->CurrentMapper->GetInput())->SetPoints(this->CurrentPoints);
      // We always create a scalar object in the enternode method.
      ((vtkPolyData *)this->CurrentMapper->GetInput())->GetPointData()->SetScalars(CurrentScalars);
      if (this->CurrentNormals)
        {
        ((vtkPolyData *)this->CurrentMapper->GetInput())->GetPointData()->SetNormals(CurrentNormals);
        this->CurrentNormals->Delete();
        this->CurrentNormals = NULL;
        }
      if (this->CurrentTCoords)
        {
        ((vtkPolyData *)this->CurrentMapper->GetInput())->GetPointData()->SetTCoords(CurrentTCoords);
        this->CurrentTCoords->Delete();
        this->CurrentTCoords = NULL;
        }
      }
    else  // must rejig
      {

      vtkDebugMacro(<<"Duplicating vertices so that tcoords and normals are correct");

      vtkPoints *new_points = vtkPoints::New();
      vtkFloatArray *new_scalars = vtkFloatArray::New();
      if (this->CurrentScalars)
        new_scalars->SetNumberOfComponents(this->CurrentScalars->GetNumberOfComponents());
      vtkFloatArray *new_tcoords = vtkFloatArray::New();
      new_tcoords->SetNumberOfComponents(2);
      vtkFloatArray *new_normals = vtkFloatArray::New();
      new_normals->SetNumberOfComponents(3);
      vtkCellArray *new_polys = vtkCellArray::New();

      // for each poly, copy its vertices into new_points (and point at them)
      // also copy its tcoords into new_tcoords
      // also copy its normals into new_normals
      // also copy its scalar into new_scalars
      this->CurrentMapper->GetInput()->GetPolys()->InitTraversal();
      if (this->CurrentTCoordCells)
        this->CurrentTCoordCells->InitTraversal();
      if (this->CurrentNormalCells)
        this->CurrentNormalCells->InitTraversal();
      int i,j;
      vtkIdType DUMMY_WARNING_PREVENTION_MECHANISM;
      vtkIdType n_pts=-1,*pts = &DUMMY_WARNING_PREVENTION_MECHANISM;
      vtkIdType n_tcoord_pts=-1,*tcoord_pts = &DUMMY_WARNING_PREVENTION_MECHANISM;
      vtkIdType n_normal_pts=-1,*normal_pts = &DUMMY_WARNING_PREVENTION_MECHANISM;
      for (i=0;i<this->CurrentMapper->GetInput()->GetPolys()->GetNumberOfCells();i++)
        {

        this->CurrentMapper->GetInput()->GetPolys()->GetNextCell(n_pts,pts);
        if (this->CurrentTCoordCells)
          this->CurrentTCoordCells->GetNextCell(n_tcoord_pts,tcoord_pts);
        if (this->CurrentNormalCells)
          this->CurrentNormalCells->GetNextCell(n_normal_pts,normal_pts);

        // If some vertices have tcoords and not others
        // then we must do something else VTK will complain. (crash on render attempt)
        // Easiest solution is to delete polys that don't have complete tcoords (if there
        // are any tcoords in the dataset)

        if (this->CurrentTCoords && n_pts!=n_tcoord_pts && this->CurrentTCoords->GetNumberOfTuples()>0)
          {
          // skip this poly
          vtkDebugMacro(<<"Skipping poly "<<i+1<<" (1-based index)");
          }
        else if (this->CurrentNormals && n_pts!=n_normal_pts && this->CurrentNormals->GetNumberOfTuples()>0)
          {
          // skip this poly
          vtkDebugMacro(<<"Skipping poly "<<i+1<<" (1-based index)");
          }
        else
          {
          // copy the corresponding points, tcoords and normals across
          for (j=0;j<n_pts;j++)
            {
            // copy the tcoord for this point across (if there is one)
            if (this->CurrentTCoords && n_tcoord_pts>0)
              new_tcoords->InsertNextTuple(this->CurrentTCoords->GetTuple(tcoord_pts[j]));
            // copy the normal for this point across (if any)
            if (this->CurrentNormals && n_normal_pts>0)
              new_normals->InsertNextTuple(this->CurrentNormals->GetTuple(normal_pts[j]));
            // copy the scalar for this point across
            if (this->CurrentScalars)
              new_scalars->InsertNextTuple(this->CurrentScalars->GetTuple(pts[j]));
            // copy the vertex into the new structure and update
            // the vertex index in the polys structure (pts is a pointer into it)
            pts[j] = new_points->InsertNextPoint(this->CurrentPoints->GetPoint(pts[j]));
            }
          // copy this poly (pointing at the new points) into the new polys list
          new_polys->InsertNextCell(n_pts,pts);
          }
        }

      // use the new structures for the output
      this->CurrentMapper->GetInput()->SetPoints(new_points);
      this->CurrentMapper->GetInput()->SetPolys(new_polys);
      if (this->CurrentTCoords)
        this->CurrentMapper->GetInput()->GetPointData()->SetTCoords(new_tcoords);
      if (this->CurrentNormals)
        this->CurrentMapper->GetInput()->GetPointData()->SetNormals(new_normals);
      if (this->CurrentScalars)
        this->CurrentMapper->GetInput()->GetPointData()->SetScalars(new_scalars);
      this->CurrentMapper->GetInput()->Squeeze();

      new_points->Delete();
      new_polys->Delete();
      new_tcoords->Delete();
      new_normals->Delete();
      new_scalars->Delete();
      }

    if (this->CurrentLut)
      {
      this->CurrentScalars->InsertNextValue(this->CurrentLut->GetNumberOfColors());
      this->CurrentMapper->SetLookupTable(CurrentLut);
      this->CurrentMapper->SetScalarVisibility(1);
      // Set for PerVertex Coloring.
      this->CurrentLut->SetTableRange(0.0,
                                      float(this->CurrentLut->GetNumberOfColors() - 1));
      this->CurrentLut->Delete();
      this->CurrentLut = NULL;
      }
    }
  else if (strcmp(fr->nodeType->getName(), "Shape") == 0)
    {
    if (this->CurrentProperty)
      this->CurrentActor->SetProperty(this->CurrentProperty);
    }
  // simply pop the current transform
  else if (strcmp(fr->nodeType->getName(), "Transform") == 0)
    {
    this->CurrentTransform->Pop();
    }

  delete fr;
}



void
vtkVRMLImporter::enterField(const char *fieldName)
{
  VrmlNodeType::FieldRec *fr = VrmlNodeType::currentField->Top();
  assert(fr != NULL);

  fr->fieldName = fieldName;
  if (fr->nodeType != NULL)
    {
    // enterField is called when parsing eventIn and eventOut IS
    // declarations, in which case we don't need to do anything special--
    // the IS IDENTIFIER will be returned from the lexer normally.
    if (fr->nodeType->hasEventIn(fieldName) ||
        fr->nodeType->hasEventOut(fieldName))
      return;

    int type = fr->nodeType->hasField(fieldName);
    if (type != 0)
      {
      // Let the lexer know what field type to expect:
      expect(type);
      }
    else
      {
      cerr << "Error: Node's of type " << fr->nodeType->getName() <<
        " do not have fields/eventIn/eventOut named " <<
        fieldName << "\n";
      // expect(ANY_FIELD);
      }
    }
  // else expect(ANY_FIELD);
}

void
vtkVRMLImporter::exitField()
{
  VrmlNodeType::FieldRec *fr = VrmlNodeType::currentField->Top();
  assert(fr != NULL);
  // For the radius field
  if (strcmp(fr->fieldName, "radius") == 0)
    {
    // Set the Sphere radius
    if (strcmp(fr->nodeType->getName(), "Sphere") == 0)
      {
      ((vtkSphereSource *)(this->CurrentSource))->SetRadius(yylval.sffloat);
      }
    // Set the Cylinder radius
    if (strcmp(fr->nodeType->getName(), "Cylinder") == 0)
      {
      ((vtkCylinderSource *)this->CurrentSource)->SetRadius(yylval.sffloat);
      }
    }
  // for the ambientIntensity field
  else if (strcmp(fr->fieldName, "ambientIntensity") == 0)
    {
    // Add to the current light
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
      {
      this->CurrentLight->SetIntensity(yylval.sffloat);
      }
    // or the current material
    else if (strcmp(fr->nodeType->getName(), "Material") == 0)
      {
      this->CurrentProperty->SetAmbient(yylval.sffloat);
      }
    }
  // For diffuseColor field, only in material node
  else if (strcmp(fr->fieldName, "diffuseColor") == 0)
    {
    this->CurrentProperty->SetDiffuseColor(
      yylval.vec3f->GetPoint(0)[0],yylval.vec3f->GetPoint(0)[1],
      yylval.vec3f->GetPoint(0)[2]);
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
    yylval.vec3f = NULL;
    }
  // For emissiveColor field, only in material node
  else if (strcmp(fr->fieldName, "emissiveColor") == 0)
    {
    this->CurrentProperty->SetAmbientColor(
      yylval.vec3f->GetPoint(0)[0],yylval.vec3f->GetPoint(0)[1],
      yylval.vec3f->GetPoint(0)[2]);
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);yylval.vec3f = NULL;
    }
  // For shininess field, only in material node
  else if (strcmp(fr->fieldName, "shininess") == 0)
    {
    this->CurrentProperty->SetSpecularPower(yylval.sffloat);
    }
  // For specularcolor field, only in material node
  else if (strcmp(fr->fieldName, "specularColor") == 0)
    {
    this->CurrentProperty->SetSpecularColor(
      yylval.vec3f->GetPoint(0)[0],yylval.vec3f->GetPoint(0)[1],
      yylval.vec3f->GetPoint(0)[2]);
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);yylval.vec3f = NULL;
    }
  // For transparency field, only in material node
  else if (strcmp(fr->fieldName, "transparency") == 0)
    {
    this->CurrentProperty->SetOpacity(1.0 - yylval.sffloat);
    }
  // For the translation field
  else if (strcmp(fr->fieldName, "translation") == 0)
    {
    // in the Transform node.
    if (strcmp(fr->nodeType->getName(), "Transform") == 0)
      {
      double *dtmp = yylval.vec3f->GetPoint(0);
      this->CurrentTransform->Translate(dtmp[0],dtmp[1],dtmp[2]);
      yylval.vec3f->Reset();
      this->DeleteObject(yylval.vec3f); yylval.vec3f = NULL;
      }
    }
  // For the scale field
  else if (strcmp(fr->fieldName, "scale") == 0)
    {
    // In the transform node
    if (strcmp(fr->nodeType->getName(), "Transform") == 0)
      {
      double *dtmp = yylval.vec3f->GetPoint(0);
      this->CurrentTransform->Scale(dtmp[0],dtmp[1],dtmp[2]);
      yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
      yylval.vec3f = NULL;
      }
    }
  // For the size field
  else if (strcmp(fr->fieldName, "size") == 0)
    {
    // set the current source (has to be a CubeSource)
    if (strcmp(fr->nodeType->getName(), "Box") == 0)
      {
      double *dtmp = yylval.vec3f->GetPoint(0);
      ((vtkCubeSource *)this->CurrentSource)->SetXLength(dtmp[0]);
      ((vtkCubeSource *)this->CurrentSource)->SetYLength(dtmp[1]);
      ((vtkCubeSource *)this->CurrentSource)->SetZLength(dtmp[2]);
      yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
      yylval.vec3f = NULL;
      }
    }
  // For the height field
  else if (strcmp(fr->fieldName, "height") == 0)
    {
    // Set the current Cone height
    if (strcmp(fr->nodeType->getName(), "Cone") == 0)
      {
      ((vtkConeSource *)this->CurrentSource)->SetHeight(yylval.sffloat);
      }
    // or set the current Cylinder height
    if (strcmp(fr->nodeType->getName(), "Cylinder") == 0)
      {
      ((vtkCylinderSource *)this->CurrentSource)->SetHeight(yylval.sffloat);
      }
    }
  // For the bottomRadius field (Only in the Cone object)
  else if (strcmp(fr->fieldName, "bottomRadius") == 0)
    {
    if (strcmp(fr->nodeType->getName(), "Cone") == 0)
      {
      ((vtkConeSource *)this->CurrentSource)->SetRadius(yylval.sffloat);
      }
    }
  //      else if (strcmp(fr->fieldName, "position") == 0) {
  //              yylval.vec3f->GetPoint(0, vals);
  //              vtkCamera *acam = vtkCamera::New();
  //              acam->SetPosition(vals);
  //              this->Renderer->SetActiveCamera(acam);
  //              yylval.vec3f->Delete();yylval.vec3f = NULL;
  //      }
  // Handle coordIndex for Indexed????Sets
  else if (strcmp(fr->fieldName, "coordIndex") == 0)
    {
    vtkCellArray *cells;
    int index, i, cnt;
    vtkPolyData *pd;

    pd = vtkPolyData::New();
    cells = vtkCellArray::New();
    index = i = cnt = 0;
    for (i = 0;i <= yylval.mfint32->GetMaxId();i++)
      {
      if (yylval.mfint32->GetValue(i) == -1)
        {
        cells->InsertNextCell(cnt,
                              (vtkIdType*)yylval.mfint32->GetPointer(index));
        index = i+1;
        cnt = 0;
        }
      else
        {
        cnt++;
        }
      }
    if (strcmp(fr->nodeType->getName(), "IndexedFaceSet") == 0)
      {
      pd->SetPolys(cells);
      }
    else
      {
      pd->SetLines(cells);
      }

    this->CurrentMapper->SetInputData(pd);
    pd->Delete();
    cells->Delete();
    yylval.mfint32->Reset();this->DeleteObject(yylval.mfint32);
    }
  // Handle point field
  else if (strcmp(fr->fieldName, "point") == 0)
    {
    // If for a coordinate node, simply used created FloatPoints
    if (strcmp(fr->nodeType->getName(), "Coordinate") == 0)
      {
      if (this->CurrentPoints)
        {
        this->CurrentPoints->Delete();
        }
      this->CurrentPoints = yylval.vec3f;
      // Seed the scalars with default values.
      this->CurrentScalars->Reset();
      for (int i=0;i < this->CurrentPoints->GetNumberOfPoints();i++)
        {
        this->CurrentScalars->InsertNextValue(i);
        }
      if (creatingDEF)
        {
        *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, this->CurrentPoints);
        creatingDEF = 0;
        }
      }
    else if (strcmp(fr->nodeType->getName(), "TextureCoordinate") == 0) // TJH
      {
      if(this->CurrentTCoords)
        {
        this->CurrentTCoords->Delete();
        }
      this->CurrentTCoords = yylval.vec2f;
      this->CurrentTCoords->Register(this);
      }
    }
  // Handle coord field, simply set the CurrentPoints
  else if (strcmp(fr->fieldName, "coord") == 0)
    {
    this->CurrentPoints = yylval.vec3f;
    this->CurrentPoints->Register(this);
    if (creatingDEF)
      {
      *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, this->CurrentPoints);
      creatingDEF = 0;
      }

    // There is no coordIndex for PointSet data, generate the PolyData here.
    if (strcmp(fr->nodeType->getName(), "PointSet") == 0)
      {
      vtkCellArray *cells;
      vtkIdType i;
      vtkPolyData *pd;

      pd = vtkPolyData::New();
      cells = vtkCellArray::New();
      for (i=0;i < yylval.vec3f->GetNumberOfPoints();i++)
        {
        cells->InsertNextCell(1, &i);
        }

      pd->SetVerts(cells);

      this->CurrentMapper->SetInputData(pd);
      pd->Delete();
      cells->Delete();
      }
    }
  // Handle color field
  else if (strcmp(fr->fieldName, "color") == 0)
    {
    // For the Light nodes
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
      {
      this->CurrentLight->SetColor(
        yylval.vec3f->GetPoint(0)[0],yylval.vec3f->GetPoint(0)[1],
        yylval.vec3f->GetPoint(0)[2]);
      yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
      yylval.vec3f = NULL;
      }
    // For the Color node, Insert colors into lookup table
    // These are associated with the points in the coord field
    // and also in the colorIndex field
    if (strcmp(fr->nodeType->getName(), "Color") == 0)
      {
      double vals4[4];
      vals4[3] = 1.0;
      vtkLookupTable *lut = vtkLookupTable::New();
      lut->SetNumberOfColors(yylval.vec3f->GetNumberOfPoints());
      lut->Build();
      for (int i=0;i < yylval.vec3f->GetNumberOfPoints();i++)
        {
        yylval.vec3f->GetPoint(i, vals4);
        lut->SetTableValue(i, vals4);
        }
      if (this->CurrentLut)
        {
        this->CurrentLut->Delete();
        }
      this->CurrentLut = lut;
      if (creatingDEF)
        {
        *VrmlNodeType::useList += new vtkVRMLUseStruct(curDEFName, this->CurrentLut);
        creatingDEF = 0;
        }
      }
    }
  // Handle colorIndex field, always for a Indexed????Set
  else if (strcmp(fr->fieldName, "colorIndex") == 0)
    {
    vtkCellArray *cells;
    int index, j;
    vtkIdType *pts=0;
    vtkIdType npts;
    vtkPolyData *pd = (vtkPolyData *)this->CurrentMapper->GetInput();
    if (pd->GetNumberOfPolys() > 0)
      cells = pd->GetPolys();
    else
      cells = pd->GetLines();
    cells->InitTraversal();
    index = 0;j = 0;

    // At this point we either have colors index by vertex or faces
    // If faces, num of color indexes must match num of faces else
    // we assume index by vertex.
    if ((yylval.mfint32->GetMaxId() + 1) == pd->GetNumberOfPolys())
      {
      for (int i=0;i <= yylval.mfint32->GetMaxId();i++)
        {
        if (yylval.mfint32->GetValue(i) >= 0)
          {
          cells->GetNextCell(npts, pts);
		  for (j = 0; j < npts; j++)
            {
	          this->CurrentScalars->SetComponent(pts[j], 0, yylval.mfint32->GetValue(i));
            }
          }
        }
      }
    // else handle colorindex by vertex
	else
      {
      cells->GetNextCell(npts, pts);
      for (int i=0;i <= yylval.mfint32->GetMaxId();i++)
        {
        if (yylval.mfint32->GetValue(index) == -1)
          {
          cells->GetNextCell(npts, pts);
          // Pass by the -1
          index++;
          j = 0;
          }
        else
          {
          // Redirect color into scalar position
          this->CurrentScalars->SetComponent(pts[j++], 0,
                                           yylval.mfint32->GetValue(index++));
          }
        }
      }
    }
  // Handle direction field
  else if (strcmp(fr->fieldName, "direction") == 0)
    {
    // For Directional light.
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0) {
    this->CurrentLight->SetFocalPoint(yylval.vec3f->GetPoint(0));
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);yylval.vec3f = NULL;
    }
    // For
    }
  // Handle intensity field
  else if (strcmp(fr->fieldName, "intensity") == 0)
    {
    // For Directional light.
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
      {
      this->CurrentLight->SetIntensity(yylval.sffloat);
      }
    // For
    }
  // Handle on field
  else if (strcmp(fr->fieldName, "on") == 0)
    {
    // For Directional light.
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
      {
      this->CurrentLight->SetSwitch(yylval.sfint);
      }
    // For
    }
  // Handle colorPerVertex field
  else if (strcmp(fr->fieldName, "colorPerVertex") == 0)
    {
    // Same for all geometry nodes.
    this->CurrentMapper->SetScalarVisibility(yylval.sfint);
    }
  // Handle vector field for Normal Node
  else if (strcmp(fr->fieldName, "vector") == 0)
    {
    // For all floats in the vec3f, copy to the normal structure.
    if (this->CurrentNormals)
      {
      this->CurrentNormals->Delete();
      }
    this->CurrentNormals = vtkFloatArray::New();
    this->CurrentNormals->SetNumberOfComponents(3);
    this->CurrentNormals->SetNumberOfTuples(yylval.vec3f->GetNumberOfPoints());
    for (int i=0;i < yylval.vec3f->GetNumberOfPoints();i++)
      {
      this->CurrentNormals->InsertTuple(i, yylval.vec3f->GetPoint(i));
      }
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
    }
  else if (strcmp(fr->fieldName, "location") == 0)
    {
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
    }
  else if (strcmp(fr->fieldName, "position") == 0)
    {
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
    }
  else if (strcmp(fr->fieldName, "center") == 0)
    {
    yylval.vec3f->Reset();this->DeleteObject(yylval.vec3f);
    }
  else if (strcmp(fr->fieldName, "texCoordIndex") == 0)
    {
    if (this->CurrentTCoordCells) {
      this->CurrentTCoordCells->Delete();
    }
    this->CurrentTCoordCells = vtkCellArray::New();

    // read the indices of the tcoords and assign accordingly
    int index, i, cnt;
    index = i = cnt = 0;
    for (i = 0;i <= yylval.mfint32->GetMaxId();i++)
      {
      if (yylval.mfint32->GetValue(i) == -1)
        {
        this->CurrentTCoordCells->InsertNextCell(cnt,
                              (vtkIdType*)yylval.mfint32->GetPointer(index));
        index = i+1;
        cnt = 0;
        }
      else
        {
        cnt++;
        }
      }
    yylval.mfint32->Reset();this->DeleteObject(yylval.mfint32);
    }
  else if (strcmp(fr->fieldName, "normalIndex") == 0)
    {
    if (this->CurrentNormalCells) {
      this->CurrentNormalCells->Delete();
    }
    this->CurrentNormalCells = vtkCellArray::New();

    // read the indices of the normals and assign accordingly
    int index, i, cnt;
    index = i = cnt = 0;
    for (i = 0;i <= yylval.mfint32->GetMaxId();i++)
      {
      if (yylval.mfint32->GetValue(i) == -1)
        {
        this->CurrentNormalCells->InsertNextCell(cnt,
                              (vtkIdType*)yylval.mfint32->GetPointer(index));
        index = i+1;
        cnt = 0;
        }
      else
        {
        cnt++;
        }
      }
    yylval.mfint32->Reset();this->DeleteObject(yylval.mfint32);
    }
  else
    {
    }
  fr->fieldName = NULL;
}

void
vtkVRMLImporter::useNode(const char *name) {

  vtkObject *useO;
  if ((useO = this->GetVRMLDEFObject(name)))
    {
    if (strstr(useO->GetClassName(), "Actor"))
      {
      vtkActor *_act = vtkActor::New();
      _act->ShallowCopy((vtkActor *)useO);
      if (this->CurrentProperty)
        _act->SetProperty(this->CurrentProperty);
      _act->SetOrientation(this->CurrentTransform->GetOrientation());
      _act->SetPosition(this->CurrentTransform->GetPosition());
      _act->SetScale(this->CurrentTransform->GetScale());
      if (this->CurrentActor)
        {
        this->CurrentActor->Delete();
        }
      this->CurrentActor = _act;
      this->Renderer->AddActor(_act);
      }
    else if (strstr(useO->GetClassName(), "PolyDataMapper"))
      {
      vtkActor *_act = vtkActor::New();
      _act->SetMapper((vtkPolyDataMapper *)useO);
      if (this->CurrentProperty)
        {
        _act->SetProperty(this->CurrentProperty);
        }
      _act->SetOrientation(this->CurrentTransform->GetOrientation());
      _act->SetPosition(this->CurrentTransform->GetPosition());
      _act->SetScale(this->CurrentTransform->GetScale());
      if (this->CurrentActor)
        {
        this->CurrentActor->UnRegister(this);
        }
      this->CurrentActor = _act;
      this->Renderer->AddActor(_act);
      }
    else if (strcmp(useO->GetClassName(), "vtkPoints") == 0)
      {
      yylval.vec3f = (vtkPoints *) useO;
      if (this->CurrentPoints)
        {
        this->CurrentPoints->Delete();
        }
      this->CurrentPoints = (vtkPoints *) useO;
      }
    else if (strcmp(useO->GetClassName(), "vtkLookupTable") == 0)
      {
      if (this->CurrentLut)
        {
        this->CurrentLut->Delete();
        }
      this->CurrentLut = (vtkLookupTable *) useO;
      // Seed the scalars with default values.
      this->CurrentScalars->Reset();
      for (int i=0;i < this->CurrentPoints->GetNumberOfPoints();i++)
        {
        this->CurrentScalars->InsertNextValue(i);
        }
      }
    }
}


// Send in the name from the VRML file, get the VTK object.
vtkObject *
vtkVRMLImporter::GetVRMLDEFObject(const char *name)
{
  // Look through the type stack:
  // Need to go from top of stack since last DEF created is most current
  for (int i = VrmlNodeType::useList->Count()-1;i >=0 ; i--)
    {
    const vtkVRMLUseStruct *nt = (*VrmlNodeType::useList)[i];
    if (nt != NULL && strcmp(nt->defName,name) == 0)
      {
      return nt->defObject;
      }
    }
  return NULL;
}


// Used by the lex input to get characters. Needed to read in memory structure

static void memyyInput(char *buf, int &result, int max_size) {

  result = static_cast<int>(
    strlen(strncpy(buf, standardNodes[memyyInput_i], max_size)));
  memyyInput_j = result - static_cast<int>(
    strlen(standardNodes[memyyInput_i]));
  if ( memyyInput_j == 0 )
    {
    memyyInput_i++;
    }
}

// Needed to reset the lex input routine to default.
static void defyyInput(char *buf, int &result, int max_size) {
  if ( yy_current_buffer->yy_is_interactive )
    {
    int c = getc( yyin );
    result = c == EOF ? 0 : 1;
    buf[0] = (char) c;
    }
  else if( ((result = static_cast<int>(fread( buf, 1, max_size, yyin ))) == 0)
            && ferror( yyin ) )
    {
    YY_FATAL_ERROR( "input in flex scanner failed" );
    }
}

