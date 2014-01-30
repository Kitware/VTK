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

    function createZoomableCanvasObject(container, img, canvas, mouseMode, externalMouseFunc) {
        // First set up some variables we will need
        var modeNone = 0,    // neither pan nor zoom, this is rotate mode
        modePan = 1,         // when dragging, it's a pan
        modeZoom = 2,        // when dragging, it's a zoom

        vpDim = [0, 0],      // viewport dimensions (canvas dimensions)
        vpAspect = 1,        // viewport aspect ratio (width/height)

        imgDim = [0, 0],     // image dimensions
        imgAspect = 1,       // image aspect ratio (width/height)

        srcOrig = [0, 0],    // upper left x,y of source image data
        srcDim = [0, 0],     // source data dimensions (how much of image to draw in viewport)

        gDim = 0,            // guide dimension, depends on aspect ratios of viewport and image

        zoomLevel = 1.0,     // zoomLevel >= 1.0, where 1.0 is zoomed all the way out
        dzScale = 0.05,      // scaling factor to control how fast we zoom in and out
        wheelZoom = 0.5,     // amount to change zoom with each wheel event
        minZoom = 0.0,       // limit how far we can zoom out
        maxZoom = 20.0,      // limit how far we can zoom in

        initialCentering = true,

        lastLocation = { 'x': 0, 'y': 0 };   // Last place mouse event happened

        /*
         * Whenever a new image is loaded, we want to make sure we make a note
         * of the image dimensions and aspect ratio.  If this is the very first
         * time, then we want to center the viewport on the image, otherwise,
         * we will just recalculate everything and redraw.
         */
        function imageLoaded() {
            // Get width, height, and aspect ratio of the image
            imgDim[0] = img[0].naturalWidth;
            imgDim[1] = img[0].naturalHeight;
            imgAspect = imgDim[0] / imgDim[1];

            if (initialCentering === true) {
                // Initially, we want the fully zoomed out zoom level
                minZoom = zoomLevel = vpDim[gDim] / imgDim[gDim];
            }

            recalculateAndRedraw();
        }

        /*
         * This function can be used on image load to center the viewport in
         * the middle of the image.
         */
        function centerViewportOnImage() {
            var center = calculateCenter([0, 0], imgDim);
            srcOrig[0] = center[0] - (srcDim[0] / 2.0);
            srcOrig[1] = center[1] - (srcDim[1] / 2.0);
        }

        /*
         * Once we know either the source width or the source height, the other
         * source dimension must be set according to the viewport aspect ratio.
         * This is because we don't want apparent stretching or shrinking of
         * the image data in either dimension.  In other words, source aspect
         * ratio should always equal viewport aspect ratio
         */
        function fixSrcDimForAspectRatio() {
            if (gDim === 0) {  // already know src width, need src height
                srcDim[1] = srcDim[0] / vpAspect;
            } else {               // already know src height, need src width
                srcDim[0] = srcDim[1] * vpAspect;
            }
        }

        /*
         * This function checks that we are never panned outside the image bounds
         */
        function fixSrcOrigForImageBounds() {
            // Check that both x and y origin don't allow for panning outside image
            for (var i = 0; i < 2; ++i) {
                if ( (srcOrig[i] + srcDim[i]) > imgDim[i] ) {
                    srcOrig[i] = imgDim[i] - srcDim[i];
                } else if ( srcOrig[i] < 0 ) {
                    srcOrig[i] = 0;
                }
            }
        }

        /*
         * Given a rectangle, specified with an origin (upper left x and y coords) and
         * dimension (width and height), calculate the implied center point.
         */
        function calculateCenter(orig, dim) {
            var center = [0, 0];
            center[0] = orig[0] + (dim[0] / 2.0);
            center[1] = orig[1] + (dim[1] / 2.0);
            return center;
        }

        /*
         * Adds mouse event handlers so that we can pan and zoom the image
         */
        function setupEvents() {
            var element = canvas;

            // Unbind all events the first time to make sure we start fresh
            element.unbind();

            // Needed this to override context menu behavior
            element.bind('contextmenu', function(evt) { return false; });

            // Wheel should zoom across browsers
            element.bind('DOMMouseScroll mousewheel', function (evt) {
                var x = (-evt.originalEvent.wheelDeltaY || evt.originalEvent.detail);
                handleZoom((x > 0 ? wheelZoom : x < 0 ? -wheelZoom : 0));
                evt.preventDefault();
            });

            // Zoom and pan events with mouse buttons and drag
            element.bind('mousedown', function(evt) {
                var current_button = evt.which;

                // alt+click simulates center button, shift+click simulates right
                if (evt.altKey) {
                    current_button = 2;
                    evt.altKey = false;
                } else if (evt.shiftKey) {
                    current_button = 3;
                    evt.shiftKey = false;
                }

                lastLocation = getRelativeLocation(canvas, evt);

                if (current_button === 2) {   // middle mouse down = pan
                    mouseMode = modePan;
                } else if (current_button === 3) {   // right mouse down = zoom
                    mouseMode = modeZoom;
                } else {
                    if (externalMouseFunc !== null) {
                        externalMouseFunc(true, lastLocation);
                    }
                }

                evt.preventDefault();
                return false;
            });

            // Send mouse movement event to the forwarding function
            element.bind('mousemove', function(e) {
                mouseInteract(e);
            });

            // Stop any zoom or pan events
            element.bind('mouseup', function(evt) {
                if (mouseMode === modeNone) {
                    if (externalMouseFunc !== null) {
                        externalMouseFunc(false, null);
                    }
                } else {
                    mouseMode = modeNone;
                    evt.preventDefault();
                }
                evt.preventDefault();
            });
        }

        /*
         * Forwards mouse movement events to either the pan or zoom functions
         * depending on the current mouse mode.
         */
        function mouseInteract(event) {
            if (mouseMode === modePan) {
                var loc = getRelativeLocation(canvas, event);
                handlePan(loc);
            } else if (mouseMode === modeZoom) {
                var loc = getRelativeLocation(canvas, event);
                var deltaY = loc.y - lastLocation.y;
                handleZoom(deltaY * dzScale);
                lastLocation = loc;
            }
        }

        /*
         * Does the actual image panning.  Panning should not mess with the
         * source width or source height, those are fixed by the current zoom
         * level.  Panning should only update the source origin (the x and y
         * coordinates of the upper left corner of the source rectangle).
         */
        function handlePan(loc) {
            // Update the source rectangle origin, but afterwards, check to
            // make sure we're not trying to look outside the image bounds.
            srcOrig[0] -= (loc.x - lastLocation.x) / zoomLevel;
            srcOrig[1] -= (loc.y - lastLocation.y) / zoomLevel;
            fixSrcOrigForImageBounds();

            // Redraw the image in the canvas
            redrawImage();
            lastLocation = loc;
        }

        /*
         * Does the actual image zooming.  Zooming first sets what the source width
         * and height should be based on the zoom level, then adjusts the source
         * origin to try and maintain the source center point.  However, zooming
         * must also not try to view outside the image bounds, so the center point
         * may be changed as a result of this.
         */
        function handleZoom(inOutAmount) {
            zoomLevel += inOutAmount;

            // Disallow zoomLevel outside allowable range
            if (zoomLevel < minZoom) {
                zoomLevel = minZoom;
            } else if (zoomLevel > maxZoom) {
                zoomLevel = maxZoom;
            }

            // Figure out where we were centered (in the source rect) before
            var center = calculateCenter(srcOrig, srcDim);

            // Calculate the new source rectangle width and height
            srcDim[gDim] = vpDim[gDim] / zoomLevel;
            fixSrcDimForAspectRatio();

            // Set up source rectangle origin to maintain the previous center, but
            // afterwards, check the origin to make sure we're not trying to look
            // outside the image bounds.
            srcOrig[0] = center[0] - (srcDim[0] / 2.0);
            srcOrig[1] = center[1] - (srcDim[1] / 2.0);
            fixSrcOrigForImageBounds();

            // Redraw the image in the canvas
            redrawImage();
        }

        /*
         * Convenience function to draw the image.  As a reminder, we always fill
         * the entire viewport.  Also, we always use the source origin and source
         * dimensions that we have calculated and maintain internally.
         */
        function redrawImage() {
            var ctx = canvas[0].getContext("2d");
            ctx.drawImage(img[0],
                          srcOrig[0],   // source image upper left x
                          srcOrig[1],   // source image upper left y
                          srcDim[0],    // source image width
                          srcDim[1],    // source image height
                          0,              // destination canvas upper left x
                          0,              // destination canvas upper left y
                          vpDim[0],     // destination canvas width
                          vpDim[1]);    // destination canvas height
        }

        /*
         * Gets the location of the mouse event relative to the canvas itself.
         */
        function getRelativeLocation(element, mouseEvent) {
            var parentOffset = element.offset();
            var relX = mouseEvent.pageX - parentOffset.left;
            var relY = mouseEvent.pageY - parentOffset.top;
            return { 'x': relX, 'y': relY };
        }

        /*
         * If the viewport may have changed, reset our internal variables
         * and make sure the canvas has the same dimensions.
         */
        function setViewportDimensions() {
            // Get width, height, and aspect ratio of the viewport (size of the box)
            vpDim[0] = container.width();
            vpDim[1] = container.height();
            vpAspect = vpDim[0] / vpDim[1];

            // Set the canvas height and width properties appropriately so that
            // drawing operations are correct.
            canvas.attr("width", vpDim[0]);
            canvas.attr("height", vpDim[1]);

            // Choose the so-called "guide dimension"
            gDim = (vpAspect > imgAspect ? 0 : 1);
        }

        /*
         * When the viewport changes in some way, this function will
         * recalculate aspect ratios and redraw for the current viewport
         * dimensions.
         */
        function recalculateAndRedraw() {
            // Fix the canvas for the viewport dimensions
            setViewportDimensions();

            // The min zoom will probably have changed if the viewport has changed
            minZoom = vpDim[gDim] / imgDim[gDim];

            // Set up initial source rectangle to draw
            srcDim[gDim] = vpDim[gDim] / zoomLevel;
            fixSrcDimForAspectRatio();
            fixSrcOrigForImageBounds();

            if (initialCentering === true) {
                centerViewportOnImage();
                initialCentering = false;
            }

            // Draw the image in the canvas
            redrawImage();
        }

        //
        // Now do some initialization
        //

        // Set up all the events
        setupEvents();

        // Kind of looks nicer if we center the viewport on the image to start with
        initialCentering = true;

        // Fix the canvas for the viewport dimensions
        setViewportDimensions();

        // Just expose a couple of methods that need to be called from outside
        return {
            'recalculateAndRedraw': recalculateAndRedraw,
            'imageLoaded': imageLoaded
        };
    }

    // ------------------------------------------------------------------------

    function createStaticImageViewer(container, func) {
        var imageContainer = $('<img/>', { class: 'image-viewer' }),
        imageCanvas = $('<canvas/>', { class: 'image-canvas' }),
        currentFileToRender = null;
        imageContainer.appendTo(imageCanvas);
        imageCanvas.appendTo(container);

        // Add zoom manager
        var manipMgr = createZoomableCanvasObject(container, imageContainer, imageCanvas, 0, func);
        container.data('zoomManager', manipMgr);

        container.bind('invalidate-size', function() {
            var mgr = container.data('zoomManager');
            mgr.recalculateAndRedraw();
        });
        imageContainer.bind('onload load', function(){
            manipMgr.imageLoaded();
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
        return imageCanvas;
    }

    // ------------------------------------------------------------------------

    function createInteractiveImageViewer(container) {
        var startPosition, dragging = false,
        thetaValues = container.data('info').arguments.theta.values,
        phiValues   = container.data('info').arguments.phi.values,
        stepPhi    = phiValues[1] - phiValues[0];
        stepTheta  = thetaValues[1] - thetaValues[0];
        currentArgs = container.data('active-args'),
        imageCanvas = createStaticImageViewer(container, function(drg, strt) {
            dragging = drg;
            startPosition = strt;
        });

        function getRelativeLocation(element, mouseEvent) {
            var parentOffset = element.parent().offset();
            var relX = mouseEvent.pageX - parentOffset.left;
            var relY = mouseEvent.pageY - parentOffset.top;
            return { 'x': relX, 'y': relY };
        }

        imageCanvas.bind('mousemove', function(event){
            event.preventDefault();
            if(dragging) {
                currentPosition = getRelativeLocation(imageCanvas, event);
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
                    startPosition = getRelativeLocation(imageCanvas, event);
                    fireLoadImage(container);
                    container.trigger('invalidate-viewport');
                }
            }
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
                        createStaticImageViewer(me, null);
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
