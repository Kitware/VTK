package require -exact vtkbase 4.2
package require -exact vtkcommon 4.2
package require -exact vtkfiltering 4.2
package require -exact vtkgraphics 4.2
package require -exact vtkimaging 4.2
package require -exact vtkio 4.2

# Catched because of pkg_mkIndex
catch {
    set ::vtk::__temp_complain_on_loading $::vtk::complain_on_loading
    set ::vtk::complain_on_loading 0
}

catch {
    package require -exact vtkhybrid 4.2
}
catch {
    package require -exact vtkparallel 4.2
}
catch {
    package require -exact vtkpatented 4.2
}
catch {
    package require -exact vtkrendering 4.2
}

catch {
    set ::vtk::complain_on_loading $::vtk::__temp_complain_on_loading
    unset ::vtk::__temp_complain_on_loading
}

package provide vtk 4.2
