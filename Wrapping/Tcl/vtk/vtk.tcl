foreach kit ${vtk::init::kits} {
  package require -exact vtk${kit} 4.3
}

package provide vtk 4.3
