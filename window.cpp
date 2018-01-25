#include "window.h"
#include "ui_window.h"
#include "enum.h"
#include <string>
#include "install.h"
#include <QValidator>
#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>

Window::Window(install *ins, bool f, options *d, QWidget *parent) :
    QMainWindow(parent),
    fierst(!f),
    dat(d),
    insDat(ins),
    ui(new Ui::Window)
{
    if(dat->tbios == false) {
        qCritical().noquote() << tr("^This PC does not supported");
    }

    //setWindowIcon(QIcon(":/yourdroid.png"));

    qDebug().noquote() << tr("Start window");

    ui->setupUi(this);
    setLabelSetInfo();

    connect(ui->returnInstallButton,SIGNAL(clicked()),SLOT(returnMainWindow()));
    connect(ui->settingsMini,&QPushButton::clicked,[=](){
        if(ui->windows->currentWidget() == ui->settingsPage) on_back_settings_clicked();
        else Settings_clicked();
    });
    connect(ui->buttonAboutReturn,SIGNAL(clicked()),SLOT(returnMainWindow()));
    connect(ui->buttonReturnMainDelete,SIGNAL(clicked()),SLOT(returnMainWindow()));
    connect(ui->windows, &QStackedWidget::currentChanged, [=](){
        if(ui->windows->currentWidget() != ui->settingsPage) lastPage = ui->windows->currentWidget();
    });
    connect(this, &Window::sendMesToStausbar, this, &Window::receiveMesToStatusbar);
    auto enableApply = [=](int i = 0) {
        ui->applaySettings->setEnabled(true);
    };
//    connect(this, &Window::logFromMainThread, [=](QtMsgType type, QString mess){
//        switch(type) {
//        case QtWarningMsg: qWarning().noquote() << mess; break;
//        case QtCriticalMsg: qCritical().noquote() << mess; break;
//        case QtFatalMsg: qCritical().noquote() << mess; qApp->exit(-1); break;
//        default: qDebug().noquote() << mess;
//        }
//    });
    connect(this, &Window::logFromMainThread, this, &Window::logFromMainThreadSlot);

//    connect(ui->winVer, &QComboBox::currentIndexChanged, [=](){
//        ui->qApplaySettings->setEnabled(true);
//    });
//    connect(ui->typeBios, &QComboBox::currentIndexChanged, [=]() {
//        ui->qApplaySettings->setEnabled(true);
//    });

    ui->progressDelete->setRange(0, 7);
    ui->progressDelete->setValue(0);
    ui->editSizeDataInstall->setValidator(new QIntValidator(100, 999999));
    ui->editDirForInstall->setValidator(new QRegExpValidator(QRegExp("[^а-яА-Я^ ]{0,999}")));
    insDat->execBars(ui->progressInstall, ui->progressDelete, ui->statusbar);

    retranslateUi(QString::fromStdString(_langHelper::to_string(dat->getLang())));

    if(fierst) Settings_clicked();
    else returnMainWindow();


}

void Window::setLabelSetInfo() {
    QString info = QString(dat->tbios ? "Uefi " : "Bios ") + QString(dat->arch ? "x64" : "x86");
#if WIN
    info = QString((dat->winv ? "win. vista+ | " : "win. xp | ")) + info;
#endif
    info = QString("<b>") + info + "<\b>";
    ui->labelSetInfo->setText(info);
}

void Window::retranslateUi(QString lang) {
    QTranslator translator;
    translator.load(QString("yourdroid_") + lang);
    qApp->installTranslator(&translator);
    ui->retranslateUi(this);
    ui->labelVersion->setText(QString("<b>") + tr("Version: ") + VERSION + "<\b>");
}

Window::~Window()
{
    delete ui;
}

void Window::returnMainWindow() {
    setWindowTitle(tr("YourDroid | Main menu"));
    ui->windows->setCurrentWidget(ui->mainWindowPage);
}

void Window::Settings_clicked()
{
    dat->read_set(false);
    ui->typeBios->setCurrentIndex((int)dat->tbios);
    ui->arch->setCurrentIndex((int)dat->arch);
    ui->winVer->setCurrentIndex((int)dat->winv);
#if LINUX
    ui->winVer->setEnabled(false);
#endif
    ui->checkEnableConSettings->setChecked(dat->getConEnable());
    ui->comboLanguageSettings->setCurrentIndex((int)dat->getLang());
    //ui->applaySettings->setEnabled(false);
    ui->windows->setCurrentWidget(ui->settingsPage);
    setWindowTitle(tr("YourDroid | Settings"));
}

