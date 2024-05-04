#include "CustomOpenGLWidget.h"
#include <QDebug>
#include <QMouseEvent>

CustomOpenGLWidget::CustomOpenGLWidget(QWidget* parent) : QOpenGLWidget(parent) {}

void CustomOpenGLWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        emit sign_mouseClicked(event->globalPosition().x(), event->globalPosition().y());
    }
}

#define GET_GLSTR(x) #x
const char* vsrc = GET_GLSTR(
    attribute vec4 vertexIn;
    attribute vec2 textureIn;
    varying vec2 textureOut;

    void main(void)
    {
        gl_Position = vertexIn;
        textureOut = textureIn;
    }
);

const char* fsrc = GET_GLSTR(
    varying vec4 textureOut;
    uniform sampler2D tex_y;
    uniform sampler2D tex_uv;

    void main(void)
    {
        vec3 yuv;
        vec3 rgb;
        yuv.x = texture2D(tex_y, textureOut.st).r - 0.0625;
        yuv.y = texture2D(tex_uv, textureOut.st).r - 0.5;
        yuv.z = texture2D(tex_uv, textureOut.st).g - 0.5;
        rgb = mat3( 1,       1,         1,
                    0,       -0.39465,  2.03211,
                    1.13983, -0.58060,  0) * yuv;
        gl_FragColor = vec4(rgb, 1);
    }
);

void CustomOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);

    static const GLfloat vertices[] {
        // 顶点坐标
        -1.0f, -1.0f,
        -1.0f, +1.0f,
        +1.0f, +1.0f,
        +1.0f, -1.0f,
        // 纹理坐标
        0.0f, 1.0f,     // 左上
        0.0f, 0.0f,     // 左下
        1.0f, 0.0f,     // 右下
        1.0f, 1.0f      // 右上
    };

    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));

    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(vsrc);

    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(fsrc);

    program = new QOpenGLShaderProgram(this);
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertexIn", VERTEXIN);
    program->bindAttributeLocation("textureIn", TEXTUREIN);
    program->link();
    program->bind();
    program->enableAttributeArray(VERTEXIN);
    program->enableAttributeArray(TEXTUREIN);
    program->setAttributeBuffer(VERTEXIN, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    program->setAttributeBuffer(TEXTUREIN, GL_FLOAT, 8 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    textureUniformY = program->uniformLocation("tex_y");
    textureUniformUV = program->uniformLocation("tex_uv");

    textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureUV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureY->create();
    textureUV->create();
    idY = textureY->textureId();
    idUV = textureUV->textureId();
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

void CustomOpenGLWidget::resizeGL(int w, int h)
{

}

void CustomOpenGLWidget::paintGL()
{
    if (yuvData == nullptr) return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, idY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoWidth, videoHeight, 0, GL_RED, GL_UNSIGNED_BYTE, yuvData->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, idUV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, videoWidth >> 1, videoHeight >> 1, 0, GL_RG, GL_UNSIGNED_BYTE, yuvData->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUniform1i(textureUniformY, 0);
    glUniform1i(textureUniformUV, 1);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    yuvData.reset();
}

void CustomOpenGLWidget::slot_showYUV(AVFramePtr frame, uint width, uint height)
{
    yuvData = frame;
    videoWidth = width;
    videoHeight = height;
    update();
}
