#ifndef __VTK_SYSTEM_INCLUDES__INSIDE
#  define __VTK_SYSTEM_INCLUDES__INSIDE
#  include "vtkWin32Header.h"
#  undef __VTK_SYSTEM_INCLUDES__INSIDE
#endif

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

#include "vtkVRMLImporter_Yacc.h"

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

#include <cassert>

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
  this->Parser = new vtkVRMLYaccData;
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

  this->Parser->memyyInput_i = 0;
  this->Parser->memyyInput_j = 0;

  vtkVRMLAllocator::Initialize();
  this->Parser->typeList = new vtkVRMLVectorType<VrmlNodeType*>;
  this->Parser->typeList->Init();

  this->Parser->useList = new vtkVRMLVectorType<vtkVRMLUseStruct *>;
  this->Parser->useList->Init();

  this->Parser->currentField = new vtkVRMLVectorType<VrmlNodeType::FieldRec *>;
  this->Parser->currentField->Init();

  if (!this->OpenImportFile())
    {
    return 0;
    }

  // This is acrually where it all takes place, Since VRML is a SG
  // And is state based, I need to create actors, cameras, and lights
  // as I go. The ImportXXXXXX rotuines are not used.
  this->Parser->CurrentProtoStack = new vtkVRMLVectorType<VrmlNodeType*>;

  // Lets redefine the YY_INPUT macro on Flex and get chars from memory
  this->Parser->theyyInput = vtkVRMLYaccData::memyyInput;
  // Crank up the yacc parser...
  this->Parser->yydebug = 0;
  this->Parser->yy_flex_debug = 0;
  /*FILE *standardNodes = fopen("standardNodes.wrl", "r");
    if (standardNodes == NULL) {
    cerr << "Error, couldn't open standardNodes.wrl file";
    return 0;
    }
    yyin = standardNodes;*/
  this->Parser->yyparse(this);
  this->Parser->yyin = NULL;
  this->Parser->yyResetLineNumber();
  //fclose(standardNodes);

  // Not sure why I have to do this but its not working when
  // When I use the FileFD file pointer...
  // File existence already checked.
  this->Parser->yyin = fopen(this->FileName, "r");

  // reset the lex input routine
  this->Parser->theyyInput = vtkVRMLYaccData::defyyInput;

  // For this little test application, pushing and popping the node
  // namespace isn't really necessary.  But each VRML .wrl file is
  // a separate namespace for PROTOs (except for things predefined
  // in the spec), and pushing/popping the namespace when reading each
  // file is a good habit to get into:
  this->Parser->pushNameSpace();
  this->Parser->yyparse(this);
  this->Parser->popNameSpace();

  fclose(this->Parser->yyin);
  this->Parser->yyin = NULL;

  delete this->Parser->CurrentProtoStack;

  // In case there was a ViewPoint introduced it usually happens prior
  // to any actors being placed in the scene, need to reset the camera
  //this->Renderer->UpdateActors();
  return 1;
}

void vtkVRMLImporter::ImportEnd ()
{
  delete this->Parser->typeList;
  this->Parser->typeList = NULL;

  delete this->Parser->currentField;
  this->Parser->currentField = NULL;

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
  delete this->Parser->useList;
  this->Parser->useList = NULL;
  vtkVRMLAllocator::CleanUp();

  delete this->Parser;
}

void vtkVRMLImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << "Defined names in File:" << endl;
  if (this->Parser->useList)
    {
      for (int i = 0;i < this->Parser->useList->Count();i++)
        {
        os << "\tName: " << (*this->Parser->useList)[i]->defName
           << " is a " << (*this->Parser->useList)[i]->defObject->GetClassName()
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

  const VrmlNodeType *t = this->Parser->find(nodeType);
  if (t == NULL)
    {
    char tmp[1000];
    sprintf(tmp, "Unknown node type '%s'", nodeType);
    this->Parser->yyerror(tmp);
    exit(99);
    }
  VrmlNodeType::FieldRec *fr = new VrmlNodeType::FieldRec;
  fr->nodeType = t;
  fr->fieldName = NULL;
  *this->Parser->currentField += fr;
  if (strcmp(fr->nodeType->getName(), "Appearance") == 0)
    {
    if (this->CurrentProperty)
      {
      this->CurrentProperty->Delete();
      }
    this->CurrentProperty = vtkProperty::New();
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName,
                                                     this->CurrentProperty);
      this->Parser->creatingDEF = 0;
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
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, pmap);
      this->Parser->creatingDEF = 0;
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
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, pmap);
      this->Parser->creatingDEF = 0;
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
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, pmap);
      this->Parser->creatingDEF = 0;

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
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName,
                                                     this->CurrentLight);
      this->Parser->creatingDEF = 0;
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
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, pmap);
      this->Parser->creatingDEF = 0;
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
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, actor);
      this->Parser->creatingDEF= 0;
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
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, pmap);
      this->Parser->creatingDEF= 0;
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
  VrmlNodeType::FieldRec *fr = this->Parser->currentField->Top();
  assert(fr != NULL);
  this->Parser->currentField->Pop();

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
  VrmlNodeType::FieldRec *fr = this->Parser->currentField->Top();
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
      this->Parser->expect(type);
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
  VrmlNodeType::FieldRec *fr = this->Parser->currentField->Top();
  assert(fr != NULL);
  // For the radius field
  if (strcmp(fr->fieldName, "radius") == 0)
    {
    // Set the Sphere radius
    if (strcmp(fr->nodeType->getName(), "Sphere") == 0)
      {
      ((vtkSphereSource *)(this->CurrentSource))->SetRadius(this->Parser->yylval.sffloat);
      }
    // Set the Cylinder radius
    if (strcmp(fr->nodeType->getName(), "Cylinder") == 0)
      {
      ((vtkCylinderSource *)this->CurrentSource)->SetRadius(this->Parser->yylval.sffloat);
      }
    }
  // for the ambientIntensity field
  else if (strcmp(fr->fieldName, "ambientIntensity") == 0)
    {
    // Add to the current light
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
      {
      this->CurrentLight->SetIntensity(this->Parser->yylval.sffloat);
      }
    // or the current material
    else if (strcmp(fr->nodeType->getName(), "Material") == 0)
      {
      this->CurrentProperty->SetAmbient(this->Parser->yylval.sffloat);
      }
    }
  // For diffuseColor field, only in material node
  else if (strcmp(fr->fieldName, "diffuseColor") == 0)
    {
    this->CurrentProperty->SetDiffuseColor(
      this->Parser->yylval.vec3f->GetPoint(0)[0],
      this->Parser->yylval.vec3f->GetPoint(0)[1],
      this->Parser->yylval.vec3f->GetPoint(0)[2]);
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
    }
  // For emissiveColor field, only in material node
  else if (strcmp(fr->fieldName, "emissiveColor") == 0)
    {
    this->CurrentProperty->SetAmbientColor(
      this->Parser->yylval.vec3f->GetPoint(0)[0],
      this->Parser->yylval.vec3f->GetPoint(0)[1],
      this->Parser->yylval.vec3f->GetPoint(0)[2]);
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
    }
  // For shininess field, only in material node
  else if (strcmp(fr->fieldName, "shininess") == 0)
    {
    this->CurrentProperty->SetSpecularPower(this->Parser->yylval.sffloat);
    }
  // For specularcolor field, only in material node
  else if (strcmp(fr->fieldName, "specularColor") == 0)
    {
    this->CurrentProperty->SetSpecularColor(
      this->Parser->yylval.vec3f->GetPoint(0)[0],
      this->Parser->yylval.vec3f->GetPoint(0)[1],
      this->Parser->yylval.vec3f->GetPoint(0)[2]);
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
    }
  // For transparency field, only in material node
  else if (strcmp(fr->fieldName, "transparency") == 0)
    {
    this->CurrentProperty->SetOpacity(1.0 - this->Parser->yylval.sffloat);
    }
  // For the translation field
  else if (strcmp(fr->fieldName, "translation") == 0)
    {
    // in the Transform node.
    if (strcmp(fr->nodeType->getName(), "Transform") == 0)
      {
      double *dtmp = this->Parser->yylval.vec3f->GetPoint(0);
      this->CurrentTransform->Translate(dtmp[0],dtmp[1],dtmp[2]);
      this->Parser->yylval.vec3f->Reset();
      this->DeleteObject(this->Parser->yylval.vec3f);
      this->Parser->yylval.vec3f = NULL;
      }
    }
  // For the scale field
  else if (strcmp(fr->fieldName, "scale") == 0)
    {
    // In the transform node
    if (strcmp(fr->nodeType->getName(), "Transform") == 0)
      {
      double *dtmp = this->Parser->yylval.vec3f->GetPoint(0);
      this->CurrentTransform->Scale(dtmp[0],dtmp[1],dtmp[2]);
      this->Parser->yylval.vec3f->Reset();
      this->DeleteObject(this->Parser->yylval.vec3f);
      this->Parser->yylval.vec3f = NULL;
      }
    }
  // For the size field
  else if (strcmp(fr->fieldName, "size") == 0)
    {
    // set the current source (has to be a CubeSource)
    if (strcmp(fr->nodeType->getName(), "Box") == 0)
      {
      double *dtmp = this->Parser->yylval.vec3f->GetPoint(0);
      ((vtkCubeSource *)this->CurrentSource)->SetXLength(dtmp[0]);
      ((vtkCubeSource *)this->CurrentSource)->SetYLength(dtmp[1]);
      ((vtkCubeSource *)this->CurrentSource)->SetZLength(dtmp[2]);
      this->Parser->yylval.vec3f->Reset();
      this->DeleteObject(this->Parser->yylval.vec3f);
      this->Parser->yylval.vec3f = NULL;
      }
    }
  // For the height field
  else if (strcmp(fr->fieldName, "height") == 0)
    {
    // Set the current Cone height
    if (strcmp(fr->nodeType->getName(), "Cone") == 0)
      {
      ((vtkConeSource *)this->CurrentSource)->SetHeight(this->Parser->yylval.sffloat);
      }
    // or set the current Cylinder height
    if (strcmp(fr->nodeType->getName(), "Cylinder") == 0)
      {
      ((vtkCylinderSource *)this->CurrentSource)->SetHeight(this->Parser->yylval.sffloat);
      }
    }
  // For the bottomRadius field (Only in the Cone object)
  else if (strcmp(fr->fieldName, "bottomRadius") == 0)
    {
    if (strcmp(fr->nodeType->getName(), "Cone") == 0)
      {
      ((vtkConeSource *)this->CurrentSource)->SetRadius(this->Parser->yylval.sffloat);
      }
    }
  //      else if (strcmp(fr->fieldName, "position") == 0) {
  //              this->Parser->yylval.vec3f->GetPoint(0, vals);
  //              vtkCamera *acam = vtkCamera::New();
  //              acam->SetPosition(vals);
  //              this->Renderer->SetActiveCamera(acam);
  //              this->Parser->yylval.vec3f->Delete();yylval.vec3f = NULL;
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
    for (i = 0;i <= this->Parser->yylval.mfint32->GetMaxId();i++)
      {
      if (this->Parser->yylval.mfint32->GetValue(i) == -1)
        {
        cells->InsertNextCell(cnt,
                              (vtkIdType*)this->Parser->yylval.mfint32->GetPointer(index));
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
    this->Parser->yylval.mfint32->Reset();
    this->DeleteObject(this->Parser->yylval.mfint32);
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
      this->CurrentPoints = this->Parser->yylval.vec3f;
      // Seed the scalars with default values.
      this->CurrentScalars->Reset();
      for (int i=0;i < this->CurrentPoints->GetNumberOfPoints();i++)
        {
        this->CurrentScalars->InsertNextValue(i);
        }
      if (this->Parser->creatingDEF)
        {
        *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentPoints);
        this->Parser->creatingDEF = 0;
        }
      }
    else if (strcmp(fr->nodeType->getName(), "TextureCoordinate") == 0) // TJH
      {
      if(this->CurrentTCoords)
        {
        this->CurrentTCoords->Delete();
        }
      this->CurrentTCoords = this->Parser->yylval.vec2f;
      this->CurrentTCoords->Register(this);
      }
    }
  // Handle coord field, simply set the CurrentPoints
  else if (strcmp(fr->fieldName, "coord") == 0)
    {
    this->CurrentPoints = this->Parser->yylval.vec3f;
    this->CurrentPoints->Register(this);
    if (this->Parser->creatingDEF)
      {
      *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentPoints);
      this->Parser->creatingDEF = 0;
      }

    // There is no coordIndex for PointSet data, generate the PolyData here.
    if (strcmp(fr->nodeType->getName(), "PointSet") == 0)
      {
      vtkCellArray *cells;
      vtkIdType i;
      vtkPolyData *pd;

      pd = vtkPolyData::New();
      cells = vtkCellArray::New();
      for (i=0;i < this->Parser->yylval.vec3f->GetNumberOfPoints();i++)
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
        this->Parser->yylval.vec3f->GetPoint(0)[0],
        this->Parser->yylval.vec3f->GetPoint(0)[1],
        this->Parser->yylval.vec3f->GetPoint(0)[2]);
      this->Parser->yylval.vec3f->Reset();
      this->DeleteObject(this->Parser->yylval.vec3f);
      this->Parser->yylval.vec3f = NULL;
      }
    // For the Color node, Insert colors into lookup table
    // These are associated with the points in the coord field
    // and also in the colorIndex field
    if (strcmp(fr->nodeType->getName(), "Color") == 0)
      {
      double vals4[4];
      vals4[3] = 1.0;
      vtkLookupTable *lut = vtkLookupTable::New();
      lut->SetNumberOfColors(this->Parser->yylval.vec3f->GetNumberOfPoints());
      lut->Build();
      for (int i=0;i < this->Parser->yylval.vec3f->GetNumberOfPoints();i++)
        {
        this->Parser->yylval.vec3f->GetPoint(i, vals4);
        lut->SetTableValue(i, vals4);
        }
      if (this->CurrentLut)
        {
        this->CurrentLut->Delete();
        }
      this->CurrentLut = lut;
      if (this->Parser->creatingDEF)
        {
        *this->Parser->useList += new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentLut);
        this->Parser->creatingDEF = 0;
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
    if ((this->Parser->yylval.mfint32->GetMaxId() + 1) == pd->GetNumberOfPolys())
      {
      for (int i=0;i <= this->Parser->yylval.mfint32->GetMaxId();i++)
        {
        if (this->Parser->yylval.mfint32->GetValue(i) >= 0)
          {
          cells->GetNextCell(npts, pts);
		  for (j = 0; j < npts; j++)
            {
            this->CurrentScalars->SetComponent(pts[j], 0, this->Parser->yylval.mfint32->GetValue(i));
            }
          }
        }
      }
    // else handle colorindex by vertex
	else
      {
      cells->GetNextCell(npts, pts);
      for (int i=0;i <= this->Parser->yylval.mfint32->GetMaxId();i++)
        {
        if (this->Parser->yylval.mfint32->GetValue(index) == -1)
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
                                           this->Parser->yylval.mfint32->GetValue(index++));
          }
        }
      }
    }
  // Handle direction field
  else if (strcmp(fr->fieldName, "direction") == 0)
    {
    // For Directional light.
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0) {
    this->CurrentLight->SetFocalPoint(this->Parser->yylval.vec3f->GetPoint(0));
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
    }
    // For
    }
  // Handle intensity field
  else if (strcmp(fr->fieldName, "intensity") == 0)
    {
    // For Directional light.
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
      {
      this->CurrentLight->SetIntensity(this->Parser->yylval.sffloat);
      }
    // For
    }
  // Handle on field
  else if (strcmp(fr->fieldName, "on") == 0)
    {
    // For Directional light.
    if (strcmp(fr->nodeType->getName(), "DirectionalLight") == 0)
      {
      this->CurrentLight->SetSwitch(this->Parser->yylval.sfint);
      }
    // For
    }
  // Handle colorPerVertex field
  else if (strcmp(fr->fieldName, "colorPerVertex") == 0)
    {
    // Same for all geometry nodes.
    this->CurrentMapper->SetScalarVisibility(this->Parser->yylval.sfint);
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
    this->CurrentNormals->SetNumberOfTuples(this->Parser->yylval.vec3f->GetNumberOfPoints());
    for (int i=0;i < this->Parser->yylval.vec3f->GetNumberOfPoints();i++)
      {
      this->CurrentNormals->InsertTuple(i, this->Parser->yylval.vec3f->GetPoint(i));
      }
    this->Parser->yylval.vec3f->Reset();this->DeleteObject(this->Parser->yylval.vec3f);
    }
  else if (strcmp(fr->fieldName, "location") == 0)
    {
    this->Parser->yylval.vec3f->Reset();this->DeleteObject(this->Parser->yylval.vec3f);
    }
  else if (strcmp(fr->fieldName, "position") == 0)
    {
    this->Parser->yylval.vec3f->Reset();this->DeleteObject(this->Parser->yylval.vec3f);
    }
  else if (strcmp(fr->fieldName, "center") == 0)
    {
    this->Parser->yylval.vec3f->Reset();this->DeleteObject(this->Parser->yylval.vec3f);
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
    for (i = 0;i <= this->Parser->yylval.mfint32->GetMaxId();i++)
      {
      if (this->Parser->yylval.mfint32->GetValue(i) == -1)
        {
        this->CurrentTCoordCells->InsertNextCell(cnt,
                              (vtkIdType*)this->Parser->yylval.mfint32->GetPointer(index));
        index = i+1;
        cnt = 0;
        }
      else
        {
        cnt++;
        }
      }
    this->Parser->yylval.mfint32->Reset();this->DeleteObject(this->Parser->yylval.mfint32);
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
    for (i = 0;i <= this->Parser->yylval.mfint32->GetMaxId();i++)
      {
      if (this->Parser->yylval.mfint32->GetValue(i) == -1)
        {
        this->CurrentNormalCells->InsertNextCell(cnt,
                              (vtkIdType*)this->Parser->yylval.mfint32->GetPointer(index));
        index = i+1;
        cnt = 0;
        }
      else
        {
        cnt++;
        }
      }
    this->Parser->yylval.mfint32->Reset();this->DeleteObject(this->Parser->yylval.mfint32);
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
      this->Parser->yylval.vec3f = (vtkPoints *) useO;
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
  for (int i = this->Parser->useList->Count()-1;i >=0 ; i--)
    {
    const vtkVRMLUseStruct *nt = (*this->Parser->useList)[i];
    if (nt != NULL && strcmp(nt->defName,name) == 0)
      {
      return nt->defObject;
      }
    }
  return NULL;
}

