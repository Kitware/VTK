## Create a little app for loading and viewing stereo-lithography files
##
source TkInteractor.tcl

# Create gui
frame .mbar -relief raised -bd 2
pack .mbar -side top -fill x

menubutton .mbar.file -text File -underline 0\
    -menu .mbar.file.open
pack .mbar.file -side left

menu .mbar.file.open
.mbar.file.open add command -label Open -command OpenFile
.mbar.file.open add command -label Exit -command exit

vtkTkRenderWidget .window -width 300 -height 300 
    BindTkRenderWidget .window
pack .window -side top -anchor nw -padx 3 -pady 3 -fill both -expand 1

# Procedure opens file and resets view
proc OpenFile {} {
    global ren renWin reader

    set types {
        {{BYU}                                  {.g}          }
        {{Stereo-Lithography}                   {.stl}        }
        {{Visualization Toolkit (polygonal)}    {.stl}        }
        {{All Files }                           *             }
    }
    set filename [tk_getOpenFile -filetypes $types]
    if { $filename != "" } {
        $ren RemoveActors bannerActor
        $ren RemoveActors actor
        if { [string match *.g $filename] } {
            set reader byu
            byu SetGeometryFilename $filename
        } elseif { [string match *.stl $filename] } {
            set reader stl
            stl SetFilename $filename
        } elseif { [string match *.vtk $filename] } {
            set reader vtk
            vtk SetFilename $filename
        } else {
            puts "Can't read this file"
            return
        }
        
        mapper SetInput [$reader GetOutput]
        $reader Update
        if { [[$reader GetOutput] GetNumberOfCells] <= 0 } {
            $ren AddActors bannerActor
        } else {
            $ren AddActors actor
        }
        $ren ResetCamera
        $renWin Render
    }
}

# Create pipeline
set reader stl
vtkSTLReader stl
vtkBYUReader byu
vtkPolyReader vtk
vtkPolyMapper   mapper
vtkActor actor
    actor SetMapper mapper

vtkVectorText banner
    banner SetText "      vtk\nStereo-Lithography\n     Viewer"
vtkPolyMapper bannerMapper
    bannerMapper SetInput [banner GetOutput]
vtkActor bannerActor
    bannerActor SetMapper bannerMapper

set renWin [.window GetRenderWindow]
set ren   [$renWin MakeRenderer]

$ren AddActors bannerActor
[$ren GetActiveCamera] Zoom 1.2
