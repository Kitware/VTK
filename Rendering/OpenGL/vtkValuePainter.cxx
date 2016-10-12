/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkValuePainter.h"
#include "vtkObjectFactory.h"

#include "vtkAbstractMapper.h"
#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkNew.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLExtensionManager.h"

#include "vtkgl.h" // vtkgl namespace

vtkInformationKeyMacro(vtkValuePainter, SCALAR_MODE, Integer);
vtkInformationKeyMacro(vtkValuePainter, SCALAR_RANGE, DoubleVector);
vtkInformationKeyMacro(vtkValuePainter, ARRAY_ID, Integer);
vtkInformationKeyMacro(vtkValuePainter, ARRAY_NAME, String);
vtkInformationKeyMacro(vtkValuePainter, ARRAY_COMPONENT, Integer);

//TODO: template MapScalars to eliminate casting/conversion
//TODO: make MapScalars extensible if user has a different function in mind
//TODO: make MapScalars output within a band so that you can multipass on >24 bit quantities (like selection does)

class vtkValuePainter::vtkInternals
{
public:
  int FieldAssociation;
  int FieldAttributeType;
  std::string FieldName;
  bool FieldNameSet;
  int Component;
  double ScalarRange[2];
  bool ScalarRangeSet;
  bool MultisampleSupport;
  bool CheckedMSS;

  vtkNew<vtkImageData> Texture;

  // Description:
  // Constructor
  vtkInternals()
  {
    this->FieldNameSet = false;
    this->FieldAssociation = 0;
    this->FieldAttributeType = 0;
    this->Component = 0;
    this->ScalarRangeSet = false;
    this->ScalarRange[0] = 0.0;
    this->ScalarRange[1] = -1.0;

    #define MML 0x1000
    this->Texture->SetExtent(0,MML,0,0, 0,0);
    vtkSmartPointer<vtkUnsignedCharArray> chars = vtkSmartPointer<vtkUnsignedCharArray>::New();
    chars->SetNumberOfComponents(3);
    chars->SetNumberOfTuples(MML);
    unsigned char color[3];
    unsigned int i;
    for (i = 0; i < MML; i++)
    {
      vtkValuePainter::ValueToColor(i, 0, MML, color);
      chars->SetTuple3(i, color[0],color[1],color[2]);
    }
    this->Texture->GetPointData()->SetScalars(chars);
    this->MultisampleSupport = false;
    this->CheckedMSS = false;
  }

