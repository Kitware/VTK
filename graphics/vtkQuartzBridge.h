#ifdef __cplusplus
extern "C" {
#endif

void QBStartTimer(float seconds);

void *QBcreateAutoReleasePool();
int QBreleaseAutoreleasePool(void *thePool);

void QBmakeApplication();
void QBrunApplication();

void *QBmakeWindow();

void QBmakeCurrentContext(void *objcController);
void QBsendDisplay(void *objcController);

void QBSetVTKRenderWindow(void *objcController, void *theVTKRenderWindow);
void QBSetVTKRenderWindowInteractor(void *objcController, void *theVTKRenderWindowInteractor);

void QBDestroyWindow(void *objcController);

void QBKillApplication(void *objcController);

void QBstartTimer(void *objcController);
void QBstopTimer(void *objcController);

void QBmakeCurrentContext(void *objcController);

void QBSetWindowSize(void *objcController, int posX, int posY, int sizeX, int sizeY);

float QBGetWindowWidth(void *objcController);
float QBGetWindowHeight(void *objcController);
float QBGetWindowXPosition(void *objcController);
float QBGetWindowYPosition(void *objcController);


#ifdef __cplusplus
};
#endif