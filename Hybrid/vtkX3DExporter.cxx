/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkX3DExporter.h"

#include "vtkActor2D.h"
#include "vtkActor2DCollection.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkSmartPointer.h"
#include "vtkCellData.h"

#include "vtkstd/string"
#include <vtksys/ios/sstream>

#define vtkX3DPrintVector3(x) \
  (x)[0] << " " << (x)[1] << " " << (x)[2]

#include "vtkX3DExporterConfiguration.h"
#ifdef VTK_X3D_USE_JAVA
# include "vtkX3DExporterJavaHelper.h"
#endif

//----------------------------------------------------------------------------
class vtkX3DExporterWriter
{
public:
  vtkX3DExporterWriter();
  ~vtkX3DExporterWriter()
    {
    this->CloseFile();
    }

  void CloseFile();
  int OpenFile(const char* file, int binary = 0);
  void Write(const char* str);

  FILE* FilePointer;

#ifdef VTK_X3D_USE_JAVA
  vtkX3DExporterJavaHelper* JavaHelper;
#endif
};

//----------------------------------------------------------------------------
vtkX3DExporterWriter::vtkX3DExporterWriter()
{
  this->FilePointer = 0;

#ifdef VTK_X3D_USE_JAVA
  this->JavaHelper = 0;
#endif
}

//----------------------------------------------------------------------------
void vtkX3DExporterWriter::Write(const char* str)
{
  if ( this->FilePointer )
    {
    fwrite(str, 1, strlen(str), this->FilePointer);
    }
#ifdef VTK_X3D_USE_JAVA
  else if ( this->JavaHelper )
    {
    this->JavaHelper->Write(str, strlen(str));
    }
#endif
}

//----------------------------------------------------------------------------
int vtkX3DExporterWriter::OpenFile(const char* file, int binary)
{
  this->CloseFile();
  if ( binary )
    {
#ifdef VTK_X3D_USE_JAVA
    this->JavaHelper = vtkX3DExporterJavaHelper::New();
    return this->JavaHelper->OpenFile(file);
#endif
    }
  else
    {
    this->FilePointer = fopen(file, "w");
    return (this->FilePointer != 0);
    }
    
#ifndef VTK_X3D_USE_JAVA    
  return 0;
#endif  
}

//----------------------------------------------------------------------------
void vtkX3DExporterWriter::CloseFile()
{
  if ( this->FilePointer )
    {
    fclose(this->FilePointer);
    this->FilePointer = 0;
    }
#ifdef VTK_X3D_USE_JAVA
  if ( this->JavaHelper )
    {
    this->JavaHelper->Close();
    this->JavaHelper->Delete();
    this->JavaHelper = 0;
    }
#endif
}

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkX3DExporter, "1.9.2.2");
vtkStandardNewMacro(vtkX3DExporter);

//----------------------------------------------------------------------------
vtkX3DExporter::vtkX3DExporter()
{
  this->Speed = 4.0;
  this->FileName = NULL;
  this->Binary = 0;
}
//----------------------------------------------------------------------------
vtkX3DExporter::~vtkX3DExporter()
{
  this->SetFileName(0);
}


//----------------------------------------------------------------------------
void vtkX3DExporter::WriteData()
{
  vtkRenderer *ren;
  vtkActorCollection *ac;
  vtkActor2DCollection *a2Dc;
  vtkActor *anActor, *aPart;
  vtkActor2D *anTextActor2D, *aPart2D;
  vtkLightCollection *lc;
  vtkLight *aLight;
  vtkCamera *cam;
  double *tempd;

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
    }

  // check if binary is available
#ifndef VTK_X3D_USE_JAVA
  if ( this->Binary )
    {
    vtkErrorMacro("Binary mode writing is not available without the Java build");
    return;
    }
