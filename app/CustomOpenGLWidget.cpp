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
    attribute vec4 inPosition;
    attribute vec2 inTexCoord;

    uniform int rotation;

    varying vec2 texCoord;

    void main(void)
    {
        gl_Position = inPosition;

        // 根据旋转角度变换纹理坐标
        vec2 rotatedTexCoords;
        if (rotation == 1) {    // 顺时针旋转90°，将纹理坐标的x和y坐标交换，然后将x坐标取反
            rotatedTexCoords = vec2(1.0 - inTexCoord.y, inTexCoord.x);
        } else if (rotation == 2) {     // 顺时针旋转180°，将纹理坐标的x和y坐标取反
            rotatedTexCoords = vec2(1.0 - inTexCoord.x, 1.0 - inTexCoord.y);
        } else if (rotation == 3) {     // 顺时针旋转270°（即逆时针旋转90°），将纹理坐标的x和y坐标交换，然后将y坐标取反
            rotatedTexCoords = vec2(inTexCoord.y, 1.0 - inTexCoord.x);
        } else {     // 旋转0°，即默认情况，不旋转
            rotatedTexCoords = inTexCoord;
        }

        texCoord = vec2(rotatedTexCoords.x, 1.0 - rotatedTexCoords.y);
    }
);

const char* fsrc = GET_GLSTR(
    varying vec2 texCoord;
    uniform sampler2D tex_y;
    uniform sampler2D tex_uv;

    // 画面调节参数
    uniform float brightness = 0.0;
    uniform float saturation = 1.0;
    uniform float vividness = 0.0;
    uniform float contrast = 1.0;
    uniform float colorTemp = 0.0;
    uniform float sharpness = 1.0;
    uniform float clarity = 1.0;

    void main(void)
    {
        // YUV 转 RGB
        vec3 yuv;
        vec3 rgb;
        yuv.x = texture2D(tex_y, texCoord.st).r - 0.0625;
        yuv.y = texture2D(tex_uv, texCoord.st).r - 0.5;
        yuv.z = texture2D(tex_uv, texCoord.st).g - 0.5;
        rgb = mat3( 1,       1,         1,
                    0,       -0.39465,  2.03211,
                    1.13983, -0.58060,  0) * yuv;

        vec3 color = rgb;

        // 亮度调节
        color = color + vec3(brightness);

        // 对比度调节
        color = (color - 0.5f) * contrast + 0.5f;

        // 饱和度调节
        float luminance = dot(color, vec3(0.299, 0.587, 0.114));
        vec3 gray = vec3(luminance, luminance, luminance);
        color = mix(gray, color, saturation);

        // 把颜色值赋值给像素
        gl_FragColor = vec4(color, 1.0f);
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
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
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
    program->bindAttributeLocation("inPosition", VERTEXIN);
    program->bindAttributeLocation("inTexCoord", TEXTUREIN);
    program->link();
    program->bind();
    program->enableAttributeArray(VERTEXIN);
    program->enableAttributeArray(TEXTUREIN);
    program->setAttributeBuffer(VERTEXIN, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    program->setAttributeBuffer(TEXTUREIN, GL_FLOAT, 8 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    textureUniformY = program->uniformLocation("tex_y");
    textureUniformUV = program->uniformLocation("tex_uv");

    m_rotationLoc = program->uniformLocation("rotation");
    m_brightnessLoc = program->uniformLocation("brightness");
    m_saturationLoc = program->uniformLocation("saturation");
    m_vividnessLoc = program->uniformLocation("vividness");
    m_contrastLoc = program->uniformLocation("contrast");
    m_colorTempLoc = program->uniformLocation("colorTemp");
    m_sharpnessLoc = program->uniformLocation("sharpness");
    m_clarityLoc = program->uniformLocation("clarity");

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

    if (rotateStatus == ROTATE_RADIUS::ROTATE_90) {
        glUniform1i(m_rotationLoc, 1);
    } else if (rotateStatus == ROTATE_RADIUS::ROTATE_180) {
        glUniform1i(m_rotationLoc, 2);
    } else if (rotateStatus == ROTATE_RADIUS::ROTATE_270) {
        glUniform1i(m_rotationLoc, 3);
    } else {
        glUniform1i(m_rotationLoc, 0);
    }

    // 传递亮度值
    glUniform1f(m_brightnessLoc, brightness / 100.0f);
    // 传递对比度值
    glUniform1f(m_contrastLoc, contrast / 100.0f + 1.0f);
    // 传递饱和度值
    glUniform1f(m_saturationLoc, saturation / 100.0f + 1.0f);

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
