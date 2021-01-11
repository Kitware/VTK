from vtkmodules.vtkCommonCore import vtkCommand


class vtkErrorObserver(object):
    def __init__(self):
        self.CallDataType = 'string0'
        self.reset()

    def __call__(self, caller, event, data):
        if event == 'ErrorEvent':
            self._error_message = data
        elif event == 'WarningEvent':
            self._warning_message = data

    def _check(self, seen, actual, expect, what):
        if seen:
            if actual.find(expect) == -1:
                msg = 'ERROR: %s message does not contain "%s" got \n"%s"' \
                            % (what, expect, self.error_message)
                raise RuntimeError(msg)
        else:
            what = what.lower()
            msg = 'ERROR: Failed to catch any %s. ' \
                  'Expected the %s message to contain "%s"' \
                  % (what, what, expect)
            raise RuntimeError(msg)
        self.reset()

    def check_error(self, expect):
        self._check(self.saw_error, self.error_message, expect, 'Error')

    def check_warning(self, expect):
        self._check(self.saw_warning, self.warning_message, expect, 'Warning')

    def reset(self):
        self._error_message = None
        self._warning_message = None

    @property
    def saw_error(self):
        return self._error_message is not None

    @property
    def error_message(self):
        return self._error_message

    @property
    def saw_warning(self):
        return self._warning_message is not None

    @property
    def warning_message(self):
        return self._warning_message
