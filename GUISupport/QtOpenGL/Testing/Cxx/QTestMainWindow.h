/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QTestMainWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <QMainWindow>

class vtkRenderWindow;

class QTestMainWindow : public QMainWindow
{
  Q_OBJECT

  public:
    QTestMainWindow(vtkRenderWindow* renWin, int ac, char** av);

    bool regressionImageResult() const;
  public slots:
    void captureImage();
  private:
    bool             RegressionImageResult;
    vtkRenderWindow* RenderWindow;

    int    argc;
    char** argv;
};
