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
#include "vtkCellArray.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLight.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkSystemIncludes.h"
#include "vtkTransform.h"
#include "vtkVRML.h"

#include <cassert>
#include <exception>
#include <sstream>

#include "vtkVRMLImporter_Yacc.h"

//----------------------------------------------------------------------------
class vtkVRMLImporterInternal
{
public:
  vtkVRMLImporterInternal() : Heap(1) {}
  vtkVRMLVectorType<vtkObject*> Heap;
};

//----------------------------------------------------------------------------
// Heap to manage memory leaks
vtkHeap *vtkVRMLAllocator::Heap = NULL;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRMLImporter);

//----------------------------------------------------------------------------
vtkVRMLImporter::vtkVRMLImporter()
{
  this->Internal = new vtkVRMLImporterInternal;
  this->CurrentActor = NULL;
  this->CurrentLight = NULL;
  this->CurrentProperty = NULL;
  this->CurrentSource = NULL;
  this->CurrentPoints = NULL;
  this->CurrentScalars = NULL;
  this->CurrentNormals = NULL;
  this->CurrentNormalCells = NULL;
  this->CurrentTCoords = NULL;
  this->CurrentTCoordCells = NULL;
  this->CurrentMapper = NULL;
  this->CurrentLut = NULL;
  this->CurrentTransform = NULL;
  this->FileName = NULL;
  this->FileFD = NULL;
  this->Parser = new vtkVRMLYaccData;
  this->ShapeResolution = 12;
}

//----------------------------------------------------------------------------
vtkVRMLImporter::~vtkVRMLImporter()
{
  if (this->CurrentTransform)
  {
    this->CurrentTransform->Delete();
    this->CurrentTransform = NULL;
  }

  delete [] this->FileName;
  this->FileName = NULL;

  while (this->Internal->Heap.Count() > 0)
  {
    vtkObject* obj = this->Internal->Heap.Pop();
    if (obj)
    {
      obj->Delete();
    }
  }
  delete this->Internal;
  this->Internal = NULL;

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
  this->Parser = NULL;
}

//----------------------------------------------------------------------------
void vtkVRMLImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
    << (this->FileName ? this->FileName : "(none)") << "\n";

  os << "Defined names in File:" << endl;
  if (this->Parser->useList)
  {
    for (int i = 0; i < this->Parser->useList->Count(); i++)
    {
      os << "\tName: " << (*this->Parser->useList)[i]->defName
        << " is a " << (*this->Parser->useList)[i]->defObject->GetClassName()
        << endl;
    }
  }
}

