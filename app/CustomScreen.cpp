#include "CustomScreen.h"

CustomScreen::CustomScreen(QWidget* parent) : QWidget(parent)
{}

void CustomScreen::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton) {
        emit sig_leftBtnClicked();
    }
}
