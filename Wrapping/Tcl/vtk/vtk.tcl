foreach kit ${vtk::init::kits} {
  package require -exact vtk${kit} 4.4
}

package provide vtk 4.4
