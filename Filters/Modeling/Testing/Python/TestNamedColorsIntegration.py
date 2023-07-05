#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersModeling import vtkBandedPolyDataContourFilter
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class NamedColorsIntegration(vtkmodules.test.Testing.vtkTest):

    def test(self):
        '''
          Create a cone, contour it using the banded contour filter and
              color it with the primary additive and subtractive colors.
        '''
        namedColors = vtkNamedColors()
        # Test printing of the object
        # Uncomment if desired
        #print namedColors

        # How to get a list of colors
        colors = namedColors.GetColorNames()
        colors = colors.split('\n')
        # Uncomment if desired
        #print 'Number of colors:', len(colors)
        #print colors

        # How to get a list of a list of synonyms.
        syn = namedColors.GetSynonyms()
        syn = syn.split('\n\n')
        synonyms = []
        for ele in syn:
            synonyms.append(ele.split('\n'))
        # Uncomment if desired
        #print 'Number of synonyms:', len(synonyms)
        #print synonyms

        # Create a cone
        coneSource = vtkConeSource()
        coneSource.SetCenter(0.0, 0.0, 0.0)
        coneSource.SetRadius(5.0)
        coneSource.SetHeight(10)
        coneSource.SetDirection(0,1,0)
        coneSource.Update();

        bounds = [1.0,-1.0,1.0,-1.0,1.0,-1.0]
        coneSource.GetOutput().GetBounds(bounds)

        elevation = vtkElevationFilter()
        elevation.SetInputConnection(coneSource.GetOutputPort());
        elevation.SetLowPoint(0,bounds[2],0);
        elevation.SetHighPoint(0,bounds[3],0);

        bcf = vtkBandedPolyDataContourFilter()
        bcf.SetInputConnection(elevation.GetOutputPort());
        bcf.SetScalarModeToValue();
        bcf.GenerateContourEdgesOn();
        bcf.GenerateValues(7,elevation.GetScalarRange());

        # Build a simple lookup table of
        # primary additive and subtractive colors.
        lut = vtkLookupTable()
        lut.SetNumberOfTableValues(7);
        rgba = [0.0,0.0,0.0,1.0]
        # Test setting and getting a color here.
        namedColors.GetColor("Red",rgba);
        namedColors.SetColor("My Red",rgba)
        namedColors.GetColor("My Red",rgba);
        lut.SetTableValue(0,rgba);
        namedColors.GetColor("DarkGreen",rgba);
        lut.SetTableValue(1,rgba);
        namedColors.GetColor("Blue",rgba);
        lut.SetTableValue(2,rgba);
        namedColors.GetColor("Cyan",rgba);
        lut.SetTableValue(3,rgba);
        namedColors.GetColor("Magenta",rgba);
        lut.SetTableValue(4,rgba);
        namedColors.GetColor("Yellow",rgba);
        lut.SetTableValue(5,rgba);
        namedColors.GetColor("White",rgba);
        lut.SetTableValue(6,rgba);
        lut.SetTableRange(elevation.GetScalarRange());
        lut.Build();

        mapper = vtkPolyDataMapper()
        mapper.SetInputConnection(bcf.GetOutputPort());
        mapper.SetLookupTable(lut);
        mapper.SetScalarModeToUseCellData();

        contourLineMapper = vtkPolyDataMapper()
        contourLineMapper.SetInputData(bcf.GetContourEdgesOutput());
        contourLineMapper.SetScalarRange(elevation.GetScalarRange());
        contourLineMapper.SetResolveCoincidentTopologyToPolygonOffset();

        actor = vtkActor()
        actor.SetMapper(mapper);

        contourLineActor = vtkActor()
        contourLineActor.SetMapper(contourLineMapper);
        rgb = [0.0,0.0,0.0]
        namedColors.GetColorRGB("black",rgb)
        contourLineActor.GetProperty().SetColor(rgb);

        renderer = vtkRenderer()
        renderWindow = vtkRenderWindow()
        renderWindow.AddRenderer(renderer);
        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renderWindow);

        renderer.AddActor(actor);
        renderer.AddActor(contourLineActor);
        namedColors.GetColorRGB("SteelBlue",rgb)
        renderer.SetBackground(rgb);

        renderWindow.Render();
        img_file = "TestNamedColorsIntegration.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(),vtkmodules.test.Testing.getAbsImagePath(img_file),threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(NamedColorsIntegration, 'test')])
