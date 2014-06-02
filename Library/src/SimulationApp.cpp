//
//  SimulationApp.cpp
//  Stonefish
//
//  Created by Patryk Cieslak on 11/28/12.
//  Copyright (c) 2012 Patryk Cieslak. All rights reserved.
//

#include "SimulationApp.h"
#include "SystemUtil.h"

SimulationApp* SimulationApp::handle = NULL;

//////////////////////////////////////
SimulationApp::SimulationApp(const char* name, int width, int height, SimulationManager* sim)
{
    SimulationApp::handle = this;
	appName = name;
    winWidth = width;
    winHeight = height;
    finished = false;
    running = false;
    simulation = sim;
    simSpeedFactor = 1;
    lastPicked = NULL;
    loading = true;
    
	fps = 0.0;
	physics = 0.0;
}

SimulationApp::~SimulationApp()
{
	if(joystick != NULL)
	{
		delete [] joystickButtons;
		delete [] joystickAxes;
		delete [] joystickHats;
	}
}

void SimulationApp::setSimulationSpeed(btScalar factor)
{
    if(factor > 0.f)
    simSpeedFactor = factor;
    else
    simSpeedFactor = 1;
}

btScalar SimulationApp::getSimulationSpeed()
{
    return simSpeedFactor;
}

SimulationManager* SimulationApp::getSimulationManager()
{
    return simulation;
}

IMGUI* SimulationApp::getHUD()
{
    return hud;
}

SDL_Joystick* SimulationApp::getJoystick()
{
    return joystick;
}

double SimulationApp::getFPS()
{
    return fps;
}

double SimulationApp::getPhysicsTime()
{
    return physics;
}

int SimulationApp::getWindowWidth()
{
    return winWidth;
}

int SimulationApp::getWindowHeight()
{
    return winHeight;
}

bool SimulationApp::isRunning()
{
	return running;
}

bool SimulationApp::isLoading()
{
    return loading;
}

const char* SimulationApp::getDataPath()
{
    return dataPath;
}

const char* SimulationApp::getShaderPath()
{
    return shaderPath;
}

void SimulationApp::Init(const char* dataPath, const char* shaderPath)
{
    this->dataPath = dataPath;
    this->shaderPath = shaderPath;
    
    //Basics
    InitializeSDL(); //Window initialization + loading thread
    cInfo("Window created. OpenGL contexts created.");
    
    //Continue initialization with console visible
    cInfo("Initializing rendering pipeline:");
    cInfo("Loading GUI...");
    InitializeGUI();
    OpenGLPipeline::getInstance()->Initialize(simulation); //OpenGL initialization
    
    cInfo("Initializing simulation:");
    InitializeSimulation();
    
    cInfo("Running...");
    
    //Close loading console - exit loading thread
    SDL_Delay(1000);
    loading = false;
    int status = 0;
    SDL_WaitThread(loadingThread, &status);
}

void SimulationApp::InitializeSDL()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    
    window = SDL_CreateWindow(appName,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              winWidth,
                              winHeight,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
                              );
   
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    
    glLoadingContext = SDL_GL_CreateContext(window);
    glMainContext = SDL_GL_CreateContext(window);
    Console::getInstance()->SetRenderSize(winWidth, winHeight);
    LoadingThreadData* data = new LoadingThreadData();
    data->app = this;
    data->mutex = Console::getInstance()->getLinesMutex();
    loadingThread = SDL_CreateThread(SimulationApp::RenderLoadingScreen, "loadingThread", data);
    
#ifdef _MSC_VER
	glewInit();
#endif
	
    joystick = NULL;
    int jcount = SDL_NumJoysticks();
    if(jcount > 0)
    {
        joystick = SDL_JoystickOpen(0);
        joystickButtons = new bool[SDL_JoystickNumButtons(joystick)];
        memset(joystickButtons, 0, SDL_JoystickNumButtons(joystick));
        joystickAxes = new int16_t[SDL_JoystickNumAxes(joystick)];
        memset(joystickAxes, 0, SDL_JoystickNumAxes(joystick)*2);
        joystickHats = new uint8_t[SDL_JoystickNumHats(joystick)];
        memset(joystickHats, 0, SDL_JoystickNumHats(joystick));
        printf("Joystick %s connected ", SDL_JoystickName(0));
        printf("(%d axes, %d hats, %d buttons).\n", SDL_JoystickNumAxes(joystick), SDL_JoystickNumHats(joystick), SDL_JoystickNumButtons(joystick));
    }
}

