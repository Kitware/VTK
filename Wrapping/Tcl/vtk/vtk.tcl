package require vtkbase
package require vtkcommon
package require vtkfiltering
package require vtkgraphics
package require vtkimaging
package require vtkio

# Catched because of pkg_mkIndex
catch {
    set ::vtk::__temp_complain_on_loading $::vtk::complain_on_loading
    set ::vtk::complain_on_loading 0
}

catch {
    package require vtkhybrid
}
catch {
    package require vtkparallel
}
catch {
    package require vtkpatented
}
catch {
    package require vtkrendering
}

catch {
    set ::vtk::complain_on_loading $::vtk::__temp_complain_on_loading
    unset ::vtk::__temp_complain_on_loading
}

package provide vtk 4.0

