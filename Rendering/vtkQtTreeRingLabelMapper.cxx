/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkQtTreeRingLabelMapper.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/

#include "vtkQImageToImageSource.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkQtTreeRingLabelMapper.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTexturedActor2D.h"
#include "vtkTextureMapToPlane.h"
#include "vtkTree.h"
#include "vtkUnicodeStringArray.h"
#include "vtkUnicodeString.h"

#include <QApplication>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QTextDocument>
#include <QTextStream>

// #include <QFile>
// #include <QIODevice>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkQtTreeRingLabelMapper);

vtkCxxSetObjectMacro(vtkQtTreeRingLabelMapper,LabelTextProperty,vtkTextProperty);

vtkQtTreeRingLabelMapper::vtkQtTreeRingLabelMapper()
{
  this->Input = NULL;
  this->Renderer = NULL;
  
  this->VCoord = vtkCoordinate::New();
  
  this->LabeledComponent = (-1);
  this->FieldDataArray = 0;
  this->FieldDataName = NULL;
  
  this->TextRotationArrayName = 0;
  this->SetTextRotationArrayName("TextRotation");
  this->SetSectorsArrayName("area");
  
  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetFontSize(10);
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();
  this->SetLabelFormat("%s");
  
  this->WindowSize[0] = this->WindowSize[1] = 0;
  
  this->PlaneSource = vtkPlaneSource::New();
  this->TextureMapToPlane = vtkTextureMapToPlane::New();
  this->polyDataMapper = vtkPolyDataMapper2D::New();
  
  this->QtImageSource = vtkQImageToImageSource::New();
  this->LabelTexture = vtkTexture::New();
  
  this->QtImage = new QImage( 1, 1, QImage::Format_ARGB32_Premultiplied );

//FIXME: QImage is initialized to grey.  This will fix that, although it is a bit of a hack...
  QPainter painter( this->QtImage );
  painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );
  painter.setCompositionMode( QPainter::CompositionMode_Clear );
  painter.drawImage( 0, 0, *this->QtImage );
  painter.setCompositionMode( QPainter::CompositionMode_SourceOver );

  this->QtImageSource->SetQImage( this->QtImage );
  this->LabelTexture->SetInput(this->QtImageSource->GetOutput());
  this->LabelTexture->PremultipliedAlphaOn();
  
  this->TextureMapToPlane->SetSRange( 0., 1. );
  this->TextureMapToPlane->SetTRange( 0., 1. );
  this->TextureMapToPlane->SetInputConnection(this->PlaneSource->GetOutputPort());
  this->TextureMapToPlane->AutomaticPlaneGenerationOn();
  
  this->polyDataMapper->SetInputConnection(this->TextureMapToPlane->GetOutputPort());
}

vtkQtTreeRingLabelMapper::~vtkQtTreeRingLabelMapper()
{
  this->SetRenderer(NULL);
  
  this->SetLabelTextProperty(NULL);
  this->SetFieldDataName(NULL);  
  
  this->SetTextRotationArrayName( 0 );
  
  this->VCoord->Delete();
  this->PlaneSource->Delete();
  this->TextureMapToPlane->Delete();
  this->polyDataMapper->Delete();
  
  this->QtImageSource->Delete();
  this->LabelTexture->Delete(); 
  
  delete this->QtImage;                            
}

//----------------------------------------------------------------------------
void vtkQtTreeRingLabelMapper::RenderOverlay(vtkViewport *viewport, 
                                             vtkActor2D *actor)
{
  vtkRenderer *ren = vtkRenderer::SafeDownCast(viewport);
  if (ren)
    {
    this->LabelTexture->Render(ren);
    }
  this->polyDataMapper->RenderOverlay(viewport, actor);
}