#ifdef USE_ADVANCED_GUI
void SimulationApp::SetCEGUIPaths()
{
    // Initialises the required directories for the DefaultResourceProvider:
    CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());
    
    // For each resource type, sets a corresponding resource group directory:
    const char* prefix = GetDataPathPrefix("CEGUI");
    char resourcePath[1024];
    
    sprintf(resourcePath, "%s/%s", prefix, "schemes/");
    rp->setResourceGroupDirectory("schemes", resourcePath);
    sprintf(resourcePath, "%s/%s", prefix, "imagesets/");
    rp->setResourceGroupDirectory("imagesets", resourcePath);
    sprintf(resourcePath, "%s/%s", prefix, "fonts/");
    rp->setResourceGroupDirectory("fonts", resourcePath);
    sprintf(resourcePath, "%s/%s", prefix, "layouts/");
    rp->setResourceGroupDirectory("layouts", resourcePath);
    sprintf(resourcePath, "%s/%s", prefix, "looknfeel/");
    rp->setResourceGroupDirectory("looknfeels", resourcePath);
    sprintf(resourcePath, "%s/%s", prefix, "lua_scripts/");
    rp->setResourceGroupDirectory("lua_scripts", resourcePath);
    sprintf(resourcePath, "%s/%s", prefix, "xml_schemas/");
    rp->setResourceGroupDirectory("schemas", resourcePath);
    sprintf(resourcePath, "%s/%s", prefix, "animations/");
    rp->setResourceGroupDirectory("animations", resourcePath);
    
    // Sets the default resource groups to be used:
    CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
    CEGUI::AnimationManager::setDefaultResourceGroup("animations");
    
    // Set-up default group for validation schemas:
    CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
    if(parser->isPropertyPresent("SchemaDefaultResourceGroup"))
        parser->setProperty("SchemaDefaultResourceGroup", "schemas");
}

void SimulationApp::BuildCEGUI()
{
    CEGUI::Window* root = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    
    CEGUI::FrameWindow *simControlWindow = static_cast<CEGUI::FrameWindow*>(CEGUI::WindowManager::getSingleton().createWindow("StonefishOrange/FrameWindow", "SimControlWindow"));
    root->addChild(simControlWindow);
    simControlWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.5f, 0), CEGUI::UDim(0.5f, 0)));
    simControlWindow->setSize(CEGUI::USize(CEGUI::UDim(0, 100.f), CEGUI::UDim(0, 100.f)));
    simControlWindow->setText("Simulation control");
}
#endif

void SimulationApp::InitializeGUI()
{
#ifdef USE_ADVANCED_GUI
    //Advanced GUI setup
    guiResourceProvider = 0;
    guiImageCodec = 0;
    guiRenderer = &CEGUI::OpenGLRenderer::create();

    const char * prefix = GetDataPathPrefix("Resources");
    char path[1024];
    strcpy(path, prefix);
    strcat(path, "/CEGUIlog.txt");
        
    CEGUI::System::create(*guiRenderer, guiResourceProvider, 0, guiImageCodec, 0, "", path);
    SetCEGUIPaths();
    CEGUI::SchemeManager::getSingleton().createFromFile("StonefishOrange.scheme");
    CEGUI::Window *rootWindow = CEGUI::WindowManager::getSingleton().createWindow("DefaultWindow","root");
    CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(rootWindow);
    BuildCEGUI();
#endif
    
    //Simple immediate GUI setup
    hud = new IMGUI();
    hud->SetRenderSize(winWidth, winHeight);
    hud->Init();
}