//----------------------------------------------------------------------------
// Open an import file. Returns zero if error.
int vtkVRMLImporter::OpenImportFile()
{
  vtkDebugMacro(<< "Opening import file");

  if (!this->FileName)
  {
    vtkErrorMacro(<< "No file specified!");
    return 0;
  }
  this->FileFD = fopen(this->FileName, "r");
  if (this->FileFD == NULL)
  {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkPoints* vtkVRMLImporter::PointsNew()
{
  vtkPoints* pts = vtkPoints::New();
  this->Internal->Heap.Push(pts);
  return pts;
}

//----------------------------------------------------------------------------
vtkFloatArray* vtkVRMLImporter::FloatArrayNew()
{
  vtkFloatArray* array = vtkFloatArray::New();
  this->Internal->Heap.Push(array);
  return array;
}

//----------------------------------------------------------------------------
vtkIdTypeArray* vtkVRMLImporter::IdTypeArrayNew()
{
  vtkIdTypeArray* array = vtkIdTypeArray::New();
  this->Internal->Heap.Push(array);
  return array;
}

//----------------------------------------------------------------------------
void vtkVRMLImporter::DeleteObject(vtkObject* obj)
{
  for (int i = 0; i < this->Internal->Heap.Count(); i++)
  {
    if (obj == this->Internal->Heap[i])
    {
      this->Internal->Heap[i] = 0;
    }
  }
  obj->Delete();
}

//----------------------------------------------------------------------------
int vtkVRMLImporter::ImportBegin()
{
  int ret = 1;
  try
  {
    if (this->CurrentTransform)
    {
      this->CurrentTransform->Delete();
    }
    this->CurrentTransform = vtkTransform::New();

    this->Parser->CurrentProtoStack = 0;
    this->Parser->memyyInput_i = 0;
    this->Parser->memyyInput_j = 0;

    vtkVRMLAllocator::Initialize();
    this->Parser->typeList = new vtkVRMLVectorType<VrmlNodeType*>;
    this->Parser->typeList->Init();

    this->Parser->useList = new vtkVRMLVectorType<vtkVRMLUseStruct*>;
    this->Parser->useList->Init();

    this->Parser->currentField = new vtkVRMLVectorType<VrmlNodeType::FieldRec*>;
    this->Parser->currentField->Init();

    if (!this->OpenImportFile())
    {
      throw std::exception();
    }

    // This is actually where it all takes place, Since VRML is a SG
    // And is state based, I need to create actors, cameras, and lights
    // as I go. The Import* routines are not used.
    this->Parser->CurrentProtoStack = new vtkVRMLVectorType<VrmlNodeType*>;

    // Lets redefine the YY_INPUT macro on Flex and get chars from memory
    this->Parser->theyyInput = vtkVRMLYaccData::memyyInput;
    // Crank up the yacc parser...
    this->Parser->yydebug = 0;
    this->Parser->yy_flex_debug = 0;
    this->Parser->yyparse(this);
    this->Parser->yyin = NULL;
    this->Parser->yyResetLineNumber();

    // Not sure why I have to do this but its not working when
    // When I use the FileFD file pointer...
    // File existence already checked.
    this->Parser->yyin = fopen(this->FileName, "r");
    if (!this->Parser->yyin)
    {
      throw std::exception();
    }

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
  }
  catch (const std::exception&)
  {
    vtkErrorMacro(<< "Unable to read VRML file! Error at line " <<
      this->Parser->currentLineNumber);
    ret = 0;
  }
  catch (std::string &s)
  {
    vtkErrorMacro(<< "Unable to read VRML file! Error at line " <<
      this->Parser->currentLineNumber <<":" << s);
    ret = 0;
  }

  if (this->Parser->yyin)
  {
    fclose(this->Parser->yyin);
    this->Parser->yyin = NULL;
  }

  delete this->Parser->CurrentProtoStack;
  this->Parser->CurrentProtoStack = NULL;

  return ret;
}

//----------------------------------------------------------------------------
void vtkVRMLImporter::ImportEnd()
{
  delete this->Parser->typeList;
  this->Parser->typeList = NULL;

  delete this->Parser->currentField;
  this->Parser->currentField = NULL;

  vtkDebugMacro(<<"Closing import file");
  if (this->FileFD != NULL)
  {
    fclose(this->FileFD);
    this->FileFD = NULL;
  }

  if (this->CurrentActor)
  {
    this->CurrentActor->Delete();
    this->CurrentActor = NULL;
  }
  if (this->CurrentLight)
  {
    this->CurrentLight->Delete();
    this->CurrentLight = NULL;
  }
  if (this->CurrentProperty)
  {
    this->CurrentProperty->Delete();
    this->CurrentProperty = NULL;
  }
  if (this->CurrentSource)
  {
    this->CurrentSource->Delete();
    this->CurrentSource = NULL;
  }
  if (this->CurrentPoints)
  {
    this->CurrentPoints->Delete();
    this->CurrentPoints = NULL;
  }
  if (this->CurrentNormals)
  {
    this->CurrentNormals->Delete();
    this->CurrentNormals = NULL;
  }
  if (this->CurrentTCoords)
  {
    this->CurrentTCoords->Delete();
    this->CurrentTCoords = NULL;
  }
  if (this->CurrentTCoordCells)
  {
    this->CurrentTCoordCells->Delete();
    this->CurrentTCoordCells = NULL;
  }
  if (this->CurrentNormalCells)
  {
    this->CurrentNormalCells->Delete();
    this->CurrentNormalCells = NULL;
  }
  if (this->CurrentScalars)
  {
    this->CurrentScalars->Delete();
    this->CurrentScalars = NULL;
  }
  if (this->CurrentMapper)
  {
    this->CurrentMapper->Delete();
    this->CurrentMapper = NULL;
  }
  if (this->CurrentLut)
  {
    this->CurrentLut->Delete();
    this->CurrentLut = NULL;
  }
  if (this->CurrentTransform)
  {
    this->CurrentTransform->Delete();
    this->CurrentTransform = NULL;
  }
}

//----------------------------------------------------------------------------
// Yacc/lex routines to add stuff to the renderer.
void vtkVRMLImporter::enterNode(const char *nodeType)
{
  const VrmlNodeType *t = this->Parser->find(nodeType);
  if (t == NULL)
  {
    std::stringstream str;
    str << "Unknown node type " << nodeType;
    this->Parser->yyerror(str.str().c_str());
    throw str.str();
  }
  VrmlNodeType::FieldRec *fr = new VrmlNodeType::FieldRec;
  fr->nodeType = t;
  fr->fieldName = NULL;
  *this->Parser->currentField += fr;
  std::string nodeTypeName(fr->nodeType->getName());
  if (nodeTypeName == "Appearance")
  {
    if (this->CurrentProperty)
    {
      this->CurrentProperty->Delete();
    }
    this->CurrentProperty = vtkProperty::New();
    if (this->Parser->creatingDEF)
    {
      *this->Parser->useList +=
        new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentProperty);
      this->Parser->creatingDEF = 0;
    }
  }
  else if (nodeTypeName == "Box" ||
    nodeTypeName == "Cone" ||
    nodeTypeName == "Cylinder" ||
    nodeTypeName == "Sphere")
  {
    if (this->CurrentSource)
    {
      this->CurrentSource->Delete();
    }
    if (nodeTypeName == "Box")
    {
      vtkCubeSource *cube = vtkCubeSource::New();
      this->CurrentSource = cube;
    }
    else if (nodeTypeName == "Cone")
    {
      vtkConeSource *cone = vtkConeSource::New();
      cone->SetResolution(this->ShapeResolution);
      this->CurrentSource = cone;
    }
    else if (nodeTypeName == "Cylinder")
    {
      vtkCylinderSource *cyl = vtkCylinderSource::New();
      cyl->SetResolution(this->ShapeResolution);
      this->CurrentSource = cyl;
    }
    else if (nodeTypeName == "Sphere")
    {
      vtkSphereSource *sphere = vtkSphereSource::New();
      sphere->SetPhiResolution(this->ShapeResolution);
      sphere->SetThetaResolution(this->ShapeResolution);
      this->CurrentSource = sphere;
    }
    this->CurrentSource->Update();
    vtkNew<vtkPolyDataMapper> pmap;
    pmap->SetInputConnection(this->CurrentSource->GetOutputPort());
    this->CurrentActor->SetMapper(pmap.Get());
    if (this->CurrentProperty)
    {
      this->CurrentActor->SetProperty(this->CurrentProperty);
    }
    if (this->Parser->creatingDEF)
    {
      *this->Parser->useList +=
        new vtkVRMLUseStruct(this->Parser->curDEFName, pmap.Get());
      this->Parser->creatingDEF = 0;
    }
  }
  else if (nodeTypeName == "DirectionalLight")
  {
    if (this->CurrentLight)
    {
      this->CurrentLight->Delete();
    }
    this->CurrentLight = vtkLight::New();
    this->Renderer->AddLight(this->CurrentLight);
    if (this->Parser->creatingDEF)
    {
      *this->Parser->useList +=
        new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentLight);
      this->Parser->creatingDEF = 0;
    }
  }
  else if (nodeTypeName == "IndexedFaceSet" ||
    nodeTypeName == "IndexedLineSet" ||
    nodeTypeName == "PointSet")
  {
    if (this->CurrentMapper)
    {
      this->CurrentMapper->Delete();
    }
    this->CurrentMapper = vtkPolyDataMapper::New();
    this->CurrentMapper->SetScalarVisibility(0);
    this->CurrentActor->SetMapper(this->CurrentMapper);
    if (this->CurrentProperty)
    {
      this->CurrentActor->SetProperty(this->CurrentProperty);
    }
    if (this->CurrentScalars)
    {
      this->CurrentScalars->Delete();
    }
    this->CurrentScalars = vtkFloatArray::New();
    if (this->Parser->creatingDEF)
    {
      *this->Parser->useList +=
        new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentMapper);
      this->Parser->creatingDEF = 0;
    }
  }
  else if (nodeTypeName == "Shape")
  {
    if (this->CurrentActor)
    {
      this->CurrentActor->Delete();
    }
    this->CurrentActor = vtkActor::New();
    if (this->CurrentProperty)
    {
      this->CurrentActor->SetProperty(this->CurrentProperty);
    }
    this->CurrentActor->SetOrientation(this->CurrentTransform->GetOrientation());
    this->CurrentActor->SetPosition(this->CurrentTransform->GetPosition());
    this->CurrentActor->SetScale(this->CurrentTransform->GetScale());
    // Add actor to renderer
    this->Renderer->AddActor(this->CurrentActor);
    if (this->Parser->creatingDEF)
    {
      *this->Parser->useList +=
        new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentActor);
      this->Parser->creatingDEF = 0;
    }
  }
  else if (nodeTypeName == "Transform")
  {
    this->CurrentTransform->Push();
  }
}

