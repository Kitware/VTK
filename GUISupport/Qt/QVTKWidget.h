/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/

// .NAME QVTKWidget - Display a vtkRenderWindow in a Qt's QWidget.
// .SECTION Description
// QVTKWidget provides a way to display VTK data in a Qt widget.

#ifndef Q_VTK_WIDGET_H
#define Q_VTK_WIDGET_H

#include <qwidget.h>
class QPaintEngine;

class vtkRenderWindow;
class QVTKInteractor;
#include <vtkRenderWindowInteractor.h>
#include <vtkCommand.h>
#include <vtkConfigure.h>
class vtkUnsignedCharArray;

#if defined(Q_WS_MAC) && QT_VERSION >= 0x040000
#include <Carbon/Carbon.h>    // Event handling for dirty region
#endif

#if defined(WIN32) && defined(VTK_BUILD_SHARED_LIBS)
#if defined(QVTK_EXPORTS) || defined(QVTKWidgetPlugin_EXPORTS)
#define QVTK_EXPORT __declspec( dllexport )
#else
#define QVTK_EXPORT __declspec( dllimport ) 
#endif
#else
#define QVTK_EXPORT
#endif

//! QVTKWidget displays a VTK window in a Qt window.
class QVTK_EXPORT QVTKWidget : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(bool automaticImageCacheEnabled
             READ isAutomaticImageCacheEnabled
             WRITE setAutomaticImageCacheEnabled)
    Q_PROPERTY(double maxRenderRateForImageCache
               READ maxRenderRateForImageCache
               WRITE setMaxRenderRateForImageCache)

    public:
#if QT_VERSION < 0x040000
  //! constructor for Qt 3
  QVTKWidget(QWidget* parent = NULL, const char* name = NULL, Qt::WFlags f = 0);
#else
  //! constructor for Qt 4
  QVTKWidget(QWidget* parent = NULL, Qt::WFlags f = 0);
#endif
  //! destructor
  virtual ~QVTKWidget();

  // Description:
  // Set the vtk render window, if you wish to use your own vtkRenderWindow
  virtual void SetRenderWindow(vtkRenderWindow*);
    
  // Description:
  // Get the vtk render window.
  virtual vtkRenderWindow* GetRenderWindow();
    
  // Description:
  // Get the Qt/vtk interactor that was either created by default or set by the user
  virtual QVTKInteractor* GetInteractor();
    
  // Description:
  // Enum for additional event types supported.
  // These events can be picked up by command observers on the interactor
  enum vtkCustomEvents
  {
    ContextMenuEvent = vtkCommand::UserEvent + 100,
    DragEnterEvent,
    DragMoveEvent,
    DragLeaveEvent,
    DropEvent
  };

  // Description:
  // Enables/disables automatic image caching.  If disabled (the default),
  // QVTKWidget will not call saveImageToCache() on its own.
  virtual void setAutomaticImageCacheEnabled(bool flag);
  virtual bool isAutomaticImageCacheEnabled() const;

  // Description:
  // If automatic image caching is enabled, then the image will be cached
  // after every render with a DesiredUpdateRate that is greater than
  // this parameter.  By default, the vtkRenderWindowInteractor will
  // change the desired render rate depending on the user's
  // interactions. (See vtkRenderWindow::DesiredUpdateRate,
  // vtkRenderWindowInteractor::DesiredUpdateRate and
  // vtkRenderWindowInteractor::StillUpdateRate for more details.)
  virtual void setMaxRenderRateForImageCache(double rate);
  virtual double maxRenderRateForImageCache() const;

  // Description:
  // Returns the current image in the window.  If the image cache is up
  // to date, that is returned to avoid grabbing other windows.
  virtual vtkUnsignedCharArray* cachedImage();
    
#if QT_VERSION < 0x040000
  // Description:
  // Handle reparenting of this widget in Qt 3.x
  virtual void reparent(QWidget* parent, Qt::WFlags f, const QPoint& p, bool showit);
#endif
    
  // Description:
  // Handle showing of the Widget
  virtual void showEvent(QShowEvent*);

  virtual QPaintEngine* paintEngine() const;

  signals:
  // Description:
  // This signal will be emitted whenever a mouse event occurs
  // within the QVTK window
  void mouseEvent(QMouseEvent* event);

  // Description:
  // This signal will be emitted whenever the cached image goes from clean
  // to dirty.
  void cachedImageDirty();

  // Description:
  // This signal will be emitted whenever the cached image is refreshed.
  void cachedImageClean();


