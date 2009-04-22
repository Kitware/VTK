/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtImages.cxx

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

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleImage.h"
#include "vtkQtInitialization.h"
#include "vtkQImageToImageSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <QApplication>
#include <QIODevice>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QTextDocument>

int TestQtImages( int argc, char* argv[] )
{
  VTK_CREATE( vtkQtInitialization, initApp );

//  QImage image2( "C:/src/testcurvedlabels.png" );
  QImage image( "C:/src/testlabels.png" );

  QPainter painter( &image );
//   QPainter painter( &image2 );
//   painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
//   painter.drawImage(0,0,image);

  painter.setPen(QPen(Qt::black, 4, Qt::SolidLine, Qt::RoundCap));
  painter.drawPoint( 300,300 );

  QString textString = QString::fromUtf8( "\302\245 \342\202\254 <sub>Ams</sub><b>ter</b>dam" ); 

//#1
//  painter.drawText( 300, 300, 300, 25, Qt::AlignLeft | Qt::AlignBottom, textString );

  painter.save();
  painter.translate( 300, 300 );
  painter.rotate( 45 );

  QFont textFont;
  textFont.setPointSize(10);
  textFont.setFamily("Arial");

  QTextDocument textDocument;
  textDocument.setHtml( textString );
  textDocument.setDefaultFont( textFont );
  textDocument.drawContents( &painter );

  painter.restore();

//   QFile file( "C:/src/qtcurvedlabels.png" );
//   file.open( QIODevice::WriteOnly);
//   image2.save(&file, "PNG");
//   file.close();

  VTK_CREATE(vtkQImageToImageSource, cis);
  cis->SetQImage( &image );

  VTK_CREATE(vtkImageActor, imgactor);
  imgactor->SetInput(cis->GetOutput());

  vtkImageData* id = imgactor->GetInput();
  VTK_CREATE(vtkRenderer, renderer);
  renderer->SetBackground(1,1,1);
  id->UpdateInformation();

  renderer->AddActor(imgactor);
  VTK_CREATE(vtkRenderWindow, window);
  window->AddRenderer(renderer);
  VTK_CREATE(vtkInteractorStyleImage, imageStyle);
  VTK_CREATE(vtkRenderWindowInteractor, interactor);
  interactor->SetInteractorStyle(imageStyle);
  window->SetSize(600,600);
  window->SetInteractor(interactor);
  window->Render();

  int retVal = vtkRegressionTestImage(window);
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return 0;
}
