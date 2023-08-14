#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include <iostream>
#include <sstream>
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

void loadInput()
{
    char buffer[1024];
    while(std::cin.getline(buffer, 1024)) {
        std::stringstream ss(buffer);
        std::string s;

        if (buffer[0] == 'v') {
            Vector3f v;
            ss >> s;
            ss >> v[0] >> v[1] >> v[2];

            if(buffer[1] == 'n') {
                vecn.push_back(Vector3f(v[0],v[1],v[2]));
            } else {
                vecv.push_back(Vector3f(v[0],v[1],v[2]));
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

        /*
        for(auto & i : vecv) {
            std::cout << i.x() << ", " << i.y() << ", " << i.z() << std::endl;
        }*/
    }
}

int main() {
    loadInput();
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
    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->setMainMenu( pMenu );
    pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
}

void MyAppDelegate::applicationDidFinishLaunching( NS::Notification* pNotification )
{
    CGRect frame = (CGRect){ {100.0, 100.0}, {512.0, 512.0} };

    _pWindow = NS::Window::alloc()->init(
            frame,
            NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled,
            NS::BackingStoreBuffered,
            false );

    _pDevice = MTL::CreateSystemDefaultDevice();

    _pMtkView = MTK::View::alloc()->init( frame, _pDevice );
    _pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    _pMtkView->setClearColor( MTL::ClearColor::Make( 0.0, 0.0, 0.0, 1.0 ) );

    _pViewDelegate = new MyMTKViewDelegate( _pDevice );
    _pMtkView->setDelegate( _pViewDelegate );

    _pWindow->setContentView( _pMtkView );
    _pWindow->setTitle( NS::String::string( "MIT 6.837 Assignment 0", NS::StringEncoding::UTF8StringEncoding ) );

    _pWindow->makeKeyAndOrderFront( nullptr );

    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
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
