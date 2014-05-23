(function ($, GLOBAL) {
    var SELECT_OPTION = '<option value="VALUE">NAME</option>',
    TEMPLATE_CANVAS = '<canvas class="front-renderer"></canvas><canvas class="single-size-back-buffer bg"></canvas><canvas class="back-buffer bg"></canvas>',
    TEMPLATE_CONTENT = '<div class="header"><span class="vtk-icon-tools toggle"></span><span class="vtk-icon-resize-full-2 reset"></span><span class="vtk-icon-play play"></span><span class="vtk-icon-stop stop"></span></div><div class="parameters"><div class="layer-selector"></div><div class="pipeline-container"><div class="pipeline"><ul>PIPELINE</ul></div><div class="background">Background<div class="right-control"><ul><li class="color" data-color="#cccccc" style="background: #cccccc"></li><li class="color" data-color="#000000" style="background: #000000"></li><li class="color" data-color="#ffffff" style="background: #ffffff"></li></ul></div></div><div class="fields"><ul><li class="time loop toggle-active"><span class="vtk-icon-clock-1 action title">Time</span><div class="right-control"><span class="value">0</span><span class="vtk-icon-to-start-1 action vcr" data-action="begin"></span><span class="vtk-icon-left-dir action vcr" data-action="previous"></span><span class="vtk-icon-right-dir action vcr" data-action="next"></span><span class="vtk-icon-to-end-1 action vcr" data-action="end"></span></div></li><li class="phi loop toggle-active"><span class="vtk-icon-resize-horizontal-1 action title">Phi</span><div class="right-control"><span class="value">0</span><span class="vtk-icon-to-start-1 action vcr" data-action="begin"></span><span class="vtk-icon-left-dir action vcr" data-action="previous"></span><span class="vtk-icon-right-dir action vcr" data-action="next"></span><span class="vtk-icon-to-end-1 action vcr" data-action="end"></span></div></li><li class="theta toggle-active"><span class="vtk-icon-resize-vertical-1 action title">Theta</span><div class="right-control"><span class="value">0</span><span class="vtk-icon-to-start-1 action vcr" data-action="begin"></span><span class="vtk-icon-left-dir action vcr" data-action="previous"></span><span class="vtk-icon-right-dir action vcr" data-action="next"></span><span class="vtk-icon-to-end-1 action vcr" data-action="end"></span></div></li><li class="compute-coverage action"><span>Compute pixel coverage</span><div class="right-control"><span class="vtk-icon-sort-alt-down"/></div></li><li class="progress"><div></div></li></ul></div></div></div>',
    PIPELINE_ENTRY = '<li class="show enabled" data-id="ID"><span class="FRONT_ICON action"></span><span class="label">LABEL</span>CONTROL</li>',
    DIRECTORY_CONTROL = '<span class="vtk-icon-plus-circled right-control action select-layer"></span><ul>CHILDREN</ul>',
    TEMPLATE_SELECTOR = '<div class="head"><span class="title">TITLE</span><span class="vtk-icon-ok action right-control validate-layer"></span></div><ul>LIST</ul>',
    TEMPLATE_LAYER_CHECK = '<li><input type="checkbox" CHECKED name="ID">NAME</li>',
    // Extract worker url
    scripts = document.getElementsByTagName('script'),
    scriptPath = scripts[scripts.length - 1].src.split('/'),
    workerURL = '/CatalystBrowser/vtkweb-composite-worker.js',
    NB_RESULT_PER_PAGE = 8;


    // Compute worker path
    scriptPath.pop();
    workerURL = scriptPath.join('/') + workerURL;

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

    function createSearchManager(container, data, basepath) {
        var layerVisibility = {},
            layer_fields = data.metadata.layer_fields,
            fields = data.metadata.fields,
            pipeline = data.metadata.pipeline,
            args = data.arguments,
            idList = [],
            dataList = [],
            result = [],
            layerOlderInvalid = true,
            filePattern = data.name_pattern.replace(/{filename}/g, 'query.json'),
            timeList = args.hasOwnProperty('time') ? args.time.values : ["0"],
            phiList = args.hasOwnProperty('phi') ? args.phi.values : ["0"],
            thetaList = args.hasOwnProperty('theta') ? args.theta.values : ["0"],
            timeCount = timeList.length,
            phiCount = phiList.length,
            thetaCount = thetaList.length,
            workerList = [],
            roundRobinWorkerId = 0,
            formulaSTR = "",
            layerToLabel = {},
            nbImages = 1,
            renderManager = null,
            layerSeriesData = {},
            palette = new Rickshaw.Color.Palette();

        // Compute number of images
        for(var key in layer_fields) {
            nbImages += layer_fields[key].length;
        }
        renderManager = createCompositeManager(container, basepath, data, nbImages);

        // Compute layer to label mapping
        function updateLayerToLabel(item) {
            if(item.type === 'layer') {
                layerToLabel[item.ids[0]] = item.name;
                layerSeriesData[item.ids[0]] = [];
            } else {
                for(var idx in item.children) {
                    updateLayerToLabel(item.children[idx]);
                }
            }
        }
        for(var idx in pipeline) {
            updateLayerToLabel(pipeline[idx]);
        }

        // Create workers
        var nbWorker = 5;
        while(nbWorker--) {
            var w = new Worker(workerURL);
            w.onmessage = processResults;
            workerList.push(w)
        }

        // ------------------------------

        function processResults(event) {
            result.push(event.data);

            var nb = $('.result-founds', container);
            nb.html( Number(nb.html()) + 1);

            if(result.length === idList.length) {
                updateGraph();
                working(false);
            }
        }

        // ------------------------------

        function updateWorkers(query) {
            var wQuery = '_' + query,
            count = workerList.length;
            while(count--) {
                workerList[count].postMessage(wQuery);
            }
        }

        // ------------------------------

        function triggerWork() {
            var count = workerList.length;
            while(count--) {
                workerList[count].postMessage('w');
            }
        }

        // ------------------------------

        function sendData(id, fields, orderCount) {
            workerList[roundRobinWorkerId].postMessage('d' + id + '|' + JSON.stringify(fields) + '|' + JSON.stringify(orderCount));
            roundRobinWorkerId = (roundRobinWorkerId + 1) % workerList.length;
        }

        // ------------------------------

        function sendNumberOfPrixel(number) {
            var sNumber = 's' + number,
            count = workerList.length;
            while(count--) {
                workerList[count].postMessage(sNumber);
            }
        }

        // ------------------------------

        function fetchData(url, fields, count) {
            $.ajax({
                url: url,
                dataType: 'json',
                success: function( data ) {
                    sendData(url, fields, data.counts);
                    if(count == 0) {
                        sendNumberOfPrixel(data.dimensions[0] * data.dimensions[1]);
                    }
                    // Fetch next one
                    processDataList();
                },
                error: function(error) {
                    console.log("error when trying to download " + url);
                    console.log(error);
                }
            });
        }

        // ------------------------------

        var processIdx = 0,
        progressBar = $('.progress > div', container);

        $('.progress', container).show();

        function processDataList() {
            if(processIdx < idList.length) {
                fetchData(idList[processIdx], dataList[processIdx], processIdx);
                processIdx++;
                progressBar.css('width', Math.floor(95*processIdx / idList.length) + '%');

                // Update page number and possible results
                $('.result-count', container).html("Found&nbsp;VALUE&nbsp;results.".replace(/VALUE/g, idList.length));
                $('.result-page-number', container).html(Math.floor(idList.length / NB_RESULT_PER_PAGE) + 1);
            } else {
                $('.compute-coverage', container).show();
                progressBar.parent().hide();

                // Update page number and possible results
                $('.result-count', container).html("Found&nbsp;VALUE&nbsp;results.".replace(/VALUE/g, idList.length));
                $('.result-page-number', container).html(Math.floor(idList.length / NB_RESULT_PER_PAGE) + 1);
            }
        }

        // ------------------------------

        // Generate image list
        while(timeCount--) {
            thetaCount = thetaList.length;
            var baseURL = basepath + '/' + filePattern.replace(/{time}/g, timeList[timeCount]);
            while(thetaCount--) {
                phiCount = phiList.length;
                var currentURL = baseURL.replace(/{theta}/g, thetaList[thetaCount]);
                while(phiCount--) {
                    var url = currentURL.replace(/{phi}/g, phiList[phiCount]);
                    dataList.push({ time: timeList[timeCount], phi: phiList[phiCount], theta: thetaList[thetaCount]});
                    idList.push(url);
                    if(idList.length === 1) {
                        processDataList();
                    }
                }
            }
        }

        // ------------------------------

        function search() {
            result = [];
            working(true);
            $('.result-founds', container).html('0');
            triggerWork();
        }

        // ------------------------------

        function updateGraph() {
            var count = result.length, series = [];

            // Update serie data
            for(var layer in layerSeriesData) {
                layerSeriesData[layer] = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
            }
            while(count--) {
                var entry = result[count].count;
                for(var layer in entry) {
                    if(layer !== '+') {
                        var percent = Math.ceil(entry[layer]);
                        layerSeriesData[layer][percent]++;
                    }
                }
            }
            // Find max with no coverage
            var max = 0;
            for(var layer in layerSeriesData) {
                count = 101;
                while(count-- && layerSeriesData[layer][count] === 0) {
                    // Just wait to find the right index
                }
                if(max < count) {
                    max = count;
                }
            }
            max += 1;

            for(var layer in layerSeriesData) {
                var name = '(' + layer + ') ' + layerToLabel[layer],
                data = [],
                color = palette.color(),
                hasData = false;

                for(var x = 1; x < max; ++x) {
                    data.push({x:x, y:layerSeriesData[layer][x]});
                    if(layerSeriesData[layer][x] > 0) {
                        hasData = true;
                    }
                }
                if(hasData) {
                    series.push({ name:name, data:data, color:color });
                }
            }

            // Create brand new graph
            var options = {
                'renderer': 'area',  // Type of chart [line, area, bar, scatterplot]
                'stacked' : true,
                'series': series,
                'axes': [ "bottom", "left", "top"], // Draw axis on border with scale
                'chart-padding': [0, 100, 0, 0],   // Graph padding [top, right, bottom, left] in px. Useful to save space for legend
            };
            $('.chart-container', container).empty().vtkChart(options);
        }


        function applyQuery(userQuery) {
            $('.toggle-stats', container).removeClass('stats');
            // Generate function str
            // Extract forumla
            formulaSTR = "var time = Number(obj.fields.time), phi = Number(obj.fields.phi), theta = Number(obj.fields.theta);\n";
            for(var layer in layerVisibility) {
                if(layerVisibility[layer]) {
                    formulaSTR += layer + ' = obj.count.' + layer + ';\n';
                }
            }

            var sortQuery = $('.sortby-expression', container).val(),
            sortFunctionSTR = "function extractValue(obj) {" + formulaSTR + "return " + sortQuery + ";}; return extractValue(a) - extractValue(b);";

            formulaSTR += "return true";
            if(userQuery.trim().length > 0) {
                formulaSTR += " && " + userQuery + ';';
            } else {
                formulaSTR += ';';
            }

            // Compile formula and run it on results
            var func = new Function("obj", formulaSTR),
            count = result.length,
            found = 0,
            finalResults = []
            sortFunc = new Function(["a","b"], sortFunctionSTR);

            $('.result-count', container).html('Found&nbsp;0&nbsp;result.');

            while(count--) {
                if(func(result[count])) {
                   found++;
                   finalResults.push(result[count]);
                }
            }

            if(sortQuery.length > 0) {
                finalResults.sort(sortFunc);
            }

            $('.result-count', container).html("Found&nbsp;VALUE&nbsp;results.".replace(/VALUE/g, found));

            var resultContainer = $('.composite-search-results', container).empty();
            count = finalResults.length,
            nbPages = Math.floor(count / NB_RESULT_PER_PAGE),
            pages = [],
            resultNumber = 0;

            if(count % NB_RESULT_PER_PAGE) {
                ++nbPages;
            }

            $('.result-page-number', container).html(nbPages);

            for(var idx = 0; idx < nbPages; ++idx) {
                var page = $('<div/>', { "class": 'result-page', "data-page": idx });
                page.appendTo(resultContainer);
                pages.push(page);
            }
            pages[0].addClass('active');

            while(count--) {
                addResultToUI(finalResults[count], pages[Math.floor(resultNumber++ / NB_RESULT_PER_PAGE)]);
            }

            // Add render callback
            // $('.composite-result', container).click(function(){
            //     var me = $(this), fields = me.attr('data-fields').split(':'),
            //     time = fields[0], phi = fields[1], theta = fields[2],
            //     imgURL = drawResult(time, phi, theta);
            //     $('img', me).attr('src', imgURL);
            // });

            $('.composite-result', container).dblclick(function(){
                var me = $(this), fields = me.attr('data-fields').split(':');

                var colorContainer = $('.color.active', container);
                if(colorContainer) {
                    container.trigger({
                        type: "open-view",
                        query: container.data('pipeline-query'),
                        args: {
                            time : fields[0],
                            phi : fields[1],
                            theta : fields[2]
                        },
                        color: colorContainer.attr('data-color')
                    });
                } else {
                    me.trigger({
                        type: "open-view",
                        query: container.data('pipeline-query'),
                        args: {
                            time : fields[0],
                            phi : fields[1],
                            theta : fields[2]
                        },
                    });
                }
            });
            renderActivePage();
        }

        // ------------------------------

        function renderActivePage(){
            var processingQueue = [];
            function renderCompositeResult() {
                if(processingQueue.length > 0) {
                    // [ container, time, phi, theta ]
                    var item = processingQueue.pop(),
                    time = item[1], phi = item[2], theta = item[3];
                    if(renderManager.updateFields(time, phi, theta)) {
                        item[0].attr('src', drawResult(time, phi, theta));
                        item[4].hide();
                    } else {
                        // Not ready yet
                        processingQueue.push(item);
                    }

                    // Process remaining
                    setTimeout(renderCompositeResult, 50);
                }
            }
            $('.result-page.active .composite-result', container).each(function(){
                var me = $(this), fields = me.attr('data-fields').split(':'),
                time = fields[0], phi = fields[1], theta = fields[2];
                processingQueue.push([$('img', me), time, phi, theta, $('ul', me)]);
            });
            processingQueue.reverse();
            $('.result-page-index', container).val(1+Number($('.result-page.active').attr('data-page')));
            renderCompositeResult();
        }

        // ------------------------------

        function addResultToUI(obj, container) {
            var buffer = [], dataFields = "";

            dataFields += obj.fields['time'] + ':' + obj.fields['phi'] + ':' + obj.fields['theta'];
            for(var field in obj.fields){
                buffer.push(field + ': ' + obj.fields[field]);
            }
            for(var layer in obj.count){
                if(layer != '+' && obj.count[layer] > 0) {
                    buffer.push(layerToLabel[layer] + ': ' + Number(obj.count[layer]).toFixed(2) + ' %');
                }
            }
            $("<div/>", {
                class: 'composite-result',
                'data-fields': dataFields,
                html: "<ul><li>" + buffer.join('</li><li>') + "</li></ul><img style='width: 100%;'/>"
            }).appendTo(container);
        }

        // ------------------------------

        function updateFields(time, phi, theta) {
            //console.log('search updateFields: ' + time + " " + phi + " " + theta);
        }

        // ------------------------------

        function working(working) {
            $('.working', container).css('visibility', working ? 'visible' : 'hidden');
            if(working) {
                $('.search-action[data-action="render"]', container).addClass('disabled');
            } else {
                $('.search-action[data-action="render"]', container).removeClass('disabled');
            }
        }

        // ------------------------------

        // function updateResult(found, total) {
        //     var nb = $('.result-founds', container),
        //     tot = $('.result-total', container);

        //     nb.html( Number(nb.html()) + found);
        //     tot.html( Number(tot.html()) + total);
        // }

        // ------------------------------

        function draw() {
            //console.log('search draw');
        }

        // ------------------------------

        function drawResult(time, phi, theta) {
            renderManager.updateFields(time, phi, theta);
            renderManager.draw();

            // Extract image and use it inside result
            return renderManager.toDataURL();
        }

        // ------------------------------

        function updatePipeline(query) {
            // update Layer Visibility
            var count = query.length;
            layerOlderInvalid = false;

            for(var idx = 0; idx < count; idx += 2) {
                var layer = query[idx],
                    visibility = (query[idx+1] != '_');

                if(!layerVisibility.hasOwnProperty(layer) || layerVisibility[layer] != visibility) {
                    layerVisibility[layer] = visibility;
                    layerOlderInvalid = true;
                }
            }

            if(layerOlderInvalid) {
                updateWorkers(query);
                $('.chart-container', container).empty();
                $('.composite-search-results', container).empty();
                result = [];
            }

            // Update render manager
            renderManager.updatePipeline(query);
        }

        // ------------------------------

        function updateColor(color) {
            renderManager.updateColor(color);
        }

        // ------------------------------

        function updatePixelRatio(field, value) {

        }

        // ------------------------------

        function updatePixelRatioOrder(field, isPlus) {
            console.log('search updatePixelRatioOrder: ' + field + " " + isPlus);
        }

        // ------------------------------

        return {
            updateFields:updateFields,
            draw:draw,
            updatePipeline:updatePipeline,
            updateColor:updateColor,
            updatePixelRatio:updatePixelRatio,
            updatePixelRatioOrder:updatePixelRatioOrder,
            search: search,
            applyQuery: applyQuery,
            renderActivePage: renderActivePage,
            layerToLabel: layerToLabel
        };
    }

    // ----------------------------------------------------------------------

    function attachTouchListener(container) {
        var current_button = null, posX, posY, defaultDragButton = 1,
        isZooming = false, isDragging = false, mouseAction = 'up', target;

        function mobileTouchInteraction(evt) {
            evt.gesture.preventDefault();
            switch(evt.type) {
                case 'drag':
                    if(isZooming) {
                        return;
                    }
                    current_button = defaultDragButton;
                    if(mouseAction === 'up') {
                        mouseAction = "down";

                        target = evt.gesture.target;
                        isDragging = true;
                    } else {
                        mouseAction = "move";
                    }

                    posX = evt.gesture.touches[0].pageX;
                    posY = evt.gesture.touches[0].pageY;
                    break;
                case 'hold':
                    if(defaultDragButton === 1) {
                        defaultDragButton = 2;
                        //container.html("Pan mode").css('color','#FFFFFF');
                    } else {
                        defaultDragButton = 1;
                        //container.html("Rotation mode").css('color','#FFFFFF');
                    }

                    break;
                case 'release':
                    //container.html('');
                    current_button = 0;
                    mouseAction = "up";
                    isZooming = false;
                    isDragging = false;
                    break;
                case 'doubletap':
                    container.trigger('resetCamera');
                    return;
                case 'pinch':
                    if(isDragging) {
                        return;
                    }
                    current_button = 3;
                    if(mouseAction === 'up') {
                        mouseAction = 'down';
                        posX = 0;
                        posY = container.height();
                        target = evt.gesture.target;
                        isZooming = true;
                    } else {
                        mouseAction = 'move';
                        posY = container.height() * (1+(evt.gesture.scale-1)/2);
                    }
                    break;
            }

            // Trigger event
            container.trigger({
                type: 'mouse',
                action: mouseAction,
                current_button: current_button,
                charCode: '',
                altKey: false,
                ctrlKey: false,
                shiftKey: false,
                metaKey: false,
                delegateTarget: target,
                pageX: posX,
                pageY: posY
            });
        }

        // Bind listener to UI container
        container.hammer({
            prevent_default : true,
            prevent_mouseevents : true,
            transform : true,
            transform_always_block : true,
            transform_min_scale : 0.03,
            transform_min_rotation : 2,
            drag : true,
            drag_max_touches : 1,
            drag_min_distance : 10,
            swipe : false,
            hold : true // To switch from rotation to pan
        }).on("doubletap pinch drag release hold", mobileTouchInteraction);
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

            // Handle mobile
            attachTouchListener(element);
            element.bind('mouse', function(e){
                // action: mouseAction,
                // current_button: current_button,
                // charCode: '',
                // altKey: false,
                // ctrlKey: false,
                // shiftKey: false,
                // metaKey: false,
                // delegateTarget: target,
                // pageX: posX,
                // pageY: posY
                var action = e.action,
                altKey = e.altKey,
                shiftKey = e.shiftKey,
                ctrlKey = e.ctrlKey,
                x = e.pageX,
                y = e.pageY,
                current_button = e.current_button;

                if(action === 'down') {
                    if (e.altKey) {
                        current_button = 2;
                        e.altKey = false;
                    } else if (e.shiftKey) {
                        current_button = 3;
                        e.shiftKey = false;
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
                    lastLocation = [x, y];

                    e.preventDefault();
                } else if(action === 'up') {
                    mouseMode = modeNone;
                    e.preventDefault();
                } else if(action === 'move') {
                    if(mouseMode != modeNone) {
                        var loc = [x,y];

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

                        // Redraw the image in the canvas
                        redrawImage();
                    }
                }
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
                    if(str[0] === '@') {
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
                return false;
            } else {
                return draw();
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
                return false;
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
            //if(bgColor) {
            //    frontCTX.fillStyle = bgColor;
            //    frontCTX.fillRect(0,0,singleImageSize[0], singleImageSize[1]);
            //    frontBuffer = frontCTX.getImageData(0, 0, singleImageSize[0], singleImageSize[1]);
            //    frontPixels = frontBuffer.data;
            //} else {
            //    frontBuffer = bgCTX.getImageData(0, (nbImages - 1) * singleImageSize[1], singleImageSize[0], singleImageSize[1]);
            //    frontPixels = frontBuffer.data;
            //}

            // Clear front pixels
            //frontCTX.fillStyle = "#ffffff";
            //frontCTX.fillRect(0,0,singleImageSize[0], singleImageSize[1]);
            frontCTX.clearRect(0, 0, singleImageSize[0], singleImageSize[1]);
            frontBuffer = frontCTX.getImageData(0, 0, singleImageSize[0], singleImageSize[1]);
            frontPixels = frontBuffer.data;

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
            return true;
        }

        function toDataURL() {
            return frontCanvas[0].toDataURL("image/png");
        }

        return {
            updateFields:updateFields,
            draw:draw,
            updatePipeline:updatePipeline,
            updateColor:updateColor,
            toDataURL:toDataURL,
            search:function(){}
        };
    }

    // ------------------------------------------------------------------------

    function createSelectColorBy(name, availableFields, fields, addRatio) {
        var buffer = [ "<div class='right-control'>" ],
        count = availableFields.length,
        value = null;

        if(count < 2) {
            buffer.push("<select style='display: none;'>");
        } else {
            buffer.push("<select>");
        }

        while(count--) {
            value = availableFields[count];
            buffer.push(SELECT_OPTION.replace(/VALUE/g, value).replace(/NAME/g, fields[value]))
        }
        buffer.push("</select>");

        // Add % slider for query
        if(addRatio) {
            buffer.push("<span class='vtk-icon-zoom-in action ratio-type shift'/><input class='pixel-ratio' type='range' min='0' max='100' value='0' name='FIELD'/><span class='ratio-value shift'>0%</span>".replace(/FIELD/g, name));
        }

        buffer.push("</div>");
        return buffer.join('');
    }

    // ------------------------------------------------------------------------

    function encodeEntry(entry, layer_fields, fields, addRatio) {
        var controlContent = "";

        if(entry['type'] === 'directory') {
            var array = entry['children'],
            count = array.length;
            for(var i = 0; i < count; ++i) {
                controlContent += encodeEntry(array[i], layer_fields, fields, addRatio).replace(/FRONT_ICON/g, 'vtk-icon-cancel-circled remove');
            }
            controlContent = DIRECTORY_CONTROL.replace(/CHILDREN/g, controlContent);
        } else {
           controlContent = createSelectColorBy(entry['ids'][0], layer_fields[entry['ids'][0]], fields, addRatio);
        }

        return PIPELINE_ENTRY.replace(/ID/g, entry['ids'].join(':')).replace(/LABEL/g, entry['name']).replace(/CONTROL/g, controlContent);
    }

    // ------------------------------------------------------------------------

    function createControlPanel(container, pipeline, layer_fields, fields, addRatio) {
        var pipelineBuffer = [], count = pipeline.length;

        // Build pipeline content
        for(var i = 0; i < count; ++i) {
            pipelineBuffer.push(encodeEntry(pipeline[i], layer_fields, fields, addRatio).replace(/FRONT_ICON/g, 'vtk-icon-eye toggle-eye'));
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
        keepAnimation = false,
        toggleLayer = {};

        function animate() {
            $('.active .vcr[data-action="next"]', container).trigger('click');
            if(keepAnimation) {
                setTimeout(animate, 200);
            }
        }

        function updatePipelineUI(query, args, color, sortValue) {
            var queryObj = {}, count = query.length, queryStr = "", sortStr = sortValue;
            for(var i = 0; i < count; i += 2) {
                queryObj[query[i]] = query[i+1];
            }

            // Update pipeline
            $('.pipeline > ul > li > ul > li', container).hide();
            $('.pipeline li', container).each(function(){
                var me = $(this),
                id = me.attr('data-id'),
                visibleLayer = (queryObj[id] != '_'),
                selectContainer = $('select', me),
                iconContainer = $('.toggle-eye', me);

                selectContainer.val(queryObj[id]);
                iconContainer.removeClass('vtk-icon-eye vtk-icon-eye-off').addClass(visibleLayer ? 'vtk-icon-eye':'vtk-icon-eye-off');
                if(visibleLayer) {
                    me.addClass('show enabled').show();
                } else {
                    me.removeClass('enabled show');
                }
            });

            // Update args
            for(var key in args) {
                var argContainer = $('.'+key, container),
                values = argContainer.attr('data-values').split(':'),
                labelContainer = $('.value', argContainer),
                count = values.length,
                targetValue = args[key];

                while(count--) {
                    if(values[count] == targetValue) {
                        labelContainer.html(targetValue);
                        argContainer.attr('data-index', count);
                        count = 0;
                    }
                }

                // Update Query STR
                queryStr += " && " + key + " == " + targetValue;
            }
            queryStr = queryStr.substr(4);

            // Update color
            $('.color').removeClass('active');
            if(color) {
                $('.color[data-color="' + color + '"]', container).addClass('active');
            }

            // Render the new content
            updatePipeline();
            updateComposite();
            manager.updateColor(color);

            // Update query if search mode
            queryExp = $('.query-expression', container);
            sortExp = $('.sortby-expression', container);
            if(queryExp) {
                queryExp.val(queryStr);
                if(sortStr) {
                    sortExp.val(sortStr);
                }
                setTimeout(function(){
                    $('.compute-coverage', container).trigger('click');
                    setTimeout(function(){
                        queryExp.trigger('change');
                    }, 600);
                }, 100);
            }
        }
        container.bind('updateControl', function(event){
            updatePipelineUI(event.query, event.args, event.color, event.sort);
        });

        function updatePipeline() {
            var query = "";

            for(var i in layers) {
                layer = layers[i];
                query += layer;
                var layerContainer = $('li[data-id="'+layer+'"]', container);
                if(layerContainer.hasClass('show') && layerContainer.hasClass('enabled')) {
                    query += $('select:eq(0)', layerContainer).val();
                    toggleLayer[layer] = true;
                } else {
                    query += '_';
                    toggleLayer[layer] = false;
                }
            }
            container.data('pipeline-query', query);
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
            container.data('args', {time:time, phi:phi, theta:theta});
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
            if(zoomableRender) {
                zoomableRender.resetCamera();
            }
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

            // Update query if search mode
            var queryExp = $('.query-expression', container);
            if(queryExp) {
                setTimeout(function(){
                    $('.compute-coverage', container).trigger('click');
                    setTimeout(function(){
                        queryExp.trigger('change');
                    }, 600);
                }, 100);
            }
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

                // Update query if search mode
                var queryExp = $('.query-expression', container);
                if(queryExp) {
                    setTimeout(function(){
                        $('.compute-coverage', container).trigger('click');
                        setTimeout(function(){
                            queryExp.trigger('change');
                        }, 600);
                    }, 100);
                }
            });
            $('input', layerSelector).change(function(){
                var me = $(this),
                checked = me.is(':checked'),
                id = me.attr('name'),
                item = $('li[data-id="' + id + '"]', container);

                if(checked) {
                    item.addClass("enabled show").show();
                } else {
                    item.removeClass("enabled show").hide();
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
            if(zoomableRender) {
                zoomableRender.paint();
            }
        });

        // Process current config
        updatePipeline();
        updateComposite();

        // ===== Search based UI =====

        $('.compute-coverage', container).click(function(){
            var pipelineContainer = $('.control', container), maxWidth = $(window).width();
            $('.chart-container', container).css('height', pipelineContainer.height()).css('width', maxWidth - pipelineContainer.width() - 50);
            updatePipeline();
            manager.search();
        });

        $('.query-expression', container).bind('change keyup', function(e){
            // Apply search
            if(e.type === 'keyup' && e.keyCode !== 13) {
                return;
            }
            var me = $(this), userQuery = me.val();
            manager.applyQuery(userQuery);
        });
        $('.sortby-expression', container).bind('change keyup', function(e){
            // Apply search
            if(e.type === 'keyup' && e.keyCode !== 13) {
                return;
            }
            var me = $('.query-expression', container), userQuery = me.val();
            manager.applyQuery(userQuery);
        });

        $('.toggle-stats', container).click(function(){
            var me = $(this), shouldShow = me.toggleClass('stats').hasClass('stats');
            if(shouldShow) {
                $('.composite-search-results .composite-result > ul', container).show();
            } else {
                $('.composite-search-results .composite-result > ul', container).hide();
            }
        });

        $('.page-result-action', container).bind('click change',function(){
            var me = $(this),
            action = me.attr('data-action'),
            pages = $('.composite-search-results .result-page', container),
            activePage = $('.composite-search-results .result-page.active', container),
            activeIdx = Number(activePage.attr("data-page")),
            nbPages = pages.length;

            pages.removeClass('active');

            if(action === "first") {
                $('.result-page[data-page=0]', container).addClass('active');
            } else if(action === "previous") {
                $('.result-page[data-page='+((nbPages + activeIdx - 1)%nbPages)+']', container).addClass('active');
            } else if(action === "next") {
                $('.result-page[data-page='+((activeIdx + 1)%nbPages)+']', container).addClass('active');
            } else if(action === "last") {
                $('.result-page[data-page='+(nbPages - 1)+']', container).addClass('active');
            } else if(action === "go-to") {
                var newIdx = Number($('.result-page-index', container).val()) - 1;
                $('.result-page[data-page='+newIdx+']', container).addClass('active');
            }
            manager.renderActivePage();
        });

        // $('.render-all-composites', container).click(function(){
        //     $('.composite-result', container).trigger('click');
        // });
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystCompositeViewer
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
                    createControlPanel(me, pipeline, layer_fields, fields, false);

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

                    $('.front-renderer', me).dblclick(function(){
                        var colorContainer = $('.color.active', me);
                        if(colorContainer) {
                            me.trigger({
                                type: "open-view",
                                query: me.data('pipeline-query'),
                                args: me.data('args'),
                                color: colorContainer.attr('data-color')
                            });
                        } else {
                            me.trigger({
                                type: "open-view",
                                query: me.data('pipeline-query'),
                                args: me.data('args')
                            });
                        }
                    });
                },
                error: function(error) {
                    console.log("error when trying to download " + dataBasePath + '/info.json');
                    console.log(error);
                }
            });
        });
    }

    /**
     * jQuery catalyst composite search constructor.
     *
     * @member jQuery.vtkCatalystCompositeSearch
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystCompositeSearch = function(dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().addClass('vtkweb-catalyst-analysis-composite-search vtkweb-catalyst-composite');

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
                    searchManager = createSearchManager(me, data, dataBasePath);

                    // Keep data info
                    me.data('basepath', dataBasePath);
                    me.data('layers', data.metadata.layers);

                    // Add control UI
                    createControlPanel(me, pipeline, layer_fields, fields, false);

                    // Enable additional fields if any (time, phi, theta)
                    var helpTxt = "", excludeList = {"filename": true, "field": true},
                    layerVarNames = "<hr/>";
                    for(var key in args) {
                        if(key === 'layer') {
                            continue;
                        }
                        var fieldContainer = $('.' + key, me);
                        if(fieldContainer) {
                            // Add documentation
                            var help = "<b>NAME</b>: VALUES <br/>".replace(/NAME/g, key),
                            values = args[key]["values"];
                            if(args[key]['type'] === 'range') {
                                values = "[MIN to MAX % MODULO]".replace(/MIN/g, values[0]).replace(/MAX/g, values[values.length - 1]).replace(/MODULO/g, (Number(values[1]) - Number(values[0])));
                            }
                            help = help.replace(/VALUES/g, values);
                            if(!excludeList.hasOwnProperty(key)) {
                                helpTxt += help;
                            }

                            fieldContainer.attr('data-values', args[key].values.join(':')).attr('data-index', '0').attr('data-size', args[key].values.length);
                        }
                    }

                    // Create layer labels
                    for(var key in searchManager.layerToLabel) {
                        layerVarNames += "<b>NAME</b>: VALUES <br/>".replace(/NAME/g, key).replace(/VALUES/g, searchManager.layerToLabel[key]);
                    }
                    helpTxt += layerVarNames;

                    // Add search/results containers
                    $('<div/>', {class: "chart-container"}).appendTo(me);
                    $('<div/>', {class: "search-toolbar", html: '<div class="table"><span class="cell"><b>Query</b></span><span class="cell expand"><input type="text" class="query-expression"></span><span class="cell"><b>Sort&nbsp;by</b></span><span class="cell expand"><input type="text" class="sortby-expression"></span><span class="cell"><span class="result-count"></span></span><span class="cell"><span class="vtk-icon-info-1 toggle-stats action" title="Toggle statistics"></span></span><span class="cell"><ul><li class="vtk-icon-to-start-1 action page-result-action" data-action="first"></li><li class="vtk-icon-left-dir-1 action page-result-action" data-action="previous"></li><li><input type="text" value="1" class="result-page-index page-result-action" data-action="go-to"></li><li> / </li><li class="result-page-number"></li><li class="vtk-icon-right-dir-1 action page-result-action" data-action="next"></li><li class="vtk-icon-to-end-1 action page-result-action" data-action="last"></li></ul></span></div></div><i>HELP</i>'.replace(/HELP/g, helpTxt)}).appendTo(me);
                    $('<div/>', {class: "composite-search-results"}).appendTo(me);

                    initializeListeners(me, searchManager, null);
                },
                error: function(error) {
                    console.log("error when trying to download " + dataBasePath + '/info.json');
                    console.log(error);
                }
            });
        });
    }
}(jQuery, window));