#!/usr/bin/env python

# a simple user interface that manipulates window level.
# places in the tcl top window.  Looks for object named viewer
#only use this interface when not doing regression tests
if (info.commands(globals(), locals(),  rtExMath) != "rtExMath"):
    # Take window level parameters from viewer
    def InitializeWindowLevelInterface (__vtk__temp0=0,__vtk__temp1=0):
        global viewer, sliceNumber
        # Get parameters from viewer
        w = viewer.GetColorWindow()
        l = viewer.GetColorLevel()
        sliceNumber = viewer.GetZSlice()
        zMin = viewer.GetWholeZMin()
        zMax = viewer.GetWholeZMax()
        #   set zMin 0
        #   set zMax 128
        frame..slice()
        label..slice.label(-text,"Slice")
        scale..slice.scale([-from,zMin,-to,zMax,-orient,horizontal,-command,SetSlice,-variable,sliceNumber])
        #   button .slice.up -text "Up" -command SliceUp
        #   button .slice.down -text "Down" -command SliceDown
        frame..wl()
        frame..wl.f1()
        label..wl.f1.windowLabel(-text,"Window")
        scale..wl.f1.window([-from,1,-to,expr.expr(globals(), locals(),["w","*","2"]),-orient,horizontal,-command,SetWindow,-variable,window])
        frame..wl.f2()
        label..wl.f2.levelLabel(-text,"Level")
        scale..wl.f2.level(-from,expr.expr(globals(), locals(),["l","-","w"]),-to,expr.expr(globals(), locals(),["l","+","w"]),-orient,horizontal,-command,SetLevel)
        checkbutton..wl.video(-text,"Inverse Video",-command,SetInverseVideo)
        # resolutions less than 1.0
        if (w < 10):
            res = expr.expr(globals(), locals(),["0.05","*","w"])
            .wl.f1.window.configure(-resolution,res,-from,res,-to,expr.expr(globals(), locals(),["2.0","*","w"]))
            .wl.f2.level.configure(-resolution,res,-from,expr.expr(globals(), locals(),["0.0","+","l","-","w"]),-to,expr.expr(globals(), locals(),["0.0","+","l","+","w"]))
            pass
        .wl.f1.window.set(w)
        .wl.f2.level.set(l)
        frame..ex()
        button..ex.exit(-text,"Exit",-command,"exit")
        pack..slice(.wl,.ex,-side,top)
        pack..slice.label(.slice.scale,-side,left)
        pack..wl.f1(.wl.f2,.wl.video,-side,top)
        pack..wl.f1.windowLabel(.wl.f1.window,-side,left)
        pack..wl.f2.levelLabel(.wl.f2.level,-side,left)
        pack..ex.exit(-side,left)

    def SetSlice (slice,__vtk__temp0=0,__vtk__temp1=0):
        global sliceNumber, viewer
        viewer.SetZSlice(slice)
        viewer.Render()

    def SetWindow (window,__vtk__temp0=0,__vtk__temp1=0):
        global viewer, video
        if (video):
            viewer.SetColorWindow(expr.expr(globals(), locals(),["-window"]))
            pass
        else:
            viewer.SetColorWindow(window)
            pass
        viewer.Render()

    def SetLevel (level,__vtk__temp0=0,__vtk__temp1=0):
        global viewer
        viewer.SetColorLevel(level)
        viewer.Render()

    def SetInverseVideo (__vtk__temp0=0,__vtk__temp1=0):
        global viewer, video, window
        if (video):
            viewer.SetColorWindow(expr.expr(globals(), locals(),["-window"]))
            pass
        else:
            viewer.SetColorWindow(window)
            pass
        viewer.Render()

    InitializeWindowLevelInterface()
    pass
else:
    viewer.Render()
    pass
# --- end of script --
