"""This is python equivalent of Wrapping/Tcl/vtktesting/mccases.tcl.
Used for setting vertex values for clipping, cutting, and contouring tests.
This script is used while running python tests translated from Tcl."""

def case1 ( scalars, IN, OUT, caseLabel ):
    scalars.InsertValue(0,IN )
    scalars.InsertValue(1,OUT)
    scalars.InsertValue(2,OUT)
    scalars.InsertValue(3,OUT)
    scalars.InsertValue(4,OUT)
    scalars.InsertValue(5,OUT)
    scalars.InsertValue(6,OUT)
    scalars.InsertValue(7,OUT)
    if IN == 1:
        caseLabel.SetText("Case 1 - 00000001")
    else :
        caseLabel.SetText("Case 1c - 11111110")
    pass
    
def case2 ( scalars, IN, OUT, caseLabel ):
    scalars.InsertValue(0,IN)
    scalars.InsertValue(1,IN)
    scalars.InsertValue(2,OUT)
    scalars.InsertValue(3,OUT)
    scalars.InsertValue(4,OUT)
    scalars.InsertValue(5,OUT)
    scalars.InsertValue(6,OUT)
    scalars.InsertValue(7,OUT)
    if IN == 1:
        caseLabel.SetText("Case 2 - 00000011")
    else:
        caseLabel.SetText("Case 2c - 11111100")
    pass


