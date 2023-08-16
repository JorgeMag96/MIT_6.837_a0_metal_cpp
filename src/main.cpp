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

class MyMTKViewDelegate : public MTK::ViewDelegate
{
public:
    explicit MyMTKViewDelegate( MTL::Device* pDevice );
    ~MyMTKViewDelegate() override;
    void drawInMTKView( MTK::View* pView ) override;

private:
    Renderer* _pRenderer;
};

class MyAppDelegate : public NS::ApplicationDelegate
{
public:
    ~MyAppDelegate() override;

    NS::Menu* createMenuBar();

    void applicationWillFinishLaunching( NS::Notification* pNotification ) override;
    void applicationDidFinishLaunching( NS::Notification* pNotification ) override;
    bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;

private:
    NS::Window* _pWindow{};
    MTK::View* _pMtkView{};
    MTL::Device* _pDevice{};
    MyMTKViewDelegate* _pViewDelegate{};
};

// This is the list of points (3D vectors)
std::vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
std::vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
std::vector<std::vector<uint32_t>> vecf;

#pragma endregion Declarations }

inline std::vector<uint32_t> extract_indexes(const std::string &s, char delimiter) {
    std::vector<uint32_t> indexes;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        indexes.push_back(stoi(token));
    }
    return indexes;
}

void loadModel(const std::string* file_name)
{
    std::cout << "Opening file: " << *file_name << "\n";

    std::ifstream file(*file_name); // Open the file with the given name
    if (!file) {
        std::cerr << "Unable to open file!\n";
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
            ss >> s;
            std::string s1, s2, s3;
            ss >> s1 >> s2 >> s3;

            std::vector<uint32_t> idx1 = extract_indexes(s1, '/');
            std::vector<uint32_t> idx2 = extract_indexes(s2, '/');
            std::vector<uint32_t> idx3 = extract_indexes(s3, '/');

            vecf.push_back(std::vector<uint32_t>({idx1[0],idx2[0],idx3[0], idx1[2], idx2[2], idx3[2]}));
        }
    }
}

int main() {
    std::string file_name = "resources/sphere.obj";
    loadModel(&file_name);

    NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

    MyAppDelegate del;

    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate( &del );
    pSharedApplication->run();

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
    delete _pViewDelegate;
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

    pApp->setMainMenu( pMenu );
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
            NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled,
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

    _pViewDelegate = new MyMTKViewDelegate( _pDevice );
    _pMtkView->setDelegate( _pViewDelegate );

    _pWindow->setContentView( _pMtkView );
    _pWindow->setTitle( NS::String::string( "MIT 6.837 Assignment 0", NS::StringEncoding::UTF8StringEncoding ) );

    _pWindow->makeKeyAndOrderFront( nullptr );

    auto* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->activateIgnoringOtherApps( true );
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
        : _pDevice( pDevice->retain() )
{
    _pCommandQueue = _pDevice->newCommandQueue();
}

Renderer::~Renderer()
{
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );
    pEnc->endEncoding();
    pCmd->presentDrawable( pView->currentDrawable() );
    pCmd->commit();

    pPool->release();
}

#pragma endregion Renderer }
