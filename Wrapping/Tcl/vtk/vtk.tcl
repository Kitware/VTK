foreach kit ${vtk::init::kits} {
  package require -exact vtk${kit} 4.5
}

package provide vtk 4.5
