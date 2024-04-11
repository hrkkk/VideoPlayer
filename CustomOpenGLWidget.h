#ifndef CUSTOMOPENGLWIDGET_H
#define CUSTOMOPENGLWIDGET_H

#include <QObject>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QWidget>


class CustomOpenGLWidget : public QOpenGLWidget, QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit CustomOpenGLWidget(QWidget* parent = nullptr);

public slots:
    void slot_showYUV(uchar* ptr, uint width, uint height);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    const int VERTEXIN = 0;
    const int TEXTUREIN = 1;
    QOpenGLShaderProgram* program;
    QOpenGLBuffer vbo;
    GLuint textureUniformY, textureUniformU, textureUniformV;
    QOpenGLTexture* textureY = nullptr, * textureU = nullptr, * textureV = nullptr;
    GLuint idY, idU, idV;
    uint videoW, videoH;
    uchar* yuvPtr = nullptr;
};

#endif // CUSTOMOPENGLWIDGET_H