void SimulationApp::InitializeSimulation()
{
    cInfo("Building scenario...");
    simulation->BuildScenario();
    cInfo("Synchronizing motion states...");
    simulation->getDynamicsWorld()->synchronizeMotionStates();
    cInfo("Simulation initialized -> using Bullet Physics %d.%d.\n", btGetVersion()/100, btGetVersion()%100);
    
    //printf("Simulation initialized -> using Bullet Physics %d.%d.\n", btGetVersion()/100, btGetVersion()%100);
}

//window
void SimulationApp::WindowEvent(SDL_Event* event)
{
    int w, h;
    
    switch(event->window.event)
    {
        case SDL_WINDOWEVENT_RESIZED:
            SDL_GetWindowSize(window, &w, &h);
#ifdef USE_ADVANCED_GUI
            CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(w,h));
#endif
            break;
    }
}

//keyboard
void SimulationApp::KeyDown(SDL_Event *event)
{
    switch (event->key.keysym.sym)
    {
        case SDLK_ESCAPE:
            Quit();
            break;
            
        case SDLK_SPACE:
            getSimulationManager()->RestartScenario();
            StartSimulation();
            break;
            
        default:
            break;
    }
}

void SimulationApp::KeyUp(SDL_Event *event)
{
}

//mouse
void SimulationApp::MouseDown(SDL_Event *event)
{
}

void SimulationApp::MouseUp(SDL_Event *event)
{
}

void SimulationApp::MouseMove(SDL_Event *event)
{
}

void SimulationApp::MouseScroll(SDL_Event *event)
{
}

//joystick
void SimulationApp::JoystickDown(SDL_Event *event)
{
}

void SimulationApp::JoystickUp(SDL_Event *event)
{
}

void SimulationApp::ProcessInputs()
{
}