//----------------------------------------------------------------------------
void vtkVRMLImporter::exitNode()
{
  VrmlNodeType::FieldRec *fr = this->Parser->currentField->Top();
  assert(fr != NULL);
  this->Parser->currentField->Pop();

  std::string nodeTypeName = fr->nodeType->getName();
  // Exiting this means we need to setup the color mode and
  // normals and other fun stuff.
  if (nodeTypeName == "IndexedFaceSet" ||
    nodeTypeName == "IndexedLineSet" ||
    nodeTypeName == "PointSet")
  {
    // if tcoords exactly correspond with vertices (or there aren't any)
    // then can map straight through as usual
    // if not then must rejig using face-correspondence
    // (VRML supports per-face tcoords)
    // a similar scheme is implemented in vtkOBJReader

    bool tcoordsCorrespond;
    if ((!this->CurrentTCoords || !this->CurrentTCoordCells) &&
      (!this->CurrentNormals || !this->CurrentNormalCells))
    {
      tcoordsCorrespond = true; // there aren't any, can proceed
    }
    else if (this->CurrentTCoords &&
      this->CurrentTCoords->GetNumberOfTuples() !=
      this->CurrentPoints->GetNumberOfPoints())
    {
      tcoordsCorrespond = false; // false, must rejig
    }
    else if (this->CurrentNormals &&
      this->CurrentNormals->GetNumberOfTuples() !=
      this->CurrentPoints->GetNumberOfPoints())
    {
      tcoordsCorrespond = false; // false, must rejig
    }
    else
    {
      // the number of polygon faces and texture faces must be equal.
      // if they are not then something is wrong
      if (this->CurrentTCoordCells &&
        this->CurrentTCoordCells->GetNumberOfCells() !=
        this->CurrentMapper->GetInput()->GetPolys()->GetNumberOfCells())
      {
        vtkErrorMacro(
          <<"Number of faces does not match texture faces, output may not be correct")
          tcoordsCorrespond = true; // don't rejig
      }
      else if (this->CurrentNormalCells &&
        this->CurrentNormalCells->GetNumberOfCells() !=
        this->CurrentMapper->GetInput()->GetPolys()->GetNumberOfCells())
      {
        vtkErrorMacro(
          <<"Number of faces does not match normal faces, output may not be correct")
          tcoordsCorrespond = true; // don't rejig
      }
      else
      {
        // count of tcoords and points is the same, must run through indices to see if they
        // correspond by index point-for-point
        tcoordsCorrespond = true; // assume true until found otherwise
        if (this->CurrentTCoords && this->CurrentTCoordCells)
        {
          vtkCellArray* polys = this->CurrentMapper->GetInput()->GetPolys();
          polys->InitTraversal();
          this->CurrentTCoordCells->InitTraversal();
          vtkIdType npts, *pts;
          while (polys->GetNextCell(npts, pts))
          {
            vtkIdType nTCoordPts, *tcoordPts;
            this->CurrentTCoordCells->GetNextCell(nTCoordPts, tcoordPts);
            if (npts != nTCoordPts)
            {
              vtkErrorMacro(
                <<"Face size differs to texture face size, output may not be correct")
                break;
            }
            for (vtkIdType j = 0; j < npts; j++)
            {
              if (pts[j] != tcoordPts[j])
              {
                tcoordsCorrespond = false; // have found an exception
                break;
              }
            }
          }
        }

        if (this->CurrentNormals && this->CurrentNormalCells)
        {
          vtkCellArray* polys = this->CurrentMapper->GetInput()->GetPolys();
          polys->InitTraversal();
          this->CurrentNormalCells->InitTraversal();
          vtkIdType npts, *pts;
          while (polys->GetNextCell(npts, pts))
          {
            vtkIdType nNormalPts, *normalPts;
            this->CurrentNormalCells->GetNextCell(nNormalPts, normalPts);
            if (npts != nNormalPts)
            {
              vtkErrorMacro(
                <<"Face size differs to normal face size, output may not be correct")
              break;
            }
            for (vtkIdType j = 0; j < npts; j++)
            {
              if (pts[j] != normalPts[j])
              {
                tcoordsCorrespond = false; // have found an exception
                break;
              }
            }
          }
        }
      }
    }

    if (tcoordsCorrespond) // no rejigging necessary
    {
      vtkPolyData *pd = this->CurrentMapper->GetInput();
      if (pd == NULL)
      {
        pd = vtkPolyData::New();
        this->CurrentMapper->SetInputData(pd);
        pd->Delete();
      }
      pd->SetPoints(this->CurrentPoints);
      // We always create a scalar object in the enternode method.
      pd->GetPointData()->SetScalars(CurrentScalars);
      if (this->CurrentNormals)
      {
        pd->GetPointData()->SetNormals(CurrentNormals);
        this->CurrentNormals->Delete();
        this->CurrentNormals = NULL;
      }
      if (this->CurrentTCoords)
      {
        pd->GetPointData()->SetTCoords(CurrentTCoords);
        this->CurrentTCoords->Delete();
        this->CurrentTCoords = NULL;
      }
    }
    else  // must rejig
    {
      vtkDebugMacro(
        <<"Duplicating vertices so that tcoords and normals are correct");

      vtkNew<vtkPoints> newPoints;
      vtkNew<vtkFloatArray> newScalars;
      if (this->CurrentScalars)
      {
        newScalars->SetNumberOfComponents(
          this->CurrentScalars->GetNumberOfComponents());
      }
      vtkNew<vtkFloatArray> newTCoords;
      newTCoords->SetNumberOfComponents(2);
      vtkNew<vtkFloatArray> newNormals;
      newNormals->SetNumberOfComponents(3);
      vtkNew<vtkCellArray> newPolys;

      // for each poly, copy its vertices into newPoints (and point at them)
      // also copy its tcoords into newTCoords
      // also copy its normals into newNormals
      // also copy its scalar into newScalars
      vtkPolyData *pd = this->CurrentMapper->GetInput();
      vtkCellArray *polys = pd->GetPolys();
      polys->InitTraversal();
      if (this->CurrentTCoordCells)
      {
        this->CurrentTCoordCells->InitTraversal();
      }
      if (this->CurrentNormalCells)
      {
        this->CurrentNormalCells->InitTraversal();
      }
      vtkIdType npts, *pts;
      for (vtkIdType i = 0; polys->GetNextCell(npts, pts); i++)
      {
        vtkIdType n_tcoord_pts = 0, *tcoord_pts = 0;
        if (this->CurrentTCoordCells)
        {
          this->CurrentTCoordCells->GetNextCell(n_tcoord_pts, tcoord_pts);
        }
        vtkIdType n_normal_pts = 0, *normal_pts = 0;
        if (this->CurrentNormalCells)
        {
          this->CurrentNormalCells->GetNextCell(n_normal_pts, normal_pts);
        }

        // If some vertices have tcoords and not others
        // then we must do something else VTK will complain.
        // (crash on render attempt)
        // Easiest solution is to delete polys that don't have complete tcoords
        // (if there are any tcoords in the dataset)

        if (this->CurrentTCoords && npts != n_tcoord_pts &&
          this->CurrentTCoords->GetNumberOfTuples() > 0)
        {
          // skip this poly
          vtkDebugMacro(<<"Skipping poly "<< i + 1 <<" (1-based index)");
        }
        else if (this->CurrentNormals && npts != n_normal_pts &&
          this->CurrentNormals->GetNumberOfTuples() > 0)
        {
          // skip this poly
          vtkDebugMacro(<<"Skipping poly "<< i + 1<< " (1-based index)");
        }
        else
        {
          // copy the corresponding points, tcoords and normals across
          for (vtkIdType j = 0; j < npts; j++)
          {
            // copy the tcoord for this point across (if there is one)
            if (this->CurrentTCoords && n_tcoord_pts > 0)
            {
              newTCoords->InsertNextTuple(
                this->CurrentTCoords->GetTuple(tcoord_pts[j]));
            }
            // copy the normal for this point across (if any)
            if (this->CurrentNormals && n_normal_pts > 0)
            {
              newNormals->InsertNextTuple(
                this->CurrentNormals->GetTuple(normal_pts[j]));
            }
            // copy the scalar for this point across
            if (this->CurrentScalars)
            {
              newScalars->InsertNextTuple(
                this->CurrentScalars->GetTuple(pts[j]));
            }
            // copy the vertex into the new structure and update
            // the vertex index in the polys structure (pts is a pointer into it)
            pts[j] = newPoints->InsertNextPoint(
              this->CurrentPoints->GetPoint(pts[j]));
          }
          // copy this poly (pointing at the new points) into the new polys list
          newPolys->InsertNextCell(npts, pts);
        }
      }

      // use the new structures for the output
      pd->SetPoints(newPoints.Get());
      pd->SetPolys(newPolys.Get());
      if (this->CurrentTCoords)
      {
        pd->GetPointData()->SetTCoords(newTCoords.Get());
      }
      if (this->CurrentNormals)
      {
        pd->GetPointData()->SetNormals(newNormals.Get());
      }
      if (this->CurrentScalars)
      {
        pd->GetPointData()->SetScalars(newScalars.Get());
      }
    }

    if (this->CurrentLut)
    {
      this->CurrentScalars->InsertNextValue(this->CurrentLut->GetNumberOfColors());
      this->CurrentMapper->SetLookupTable(CurrentLut);
      this->CurrentMapper->SetScalarVisibility(1);
      // set for per vertex coloring
      this->CurrentLut->SetTableRange(0.0,
        float(this->CurrentLut->GetNumberOfColors() - 1));
      this->CurrentLut->Delete();
      this->CurrentLut = NULL;
    }
  }
  else if (nodeTypeName == "Shape")
  {
    if (this->CurrentProperty)
    {
      this->CurrentActor->SetProperty(this->CurrentProperty);
      this->CurrentProperty->Delete();
      this->CurrentProperty = 0;
    }
  }
  // simply pop the current transform
  else if (nodeTypeName == "Transform")
  {
    this->CurrentTransform->Pop();
  }

  delete fr;
}