void Window::on_applaySettings_clicked()
{
    if(langChanged) {
        langChanged = false;
        qInfo().noquote() << tr("^For applying language application should restart");
//        retranslateUi(QString::fromStdString(_langHelper::to_string(
//                                                 (_lang)ui->comboLanguageSettings->currentIndex())));
    }
    dat->write_set(true, ui->arch->currentIndex(),
                   ui->typeBios->currentIndex(),
                   ui->winVer->currentIndex(),
                   ui->checkEnableConSettings->checkState() == Qt::Checked,
                   (_lang)ui->comboLanguageSettings->currentIndex());
    log::setEnabledCon(ui->checkEnableConSettings->checkState() == Qt::Checked);
    setLabelSetInfo();
}

void Window::on_restoreSettings_clicked()
{
    dat->read_set(true);
    ui->typeBios->setCurrentIndex((int)dat->tbios);
    ui->arch->setCurrentIndex((int)dat->arch);
    ui->winVer->setCurrentIndex((int)dat->winv);
    ui->checkEnableConSettings->setChecked(dat->getConEnable());
    ui->comboLanguageSettings->setCurrentIndex((int)dat->getLang());
}

void Window::on_installButtonMain_clicked()
{
    ui->radioInstallOnPart->setEnabled(false);
    ui->radioInstallOnDir->setChecked(true);

    ui->radioDownload->setEnabled(false);
    ui->radioChooseFromDisk->setChecked(true);

    //ui->progressInstall->setRange(0, 100);
    ui->progressInstall->setValue(0);

    setWindowTitle(tr("YourDroid | Install"));
    ui->windows->setCurrentWidget(ui->installPage);
    ui->comboBoot->clear();
    if(!dat->tbios) {
        ui->comboBoot->addItem("Grub legasy");
        //if(dat->winv && dat->os) ui->comboBoot->addItem("Windows BOOTMGR");
    }
    //if(!dat->winv && dat->os) ui->comboBoot->addItem("Windows NTLDR");
    if(dat->tbios) {
        //ui->comboBoot->addItem("rEFInd");
        if(dat->os) ui->comboBoot->addItem("Gummiboot");
        else ui->comboBoot->addItem("Grub2");
    }
}

void Window::on_buttonChooseImage_clicked()
{
    QString image = QFileDialog::getOpenFileName(0, tr("Choose image for install"), "", "*.iso");
    if(image.length() > 0) {
        ui->editImageFromDisk->setText(image);
        qDebug().noquote() << qApp->translate("log", "Choose image for install: ") + image;
    }
}

void Window::on_back_settings_clicked()
{
    if(fierst) {
        fierst = !fierst;
        on_applaySettings_clicked();
        returnMainWindow();
    }
    else if(lastPage == ui->settingsPage) Settings_clicked();
    else if(lastPage == ui->mainWindowPage) returnMainWindow();
    else if(lastPage == ui->aboutPage) on_buttonAboutMain_clicked();
    else if(lastPage == ui->installPage) on_installButtonMain_clicked();
    else if(lastPage == ui->daletePage) on_deleteButtonMain_clicked();
}

void Window::on_buttonChooseDirForInstall_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(0, tr("Choose directory for install"), "");
    if(dir.length() > 0) {
        ui->editDirForInstall->setText(dir);
        qDebug().noquote() << qApp->translate("log", "Choose dir for install: ") + dir;
    }
}

