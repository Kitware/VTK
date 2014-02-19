(function ($, GLOBAL) {
    var SLIDER_TEMPLATE = '<div class="label"><span class="flag vtk-icon-flag"/>LABEL<span class="NAME-value">DEFAULT</span></div><input type="range" min="0" max="SIZE" value="INDEX" name="NAME" data-values="VALUES"/>',
    SELECT_TEMPLATE = ' <div class="label select"><span class="flag vtk-icon-flag"/>LABEL<select name="NAME">VALUES</select></div>',
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

            container.data('session').call("vtk:updateActiveArgument", name, value).then(function(){
                container.data('viewport').render(callback);
            });
        }
    }

    // ========================================================================
    // UI
    // ========================================================================

    var WidgetFactory = {
        "range": function(name, label, values, defaultValue) {
            return templateReplace(SLIDER_TEMPLATE, name, label, values, defaultValue);
        },
        "list": function(name, label, values, defaultValue) {
            var options = [];
            for(var idx in values) {
                var selected = (values[idx] === defaultValue) ? 'selected="selected"' : '';
                options.push(OPTION_TEMPLATE.replace('VALUE', values[idx]).replace('SELECTED', selected));
            }
            return templateReplace(SELECT_TEMPLATE, name, label, [ options.join('') ], defaultValue);
        }
    };

    // ------------------------------------------------------------------------

    function templateReplace( templateString, name, label, values, defaultValue) {
        return templateString.replace(/NAME/g, name).replace(/LABEL/g, label).replace(/VALUES/g, values.join(':')).replace(/SIZE/g, values.length - 1).replace(/DEFAULT/g, defaultValue).replace(/INDEX/g, values.indexOf(defaultValue));
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
            defaultValue = args[key]['default'];

            // Update default value
            updateActiveArgument(container, name, defaultValue);

            // Filter out from UI some pre-defined args
            if(EXCLUDE_ARGS.hasOwnProperty(key)) {
                continue;
            }

            // Build widget if needed
            if(values.length > 1) {
                 htmlBuffer.push(WidgetFactory[type](name, label, values, defaultValue));
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

                        // Load files
                           session.call("vtk:openFileFromPath", data["files"]).then(function(){
                              viewport.render();
                           },function(e){
                              console.log(e);
                           });

                        // Create Control UI
                           session.call("vtk:getArguments").then(function(args){
                               createControlPanel(me, args);
                               initializeListener(me);
                           });

                        // Update stop method to use the connection
                        stop = function() {
                            connection.session.call('vtk:exit').then(function() {connection.session.close();});
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

}(jQuery, window));
