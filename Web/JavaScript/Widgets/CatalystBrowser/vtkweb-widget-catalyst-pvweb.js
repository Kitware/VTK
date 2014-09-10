(function ($, GLOBAL) {
    var SLIDER_TEMPLATE = 'PRIORITY<div class="label"><span class="flag vtk-icon-flag"/>LABEL<span class="NAME-value">DEFAULT</span></div><input type="range" min="0" max="SIZE" value="INDEX" name="NAME" data-values="VALUES"/>',
    SELECT_TEMPLATE = 'PRIORITY<div class="label select"><span class="flag vtk-icon-flag"/>LABEL<select name="NAME">VALUES</select></div>',
    OPTION_TEMPLATE = '<option SELECTED>VALUE</option>',
    EXCLUDE_ARGS = { "theta": true };

    // ========================================================================
    // Listeners
    // ========================================================================

    function initializeListeners(container) {
        var play = $('.play', container),
        stop = $('.stop', container),
        activeArgName = null,
        activeValues = [],
        activeValueIndex = 0,
        keepAnimation = false;

        function animate_callback() {
            if(keepAnimation) {
                setTimeout(animate, 150);
            }
        }

        function animate() {
            if(activeArgName !== null) {
                activeValueIndex++;
                activeValueIndex = activeValueIndex % activeValues.length;
                updateActiveArgument(container, activeArgName, activeValues[activeValueIndex], animate_callback);
            }
        }

        // Attach slider listener
        $('input[type="range"]', container).bind('change keyup',function(){
            var slider = $(this),
            name = slider.attr('name'),
            values = slider.attr('data-values').split(":"),
            idx = slider.val();

            updateActiveArgument(container, name, values[idx]);
        });

        // Attach select listener
        $('select', container).change(function(){
            var select = $(this),
            name = select.attr('name'),
            value = select.val();

            updateActiveArgument(container, name, value);
        });

        $('.toggle', container).click(function(){
            container.toggleClass('small');
        });

        $('.reset', container).click(function(){
            container.data('viewport').resetCamera();
        });

        $('.label', container).click(function(){
            var me = $(this),
            all = $('.label', container),
            selectObj = $('select', me.parent()),
            sliderObj = $('input', me.parent());

            // Handle flag visibility
            all.removeClass('active');
            me.addClass('active');

            // Extract active parameter
            if(selectObj.length) {
                activeArgName = selectObj.attr('name');
                activeValueIndex = 0;
                activeValues = [];
                $('option', selectObj).each(function(idx, elm) {
                   activeValues.push($(this).text());
                });
            }
            if(sliderObj.length) {
                activeArgName = sliderObj.attr('name');
                activeValueIndex = sliderObj.val();
                activeValues = sliderObj.attr('data-values').split(':');
            }
        });

        play.click(function(){
            play.hide();
            stop.show();
            keepAnimation = true;
            animate();
        });
        stop.click(function(){
            stop.hide();
            play.show();
            keepAnimation = false;
        });
    }

    // ------------------------------------------------------------------------

    function updateActiveArgument(container, name, value, callback) {
        if(container.data('active-args')[name] !== value) {
            info = container.data('info');
            container.data('active-args')[name] = value;
            $('span.'+name+'-value', container).html(value);

            container.data('session').call("catalyst.active.argument.update", [name, value]).then(function(){
                container.data('viewport').render(callback);
            });
        }
    }

    // ========================================================================
    // UI
    // ========================================================================

    var WidgetFactory = {
        "range": function(name, label, values, defaultValue, priority) {
            return templateReplace(SLIDER_TEMPLATE, name, label, values, defaultValue, priority);
        },
        "list": function(name, label, values, defaultValue, priority) {
            var options = [];
            for(var idx in values) {
                var selected = (values[idx] === defaultValue) ? 'selected="selected"' : '';
                options.push(OPTION_TEMPLATE.replace('VALUE', values[idx]).replace('SELECTED', selected));
            }
            return templateReplace(SELECT_TEMPLATE, name, label, [ options.join('') ], defaultValue, priority);
        }
    };

    // ------------------------------------------------------------------------

    function templateReplace( templateString, name, label, values, defaultValue, priority) {
        return templateString.replace(/NAME/g, name).replace(/LABEL/g, label).replace(/VALUES/g, values.join(':')).replace(/SIZE/g, values.length - 1).replace(/DEFAULT/g, defaultValue).replace(/INDEX/g, values.indexOf(defaultValue)).replace(/PRIORITY/g, "                          ".substring(0,priority));
    }

    // ------------------------------------------------------------------------

    function createControlPanel(container, args) {
        var htmlBuffer = [],
        controlContainer = $('<div/>', {
            class: 'control',
            html: '<div class="header"><span class="vtk-icon-tools toggle"/><span class="vtk-icon-resize-full-2 reset"/><span class="vtk-icon-play play"/><span class="vtk-icon-stop stop"/></div><div class="parameters"></div>'
        });

        // Loop over each option
        for (key in args) {
            var name = key,
            type = args[key].type,
            label = args[key].label,
            values = args[key].values,
            priority = args[key].priority,
            defaultValue = args[key]['default'];

            // Update default value
            updateActiveArgument(container, name, defaultValue);

            // Filter out from UI some pre-defined args
            if(EXCLUDE_ARGS.hasOwnProperty(key)) {
                continue;
            }

            // Build widget if needed
            if(values.length > 1) {
                 htmlBuffer.push(WidgetFactory[type](name, label, values, defaultValue, priority));
            }
        }

        // Add control panel to UI
        htmlBuffer.sort();
        $('<ul/>', {
            html: '<li>' + htmlBuffer.join('</li><li>') + '</li>'
        }).appendTo($('.parameters', controlContainer));
        controlContainer.appendTo(container);

        // Attache listeners
        initializeListeners(container);
    }

    // ========================================================================
    // JQuery
    // ========================================================================

    /**
     * jQuery catalyst view constructor.
     *
     * @member fn.vtkCatalystPVWeb
     * @param basePath
     * Root directory for data to visualize
     */

     $.fn.vtkCatalystPVWeb = function(dataBasePath) {
        return this.each(function() {
            var me = $(this).empty().addClass('vtk-catalyst-pvweb small'); //.unbind();

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Store metadata
                    me.data('info', data);
                    me.data('active-args', {});

                    var config = {
                        sessionManagerURL: vtkWeb.properties.sessionManagerURL,
                        application: data['apps'],
                        dataDir: data["working_dir"],
                        type: data['pipeline-type']
                    },
                    stop = vtkWeb.NoOp,
                    start = function(connection) {
                        // Create viewport
                        var viewport = vtkWeb.createViewport({session:connection.session}),
                        session = connection.session;
                        me.data('viewport', viewport);
                        me.data('session', session);

                        viewport.bind(me[0]);

                        // Init pipeline
                        if(data.hasOwnProperty('configuration')) {
                           session.call("catalyst.pipeline.initialize", [data["configuration"]]).then(function(){
                              viewport.render();
                           },function(e){
                              console.log(e);
                           });
                        }

                        // Load files
                        if(data.hasOwnProperty('files')) {
                           session.call("catalyst.file.open", [data["files"]]).then(function(){
                              viewport.render();
                           },function(e){
                              console.log(e);
                           });
                        }

                        // Create Control UI
                        session.call("catalyst.arguments.get").then(function(args){
                            createControlPanel(me, args);
                        });

                        // Update stop method to use the connection
                        stop = function() {
                            session.call('application.exit.later', [5]).then(function() {
                                try {
                                    connection.connection.close();
                                } catch (closeError) {
                                    console.log(closeError);
                                }
                            }, function(err) {
                                console.log(err);
                            });
                        }

                        $('.close',me.parent()).click(stop);
                    };

                    // Try to launch the Viz process
                    vtkWeb.smartConnect(config, start, function(code,reason){
                        alert(reason);
                    });
                },
                error: function(error) {
                    console.log("error");
                    console.log(error);
                }
            });
        });
    }

    /**
     * Alternate jQuery catalyst view constructor.
     *
     * @member fn.vtkCatalystPVWeb
     * @param basePath
     * Root directory for data to visualize
     */

     $.fn.vtkCatalystPVWebDirect = function(data) {
        // FIXME: This function and the above function should be refactored
        // FIXME: together to avoid duplicated code.
        return this.each(function() {
            var me = $(this).empty().addClass('vtk-catalyst-pvweb small'); //.unbind();

            // Store metadata
            me.data('info', data);
            me.data('active-args', {});

            var stop = vtkWeb.NoOp,
            start = function(connection) {
                // Create viewport
                var viewport = vtkWeb.createViewport({session:connection.session}),
                session = connection.session;
                me.data('viewport', viewport);
                me.data('session', session);

                viewport.bind(me[0]);

                // Init pipeline
                if(data.hasOwnProperty('configuration')) {
                    session.call("catalyst.pipeline.initialize", [data["configuration"]]).then(function(){
                        viewport.render();
                    },function(e){
                        console.log("There was an error calling 'catalyst.pipeline.initialize':");
                        console.log(e);
                    });
                }

                // Load files
                if(data.hasOwnProperty('files')) {
                    session.call("catalyst.file.open", [data["files"]]).then(function(){
                        viewport.render();
                    },function(e){
                        console.log("There was an error calling 'catalyst.file.open':");
                        console.log(e);
                    });
                }

                // Create Control UI
                session.call("catalyst.arguments.get").then(function(args) {
                    createControlPanel(me, args);
                }, function(err) {
                    console.log("There was an error calling 'catalyst.arguments.get':");
                    console.log(err);
                });

                // Update stop method to use the connection
                stop = function() {
                    session.call('application.exit.later', [5]).then(function() {
                        try {
                            connection.connection.close();
                        } catch (closeError) {
                            console.log("Caught exception calling connection.close():");
                            console.log(closeError);
                        }
                    }, function(err) {
                        console.log("There was an error calling 'application.exit.later':");
                        console.log(err);
                    });
                }

                me.bind('stop-vtk-connection', stop);
            };

            // Try to launch the Viz process
            vtkWeb.connect(data, start, function(code,reason){
                console.log(reason);
            });
        });
    }

}(jQuery, window));
