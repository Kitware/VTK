import vtk.*;

public class AxesActor extends vtkAssembly {

  private vtkRenderer ren;
  private vtkActor axisActor;
  private double textScale = 0.25;
  private double axisLength = 0.8;
  private double axisTextLength = 1.2;
  private vtkActor2D axesTextActor = new vtkActor2D();
  private vtkLabeledDataMapper ldm = new vtkLabeledDataMapper();

  public AxesActor(vtkRenderer _ren) {
    super();
    ren = _ren;
    createAxes();
  }

  public void createAxes() {
    vtkAxes axes = new vtkAxes();
    axes.SetOrigin(0, 0, 0);
    axes.SetScaleFactor(axisLength);

    vtkCharArray ca = new vtkCharArray();
    ca.SetName("labels");
    ca.SetArray("XYZ", 3, 1);

    vtkPoints pts = new vtkPoints();
    pts.InsertPoint(0, axisLength, 0.0, 0.0); 
    pts.InsertPoint(1, 0.0, axisLength, 0.0);
    pts.InsertPoint(2, 0.0, 0.0, axisLength);

    vtkPolyData ptset = new vtkPolyData();
    ptset.SetPoints(pts);
    int index = ptset.GetPointData().AddArray(ca);
    ptset.Update();

    ldm.SetInput(ptset);
    ldm.SetLabelFormat("%c");
    ldm.SetFontSize(32);
    ldm.SetLabelModeToLabelFieldData();
    ldm.SetFieldDataArray(index);
    ldm.GetLabelTextProperty().SetColor(1.0, 1.0, 1.0);

    vtkTubeFilter tube = new vtkTubeFilter();
    tube.SetInput(axes.GetOutput());
    tube.SetRadius(0.05);
    tube.SetNumberOfSides(8);

    vtkPolyDataMapper tubeMapper = new vtkPolyDataMapper();
    tubeMapper.SetInput(tube.GetOutput());

    vtkActor tubeActor = new vtkActor();
    tubeActor.SetMapper(tubeMapper);
    tubeActor.PickableOff();

    int coneRes = 12;
    double coneScale = 0.3;

    //--- x-Cone
    vtkConeSource xcone = new vtkConeSource();
    xcone.SetResolution(coneRes);
    vtkPolyDataMapper xconeMapper = new vtkPolyDataMapper();
    xconeMapper.SetInput(xcone.GetOutput());
    vtkActor xconeActor = new vtkActor();
    xconeActor.SetMapper(xconeMapper);
    xconeActor.GetProperty().SetColor(1,0,0);
    xconeActor.SetScale(coneScale, coneScale, coneScale);
    xconeActor.SetPosition(axisLength, 0.0, 0.0);

    //--- y-Cone
    vtkConeSource ycone = new vtkConeSource();
    ycone.SetResolution(coneRes);
    vtkPolyDataMapper yconeMapper = new vtkPolyDataMapper();
    yconeMapper.SetInput(ycone.GetOutput());
    vtkActor yconeActor = new vtkActor();
    yconeActor.SetMapper(yconeMapper);
    yconeActor.GetProperty().SetColor(1,1,0);
    yconeActor.RotateZ(90);
    yconeActor.SetScale(coneScale, coneScale, coneScale);
    yconeActor.SetPosition(0.0, axisLength, 0.0);

    //--- z-Cone
    vtkConeSource zcone = new vtkConeSource();
    zcone.SetResolution(coneRes);
    vtkPolyDataMapper zconeMapper = new vtkPolyDataMapper();
    zconeMapper.SetInput(zcone.GetOutput());
    vtkActor zconeActor = new vtkActor();
    zconeActor.SetMapper(zconeMapper);
    zconeActor.GetProperty().SetColor(0,1,0);
    zconeActor.RotateY(-90);
    zconeActor.SetScale(coneScale, coneScale, coneScale);
    zconeActor.SetPosition(0.0, 0.0, axisLength);

    axesTextActor.SetMapper(ldm);
    ren.AddActor2D(axesTextActor);
  
    this.AddPart(tubeActor);
    this.AddPart(xconeActor);
    this.AddPart(yconeActor);
    this.AddPart(zconeActor);

    ren.AddActor(this);

  }

  public void setAxesVisibility(boolean ison) {
    this.SetVisibility(ison ? 1 : 0);
    axesTextActor.SetVisibility(ison ? 1 : 0);
  }

}
