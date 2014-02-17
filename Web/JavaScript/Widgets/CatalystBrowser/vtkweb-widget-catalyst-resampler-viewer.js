(function ($, GLOBAL) {
    var CONTENT_TEMPLATE = '<div class="toolbar"><span class="label">Field<select name="field">FIELDS</select></span><span class="label">Probe<select name="probe"><option value="0">X</option><option value="1">Y</option><option value="2">Z</option></select></span> Slice <span class="slice-value txt-feedback">0</span> Time <span class="time-value txt-feedback">0</span></div><div class="control"><div class="header"><span class="vtk-icon-tools toggle"/><span class="vtk-icon-play play"/><span class="vtk-icon-stop stop"/></div><div class="parameters"><div class="label" data-name="slice"><span class="flag vtk-icon-flag"/>Slice<span class="slice-value">0</span></div><input type="range" min="0" max="NB_SLICES" value="0" name="slice"/><div class="label" data-name="time"><span class="flag vtk-icon-flag"/>Time<span class="time-value">0</span></div><input type="range" min="0" max="NB_TIMES" value="0" name="time"/></div></div><div class="image-sample" style="padding: 10px;"><canvas style="border: solid 1px black;"><img/></canvas></div><div class="chart-sample"></div>',
    OPTION_TEMPLATE = '<option>VALUE</option>';

    // ========================================================================
    // Helper
    // ========================================================================

    function getFileName(filePattern, args) {
        var fileName = filePattern;
        for(key in args) {
            fileName = fileName.replace('{'+key+'}', args[key]);
        }
        return fileName;
    }

    // ------------------------------------------------------------------------

    function getOptions(values) {
        var buffer = [];
        for(var idx in values) {
            buffer.push(OPTION_TEMPLATE.replace('VALUE', values[idx]));
        }
        return buffer.join('\n');
    }

    // ------------------------------------------------------------------------

    function update(container) {
        var field = $('select[name="field"]', container).val(),
        slice = $('input[name="slice"]', container).val(),
        time = $('input[name="time"]', container).val(),
        probeAxis = $('select[name="probe"]', container).val();

        downloadJSON(container, slice, probeAxis, field, time);
        downloadImage(container, slice, field, time);
    }

    // ========================================================================
    // Download manager
    // ========================================================================

    function downloadJSON(container, slice, probeAxis, field, time) {
        var sliceList = [],
        cache = container.data('json-cache'),
        info = container.data('info'),
        basepath = container.data('base-path'),
        urls = [];

        if(probeAxis === "2") {
            sliceList = info['arguments']['slice']['values'];
        } else {
            sliceList.push(slice);
        }

        function updateChart(){
            if(0 === urls.length) {
                container.trigger('invalidate-chart');
            }
        }

        function download(fileName) {
            $.getJSON( basepath + '/' + fileName, function(data){
                cache[fileName] = data;
                if(urls.length > 0) {
                    download(urls.pop());
                } else {
                    updateChart();
                }
            }).fail(function(e){
                console.log('Fail to download ' + fileName);
                console.log(e);
            });
        }

        // Fill download queue
        for(var idx in sliceList) {
            var fileName = getFileName(info['name_pattern'], {'field': field, 'slice': sliceList[idx], 'time': time, 'format': 'json'});
            if(!cache.hasOwnProperty(fileName)) {
                urls.push(fileName);
            }
        }

        // Trigger download or update chart
        if(urls.length > 0) {
            download(urls.pop());
        } else {
            updateChart();
        }
    }

    // ------------------------------------------------------------------------

    function downloadImage(container, slice, field, time) {
        var info = container.data('info'),
        basepath = container.data('base-path'),
        fileName = getFileName(info['name_pattern'], {'field': field, 'slice': slice.toString(), 'time': time, 'format': 'jpg'}),
        img = $('img', container);
        img.attr('src', basepath + '/' + fileName);
    }

    // ========================================================================
    // Chart management
    // ========================================================================

    function updateChart(container) {
        var cache = container.data('json-cache'),
        info = container.data('info'),
        probe = container.data('probe-coord'),
        field = $('select[name="field"]', container).val(),
        slice = Number($('input[name="slice"]', container).val()),
        time = $('input[name="time"]', container).val(),
        probeAxis = $('select[name="probe"]', container).val(),
        fileName = getFileName(info['name_pattern'], {'field': field, 'slice': slice, 'time': time, 'format': 'json'}),
        size = [0,0,0],
        offset = 0, step = 0, nbSteps = 0, data = [],
        chartContainer = $(".chart-sample", container);

        if(cache[fileName] === undefined) {
            return;
        }
        size = cache[fileName]["dimensions"];

        function clamp(value) {
            return (value == null || value < -1e10) ? null : value;
        }

        if(probe === undefined) {
            return;
        }

        // Figure out way to traverse
        if(probeAxis === '0') {
            // Along X
            nbSteps = size[0];
            step = 1;
            offset = (size[1] - probe[1]) * size[0];
        } else if(probeAxis === '1') {
            // Along Y
            nbSteps = size[1];
            step = size[0];
            offset = probe[0];
        } else if(probeAxis === '2') {
            // Along Z
            nbSteps = size[2];
            step = -1;
            offset = probe[0] + (size[1] - probe[1]) * size[0];
        }

        // Extract data
        if(probeAxis === '2') {
            sliceList = info['arguments']['slice']['values'];
            // Need multi-files
            for(var idx in sliceList) {
                f = getFileName(info['name_pattern'], {'field': field, 'slice': sliceList[idx], 'time': time, 'format': 'json'});
                sliceDataField = cache[f][field];
                data.push({x: Number(idx), y: clamp(sliceDataField[offset])})
            }
        } else if(probeAxis === '1') {
            // Same slice
            sliceDataField = cache[fileName][field];
            for(var i = 0; i < nbSteps; ++i) {
                data.push({x: i, y: clamp(sliceDataField[offset + (i*step)])})
            }
        } else if(probeAxis === '0') {
            // Same slice
            sliceDataField = cache[fileName][field];
            for(var i = 0; i < nbSteps; ++i) {
                data.push({x: i, y: clamp(sliceDataField[offset + (i*step)])})
            }
        }

        // Update UI with chart
        if(chartContainer.hasClass('vtk-chart')) {
            // Update
            chartContainer.vtkChartUpdateData([ { data: data, color: 'steelblue', name: field } ], true);
            chartContainer.vtkChartConfigure({'chart-padding': [0, 0, 0, 0]});
        } else {
            // Create chart
            chartContainer.vtkChart({
                'legend': {basic: false, toggle: false, highlight: false},
                'renderer': 'line',
                'series': [ { data: data, color: 'steelblue', name: field } ],
                'axes': [ "bottom", "left", "top"],
                'chart-padding': [0, 0, 0, 0]
            });
        }
    }

    // ========================================================================
    // Listeners
    // ========================================================================

    function initializeListeners(container) {
        // Attach redraw callback
        var canvas = $('canvas', container),
        chartContainer = $('.chart-sample', container),
        image = $('img', container),
        enableProbing = false,
        startSliding = false,
        sliders = $('input', container),
        timeSlider = $('input[name="time"]', container),
        sliceSlider = $('input[name="slice"]', container),
        timeTxt = $('span.time-value', container),
        sliceTxt = $('span.slice-value', container),
        activeSlider = null;
        dropDowns = $('select', container),
        currentSlideValue = 0,
        maxSlideValue = 1,
        keepAnimation = false,
        play = $('.play', container),
        stop = $('.stop', container);

        function animate() {
            if(activeSlider) {
                currentSlideValue = (currentSlideValue + 1) % maxSlideValue;
                activeSlider.val(currentSlideValue);
                if(keepAnimation) {
                    setTimeout(animate, 150);
                }
                updateAll();
            }
        }

        // Generic data update
        function updateAll() {
            timeTxt.html(timeSlider.val());
            sliceTxt.html(sliceSlider.val());
            update(container);
            paint();
        }

        // Callback methods to probe data
        function probe(event) {
            if(enableProbing) {
                var offset = canvas.offset(),
                z = $('input[name="slice"]', container).val(),
                scale = image[0].naturalWidth / canvas.width();
                container.data('probe-coord',
                    [Math.floor(scale*(event.pageX - offset.left)), Math.floor(scale*(event.pageY - offset.top)), Number(z)]);
                updateAll();
            }
        }

        // Callback methods to paint image
        function paint() {
            if(image[0].naturalWidth == 0) {
                setTimeout(paint, 100);
                return;
            }

            var ctx = canvas[0].getContext("2d"),
            w = canvas.parent().width(),
            img = image[0],
            ih = img.naturalHeight,
            iw = img.naturalWidth,
            ratio = ih / iw,
            scale = iw / w,
            headHeight = canvas.offset().top - container.offset().top + 22,
            probePoint = container.data('probe-coord');

            canvas.css('left', '20px').attr('width', w + 'px').attr('height', Math.ceil(w*ratio) + 'px');
            chartContainer.css('width', '100%').css('height', (container.height() - Math.ceil(w*ratio) - headHeight) + 'px');
            ctx.drawImage(image[0],
                          0,   // source image upper left x
                          0,   // source image upper left y
                          iw,    // source image width
                          ih,    // source image height
                          0,              // destination canvas upper left x
                          0,              // destination canvas upper left y
                          canvas.width(),     // destination canvas width
                          canvas.height());    // destination canvas height

            // Draw line
            if(probePoint) {
                var probeAxis = $('select[name="probe"]', container).val();
                ctx.strokeStyle = "#000000";
                ctx.fillStyle="#FFFFFF";
                if(probeAxis === '0') {
                    // Along X
                    var y = probePoint[1] / scale;
                    ctx.rect(0,y,canvas.width(), Math.ceil(scale));
                    ctx.stroke();
                    ctx.fill();
                } else if(probeAxis === '1') {
                    // Along Y
                    var x = probePoint[0] / scale;
                    ctx.rect(x,scale,Math.ceil(scale),canvas.height());
                    ctx.stroke();
                    ctx.fill();
                } else if(probeAxis === '2') {
                    // Along Z
                    var x = probePoint[0] / scale, y = probePoint[1] / scale;
                    ctx.beginPath();
                    ctx.arc(x,y,5,0,2*Math.PI);
                    ctx.fill();
                    ctx.stroke();
                }
            }
        }

        // Attach probing listeners
        canvas.bind('mousemove', probe);
        canvas.bind('mouseup', function(){ enableProbing = false; });
        canvas.bind('mousedown', function(e){
            enableProbing = true;
            probe(e);
        });

        // Attach auto-paint listener
        image.bind('load onload', paint);

        // Attach graph update
        container.bind('invalidate-chart', function(){
            updateChart(container);
        });

        // Attach dropDowns listeners
        dropDowns.bind('change', updateAll);

        // Attach slider listener
        sliders.bind('change keyup', updateAll);
        sliders.bind('mousedown', function(){
            startSliding = true;
        });
        sliders.bind('mouseup', function(){
            startSliding = false;
        });
        sliders.bind('mousemove', function(){
            if(startSliding) {
                updateAll();
            }
        });

        $('div.label', container).click(function(){
            var me = $(this),
            all = $('.label', container);

            activeSlider = $('input[name="' + me.attr('data-name') + '"]', container);

            if(activeSlider) {
                currentSlideValue = Number(activeSlider.val());
                maxSlideValue = Number(activeSlider.attr('max'));

                // Handle flag visibility
                all.removeClass('active');
                me.addClass('active');
            }
        });

        $('.toggle', container).click(function(){
            container.toggleClass('small');
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

    $.fn.vtkCatalystResamplerViewer = function(dataBasePath) {
        return this.each(function() {
            var me = $(this).empty().addClass('vtk-catalyst-resample-viewer small').unbind();

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Store metadata
                    me.data('info', data);
                    me.data('active-args', {});
                    me.data('json-cache', {});
                    me.data('base-path', dataBasePath);

                    // Create UI
                    me.html(CONTENT_TEMPLATE
                        .replace('FIELDS', getOptions(data['arguments']['field']['values']))
                        .replace('NB_SLICES', data['arguments']['slice']['values'].length-1)
                        .replace('NB_TIMES', data['arguments']['time']['values'].length-1)
                    );
                    initializeListeners(me);

                    // Load data
                    update(me);
                },
                error: function(error) {
                    console.log("error when trying to download " + dataBasePath + '/info.json');
                    console.log(error);
                }
            });
        });
    }

    }(jQuery, window));