//----------------------------------------------------------------------------
void vtkQtTreeRingLabelMapper::RenderOpaqueGeometry(vtkViewport *viewport, 
                                                    vtkActor2D *actor)
{
  if(!QApplication::instance())
    {
    vtkErrorMacro("This class requires a QApplication instance.");
    return;
    }
  
  vtkTextProperty *tprop = this->LabelTextProperty;
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render labels");
    return;
    }
  
  int numComp = 0, activeComp = 0;
  vtkAbstractArray *abstractData;
  vtkDataArray *numericData, *sectorInfo;
  vtkStringArray *stringData;
  vtkUnicodeStringArray *uStringData;
  vtkTree *input=this->GetInputTree();
  if ( !input )
    {
    vtkErrorMacro(<<"Need input tree to render labels (2)");
    return;
    }
  
  if( input->GetNumberOfVertices() == 0 )
    {
    return;
    }
  
  vtkDataSetAttributes *pd = input->GetVertexData();
  sectorInfo = this->GetInputArrayToProcess(0, input);
  if( !sectorInfo )
    {
    vtkErrorMacro(<< "Input Tree does not have sector information.");
    return;
    }
  
  vtkRenderer* renderer = vtkRenderer::SafeDownCast( viewport );
  
  if ( this->CurrentViewPort != viewport || this->GetMTime() > this->BuildTime || 
       input->GetMTime() > this->BuildTime || tprop->GetMTime() > this->BuildTime ||
       renderer->GetActiveCamera()->GetMTime() > this->BuildTime )
    {
    vtkDebugMacro(<<"Rebuilding labels");
    
    int *size = renderer->GetSize();
    this->WindowSize[0] = size[0];
    this->WindowSize[1] = size[1];
    
    // See if we have to recalculate fonts sizes
    if (this->CurrentViewPort != viewport) 
      {
      this->CurrentViewPort = viewport;
      }
    
    // figure out what to label, and if we can label it
    abstractData = NULL;
    numericData = NULL;
    stringData = NULL;
    uStringData = NULL;
    switch (this->LabelMode)
      {
      case VTK_LABEL_SCALARS:
        if ( pd->GetScalars() )
          {
          numericData = pd->GetScalars();
          }
        break;
      case VTK_LABEL_VECTORS:   
        if ( pd->GetVectors() )
          {
          numericData = pd->GetVectors();
          }
        break;
      case VTK_LABEL_NORMALS:    
        if ( pd->GetNormals() )
          {
          numericData = pd->GetNormals();
          }
        break;
      case VTK_LABEL_TCOORDS:    
        if ( pd->GetTCoords() )
          {
          numericData = pd->GetTCoords();
          }
        break;
      case VTK_LABEL_TENSORS:    
        if ( pd->GetTensors() )
          {
          numericData = pd->GetTensors();
          }
        break;
      case VTK_LABEL_FIELD_DATA:
      {
      int arrayNum;
      if (this->FieldDataName != NULL)
        {
        abstractData = pd->GetAbstractArray(this->FieldDataName, arrayNum);
        }
      else
        {
        arrayNum = (this->FieldDataArray < pd->GetNumberOfArrays() ?
                    this->FieldDataArray : pd->GetNumberOfArrays() - 1);
        abstractData = pd->GetAbstractArray(arrayNum);
        }
      numericData = vtkDataArray::SafeDownCast(abstractData);
      stringData = vtkStringArray::SafeDownCast(abstractData);
      uStringData = vtkUnicodeStringArray::SafeDownCast(abstractData);
      };
      break;
      }
    
    // determine number of components and check input
    if( numericData )
      {
      numComp = numericData->GetNumberOfComponents();
      activeComp = 0;
      if ( this->LabeledComponent >= 0 )
        {
        activeComp = (this->LabeledComponent < numComp ? 
                      this->LabeledComponent : numComp - 1);
        numComp = 1;
        }
      }
    else if( !stringData && !uStringData )
      {
      vtkErrorMacro(<<"Need input data to render labels (3)");
      return;
      }
    
    this->LabelTree(input, sectorInfo, numericData, stringData, uStringData,
                    activeComp, numComp, viewport );
    }
  
  VTK_CREATE( vtkQImageToImageSource, qis );
  qis->SetQImage( this->QtImage );
  this->LabelTexture->SetInput( qis->GetOutput() );
  this->LabelTexture->PremultipliedAlphaOn();

  this->PlaneSource->SetOrigin( 0, 0, 0 );
  this->PlaneSource->SetPoint1( this->WindowSize[0], 0, 0 );
  this->PlaneSource->SetPoint2( 0, this->WindowSize[1], 0 );
  
  this->polyDataMapper->RenderOpaqueGeometry( viewport, actor );
}

