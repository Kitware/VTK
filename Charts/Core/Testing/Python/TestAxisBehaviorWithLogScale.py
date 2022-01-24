import itertools
import vtk
from vtk.test import Testing


class TestAxisBehaviorWithLogScale(Testing.vtkTest):

    # Use human readable names to verify correctness of created images. Defaults to False such that generated
    # images are recognized by CMake.
    HUMAN_READABLE_NAMES = False

    cfg = {
        "D": {"neg": 1, "pos": 2},                                  # Data used in plot:    Negative (and positive) ; positive (only)
        "R": {"neg": [-5, 10], "pos": [5, 10]},                     # Y axis range:         Negative (and positive) ; positive (only)
        "B": {"fix": vtk.vtkAxis.FIXED, "auto": vtk.vtkAxis.AUTO},  # Y axis behavior:      Fixed ; Auto
        "N": {"false": False, "true": True},                        # Chart's IgnoreNanInBounds value
    }

    @classmethod
    def cfg_generator(cls):
        names, values = [], []
        for cName, cValues in cls.cfg.items():
            names.append([f"{cName}{vName}" for vName in cValues.keys()])
            values.append([value for value in cValues.values()])
        for name_tuple, value_tuple in zip(itertools.product(*names), itertools.product(*values)):
            yield "_".join(name_tuple), value_tuple

    @staticmethod
    def setup(data_column, y_range, y_behavior, ignore_nan):
        renwin = vtk.vtkRenderWindow()
        renwin.SetSize(400, 400)
        renderer = vtk.vtkRenderer()
        renderer.SetBackground([1, 1, 1])
        renwin.AddRenderer(renderer)
        renwin.SetMultiSamples(0)

        chart_scene = vtk.vtkContextScene()
        chart_actor = vtk.vtkContextActor()
        chart_actor.SetScene(chart_scene)
        renderer.AddActor(chart_actor)
        chart_scene.SetRenderer(renderer)

        chart = vtk.vtkChartXY()
        chart.SetIgnoreNanInBounds(ignore_nan)
        chart_scene.AddItem(chart)

        table = vtk.vtkTable()
        arrX = vtk.vtkFloatArray()
        arrX.SetName('X Axis')
        arrN = vtk.vtkFloatArray()
        arrN.SetName('Negative Line')
        arrP = vtk.vtkFloatArray()
        arrP.SetName('Positive Line')
        table.AddColumn(arrX)
        table.AddColumn(arrN)
        table.AddColumn(arrP)

        numPoints = 101
        table.SetNumberOfRows(numPoints)
        for i in range(numPoints):
            x = i / (numPoints - 1)
            table.SetValue(i, 0, x)
            table.SetValue(i, 1, 20*x - 10)
            table.SetValue(i, 2, 10*x + 1e-6)

        points = chart.AddPlot(vtk.vtkChart.LINE)
        points.SetInputData(table, 0, data_column)
        points.SetColor(0, 0, 255, 255)
        points.SetWidth(5.0)
        if y_range is not None:
            points.GetYAxis().SetRange(*y_range)
            points.GetYAxis().SetBehavior(y_behavior)
        points.GetYAxis().SetLogScale(True)

        return renwin

    def testAxisBehaviorWithLogScale(self):
        """Test if different combinations of axis range & behavior produce the correct plot when the log scale is
        enabled for different kind of data (positive or negative) and IgnoreNanInBounds (enabled or disabled)."""
        for i, (name, cfg) in enumerate(self.cfg_generator()):
            if self.HUMAN_READABLE_NAMES:
                img_file = f"TestABWLS_{name}.png"
            else:
                suffix = f"_{i}" if i > 0 else ""
                img_file = f"TestAxisBehaviorWithLogScale{suffix}.png"
            renwin = self.setup(*cfg)
            Testing.compareImage(renwin, Testing.getAbsImagePath(img_file))
        Testing.interact()


if __name__ == "__main__":
    Testing.main([(TestAxisBehaviorWithLogScale, 'test')])