//event loop
void SimulationApp::EventLoop()
{
    SDL_Event event;
    bool mouseWasDown = false;
    startTime = GetTimeInMicroseconds();
#ifdef USE_ADVANCED_GUI
    CEGUI::GUIContext& guiContext = CEGUI::System::getSingleton().getDefaultGUIContext();
#endif
    
    while(!finished)
    {
        SDL_FlushEvents(SDL_FINGERDOWN, SDL_MULTIGESTURE);
#ifdef USE_ADVANCED_GUI
        CEGUI::Window *rootWindow = guiContext.getRootWindow();
#endif
        if(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_WINDOWEVENT:
                    WindowEvent(&event);
                    break;
                
                case SDL_TEXTINPUT:
#ifdef USE_ADVANCED_GUI
                    guiContext.injectChar(event.text.text[0]);
#endif
                    break;
                    
                case SDL_KEYDOWN:
                {
#ifdef USE_ADVANCED_GUI
                    switch(event.key.keysym.sym)
                    {

                        case SDLK_BACKSPACE:
                            guiContext.injectKeyDown(CEGUI::Key::Backspace);
                            break;
                            
                        case SDLK_LEFT:
                            guiContext.injectKeyDown(CEGUI::Key::ArrowLeft);
                            break;
                            
                        case SDLK_RIGHT:
                            guiContext.injectKeyDown(CEGUI::Key::ArrowRight);
                            break;
                            
                        case SDLK_UP:
                            guiContext.injectKeyDown(CEGUI::Key::ArrowUp);
                            break;
                            
                        case SDLK_DOWN:
                            guiContext.injectKeyDown(CEGUI::Key::ArrowDown);
                            break;
                            
                        default:
                            guiContext.injectKeyDown((CEGUI::Key::Scan)event.key.keysym.scancode);
                            break;
                    }
                    //guiContext.injectChar(event.key.keysym.sym);
#endif
                    hud->KeyDown(event.key.keysym.sym);
                    KeyDown(&event);
                    break;
                }
                    
                case SDL_KEYUP:
                {
#ifdef USE_ADVANCED_GUI
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_BACKSPACE:
                            guiContext.injectKeyUp(CEGUI::Key::Backspace);
                            break;
                        
                        case SDLK_LEFT:
                            guiContext.injectKeyUp(CEGUI::Key::ArrowLeft);
                            break;
                        
                        case SDLK_RIGHT:
                            guiContext.injectKeyUp(CEGUI::Key::ArrowRight);
                            break;
                        
                        case SDLK_UP:
                            guiContext.injectKeyUp(CEGUI::Key::ArrowUp);
                            break;
                        
                        case SDLK_DOWN:
                            guiContext.injectKeyUp(CEGUI::Key::ArrowDown);
                            break;
                        
                        default:
                            guiContext.injectKeyUp((CEGUI::Key::Scan)event.key.keysym.scancode);
                            break;
                    }
#endif
                    hud->KeyUp(event.key.keysym.sym);
                    KeyUp(&event);
                    break;
                }
                    
                case SDL_MOUSEBUTTONDOWN:
#ifdef USE_ADVANCED_GUI
                    switch (event.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            guiContext.injectMouseButtonDown(CEGUI::LeftButton);
                            break;
                        
                        case SDL_BUTTON_RIGHT:
                            guiContext.injectMouseButtonDown(CEGUI::RightButton);
                            break;
                    }
                    
                    if(guiContext.getWindowContainingMouse() == rootWindow)
                    {
                        hud->MouseDown(event.button.x, event.button.y, event.button.button == SDL_BUTTON_LEFT);
                        mouseWasDown = true;
                    }
#else
                    hud->MouseDown(event.button.x, event.button.y, event.button.button == SDL_BUTTON_LEFT);
                    mouseWasDown = true;
#endif
                    break;
                    
                case SDL_MOUSEBUTTONUP:
#ifdef USE_ADVANCED_GUI
                    switch (event.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            guiContext.injectMouseButtonUp(CEGUI::LeftButton);
                            break;
                            
                        case SDL_BUTTON_RIGHT:
                            guiContext.injectMouseButtonUp(CEGUI::RightButton);
                            break;
                    }
#endif
                    hud->MouseUp(event.button.x, event.button.y, event.button.button == SDL_BUTTON_LEFT);
                    MouseUp(&event);
                    break;
                    
                case SDL_MOUSEMOTION:
#ifdef USE_ADVANCED_GUI
                    guiContext.injectMousePosition(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
#endif
                    hud->MouseMove(event.motion.x, event.motion.y);
                    MouseMove(&event);
                    break;
                    
                case SDL_MOUSEWHEEL:
#ifdef USE_ADVANCED_GUI
                    guiContext.injectMouseWheelChange(event.wheel.y);
            
                    if(guiContext.getWindowContainingMouse() == rootWindow)
                        MouseScroll(&event);
#else
                    MouseScroll(&event);
#endif
                    break;
                    
                case SDL_JOYBUTTONDOWN:
                    joystickButtons[event.jbutton.button] = true;
                    JoystickDown(&event);
                    break;
                    
                case SDL_JOYBUTTONUP:
                    joystickButtons[event.jbutton.button] = false;
                    JoystickUp(&event);
                    break;
                    
                case SDL_QUIT:
                    finished = true;
                    break;
            }
        }

        if(joystick != NULL)
        {
            for(int i=0; i<SDL_JoystickNumAxes(joystick); i++)
                joystickAxes[i] = SDL_JoystickGetAxis(joystick, i);
        
            for(int i=0; i<SDL_JoystickNumHats(joystick); i++)
                joystickHats[i] = SDL_JoystickGetHat(joystick, i);
        }
        
        ProcessInputs();
        AppLoop();
        
        //workaround for checking if IMGUI is being manipulated
        if(mouseWasDown && !hud->isAnyActive())
        {
            lastPicked = simulation->PickEntity(event.button.x, event.button.y);
            MouseDown(&event);
        }
        mouseWasDown = false;
    }
}

