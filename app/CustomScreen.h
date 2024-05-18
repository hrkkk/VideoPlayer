#ifndef CUSTOMSCREEN_H
#define CUSTOMSCREEN_H

#include <QLabel>
#include <QObject>
#include <QWidget>
#include <QMouseEvent>

class CustomScreen : public QWidget
{
    Q_OBJECT
public:
    explicit CustomScreen(QWidget* parent = nullptr);

signals:
    void sig_leftBtnClicked();

protected:
    void mousePressEvent(QMouseEvent* e) override;

};

#endif // CUSTOMSCREEN_H