void vtkQtTreeRingLabelMapper::LabelTree(
  vtkTree *tree, vtkDataArray *sectorInfo, vtkDataArray *numericData, vtkStringArray *stringData, vtkUnicodeStringArray *uStringData,
  int activeComp, int numComps, vtkViewport* viewport )
{
  delete this->QtImage;
  this->QtImage = new QImage( this->WindowSize[0], this->WindowSize[1], QImage::Format_ARGB32 );
  
  char string[1024];
  vtkIdType i, root = tree->GetRoot();
  if (root < 0)
    {
    vtkErrorMacro(<< "Input Tree does not have a root.");
    return;
    }
  
  vtkDataArray *TextRotationArray = tree->GetVertexData()->GetArray(this->TextRotationArrayName); 
  
//FIXME - The image surface appears to be initialized to grey.  This clears it, but seems like there should be a more appropriate way to do this...
  QPainter painter( this->QtImage );
  painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );
  painter.setCompositionMode( QPainter::CompositionMode_Clear );
  painter.drawImage( 0, 0, *this->QtImage );
  painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
//end FIXME
  
  //set text properties from LabelTextProperty
  QFont fontSpec( this->LabelTextProperty->GetFontFamilyAsString() );
  fontSpec.setStyleStrategy( QFont::PreferAntialias );
  fontSpec.setBold( this->LabelTextProperty->GetBold() );
  fontSpec.setItalic( this->LabelTextProperty->GetItalic() );
  fontSpec.setPointSize( this->LabelTextProperty->GetFontSize() );
  
  double* fc = this->LabelTextProperty->GetColor();
  
  double slimits[4], sdimDC[2], textPosDC[2];
  for( i = 0; i < tree->GetNumberOfVertices(); i++ )
    {
    sectorInfo->GetTuple(i, slimits);
    
    //check to see if the point is in the window
    if( !this->PointInWindow(slimits, sdimDC, textPosDC, viewport) )
      {
      continue;
      }
    
    //check to see if the text will fit in the sector
    this->GetVertexLabel(i, numericData, stringData, uStringData, activeComp, numComps, string);
    QString ResultString(string);
    
    double x[3];
    x[0] = textPosDC[0];
    x[1] = textPosDC[1];
    
    //Now, create the layout for the actual label...
    QString textString, testString;
    
    //Qt's coordinate system starts at the top left corner of the layout...
    // vtk has been using the text baseline as the starting point, so
    // we need to add a correction factor to account for the difference
    QFontMetricsF fontMetric( fontSpec );
    double baseline = fontMetric.ascent();
    
    //set ellipsis bounds for this piece of text
    //Note, don't use ellipsis unless at least 5 characters (w's) can be displayed...
    QString minString( "wwwww" );
    double allowedTextWidth = 0;
    if( sdimDC[0] > sdimDC[1] )
      {
      if( sdimDC[0] < fontMetric.width(minString) )
        {
        continue;
        }
      allowedTextWidth = static_cast<int>(sdimDC[0]);
      }
    else
      {
      if( sdimDC[1] < fontMetric.width(minString) )
        {
        continue;
        }
      allowedTextWidth = static_cast<int>(sdimDC[1]);
      }
//FIXME - This next step assumes no markup to the original text, which is probably a bad
//  choice, but is necessary due to Qt's current methods for handling and computing
//  rich text widths and ellipsis...
//    QTextStream(&testString) << fontMetric.elidedText( QString::fromUtf8( ResultString.c_str() ), Qt::ElideRight, allowedTextWidth );
    QTextStream(&testString) << fontMetric.elidedText( ResultString, Qt::ElideRight, allowedTextWidth );
    QTextStream(&textString) << "<span>" << testString << "</span>";
//end FIXME
    
    //check to see if the text will fit in the sector; 
    //  if not, don't draw the label...
    if( sdimDC[0] > sdimDC[1] )
      {
      if( sdimDC[1] < fontMetric.height() )
        continue;
      }
    else
      {
      if( sdimDC[0] < fontMetric.height() )
        continue;
      }
    
    double delta_x = 0., delta_y = 0.;

    switch( this->LabelTextProperty->GetJustification() )
      {
      case VTK_TEXT_LEFT: 
        break;
      case VTK_TEXT_CENTERED:
//FIXME - The width is not correct for html encodings...
        delta_x = -(fontMetric.width(testString))/ 2.;
        break;
      case VTK_TEXT_RIGHT:
//FIXME - The width is not correct for html encodings...
        delta_x = -fontMetric.width(testString); 
        break;
      }
    
    switch (this->LabelTextProperty->GetVerticalJustification())
      {
      case VTK_TEXT_TOP: 
        break;
      case VTK_TEXT_CENTERED: 
        delta_y = -(fontMetric.height())/2.;
//        delta_y = -fontMetric.ascent()/2.;
        break;
      case VTK_TEXT_BOTTOM: 
        delta_y = -baseline;
        break;
      }
    
    double h = this->WindowSize[1];
    
    //specify the clockwise text rotation angle
    double rotation = 0.;
    if( TextRotationArray )
      {
      TextRotationArray->GetTuple(i, &rotation);
      rotation *= -1.;
      }
    
    if( this->LabelTextProperty->GetShadow() )
      {
      painter.save();
      
      int shOff[2];
      this->LabelTextProperty->GetShadowOffset( shOff );
      
      painter.translate( x[0], h-x[1] );
 
      // Make sure the shadow offset is an even number of pixels in x and y.
      // This allows the shadow text to render the same way as the main text.
      QTransform t;
      t.rotate(rotation);
      t.translate(shOff[0], -shOff[1]);
      QPointF pt = t.map(QPoint(0, 0));
      painter.translate(static_cast<int>(pt.x()+0.5), static_cast<int>(pt.y()+0.5));

      painter.rotate( rotation );
      painter.translate( delta_x, delta_y );
      
      double shadowColor[3];
      this->LabelTextProperty->GetShadowColor( shadowColor );
      
      QTextDocument( textDocument );
      textDocument.setDocumentMargin(0);
      textDocument.setDefaultFont( fontSpec );
      QString shadowStyleSheet;
      QTextStream(&shadowStyleSheet) << "* { color: rgb( " << shadowColor[0]*255 << ", " << shadowColor[1]*255 << ", " << shadowColor[2]*255 << " ) }";
      textDocument.setDefaultStyleSheet( shadowStyleSheet );
      textDocument.setHtml( textString );
      textDocument.drawContents( &painter );

      painter.restore();
      }
    
    painter.save();
    painter.translate( x[0], h-x[1] );
    painter.rotate( rotation );
    painter.translate( delta_x, delta_y );
    
    QTextDocument( textDocument );
    textDocument.setDocumentMargin(0);
    textDocument.setDefaultFont( fontSpec );
    QString styleSheet;
    QTextStream(&styleSheet) << "* { color: rgb( " << fc[0]*255 << ", " << fc[1]*255 << ", " << fc[2]*255 << " ) }";
    textDocument.setDefaultStyleSheet( styleSheet );
    textDocument.setHtml( textString );
    textDocument.drawContents( &painter );

    painter.restore();
    }
  
