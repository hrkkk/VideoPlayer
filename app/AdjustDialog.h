#ifndef ADJUSTDIALOG_H
#define ADJUSTDIALOG_H

#include <QDialog>

namespace Ui {
class AdjustDialog;
}

extern std::atomic<int> brightness;
extern std::atomic<int> saturation;
extern std::atomic<int> vividness;
extern std::atomic<int> contrast;
extern std::atomic<int> colorTemp;
extern std::atomic<int> sharpness;
extern std::atomic<int> clarity;

class AdjustDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdjustDialog(QWidget *parent = nullptr);
    ~AdjustDialog();

    void setDefaultValue();

private:
    Ui::AdjustDialog *ui;
};

#endif // ADJUSTDIALOG_H
