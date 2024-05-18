#ifndef CUSTOMOPENGLWIDGET_H
#define CUSTOMOPENGLWIDGET_H

#include <QObject>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QWidget>
#include "PlayerContext.h"

enum ROTATE_RADIUS {
    ROTATE_0,
    ROTATE_90,
    ROTATE_180,
    ROTATE_270
};
enum LAYER_MIX_MODE {
    NO_MIX,
    MULTIPLY,
    SCREEN,
    OVERLAY,
    SOFT_LIGHT,
    HARD_LIGHT
};
extern std::atomic<ROTATE_RADIUS> rotateStatus;
extern std::atomic<int> brightness;
extern std::atomic<int> saturation;
extern std::atomic<int> vividness;
extern std::atomic<int> contrast;
extern std::atomic<int> colorTemp;
extern std::atomic<int> sharpness;
extern std::atomic<int> clarity;

using AVFramePtr = std::shared_ptr<AVFrame>;

class CustomOpenGLWidget : public QOpenGLWidget, QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit CustomOpenGLWidget(QWidget* parent = nullptr);

signals:
    void sign_mouseClicked(int x, int y);

public slots:
    void slot_showYUV(AVFramePtr data, uint width, uint height);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    const int VERTEXIN = 0;
    const int TEXTUREIN = 1;
    QOpenGLShaderProgram* program;
    QOpenGLBuffer vbo;
    GLuint textureUniformY, textureUniformUV;
    QOpenGLTexture* textureY = nullptr, * textureUV = nullptr;
    GLuint idY, idUV;
    uint videoWidth, videoHeight;
    AVFramePtr yuvData;

    int m_rotationLoc;
    int m_brightnessLoc;
    int m_saturationLoc;
    int m_vividnessLoc;
    int m_contrastLoc;
    int m_colorTempLoc;
    int m_sharpnessLoc;
    int m_clarityLoc;
};

#endif // CUSTOMOPENGLWIDGET_H
