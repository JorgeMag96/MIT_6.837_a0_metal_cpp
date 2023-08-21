#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include "vecmath/Vector3f.h"

#pragma region Declarations {

class Renderer
{
public:
    explicit Renderer( MTL::Device* pDevice );
    ~Renderer();
    void draw( MTK::View* pView );

private:
    MTL::Device* _pDevice;
    MTL::CommandQueue* _pCommandQueue;
};

/**
 * <br>
 * This class contains methods for responding to a MetalKit view's drawing and resizing events.
 * <a href="https://developer.apple.com/documentation/metalkit/mtkviewdelegate?language=objc">Docs</a>.
 */
class MyMTKViewDelegate : public MTK::ViewDelegate
{
public:
    explicit MyMTKViewDelegate( MTL::Device* pDevice );
    ~MyMTKViewDelegate() override;

    /**
     * <br>
     * Draws the view's contents.
     * <a href="https://developer.apple.com/documentation/metalkit/mtkviewdelegate/1535942-drawinmtkview?language=objc">Docs</a>.
     * @param pView The view requesting that its contents be redrawn.
     */
    void drawInMTKView( MTK::View* pView ) override;

private:
    Renderer* _pRenderer;
};

/**
 * <br>
 * A set of methods that manage your app’s life cycle and its interaction with common system services.
 * <a href="https://developer.apple.com/documentation/appkit/nsapplicationdelegate?language=objc">Docs</a>.
 */
class MyAppDelegate : public NS::ApplicationDelegate
{
public:
    ~MyAppDelegate() override;

    static NS::Menu* createMenuBar();

    /**
     * <br>
     * Tells the delegate that the app's initialization is about to complete.
     * <a href="https://developer.apple.com/documentation/appkit/nsapplicationdelegate/1428623-applicationwillfinishlaunching?language=objc">Docs</a>.
     * @param pNotification : <a href="https://developer.apple.com/documentation/appkit/nsapplicationwillfinishlaunchingnotification?language=objc">ref</a>.
     */
    void applicationWillFinishLaunching( NS::Notification* pNotification ) override;

    /**
     * <br>
     * Tells the delegate that the app’s initialization is complete but it hasn’t received its first event.
     * <a href="https://developer.apple.com/documentation/appkit/nsapplicationdelegate/1428385-applicationdidfinishlaunching?language=objc">Docs</a>.
     * @param pNotification : <a href="https://developer.apple.com/documentation/appkit/nsapplicationdidfinishlaunchingnotification?language=objc">ref</a>.
     */
    void applicationDidFinishLaunching( NS::Notification* pNotification ) override;

    /**
     * <br>
     * Returns a Boolean value that indicates if the app terminates once the last window closes.
     * @param pSender : The application object whose last window was closed.
     * @return false if the application should not be terminated when its last window is closed; otherwise, true to terminate the application.
     */
    bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;

private:
    NS::Window* _pWindow{};
    MTK::View* _pMtkView{};
    MTL::Device* _pDevice{};
    MyMTKViewDelegate* _pMtkViewDelegate{};
};

// This is the list of points (dynamic array of 3D vectors)
std::vector<Vector3f> vecv;

// This is the list of normals (dynamic array of 3D vectors)
std::vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
std::vector<std::vector<uint32_t>> vecf;

#pragma endregion Declarations }

/**
 * <br>
 * Loads an .obj file into the vecv, vecn, and vecf vectors.
 *
 * @param file_name : string pointer representing the .obj file name
 */
