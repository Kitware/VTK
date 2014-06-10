/**
 * vtkWeb JavaScript Library.
 *
 * This testing module is made to test vtkWeb components
 *
 * This module registers itself as: 'vtkweb-testing'
 *
 * @class vtkWeb.testing
 *
 * @singleton
 */
 (function (GLOBAL, $) {

    var parentModule = {}, module = {}, testToRun = 0;
    testSuite = {}, testOrder = [], outputLog = [], w = $(window),
    testTotal = 0, testRunned = 0, success = 0, failure = 0,
     testNameToRun = [],
    testLineTemplate = '<tr id="NAME"><td><input type="checkbox" value="NAME" checked/></td><td width="100%">NAME</td><td align="center" class="status">STATUS</td></tr>',
    tableHeader = '<thead><tr><th><input type="checkbox" value="select-all-test" title="Toggle all test selection" checked/></th><th width="100%">Test names</th><th>Status</th></tr>',
    outputLogHTML = '<div class="row-fluid log" style="display: none;"><div class="span12"><textarea style="width: 98%;height: 150px;"></textarea></div></div></thead>',
    controlHTML = '<table class="table table-striped"><thead><tr><th width="100%"></th><th><div class="run-tests vtk-icon-play"></div></th><th><div class="toggle-log vtk-icon-tools"></div></th><th class="summary" style="border-radius: 5px;">Results</th><th><span id="passedTestsSpan" class="badge badge-success">0</span></th><th><span id="failedTestsSpan" class="badge" style="background-color: red">0</span></th></tr></thead></table>',
    statusSuccess = '<div class="vtk-icon-ok-circled" style="color: green;"></div>',
    statusFailure = '<div class="vtk-icon-cancel-circled" style="color: red;"></div>',
    statusNotRunned = '<div class="vtk-icon-help-circled"></div>';

    // ----------------------------------------------------------------------
    // Init vtkWeb.testing module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("vtkWeb")) {
        parentModule = GLOBAL.vtkWeb || {};
    } else {
        GLOBAL.vtkWeb = parentModule;
    }
    if (parentModule.hasOwnProperty("testing")) {
        module = GLOBAL.vtkWeb.testing || {};
    } else {
        GLOBAL.vtkWeb.testing = module;
    }

    // ----------------------------------------------------------------------
    // Internal methods
    // ----------------------------------------------------------------------

    function registerTest(name, func) {
        testOrder.push(name);
        testSuite[name] = {
            'name': name,
            'func': func,
            'active': true,
            'message': null,
            'success': false,
            'done': false
        };
    }

    // ----------------------------------------------------------------------

    function clearLog() {
        outputLog = [];
    }

    // ----------------------------------------------------------------------

    function logToString() {
        return outputLog.join('\n');
    }

    // ----------------------------------------------------------------------

    function clearResults() {
        outputLog = [];
        testTotal = 0;
        testRunned = 0;
        success = 0;
        failure = 0;
        for(var test in testSuite) {
            test.success = false;
            test.message = null;
            test.done = false;
        }
        w.trigger('vtk-test-clear');
    }

    // ----------------------------------------------------------------------

    function log(msg) {
        outputLog.push(msg);
        w.trigger('vtk-test-log-update');
    }

    // ----------------------------------------------------------------------

    function resultCallback(result) {
        var test = testSuite[result.name];

        // Update test info
        test.done = true;
        test.message = result.message;
        test.success = result.success;

        // Less test remaining
        ++testRunned;
        if(result.success) {
            ++success;
        } else {
            ++failure;
        }

        // Update log
        log((test.success?"+":"-") + result.name + ": " + test.message);

        // Trigger event
        result.count = testRunned;
        result.total = testTotal;
        result.coutSuccess = success;
        result.countFailure = failure;
        result.type = 'vtk-test-complete';
        w.trigger(result);

        if(testTotal === testRunned) {
            w.trigger('vtk-test-done');
        } else {
            runAnotherTest();
        }
    }

    // ----------------------------------------------------------------------

    function testToString(testName) {
        return testLineTemplate.replace(/ID/g, testName).replace(/NAME/g, testName).replace('STATUS', statusNotRunned);
    }

     // ----------------------------------------------------------------------

     function runAnotherTest() {
         var testName = testNameToRun.shift();
         var testFunction = testSuite[testName].func;
         testFunction(testName, resultCallback);
     }

    // ----------------------------------------------------------------------
    // Export methods to the vtkWeb.testing module
    // ----------------------------------------------------------------------

    /**
     * Register a set of methods as tests.
     *
     * @class vtkWeb.testing
     * @method registerTests
     * @param {Object} testSuiteToRegister
     *         Should contains a set of methods that will be
     *         executed.
     */

    module.registerTests = function(testSuiteToRegister) {
        for (var testName in testSuiteToRegister) {
            registerTest(testName, testSuiteToRegister[testName]);
        }
    }

    // ----------------------------------------------------------------------

    /**
     * Trigger the run of all the active test that have been
     * previously registered.
     *
     * @class vtkWeb.testing
     * @method runTests
     */

    module.runTests = function() {
        clearResults();
        log("Start testings...");

        // Extract active tests list
        testNameToRun = [];
        for(var idx in testOrder) {
            if(testSuite[testOrder[idx]].active) {
                testNameToRun.push(testOrder[idx]);
            }
        }
        testTotal = testNameToRun.length;

        // Start the tests running
        runAnotherTest();
    }

    // ----------------------------------------------------------------------

    /**
     * Toggle test to be run.
     *
     * @class vtkWeb.testing
     * @method enableTest
     */

    module.enableTest = function(name, status) {
        testSuite[name].active = status;
    }

    // ----------------------------------------------------------------------

    /**
     * Return test results summary.
     *
     * @class vtkWeb.testing
     * @method getCurrentTestResults
     */

    module.getCurrentTestResults = function() {
        return {
            'finished': (testTotal === testRunned),
            'testCount': testTotal,
            'successes': success,
            'failures': failure
        };
    }

    // ----------------------------------------------------------------------

    /**
     * Retrieve the test log, which contains all detailed error outputs from
     * the tests.
     *
     * @class vtkWeb.testing
     * @method getTestLog
     */

    module.getTestLog = logToString;

    // ----------------------------------------------------------------------
    // Provide JQuery widgets
    // ----------------------------------------------------------------------

    /**
     * Fill the selected container with test list and
     * action buttons to toggle and run them.
     *
     * @class jquery
     * @method vtkWebTestList
     */
    $.fn.vtkWebTestList = function(options) {
        return this.each(function() {
            var me = $(this).empty(), buffer = [];

            // Generate HTML
            buffer.push(controlHTML);
            buffer.push('<table class="table table-striped test-panel">');
            buffer.push(tableHeader);
            buffer.push('<tbody>');
            for(var idx in testOrder) {
                buffer.push(testToString(testOrder[idx]));
            }
            buffer.push('</tbody></table>');
            buffer.push(outputLogHTML);
            me[0].innerHTML = buffer.join('');

            // Initialize listeners
            $('input', me).change(function(){
                var checkbox = $(this),
                checked = checkbox.is(":checked"),
                testName = checkbox.val();

                if(testSuite.hasOwnProperty(testName)) {
                    testSuite[testName].active = checked;
                } else {
                    $('input', me).prop('checked', checked);
                    for(var test in testSuite) {
                        testSuite[test].active = checked;
                    }
                }
                clearResults();
            });

            $('.toggle-log', me).click(function(){
                var log = $('.log');
                var results = $('.test-panel');
                if(log.is(":visible")) {
                    log.hide();
                    results.show();
                } else {
                    log.show();
                    results.hide();
                }
            });

            $('.run-tests', me).click(function(){
                module.runTests();
            });

            // Bind events to UI updates
            w.bind('vtk-test-clear', function(){
                updateTextArea();
                updateResultSummary();
                $(".status").html(statusNotRunned);
            }).bind('vtk-test-complete', function(event){
                updateTextArea();
                updateResultSummary();
                updateStatus(event.name);
            });
        });
    };

    // ----------------------------------------------------------------------

    function updateTextArea() {
        $('textarea')[0].innerHTML = logToString();
    }

    // ----------------------------------------------------------------------

    function updateResultSummary() {
        $('#passedTestsSpan').html(success);
        $('#failedTestsSpan').html(failure);
        if(testTotal !== 0 && testTotal === testRunned) {
            if(success === testTotal) {
                $('.summary').css('background', 'green');
            } else {
                $('.summary').css('background', 'red');
            }
        } else {
            $('.summary').css('background', 'none');
        }
    }

    // ----------------------------------------------------------------------

    function updateStatus(testName) {
        var test = testSuite[testName]
        $("#" + testName + " .status").html(test.success ? statusSuccess : statusFailure).attr('title', test.message);
    }

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        parentModule.registerModule('vtkweb-testing');
    } else {
        console.error('Module failed to register, jQuery is missing.');
    }
} catch(err) {
  console.error('Caught exception while registering module: ' + err.message);
}

}(window, jQuery));
