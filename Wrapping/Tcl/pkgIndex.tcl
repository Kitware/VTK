# Tcl package index file, version 1.0

package ifneeded vtktcl 4.0 {
    set ok 1
    
    if {$tcl_platform(platform) == "windows"} {
        # Try to load at least the Common part
        if {[catch {
            load vtkCommonTCL
        } errormsg ]} {
            puts $errormsg
            set ok 0
        } else {
            # Try to load the other components
            catch {
                load vtkFilteringTCL
                load vtkGraphicsTCL
                load vtkHybridTCL
                load vtkImagingTCL
                load vtkIOTCL
                load vtkParallelTCL
                load vtkPatentedTCL
            }
            # Try to set the exit method of the interactor
            if {![catch {load vtkRenderingTCL}] && \
                    [catch {
                vtkWin32RenderWindowInteractor __temp_vtkwin32iren__
                __temp_vtkwin32iren__ SetClassExitMethod exit
                __temp_vtkwin32iren__ Delete
            } errormsg ]} {
                # warn the user, but do not prevent to provide the package
                puts $errormsg
            }
        }
    }

    if {$ok} {
        package provide vtktcl 4.0
    }
}