void loadModel(const std::string& file_name)
{
    std::cout << "Loading model: " << file_name << std::endl;

    std::ifstream file(file_name);
    if (!file) {
        std::cerr << "Unable to open " << file_name << "!\n";
        return;
    }

    char buffer[1024];
    while(file.getline(buffer, 1024)) {
        std::stringstream ss(buffer);
        std::string s;

        if (buffer[0] == 'v') {
            Vector3f v;
            ss >> s;
            ss >> v[0] >> v[1] >> v[2];

            if(buffer[1] == 'n') {
                vecn.push_back(v);
            } else {
                vecv.push_back(v);
            }
            continue;
        }

        if(buffer[0] == 'f') {
            ss >> s; // Skip first space, because lines look like this: "f 258/270/258 278/291/278 277/290/277"
            uint32_t indexes[9];
            for(int i = 0; i < 3; i++) { // 258/270/258 then 278/291/278 and finally 277/290/277 (3 segments)
                std::string indexesGroup;
                ss >> indexesGroup; // first iteration contains "258/270/258" based on examples in previous comments

                std::string token;
                std::istringstream tokenStream(indexesGroup);
                while (std::getline(tokenStream, token, '/')) { // 258 then 270 and finally 258
                    indexes[(3 * i) + i] = stoi(token);
                }
            }

            //                  258         278         277         258         278         277
            vecf.push_back( {indexes[0], indexes[3], indexes[6], indexes[2], indexes[5], indexes[8]} );
        }
    }

    std::cout << file_name << " loaded successfully." << std::endl;
}

int main() {

    std::string file_name = "resources/sphere.obj";
    loadModel(file_name);

    // An object that supports Cocoa's reference-counted memory management system.
    // Docs: https://developer.apple.com/documentation/foundation/nsautoreleasepool?language=objc
    NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

    // Our own implementation of NS::ApplicationDelegate
    MyAppDelegate del;

    // An object that manages an app's main event loop and resources used by all of that app's objects.
    // Docs: https://developer.apple.com/documentation/appkit/nsapplication?language=objc
    NS::Application* pSharedApplication;

    // Returns the application instance, creating it if it doesn't exist yet.
    // Docs: https://developer.apple.com/documentation/appkit/nsapplication/1428360-sharedapplication?language=objc
    pSharedApplication = NS::Application::sharedApplication();

    pSharedApplication->setDelegate( &del );
    pSharedApplication->run(); // Starts the main event loop.

    // Releases and pops the receiver.
    // Docs: https://developer.apple.com/documentation/foundation/nsautoreleasepool/1807014-release?language=objc
    pAutoreleasePool->release();

    return 0;
}

#pragma mark - AppDelegate
#pragma region AppDelegate {

MyAppDelegate::~MyAppDelegate()
{
    _pMtkView->release();
    _pWindow->release();
    _pDevice->release();
    delete _pMtkViewDelegate;
}

NS::Menu* MyAppDelegate::createMenuBar()
{
    using NS::StringEncoding::UTF8StringEncoding;
    using namespace NS;

    Menu* pMainMenu = Menu::alloc()->init();
    MenuItem* pAppMenuItem = MenuItem::alloc()->init();
    Menu* pAppMenu = Menu::alloc()->init( String::string( "Appname", UTF8StringEncoding ) );

    String* appName = RunningApplication::currentApplication()->localizedName();
    String* quitItemName = String::string( "Quit ", UTF8StringEncoding )->stringByAppendingString( appName );
    SEL quitCb = MenuItem::registerActionCallback( "appQuit", [](void*,SEL,const Object* pSender){
        auto pApp = Application::sharedApplication();
        pApp->terminate( pSender );
    } );

    MenuItem* pAppQuitItem = pAppMenu->addItem( quitItemName, quitCb, String::string( "q", UTF8StringEncoding ) );
    pAppQuitItem->setKeyEquivalentModifierMask( EventModifierFlagCommand );
    pAppMenuItem->setSubmenu( pAppMenu );

    MenuItem* pWindowMenuItem = MenuItem::alloc()->init();
    Menu* pWindowMenu = Menu::alloc()->init( String::string( "Window", UTF8StringEncoding ) );

    SEL closeWindowCb = MenuItem::registerActionCallback( "windowClose", [](void*, SEL, const Object*){
        auto pApp = Application::sharedApplication();
        pApp->windows()->object< Window >(0)->close();
    } );
    MenuItem* pCloseWindowItem = pWindowMenu->addItem( String::string( "Close Window", UTF8StringEncoding ), closeWindowCb, String::string( "w", UTF8StringEncoding ) );
    pCloseWindowItem->setKeyEquivalentModifierMask( EventModifierFlagCommand );

    pWindowMenuItem->setSubmenu( pWindowMenu );

    pMainMenu->addItem( pAppMenuItem );
    pMainMenu->addItem( pWindowMenuItem );

    pAppMenuItem->release();
    pWindowMenuItem->release();
    pAppMenu->release();
    pWindowMenu->release();

    return pMainMenu->autorelease();
}

