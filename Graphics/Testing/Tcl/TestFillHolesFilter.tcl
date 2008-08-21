package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create some data, remove some polygons
vtkPolyData pd
vtkPoints pts
vtkCellArray polys
pd SetPoints pts
pd SetPolys polys


set xRes 10
set yRes 10
set xPtsRes [expr $xRes + 1]
set yPtsRes [expr $yRes + 1]

#insert points
for {set j 0} {$j < $yPtsRes} {incr j} {
    for {set i 0} {$i < $xPtsRes} {incr i} {
        pts InsertNextPoint $i $j 0.0
    }
}

#insert cells
for {set j 1} {$j <= $yRes} {incr j} {
    for {set i 1} {$i <= $xRes} {incr i} {
        set cellId [expr $i -1 + $yRes*($j-1)]
        if { $cellId != 48 &&\
               $cellId != 12 && $cellId != 13 && $cellId != 23 &&\
               $cellId != 60 &&\
               $cellId != 83 && $cellId != 72 &&\
               $cellId != 76 && $cellId != 77 && $cellId != 78 && $cellId != 87  } {
            polys InsertNextCell 4
            polys InsertCellPoint [expr $i - 1 + (($j-1)*$yPtsRes)]
            polys InsertCellPoint [expr $i     + (($j-1)*$yPtsRes)]
            polys InsertCellPoint [expr $i     + ($j*$yPtsRes)]
            polys InsertCellPoint [expr $i - 1 + ($j*$yPtsRes)]
        }
    }
}

# Fill the holes
vtkFillHolesFilter fill
fill SetInput pd
fill SetHoleSize 20.0

# Mapping and actor
vtkPolyDataMapper map
#    map SetInput pd
    map SetInputConnection [fill GetOutputPort]
vtkActor actor
    actor SetMapper map
    [actor GetProperty] SetColor 1 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 300 300

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
