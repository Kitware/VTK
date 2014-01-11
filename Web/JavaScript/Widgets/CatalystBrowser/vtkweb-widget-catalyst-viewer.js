(function ($, GLOBAL) {
    var SLIDER_TEMPLATE = '<div class="label">LABEL<span class="NAME-value">DEFAULT</span></div><input type="range" min="0" max="SIZE" value="INDEX" name="NAME" data-values="VALUES"/>',
    SELECT_TEMPLATE = ' <div class="label select">LABEL<select name="NAME">VALUES</select></div>',
    OPTION_TEMPLATE = '<option>VALUE</option>',
    EXCLUDE_ARGS = { "phi": true, "theta": true };

    // ========================================================================
    // Download manager
    // ========================================================================

    function createDownloadManager(container, poolSize, basepath) {
        var idleImages = [], processingQueue = [], manager = {
            clearProcessingQueue: function() {
                processingQueue = [];
            },

            download: function(url) {
                processingQueue.push(url);
                download();
            },

            downloadFiles: function(filePattern, argName, argValues, args) {
                var baseFileName = filePattern, rStr = '{'+argName+'}';

                for(key in args) {
                    if(key !== argName) {
                        baseFileName = baseFileName.replace('{'+key+'}', args[key]);
                    }
                }

                for(idx in argValues) {
                    processingQueue.push(basepath + '/' + baseFileName.replace(rStr, argValues[idx]));
                }
            }
        };

        // Attach download manager to container
        container.data('download-manager', manager);
        container.bind('load-image', function(e) {
            manager.download(basepath + '/' + e.filename);
        });

        function download() {
            while(idleImages.length > 0 && processingQueue.length > 0) {
                var img = idleImages.pop(),
                url = processingQueue.pop();
                img.src = url;
            }
        }

        function onLoadCallback(arg) {
            var me = $(this), url = me.attr('src');
            idleImages.push(this);
            container.trigger({
                type: "image-loaded",
                url: url
            });
            download();
        }

        function onError() {
            idleImages.push(this);
            download();
        }

        for(var i = 0; i < poolSize; ++i) {
            var img = new Image();
            img.onload = onLoadCallback;
            img.onabort = onError;
            img.onerror = onError;
            idleImages.push(img);
        }

        return manager;
    }

    // ========================================================================
    // Events
    // ========================================================================

    function fireLoadImage(container) {
        // Extrat container info
        var filename = container.data('info')['name_pattern'],
        args = container.data('active-args');

        // Update filename
        for(key in args) {
            filename = filename.replace('{'+key+'}', args[key]);
        }

        // Trigger event
        container.trigger({
            type: 'load-image',
            arguments: args,
            filename: filename
        });
    }

    // ========================================================================
    // Listeners
    // ========================================================================

    function initializeListeners(container) {
        // Attach slider listener
        $('input[type="range"]', container).bind('change keyup mousemove',function(){
            var slider = $(this),
            name = slider.attr('name'),
            values = slider.attr('data-values').split(":"),
            idx = slider.val();

            updateActiveArgument(container, name, values[idx]);
        })

        // Attach select listener
        $('select', container).change(function(){
            var select = $(this),
            name = select.attr('name'),
            value = select.val();

            updateActiveArgument(container, name, value);
        })
    }

    // ------------------------------------------------------------------------

    function updateActiveArgument(container, name, value) {
        if(container.data('active-args')[name] !== value) {
            var downloadManager = container.data('download-manager'),
            info = container.data('info');
            container.data('active-args')[name] = value;
            $('span.'+name+'-value', container).html(value);
            downloadManager.clearProcessingQueue();
            fireLoadImage(container);

            // Try to cache all argument values
            if(container.data('preload')) {
                downloadManager.downloadFiles(info['name_pattern'], name, info['arguments'][name]['values'], container.data('active-args'));
            }
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
                options.push(OPTION_TEMPLATE.replace('VALUE', values[idx]));
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
        var htmlBuffer = [];

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
            class: "control",
            html: '<li>' + htmlBuffer.join('</li><li>') + '</li>'
        }).appendTo(container);

        // Attache listeners
        initializeListeners(container);
    }

    // ------------------------------------------------------------------------

    function createStaticImageViewer(container) {
        var imageContainer = $('<img/>', { class: 'image-viewer' }),
        currentFileToRender = null;
        imageContainer.appendTo(container);
        imageContainer.bind('onload load', function(){
            container.trigger('image-render');
        });
        container.bind('image-loaded', function(event){
            if(currentFileToRender === null || event.url.indexOf(currentFileToRender) != -1) {
                imageContainer.attr('src', event.url);
            }
        });
        container.bind('load-image', function(event){
            currentFileToRender = event.filename;
        });
        return imageContainer;
    }

    // ------------------------------------------------------------------------

    function createInteractiveImageViewer(container) {
        var startPosition, dragging = false,
        thetaValues = container.data('info').arguments.theta.values,
        phiValues   = container.data('info').arguments.phi.values,
        stepPhi    = phiValues[1] - phiValues[0];
        stepTheta  = thetaValues[1] - thetaValues[0];
        currentArgs = container.data('active-args'),
        imageContainer = createStaticImageViewer(container);

        function getRelativeLocation(element, mouseEvent) {
            var parentOffset = element.parent().offset();
            var relX = mouseEvent.pageX - parentOffset.left;
            var relY = mouseEvent.pageY - parentOffset.top;
            return { 'x': relX, 'y': relY };
        }

        imageContainer.bind('mousedown', function(event){
            event.preventDefault();
            dragging = true;
            startPosition = getRelativeLocation(imageContainer, event);
        }).bind('mousemove', function(event){
            event.preventDefault();
            if(dragging) {
                currentPosition = getRelativeLocation(imageContainer, event);
                currentPhi = currentArgs.phi;
                currentTheta = currentArgs.theta;
                currentPhiIdx = phiValues.indexOf(currentPhi);
                currentThetaIdx = thetaValues.indexOf(currentTheta);

                deltaPhi = currentPosition.x - startPosition.x;
                deltaTheta = currentPosition.y - startPosition.y;

                changeDetected = false;
                if(Math.abs(deltaPhi) > stepPhi) {
                    changeDetected = true;
                    currentPhiIdx += (deltaPhi > 0) ? 1 : -1;
                    if(currentPhiIdx >= phiValues.length) {
                        currentPhiIdx -= phiValues.length;
                    } else if(currentPhiIdx < 0) {
                        currentPhiIdx += phiValues.length;
                    }
                    currentArgs['phi'] = phiValues[currentPhiIdx];
                }

                if(Math.abs(deltaTheta) > stepTheta) {
                    changeDetected = true;
                    currentThetaIdx += (deltaTheta > 0) ? 1 : -1;
                    if(currentThetaIdx >= thetaValues.length) {
                        currentThetaIdx -= 1;
                    } else if(currentThetaIdx < 0) {
                        currentThetaIdx += 1;
                    }
                    currentArgs['theta'] = thetaValues[currentThetaIdx];
                }

                if(changeDetected) {
                    startPosition = getRelativeLocation(imageContainer, event);
                    fireLoadImage(container);
                }
            }
        }).bind('mouseup', function(event){
            event.preventDefault();
            dragging = false;
        });
    }

    // ========================================================================
    // JQuery
    // ========================================================================

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystViewer = function(dataBasePath, preload) {
        return this.each(function() {
            var me = $(this).empty().addClass('vtk-catalyst-viewer').unbind();

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Store metadata
                    me.data('info', data);
                    me.data('active-args', {});
                    me.data('base-path', dataBasePath);
                    me.data('preload', (preload ? true : false));

                    // Create download manager
                    createDownloadManager(me, 5, dataBasePath);

                    // Create Control UI
                    createControlPanel(me, data.arguments);

                    // Create interactive viewer
                    if(data.arguments.hasOwnProperty('phi')) {
                        createInteractiveImageViewer(me);
                    } else {
                        createStaticImageViewer(me);
                    }

                    // Load default image
                    fireLoadImage(me);
                },
                error: function(error) {
                    console.log("error");
                    console.log(error);
                }
            });
        });
    }

    }(jQuery, window));