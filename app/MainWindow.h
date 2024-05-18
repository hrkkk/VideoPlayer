#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include "PlayerContext.h"
#include "InfoDialog.h"
#include "AdjustDialog.h"

using AVFramePtr = std::shared_ptr<AVFrame>;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initMenu();
    void initFileTree();
    void startWork(const QString& fileName);
    void stopWork();

private:
    Ui::MainWindow *ui;
    InfoDialog* m_infoDialog;
    AdjustDialog* m_adjustDialog;
    QMenu* m_menu;
};
#endif // MAINWINDOW_H
