#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>

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

signals:
    void sig_showImage(uchar* ptr, uint width, uint height);

private:
    void initMenu();

private:
    Ui::MainWindow *ui;
    QMenu* m_menu;
};
#endif // MAINWINDOW_H
