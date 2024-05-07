#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <QObject>
#include "PlayerContext.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class InfoDialog;
}
QT_END_NAMESPACE

class InfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InfoDialog(QWidget *parent = nullptr);
    ~InfoDialog();

    void clearFileInfo();
    void setFileInfo(PlayerContext* playerCtx);

private:
    Ui::InfoDialog *ui;
};

#endif // INFODIALOG_H