//   QFile file( "C:/src/qtfonts.png" );
//   file.open( QIODevice::WriteOnly);
//   this->QtImage->save(&file, "PNG");
//   file.close();
  
  this->BuildTime.Modified();
}

bool vtkQtTreeRingLabelMapper::PointInWindow(double *sinfo, double *newDim, 
                                             double *textPosDC, vtkViewport *viewport)
{
  double r = (0.5*(sinfo[3] - sinfo[2])) + sinfo[2];
  double theta = sinfo[0] + (0.5*(sinfo[1]-sinfo[0]));
  double x = r * cos( vtkMath::RadiansFromDegrees( theta ) );
  double y = r * sin( vtkMath::RadiansFromDegrees( theta ) );
  
  this->VCoord->SetViewport(viewport);
  this->VCoord->SetValue(x, y, 0.);
  int *dc = VCoord->GetComputedDisplayValue(0);
  textPosDC[0] = dc[0];
  textPosDC[1] = dc[1];
  
  // Get the window extents
  vtkWindow* win = viewport->GetVTKWindow();
  int *winSize = win->GetSize();
  
  bool return_value = true;
  if( dc[0] < 0 || dc[0] > winSize[0] )
    return_value = false;
  if( dc[1] < 0 || dc[1] > winSize[1] )
    return_value = false;
  
  double height = 0., width = 0.;
  double xlc = sinfo[2] * cos( vtkMath::RadiansFromDegrees(sinfo[0]) );
  double xuc = sinfo[3] * cos( vtkMath::RadiansFromDegrees(sinfo[0]) );
  double ylc = sinfo[2] * sin( vtkMath::RadiansFromDegrees(sinfo[0]) );
  double yuc = sinfo[3] * sin( vtkMath::RadiansFromDegrees(sinfo[0]) );
  this->VCoord->SetValue(xlc, ylc, 0.);
  int *dc1 = VCoord->GetComputedDisplayValue(0);
  double dc1x = dc1[0];
  double dc1y = dc1[1];
  this->VCoord->SetValue(xuc, yuc, 0.);
  int *dc2 = VCoord->GetComputedDisplayValue(0);
  double dc2x = dc2[0];
  double dc2y = dc2[1];
  height = sqrt( ((dc2x-dc1x)*(dc2x-dc1x)) + ((dc2y-dc1y)*(dc2y-dc1y)) );
  
  double widthWC = r * vtkMath::RadiansFromDegrees( sinfo[1] - sinfo[0] );
  width = widthWC * height / sqrt( (xuc-xlc)*(xuc-xlc) + (yuc-ylc)*(yuc-ylc) );
  newDim[0] = width;
  newDim[1] = height;
  
  // We are done with the coordinate, so release the viewport
  this->VCoord->SetViewport(NULL);
  
  return return_value;
}