#endif

  // Let's assume the first renderer is the right one
  // first make sure there is only one renderer in this rendering window
  //if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
  //  {
  //  vtkErrorMacro(<< "X3D files only support one renderer per window.");
  //  return;
  //  }

  // get the renderer
  ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
    {
    vtkErrorMacro(<< "no actors found for writing X3D file.");
    return;
    }

  // try opening the files
  
  vtkX3DExporterWriter writer;

  if (!writer.OpenFile(this->FileName, this->Binary))
    {
    vtkErrorMacro(<< "unable to open X3D file " << this->FileName);
    return;
    }

  //
  //  Write header
  //
  vtkDebugMacro("Writing X3D file");

  vtksys_ios::ostringstream ostr;
  ostr << "<?xml version=\"1.0\" encoding =\"UTF-8\"?>\n\n"
    << "<X3D profile=\"Immersive\" version=\"3.0\">\n"
    << "  <head>\n"
    << "    <meta name=\"filename\" content=\"" << this->FileName << "\"/>\n"
    << "    <meta name=\"author\" content=\"The Visualization ToolKit\"/>\n"
    << "    <meta name=\"numberofelements\" content=\""
    << ren->GetActors()->GetNumberOfItems() << "\"/>\n"
    << "  </head>\n\n";


  ostr << "  <Scene>\n";

  // Start write the Background
  double background[3];
  ren->GetBackground(background);
  ostr << "    <Background  "
    << "   skyColor=\"" << vtkX3DPrintVector3(background) << "\"/>\n";
  // End of Background

  // do the camera
  cam = ren->GetActiveCamera();
  tempd = cam->GetOrientationWXYZ();
  ostr << "    <Viewpoint  fieldOfView=\""
    << (cam->GetViewAngle()*3.1415926/180.0) << "\""
    << "   position=\"" << vtkX3DPrintVector3(cam->GetPosition()) << "\""
    << "   description=\"Default View\""
    << "   orientation=\"" << tempd[1] << " " << tempd[2] << " " << tempd[3]
    << " " << (tempd[0]*3.1415926/180.0) << "\""
    << "   centerOfRotation=\"" << vtkX3DPrintVector3(cam->GetFocalPoint())  
    << "\"/>\n";

  // do the lights first the ambient then the others
  ostr << "    <NavigationInfo type='\"EXAMINE\" \"FLY\" \"ANY\"' speed=\""
    << this->Speed << "\"";

  // Headlight refers to the implicit headlight. 
  if (this->HasHeadLight(ren))
    {
    ostr << "  headlight=\"true\"/>\n\n";
    }
  else
    {
    ostr << "  headlight=\"false\"/>\n\n";
    }

  ostr << "    <DirectionalLight ambientIntensity=\"1\" intensity=\"0\" "
    << "  color=\"" << vtkX3DPrintVector3(ren->GetAmbient()) << "\"/>\n\n";


  // label ROOT
  ostr << "    <Transform  DEF=\"ROOT\"  translation=\"0.0 0.0 0.0\">\n";

  // Write first batch
  writer.Write(ostr.str().c_str());

  // make sure we have a default light
  // if we dont then use a headlight
  lc = ren->GetLights();
  vtkCollectionSimpleIterator lsit;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit)); )
    {
    // Skip headlight, since it was added as a part of NavigationInfo.
    if (!aLight->LightTypeIsHeadlight())
      {
      this->WriteALight(aLight, &writer);
      }
    }

  // do the actors now
  ac = ren->GetActors();
  vtkAssemblyPath *apath;
  vtkCollectionSimpleIterator ait;
  int index=0;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
    {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
      {
      if(anActor->GetVisibility()!=0)
        {
        aPart=(vtkActor *)apath->GetLastNode()->GetViewProp();
        this->WriteAnActor(aPart, &writer, index);
        index++;
        }
      }
    }

  // Write batch
  writer.Write("    </Transform>\n");

  //////////////////////////////////////////////
  // do the 2D actors now
  a2Dc = ren->GetActors2D();

  if(a2Dc->GetNumberOfItems()!=0)
    {
    vtksys_ios::ostringstream ostr2;
    ostr2 << "  <ProximitySensor  DEF=\"PROX_LABEL\" "
      << " size=\"1000000.0 1000000.0 1000000.0\"/>\n";

    //disable collision for the text annotations
    ostr2 << "  <Collision  enabled=\"false\">\n";

    //add a Label TRANS_LABEL for the text annotations and the sensor
    ostr2 << "    <Transform  DEF=\"TRANS_LABEL\" >\n";

    // Write batch
    writer.Write(ostr2.str().c_str());

    vtkAssemblyPath *apath2D;
    vtkCollectionSimpleIterator ait2D;
    for (a2Dc->InitTraversal(ait2D);
      (anTextActor2D = a2Dc->GetNextActor2D(ait2D)); )
      {

      for (anTextActor2D->InitPathTraversal();
        (apath2D=anTextActor2D->GetNextPath()); )
        {
        aPart2D=(vtkActor2D *)apath2D->GetLastNode()->GetViewProp();
        this->WriteanTextActor2D(aPart2D, &writer);
        }
      }

    vtksys_ios::ostringstream ostr3;
    ostr3
      << "    </Transform>\n"
      << "  </Collision>\n"
      <<
      "<ROUTE fromNode=\"PROX_LABEL\" fromField=\"position_changed\""
      << " toNode=\"TRANS_LABEL\" toField=\"translation\"/>\n"
      << "<ROUTE fromNode=\"PROX_LABEL\" fromField=\"orientation_changed\""
      << " toNode=\"TRANS_LABEL\" toField=\"rotation\"/>\n";

    // Write batch
    writer.Write(ostr3.str().c_str());
    }
  /////////////////////////////////////////////////

  writer.Write("  </Scene>\n"
    "</X3D>\n");

  writer.CloseFile();
}

