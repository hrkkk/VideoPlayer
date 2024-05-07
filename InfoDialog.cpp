#include "InfoDialog.h"
#include "./ui_InfoDialog.h"

InfoDialog::InfoDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InfoDialog)
{
    ui->setupUi(this);
}

InfoDialog::~InfoDialog()
{}

void InfoDialog::clearFileInfo()
{
    ui->treeWidget->clear();
}

void InfoDialog::setFileInfo(PlayerContext* playerCtx)
{
    FileInfo fileInfo = playerCtx->m_fileInfo;

    // 清空所有item
    ui->treeWidget->clear();

    QTreeWidgetItem* rootItem = new QTreeWidgetItem(QStringList() << QString::fromStdString(fileInfo.filename));
    ui->treeWidget->addTopLevelItem(rootItem);

    rootItem->addChild(new QTreeWidgetItem(QStringList() << "文件路径：" + QString::fromStdString(fileInfo.filename)));
    rootItem->addChild(new QTreeWidgetItem(QStringList() << "文件大小：" + QString::fromStdString(Utils::byteFormat(fileInfo.fileSize))));
    rootItem->addChild(new QTreeWidgetItem(QStringList() << "封装格式：" + QString::fromStdString(fileInfo.formatName)));
    rootItem->addChild(new QTreeWidgetItem(QStringList() << "封装格式全称：" + QString::fromStdString(fileInfo.formatFullName)));
    rootItem->addChild(new QTreeWidgetItem(QStringList() << "总时长：" + QString::fromStdString(Utils::timeFormat(fileInfo.duration))));
    rootItem->addChild(new QTreeWidgetItem(QStringList() << "总比特率：" + QString::number(fileInfo.bitRate)));
    rootItem->addChild(new QTreeWidgetItem(QStringList() << "流个数：" + QString::number(fileInfo.streamNum)));
    rootItem->addChild(new QTreeWidgetItem(QStringList() << "元数据：" + QString::fromStdString(fileInfo.metadata)));

    if (playerCtx->m_streamSet.contains(AVMEDIA_TYPE_VIDEO)) {
        QTreeWidgetItem* videoItem = new QTreeWidgetItem(rootItem, QStringList() << "Video");
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "编解码器：" + QString::fromStdString(fileInfo.videoCodecName)));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "编解码器全称：" + QString::fromStdString(fileInfo.videoCodecFullName)));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "视频时长：" + QString::fromStdString(Utils::timeFormat(fileInfo.videoDuration))));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "宽度：" + QString::number(fileInfo.width)));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "高度：" + QString::number(fileInfo.height)));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "显示比例：" + QString::number(fileInfo.aspectRatio[0]) + " : " + QString::number(fileInfo.aspectRatio[1])));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "像素位深：" + QString::number(fileInfo.pixelDepth) + " bits"));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "色彩空间：" + QString::fromStdString(fileInfo.colorSpace)));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "色度下采样：" + QString::fromStdString(fileInfo.chromaSubsampling)));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "帧率：" + QString::number(fileInfo.frameRate)));
        videoItem->addChild(new QTreeWidgetItem(QStringList() << "视频流大小：" + QString::fromStdString(Utils::byteFormat(fileInfo.videoStreamSize))));
    }

    if (playerCtx->m_streamSet.contains(AVMEDIA_TYPE_AUDIO)) {
        QTreeWidgetItem* audioItem = new QTreeWidgetItem(rootItem, QStringList() << "Audio");
        audioItem->addChild(new QTreeWidgetItem(QStringList() << "编解码器：" + QString::fromStdString(fileInfo.audioCodecName)));
        audioItem->addChild(new QTreeWidgetItem(QStringList() << "编解码器全称：" + QString::fromStdString(fileInfo.audioCodecFullName)));
        audioItem->addChild(new QTreeWidgetItem(QStringList() << "音频时长：" + QString::fromStdString(Utils::timeFormat(fileInfo.audioDuration))));
        audioItem->addChild(new QTreeWidgetItem(QStringList() << "采样率：" + QString::number(fileInfo.sampleRate) + " Hz"));
        audioItem->addChild(new QTreeWidgetItem(QStringList() << "通道数：" + QString::number(fileInfo.channels)));
        audioItem->addChild(new QTreeWidgetItem(QStringList() << "样本位深：" + QString::number(fileInfo.sampleDepth) + " bits"));
        audioItem->addChild(new QTreeWidgetItem(QStringList() << "音频流大小：" + QString::fromStdString(Utils::byteFormat(fileInfo.audioStreamSize))));
    }

    ui->treeWidget->expandAll();
}
