(function ($, GLOBAL) {
    var SELECT_OPTION = '<option value="VALUE">NAME</option>',
    TEMPLATE_CANVAS = '<canvas class="front-renderer"></canvas><canvas class="single-size-back-buffer bg"></canvas><canvas class="back-buffer bg"></canvas>',
    TEMPLATE_CONTENT = '<div class="header"><span class="vtk-icon-tools toggle"></span><span class="vtk-icon-resize-full-2 reset"></span><span class="vtk-icon-play play"></span><span class="vtk-icon-stop stop"></span></div><div class="parameters"><div class="layer-selector"></div><div class="pipeline-container"><div class="pipeline"><ul>PIPELINE</ul></div><div class="background">Background<div class="right-control"><ul><li class="color" data-color="#cccccc" style="background: #cccccc"></li><li class="color" data-color="#000000" style="background: #000000"></li><li class="color" data-color="#ffffff" style="background: #ffffff"></li></ul></div></div><div class="fields"><ul><li class="time loop toggle-active"><span class="vtk-icon-clock-1 action title">Time</span><div class="right-control"><span class="value">0</span><span class="vtk-icon-to-start-1 action vcr" data-action="begin"></span><span class="vtk-icon-left-dir action vcr" data-action="previous"></span><span class="vtk-icon-right-dir action vcr" data-action="next"></span><span class="vtk-icon-to-end-1 action vcr" data-action="end"></span></div></li><li class="phi loop toggle-active"><span class="vtk-icon-resize-horizontal-1 action title">Phi</span><div class="right-control"><span class="value">0</span><span class="vtk-icon-to-start-1 action vcr" data-action="begin"></span><span class="vtk-icon-left-dir action vcr" data-action="previous"></span><span class="vtk-icon-right-dir action vcr" data-action="next"></span><span class="vtk-icon-to-end-1 action vcr" data-action="end"></span></div></li><li class="theta toggle-active"><span class="vtk-icon-resize-vertical-1 action title">Theta</span><div class="right-control"><span class="value">0</span><span class="vtk-icon-to-start-1 action vcr" data-action="begin"></span><span class="vtk-icon-left-dir action vcr" data-action="previous"></span><span class="vtk-icon-right-dir action vcr" data-action="next"></span><span class="vtk-icon-to-end-1 action vcr" data-action="end"></span></div></li></ul></div></div></div>',
    PIPELINE_ENTRY = '<li class="show enabled" data-id="ID"><span class="FRONT_ICON action"></span><span class="label">LABEL</span>CONTROL</li>',
    DIRECTORY_CONTROL = '<span class="vtk-icon-plus-circled right-control action select-layer"></span><ul>CHILDREN</ul>',
    TEMPLATE_SELECTOR = '<div class="head"><span class="title">TITLE</span><span class="vtk-icon-ok action right-control validate-layer"></span></div><ul>LIST</ul>',
    TEMPLATE_LAYER_CHECK = '<li><input type="checkbox" CHECKED name="ID">NAME</li>';



    // ========================================================================
    // Helper method
    // ========================================================================

    function getRelativeLocation(element, mouseEvent) {
        var parentOffset = element.offset(),
        x = mouseEvent.pageX || mouseEvent.originalEvent.pageX || mouseEvent.originalEvent.mozMovementX,
        y = mouseEvent.pageY || mouseEvent.originalEvent.pageY || mouseEvent.originalEvent.mozMovementY,
        relX = x - parentOffset.left,
        relY = y - parentOffset.top;
        return [ relX, relY ];
    }

    // ------------------------------------------------------------------------

    function createZoomableCanvasObject(container, bgCanvas, frontCanvas, pixelZoomRatio, stepPhi, stepTheta) {
        // First set up some variables we will need
        var modeRotation = 1,   // when dragging, it's a rotation
        modePan = 2,        // when dragging, it's a pan
        modeZoom = 3,           // when dragging, it's a zoom
        modeNone = 0,           // No mouse move handling
        mouseMode = modeNone,   // Current mode

        dzScale = 0.005,  // scaling factor to control how fast we zoom in and out
        wheelZoom = 0.05, // amount to change zoom with each wheel event

        drawingCenter = [0,0],  // Drawing parameters
        zoomLevel = 1.0,        //

        maxZoom = pixelZoomRatio, // limit how far we can zoom in
        minZoom = 1 / maxZoom,    // limit how far we can zoom out

        lastLocation = [0,0];  // Last place mouse event happened

        if(stepPhi === 0) {
            modeRotation = modePan;
        }

        /*
         * Adds mouse event handlers so that we can pan and zoom the image
         */
        function setupEvents() {
            var element = frontCanvas;

            // Needed this to override context menu behavior
            element.bind('contextmenu', function(evt) { evt.preventDefault(); });

            // Wheel should zoom across browsers
            element.bind('DOMMouseScroll mousewheel', function (evt) {
                var x = (-evt.originalEvent.wheelDeltaY || evt.originalEvent.detail);

                lastLocation = getRelativeLocation(frontCanvas, evt);
                handleZoom((x > 0 ? wheelZoom : x < 0 ? -wheelZoom : 0));
                evt.preventDefault();

                // Redraw the image in the canvas
                redrawImage();
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

                // Detect interaction mode
                switch(current_button) {
                    case 2: // middle mouse down = pan
                        mouseMode = modePan;
                        break;
                    case 3: // right mouse down = zoom
                        mouseMode = modeZoom;
                        break;
                    default:
                        mouseMode = modeRotation;
                        break;
                }

                // Store mouse location
                lastLocation = getRelativeLocation(frontCanvas, evt);

                evt.preventDefault();
            });

            // Send mouse movement event to the forwarding function
            element.bind('mousemove', function(e) {
                if(mouseMode != modeNone) {
                    var loc = getRelativeLocation(frontCanvas, e);

                    // Can NOT use switch as (modeRotation == modePan) is
                    // possible when Pan should take over rotation as
                    // rotation is not possible
                    if(mouseMode === modePan) {
                        handlePan(loc);
                    } else if (mouseMode === modeZoom) {
                        var deltaY = loc[1] - lastLocation[1];
                        handleZoom(deltaY * dzScale);

                        // Update mouse location
                        lastLocation = loc;
                    } else {
                       handleRotation(loc);
                    }

                    // Redraw the image in the frontCanvas
                    redrawImage();
                }
            });

            // Stop any zoom or pan events
            element.bind('mouseup', function(evt) {
                mouseMode = modeNone;
                evt.preventDefault();
            });
        }

        /*
         * If the data can rotate
         */
        function handleRotation(loc) {
            var deltaPhi = (loc[0] - lastLocation[0]),
            deltaTheta = (loc[1] - lastLocation[1]),
            changeDetected = false;

            if(Math.abs(deltaPhi) > stepPhi) {
                changeDetected = true;
                if(deltaPhi > 0) {
                    $('.phi span[data-action="next"]', container).trigger('click');
                } else {
                    $('.phi span[data-action="previous"]', container).trigger('click');
                }
            }

            if(Math.abs(deltaTheta) > stepTheta) {
                changeDetected = true;
                if(deltaTheta > 0) {
                    $('.theta span[data-action="next"]', container).trigger('click');
                } else {
                    $('.theta span[data-action="previous"]', container).trigger('click');
                }
            }

            if(changeDetected) {
                // Update mouse location
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
            drawingCenter[0] += (loc[0] - lastLocation[0]);
            drawingCenter[1] += (loc[1] - lastLocation[1]);

            // Update mouse location
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
            var beforeZoom = zoomLevel,
            afterZoom = beforeZoom + inOutAmount;

            // Disallow zoomLevel outside allowable range
            if (afterZoom < minZoom) {
                afterZoom = minZoom;
            } else if (afterZoom > maxZoom) {
                afterZoom = maxZoom;
            }

            if(beforeZoom != afterZoom) {
                zoomLevel = afterZoom;
                // FIXME ----------------------------------------------------------------
                // zoom by keeping location of "lastLocation" in the same screen position
                // FIXME ----------------------------------------------------------------
            }
        }

        /*
         * Convenience function to draw the image.  As a reminder, we always fill
         * the entire viewport.  Also, we always use the source origin and source
         * dimensions that we have calculated and maintain internally.
         */
        function redrawImage() {
            var frontCtx = frontCanvas[0].getContext("2d"),
            w = container.width(),
            h = container.height(),
            iw = bgCanvas[0].width,
            ih = bgCanvas[0].height;

            if(iw === 0) {
                setTimeout(redrawImage, 100);
            } else {
                frontCanvas.attr("width", w);
                frontCanvas.attr("height", h);
                frontCtx.clearRect(0, 0, w, h);

                var tw = Math.floor(iw*zoomLevel),
                th = Math.floor(ih*zoomLevel),
                tx = drawingCenter[0] - (tw/2),
                ty = drawingCenter[1] - (th/2),
                dx = (tw > w) ? (tw - w) : (w - tw),
                dy = (th > h) ? (th - h) : (h - th),
                centerBounds = [ (w-dx)/2 , (h-dy)/2, (w+dx)/2, (h+dy)/2 ];

                if( drawingCenter[0] < centerBounds[0] || drawingCenter[0] > centerBounds[2]
                    || drawingCenter[1] < centerBounds[1] || drawingCenter[1] > centerBounds[3] ) {
                    drawingCenter[0] = Math.min( Math.max(drawingCenter[0], centerBounds[0]), centerBounds[2] );
                    drawingCenter[1] = Math.min( Math.max(drawingCenter[1], centerBounds[1]), centerBounds[3] );
                    tx = drawingCenter[0] - (tw/2);
                    ty = drawingCenter[1] - (th/2);
                }

                frontCtx.drawImage(bgCanvas[0],
                              0,   0, iw, ih,  // Source image   [Location,Size]
                              tx, ty, tw, th); // Traget drawing [Location,Size]
            }
        }

        /*
         * Make sure the image will fit inside container as ZoomOut
         */

        function resetCamera() {
            var w = container.width(),
            h = container.height(),
            iw = bgCanvas[0].width,
            ih = bgCanvas[0].height;

            if(iw === 0) {
                setTimeout(resetCamera, 100);
            } else {
                zoomLevel = minZoom = Math.min( w / iw, h / ih );
                drawingCenter[0] = w/2;
                drawingCenter[1] = h/2;
                redrawImage();
            }
        }

        // Now do some initialization
        setupEvents();
        resetCamera();

        // Just expose a couple of methods that need to be called from outside
        return {
            'resetCamera': resetCamera,
            'paint': redrawImage
        };
    }

    // ------------------------------------------------------------------------

    function createCompositeManager(container, basepath, info, nbImages) {
        var activeQuery = "",
        pathPattern = info.name_pattern,
        activeKey = null,
        cache = {},
        orderMapping = {},
        layerOffset = null,
        offsetMap = info.metadata.offset,
        singleImageSize = info.metadata.dimensions,
        fullImageSize = [ singleImageSize[0], singleImageSize[1] * nbImages],
        bgColor = null;

        // Add UI components to container
        $('<div/>', {
            class: 'composite-view',
            html: TEMPLATE_CANVAS
        }).appendTo(container);

        var bgCanvas = $('.back-buffer', container),
        frontCanvas = $('.single-size-back-buffer', container),
        bgCTX = bgCanvas[0].getContext('2d'),
        frontCTX = frontCanvas[0].getContext('2d');

        // Update bg canvas size to match image size
        bgCanvas.attr('width', fullImageSize[0]).attr('height', fullImageSize[1]);
        frontCanvas.attr('width', singleImageSize[0]).attr('height', singleImageSize[1]);

        // Create helper methods
        // -----------------------------------------
        function downloadImage(key, url) {
            var img = new Image();

            function onLoad() {
                cache[key]['image'] = img;
                if(cache[key].hasOwnProperty('json')) {
                    draw();
                }
            }

            function onError() {
                console.log('Error loading image ' + url + ' for key ' + key);
            }

            img.onload = onLoad;
            img.onerror = onError;
            img.src = url;
            if (img.complete) {
                onLoad();
            }
        }

        // -----------------------------------------

        function downloadComposite(key, url) {
            jQuery.getJSON(url, function(data){
                // Process composite
                var composite = data["pixel-order"].split('+'),
                count = composite.length;
                while(count--) {
                    var str = composite[count];
                    if(str.startsWith('@')) {
                        composite[count] = Number(str.substr(1))
                    } else {
                        if(!orderMapping.hasOwnProperty(str)) {
                            // Compute offset
                            orderMapping[str] = computeOffset(str);
                        }
                    }
                }

                cache[key]['composite'] = composite;
                cache[key]['json'] = data;
                if(cache[key].hasOwnProperty('image')) {
                    draw();
                }
            });
        }

        // -----------------------------------------

        function updateColor(color) {
            bgColor = color;
            draw();
        }

        // -----------------------------------------

        function updateFields(time, phi, theta) {
            activeKey = pathPattern.replace('{time}', time).replace('{phi}', phi).replace('{theta}', theta);

            if(!cache.hasOwnProperty(activeKey)) {
                // Trigger download
                cache[activeKey] = {};
                downloadImage(activeKey, basepath + '/' + activeKey.replace('{filename}', 'rgb.jpg'));
                downloadComposite(activeKey, basepath + '/' + activeKey.replace('{filename}', 'composite.json'));
            } else {
                draw();
            }
        }

        // -----------------------------------------

        function computeOffset(order) {
            var count = order.length;
            for(var i = 0; i < count; ++i) {
                var offset = layerOffset[order[i]];
                if(offset > -1) {
                    return offset;
                }
            }
            return -1;
        }

        // -----------------------------------------

        function computeLayerOffset(query) {
            var count = query.length;
            layerOffset = {};

            for(var i = 0; i < count; i += 2) {
                var layer = query[i],
                field = query[i+1];

                if(field === '_') {
                   layerOffset[layer] = -1;
                } else {
                    layerOffset[layer] = nbImages - offsetMap[query.substr(i,2)] - 1;
                }
            }
        }

        // -----------------------------------------

        function updatePipeline(query) {
            if(activeQuery !== query) {
                activeQuery = query;

                // Update current offset for each layer
                computeLayerOffset(query);

                // Loop over all possible order and compute offset
                for(var order in orderMapping) {
                    orderMapping[order] = computeOffset(order);
                }

                // Render result
                draw();
            }
        }

        // -----------------------------------------

        function draw() {
            if(!cache.hasOwnProperty(activeKey) || !cache[activeKey].hasOwnProperty('composite') || !cache[activeKey].hasOwnProperty('image')) {
                return;
            }
            var composite = cache[activeKey]['composite'],
            img = cache[activeKey]['image'],
            localOrder = orderMapping,
            fullPixelOffset = singleImageSize[0] * singleImageSize[1] * 4,
            count = composite.length;

            // Fill buffer with image
            bgCTX.drawImage(img, 0, 0);

            var pixelBuffer = bgCTX.getImageData(0, 0, fullImageSize[0], fullImageSize[1]).data,
            frontBuffer = null, frontPixels = null, pixelIdx = 0, localIdx;

            // Fill with bg color
            if(bgColor) {
                frontCTX.fillStyle = bgColor;
                frontCTX.fillRect(0,0,singleImageSize[0], singleImageSize[1]);
                frontBuffer = frontCTX.getImageData(0, 0, singleImageSize[0], singleImageSize[1]);
                frontPixels = frontBuffer.data;
            } else {
                frontBuffer = bgCTX.getImageData(0, (nbImages - 1) * singleImageSize[1], singleImageSize[0], singleImageSize[1]);
                frontPixels = frontBuffer.data;
            }

            for(var i = 0; i < count; ++i) {
                var order = composite[i];
                if(order > 0) {
                    pixelIdx += order;
                } else {
                    var offset = localOrder[order];

                    if(offset > -1) {
                        localIdx = 4 * pixelIdx;
                        offset *= fullPixelOffset;
                        offset += localIdx;
                        frontPixels[ localIdx     ] = pixelBuffer[ offset     ];
                        frontPixels[ localIdx + 1 ] = pixelBuffer[ offset + 1 ];
                        frontPixels[ localIdx + 2 ] = pixelBuffer[ offset + 2 ];
                        frontPixels[ localIdx + 3 ] = 255;
                    }
                    // Move forward
                    ++pixelIdx;
                }
            }

            // Draw buffer to canvas
            frontCTX.putImageData(frontBuffer, 0, 0);
            container.trigger('render-bg');
        }

        return {
            updateFields:updateFields,
            draw:draw,
            updatePipeline:updatePipeline,
            updateColor:updateColor
        };
    }

    // ------------------------------------------------------------------------

    function createSelectColorBy(availableFields, fields) {
        var buffer = [ "<select class='right-control'>" ],
        count = availableFields.length,
        value = null;

        if(count < 2) {
            buffer = ["<select class='right-control' style='display: none;'>"];
        }

        while(count--) {
            value = availableFields[count];
            buffer.push(SELECT_OPTION.replace(/VALUE/g, value).replace(/NAME/g, fields[value]))
        }

        buffer.push("</select>");
        return buffer.join('');
    }

    // ------------------------------------------------------------------------

    function encodeEntry(entry, layer_fields, fields) {
        var controlContent = "";

        if(entry['type'] === 'directory') {
            var array = entry['children'],
            count = array.length;
            for(var i = 0; i < count; ++i) {
                controlContent += encodeEntry(array[i], layer_fields, fields).replace(/FRONT_ICON/g, 'vtk-icon-cancel-circled remove');
            }
            controlContent = DIRECTORY_CONTROL.replace(/CHILDREN/g, controlContent);
        } else {
           controlContent = createSelectColorBy(layer_fields[entry['ids'][0]], fields);
        }

        return PIPELINE_ENTRY.replace(/ID/g, entry['ids'].join(':')).replace(/LABEL/g, entry['name']).replace(/CONTROL/g, controlContent);
    }

    // ------------------------------------------------------------------------

    function createControlPanel(container, pipeline, layer_fields, fields) {
        var pipelineBuffer = [], count = pipeline.length;

        // Build pipeline content
        for(var i = 0; i < count; ++i) {
            pipelineBuffer.push(encodeEntry(pipeline[i], layer_fields, fields).replace(/FRONT_ICON/g, 'vtk-icon-eye toggle-eye'));
        }

        $('<div/>', {
            class: 'control',
            html: TEMPLATE_CONTENT.replace(/PIPELINE/g, pipelineBuffer.join(''))
        }).appendTo(container);

        $('li > ul > li', container).removeClass('enabled').hide();
    }

    // ------------------------------------------------------------------------

    function initializeListeners(container, manager, zoomableRender) {
        var layers = container.data('layers'),
        animationWorkIndex = 0,
        play = $('.play', container),
        stop = $('.stop', container),
        keepAnimation = false;

        function animate() {
            $('.active .vcr[data-action="next"]', container).trigger('click');
            if(keepAnimation) {
                setTimeout(animate, 200);
            }
        }

        function updatePipeline() {
            var query = "";

            for(var i in layers) {
                layer = layers[i];
                query += layer;
                var layerContainer = $('li[data-id="'+layer+'"]', container);
                if(layerContainer.hasClass('show') && layerContainer.hasClass('enabled')) {
                    query += $('select:eq(0)', layerContainer).val();
                } else {
                    query += '_';
                }
            }

            manager.updatePipeline(query);
        }

        function extractFieldValue(fieldContainer) {
            return fieldContainer.attr('data-values').split(':')[Number(fieldContainer.attr('data-index'))];
        }

        function updateComposite() {
            var time = extractFieldValue($('.time', container)),
            phi = extractFieldValue($('.phi', container)),
            theta = extractFieldValue($('.theta', container));
            manager.updateFields(time, phi, theta);
        }

        $('.color', container).click(function(){
            var me = $(this),
            hasColor = me.hasClass('active'),
            color = me.attr('data-color');

            if(!hasColor) {
                $('.color', me.parent()).removeClass('active');
            }
            me.toggleClass('active');

            manager.updateColor(hasColor ? null : color);
        });

        $('.toggle', container).click(function(){
            container.toggleClass('small');
        });

        $('.toggle-active', container).click(function(){
            $(this).toggleClass('active');
        });

        $('.reset', container).click(function(){
            zoomableRender.resetCamera();
        });

        $('.toggle-eye', container).click(function(){
            var me = $(this),
            isVisible = me.hasClass('vtk-icon-eye'),
            all = $('li', me.parent());
            me.removeClass('vtk-icon-eye vtk-icon-eye-off')
              .addClass(isVisible ? 'vtk-icon-eye-off' : 'vtk-icon-eye');

            // Update class for pipeline
            all.removeClass('show');
            me.parent().removeClass('show');
            if(!isVisible) {
                all.addClass('show');
                me.parent().addClass('show');
            }

            updatePipeline();
        });

        $('.remove', container).click(function(){
            $(this).parent().removeClass('enabled').hide();
            updatePipeline();
        });

        $('.vcr', container).click(function(){
            var me = $(this),
            action = me.attr('data-action'),
            root = me.closest('li'),
            idx = Number(root.attr('data-index')),
            size = Number(root.attr('data-size')),
            values = root.attr('data-values').split(':'),
            valueContainer = $('.value', root),
            canLoop = root.hasClass('loop'),
            changeFound = false;

            root.toggleClass('active');
            switch(action) {
                case 'begin':
                    idx = 0;
                break;
                case 'previous':
                    if(canLoop || idx > 0) {
                        idx = (idx + size - 1) % size;
                        changeFound = true;
                    }
                break;
                case 'next':
                    if(canLoop || idx + 1 < size) {
                        idx = (idx + 1) % size;
                        changeFound = true;
                    }
                break;
                case 'end':
                    idx = size -1;
                break;
            }
            root.attr('data-index', idx);
            valueContainer.html(values[idx]);

            if(changeFound) {
                updateComposite();
            }
        });

        $('select', container).change(updatePipeline);

        $('.select-layer', container).click(function(){
            var me = $(this),
            pipelineContainer = $('.pipeline-container', container),
            layerSelector = $('.layer-selector', container),
            title = me.parent().children('span.label:eq(0)').html(),
            buffer = [];

            $('span.label', me.parent().children('ul')).each(function(){
                var me = $(this);
                buffer.push(TEMPLATE_LAYER_CHECK.replace(/ID/g, me.parent().attr('data-id')).replace(/NAME/g, me.html()).replace(/CHECKED/g, me.is(":visible") ? "checked=''" : ""));
            });

            layerSelector.empty()[0].innerHTML = TEMPLATE_SELECTOR.replace(/TITLE/g, title).replace(/LIST/g, buffer.join(''));

            // add listeners
            $('.validate-layer', layerSelector).click(function(){
                pipelineContainer.show();
                layerSelector.hide();

                updatePipeline();
            });
            $('input', layerSelector).change(function(){
                var me = $(this),
                checked = me.is(':checked'),
                id = me.attr('name'),
                item = $('li[data-id="' + id + '"]', container);

                if(checked) {
                    item.addClass("enabled").show();
                } else {
                    item.removeClass("enabled").hide();
                }
            });

            pipelineContainer.hide();
            layerSelector.show();
        });

        play.click(function(){
            animationWorkIndex = 0;
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

        // Forward render call to front buffer
        container.bind('render-bg', function(){
            zoomableRender.paint();
        });

        // Process current config
        updatePipeline();
        updateComposite();
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystCompositeViewer = function(dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().addClass('vtkweb-catalyst-composite small');

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Extract info to local vars
                    var layer_fields = data.metadata.layer_fields,
                    fields = data.metadata.fields,
                    pipeline = data.metadata.pipeline,
                    args = data.arguments,
                    nbImages = 1,
                    deltaPhi = (args.hasOwnProperty('phi')) ? (Number(args.phi.values[1]) - Number(args.phi.values[0])) : 0,
                    deltaTheta = (args.hasOwnProperty('theta')) ? (Number(args.theta.values[1]) - Number(args.theta.values[0])) : 0;

                    // Compute number of images
                    for(var key in layer_fields) {
                        nbImages += layer_fields[key].length;
                    }

                    // Keep data info
                    me.data('basepath', dataBasePath);
                    me.data('layers', data.metadata.layers);

                    // Add control UI
                    createControlPanel(me, pipeline, layer_fields, fields);

                    // Add rendering view
                    var manager = createCompositeManager(me, dataBasePath, data, nbImages);
                    me.data('compositeManager', manager);



                    var zoomableRender = createZoomableCanvasObject(me, $('.single-size-back-buffer', me), $('.front-renderer', me), 10, deltaPhi, deltaTheta);
                    me.data('zoomableRender', zoomableRender);

                    // Enable additional fields if any (time, phi, theta)
                    for(var key in args) {
                        var fieldContainer = $('.' + key, me);
                        if(fieldContainer) {
                            fieldContainer.attr('data-values', args[key].values.join(':')).attr('data-index', '0').attr('data-size', args[key].values.length);
                            fieldContainer.show();
                        }
                    }

                    // Attach interaction listeners
                    initializeListeners(me, manager, zoomableRender);
                },
                error: function(error) {
                    console.log("error when trying to download " + dataBasePath + '/info.json');
                    console.log(error);
                }
            });
        });
    }

}(jQuery, window));