public slots:
  // Description:
  // This will mark the cached image as dirty.  This slot is automatically
  // invoked whenever the render window has a render event or the widget is
  // resized.  Your application should invoke this slot whenever the image in
  // the render window is changed by some other means.  If the image goes
  // from clean to dirty, the cachedImageDirty() signal is emitted.
  void markCachedImageAsDirty();

  // Description:
  // If the cached image is dirty, it is updated with the current image in
  // the render window and the cachedImageClean() signal is emitted.
  void saveImageToCache();

protected:
  // overloaded resize handler
  virtual void resizeEvent(QResizeEvent* event);
  // overloaded move handler
  virtual void moveEvent(QMoveEvent* event);
  // overloaded paint handler
  virtual void paintEvent(QPaintEvent* event);

  // overloaded mouse press handler
  virtual void mousePressEvent(QMouseEvent* event);
  // overloaded mouse move handler
  virtual void mouseMoveEvent(QMouseEvent* event);
  // overloaded mouse release handler
  virtual void mouseReleaseEvent(QMouseEvent* event);
  // overloaded key press handler
  virtual void keyPressEvent(QKeyEvent* event);
  // overloaded key release handler
  virtual void keyReleaseEvent(QKeyEvent* event);
  // overloaded enter event
  virtual void enterEvent(QEvent*);
  // overloaded leave event
  virtual void leaveEvent(QEvent*);
#ifndef QT_NO_WHEELEVENT
  // overload wheel mouse event
  virtual void wheelEvent(QWheelEvent*);
#endif
  // overload focus event
  virtual void focusInEvent(QFocusEvent*);
  // overload focus event
  virtual void focusOutEvent(QFocusEvent*);
  // overload Qt's event() to capture more keys
  bool event( QEvent* e );
    
  // overload context menu event
  virtual void contextMenuEvent(QContextMenuEvent*);
  // overload drag enter event
  virtual void dragEnterEvent(QDragEnterEvent*);
  // overload drag move event
  virtual void dragMoveEvent(QDragMoveEvent*);
  // overload drag leave event
  virtual void dragLeaveEvent(QDragLeaveEvent*);
  // overload drop event
  virtual void dropEvent(QDropEvent*);
     
  // the vtk render window
  vtkRenderWindow* mRenWin;
    
  // set up an X11 window based on a visual and colormap
  // that VTK chooses
  void x11_setup_window();

#if defined(Q_WS_MAC) && QT_VERSION < 0x040000
  void macFixRect();
  virtual void setRegionDirty(bool);
  virtual void macWidgetChangedWindow();
#endif    

#if defined(Q_WS_MAC) && QT_VERSION >= 0x040000
  EventHandlerUPP DirtyRegionHandlerUPP;
  EventHandlerRef DirtyRegionHandler;
  static OSStatus DirtyRegionProcessor(EventHandlerCallRef er, EventRef event, void*);
#endif

private slots:
void internalMacFixRect();

protected:
    
  vtkUnsignedCharArray* mCachedImage;
  bool cachedImageCleanFlag;
  bool automaticImageCache;
  double maxImageCacheRenderRate;
  
private:
  //! unimplemented operator=
  QVTKWidget const& operator=(QVTKWidget const&);
  //! unimplemented copy
  QVTKWidget(const QVTKWidget&);

};

class QVTKInteractorInternal;

// .NAME QVTKInteractor - An interactor for the QVTKWidget.
// .SECTION Description
// QVTKInteractor is an interactor for a QVTKWiget.

class QVTK_EXPORT QVTKInteractor : public QObject, public vtkRenderWindowInteractor
{
  Q_OBJECT
public:
  static QVTKInteractor* New();
  vtkTypeMacro(QVTKInteractor,vtkRenderWindowInteractor);

  // Description:
  // Overloaded terminiate app, which does nothing in Qt.
  // Use qApp->exit() instead.
  virtual void TerminateApp();
  
  // Description:
  // Overloaded start method does nothing.
  // Use qApp->exec() instead.
  virtual void Start();
 
public slots:
// timer event slot
virtual void TimerEvent(int timerId);

protected:
  // constructor
  QVTKInteractor();
  // destructor
  ~QVTKInteractor();

  // create a Qt Timer
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  // destroy a Qt Timer
  virtual int InternalDestroyTimer(int platformTimerId);

private:

  QVTKInteractorInternal* Internal;

  // unimplemented copy
  QVTKInteractor(const QVTKInteractor&);
  // unimplemented operator=
  void operator=(const QVTKInteractor&);

};


#endif