//----------------------------------------------------------------------------
int vtkX3DExporter::HasHeadLight(vtkRenderer* ren)
{
  // make sure we have a default light
  // if we dont then use a headlight
  vtkLightCollection* lc = ren->GetLights();
  vtkCollectionSimpleIterator lsit;
  vtkLight* aLight=0;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit)); )
    {
    if (aLight->LightTypeIsHeadlight())
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WriteALight(vtkLight *aLight,
  vtkX3DExporterWriter* writer)
{
  double *pos, *focus, *color;
  double dir[3];

  vtksys_ios::ostringstream ostr;

  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  color = aLight->GetColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  vtkMath::Normalize(dir);

  if (aLight->GetPositional())
    {
    double *attn;

    if (aLight->GetConeAngle() >= 180.0)
      {
      ostr << "    <PointLight ";
      }
    else
      { 
      ostr << "    <SpotLight "
        << "  direction=\"" << vtkX3DPrintVector3(dir) << "\""
        << "  cutOffAngle=\"" << aLight->GetConeAngle() << "\"";
      }
    ostr << "  location=\"" << vtkX3DPrintVector3(pos) << "\"";
    attn = aLight->GetAttenuationValues();
    ostr << "  attenuation=\"" << vtkX3DPrintVector3(attn) << "\"";
    }
  else
    {
    ostr << "    <DirectionalLight"
      << "      direction=\"" << vtkX3DPrintVector3(dir) << "\"";
    }

  ostr << "  color=\"" << vtkX3DPrintVector3(color) << "\""
    << "  intensity=\"" << aLight->GetIntensity() << "\"";
  if (aLight->GetSwitch())
    {
    ostr << "  on=\"true\"/>\n\n";
    }
  else
    {
    ostr << "  on=\"false\"/>\n\n";
    }
  writer->Write(ostr.str().c_str());
}

//----------------------------------------------------------------------------
static void vtkX3DExporterUseData(
  vtkDataArray *normals,
  vtkDataArray *tcoords, 
  vtkUnsignedCharArray *colors,
  int index, 
  vtksys_ios::ostringstream& ostr)
{
  char indexString[100];
  sprintf(indexString, "%04d", index);

  // write out the points
  ostr << "            <Coordinate USE=\"VTKcoordinates" << indexString 
    << "\" />\n";

  // write out the point data
  if (normals)
    {
    ostr << "            <Normal USE=\"VTKnormals" << indexString << "\" />\n";
    }

  // write out the point data
  if (tcoords)
    {
    ostr << "            <TextureCoordinate USE=\"VTKtcoords" 
      << indexString << "\" />\n";
    }

  // write out the point data
  if (colors)
    {
    ostr << "            <Color USE=\"VTKcolors" << indexString << "\" />\n";
    }
}

//----------------------------------------------------------------------------
static void vtkX3DExporterWriteData(vtkPoints *points, 
  vtkDataArray *normals,
  vtkDataArray *tcoords, 
  vtkUnsignedCharArray *colors,
  int index, 
  vtksys_ios::ostringstream& ostr)
{
  double *p;
  int i;
  
  char indexString[100];
  sprintf(indexString, "%04d", index);
  
  // write out the points
  ostr << "            <Coordinate DEF =\"VTKcoordinates" << indexString
    << "\"  \n"
    << "              point =\"\n";
  for (i = 0; i < points->GetNumberOfPoints(); i++)
    {
    p = points->GetPoint(i);
    ostr << "              " << vtkX3DPrintVector3(p) << ",\n";
    }
  ostr << "              \"\n"
    << "            />\n";

  // write out the point data
  if (normals)
    {
    ostr << "            <Normal DEF =\"VTKnormals" << indexString << "\"  \n"
      << "              vector =\"\n";
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
      {
      p = normals->GetTuple(i);
      ostr << "           " << vtkX3DPrintVector3(p) << ",\n";
      }
    ostr << "            \"\n"
      << "          />\n";
    }

  // write out the point data
  if (tcoords)
    {
    ostr << "            <TextureCoordinate DEF =\"VTKtcoords" << indexString
      << "\"  \n"
      << "              point =\"\n";
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
      {
      p = tcoords->GetTuple(i);
      ostr << "           " << p[0] << " " << p[1] << ",\n";
      }
    ostr << "            \"\n"
      << "          />\n";
    }

  // write out the point data
  if (colors)
    {
    unsigned char c[4];

    ostr << "            <Color DEF =\"VTKcolors" << indexString << "\"  \n"
      << "              color=\"\n";
    for (i = 0; i < colors->GetNumberOfTuples(); i++)
      {
      colors->GetTupleValue(i,c);
      ostr << "           " << (c[0]/255.0) << " " << (c[1]/255.0) << " "
        << (c[2]/255.0) << ",\n";
      }
    ostr << "            \"\n"
      << "          />\n";
    }
}

//----------------------------------------------------------------------------
static bool vtkX3DExporterWriterRenderVerts(
  vtkPoints* points, vtkCellArray* cells,
  vtkUnsignedCharArray* colors, bool cell_colors,
  vtksys_ios::ostringstream& ostr)
{
  vtksys_ios::ostringstream color_stream;
  vtksys_ios::ostringstream coordinate_stream;

  vtkIdType cellId = 0;
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  for (cells->InitTraversal(); cells->GetNextCell(npts,indx); cellId++)
    {
    for (vtkIdType cc=0; cc < npts; cc++)
      {
      double* point = points->GetPoint(indx[cc]);
      coordinate_stream << "             " << vtkX3DPrintVector3(point) << ",\n";

      if (colors)
        {
        unsigned char color[4];
        if (cell_colors)
          {
          colors->GetTupleValue(cellId, color);
          }
        else
          {
          colors->GetTupleValue(indx[cc], color);
          }

        double dcolor[3];
        dcolor[0] = color[0]/255.0;
        dcolor[1] = color[1]/255.0;
        dcolor[2] = color[2]/255.0;
        color_stream << "             " << vtkX3DPrintVector3(dcolor) << ",\n";
        }
      }
    }
  ostr << "           <PointSet>\n"
       << "             <Coordinate \n"
       << "               point=\"\n" << coordinate_stream.str().c_str()
       << "                     \" \n"
       << "             />\n";

  if (colors)
    {
    ostr << "           <Color \n"
         << "             color=\"\n" << color_stream.str().c_str()
         << "                   \" \n"
         << "           />\n";
    }
  ostr   << "         </PointSet>\n";
  return true; 
}

//----------------------------------------------------------------------------
static bool vtkX3DExporterWriterRenderFaceSet(
  int cellType,
  int representation,
  vtkPoints* points,
  vtkIdType cellOffset,
  vtkCellArray* cells,
  vtkUnsignedCharArray* colors, bool cell_colors,
  vtkDataArray* normals, bool cell_normals, 
  vtkDataArray* tcoords, 
  bool common_data_written, int index, vtksys_ios::ostringstream& ostr)
{
  vtksys_ios::ostringstream coordindex_stream;
  vtksys_ios::ostringstream cellindex_stream;
  
  vtkIdType npts = 0;
  vtkIdType *indx = 0;

  if (cellType == VTK_POLYGON || cellType == VTK_POLY_LINE)
    {
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); cellOffset++)
      {
      coordindex_stream << "             ";
      for (vtkIdType cc=0; cc < npts; cc++)
        {
        coordindex_stream << (int)indx[cc] << " ";
        }

      if (representation == VTK_WIREFRAME && npts>2 && cellType == VTK_POLYGON)
        {
        // close the polygon.
        coordindex_stream << (int)indx[0] << " ";
        }
      coordindex_stream << "-1\n";

      vtkIdType cellid = cellOffset;
      cellindex_stream << "             " << (int)cellid << ",\n ";
      }
    }
  else // cellType == VTK_TRIANGLE_STRIP
    {
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); cellOffset++)
      {
      for (vtkIdType cc=2; cc < npts; cc++)
        {
        vtkIdType i1;
        vtkIdType i2;
        if (cc%2)
          {
          i1 = cc - 1;
          i2 = cc - 2;
          }
        else
          {
          i1 = cc - 2;
          i2 = cc - 1;
          }

        coordindex_stream << "             " << "              "
          << ((int)indx[i1]) << " " << ((int)indx[i2])
          << " " << ((int)indx[cc]);
        if (representation == VTK_WIREFRAME)
          {
            // close the polygon when drawing lines
          coordindex_stream << " " << (int)indx[i1];
          }
        coordindex_stream << " -1,\n";

        vtkIdType cellid = cellOffset;
        cellindex_stream << "             "<< (int)cellid << ",\n ";
        }
      }
    }

  if (representation == VTK_SURFACE)
    {
    ostr  << "          <IndexedFaceSet \n"
          << "            solid=\"false\"\n"
          << "            colorPerVertex=\"" << (cell_colors? "false" : "true") << "\"\n"
          << "            normalPerVertex=\"" << (cell_normals? "false" : "true") << "\"\n"
          << "            coordIndex=\"\n" << coordindex_stream.str().c_str() << "\"\n";
    }
  else
    {
    // don't save normals/tcoords when saving wireframes.
    normals = 0;
    tcoords = 0;

    ostr  << "          <IndexedLineSet \n"
          << "            colorPerVertex=\"" << (cell_colors? "false" : "true") << "\"\n"
          << "            coordIndex=\"\n" << coordindex_stream.str().c_str() << "\"\n";
    }

  if (normals && cell_normals && representation == VTK_SURFACE)
    {
    ostr  << "            normalIndex=\"\n" << cellindex_stream.str().c_str() << "\"\n";
    }

  if (colors && cell_colors)
    {
    ostr  << "            colorIndex=\"\n" << cellindex_stream.str().c_str() << "\"\n";
    }
  ostr  << "            >\n";

  // Now save Coordinate, Color, Normal TextureCoordinate nodes.
  // Use DEF/USE to avoid duplicates.
  if (!common_data_written)
    {
    vtkX3DExporterWriteData(points, normals, tcoords, colors, index, ostr);
    }
  else
    {
    vtkX3DExporterUseData(normals, tcoords, colors, index, ostr);
    }

  if (representation == VTK_SURFACE)
    {
    ostr  << "          </IndexedFaceSet>\n";
    }
  else
    {
    ostr  << "          </IndexedLineSet>\n";
    }

  return true;
}