  // Description:
  // Destructor
  ~vtkInternals()
  {
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkValuePainter);

//----------------------------------------------------------------------------
vtkValuePainter::vtkValuePainter()
{
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkValuePainter::~vtkValuePainter()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkValuePainter::SetInputArrayToProcess(
      int fieldAssociation,
      const char* name)
{
  if ( !this->Internals->FieldNameSet
    || (this->Internals->FieldAssociation != fieldAssociation)
    || (this->Internals->FieldName != name) )
  {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldName = name;
    this->Internals->FieldNameSet = true;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkValuePainter::SetInputArrayToProcess(
      int fieldAssociation,
      int fieldAttributeType)
{
  if ( (this->Internals->FieldAssociation != fieldAssociation)
    || (this->Internals->FieldAttributeType != fieldAttributeType)
    || this->Internals->FieldNameSet )
  {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldAttributeType = fieldAttributeType;
    this->Internals->FieldNameSet = false;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkValuePainter::SetInputComponentToProcess(int comp)
{
  if ( this->Internals->Component != comp)
  {
    this->Internals->Component = comp;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkValuePainter::SetScalarRange(double min, double max)
{
  if (this->Internals->ScalarRange[0] != min ||
      this->Internals->ScalarRange[1] != max)
  {
    this->Internals->ScalarRange[0] = min;
    this->Internals->ScalarRange[1] = max;
    this->Internals->ScalarRangeSet = (max > min);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkValuePainter::ProcessInformation(vtkInformation* info)
{
  bool modify = false;
  bool byname = false;

  int fa = this->Internals->FieldAssociation;
  int aidx = this->Internals->FieldAttributeType;
  std::string aname = this->Internals->FieldName;

  if (info->Has(vtkValuePainter::SCALAR_MODE()))
  {
    if (this->Internals->FieldAssociation != info->Get(vtkValuePainter::SCALAR_MODE()))
    {
      fa = info->Get(vtkValuePainter::SCALAR_MODE());
      modify = true;
    }
  }

  if (info->Has(vtkValuePainter::ARRAY_ID()))
  {
    if (this->Internals->FieldAttributeType != info->Get(vtkValuePainter::ARRAY_ID()))
    {
      aidx = info->Get(vtkValuePainter::ARRAY_ID());
      modify = true;
      byname = false;
    }
  }

  if (info->Has(vtkValuePainter::ARRAY_NAME()))
  {
    if (this->Internals->FieldName != info->Get(vtkValuePainter::ARRAY_NAME()))
    {
      aname = info->Get(vtkValuePainter::ARRAY_NAME());
      modify = true;
      byname = true;
    }
  }

  if (modify)
  {
    if (byname)
    {
      this->SetInputArrayToProcess(fa, aname.c_str());
    }
    else
    {
      this->SetInputArrayToProcess(fa, aidx);
    }
  }

  if (info->Has(vtkValuePainter::ARRAY_COMPONENT()))
  {
    if (this->Internals->Component != info->Get(vtkValuePainter::ARRAY_COMPONENT()))
    {
      this->SetInputComponentToProcess(info->Get(vtkValuePainter::ARRAY_COMPONENT()));
    }
  }

  if (info->Has(vtkValuePainter::SCALAR_RANGE()))
  {
    double *nr = info->Get(vtkValuePainter::SCALAR_RANGE());
    if (this->Internals->ScalarRange[0] != nr[0] || this->Internals->ScalarRange[1] != nr[1])
    {
      this->SetScalarRange(nr[0], nr[1]);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkValuePainter::ValueToColor(double value, double min, double scale,
                                   unsigned char *color)
{
  //TODO: make this configurable
  double valueS = (value - min)/scale;
  valueS = (valueS<0.0?0.0:valueS); //prevent underflow
  valueS = (valueS>1.0?1.0:valueS); //prevent overflow
  int valueI = valueS * 0xfffffe + 0x1; //0 is reserved as "nothing"

  color[0] = (unsigned char)((valueI & 0xff0000)>>16);
  color[1] = (unsigned char)((valueI & 0x00ff00)>>8);
  color[2] = (unsigned char)((valueI & 0x0000ff));
}

//-----------------------------------------------------------------------------
void vtkValuePainter::ColorToValue(unsigned char *color, double min, double scale,
                                   double &value)
{
  //TODO: make this configurable
  int valueI = ((int)(*(color+0)))<<16 | ((int)(*(color+1)))<<8 | ((int)(*(color+2)));
  double valueS = (valueI-0x1)/(double)0xfffffe; //0 is reserved as "nothing"
  value = valueS*scale + min;
}

//-----------------------------------------------------------------------------
void vtkValuePainter::RenderInternal(
  vtkRenderer* renderer,
  vtkActor* vtkNotUsed(actor),
  unsigned long typeflags,
  bool vtkNotUsed(forceCompileOnly))
{
  if (typeflags == 0)
  {
    // No primitive to render.
    return;
  }

  this->Timer->StartTimer();

  vtkOpenGLClearErrorMacro();

  if (!this->Internals->CheckedMSS)
  {
    this->Internals->CheckedMSS = true;
    vtkOpenGLRenderWindow * context = vtkOpenGLRenderWindow::SafeDownCast
      (renderer->GetRenderWindow());
    if (context)
    {
      vtkOpenGLExtensionManager *manager = context->GetExtensionManager();
        // don't need any of the functions so don't bother
        // to load the extension, but do make sure enums are
        // defined.
      this->Internals->MultisampleSupport
        = manager->ExtensionSupported("GL_ARB_multisample")==1;
    }
  }

  //set render state so that colors we draw are not altered at all
  int oldSampling = 0;
  if (this->Internals->MultisampleSupport)
  {
    oldSampling = glIsEnabled(vtkgl::MULTISAMPLE);
    glDisable(vtkgl::MULTISAMPLE);
  }
  int oldLight = glIsEnabled(GL_LIGHTING);
  int oldBlend = glIsEnabled(GL_BLEND);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glColor4f(1,1,1,1);

  vtkPolyData* pd = this->GetInputAsPolyData();

  vtkIdType startCell = 0;

  if (typeflags & vtkPainter::VERTS)
  {
    this->DrawCells(VTK_POLY_VERTEX, pd->GetVerts(), startCell, renderer);
  }

  startCell += pd->GetNumberOfVerts();
  if (typeflags & vtkPainter::LINES)
  {
    this->DrawCells(VTK_POLY_LINE, pd->GetLines(), startCell, renderer);
  }

  startCell += pd->GetNumberOfLines();
  if (typeflags & vtkPainter::POLYS)
  {
    this->DrawCells(VTK_POLYGON, pd->GetPolys(), startCell, renderer);
  }

  startCell += pd->GetNumberOfPolys();
  if (typeflags & vtkPainter::STRIPS)
  {
    this->DrawCells(VTK_TRIANGLE_STRIP, pd->GetStrips(), startCell, renderer);
  }

  //restore render state to whatever it was before
  if (oldSampling)
  {
    glEnable(vtkgl::MULTISAMPLE);
  }
  if (oldLight)
  {
    glEnable(GL_LIGHTING);
  }
  if (oldBlend)
  {
    glEnable(GL_BLEND);
  }

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
}

//-----------------------------------------------------------------------------
void vtkValuePainter::DrawCells(
  int mode, vtkCellArray *connectivity, vtkIdType startCellId,
  vtkRenderer *renderer)
{
  vtkPolyData* pd = this->GetInputAsPolyData();
  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();

  vtkPoints* p = pd->GetPoints();
  if (!p)
  {
    return;
  }
  vtkIdType npts, *pts;
  vtkIdType cellId = startCellId;

  int pointtype = p->GetDataType();
  void* voidpoints = p->GetVoidPointer(0);
  int count = 0;

  int doingCells;
  vtkDataArray *values;

  if (this->Internals->FieldNameSet)
  {
    values = vtkAbstractMapper::GetScalars
      (pd, this->Internals->FieldAssociation, VTK_GET_ARRAY_BY_NAME, 1, this->Internals->FieldName.c_str(), doingCells);
  }
  else
  {
    values = vtkAbstractMapper::GetScalars
      (pd, this->Internals->FieldAssociation, VTK_GET_ARRAY_BY_ID, this->Internals->FieldAttributeType, NULL, doingCells);
  }
  if (!values)
  {
    vtkWarningMacro("Could not find array to draw.")
    return;
  }

  int comp = this->Internals->Component;
  if (comp < 0 || comp >= values->GetNumberOfComponents())
  {
    comp = 0;
  }
  double *minmax;
  if (this->Internals->ScalarRangeSet)
  {
    minmax = this->Internals->ScalarRange;
  }
  else
  {
    minmax = values->GetRange(comp);
  }
  double scale = minmax[1]-minmax[0];
  if (scale <= 0)
  {
    scale = values->GetDataTypeMax()-values->GetDataTypeMin();
  }

  vtkSmartPointer<vtkOpenGLTexture> internalColorTexture;
  if (!doingCells)
  {
    internalColorTexture = vtkSmartPointer<vtkOpenGLTexture>::New();
    //texture ensures that GL interpolates point values across polygons
    internalColorTexture->RepeatOff();
    internalColorTexture->SetInputData(this->Internals->Texture.GetPointer());
    internalColorTexture->Load(renderer);
  }

  unsigned char color[3];
  for (connectivity->InitTraversal(); connectivity->GetNextCell(npts, pts); count++)
  {
    device->BeginPrimitive(mode);

    if (doingCells)
    {
      double value = values->GetComponent(cellId, comp);
      this->ValueToColor(value, minmax[0], scale, color);

      renderer->GetRenderWindow()->GetPainterDeviceAdapter()
        ->SendAttribute(
        vtkDataSetAttributes::SCALARS, 3, VTK_UNSIGNED_CHAR, color);
    }

    for (vtkIdType cellpointi = 0; cellpointi < npts; cellpointi++)
    {
      vtkIdType pointId = pts[cellpointi];

      if (!doingCells)
      {
        double value = (values->GetComponent(pointId, comp) - minmax[0])/scale;

        renderer->GetRenderWindow()->GetPainterDeviceAdapter()
          ->SendAttribute(vtkDataSetAttributes::TCOORDS,1,VTK_DOUBLE,&value, 0);
      }

      renderer->GetRenderWindow()->GetPainterDeviceAdapter()
        ->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
                        pointtype, voidpoints, 3*pointId);
    }


    device->EndPrimitive();
    cellId++;
    if (count == 10000)
    {
      count = 0;
      // report progress
      this->UpdateProgress(static_cast<double>(cellId - startCellId)/this->TotalCells);
      // Abort render.
      if (renderer->GetRenderWindow()->CheckAbortStatus())
      {
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkValuePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
