namespace eval ::vtk {

    namespace export *

    # -------------------------------------------------------------------
    # Create vtkTkRenderWidget bindings, setup observers

    proc bind_tk_render_widget {vtkrw} {
        bind_tk_widget $vtkrw [$vtkrw GetRenderWindow]
    }
}

# Handle deprecated calls

proc BindTkRenderWidget {widget} {
    puts stderr "BindTkImageViewer is deprecated. Please use ::vtk::bind_tk_render_widget instead"
    ::vtk::bind_tk_render_widget $widget
}
