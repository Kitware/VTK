#include <QtQml/QQmlApplicationEngine>

#include <QtQuick/QQuickWindow>

#include <QtGui/QGuiApplication>
#include <QtGui/QSurfaceFormat>

#include <QQuickVTKItem.h>
#include <QVTKRenderWindowAdapter.h>

#include <vtkActor.h>
#include <vtkBoxWidget.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkConeSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

namespace
{

struct MyVtkItem : QQuickVTKItem
{
  struct Data : vtkObject
  {
    static Data* New();
    vtkTypeMacro(Data, vtkObject);

    vtkNew<vtkBoxWidget> boxWidget;
  };

  struct Callback : vtkCommand
  {
    static Callback* New();
    void Execute(vtkObject* caller, unsigned long, void*) override
    {
      vtkNew<vtkTransform> t;
      auto widget = reinterpret_cast<vtkBoxWidget*>(caller);
      widget->GetTransform(t);
      widget->GetProp3D()->SetUserTransform(t);
    }
  };

  vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override
  {
    // Create our vtk user data
    vtkNew<Data> vtk;

    // Create a cone pipeline and add it to the view
    vtkNew<vtkConeSource> cone;
    cone->SetHeight(3.0);
    cone->SetRadius(1.0);
    cone->SetResolution(10);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkNamedColors> colors;
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(colors->GetColor3d("Bisque").GetData());

    vtkNew<vtkRenderer> renderer;
    renderer->AddActor(actor);
    renderer->ResetCamera();
    renderer->SetBackground(colors->GetColor3d("LightBlue").GetData());

    renderWindow->AddRenderer(renderer);
    renderWindow->SetMultiSamples(16);

    vtk->boxWidget->SetInteractor(renderWindow->GetInteractor());
    vtk->boxWidget->SetPlaceFactor(1.25);
    vtk->boxWidget->GetOutlineProperty()->SetColor(colors->GetColor3d("Gold").GetData());
    vtk->boxWidget->SetProp3D(actor);
    vtk->boxWidget->PlaceWidget();
    vtk->boxWidget->On();

    vtkNew<Callback> callback;
    vtk->boxWidget->AddObserver(vtkCommand::InteractionEvent, callback);

    return vtk;
  }
};
vtkStandardNewMacro(MyVtkItem::Data);
vtkStandardNewMacro(MyVtkItem::Callback);

}

int main(int argc, char* argv[])
{
  QQuickVTKItem::setGraphicsApi();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

  QGuiApplication app(argc, argv);

  qmlRegisterType<MyVtkItem>("com.vtk.example", 1, 0, "MyVtkItem");

  QQmlApplicationEngine engine;
  engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
  if (engine.rootObjects().isEmpty())
    return -1;

  return app.exec();
}
