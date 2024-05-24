// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * All of these functions are invoked from C++ vtkWebAssemblyRenderWindowInteractor.
 * They are not meant for public use.
 */

var vtkWebAssemblyRenderWindowInteractor = {
  $VTKCanvas__deps: ['$findCanvasEventTarget', 'malloc'],
  $VTKCanvas : {
    /**
     * This is a general function invoked by the proxying methods below.
     * Initializes the canvas at elementId and applies style so that the canvas expands to
     * take up entire space of the parent element.
     * Here is how you can call this from C/C++ code:
     * ```cpp
     * #include "vtkWebAssemblyRenderWindowInteractor.h" // for vtkInitializeCanvasElement
     * vtkInitializeCanvasElement("#canvas", true);
     * ```
     * @param {number} elementId the selector of the canvas.
     * @param {boolean} applyStyle whether to modify the style of the canvas and parent element.
     */
    initializeCanvasElement: (elementId, applyStyle) => {
      const canvasElem = findCanvasEventTarget(elementId, applyStyle);
      if (!canvasElem) {
        return;
      }
      const containerElem = canvasElem.parentElement;
      const body = document.querySelector('body');
      if (applyStyle) {
        if (body === containerElem) {
          // fill up entire space of the body.
          body.style.margin = 0;
          body.style.width = '100vw';
          body.style.height = '100vh';
        } else {
          containerElem.style.position = 'relative';
          containerElem.style.width = '100%';
          containerElem.style.height = '100%';
        }
        canvasElem.style.position = 'absolute';
        canvasElem.style.top = 0;
        canvasElem.style.left = 0;
        canvasElem.style.width = '100%';
        canvasElem.style.height = '100%';
      }
    },

    /**
     * This is a general function invoked by the proxying methods below.
     * Get the size of the canvas from the parent element of the canvas, accounting for hi-dpi.
     * Here is how you can call this from C/C++ code:
     * ```cpp
     * #include "vtkWebAssemblyRenderWindowInteractor.h" // for vtkGetParentElementBoundingRectSize
     * int32_t* canvasSize = vtkGetParentElementBoundingRectSize("#canvas");
     * ```
     * @param {number} elementId the selector of the canvas.
     * @returns pointer to an integer array containing width and height in pixels.
     */
    getParentElementBoundingRectSize: (elementId) => {
      const canvasElem = findCanvasEventTarget(elementId);
      if (!canvasElem) {
        return 0;
      }
      const containerElem = canvasElem.parentElement;
      const dpr = window.devicePixelRatio;
      const width = containerElem.getBoundingClientRect().width;
      const height = containerElem.getBoundingClientRect().height;
      const w = Math.floor(width * dpr + 0.5);
      const h = Math.floor(height * dpr + 0.5);
      const sizePtr = _malloc(8); // width and height get sent to C++ as 32-bit integers.
      const idx = {{{ getHeapOffset('sizePtr', 'i32') }}};
      HEAP32.set([w, h], idx);
      return sizePtr;
    },
  },
  /**
   * Creates a one shot timer on the calling thread.
   * Here is how you can call this from C/C++ code:
   * ```cpp
   * vtkCreateTimer(1000, true, callbackFunc, (void*)userData);
   * ```
   * @param {number} duration in miliseconds
   * @param {boolean} isOneShot whether the timer is one shot i.e, setTimeout vs setInterval
   * @param {number} callback pointer to a C/C++ callback function
   * @param {number} userData pointer to a a void* argument which will be passed to callback. This is a vtkWebAssemblyRenderWindowInteractor::TimerBridgeData
   * @returns an identifier which can be used to destroy the one shot timer.
   * @note
   * Use this instead of emscripten_set_timeout because VTK applications should be able
   * to exit even when timers have not fired.
   * `emscripten_set_timeout` keeps the wasm runtime alive until all timers have fired.
   */
  vtkCreateTimer__sig: 'ipi**',
  vtkCreateTimer: (duration, isOneShot, callback, userData) => {
    if (isOneShot) {
      return setTimeout(arg => {{{ makeDynCall('vp', 'callback') }}}(arg), duration, userData);
    } else {
      return setInterval(arg => {{{ makeDynCall('vp', 'callback') }}}(arg), duration, userData);
    }
  },

  /**
   * Destroy the given one shot timer.
   * ```cpp
   * vtkDestroyTimer(timerId, isOneShot);
   * ```
   * @param {number} platformTimerId
   * @param {boolean} isOneShot whether the timer is one shot i.e, clearTimeout vs clearInterval
   */
  vtkDestroyTimer: (platformTimerId, isOneShot) => {
    if (isOneShot) {
      clearTimeout(platformTimerId);
    } else {
      clearInterval(platformTimerId);
    }
  },

#if PTHREADS
  $getParentElementBoundingRectSizeCallingThread: (elementId) => {
    return VTKCanvas.getParentElementBoundingRectSize(elementId);
  },

  $getParentElementBoundingRectSizeMainThread__proxy: 'sync',
  $getParentElementBoundingRectSizeMainThread__deps: ['$getParentElementBoundingRectSizeCallingThread'],
  $getParentElementBoundingRectSizeMainThread: (elementId) => getParentElementBoundingRectSizeCallingThread(elementId),

  vtkGetParentElementBoundingRectSize__deps: ['$getParentElementBoundingRectSizeMainThread'],
  vtkGetParentElementBoundingRectSize__sig: 'pp',
  vtkGetParentElementBoundingRectSize: (elementId) => {
    return getParentElementBoundingRectSizeMainThread(elementId);
  },
#else
  vtkGetParentElementBoundingRectSize__sig: 'pp',
  vtkGetParentElementBoundingRectSize: (elementId) => {
    return VTKCanvas.getParentElementBoundingRectSize(elementId);
  },
#endif

#if PTHREADS
  $initializeCanvasElementCallingThread: (elementId, applyStyle) => {
    VTKCanvas.initializeCanvasElement(elementId, applyStyle);
  },

  $initializeCanvasElementMainThread__proxy: 'sync',
  $initializeCanvasElementMainThread__deps: ['$initializeCanvasElementCallingThread'],
  $initializeCanvasElementMainThread: (elementId, applyStyle) => initializeCanvasElementCallingThread(elementId, applyStyle),

  vtkInitializeCanvasElement__deps: ['$initializeCanvasElementMainThread'],
  vtkInitializeCanvasElement__sig: 'vpi',
  vtkInitializeCanvasElement: (elementId, applyStyle) => {
    return initializeCanvasElementMainThread(elementId, applyStyle);
  },
#else
  vtkInitializeCanvasElement__sig: 'vpi',
  vtkInitializeCanvasElement: (elementId, applyStyle) => {
    VTKCanvas.initializeCanvasElement(elementId, applyStyle);
  },
#endif
};

autoAddDeps(vtkWebAssemblyRenderWindowInteractor, '$VTKCanvas');
mergeInto(LibraryManager.library, vtkWebAssemblyRenderWindowInteractor);