void SimulationApp::AppLoop()
{
    //FPS update
    uint64_t endTime = GetTimeInMicroseconds();
    double eleapsed = (endTime - startTime)/1000000.0;
    fps = 1.0/eleapsed;
    startTime = endTime;
    
    //Simulation
    if(running)
    {
        simulation->AdvanceSimulation(GetTimeInMicroseconds()*simSpeedFactor);
        physics = simulation->getPhysicsTimeInMiliseconds();
    }
    
    //Rendering
    OpenGLPipeline::getInstance()->Render();
    
    //GUI
    hud->Begin();
    DoHUD();
    hud->End();
#ifdef USE_ADVANCED_GUI
    glActiveTextureARB(GL_TEXTURE0_ARB);
    CEGUI::GUIContext& guiContext = CEGUI::System::getSingleton().getDefaultGUIContext();
    CEGUI::System::getSingleton().renderAllGUIContexts();
    CEGUI::System::getSingleton().injectTimePulse((float)eleapsed);
    guiContext.injectTimePulse((float)eleapsed);
#endif
    glFlush();
	SDL_GL_SwapWindow(window);
}

void SimulationApp::DoHUD()
{
    char buffer[256];
    GLfloat white[4] = {1.f,1.f,1.f,1.f};
    
    ui_id label3;
    label3.owner = 0;
    label3.item = 2;
    label3.index = 0;
    sprintf(buffer, "Simulation speed: %1.2fx", getSimulationSpeed());
    getHUD()->DoLabel(label3, 140, 14, white, buffer);
    
    ui_id slider1;
    slider1.owner = 0;
    slider1.item = 3;
    slider1.index = 0;
    getSimulationManager()->setStepsPerSecond(getHUD()->DoSlider(slider1, 12, 40, 100.0, 5.0, 20.0, 60.0, 1000.0, getSimulationManager()->getStepsPerSecond(), "Steps/s"));
    
    if(lastPicked != NULL)
    {
        ui_id label4;
        label4.owner = 0;
        label4.item = 4;
        label4.index = 0;
        sprintf(buffer, "Last picked entity: %s", lastPicked->getName().c_str());
        getHUD()->DoLabel(label3, 300, 14, white, buffer);
    }
    
	/*ui_id panel1;
    panel1.owner = 0;
    panel1.item = 0;
    panel1.index = 0;
    getHUD()->DoPanel(panel1, 20, getWindowHeight()-100, 200, 50, "Test");*/
    
    double fps = getFPS();
    ui_id label1;
    label1.owner = 0;
    label1.item = 0;
    label1.index = 0;
    sprintf(buffer, "FPS: %1.2lf", fps);
    getHUD()->DoLabel(label1, 10, getWindowHeight()-15, white, buffer);
    
    ui_id label2;
    label2.owner = 0;
    label2.item = 1;
    label2.index = 0;
    
    sprintf(buffer, "Physics time: %1.1lf%% (%1.2lf ms)", getPhysicsTime()/(10.0/fps), getPhysicsTime());
    getHUD()->DoLabel(label2, 90, getWindowHeight()-15, white, buffer);
}

void SimulationApp::StartSimulation()
{
    simulation->StartSimulation();
    running = true;
}

void SimulationApp::StopSimulation()
{
    running = false;
    physics = 0.f;
    simulation->StopSimulation();
}

void SimulationApp::Quit()
{
    finished = true;
}

void SimulationApp::CleanUp()
{
    if(simulation != NULL)
        delete simulation;
    
    if(hud != NULL)
        delete hud;
    
    if(joystick != NULL)
        SDL_JoystickClose(0);
    
    SDL_GL_DeleteContext(glLoadingContext);
    SDL_GL_DeleteContext(glMainContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

///////Static/////////////////
SimulationApp* SimulationApp::getApp()
{
    return SimulationApp::handle;
}

int SimulationApp::RenderLoadingScreen(void* data)
{
    //Get application
    LoadingThreadData* ltdata = (LoadingThreadData*)data;
    
    //Make drawing in this thread possible
    SDL_GL_MakeCurrent(ltdata->app->window, ltdata->app->glLoadingContext);
    
    //Render loading screen
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    while(ltdata->app->loading)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        
        //Lock to prevent adding lines to the console while rendering
        SDL_LockMutex(ltdata->mutex);
        Console::getInstance()->Render();
        SDL_UnlockMutex(ltdata->mutex);
        
        glFlush();
        glFinish();
        SDL_GL_SwapWindow(ltdata->app->window);
    }
    
    //Detach thread from GL context
    SDL_GL_MakeCurrent(ltdata->app->window, NULL);
    
    return 0;
}


