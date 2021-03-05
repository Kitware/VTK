/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQmlVTKPlugin.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef QQmlVTKPlugin_h
#define QQmlVTKPlugin_h

// vtk includes
#include "vtkGUISupportQtQuickModule.h" // for export macro

// Qt includes
#include <QQmlExtensionPlugin>

// Forward declarations

/**
 * \class QQmlVTKPlugin
 * \brief Plugin class to expose VTK C++ to QML
 */
class VTKGUISUPPORTQTQUICK_EXPORT QQmlVTKPlugin : public QQmlExtensionPlugin
{
  Q_OBJECT
  typedef QQmlExtensionPlugin Superclass;

  Q_PLUGIN_METADATA(IID "org.kitware.VTK")

public:
  /**
   * Constructor
   */
  QQmlVTKPlugin() = default;

  /**
   * Destructor
   */
  virtual ~QQmlVTKPlugin() = default;

  /**
   * Register QML types provided by VTK
   */
  void registerTypes(const char* uri);

  /**
   * Initialize the extension using the QQmlEngine
   *
   * \sa cleanup
   */
  void initializeEngine(QQmlEngine* engine, const char* uri);

protected Q_SLOTS:
  /**
   * Destroy any singleton instances that were created during initializeEngine
   *
   * \sa initializeEngine
   */
  void cleanup();

private:
  Q_DISABLE_COPY(QQmlVTKPlugin);
};

#endif // QQmlVTKPlugin_h