//----------------------------------------------------------------------------
static bool vtkX3DExporterWriterUsingCellColors(vtkActor* anActor)
{
  int cellFlag = 0;
  vtkMapper* mapper = anActor->GetMapper();
  vtkAbstractMapper::GetScalars(
    mapper->GetInput(), 
    mapper->GetScalarMode(), 
    mapper->GetArrayAccessMode(),
    mapper->GetArrayId(),
    mapper->GetArrayName(), cellFlag);
  return (cellFlag == 1);
}

//----------------------------------------------------------------------------
// Render polydata as a point set. This method correctly renders cell as well as
// point colors.
static bool vtkX3DExporterWriterRenderPoints(
  vtkPolyData* pd, 
  vtkUnsignedCharArray* colors,
  bool cell_colors,
  vtksys_ios::ostringstream& geometry_stream)
{
  if (pd->GetNumberOfCells() == 0)
    {
    return false;
    }

  vtksys_ios::ostringstream color_stream;
  vtksys_ios::ostringstream coordinate_stream;

  vtkPoints* points = pd->GetPoints();

  // We render as cells so that even when coloring with cell data, the points
  // are assigned colors correctly.

  if ( (colors !=0) && cell_colors)
    {
    // Cell colors are used, however PointSet element can only have point
    // colors, hence we use this method. Although here we end up with duplicate
    // points, that's exactly what happens in case of OpenGL rendering, so it's
    // okay.
    vtkIdType numCells = pd->GetNumberOfCells();
    vtkIdList* pointIds = vtkIdList::New();
    for (vtkIdType cid =0; cid < numCells; cid++)
      {
      pointIds->Reset();
      pd->GetCellPoints(cid, pointIds);

      // Get the color for this cell.
      unsigned char color[4];
      colors->GetTupleValue(cid, color);
      double dcolor[3];
      dcolor[0] = color[0]/255.0;
      dcolor[1] = color[1]/255.0;
      dcolor[2] = color[2]/255.0;

      for (vtkIdType cc=0; cc < pointIds->GetNumberOfIds(); cc++)
        {
        vtkIdType pid = pointIds->GetId(cc);
        double* point = points->GetPoint(pid);
        coordinate_stream << "             " << vtkX3DPrintVector3(point) <<",\n";
        color_stream << "             "<< vtkX3DPrintVector3(dcolor) << ",\n";
        }
      }
    pointIds->Delete();
    }
  else
    {
    // Colors are point colors, simply render all the points and corresponding
    // colors.
    vtkIdType numPoints = points->GetNumberOfPoints();
    for (vtkIdType pid=0; pid < numPoints; pid++)
      {
      double* point = points->GetPoint(pid);
      coordinate_stream << "             "<< vtkX3DPrintVector3(point) <<",\n";

      if (colors)
        {
        unsigned char color[4];
        colors->GetTupleValue(pid, color);
        double dcolor[3];
        dcolor[0] = color[0]/255.0;
        dcolor[1] = color[1]/255.0;
        dcolor[2] = color[2]/255.0;
        color_stream << "             " << vtkX3DPrintVector3(dcolor) << ",\n";
        }
      }
    }

  geometry_stream << "            <PointSet>\n"
                  << "              <Coordinate \n"
                  << "                point=\"\n" << coordinate_stream.str().c_str()
                  << "                      \" \n"
                  << "              />\n";

  if (colors)
    {
    geometry_stream << "            <Color \n"
                    << "              color=\"\n" << color_stream.str().c_str()
                    << "                    \" \n"
                    << "            />\n";
    }
  geometry_stream << "  </PointSet>\n";
  return true; 
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WriteAnActor(vtkActor *anActor,
  vtkX3DExporterWriter* writer, int index)
{
  vtkDataSet *ds;
  vtkPolyData *pd;
  vtkSmartPointer<vtkGeometryFilter> gf;
  vtkPointData *pntData;
  vtkCellData *cellData;
  vtkPoints *points;
  vtkDataArray *normals = NULL;
  vtkDataArray *tcoords = NULL;
  vtkProperty *prop;
  double *tempd;
  double tempf2;
  vtkUnsignedCharArray *colors;
  vtkTransform *trans;
  int totalValues;
  int i;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == NULL)
    {
    return;
    }

  // Essential to turn of interpolate scalars otherwise GetScalars() may return
  // NULL. We restore value before returning.
  int isbm = anActor->GetMapper()->GetInterpolateScalarsBeforeMapping();
  anActor->GetMapper()->SetInterpolateScalarsBeforeMapping(0);

  // first stuff out the transform
  trans = vtkTransform::New();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());

  vtksys_ios::ostringstream ostr;
  ostr << "      <Transform ";
  tempd = trans->GetPosition();
  ostr << " translation=\"" << vtkX3DPrintVector3(tempd) << "\"";
  tempd = trans->GetOrientationWXYZ();
  ostr << " rotation=\"" << tempd[1] << " " << tempd[2] << " "
    << tempd[3] << " " << (tempd[0]*3.1415926/180.0) << "\"";
  tempd = trans->GetScale();
  ostr << " scale=\"" << vtkX3DPrintVector3(tempd) << "\">\n";
  trans->Delete();

  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();

  // we really want polydata
  if ( ds->GetDataObjectType() != VTK_POLY_DATA )
    {
    gf = vtkSmartPointer<vtkGeometryFilter>::New();
    gf->SetInput(ds);
    gf->Update();
    pd = gf->GetOutput();
    }
  else
    {
    pd = (vtkPolyData *)ds;
    }

  prop = anActor->GetProperty();
  points = pd->GetPoints();
  pntData = pd->GetPointData();
  tcoords = pntData->GetTCoords();
  cellData = pd->GetCellData();

  colors  = anActor->GetMapper()->MapScalars(255.0);

  // Are we using cell colors.
  bool cell_colors = vtkX3DExporterWriterUsingCellColors(anActor);

  normals = pntData->GetNormals();

  // Are we using cell normals.
  bool cell_normals = false;
  if (prop->GetInterpolation() == VTK_FLAT || !normals)
    {
    // use cell normals, if any.
    normals = cellData->GetNormals();
    cell_normals = true;
    }

  // write out the material properties to the mat file
  int representation = prop->GetRepresentation();
  vtksys_ios::ostringstream appearance_stream;
  appearance_stream
    << "          <Appearance >\n"
    << "            <Material "
    << " ambientIntensity=\"" << prop->GetAmbient() << "\"";
  // if we don't have colors and we have only lines & points
  // use emissive to color them
  if (!(normals || colors || pd->GetNumberOfPolys() ||
      pd->GetNumberOfStrips()))
    {
    tempf2 = prop->GetAmbient();
    tempd = prop->GetAmbientColor();
    appearance_stream << " emissiveColor=\""
      << (tempd[0]*tempf2) << " "
      << (tempd[1]*tempf2) << " "
      << (tempd[2]*tempf2) << "\"";
    }
  else
    {
    appearance_stream << " emissiveColor=\"0 0 0\"";
    }
  tempf2 = prop->GetDiffuse();
  tempd = prop->GetDiffuseColor();
  appearance_stream << " diffuseColor=\""
    << (tempd[0]*tempf2) << " " << (tempd[1]*tempf2)
    << " " << (tempd[2]*tempf2) << "\"",
  tempf2 = prop->GetSpecular();
  tempd = prop->GetSpecularColor();
  appearance_stream << " specularColor=\""
    << (tempd[0]*tempf2) << " " << tempd[1]*tempf2
    << " " << (tempd[2]*tempf2) << "\""
    << " shininess=\"" << (prop->GetSpecularPower()/128.0) << "\""
    << " transparency=\"" << (1.0 - prop->GetOpacity()) << "\""
    << "/>\n"; // close matrial

  // is there a texture map
  if (anActor->GetTexture())
    {
    vtkTexture *aTexture = anActor->GetTexture();
    int *size, xsize, ysize, bpp;
    vtkDataArray *scalars;
    vtkDataArray *mappedScalars;
    unsigned char *txtrData;

    // make sure it is updated and then get some info
    if (aTexture->GetInput() == NULL)
      {
      vtkErrorMacro(<< "texture has no input!\n");
      return;
      }
    aTexture->GetInput()->Update();
    size = aTexture->GetInput()->GetDimensions();
    scalars = aTexture->GetInput()->GetPointData()->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
      }

    // make sure using unsigned char data of color scalars type
    if (aTexture->GetMapColorScalarsThroughLookupTable () ||
      (scalars->GetDataType() != VTK_UNSIGNED_CHAR) )
      {
      mappedScalars = aTexture->GetMappedScalars ();
      }
    else
      {
      mappedScalars = scalars;
      }

    // we only support 2d texture maps right now
    // so one of the three sizes must be 1, but it 
    // could be any of them, so lets find it
    if (size[0] == 1)
      {
      xsize = size[1]; ysize = size[2];
      }
    else
      {
      xsize = size[0];
      if (size[1] == 1)
        {
        ysize = size[2];
        }
      else
        {
        ysize = size[1];
        if (size[2] != 1)
          {
          vtkErrorMacro(<< "3D texture maps currently are not supported!\n");
          return;
          }
        }
      }

    appearance_stream << "            <PixelTexture \n";
    bpp = mappedScalars->GetNumberOfComponents();
    appearance_stream << "              image=\"" << xsize << " "
      << ysize << " " << bpp << "\"\n";
    txtrData = static_cast<vtkUnsignedCharArray*>(mappedScalars)->
      GetPointer(0);
    totalValues = xsize*ysize;
    for (i = 0; i < totalValues; i++)
      {
      char buffer[10];
      sprintf(buffer,"0x%.2x",*txtrData);
      appearance_stream << buffer;
      txtrData++;
      if (bpp > 1) 
        {
        sprintf(buffer,"%.2x",*txtrData);
        appearance_stream << buffer;
        txtrData++;
        }
      if (bpp > 2) 
        {
        sprintf(buffer,"%.2x",*txtrData);
        appearance_stream << buffer;
        txtrData++;
        }
      if (bpp > 3) 
        {
        sprintf(buffer,"%.2x",*txtrData);
        appearance_stream << buffer;
        txtrData++;
        }
      if (i%8 == 0)
        {
        appearance_stream << "\n";
        }
      else
        {
        appearance_stream << " ";
        }
      }
    if (!(aTexture->GetRepeat()))
      {
      appearance_stream << "              repeatS=\"false\"\n";
      appearance_stream << "              repeatT=\"false\"\n";
      }
    appearance_stream << "              />\n"; // close texture
    }
  appearance_stream << "            </Appearance>\n"; // close appearance

  if (representation == VTK_POINTS)
    {
    vtksys_ios::ostringstream geometry_stream;
    // If representation is points, then we don't have to render different cell
    // types in separate shapes, since the cells type no longer matter.
    if (vtkX3DExporterWriterRenderPoints(pd, colors, cell_colors, geometry_stream))
      {
      ostr  << "        <Shape>\n"
            << appearance_stream.str().c_str()
            << geometry_stream.str().c_str()
            << "        </Shape>\n"; // close the  Shape
      }
    ostr << "      </Transform>\n"; // close the original transform
    writer->Write(ostr.str().c_str());
    }
  else
    {
    // When rendering as lines or surface, we need to respect the cell
    // structure. This requires rendering polys, tstrips, lines, verts in
    // separate shapes.
    vtkCellArray* verts = pd->GetVerts();
    vtkCellArray* lines = pd->GetLines();
    vtkCellArray* polys = pd->GetPolys();
    vtkCellArray* tstrips = pd->GetStrips();

    vtksys_ios::ostringstream geometry_stream;

    vtkIdType numVerts = verts->GetNumberOfCells();
    vtkIdType numLines = lines->GetNumberOfCells();
    vtkIdType numPolys = polys->GetNumberOfCells();
    vtkIdType numStrips = tstrips->GetNumberOfCells();

    bool common_data_written = false;
    if (numPolys > 0)
      {
      // reset stream.
      geometry_stream.str("");
      if (vtkX3DExporterWriterRenderFaceSet(VTK_POLYGON, representation, points,
          (numVerts+numLines), polys, 
          colors, cell_colors, normals, cell_normals, 
          tcoords, common_data_written, index, geometry_stream))
        {
        common_data_written = true;
        ostr  << "        <Shape>\n"
              << appearance_stream.str().c_str()
              << geometry_stream.str().c_str()
              << "        </Shape>\n"; // close the  Shape
        }
      }

    if (numStrips > 0)
      {
      // reset stream.
      geometry_stream.str("");
      if (vtkX3DExporterWriterRenderFaceSet(VTK_TRIANGLE_STRIP,
          representation, points,
          (numVerts+numLines+numPolys), tstrips, 
          colors, cell_colors, normals, cell_normals, 
          tcoords, common_data_written, index, geometry_stream))
        {
        common_data_written = true;
        ostr  << "        <Shape>\n"
              << appearance_stream.str().c_str()
              << geometry_stream.str().c_str()
              << "        </Shape>\n"; // close the  Shape
        }
      }

    if (numLines > 0)
      {
      // reset stream.
      geometry_stream.str("");
      if (vtkX3DExporterWriterRenderFaceSet(VTK_POLY_LINE,
          (representation==VTK_SURFACE? VTK_WIREFRAME:representation), 
          points, (numVerts), lines, 
          colors, cell_colors, normals, cell_normals, 
          tcoords, common_data_written, index, geometry_stream))
        {
        common_data_written = true;
        ostr  << "        <Shape>\n"
          << appearance_stream.str().c_str()
          << geometry_stream.str().c_str()
          << "        </Shape>\n"; // close the  Shape
        }
      }

    if (numVerts > 0)
      {
      // reset stream.
      if (vtkX3DExporterWriterRenderVerts(
          points, verts,
          colors, cell_normals, geometry_stream))
        {
        ostr  << "        <Shape>\n"
          << appearance_stream.str().c_str()
          << geometry_stream.str().c_str()
          << "        </Shape>\n"; // close the  Shape
        }
      }

    ostr << "      </Transform>\n"; // close the original transform
    writer->Write(ostr.str().c_str());
    }

  anActor->GetMapper()->SetInterpolateScalarsBeforeMapping(isbm);
}

