proc processa {} {
    vtkConeSource cone
    vtkElevationFilter elev 
    vtkOutputPort upStreamPort
    upStreamPort SetController controller
    
    cone SetResolution 8
    elev SetInput [cone GetOutput]
    upStreamPort SetInput [elev GetPolyDataOutput]
    upStreamPort SetTag 999

    upStreamPort WaitForUpdate
  
    cone Delete
    elev Delete
    upStreamPort Delete
}

proc processb {} {
    vtkInputPort downStreamPort
    downStreamPort SetController controller
    downStreamPort SetRemoteProcessId 0
    downStreamPort SetTag 999
    [downStreamPort GetPolyDataOutput] SetUpdateExtent 0  4
    downStreamPort Update

    set data [downStreamPort GetPolyDataOutput]
    
    vtkPolyDataMapper coneMapper
    coneMapper SetInput [downStreamPort GetPolyDataOutput]

    vtkActor coneActor
    coneActor SetMapper coneMapper
    
    vtkRenderer ren 
    ren AddActor coneActor
    ren SetBackground 0.1 0.3 0.5
    
    vtkRenderWindow renWin
    renWin AddRenderer ren
    renWin SetSize 300 300

    vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    
    
    iren Initialize
    iren Start
    controller TriggerRMI 0 239954
}

vtkMPIController controller
set myId [controller GetLocalProcessId]

wm withdraw .

if {$myId == 0 } {
    processa
} else {
    processb
}

controller Finalize
vtkCommand DeleteAllObjects
exit