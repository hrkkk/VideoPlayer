#include "CustomProgressBar.h"

CustomProgressBar::CustomProgressBar(QWidget *parent)
    : QWidget(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint);          //隐藏窗口
    this->setAttribute(Qt::WA_TranslucentBackground, true); //窗口透明
}

CustomProgressBar::~CustomProgressBar()
{
}

double CustomProgressBar::getPos()
{
    return m_upperPos;
}

void CustomProgressBar::slotSetValue(double pos1, double pos2)
{
    m_upperPos = pos1;
    m_bottomPos = pos2;
    update();
}

void CustomProgressBar::slotSetValue(double pos)
{
    m_upperPos = pos;
    update();
}


void CustomProgressBar::mousePressEvent(QMouseEvent *ev)
{
    //double pos = (double)ev->pos().x() / (double)width();
    //if (pos >= 1)
    //	pos = 1;

    //if (pos <= 0)
    //	pos = 0;

    //m_pos = pos;
    //update();
    //qDebug() << "seek pos = " << pos;
    //emit sigCustomSliderValueChanged(pos);
}

void CustomProgressBar::mouseMoveEvent(QMouseEvent *ev)
{
    double pos = (double)ev->pos().x() / (double)width();
    if (pos >= 1)
        pos = 1;

    if (pos <= 0)
        pos = 0;

    m_upperPos = pos;
    update();
}

void CustomProgressBar::mouseReleaseEvent(QMouseEvent *ev)
{
    double pos = (double)ev->pos().x() / (double)width();
    emit sigCustomSliderValueChanged(pos);
}

void CustomProgressBar::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //绘制底图矩形
    QBrush brush;
    brush.setColor(QColor(233,233,233));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.drawRoundedRect(this->rect(), 5, 5);


    //绘制底层缓冲进度
    QLinearGradient bottomRadial;

    bottomRadial.setStart(0, 0);
    bottomRadial.setFinalStop(0, 1);

    //设置起始点颜色，0表示起始
    bottomRadial.setColorAt(0, QColor("#87CEFA"));

    //设置终点颜色 1表示终点
    bottomRadial.setColorAt(1, QColor("#868887"));

    //设置延展方式
    bottomRadial.setSpread(QGradient::PadSpread);

    QPen bottomPen(QBrush("#868887"), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(bottomPen);

    //设置画刷
    painter.setBrush(bottomRadial);

    QRect bottomRect = this->rect();
    bottomRect.setWidth(bottomRect.width() * m_bottomPos);

    //画矩形
    painter.drawRoundedRect(bottomRect, 5, 5);



    //绘制上层播放进度
    QLinearGradient radial;

    radial.setStart(0, 0);
    radial.setFinalStop(0, 1);

    //设置起始点颜色，0表示起始
    radial.setColorAt(0, QColor("#87CEFA"));

    //设置终点颜色 1表示终点
    radial.setColorAt(1, QColor("#1E90FF"));

    //设置延展方式
    radial.setSpread(QGradient::PadSpread);

    QPen upperPen(QBrush("#1E90FF"), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(upperPen);

    //设置画刷
    painter.setBrush(radial);

    QRect rect = this->rect();
    rect.setWidth(rect.width() * m_upperPos);

    //画矩形
    painter.drawRoundedRect(rect, 5, 5);
}
