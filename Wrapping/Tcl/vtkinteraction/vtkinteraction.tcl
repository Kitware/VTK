package require -exact vtkrendering 4.2

foreach s {Interactor bindings-rw bindings-iw bindings setget} {
  source [file join [file dirname [info script]] "${s}.tcl"]
}

package provide vtkinteraction 4.2