void Window::on_buttonInstallInstall_clicked()
{
    ui->progressInstall->setValue(0);
    ui->returnInstallButton->setEnabled(false);
    ui->buttonInstallInstall->setEnabled(false);
    ui->statusbar->showMessage(tr("Checking"));
    qDebug().noquote() << tr("Checking data for install...");
    QString image, dir, name;
    auto end = [=](){
        ui->statusbar->showMessage("Готово");
        ui->returnInstallButton->setEnabled(true);
        ui->buttonInstallInstall->setEnabled(true);
    };
    if((image = ui->editImageFromDisk->text()).length() == 0) {
        qCritical().noquote() << tr("^Did not choose image");
        end();
        return;
    }
    if(!QFile::exists(image)) {
        qCritical().noquote() << tr("^Choosen image does not exist");
        end();
        return;
    }
    if((dir = ui->editDirForInstall->text()).length() == 0 ) {
        qCritical().noquote() << tr("^Did not choose folder");
        end();
        return;
    }
    if((dir = ui->editDirForInstall->text()).length() == OS * 2 + 1 ) {
        qCritical().noquote() << tr("^Choosen folder is root");
        end();
        return;
    }
    if(!ui->editDirForInstall->hasAcceptableInput()) {
        qCritical().noquote() << tr("^Invalid path");
        end();
        return;
    }
//    if(!(new QDir())->exists(dir)) {
//        qCritical().noquote() << tr("^Selected folder does not exist");
//        end();
//        return;
//    }
    if((name = ui->editName->text()).length() == 0) {
        qCritical().noquote() << tr("^Did not fill in the name");
        end();
        return;
    }
    if((name = ui->editSizeDataInstall->text()).length() == 0) {
        qCritical().noquote() << tr("^Did not fill in the size of data.img");
        end();
        return;
    }
    for(int i = 0; i < insDat->systemsVector().length(); i++) {
        if(ui->editName->text() == (insDat->systemsVector())[i].name) {
            qCritical().noquote() << QObject::tr("^The system with written name already exists");
            end();
            return;
        }
    }
    if(!QDir(ui->editDirForInstall->text()).isEmpty()) { //if dir is not empty
        qWarning().noquote() << tr("^Choosen folder is not empty. Some files will overwrite. Press cancel to abort|+-|");
        if(log::getLastPressedButton() == QMessageBox::Cancel) {
            end();
            return;
        }
    }

#if LINUX
    QPair<bool, QString> result = insDat->mountImage(ui->editImageFromDisk->text());
    if(!result.first) {
        qCritical().noquote() << QObject::tr("^Could not mount image: %1").arg(result.second);
        insDat->unmountImage();
        end();
        return;
    }
#endif

    int ret = 0;
    if(!(ret = insDat->isInvalidImage(
            #if WIN
                ui->editImageFromDisk->text()
            #endif
                ))) {
        if(ret != 2) qCritical().noquote() << QObject::tr("^Image has not needed files");
        end();
        return;
    }

    qDebug().noquote() << QObject::tr("Data of install is valid");
    ui->statusbar->showMessage(QObject::tr("Data of install is valid"));

    //ui->progressInstall->setRange(0, (ui->radioChooseFromDisk->isChecked() && !ui->radioDownload->isChecked()) ? 125 : 150);
    QString boot = ui->comboBoot->currentText();
    if(boot == "Grub legasy") boot = "grub_legasy";
    else if(boot == "Windows NTLDR") boot = "ntldr";
    else if(boot == "Windows BOOTMGR") boot = "bootmgr";
    else boot = boot.toLower();
    _bootloader bootloader = _bootloaderHelper::from_string(boot.toStdString());
    _typePlace typePlace = ui->radioInstallOnDir->isChecked() ? _typePlace::dir : _typePlace::partition;
#define CHECK_ABORT() if(abort) return;
    insDat->addSystem(bootloader, typePlace, ui->editDirForInstall->text(), ui->editImageFromDisk->text(), ui->editName->text(), false);
    QFutureWatcher<void> *resMonitor = new QFutureWatcher<void>;
    connect(resMonitor, &QFutureWatcher<void>::finished, [&](){
        ui->statusbar->showMessage("Готово");
        ui->returnInstallButton->setEnabled(true);
        ui->buttonInstallInstall->setEnabled(true);
        ui->progressInstall->setValue(ui->progressInstall->maximum());
    });

    int step = insDat->sizeOfFiles() / 4;
    ui->progressInstall->setRange(0, step * ((OS) ? 6 : 7));

    int progressComplete = 0;

    connect(insDat, &install::progressChange, this, [&](int progress){
        ui->progressInstall->setValue(progress + progressComplete);
    });
    connect(insDat, &install::fileEnded, this, [&](int value){
        progressComplete += value;
        ui->progressInstall->setValue(progressComplete);
    });
    connect(this, &Window::progressAddStep, this, [&](){
        ui->progressInstall->setValue(ui->progressInstall->maximum() / 7);
    });
    connect(insDat, &install::logWindow, this, &Window::logFromMainThreadSlot);

    auto res = QtConcurrent::run([=](){ // auto - QFuture
        bool abort = false;
        connect(insDat, &install::abort, [&](QString mes){
            abort = true;
            emit logFromMainThread(QtCriticalMsg,
                                   tr("^Fatal error while installing: %1")
                                   .arg(mes.isEmpty() ? "Message of error is empty" : mes));
#if LINUX
            insDat->unmountImage();
#endif
        });
        qDebug().noquote() << tr("Start install");
        qDebug().noquote() << tr("Unpacking iso...");
        emit sendMesToStausbar(tr("Unpacking iso..."));
        insDat->unpackSystem();
        CHECK_ABORT();
        qDebug().noquote() << tr("Creating data.img...");
        emit sendMesToStausbar(tr("Creating data.img..."));
        emit progressAddStep();
        insDat->createDataImg(ui->editSizeDataInstall->text().toInt());
        CHECK_ABORT();
        qDebug().noquote() << tr("Installing bootloader...");
        emit sendMesToStausbar(tr("Installing bootloader..."));
        emit progressAddStep();
        insDat->registerBootloader();
        CHECK_ABORT();
#if LINUX
        emit sendMesToStausbar(tr("Unmounting image..."));
        emit progressAddStep();
        insDat->unmountImage();
#endif
        insDat->systemEnd();
        insDat->write();
        CHECK_ABORT();
        qDebug().noquote() << tr("Finish install");
    });
    resMonitor->setFuture(res);
#undef CHECK_ABORT
}

