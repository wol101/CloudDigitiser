#include <QtCore/QCoreApplication>
#include <QResizeEvent>

#ifdef Q_OS_MAC
const char *mesh_path="/Users/wis/Synchronised/E-docs/Computing/Irrlicht/irrlicht-1.8.3/media/sydney.md2";
const char *texture_path="/Users/wis/Synchronised/E-docs/Computing/Irrlicht/irrlicht-1.8.3/media/sydney.bmp";
#endif
#ifdef Q_OS_WIN32
const char *mesh_path="c:/Users/wis/Documents/Unix/src/irrlicht-1.8.3/irrlicht-1.8.3/media/sydney.md2";
const char *texture_path="c:/Users/wis/Documents/Unix/src/irrlicht-1.8.3/irrlicht-1.8.3/media/sydney.bmp";
#endif
#ifdef Q_OS_LINUX
const char *mesh_path="/home/wis/Synchronised/E-docs/Computing/Irrlicht/irrlicht-1.8.3/media/sydney.md2";
const char *texture_path="/home/wis/Synchronised/E-docs/Computing/Irrlicht/irrlicht-1.8.3/media/sydney.bmp";
#endif

#include "IrrlichtWindow.h"

IrrlichtWindow::IrrlichtWindow(QWindow *parent)
    : QWindow(parent)
{
    m_update_pending = false;

    m_irrlichtDevice = 0;
    m_videoDriver = 0;
    m_sceneManager = 0;

    m_backgroundColour = irr::video::SColor(255,100,101,140);

    m_showDemo = false;
}

IrrlichtWindow::~IrrlichtWindow()
{
    if (m_irrlichtDevice) m_irrlichtDevice->drop();
}

void IrrlichtWindow::render(QPainter *painter)
{
    Q_UNUSED(painter);
}

void IrrlichtWindow::initializeIrrlicht()
{
    irr::SIrrlichtCreationParameters createParams;

    // set the defaults for the device
    createParams.DeviceType = irr::EIDT_BEST;
#ifdef USE_SOFTWARE_RENDERER
    createParams.DriverType = irr::video::EDT_BURNINGSVIDEO; //irr::video::EDT_OPENGL;
#else
    createParams.DriverType = irr::video::EDT_OPENGL;
#endif
    createParams.WindowId = (void *) this->winId();
    createParams.WindowSize = irr::core::dimension2d<irr::u32>(this->width(), this->height());
    createParams.Bits = 32;
    createParams.ZBufferBits = 24;
    createParams.Fullscreen = false;
    createParams.Stencilbuffer = false;
    createParams.Vsync = true;
    createParams.AntiAlias = 1;
    createParams.HandleSRGB = false;
    createParams.WithAlphaChannel = true;
    createParams.Doublebuffer = true;
    createParams.IgnoreInput = false;
    createParams.Stereobuffer = false;
    createParams.HighPrecisionFPU = true;
    createParams.EventReceiver = 0;
    createParams.DisplayAdapter = 0;
    createParams.DriverMultithreaded = false;
    createParams.UsePerformanceTimer = true;

    if (m_irrlichtDevice) m_irrlichtDevice->drop();
    m_irrlichtDevice = irr::createDeviceEx(createParams);
    m_videoDriver = m_irrlichtDevice->getVideoDriver();
    m_sceneManager = m_irrlichtDevice->getSceneManager();

    // set some useful defaults for the driver
    m_videoDriver->setTextureCreationFlag(irr::video::ETCF_ALWAYS_32_BIT, true);
    m_videoDriver->setTextureCreationFlag(irr::video::ETCF_ALLOW_NON_POWER_2, true);
    m_videoDriver->setTextureCreationFlag(irr::video::ETCF_ALLOW_MEMORY_COPY, true);
    m_videoDriver->setTextureCreationFlag(irr::video::ETCF_CREATE_MIP_MAPS, true);

    if (m_showDemo)
    {
        // this code is just to create something to look at
        irr::scene::ICameraSceneNode* cam = m_sceneManager->addCameraSceneNode(0, irr::core::vector3df(20, 20, -50), irr::core::vector3df(0, 5, 0));
        if (!cam)
        {
            qWarning("Unable to create camera\n");
            return;
        }
        irr::scene::IAnimatedMesh *mesh = m_sceneManager->getMesh(mesh_path);
        if (!mesh)
        {
            qWarning("Unable to load mesh\n");
            return;
        }
        irr::scene::IAnimatedMeshSceneNode *node = m_sceneManager->addAnimatedMeshSceneNode(mesh);
        if (!node)
        {
            qWarning("Unable to create scene node\n");
            return;
        }
        node->setMaterialFlag(irr::video::EMF_LIGHTING, false);
        node->setMD2Animation(irr::scene::EMAT_STAND);
        irr::video::ITexture *texture = m_videoDriver->getTexture(texture_path);
        if (!texture)
        {
            qWarning("Unable to load texture\n");
            return;
        }
        node->setMaterialTexture(0, texture);
    }
}

void IrrlichtWindow::render()
{
    if (m_irrlichtDevice && m_videoDriver && m_sceneManager)
    {
        m_irrlichtDevice->getTimer()->tick(); // need to do this by hand because we are not using the irr::IrrlichtDevice::run() method

        m_videoDriver->beginScene(static_cast<irr::u16>(irr::video::ECBF_COLOR | irr::video::ECBF_DEPTH), m_backgroundColour);
        m_sceneManager->drawAll();
        m_videoDriver->endScene();
    }
}

void IrrlichtWindow::renderLater()
{
    if (!m_update_pending) {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool IrrlichtWindow::event(QEvent *event)
{
//    qDebug("event->type() = %d", event->type());
    switch (event->type()) {
    case QEvent::UpdateRequest:
        m_update_pending = false;
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void IrrlichtWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void IrrlichtWindow::renderNow()
{
    if (!isExposed())
        return;

    render();
}

void IrrlichtWindow::resizeEvent(QResizeEvent *event)
{
    if (m_videoDriver)
    {
        irr::core::dimension2d<irr::u32> size;
        size.Width = event->size().width();
        size.Height = event->size().height();
        m_videoDriver->OnResize(size);
        if (m_sceneManager)
        {
            irr::scene::ICameraSceneNode* camera = m_sceneManager->getActiveCamera();
            if (camera)
                camera->setAspectRatio((float)size.Width / (float)size.Height);
        }
        renderLater();
    }
}

bool IrrlichtWindow::showDemo() const
{
    return m_showDemo;
}

void IrrlichtWindow::setShowDemo(bool showDemo)
{
    m_showDemo = showDemo;
}

irr::video::SColor IrrlichtWindow::backgroundColour() const
{
    return m_backgroundColour;
}

void IrrlichtWindow::setBackgroundColour(const irr::video::SColor &BackgroundColour)
{
    m_backgroundColour = BackgroundColour;
}

irr::scene::ISceneManager *IrrlichtWindow::sceneManager() const
{
    return m_sceneManager;
}

irr::video::IVideoDriver *IrrlichtWindow::videoDriver() const
{
    return m_videoDriver;
}

irr::IrrlichtDevice *IrrlichtWindow::irrlichtDevice() const
{
    return m_irrlichtDevice;
}