void MyAppDelegate::applicationWillFinishLaunching( NS::Notification* pNotification )
{
    NS::Menu* pMenu = createMenuBar();

    // An object that manages an app’s main event loop and resources used by all of that app’s objects.
    // Docs: https://developer.apple.com/documentation/appkit/nsapplication?language=objc
    auto* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );


    // Use this property to assign a new menu bar for your app or to access the current menu bar.
    // Docs: https://developer.apple.com/documentation/appkit/nsapplication/1428634-mainmenu?language=objc
    pApp->setMainMenu( pMenu );

    // The application is an ordinary app that appears in the Dock and may have a user interface.
    // Docs: https://developer.apple.com/documentation/appkit/nsapplication/1428621-setactivationpolicy?language=objc
    //       https://developer.apple.com/documentation/appkit/nsapplicationactivationpolicy/nsapplicationactivationpolicyregular?language=objc
    pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
}

void MyAppDelegate::applicationDidFinishLaunching( NS::Notification* pNotification )
{
    // Origin and size of the window’s content area in screen coordinates.
    // Note that the window server limits window position coordinates to ±16,000 and sizes to 10,000.
    CGRect frame = (CGRect){ {500.0, 300.0}, {512.0, 512.0} };

    // A window that an app displays on the screen.
    // Docs: https://developer.apple.com/documentation/appkit/nswindow?language=objc
    _pWindow = NS::Window::alloc()->init(
            frame,
            // The window displays a close button. The window displays a title bar.
            NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled,
            // The window renders all drawing into a display buffer and then flushes it to the screen.
            NS::BackingStoreBuffered,
            // Specifies whether the window server creates a window device for the window immediately.
            // When YES, the window server defers creating the window device until the window is moved onscreen.
            // All display messages sent to the window or its views are postponed until the window is created, just before it’s moved onscreen.
            false);

    // Start by getting a GPU device.
    // Each Metal device instance represents a GPU and is the main starting point for your app’s interaction with it.
    // All the objects your app needs to interact with Metal come from a MTLDevice that you acquire at runtime.
    // Docs: https://developer.apple.com/documentation/metal/gpu_devices_and_work_submission/getting_the_default_gpu?language=objc
    _pDevice = MTL::CreateSystemDefaultDevice();

    // Initializes a view with the specified frame rectangle and Metal device (GPU).
    // Docs: https://developer.apple.com/documentation/metalkit/mtkview?language=objc
    _pMtkView = MTK::View::alloc()->init( frame, _pDevice );

    // Ordinary format with four 8-bit normalized unsigned integer components in BGRA order with conversion between
    // sRGB and linear space.
    // Docs: https://developer.apple.com/documentation/metalkit/mtkview/1535940-colorpixelformat?language=objc
    _pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );

    // The color to use to clear the color target when creating a render pass descriptor.
    // Docs: https://developer.apple.com/documentation/metalkit/mtkview/1536036-clearcolor?language=objc
    _pMtkView->setClearColor( MTL::ClearColor::Make( 0.0, 0.0, 0.0, 1.0 ) );

    _pMtkViewDelegate = new MyMTKViewDelegate(_pDevice );

    // Use a delegate to provide a drawing method to a MTKView object and respond to rendering events without subclassing the MTKView class.
    _pMtkView->setDelegate(_pMtkViewDelegate );

    // The window's content view, the highest accessible view object in the window’s view hierarchy.
    // Docs: https://developer.apple.com/documentation/appkit/nswindow/1419160-contentview?language=objc
    _pWindow->setContentView( _pMtkView );

    // The string that appears in the title bar of the window or the path to the represented file.
    // Docs: https://developer.apple.com/documentation/appkit/nswindow/1419404-title?language=objc
    _pWindow->setTitle( NS::String::string( "MIT 6.837 Assignment 0", NS::StringEncoding::UTF8StringEncoding ) );

    // Moves the window to the front of the screen list, within its level, and makes it the key window; that is, it shows the window.
    // Docs: https://developer.apple.com/documentation/appkit/nswindow/1419208-makekeyandorderfront?language=objc
    _pWindow->makeKeyAndOrderFront( nullptr );

    // An object that manages an app’s main event loop and resources used by all of that app’s objects.
    // Docs: https://developer.apple.com/documentation/appkit/nsapplication?language=objc
    auto* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );

    // Makes the receiver the active app.
    // Docs: https://developer.apple.com/documentation/appkit/nsapplication/1428468-activateignoringotherapps?language=objc
    pApp->activateIgnoringOtherApps(true);
}

