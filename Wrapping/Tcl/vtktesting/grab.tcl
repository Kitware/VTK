package require -exact vtkrendering 4.2

namespace eval ::vtk {

    proc grab_screenshot {renderer basename} {

        $renderer Render

        set rs \
            [::vtk::new_widget_object "_${renderer}" vtkRendererSource rs]
        $rs SetInput $renderer
        $rs DepthValuesOff
        $rs DepthValuesInScalarsOn

        set pw \
            [::vtk::new_widget_object "_${renderer}" vtkPNGWriter pw]
        $pw SetInput [$rs GetOutput]
        $pw SetFileName "$basename-rgbz.png"
        $pw Write

        set iec \
        [::vtk::new_widget_object "_${renderer}" vtkImageExtractComponents iec]
        $iec SetInput [$rs GetOutput]
        $iec SetComponents 3

        $pw SetInput [$iec GetOutput]
        $pw SetFileName "$basename-z.png"
        $pw Write

        $iec SetComponents 0 1 2
        $pw SetFileName "$basename-rgb.png"
        $pw Write

        $rs Delete
        $pw Delete
        $iec Delete
    }
}
