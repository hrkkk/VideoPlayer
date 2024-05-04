#ifndef CUSTOMPROGRESSBAR_H
#define CUSTOMPROGRESSBAR_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QShowEvent>
#include <QPainter>

class CustomProgressBar : public QWidget
{
    Q_OBJECT
public:
    explicit CustomProgressBar(QWidget *parent = nullptr);
    ~CustomProgressBar();

signals:
    void sign_sliderValueChanged(double pos);//自定义的鼠标单击信号，用于捕获并处理

public:
    //获取pos
    double getPos();

public slots:
    //设置0~1
    void slotSetValue(double pos1, double pos2);
    void slotSetValue(double pos);

protected:
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void paintEvent(QPaintEvent *) override;

private:
    double m_upperPos = 0;
    double m_bottomPos = 0;
};

#endif // CUSTOMPROGRESSBAR_H