//----------------------------------------------------------------------------
void vtkQtTreeRingLabelMapper::SetSectorsArrayName(const char* name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
}

vtkTree *vtkQtTreeRingLabelMapper::GetInputTree() 
{
  return vtkTree::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

void vtkQtTreeRingLabelMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WindowSize: " << this->WindowSize[0] << "w x" << this->WindowSize[1] << "h\n";
  if (this->LabelTextProperty)
    {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Label Text Property: (none)\n";
    } 
  os << indent << "TextRotationArrayName: " << (this->TextRotationArrayName ? this->TextRotationArrayName : "(none)") << endl;
}

void vtkQtTreeRingLabelMapper::GetVertexLabel(
  vtkIdType vertex, vtkDataArray *numericData, vtkStringArray *stringData, vtkUnicodeStringArray* uStringData, 
  int activeComp, int numComp, char *string)
{
  char format[1024];
  double val;
  int j;
  if ( numericData )
    {
    if ( numComp == 1 )
      {
      if (numericData->GetDataType() == VTK_CHAR) 
        {
        if (strcmp(this->LabelFormat,"%c") != 0) 
          {
          vtkErrorMacro(<<"Label format must be %c to use with char");
          string[0] = '\0';
          return;
          }
        sprintf(string, this->LabelFormat, 
                static_cast<char>(numericData->GetComponent(vertex, activeComp)));
        } 
      else 
        {
        sprintf(string, this->LabelFormat, 
                numericData->GetComponent(vertex, activeComp));
        }
      }
    else
      {
      strcpy(format, "("); strcat(format, this->LabelFormat);
      for (j=0; j<(numComp-1); j++)
        {
        sprintf(string, format, numericData->GetComponent(vertex, j));
        strcpy(format,string); strcat(format,", ");
        strcat(format, this->LabelFormat);
        }
      sprintf(string, format, numericData->GetComponent(vertex, numComp-1));
      strcat(string, ")");
      }
    }
  else if (stringData)// rendering string data
    {
    if (strcmp(this->LabelFormat,"%s") != 0) 
      {
      vtkErrorMacro(<<"Label format must be %s to use with strings");
      string[0] = '\0';
      return;
      }
    sprintf(string, this->LabelFormat, 
            stringData->GetValue(vertex).c_str());
    }
  else if (uStringData)// rendering unicode string data
    {
    if (strcmp(this->LabelFormat,"%s") != 0) 
      {
      vtkErrorMacro(<<"Label format must be %s to use with strings");
      string[0] = '\0';
      return;
      }
    sprintf(string, this->LabelFormat, uStringData->GetValue(vertex).utf8_str());
    }
  else // Use the vertex id
    {
    val = static_cast<double>(vertex);
    sprintf(string, this->LabelFormat, val);
    }
}

unsigned long vtkQtTreeRingLabelMapper::GetMTime()
{
  unsigned long filterMTime = this->MTime.GetMTime();
  if( this->Renderer )
    {
    vtkRenderWindow* rw = this->Renderer->GetRenderWindow();
    if ( rw )
      {
      unsigned long renWindMTime = rw->GetMTime();
      if ( renWindMTime > filterMTime )
        {
        int* rwSize = rw->GetSize();
        if ( rwSize[0] != this->WindowSize[0] || rwSize[1] != this->WindowSize[1] )
          {
          return renWindMTime;
          }
        }
      }
    }
  return filterMTime;
}
