#include "dockwidgets/ImageDataWidget.h"

#include <QBoxLayout>
#include <QCompleter>
#include <QDebug>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QStyleOption>

#include <QToolBar>

struct ImageDataWidget::pimpl
{
    QWidget contents;
    QVBoxLayout vlay;
    QFormLayout formlay;

    QLabel lblFilePath;
    QLineEdit txtFilePath;
    QPushButton cmdBrowseFilePath;
    QDoubleSpinBox spinFrameRate;
    QSpinBox spinDownTime;
    QSpinBox spinDownSpace;
    QProgressBar progBar;
    QPushButton cmdLoad;

    QRadioButton optFolder;
    QRadioButton optFile;

    QCompleter completer;
    QFileSystemModel fsmodel_folder;
    QFileSystemModel fsmodel_file;

    void init()
    {
        {
            // * QFileSystemModel doesn't respond well when changing filter, so just create two models
            fsmodel_folder.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
            fsmodel_folder.setRootPath(QDir::currentPath());
            fsmodel_file.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
            fsmodel_file.setRootPath(QDir::currentPath());
        }
        {
            completer.setModel(&fsmodel_folder);
            completer.setCompletionMode(QCompleter::InlineCompletion);
        }

        {
            optFile.setText("Multi-page File");
            optFile.setObjectName("optFile");
            optFolder.setText("Folder");
            optFolder.setToolTip("Select this option if your dataset is made up of several Tiff files with one file for each frame.\nThe files must be in a folder of their own and will be read in alphanumeric order.");
            optFolder.setToolTip("Select this option if your data are contained in a single 'multipage' Tiff file.");
            optFolder.setChecked(true);
            optFolder.setObjectName("optFolder");

        }

        {
            txtFilePath.setCompleter(&completer);
            txtFilePath.setMinimumWidth(100);
            txtFilePath.setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Preferred);
            txtFilePath.setObjectName("txtFilePath");
        }
        {
            cmdBrowseFilePath.setText("...");
            const QSize textSize = cmdBrowseFilePath.fontMetrics().size(Qt::TextShowMnemonic, cmdBrowseFilePath.text());
            QStyleOptionButton opt;
            opt.initFrom(&cmdBrowseFilePath);
            opt.rect.setSize(textSize);
            cmdBrowseFilePath.setFixedWidth(cmdBrowseFilePath.style()->sizeFromContents(QStyle::CT_PushButton, &opt, textSize, &cmdBrowseFilePath).width());
        }
        {
            spinFrameRate.setValue(30);
            spinFrameRate.setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
            spinFrameRate.setMaximum(999);
            spinFrameRate.setMinimum(1);
            const QSize textSize = cmdBrowseFilePath.fontMetrics().size(Qt::TextShowMnemonic, "999");
            spinFrameRate.setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            spinFrameRate.setToolTip(tr("Set the frame rate in which the data were recorded\nin frames per second (FPS)"));
            spinFrameRate.setObjectName("spinFrameRate");
        }
        {
            spinDownTime.setMinimum(1);
            spinDownTime.setMaximum(100);
            spinDownTime.setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            spinDownTime.setToolTip(tr("Import only every nth frame\nNote: the frame rate should reflect the raw value\nNote: frame subsets for multipage tiff files do not speed up loading"));
            spinDownTime.setObjectName("spinDownTime");
        }
        {
            spinDownSpace.setMinimum(1);
            spinDownSpace.setMaximum(100);
            spinDownSpace.setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            spinDownSpace.setToolTip(tr("Import only every nth pixel"));
            spinDownSpace.setObjectName("spinDownSpace");
        }
        {
            cmdLoad.setText("Load Files");
            cmdLoad.setEnabled(false);
            cmdLoad.setToolTip(tr("Load files. This button will be disabled if:\nFolder is selected and the File Path is not a path containing tiff files\nFile is selected and the File Name is not a tiff file"));
            cmdLoad.setMaximumWidth(320);
            cmdLoad.setObjectName("cmdLoad");
        }
        {
            progBar.setMaximum(100);
            progBar.setVisible(false);
        }
        vlay.setContentsMargins(10, 0, 10, 10);
    }

    void layout()
    {
        contents.setLayout(&vlay);
        vlay.addLayout(&formlay);

        completer.setParent(&contents);
        {
            QVBoxLayout *lay = new QVBoxLayout;
            lay->addWidget(&optFolder);
            lay->addWidget(&optFile);
            formlay.addRow("Dataset Type:", lay);
        }
        formlay.addRow(new QLabel(" "));

        {
            QHBoxLayout *lay = new QHBoxLayout;
            lay->addWidget(&txtFilePath);
            lay->addWidget(&cmdBrowseFilePath);
            lay->setAlignment(Qt::AlignLeft);
            lay->setContentsMargins(0, 0, 0, 0);
            formlay.addRow(&lblFilePath, lay);
        }

        formlay.addRow(tr("Frame Rate:"), &spinFrameRate);
        formlay.addRow(tr("Frame Subset:"), &spinDownTime);
        formlay.addRow(tr("Pixel Subset:"), &spinDownSpace);

        formlay.addRow(new QLabel(" "));
        formlay.addRow(&cmdLoad);
        

        vlay.addWidget(&progBar);
        vlay.addStretch();
    }

    void browse(ImageDataWidget *par)
    {
        const bool isfile = optFile.isChecked();

        QString initpath = QDir::currentPath();
        QString currpath = txtFilePath.text();
        if (!currpath.isEmpty())
        {
            initpath = QFileInfo(currpath).absolutePath();
        }

        QString fileName = QFileDialog::getOpenFileName(par,
                                                        isfile ? tr("Select a Multi-Page Tiff File") : tr("Select a Tiff File in the Dataset"),
                                                        initpath,
                                                        tr("Tiff Files (*.tif *.tiff)"));

        if (!fileName.isEmpty())
        {
            if (isfile)
            {
                txtFilePath.setText(QDir::toNativeSeparators(fileName));
            }
            else
            {
                auto fp = QFileInfo(fileName).absolutePath();
                fp = QDir::toNativeSeparators(fp);
                txtFilePath.setText(fp);
            }
        }
    }

    void filePathChanged(const QString &filepath)
    {
        cmdLoad.setEnabled(false);

        if (optFile.isChecked())
        {
            if (QFileInfo(filepath).isFile() && (QFileInfo(filepath).suffix().toLower() == "tif" || QFileInfo(filepath).suffix().toLower() == "tiff"))
            {
                cmdLoad.setEnabled(true);
            }
        }
        else if (QFileInfo(filepath).isDir())
        {
            QStringList fl = QDir(filepath).entryList(QStringList() << "*.tif"
                                                                    << "*.tiff",
                                                      QDir::Files);
            if (fl.size() > 0)
            {
                cmdLoad.setEnabled(true);
            }
        }
    }

    void load(ImageDataWidget *par)
    {
        bool isfolder = optFolder.isChecked();
        QStringList filelist;
        if (isfolder)
        {
            QFileInfoList fileinfolist = QDir(txtFilePath.text()).entryInfoList(QStringList() << "*.tif"
                                                                                              << "*.tiff",
                                                                                QDir::Files);
            for (auto &filename : fileinfolist)
            {
                filelist << filename.absoluteFilePath();
            }
        }
        else
        {
            filelist = QStringList({txtFilePath.text()});
        }

        emit par->fileLoadRequested(filelist, spinFrameRate.value(), spinDownTime.value(), spinDownSpace.value(), isfolder);
    }

    void optfilefolder()
    {
        if (optFile.isChecked())
        {
            lblFilePath.setText(tr("File Name:"));
            completer.setModel(&fsmodel_file);
            txtFilePath.setToolTip(tr("Path to a multi-page tiff file"));
            cmdBrowseFilePath.setToolTip(tr("Open a dialog to select a file"));
        }
        else
        {
            lblFilePath.setText(tr("File Path:"));
            completer.setModel(&fsmodel_folder);
            txtFilePath.setToolTip(tr("Path to a folder that contains a dataset (i.e. tiff files)"));
            cmdBrowseFilePath.setToolTip(tr("Open a dialog to select a folder"));
        }
        filePathChanged(txtFilePath.text());
    }
};

