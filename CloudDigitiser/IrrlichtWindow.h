#ifndef IRRLICHTWINDOW_H
#define IRRLICHTWINDOW_H

#include <QWindow>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <irrlicht.h>
#pragma GCC diagnostic pop

class IrrlichtWindow : public QWindow
{
    Q_OBJECT
public:
    explicit IrrlichtWindow(QWindow *parent = 0);
    ~IrrlichtWindow();

    virtual void render(QPainter *painter);
    virtual void render();

    virtual void initializeIrrlicht();

    irr::IrrlichtDevice *irrlichtDevice() const;

    irr::video::IVideoDriver *videoDriver() const;

    irr::scene::ISceneManager *sceneManager() const;

    irr::video::SColor backgroundColour() const;
    void setBackgroundColour(const irr::video::SColor &backgroundColour);

    void setShowDemo(bool showDemo);

    bool showDemo() const;

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;

    void exposeEvent(QExposeEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:
    bool m_update_pending;

    irr::IrrlichtDevice *m_irrlichtDevice;
    irr::video::IVideoDriver* m_videoDriver;
    irr::scene::ISceneManager* m_sceneManager;

    irr::video::SColor m_backgroundColour;

    bool m_showDemo;
};

#endif // IRRLICHTWINDOW_H