//----------------------------------------------------------------------------
void vtkVRMLImporter::enterField(const char *fieldName)
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
    {
      return;
    }

    int type = fr->nodeType->hasField(fieldName);
    if (type != 0)
    {
      // Let the lexer know what field type to expect:
      this->Parser->expect(type);
    }
    else
    {
      vtkErrorMacro(<< "Error: Node's of type " << fr->nodeType->getName() <<
        " do not have fields/eventIn/eventOut named " <<
        fieldName);
      // expect(ANY_FIELD);
    }
  }
  // else expect(ANY_FIELD);
}

//----------------------------------------------------------------------------
void vtkVRMLImporter::exitField()
{
  VrmlNodeType::FieldRec *fr = this->Parser->currentField->Top();
  assert(fr != NULL);
  std::string fieldName(fr->fieldName);
  std::string nodeTypeName(fr->nodeType->getName());

  // For the radius field
  if (fieldName == "radius")
  {
    // Set the Sphere radius
    if (nodeTypeName == "Sphere")
    {
      static_cast<vtkSphereSource*>(this->CurrentSource)->
        SetRadius(this->Parser->yylval.sffloat);
    }
    // Set the Cylinder radius
    else if (nodeTypeName == "Cylinder")
    {
      static_cast<vtkCylinderSource*>(this->CurrentSource)->
        SetRadius(this->Parser->yylval.sffloat);
    }
  }
  // For the ambientIntensity field
  else if (fieldName == "ambientIntensity")
  {
    // Add to the current light
    if (nodeTypeName == "DirectionalLight")
    {
      this->CurrentLight->SetIntensity(this->Parser->yylval.sffloat);
    }
    // or the current material
    else if (nodeTypeName == "Material")
    {
      this->CurrentProperty->SetAmbient(this->Parser->yylval.sffloat);
    }
  }
  // For diffuseColor field, only in material node
  else if (fieldName == "diffuseColor")
  {
    this->CurrentProperty->SetDiffuseColor(
      this->Parser->yylval.vec3f->GetPoint(0));
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  // For emissiveColor field, only in material node
  else if (fieldName == "emissiveColor")
  {
    this->CurrentProperty->SetAmbientColor(
      this->Parser->yylval.vec3f->GetPoint(0));
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  // For shininess field, only in material node
  else if (fieldName == "shininess")
  {
    this->CurrentProperty->SetSpecularPower(this->Parser->yylval.sffloat);
  }
  // For specularcolor field, only in material node
  else if (fieldName == "specularColor")
  {
    this->CurrentProperty->SetSpecularColor(
      this->Parser->yylval.vec3f->GetPoint(0));
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  // For transparency field, only in material node
  else if (fieldName == "transparency")
  {
    this->CurrentProperty->SetOpacity(1.0 - this->Parser->yylval.sffloat);
  }
  // For the translation field of the Transform node
  else if (fieldName == "translation" && nodeTypeName == "Transform")
  {
    this->CurrentTransform->Translate(this->Parser->yylval.vec3f->GetPoint(0));
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  // For the scale field of the transform node
  else if (fieldName == "scale" && nodeTypeName == "Transform")
  {
    this->CurrentTransform->Scale(this->Parser->yylval.vec3f->GetPoint(0));
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  // For the size field of the current cube source
  else if (fieldName == "size" && nodeTypeName == "Box")
  {
    vtkCubeSource* cube = static_cast<vtkCubeSource*>(this->CurrentSource);
    double *len = this->Parser->yylval.vec3f->GetPoint(0);
    cube->SetXLength(len[0]);
    cube->SetYLength(len[1]);
    cube->SetZLength(len[2]);
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  // For the height field
  else if (fieldName == "height")
  {
    // Set the current Cone height
    if (nodeTypeName == "Cone")
    {
      static_cast<vtkConeSource*>(this->CurrentSource)->
        SetHeight(this->Parser->yylval.sffloat);
    }
    // or set the current Cylinder height
    else if (nodeTypeName == "Cylinder")
    {
      static_cast<vtkCylinderSource*>(this->CurrentSource)->
        SetHeight(this->Parser->yylval.sffloat);
    }
  }
  // For the bottomRadius field (only for Cone shapes)
  else if (fieldName == "bottomRadius" && nodeTypeName == "Cone")
  {
    static_cast<vtkConeSource*>(this->CurrentSource)->
      SetRadius(this->Parser->yylval.sffloat);
  }
  // Handle coordIndex for Indexed*Sets
  else if (fieldName == "coordIndex")
  {
    vtkNew<vtkPolyData> pd;
    vtkNew<vtkCellArray> cells;
    vtkIdType index = 0, cnt = 0;
    vtkIdType nbPoints = this->Parser->yylval.mfint32->GetMaxId();
    for (vtkIdType i = 0; i <= nbPoints; i++)
    {
      if (this->Parser->yylval.mfint32->GetValue(i) == -1)
      {
        cells->InsertNextCell(cnt, this->Parser->yylval.mfint32->GetPointer(index));
        index = i + 1;
        cnt = 0;
      }
      else
      {
        cnt++;
      }
    }
    if (cnt > 0)
    {
      cells->InsertNextCell(cnt, this->Parser->yylval.mfint32->GetPointer(index));
    }
    if (nodeTypeName == "IndexedFaceSet")
    {
      pd->SetPolys(cells.Get());
    }
    else
    {
      pd->SetLines(cells.Get());
    }

    this->CurrentMapper->SetInputData(pd.Get());
    this->Parser->yylval.mfint32->Reset();
    this->DeleteObject(this->Parser->yylval.mfint32);
  }
  // Handle point field
  else if (fieldName == "point")
  {
    // If for a coordinate node, simply used created FloatPoints
    if (nodeTypeName == "Coordinate")
    {
      if (this->CurrentPoints)
      {
        this->CurrentPoints->Delete();
      }
      this->CurrentPoints = this->Parser->yylval.vec3f;
      this->CurrentPoints->Register(this);
      // Seed the scalars with default values.
      this->CurrentScalars->Reset();
      vtkIdType nbPoints = this->CurrentPoints->GetNumberOfPoints();
      for (vtkIdType i = 0; i < nbPoints; i++)
      {
        this->CurrentScalars->InsertNextValue(i);
      }
      if (this->Parser->creatingDEF)
      {
        *this->Parser->useList +=
          new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentPoints);
        this->Parser->creatingDEF = 0;
      }
    }
    else if (nodeTypeName == "TextureCoordinate")
    {
      if (this->CurrentTCoords)
      {
        this->CurrentTCoords->Delete();
      }
      this->CurrentTCoords = this->Parser->yylval.vec2f;
      this->CurrentTCoords->Register(this);
    }
  }
  // Handle coord field, simply set the CurrentPoints
  else if (fieldName == "coord")
  {
    if (this->CurrentPoints)
    {
      this->CurrentPoints->Delete();
    }
    this->CurrentPoints = this->Parser->yylval.vec3f;
    this->CurrentPoints->Register(this);
    if (this->Parser->creatingDEF)
    {
      *this->Parser->useList +=
        new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentPoints);
      this->Parser->creatingDEF = 0;
    }

    // There is no coordIndex for PointSet data, generate the PolyData here.
    if (nodeTypeName == "PointSet")
    {
      vtkNew<vtkPolyData> pd;
      vtkNew<vtkCellArray> cells;
      vtkIdType nbPoints = this->Parser->yylval.vec3f->GetNumberOfPoints();
      for (vtkIdType i = 0; i < nbPoints; i++)
      {
        cells->InsertNextCell(1, &i);
      }

      pd->SetVerts(cells.Get());
      this->CurrentMapper->SetInputData(pd.Get());
    }
  }
  // Handle color field
  else if (fieldName == "color")
  {
    // For the Light nodes
    if (nodeTypeName == "DirectionalLight")
    {
      this->CurrentLight->SetColor(
        this->Parser->yylval.vec3f->GetPoint(0));
      this->Parser->yylval.vec3f->Reset();
      this->DeleteObject(this->Parser->yylval.vec3f);
      this->Parser->yylval.vec3f = NULL;
    }
    // For the Color node, Insert colors into lookup table
    // These are associated with the points in the coord field
    // and also in the colorIndex field
    if (nodeTypeName == "Color")
    {
      if (this->CurrentLut)
      {
        this->CurrentLut->Delete();
      }
      this->CurrentLut = vtkLookupTable::New();

      vtkIdType nbPoints = this->Parser->yylval.vec3f->GetNumberOfPoints();
      this->CurrentLut->SetNumberOfColors(nbPoints);
      this->CurrentLut->Build();
      for (vtkIdType i = 0; i < nbPoints; i++)
      {
        double vals4[4];
        this->Parser->yylval.vec3f->GetPoint(i, vals4);
        vals4[3] = 1.0;
        this->CurrentLut->SetTableValue(i, vals4);
      }
      if (this->Parser->creatingDEF)
      {
        *this->Parser->useList +=
          new vtkVRMLUseStruct(this->Parser->curDEFName, this->CurrentLut);
        this->Parser->creatingDEF = 0;
      }
    }
  }
  // Handle colorIndex field, always for an Indexed*Set
  else if (fieldName == "colorIndex")
  {
    vtkPolyData *pd = this->CurrentMapper->GetInput();
    if (pd == NULL)
    {
      pd = vtkPolyData::New();
      this->CurrentMapper->SetInputData(pd);
      pd->Delete();
    }
    vtkCellArray *cells = (pd->GetNumberOfPolys() > 0) ?
      pd->GetPolys() : pd->GetLines();
    cells->InitTraversal();
    vtkIdType *pts, npts;
    // At this point we either have colors index by vertex or faces
    // If faces, num of color indexes must match num of faces else
    // we assume index by vertex.
    if ((this->Parser->yylval.mfint32->GetMaxId() + 1) == pd->GetNumberOfPolys())
    {
      for (vtkIdType i = 0; i <= this->Parser->yylval.mfint32->GetMaxId(); i++)
      {
        if (this->Parser->yylval.mfint32->GetValue(i) >= 0)
        {
          cells->GetNextCell(npts, pts);
          for (vtkIdType j = 0; j < npts; j++)
          {
            this->CurrentScalars->SetComponent(pts[j], 0,
              this->Parser->yylval.mfint32->GetValue(i));
          }
        }
      }
    }
    // else handle colorindex by vertex
    else
    {
      cells->GetNextCell(npts, pts);
      vtkIdType len = this->Parser->yylval.mfint32->GetMaxId();
      for (vtkIdType i = 0, j = 0, index = 0; i <= len; i++)
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
          // for some files j can go past the number
          // of pts causing a segfault
          // so we explicitly check to be safe
          if (j < npts)
          {
            // Redirect color into scalar position
            this->CurrentScalars->SetComponent(pts[j++], 0,
              this->Parser->yylval.mfint32->GetValue(index++));
          }
        }
      }
    }
  }
  // Handle direction field of Directional light.
  else if (fieldName == "direction" && nodeTypeName == "DirectionalLight")
  {
    this->CurrentLight->SetFocalPoint(this->Parser->yylval.vec3f->GetPoint(0));
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  // Handle intensity field of Directional light.
  else if (fieldName == "intensity" && nodeTypeName == "DirectionalLight")
  {
    this->CurrentLight->SetIntensity(this->Parser->yylval.sffloat);
  }
  // Handle on field of Directional light.
  else if (fieldName == "on" && nodeTypeName == "DirectionalLight")
  {
    this->CurrentLight->SetSwitch(this->Parser->yylval.sfint);
  }
  // Handle colorPerVertex field
  else if (fieldName == "colorPerVertex")
  {
    // Same for all geometry nodes.
    this->CurrentMapper->SetScalarVisibility(this->Parser->yylval.sfint);
  }
  // Handle vector field for Normal Node
  else if (fieldName == "vector")
  {
    // For all floats in the vec3f, copy to the normal structure.
    if (this->CurrentNormals)
    {
      this->CurrentNormals->Delete();
    }
    this->CurrentNormals = vtkFloatArray::New();
    this->CurrentNormals->SetNumberOfComponents(3);
    vtkIdType nbPoints = this->Parser->yylval.vec3f->GetNumberOfPoints();
    this->CurrentNormals->SetNumberOfTuples(nbPoints);
    for (vtkIdType i = 0; i < nbPoints; i++)
    {
      this->CurrentNormals->InsertTuple(i,
        this->Parser->yylval.vec3f->GetPoint(i));
    }
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  else if (fieldName == "location")
  {
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  else if (fieldName == "position")
  {
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  else if (fieldName == "center")
  {
    this->Parser->yylval.vec3f->Reset();
    this->DeleteObject(this->Parser->yylval.vec3f);
    this->Parser->yylval.vec3f = NULL;
  }
  else if (fieldName == "texCoordIndex")
  {
    if (this->CurrentTCoordCells)
    {
      this->CurrentTCoordCells->Delete();
    }
    this->CurrentTCoordCells = vtkCellArray::New();

    // read the indices of the tcoords and assign accordingly
    vtkIdType index = 0, cnt = 0;
    vtkIdType len = this->Parser->yylval.mfint32->GetMaxId();
    for (vtkIdType i = 0; i <= len; i++)
    {
      if (this->Parser->yylval.mfint32->GetValue(i) == -1)
      {
        this->CurrentTCoordCells->InsertNextCell(cnt,
          this->Parser->yylval.mfint32->GetPointer(index));
        index = i+1;
        cnt = 0;
      }
      else
      {
        cnt++;
      }
    }
    if (cnt > 0)
    {
        this->CurrentTCoordCells->InsertNextCell(cnt,
          this->Parser->yylval.mfint32->GetPointer(index));
    }
    this->Parser->yylval.mfint32->Reset();
    this->DeleteObject(this->Parser->yylval.mfint32);
  }
  else if (fieldName == "normalIndex")
  {
    if (this->CurrentNormalCells)
    {
      this->CurrentNormalCells->Delete();
    }
    this->CurrentNormalCells = vtkCellArray::New();

    // read the indices of the normals and assign accordingly
    vtkIdType index = 0, cnt = 0;
    vtkIdType len = this->Parser->yylval.mfint32->GetMaxId();
    for (vtkIdType i = 0; i <= len; i++)
    {
      if (this->Parser->yylval.mfint32->GetValue(i) == -1)
      {
        this->CurrentNormalCells->InsertNextCell(cnt,
          this->Parser->yylval.mfint32->GetPointer(index));
        index = i+1;
        cnt = 0;
      }
      else
      {
        cnt++;
      }
    }
     if (cnt > 0)
     {
        this->CurrentNormalCells->InsertNextCell(cnt,
          this->Parser->yylval.mfint32->GetPointer(index));
     }
    this->Parser->yylval.mfint32->Reset();
    this->DeleteObject(this->Parser->yylval.mfint32);
  }
  fr->fieldName = NULL;
}

//----------------------------------------------------------------------------
void vtkVRMLImporter::useNode(const char *name)
{
  vtkObject *useO = this->GetVRMLDEFObject(name);
  if (!useO)
  {
    return;
  }
  std::string className = useO->GetClassName();
  if (className.find("Actor") != std::string::npos)
  {
    vtkActor *actor = vtkActor::New();
    actor->ShallowCopy(static_cast<vtkActor*>(useO));
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
    this->Renderer->AddActor(actor);
  }
  else if (className.find("PolyDataMapper") != std::string::npos)
  {
    vtkActor *actor = vtkActor::New();
    actor->SetMapper(static_cast<vtkPolyDataMapper*>(useO));
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
    this->Renderer->AddActor(actor);
  }
  else if (className == "vtkPoints")
  {
    vtkPoints *points = static_cast<vtkPoints*>(useO);
    this->Parser->yylval.vec3f = points;
    points->Register(this);
    if (this->CurrentPoints)
    {
      this->CurrentPoints->Delete();
    }
    this->CurrentPoints = points;
  }
  else if (className == "vtkLookupTable")
  {
    vtkLookupTable *lut = static_cast<vtkLookupTable*>(useO);
    lut->Register(this);
    if (this->CurrentLut)
    {
      this->CurrentLut->Delete();
    }
    this->CurrentLut = lut;
    // Seed the scalars with default values.
    this->CurrentScalars->Reset();
    vtkIdType nbPts = this->CurrentPoints->GetNumberOfPoints();
    for (vtkIdType i = 0; i < nbPts; i++)
    {
      this->CurrentScalars->InsertNextValue(i);
    }
  }
}

//----------------------------------------------------------------------------
// Send in the name from the VRML file, get the VTK object.
vtkObject* vtkVRMLImporter::GetVRMLDEFObject(const char *name)
{
  // Look through the type stack:
  // Need to go from top of stack since last DEF created is most current
  for (int i = this->Parser->useList->Count() - 1; i >= 0; i--)
  {
    const vtkVRMLUseStruct *nt = (*this->Parser->useList)[i];
    if (nt != NULL && strcmp(nt->defName, name) == 0)
    {
      return nt->defObject;
    }
  }
  return NULL;
}