//----------------------------------------------------------------------------
void vtkX3DExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->FileName)
    {
    os << indent << "FileName: " << this->FileName << "\n";
    }
  else
    {
    os << indent << "FileName: (null)\n";
    }
  os << indent << "Speed: " << this->Speed << "\n";
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WriteanTextActor2D(vtkActor2D *anTextActor2D,
    vtkX3DExporterWriter* writer)
{
  char *ds;
  double x,y;
  vtkTextMapper *tm;

  // see if the actor has a mapper. it could be an assembly
  if ((vtkTextMapper*)(anTextActor2D->GetMapper()) == NULL)
    {
    return;
    }

  // add a sensor with a big size for the text annotations

  vtksys_ios::ostringstream ostr;
  tm=(vtkTextMapper*)anTextActor2D->GetMapper();
  ds = NULL;
  ds = tm->GetInput();

  if (ds==NULL)
    {
    return;
    }

  x=((anTextActor2D->GetPosition()[0])/(this->RenderWindow->GetSize()[0]));
  x-=0.5;
  y=((anTextActor2D->GetPosition()[1])/(this->RenderWindow->GetSize()[1]));
  y-=0.5;

  ostr << "      <Transform  translation=\"" << x << " " << y << " -2\" "
    << "scale=\"0.002 0.002 0.002\">\n"
    << "        <Shape >\n"
    << "          <Appearance >\n"
    << "            <Material  diffuseColor=\"0 0 1\" "
    << " emissiveColor=\""
    << tm->GetTextProperty()->GetColor()[0] << " "
    << tm->GetTextProperty()->GetColor()[1] << " "
    << tm->GetTextProperty()->GetColor()[2] << "\"/>\n"
    << "          </Appearance>\n"
    << "          <Text  string=\"" << ds << "\">\n";

  vtkstd::string style;

  style = " family=\"";
  switch(tm->GetTextProperty()->GetFontFamily())
    {
  case 0:
  default:
    style+="SANS";
    break;
  case 1:
    style+="TIPEWRITER";
    break;
  case 2:
    style+="SERIF";
    break;
    }
  style += "\" topToBottom=\"";
  switch( tm->GetTextProperty()->GetVerticalJustification())
    {
  case 0:
  default: 
    style += "false\"";
    break;
  case 2:
    style += "true\"";
    break;
    }

  style+="  justify='\"";

  switch  (tm->GetTextProperty()->GetJustification())
    {
  case 0:
  default:
    style += "BEGIN\"";
    break;
  case 2:
    style += "END\"";
    break;
    }

  style += " \"BEGIN\"'";

  ostr << "            <FontStyle  " << style.c_str()
    << " size=\"" << tm->GetTextProperty()->GetFontSize() << "\"/>\n"
    << "          </Text>\n"
    << "        </Shape>\n"
    << "      </Transform>\n";
  writer->Write(ostr.str().c_str());
}