void Window::on_buttonAboutMain_clicked()
{
    setWindowTitle(tr("YourDroid | About"));
    ui->windows->setCurrentWidget(ui->aboutPage);
}

void Window::on_comboBoot_currentIndexChanged(const QString &arg1)
{
    qDebug().noquote() << qApp->translate("log", "Choose ") + arg1;
    if(arg1 == "Grub2") {
        ui->labelAboutBootloader->setText("Рекомендуется для компьютеров.");
        ui->labelAboutBootloader_2->setText("Текстовый.");
    }
    else if(arg1 == "Gummiboot") {
        ui->labelAboutBootloader->setText("Рекомендуется для планшетов.");
        ui->labelAboutBootloader_2->setText("Управление качалкой громкости.");
    }
    else if(arg1 == "rEFInd") ui->labelAboutBootloader->setText("Поддержка сенс. экрана, работает не везде");
    else if(arg1 == "Windows BOOTMGR") ui->labelAboutBootloader->setText("Рекомендуется для планшетов, c win 8+. В win 8+ сенсорный");
    else if(arg1 == "Windows NTDLR") ui->labelAboutBootloader->setText("Не рекомендуется. Текстовый");
    else if(arg1 == "Grub legasy") ui->labelAboutBootloader->setText("Рекомендуется для старых компьютеров. Текстовый");
}

void Window::on_deleteButtonMain_clicked()
{
    ui->progressDelete->setValue(0);

    qDebug().noquote() << tr("Clearing systems list...");
    ui->comboSystemDelete->blockSignals(true);
    ui->comboSystemDelete->clear();
    ui->comboSystemDelete->blockSignals(false);

    qDebug().noquote() << tr("Filling systems list...");
    for(auto sys : insDat->systemsVector()) ui->comboSystemDelete->addItem(sys.name);

    setWindowTitle(tr("YourDroid | Delete"));
    ui->windows->setCurrentWidget(ui->daletePage);
}

void Window::on_comboSystemDelete_currentIndexChanged(int index)
{
    ui->labelPlaceDeleteText->setText(insDat->systemsVector()[index].place);
    ui->labelbootloaderDeleteText->setText(QString::fromStdString(_bootloaderHelper::to_string(insDat->systemsVector()[index].bootloader)));
}
void Window::on_buttonDeleteDelete_clicked()
{
    if(ui->comboSystemDelete->currentIndex() == -1) return;
    ui->buttonDeleteDelete->setEnabled(false);
    ui->buttonReturnMainDelete->setEnabled(false);
    ui->settingsMini->setEnabled(false);
    ui->comboSystemDelete->setEnabled(false);
    int num = ui->comboSystemDelete->currentIndex();
    qDebug().noquote() << qApp->translate("log", "Deleting ") + (insDat->systemsVector().begin() + num)->name;
    insDat->delSystemFiles(num);
    insDat->deleteBootloader(num);
    insDat->oldSysEdit() = true;
    insDat->deleteEntry(num);
    on_deleteButtonMain_clicked();
    ui->statusbar->showMessage(tr("Ready"));
    ui->buttonDeleteDelete->setEnabled(true);
    ui->buttonReturnMainDelete->setEnabled(true);
    ui->settingsMini->setEnabled(true);
    ui->comboSystemDelete->setEnabled(true);
}

void Window::receiveMesToStatusbar(QString mes) {
    ui->statusbar->showMessage(mes);
}

void Window::closeEvent(QCloseEvent *event) {
    exiting = true;
    emit closed();
    event->accept();
}

void Window::changeEvent(QEvent *event) {
//    if(event->type() == QEvent::WindowStateChange) emit deactived();
//    else if(event->type() == QEvent::WindowActivate) emit actived();
    event->accept();
}

void Window::consoleHided() {
    if(exiting) return;
    dat->setConEnable(false);
    dat->autowrite_set();
    ui->checkEnableConSettings->setChecked(false);
}

void Window::on_comboLanguageSettings_currentIndexChanged(int index)
{
    langChanged = true;
}

void Window::logFromMainThreadSlot(QtMsgType type, QString mess) {
    switch(type) {
    case QtWarningMsg: qWarning().noquote() << mess; break;
    case QtCriticalMsg: qCritical().noquote() << mess; break;
    case QtFatalMsg: qCritical().noquote() << mess; qApp->exit(-1); break;
    default: qDebug().noquote() << mess;
    }
}