bool MyAppDelegate::applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender )
{
    return true;
}

#pragma endregion AppDelegate }


#pragma mark - ViewDelegate
#pragma region ViewDelegate {

MyMTKViewDelegate::MyMTKViewDelegate( MTL::Device* pDevice )
        : MTK::ViewDelegate()
        , _pRenderer( new Renderer( pDevice ) )
{
}

MyMTKViewDelegate::~MyMTKViewDelegate()
{
    delete _pRenderer;
}

void MyMTKViewDelegate::drawInMTKView( MTK::View* pView )
{
    _pRenderer->draw( pView );
}

#pragma endregion ViewDelegate }


#pragma mark - Renderer
#pragma region Renderer {

Renderer::Renderer( MTL::Device* pDevice )
        : _pDevice( pDevice->retain() ) // Increments the receiver’s reference count. Docs: https://developer.apple.com/documentation/objectivec/1418956-nsobject/1571946-retain?language=objc
{

    // Creates a queue you use to submit rendering and computation commands to a GPU.
    // Docs: https://developer.apple.com/documentation/metal/mtldevice/1433388-newcommandqueue?language=objc
    //       https://developer.apple.com/documentation/metal/mtlcommandqueue?language=objc
    _pCommandQueue = _pDevice->newCommandQueue();
}

Renderer::~Renderer()
{
    _pCommandQueue->release(); // Releases and pops the receiver.
    _pDevice->release(); // Releases and pops the receiver.
}

void Renderer::draw( MTK::View* pView )
{
    // An object that supports Cocoa’s reference-counted memory management system.
    // Docs: https://developer.apple.com/documentation/foundation/nsautoreleasepool?language=objc
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    // Returns a command buffer from the command queue that maintains strong references to resources.
    // CommandBuffer is a container that stores a sequence of GPU commands that you encode into it.
    // Docs: https://developer.apple.com/documentation/metal/mtlcommandqueue/1508686-commandbuffer?language=objc
    //       https://developer.apple.com/documentation/metal/mtlcommandbuffer?language=objc
    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();

    // A render pass descriptor to draw into the current drawable.
    // RenderPassDescriptor is a group of render targets that hold the results of a render pass.
    // Docs: https://developer.apple.com/documentation/metalkit/mtkview/1536024-currentrenderpassdescriptor?language=objc
    //       https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor?language=objc
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();

    // Creates a render command encoder from a descriptor.
    // RenderCommandEnconder is an interface that encodes a render pass into a command buffer, including all its draw calls and configuration.
    // Docs: https://developer.apple.com/documentation/metal/mtlcommandbuffer/1442999-rendercommandencoderwithdescript?language=objc
    //       https://developer.apple.com/documentation/metal/mtlrendercommandencoder?language=objc
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

    // Declares that all command generation from the encoder is completed.
    // Docs: https://developer.apple.com/documentation/metal/mtlcommandencoder/1458038-endencoding?language=objc
    pEnc->endEncoding();

    // Presents a drawable as early as possible.
    // Docs: https://developer.apple.com/documentation/metal/mtlcommandbuffer/1443029-presentdrawable?language=objc
    pCmd->presentDrawable( pView->currentDrawable() );

    // Submits the command buffer to run on the GPU.
    // Docs: https://developer.apple.com/documentation/metal/mtlcommandbuffer/1443003-commit?language=objc
    pCmd->commit();

    // Releases and pops the receiver.
    // Docs: https://developer.apple.com/documentation/foundation/nsautoreleasepool/1807014-release?language=objc
    pPool->release();
}

#pragma endregion Renderer }
