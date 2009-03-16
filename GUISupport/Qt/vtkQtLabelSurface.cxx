/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkQtLabelSurface.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQtLabelSurface.h"

#include "vtkQImageToImageSource.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStringArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTextProperty.h"
#include "vtkTextureMapToPlane.h"
#include "vtkUnsignedCharArray.h"

#include "vtkQtInitialization.h"
#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QTextDocument>
#include <QTextOption>
#include <QTextStream>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkQtLabelSurface, "1.2");
vtkStandardNewMacro(vtkQtLabelSurface);
vtkCxxSetObjectMacro(vtkQtLabelSurface,LabelTextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Creates a new label mapper
vtkQtLabelSurface::vtkQtLabelSurface()
{
  VTK_CREATE( vtkQtInitialization, initApp );
  
  this->Input = NULL;
  this->Renderer = NULL;
  
  this->LabeledComponent = (-1);
  this->FieldDataArray = 0;
  this->FieldDataName = NULL;
  
  this->SetTextRotationArrayName( "TextRotation" );
  
  this->NumberOfLabels = 0;
  this->NumberOfLabelsAllocated = 0;
  
  this->LabelPositions = 0;
  this->AllocateLabels(50);
  
  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetFontSize(12);
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();
  
  this->DataExtent[0] = 0;
  this->DataExtent[1] = 0;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = 0;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = 0;
  
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
vtkQtLabelSurface::~vtkQtLabelSurface()
{ 
  this->SetRenderer(NULL);
  
  delete [] this->LabelPositions;
  
  this->SetLabelTextProperty(NULL);
  this->SetFieldDataName(NULL);  
  
  this->SetTextRotationArrayName(0);
}

//----------------------------------------------------------------------------
void vtkQtLabelSurface::AllocateLabels(int numLabels)
{
  if (numLabels > this->NumberOfLabelsAllocated)
    {
    int i;
    // delete old stuff
    delete [] this->LabelPositions;
    this->LabelPositions = 0;
    
    this->NumberOfLabelsAllocated = numLabels;
    
    // Allocate and initialize new stuff
    this->LabelPositions = new double[this->NumberOfLabelsAllocated*3];
    for (i=0; i<this->NumberOfLabelsAllocated; i++)
      {
      this->LabelPositions[3*i] = 0;
      this->LabelPositions[3*i+1] = 0;
      this->LabelPositions[3*i+2] = 0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtLabelSurface::SetInput(vtkDataObject* input)
{
  if (input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(0, 0);
    }
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkQtLabelSurface::GetInput()
{
  return vtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

int vtkQtLabelSurface::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  
  if ( this->Renderer == NULL )
    {
    vtkErrorMacro(<<"Renderer must be set");
    return 0;
    }
  if( this->Renderer->GetRenderWindow() == 0 )
    {
    return 1;
    }
  
  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double w = size[0]-1;
  double h = size[1]-1;
  
  this->DataExtent[1] = w;
  this->DataExtent[3] = h;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->DataExtent,6);
  return 1;
}

//----------------------------------------------------------------------------
int vtkQtLabelSurface::RequestData( vtkInformation *vtkNotUsed(request),
                                    vtkInformationVector **vtkNotUsed(inputVector), 
                                    vtkInformationVector *outputVector)
{ 
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *outInfo2 = outputVector->GetInformationObject(1);
  vtkImageData *output = vtkImageData::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT() ) );
  vtkPolyData *output2 = vtkPolyData::SafeDownCast( outInfo2->Get(vtkDataObject::DATA_OBJECT() ) );
  
  if ( this->Renderer == NULL )
    {
    vtkErrorMacro(<<"Renderer must be set");
    return 0;
    }
  if( this->Renderer->GetRenderWindow() == 0 )
    {
    return 1;
    }
  
  int *size = this->Renderer->GetRenderWindow()->GetSize();
  int width = size[0];
  int height = size[1];
  
  QImage surface( width, height, QImage::Format_ARGB32 );
  
//FIXME - The image surface appears to be initialized to grey.  This clears it, but seems like there should be a more appropriate way to do this...
  QPainter painter( &surface );
  painter.setCompositionMode( QPainter::CompositionMode_Clear );
  painter.drawImage( 0,0,surface );
  painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
//end FIXME
  
  vtkTextProperty *tprop = this->LabelTextProperty;
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render labels");
    return 0;
    }
  
  vtkDataObject *inputDO = this->GetInputDataObject(0, 0);
  if ( ! inputDO )
    {
    this->NumberOfLabels = 0;
    vtkErrorMacro(<<"Need input data to render labels (2)");
    return 0;
    }
  
  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
       inputDO->GetMTime() > this->BuildTime ||
       tprop->GetMTime() > this->BuildTime)
    {
    this->BuildLabels( &painter );
    }
  
  VTK_CREATE( vtkQImageToImageSource, cis );
  cis->SetQImage( &surface );
  cis->Update();
  output->ShallowCopy( cis->GetOutput() );
  
  //Now create the output2 polydata that will be textured by 
  // the image data contained in output
  VTK_CREATE( vtkPlaneSource, planeSource );
  planeSource->SetOrigin( 0, 0, 0 );
  planeSource->SetPoint1( width, 0, 0 );
  planeSource->SetPoint2( 0, height, 0 );
  
  VTK_CREATE( vtkTextureMapToPlane, tmap );
  tmap->SetInputConnection( planeSource->GetOutputPort() );
  tmap->AutomaticPlaneGenerationOn();
  tmap->SetSRange( 0., 1. );
  tmap->SetTRange( 0., 1. );
  tmap->Update();
  
  output2->ShallowCopy( tmap->GetOutput() );
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkQtLabelSurface::BuildLabels( QPainter* painter )
{
  vtkDebugMacro(<<"Rebuilding labels");
  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(inputDO);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(inputDO);
  if (ds)
    {
    this->AllocateLabels(ds->GetNumberOfPoints());
    this->NumberOfLabels = 0;
    this->BuildLabelsInternal( ds, painter );
    }
  else if (cd)
    {
    this->AllocateLabels(cd->GetNumberOfPoints());
    this->NumberOfLabels = 0;
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
         iter->GoToNextItem())
      {
      ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
        {
        this->BuildLabelsInternal( ds, painter );
        }
      }
    iter->Delete();
    }
  else
    {
    vtkErrorMacro("Unsupported data type: " << inputDO->GetClassName());
    }
  
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkQtLabelSurface::BuildLabelsInternal(vtkDataSet* input, QPainter* painter )
{
  int i, numComp = 0;
  vtkAbstractArray *abstractData = NULL;
  vtkDataArray *numericData = NULL;
  vtkStringArray *stringData = NULL;
  
  vtkPointData *pd = input->GetPointData();
  // figure out what to label, and if we can label it
  int arrayNum;
  if (this->FieldDataName != NULL)
    {
    vtkDebugMacro(<<"Labeling field data array " << this->FieldDataName);
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
  
  // determine number of components and check input
  if ( stringData )
    {
    numComp = stringData->GetNumberOfComponents();
    }
  else
    {
    if (this->FieldDataName)
      {
      vtkWarningMacro(<< "Could not find label array ("
                      << this->FieldDataName << ") "
                      << "in input.");
      }
    else 
      {
      vtkWarningMacro(<< "Could not find label array ("
                      << "index " << this->FieldDataArray << ") "
                      << "in input.");
      }
    
    return;
    }
  
  vtkDataArray *TextRotationArray = pd->GetArray(this->TextRotationArrayName); 
  
  int numCurLabels = input->GetNumberOfPoints(); 
  if (this->NumberOfLabelsAllocated < (this->NumberOfLabels + numCurLabels))
    {
    vtkErrorMacro(
      "Number of labels must be allocated before this method is called.");
    return;
    }
  
  //we need the window height to set the text correctly...  
  int *size = this->Renderer->GetRenderWindow()->GetSize();
//  double w = size[0]-1;
  double h = size[1]-1;
  
  //set text properties from LabelTextProperty
  QFont fontSpec( this->LabelTextProperty->GetFontFamilyAsString() );
  fontSpec.setBold( this->LabelTextProperty->GetBold() );
  fontSpec.setItalic( this->LabelTextProperty->GetItalic() );
  fontSpec.setPointSize( this->LabelTextProperty->GetFontSize() );
  
  double* fc = this->LabelTextProperty->GetColor();
  
  for (i=0; i < numCurLabels; i++)
    {
    vtkStdString ResultString = stringData->GetValue(i);
    
    double x[3];
    input->GetPoint(i, x);
    this->LabelPositions[3*(i+this->NumberOfLabels)] = x[0];
    this->LabelPositions[3*(i+this->NumberOfLabels)+1] = x[1];
    this->LabelPositions[3*(i+this->NumberOfLabels)+2] = x[2];
    
    
//FIXME - This ensures all label colorings are consistent.  Is this appropriate?
//    QString textString = QString::fromUtf8( ResultString.c_str() );
    QString textString, testString;
    QTextStream(&textString) << "<span>" << QString::fromUtf8( ResultString.c_str() ) << "</span>";
    QTextStream(&testString) << QString::fromUtf8( ResultString.c_str() );
//end FIXME
    
    //Qt's coordinate system starts at the top left corner of the layout...
    // vtk has been using the text baseline as the starting point, so
    // we need to add a correction factor to account for the difference
    QFontMetrics fontMetric( fontSpec );
    int baseline = fontMetric.ascent();
    
    double delta_x = 0., delta_y = 0.;
    switch( this->LabelTextProperty->GetJustification() )
      {
      case VTK_TEXT_LEFT: 
        break;
      case VTK_TEXT_CENTERED:
//FIXME - The width is not correct for html encodings...
        delta_x = -fontMetric.width(testString)/ 2.;
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
        delta_y = -fontMetric.height()/2.;
        break;
      case VTK_TEXT_BOTTOM: 
        delta_y = -baseline;
//        delta_y = -fontMetric.height();
        break;
      }
    
    //specify the clockwise text rotation angle
    double rotation = 0.;
    if( TextRotationArray )
      {
      TextRotationArray->GetTuple(i, &rotation);
      rotation *= -1.;
      }
//    radian_rotation = 45.;
    
    if( this->LabelTextProperty->GetShadow() )
      {
      painter->save();
      
      int shOff[2];
      this->LabelTextProperty->GetShadowOffset( shOff );
      
      painter->translate( x[0], h-x[1] );
      painter->rotate( rotation );
      painter->translate( delta_x, delta_y );
      painter->translate( shOff[0], -shOff[1] );
      
      double shadowColor[3];
      this->LabelTextProperty->GetShadowColor( shadowColor );
      
      QTextDocument( textDocument );
      textDocument.setDefaultFont( fontSpec );
      QString shadowStyleSheet;
      QTextStream(&shadowStyleSheet) << "* { color: rgb( " << shadowColor[0]*255 << ", " << shadowColor[1]*255 << ", " << shadowColor[2]*255 << " ) }";
      textDocument.setDefaultStyleSheet( shadowStyleSheet );
      textDocument.setHtml( textString );
      textDocument.drawContents( painter );
      
      painter->restore();
      }
    
    painter->save();
    painter->translate( x[0], h-x[1] );
    painter->rotate( rotation );
    painter->translate( delta_x, delta_y );
    
    QTextDocument( textDocument );
    textDocument.setDefaultFont( fontSpec );
    QString styleSheet;
    QTextStream(&styleSheet) << "* { color: rgb( " << fc[0]*255 << ", " << fc[1]*255 << ", " << fc[2]*255 << " ) }";
    textDocument.setDefaultStyleSheet( styleSheet );
    textDocument.setHtml( textString );
    textDocument.drawContents( painter );
    
    painter->restore();
    }
  
  this->NumberOfLabels += numCurLabels;
}

//----------------------------------------------------------------------------
int vtkQtLabelSurface::FillInputPortInformation(
  int vtkNotUsed( port ), vtkInformation* info)
{
  // Can handle composite datasets.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkQtLabelSurface::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    }
  else if (port == 1)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkQtLabelSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  if (this->LabelTextProperty)
    {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Label Text Property: (none)\n";
    }

  os << indent << "Labeled Component: ";
  if ( this->LabeledComponent < 0 )
    {
    os << "(All Components)\n";
    }
  else
    {
    os << this->LabeledComponent << "\n";
    }

  os << indent << "Field Data Array: " << this->FieldDataArray << "\n";
  os << indent << "Field Data Name: " << (this->FieldDataName ? this->FieldDataName : "Null") << "\n";
}

// ----------------------------------------------------------------------
void vtkQtLabelSurface::SetFieldDataArray(int arrayIndex)
{
  if (this->FieldDataName)
    {
    delete [] this->FieldDataName;
    this->FieldDataName = NULL;
    }

  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting FieldDataArray to " << arrayIndex ); 

  if (this->FieldDataArray != (arrayIndex < 0 ? 0 : 
                               (arrayIndex > VTK_LARGE_INTEGER ? VTK_LARGE_INTEGER : arrayIndex)))
    {
    this->FieldDataArray = ( arrayIndex < 0 ? 0 : 
                             (arrayIndex > VTK_LARGE_INTEGER ? VTK_LARGE_INTEGER : arrayIndex ));
    this->Modified();
    }
}

// ----------------------------------------------------------------------
void vtkQtLabelSurface::SetFieldDataName(const char *arrayName)
{
  vtkDebugMacro(<< this->GetClassName() 
                << " (" << this << "): setting " << "FieldDataName" 
                << " to " << (arrayName?arrayName:"(null)") ); 

  if ( this->FieldDataName == NULL && arrayName == NULL) { return; } 
  if ( this->FieldDataName && arrayName && (!strcmp(this->FieldDataName,arrayName))) { return;} 
  if (this->FieldDataName) { delete [] this->FieldDataName; } 
  if (arrayName) 
    { 
    this->FieldDataName = new char[strlen(arrayName)+1]; 
    strcpy(this->FieldDataName,arrayName); 
    } 
   else 
    { 
    this->FieldDataName = NULL; 
    } 
  this->Modified(); 
}
