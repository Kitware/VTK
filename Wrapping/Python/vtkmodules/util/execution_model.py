"""Utility classes to help with the simpler Python interface
for connecting and executing pipelines."""

class select_ports(object):
    """Helper class for selecting input and output ports when
    connecting pipeline objects with the >> operator.
    Example uses:
    # Connect a source to the second input of a filter.
    source >> select_ports(1, filter)
    # Connect the second output of a source to a filter.
    select_ports(source, 1) >> filter
    # Combination of both: Connect source to second
    # input of the filter, then connect the second
    # output of that filter to another one.
    source >>> select_ports(1, filter, 1) >> filter2
    """
    def __init__(self, *args):
        """This constructor takes 2 or 3 arguments.
        The possibilities are:
        select_ports(input_port, algorithm)
        select_ports(algorithm, output_port)
        select_ports(input_port, algorithm, output_port)
        """
        nargs = len(args)
        if nargs < 2 or nargs > 3:
            raise ValueError("Expecting 2 or 3 arguments")

        self.input_port = None
        self.output_port = None
        before_alg = True
        for arg in args:
            if hasattr(arg, "IsA") and arg.IsA("vtkAlgorithm"):
                self.algorithm = arg
                before_alg = False
            else:
                if before_alg:
                    self.input_port = arg
                else:
                    self.output_port = arg
        if not self.input_port:
            self.input_port = 0
        if not self.output_port:
            self.output_port = 0

    def SetInputConnection(self, inp):
        self.algorithm.SetInputConnection(self.input_port, inp)

    def GetOutputPort(self):
        return self.algorithm.GetOutputPort(self.output_port)

    def execute(self):
        """Execute the algorithm and return the output from the selected
        output port."""
        self.algorithm.execute()
        return self.algorithm.GetOutputDataObject(self.output_port)

    def __rshift__(self, rhs):
        return Pipeline(self, rhs)

class Pipeline(object):
    """Pipeline objects are created when 2 or more algorithms are
    connected with the >> operator. They store the first and last
    algorithms in the pipeline and enable connecting more algorithms
    and executing the pipeline. One should not have to create Pipeline
    objects directly. They are created by the use of the >> operator."""

    PIPELINE = 0
    ALGORITHM = 1
    DATA = 2
    UNKNOWN = 3

    def __init__(self, lhs, rhs):
        """Create a pipeline object that connects two objects of the
        following type: data object, pipeline object, algorithm object."""

        left_type = self._determine_type(lhs)
        right_type = self._determine_type(rhs)
        if left_type == Pipeline.UNKNOWN or right_type == Pipeline.UNKNOWN:
            raise TypeError(
                    f"unsupported operand type(s) for >>: {type(lhs).__name__} and {type(rhs).__name__}")
        if right_type == Pipeline.ALGORITHM:
            if left_type == Pipeline.ALGORITHM:
                rhs.SetInputConnection(lhs.GetOutputPort())
                self.first = lhs
                self.last = rhs
            elif left_type == Pipeline.PIPELINE:
                rhs.SetInputConnection(lhs.last.GetOutputPort())
                self.first = lhs.first
                self.last = rhs
            elif left_type == Pipeline.DATA:
                from vtkmodules.vtkCommonExecutionModel import vtkTrivialProducer
                source = vtkTrivialProducer()
                source.SetOutput(lhs)
                rhs.SetInputConnection(source.GetOutputPort())
                self.first = source
                self.last = rhs
        elif right_type == Pipeline.PIPELINE:
            if left_type == Pipeline.ALGORITHM:
                self.first = lhs
                self.last = rhs.last
                rhs.first.SetInputConnection(lhs.GetOutputPort())
            elif left_type == Pipeline.PIPELINE:
                rhs.first.SetInputConnection(lhs.last.GetOutputPort())
                self.first = lhs.first
                self.last = rhs.last
            elif left_type == Pipeline.DATA:
                from vtkmodules.vtkCommonExecutionModel import vtkTrivialProducer
                source = vtkTrivialProducer()
                source.SetOutput(lhs)
                rhs.first.SetInputConnection(source.GetOutputPort())
                self.first = source
                self.last = rhs.last

    def _determine_type(self, arg):
        if type(arg) is Pipeline:
            return Pipeline.PIPELINE
        if hasattr(arg, "SetInputConnection"):
            return Pipeline.ALGORITHM
        if hasattr(arg, "IsA") and arg.IsA("vtkDataObject"):
            return Pipeline.DATA
        return Pipeline.UNKNOWN

    def execute(self):
        """Execute the last algorithm in the pipeline and return its
        output."""
        return self.last.execute()

    def __rshift__(self, rhs):
        return Pipeline(self, rhs)
