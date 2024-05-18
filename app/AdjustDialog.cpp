#include "AdjustDialog.h"
#include "ui_AdjustDialog.h"

AdjustDialog::AdjustDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AdjustDialog)
{
    ui->setupUi(this);

    connect(ui->slider_brightness, &QSlider::valueChanged, this, [this](int value) {
        ui->label_brightness->setText(QString::number(value));
        brightness = value;
    });
    connect(ui->slider_colorTemp, &QSlider::valueChanged, this, [this](int value) {
        ui->label_colorTemp->setText(QString::number(value));
        colorTemp = value;
    });
    connect(ui->slider_clarity, &QSlider::valueChanged, this, [this](int value) {
        ui->label_clarity->setText(QString::number(value));
        clarity = value;
    });
    connect(ui->slider_contrast, &QSlider::valueChanged, this, [this](int value) {
        ui->label_contrast->setText(QString::number(value));
        contrast = value;
    });
    connect(ui->slider_saturation, &QSlider::valueChanged, this, [this](int value) {
        ui->label_saturation->setText(QString::number(value));
        saturation = value;
    });
    connect(ui->slider_sharpness, &QSlider::valueChanged, this, [this](int value) {
        ui->label_sharpness->setText(QString::number(value));
        sharpness = value;
    });
    connect(ui->slider_vividness, &QSlider::valueChanged, this, [this](int value) {
        ui->label_vividness->setText(QString::number(value));
        vividness = value;
    });

    setDefaultValue();
}

AdjustDialog::~AdjustDialog()
{
    delete ui;
}

void AdjustDialog::setDefaultValue()
{
    ui->slider_brightness->setValue(0);
    ui->label_brightness->setText(QString::number(0));

    ui->slider_colorTemp->setValue(0);
    ui->label_colorTemp->setText(QString::number(0));

    ui->slider_clarity->setValue(0);
    ui->label_clarity->setText(QString::number(0));

    ui->slider_contrast->setValue(0);
    ui->label_contrast->setText(QString::number(0));

    ui->slider_saturation->setValue(0);
    ui->label_saturation->setText(QString::number(0));

    ui->slider_sharpness->setValue(0);
    ui->label_sharpness->setText(QString::number(0));

    ui->slider_vividness->setValue(0);
    ui->label_vividness->setText(QString::number(0));
}