ImageDataWidget::ImageDataWidget(QWidget *parent) : DockWidgetWithSettings(parent), impl(std::make_unique<pimpl>())
{
    toplay.addWidget(&impl->contents);

    impl->init();
    impl->layout();

    connect(&impl->cmdBrowseFilePath, &QPushButton::clicked, [=]
            { impl->browse(this); });
    connect(&impl->txtFilePath, &QLineEdit::textChanged, [=](const QString &filepath)
            { impl->filePathChanged(filepath); });
    connect(&impl->cmdLoad, &QPushButton::clicked, [=]
            { impl->load(this); });
    connect(&impl->spinFrameRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImageDataWidget::frameRateChanged);
    connect(&impl->optFile, &QRadioButton::toggled, [=]
            { impl->optfilefolder(); });
    connect(&impl->optFolder, &QRadioButton::toggled, [=]
            { impl->optfilefolder(); });
}
ImageDataWidget::~ImageDataWidget() { }

void ImageDataWidget::setContentsEnabled(bool onoff)
{
    impl->contents.setEnabled(onoff);
}

void ImageDataWidget::setProgBar(int val)
{
    if (val >= 0 && val <= impl->progBar.maximum())
    {
        impl->progBar.setVisible(true);
        impl->progBar.setValue(val);
        return;
    }
    impl->progBar.setVisible(false);
}

void ImageDataWidget::saveSettings(QSettings &settings) const
{
    settings.beginGroup("ImageData");
    settings.setValue("dorestore", getSettingsStorage());
    if (getSettingsStorage()) {
        settings.setValue("filepath", impl->txtFilePath.text());
        settings.setValue("framerate", impl->spinFrameRate.value());
        settings.setValue("downtime", impl->spinDownTime.value());
        settings.setValue("downspace", impl->spinDownSpace.value());
        settings.setValue("mode", impl->optFile.isChecked() ? "file" : "folder");
    }
    settings.endGroup();
}
void ImageDataWidget::restoreSettings(QSettings &settings)
{
    settings.beginGroup("ImageData");
    setSettingsStorage(settings.value("dorestore", true).toBool());
    if (getSettingsStorage()) {
        impl->txtFilePath.setText(settings.value("filepath", "").toString());
        impl->spinFrameRate.setValue(settings.value("framerate", 30.).toDouble());
        impl->spinDownTime.setValue(settings.value("downtime", 1).toInt());
        impl->spinDownSpace.setValue(settings.value("downspace", 1).toInt());
        impl->optFile.setChecked(settings.value("mode", "folder").toString() == "file");
    }
    settings.endGroup();
}
void ImageDataWidget::resetSettings()
{
    impl->txtFilePath.setText("");
    impl->spinFrameRate.setValue(30.);
    impl->spinDownTime.setValue(1);
    impl->spinDownSpace.setValue(1);
    impl->optFolder.setChecked(true);
    impl->optfilefolder();
    
}
