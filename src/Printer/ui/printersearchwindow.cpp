/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     liurui <liurui_cm@deepin.com>
 *
 * Maintainer: liurui <liurui_cm@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "printersearchwindow.h"
#include "zdrivermanager.h"
#include "cupsattrnames.h"
#include "addprinter.h"
#include "common.h"
#include "uisourcestring.h"
#include "permissionswindow.h"
#include "printerservice.h"

#include <DIconButton>
#include <DComboBox>
#include <DListWidget>
#include <DSpinner>
#include <DWidgetUtil>
#include <DListView>
#include <DLineEdit>
#include <DTitlebar>
#include <DMessageManager>
#include <DDialog>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QStackedWidget>
#include <QRegExp>

PrinterSearchWindow::PrinterSearchWindow(QWidget *parent)
    : DMainWindow(parent)
    , m_isAutoInstallDriver(false)
    , m_isManInstallDriver(false)
    , m_isURIInstallDriver(false)
{
    initUi();
    initConnections();
}

PrinterSearchWindow::~PrinterSearchWindow()
{
    if (m_pInstallDriverWindow)
        m_pInstallDriverWindow->deleteLater();
    if (m_pInstallPrinterWindow)
        m_pInstallPrinterWindow->deleteLater();
}

void PrinterSearchWindow::initUi()
{
    titlebar()->setMenuVisible(false);
    titlebar()->setTitle("");
    titlebar()->setIcon(QIcon(":/images/dde-printer.svg"));
    // 去掉最大最小按钮
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);
    setWindowModality(Qt::ApplicationModal);
    resize(682, 532);

    // 左侧菜单列表
    m_pTabListView = new DListView();
    QStandardItemModel *pTabModel = new QStandardItemModel(m_pTabListView);
    QStandardItem *pWidgetItemAuto = new QStandardItem(QIcon::fromTheme("dp_auto_searching"), tr("Discover printer"));
    pWidgetItemAuto->setSizeHint(QSize(108, 48));
    QStandardItem *pWidgetItemManual = new QStandardItem(QIcon::fromTheme("dp_manual_search"), tr("Find printer"));
    pWidgetItemManual->setSizeHint(QSize(108, 48));
    QStandardItem *pWidgetItemURI = new QStandardItem(QIcon::fromTheme("dp_uri"), tr("Enter URI"));
    pWidgetItemURI->setSizeHint(QSize(108, 48));
    pTabModel->appendRow(pWidgetItemAuto);
    pTabModel->appendRow(pWidgetItemManual);
    pTabModel->appendRow(pWidgetItemURI);
    m_pTabListView->setModel(pTabModel);
    m_pTabListView->setCurrentIndex(pTabModel->index(0, 0));
    m_pTabListView->setFocusPolicy(Qt::NoFocus);
    m_pTabListView->setFixedWidth(128);


    m_pStackedWidget = new QStackedWidget();
    // 右侧 自动查找
    m_pLabelPrinter = new QLabel(tr("Select a printer"));
    QFont font;
    font.setBold(true);
    m_pLabelPrinter->setFont(font);
    m_pBtnRefresh = new DIconButton(this);
    m_pBtnRefresh->setIcon(QIcon::fromTheme("dp_refresh"));
    m_pBtnRefresh->setFixedSize(36, 36);
    m_pBtnRefresh->setToolTip(tr("Refresh"));
    QHBoxLayout *pHLayout1 = new QHBoxLayout();
    pHLayout1->addWidget(m_pLabelPrinter);
    pHLayout1->addWidget(m_pBtnRefresh, 0, Qt::AlignRight);

    m_pPrinterListViewAuto = new DListView(this);
    m_pPrinterListViewAuto->setSpacing(10);
    m_pPrinterListViewAuto->setEditTriggers(DListView::NoEditTriggers);
    m_pPrinterListViewAuto->setTextElideMode(Qt::ElideRight);
    m_pPrinterListModel = new QStandardItemModel(m_pPrinterListViewAuto);
    m_pPrinterListViewAuto->setModel(m_pPrinterListModel);

    QLabel *pLabelDriver1 = new QLabel(UI_PRINTERSEARCH_DRIVER);
    m_pAutoDriverCom = new QComboBox();
    m_pAutoDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
    QHBoxLayout *pHLayout2 = new QHBoxLayout();
    pHLayout2->addWidget(pLabelDriver1);
    pHLayout2->addWidget(m_pAutoDriverCom, 1);
    pHLayout2->setSpacing(20);

    m_pAutoSpinner = new DSpinner();
    m_pAutoSpinner->setFixedSize(30, 30);
    m_pAutoInstallDriverBtn = new QPushButton(tr("Start Installation"));
    m_pAutoInstallDriverBtn->setEnabled(false);
    m_pAutoInstallDriverBtn->setFixedHeight(30);
    QHBoxLayout *pHLayout3 = new QHBoxLayout();
    pHLayout3->addStretch();
    pHLayout3->addWidget(m_pAutoInstallDriverBtn);
    pHLayout3->addStretch();

    QVBoxLayout *pVLayout1 = new QVBoxLayout();
    pVLayout1->addLayout(pHLayout1);
    pVLayout1->addWidget(m_pPrinterListViewAuto);
    pVLayout1->addLayout(pHLayout2);
    pVLayout1->addWidget(m_pAutoSpinner, 0, Qt::AlignCenter);
    pVLayout1->addLayout(pHLayout3);

    QWidget *pWidget1 = new QWidget();
    pWidget1->setLayout(pVLayout1);
    m_pStackedWidget->addWidget(pWidget1);
    // 右侧 手动查找
    m_pLabelLocation = new QLabel(tr("IP address"));
    m_pLineEditLocation = new QLineEdit();
    m_pLineEditLocation->setPlaceholderText(tr("Please enter the IP address"));
    m_pBtnFind = new QPushButton(tr("Find"));
    QHBoxLayout *pHLayout4 = new QHBoxLayout();
    pHLayout4->addWidget(m_pLabelLocation);
    pHLayout4->addSpacing(26);
    pHLayout4->addWidget(m_pLineEditLocation);
    pHLayout4->addSpacing(10);
    pHLayout4->addWidget(m_pBtnFind);

    m_pPrinterListViewManual = new DListView();
    m_pPrinterListViewManual->setSpacing(10);
    m_pPrinterListViewManual->setEditTriggers(DListView::NoEditTriggers);
    m_pPrinterListViewManual->setTextElideMode(Qt::ElideRight);
    m_pPrinterListModelManual = new QStandardItemModel();
    m_pPrinterListViewManual->setModel(m_pPrinterListModelManual);

    QLabel *pLabelDriver2 = new QLabel(UI_PRINTERSEARCH_DRIVER);
    m_pManDriverCom = new QComboBox();
    m_pManDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
    QHBoxLayout *pHLayout5 = new QHBoxLayout();
    pHLayout5->addWidget(pLabelDriver2);
    pHLayout5->addWidget(m_pManDriverCom, 1);
    pHLayout5->setSpacing(20);

    m_pManSpinner = new DSpinner();
    m_pManSpinner->setFixedSize(30, 30);
    m_pManSpinner->setVisible(false);
    m_pManInstallDriverBtn = new QPushButton(UI_PRINTERSEARCH_INSTALLDRIVER_NEXT);
    m_pManInstallDriverBtn->setEnabled(false);
    m_pManInstallDriverBtn->setFixedHeight(30);
    QHBoxLayout *pHLayout6 = new QHBoxLayout();
    pHLayout6->addStretch();
    pHLayout6->addWidget(m_pManInstallDriverBtn);
    pHLayout6->addStretch();
    QVBoxLayout *pVLayout2 = new QVBoxLayout();
    pVLayout2->addLayout(pHLayout4);
    pVLayout2->addWidget(m_pPrinterListViewManual);
    pVLayout2->addLayout(pHLayout5);
    pVLayout2->addWidget(m_pManSpinner, 0, Qt::AlignCenter);
    pVLayout2->addLayout(pHLayout6);
    QWidget *pWidget2 = new QWidget();
    pWidget2->setLayout(pVLayout2);
    m_pStackedWidget->addWidget(pWidget2);

    // 右侧 URI
    m_pLabelURI = new QLabel(tr("URI"));
    m_pLabelURI->setAlignment(Qt::AlignCenter);
    m_pLineEditURI = new QLineEdit();
    m_pLineEditURI->setPlaceholderText(tr("Enter device URI"));
    m_pLabelTip = new QLabel(tr("Examples:") + "\n" + UI_PRINTERSEARCH_URITIP);
    QLabel *pLabelDriver3 = new QLabel(UI_PRINTERSEARCH_DRIVER);
    m_pURIDriverCom = new QComboBox();
    m_pURIDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
    QHBoxLayout *pHLayout7 = new QHBoxLayout();
    pHLayout7->addWidget(pLabelDriver3);
    pHLayout7->addWidget(m_pURIDriverCom, 1);
    pHLayout7->setSpacing(20);


    m_pURIInstallDriverBtn = new QPushButton(UI_PRINTERSEARCH_INSTALLDRIVER_NEXT);
    m_pURIInstallDriverBtn->setEnabled(false);
    m_pURIInstallDriverBtn->setFixedHeight(30);
    QHBoxLayout *pHLayout8 = new QHBoxLayout();
    pHLayout8->addStretch();
    pHLayout8->addWidget(m_pURIInstallDriverBtn);
    pHLayout8->addStretch();

    QGridLayout *pURIGLayout = new QGridLayout();
    pURIGLayout->addWidget(m_pLabelURI, 0, 0);
    pURIGLayout->addWidget(m_pLineEditURI, 0, 1);
    pURIGLayout->addWidget(m_pLabelTip, 1, 1);
    pURIGLayout->setSpacing(20);
    pURIGLayout->setRowStretch(2, 1);

    QVBoxLayout *pURIVLayout = new QVBoxLayout();
    pURIVLayout->addLayout(pURIGLayout);
    pURIVLayout->addLayout(pHLayout7);
    pURIVLayout->addLayout(pHLayout8);

    QWidget *pWidget3 = new QWidget();
    pWidget3->setLayout(pURIVLayout);
    m_pStackedWidget->addWidget(pWidget3);


    // 右侧整体布局
    QVBoxLayout *pRightVLayout = new QVBoxLayout();
    pRightVLayout->addWidget(m_pStackedWidget);
    pRightVLayout->setContentsMargins(10, 5, 10, 20);


    // 整体布局
    QHBoxLayout *m_pMainHLayout = new QHBoxLayout();
    m_pMainHLayout->addWidget(m_pTabListView, 1);
    m_pMainHLayout->addLayout(pRightVLayout, 2);
    m_pMainHLayout->setContentsMargins(0, 0, 0, 0);
    QWidget *pCentralWidget = new QWidget();
    pCentralWidget->setLayout(m_pMainHLayout);
    takeCentralWidget();
    setCentralWidget(pCentralWidget);

    // 手动安装驱动
    m_pInstallDriverWindow = new InstallDriverWindow();
    m_pInstallDriverWindow->setPreWidget(this);
    Dtk::Widget::moveToCenter(this);
}

void PrinterSearchWindow::initConnections()
{
    connect(m_pTabListView, QOverload<const QModelIndex &>::of(&DListView::currentChanged), this, &PrinterSearchWindow::listWidgetClickedSlot);

    connect(m_pPrinterListViewAuto, &DListView::clicked, this, &PrinterSearchWindow::printerListClickedSlot);

    connect(m_pPrinterListViewManual, &DListView::clicked, this, &PrinterSearchWindow::printerListClickedSlot);

    connect(m_pBtnRefresh, &DIconButton::clicked, this, &PrinterSearchWindow::refreshPrinterListSlot);

    connect(m_pBtnFind, &QPushButton::clicked, this, &PrinterSearchWindow::searchPrintersByManual);

    connect(m_pAutoInstallDriverBtn, &QPushButton::clicked, this, &PrinterSearchWindow::installDriverSlot);
    connect(m_pManInstallDriverBtn, &QPushButton::clicked, this, &PrinterSearchWindow::installDriverSlot);
    connect(m_pURIInstallDriverBtn, &QPushButton::clicked, this, &PrinterSearchWindow::installDriverSlot);

    connect(m_pAutoDriverCom, QOverload<int >::of(&QComboBox::currentIndexChanged), this, &PrinterSearchWindow::driverChangedSlot);
    connect(m_pManDriverCom, QOverload<int >::of(&QComboBox::currentIndexChanged), this, &PrinterSearchWindow::driverChangedSlot);
    connect(m_pURIDriverCom, QOverload<int >::of(&QComboBox::currentIndexChanged), this, &PrinterSearchWindow::driverChangedSlot);


    connect(m_pLineEditURI, &QLineEdit::textChanged, this, &PrinterSearchWindow::lineEditURIChanged);

    connect(m_pLineEditLocation, &QLineEdit::editingFinished, this, [ this ]() {
        // 按下enter会触发两次信号，需要过滤掉失去焦点之后的信号 并且判断校验结果
        if (m_pLineEditLocation->hasFocus())
            searchPrintersByManual();
    });
}

void PrinterSearchWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    //启动自动刷新打印机列表
    if (m_pTabListView->currentIndex().row() == 0 && m_pPrinterListViewAuto->count() == 0)
        refreshPrinterListSlot();
}

QStandardItem *PrinterSearchWindow::getCheckedItem(int type)
{
    QStandardItem *pCheckedItem = nullptr;
    if (type == 0) {
        int count = m_pPrinterListModel->rowCount();
        for (int i = 0; i < count; ++ i) {
            QStandardItem *pTemp = m_pPrinterListModel->item(i);
            if (pTemp->checkState() == Qt::Checked) {
                pCheckedItem = pTemp;
                break;
            }
        }
    } else if (type == 1) {
        int count = m_pPrinterListModelManual->rowCount();
        for (int i = 0; i < count; ++ i) {
            QStandardItem *pTemp = m_pPrinterListModelManual->item(i);
            if (pTemp->checkState() == Qt::Checked) {
                pCheckedItem = pTemp;
                break;
            }
        }
    }
    return pCheckedItem;
}

void PrinterSearchWindow::autoInstallPrinter(int type, const TDeviceInfo &device)
{
    QComboBox *pDriverCombo = nullptr;
    if (type == 0) {
        pDriverCombo = m_pAutoDriverCom;
    } else if (type == 1) {
        pDriverCombo = m_pManDriverCom;
    } else {
        pDriverCombo = m_pURIDriverCom;
    }
    QMap<QString, QVariant> solution = pDriverCombo->currentData().value<QMap<QString, QVariant>>();
    AddPrinterTask *task = g_addPrinterFactoty->createAddPrinterTask(device, solution);
    close();
    m_pInstallPrinterWindow = new InstallPrinterWindow(this);
    m_pInstallPrinterWindow->show();
    m_pInstallPrinterWindow->setTask(task);
    m_pInstallPrinterWindow->setDevice(device);
    QMap<QString, QVariant> dataMap;
    int driverCount = pDriverCombo->count();
    for (int i = 0; i < driverCount; ++i) {
        if (i != pDriverCombo->currentIndex() && i != pDriverCombo->count() - 1) {
            dataMap.insert(pDriverCombo->itemText(i), pDriverCombo->itemData(i));
        }
    }
    m_pInstallPrinterWindow->copyDriverData(dataMap);
    connect(task, &AddPrinterTask::signalStatus, m_pInstallPrinterWindow, &InstallPrinterWindow::receiveInstallationStatusSlot);
    connect(m_pInstallPrinterWindow, &InstallPrinterWindow::updatePrinterList, this, &PrinterSearchWindow::updatePrinterList);
    task->doWork();
}


void PrinterSearchWindow::listWidgetClickedSlot(const QModelIndex &previous)
{
    Q_UNUSED(previous)
    // 0 自动查找 ;1 手动；2 URI
    if (m_pStackedWidget->count() > m_pTabListView->currentIndex().row()) {
        m_pStackedWidget->setCurrentIndex(m_pTabListView->currentIndex().row());
        if (m_pTabListView->currentIndex().row() == 0) {


        } if (m_pTabListView->currentIndex().row() == 1) {


        } else if (m_pTabListView->currentIndex().row() == 2) {

        }
    }
}

void PrinterSearchWindow::getDeviceResultSlot(int id, int state)
{
    Q_UNUSED(id)
    if (state == ETaskStatus::TStat_Suc || state == ETaskStatus::TStat_Fail) {
        sender()->deleteLater();
        m_pAutoSpinner->stop();
        m_pAutoSpinner->setVisible(false);
        m_pAutoInstallDriverBtn->setVisible(true);
        if (m_pPrinterListViewAuto->count() > 0) {
            m_pAutoInstallDriverBtn->setEnabled(true);
        } else {
            m_pAutoInstallDriverBtn->setEnabled(false);
        }
        m_pBtnRefresh->blockSignals(false);
        m_pPrinterListViewAuto->blockSignals(false);
    } else if (state == ETaskStatus::TStat_Update) {
        ReflushDevicesByBackendTask *task = static_cast<ReflushDevicesByBackendTask *>(sender());
        if (!task)
            return;
        QList<TDeviceInfo> deviceList = task->getResult();
        int index = m_pPrinterListViewAuto->count();
        for (; index < deviceList.count(); index++) {
            TDeviceInfo info = deviceList[index];

            QString strDesc = info.strInfo.isEmpty() ? info.strMakeAndModel : info.strInfo + " ";
            QString strUri = info.uriList[0];
            if (strDesc.isEmpty())
                strDesc = strUri;
            else {
                QString strHost = getHostFromUri(strUri);
                QString protocol = strUri.left(strUri.indexOf(":/"));
                if (!strHost.isEmpty())
                    strDesc += QString(" (%1)").arg(strHost);
                if (!protocol.isEmpty())
                    strDesc += QObject::tr("(use %1 protocol)").arg(protocol);
            }
            QStandardItem *pItem = new QStandardItem(info.strName);
            pItem->setText(strDesc);
            pItem->setIcon(QIcon::fromTheme("dp_printer_list"));
            // 将结构体转化为QVariant,需要再转回来
            pItem->setData(QVariant::fromValue(info), Qt::UserRole + 1);
            pItem->setSizeHint(QSize(480, 50));
            m_pPrinterListModel->appendRow(pItem);
        }
    }
}

void PrinterSearchWindow::getDeviceResultByManualSlot(int id, int state)
{
    Q_UNUSED(id)
    if (state == ETaskStatus::TStat_Suc || state == ETaskStatus::TStat_Fail) {
        sender()->deleteLater();
        m_pManSpinner->stop();
        m_pManSpinner->setVisible(false);
        m_pManInstallDriverBtn->setVisible(true);
        if (m_pPrinterListViewManual->count() > 0) {
            m_pManInstallDriverBtn->setEnabled(true);
        } else {
            m_pManInstallDriverBtn->setEnabled(false);
        }
        //解除阻塞
        m_pBtnFind->blockSignals(false);
        m_pLineEditLocation->blockSignals(false);
        m_pPrinterListViewManual->blockSignals(false);
        if (TStat_Fail == state) {
            DDialog dlg(this);
            TaskInterface *task = static_cast<TaskInterface *>(sender());

            dlg.setMessage(task->getErrorString());
            dlg.setIcon(QIcon(":/images/warning_logo.svg"));
            dlg.addButton(tr("Sure"));
            dlg.setContentsMargins(10, 15, 10, 15);
            dlg.setModal(true);
            dlg.setFixedSize(452, 202);
            dlg.exec();
        }
    } else if (state == ETaskStatus::TStat_Update) {
        ReflushDevicesByHostTask *task = static_cast<ReflushDevicesByHostTask *>(sender());
        if (!task)
            return;
        QList<TDeviceInfo> deviceList = task->getResult();
        int index = m_pPrinterListViewManual->count();
        for (; index < deviceList.count(); index++) {
            TDeviceInfo info = deviceList[index];

            qInfo() << "Update" << info.toString();
            QString strDesc = info.strInfo.isEmpty() ? info.strMakeAndModel : info.strInfo + " ";
            QString strUri = info.uriList[0];
            if (strDesc.isEmpty())
                strDesc = strUri;
            else {
                QString protocol = strUri.left(strUri.indexOf(":/"));
                if (!protocol.isEmpty())
                    strDesc += QObject::tr("(use %1 protocol)").arg(protocol);
            }
            QStandardItem *pItem = new QStandardItem(info.strName);
            pItem->setText(strDesc);
            pItem->setIcon(QIcon::fromTheme("dp_printer_list"));
            // 将结构体转化为QVariant,需要再转回来
            pItem->setData(QVariant::fromValue(info), Qt::UserRole + 1);
            pItem->setSizeHint(QSize(480, 50));
            m_pPrinterListModelManual->appendRow(pItem);
        }
    }
}

void PrinterSearchWindow::printerListClickedSlot(const QModelIndex &index)
{
    DriverManager *pDManager = DriverManager::getInstance();
    QList<QMap<QString, QString>> driverList;
    QVariant temp ;
    if (sender() == m_pPrinterListViewAuto) {
        temp = m_pPrinterListViewAuto->currentIndex().data(Qt::UserRole + 1);
        for (int row = 0; row < m_pPrinterListModel->rowCount(); ++row) {
            if (row != index.row())
                m_pPrinterListModel->item(row)->setCheckState(Qt::Unchecked);
            else {
                m_pPrinterListModel->item(row)->setCheckState(Qt::Checked);
            }
        }
        m_pPrinterListViewAuto->blockSignals(true);
        m_pAutoInstallDriverBtn->setVisible(false);
        m_pAutoSpinner->setVisible(true);
        m_pAutoSpinner->start();
        TDeviceInfo device = temp.value<TDeviceInfo>();
        DriverSearcher *pDriverSearcher = pDManager->createSearcher(device);
        connect(pDriverSearcher, &DriverSearcher::signalDone, this, &PrinterSearchWindow::driverAutoSearchedSlot);
        pDriverSearcher->startSearch();
    } else {
        temp = m_pPrinterListViewManual->currentIndex().data(Qt::UserRole + 1);
        for (int row = 0; row < m_pPrinterListModelManual->rowCount(); ++row) {
            if (row != index.row())
                m_pPrinterListModelManual->item(row)->setCheckState(Qt::Unchecked);
            else {
                m_pPrinterListModelManual->item(row)->setCheckState(Qt::Checked);
            }
        }
        //驱动下拉列表的数据是异步获取的，在获取到数据之前先阻塞当前列表的信号，防止本次数据获取之前，用户又开始点击另外的列
        // 导致驱动和打印机数据无法对应，安装的时候崩溃
        m_pPrinterListViewManual->blockSignals(true);
        m_pManInstallDriverBtn->setVisible(false);
        m_pManSpinner->setVisible(true);
        m_pManSpinner->start();
        TDeviceInfo device = temp.value<TDeviceInfo>();
        DriverSearcher *pDriverSearcher = pDManager->createSearcher(device);
        connect(pDriverSearcher, &DriverSearcher::signalDone, this, &PrinterSearchWindow::driverManSearchedSlot);
        pDriverSearcher->startSearch();
    }


}

void PrinterSearchWindow::driverAutoSearchedSlot()
{
    DriverSearcher *pDriverSearcher = static_cast<DriverSearcher *>(sender());
    if (pDriverSearcher) {
        m_pAutoDriverCom->clear();
        QList<QMap<QString, QVariant>> drivers = pDriverSearcher->getDrivers();
        pDriverSearcher->deleteLater();
        foreach (auto driver, drivers) {
            //将驱动结构体存在item中，方便后续安装打印机
            QString strDesc;
            if (driver.value("ppd-make-and-model").isNull())
                strDesc = driver.value("ppd-make").toString() + " " + driver.value("ppd-model").toString();
            else
                strDesc = driver.value("ppd-make-and-model").toString();

            bool isExcat = true;
            QString strReplace;
            //本地都是精确匹配，服务器获取可能不是精确匹配
            if (driver.value(SD_KEY_from).toInt() == PPDFrom_Server) {
                isExcat = driver.value("excat").toBool();
            }
            if (isExcat) strReplace = tr(" (recommended)");
            //去掉自带的recommended字段
            strDesc.replace("(recommended)", "");
            //如果是精确匹配添加 推荐 字段
            strDesc += strReplace;
            m_pAutoDriverCom->addItem(strDesc, QVariant::fromValue(driver));
        }
        m_pAutoDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
    }
    m_pAutoSpinner->stop();
    m_pAutoSpinner->setVisible(false);
    m_pAutoInstallDriverBtn->setVisible(true);
    m_pAutoInstallDriverBtn->setEnabled(true);
    if (m_pAutoDriverCom->count() >= 2) {
        m_pAutoInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_AUTO);
        m_isAutoInstallDriver = true;
    } else {
        m_pAutoInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_NEXT);
        m_isAutoInstallDriver = false;
    }
    //解除信号阻塞，重新响应列表的点击事件

    m_pPrinterListViewAuto->blockSignals(false);

}

void PrinterSearchWindow::driverManSearchedSlot()
{
    DriverSearcher *pDriverSearcher = static_cast<DriverSearcher *>(sender());
    if (pDriverSearcher) {
        m_pManDriverCom->clear();
        QList<QMap<QString, QVariant>> drivers = pDriverSearcher->getDrivers();
        pDriverSearcher->deleteLater();
        foreach (auto driver, drivers) {
            //将驱动结构体存在item中，方便后续安装打印机
            QString strDesc;
            if (driver.value("ppd-make-and-model").isNull())
                strDesc = driver.value("ppd-make").toString() + " " + driver.value("ppd-model").toString();
            else
                strDesc = driver.value("ppd-make-and-model").toString();

            bool isExcat = true;
            QString strReplace;
            //本地都是精确匹配，服务器获取可能不是精确匹配
            if (driver.value(SD_KEY_from).toInt() == PPDFrom_Server) {
                isExcat = driver.value("excat").toBool();
            }
            if (isExcat) strReplace = tr(" (recommended)");
            //去掉自带的recommended字段
            strDesc.replace("(recommended)", "");
            //如果是精确匹配添加 推荐 字段
            strDesc += strReplace;
            m_pManDriverCom->addItem(strDesc, QVariant::fromValue(driver));
        }
        m_pManDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
    }
    m_pManSpinner->stop();
    m_pManSpinner->setVisible(false);
    m_pManInstallDriverBtn->setVisible(true);
    m_pManInstallDriverBtn->setEnabled(true);
    if (m_pManDriverCom->count() >= 2) {
        m_pManInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_AUTO);
        m_isManInstallDriver = true;
    } else {
        m_pManInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_NEXT);
        m_isManInstallDriver = false;
    }
    //解除信号阻塞，重新响应列表的点击事件

    m_pPrinterListViewManual->blockSignals(false);

}


void PrinterSearchWindow::refreshPrinterListSlot()
{
    m_pBtnRefresh->blockSignals(true);
    m_pBtnRefresh->setVisible(true);

    m_pAutoInstallDriverBtn->setVisible(false);
    m_pAutoInstallDriverBtn->setEnabled(false);
    //屏蔽点击信号，避免未刷新完打印机就开始刷新驱动，目前只有一个loading动画
    m_pPrinterListViewAuto->blockSignals(true);
    ReflushDevicesByBackendTask *task = new ReflushDevicesByBackendTask();
    //lambda表达式使用directConnection,这里需要在UI线程执行槽函数
    connect(task, &ReflushDevicesByBackendTask::signalStatus, this, &PrinterSearchWindow::getDeviceResultSlot);
    task->start();

    m_pAutoInstallDriverBtn->setVisible(false);
    m_pAutoSpinner->setVisible(true);
    m_pAutoSpinner->start();
    m_pPrinterListModel->clear();
    m_pAutoDriverCom->clear();
    m_pAutoDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
}

void PrinterSearchWindow::searchPrintersByManual()
{
    QString location = m_pLineEditLocation->text();
    if (location.isEmpty())
        return;
    // 阻塞信号防止查询完成之前重复查询打印机
    m_pBtnFind->blockSignals(true);
    m_pLineEditLocation->blockSignals(true);
    m_pPrinterListViewManual->blockSignals(true);
    ReflushDevicesByHostTask *task = new ReflushDevicesByHostTask(location);
    connect(task, &ReflushDevicesByHostTask::signalStatus, this, &PrinterSearchWindow::getDeviceResultByManualSlot);
    // 当使用ip查找打印机的时候，smb协议流程会出现用户名和密码的输入提示框
    connect(task, &ReflushDevicesByHostTask::signalSmbPassWord, this, &PrinterSearchWindow::smbInfomationSlot, Qt::BlockingQueuedConnection);
    task->start();

    m_pManInstallDriverBtn->setVisible(false);
    m_pManSpinner->setVisible(true);
    m_pManSpinner->start();
    m_pPrinterListModelManual->clear();
    m_pManDriverCom->clear();
    m_pManDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
}

void PrinterSearchWindow::lineEditURIChanged(QString uri)
{
    QRegExp reg("(\\S+)(://)(\\S+)");
    QRegExpValidator v(reg);
    int pos = 0;
    QValidator::State state = v.validate(uri, pos);
    if (state == QValidator::Acceptable) {
        QMap<QString, QVariant> driver = g_driverManager->getEveryWhereDriver(uri);
        if (m_pURIDriverCom->count() < 2 && !driver.isEmpty()) {
            m_pURIDriverCom->clear();
            m_pURIDriverCom->addItem(driver[CUPS_PPD_MAKE_MODEL].toString(), driver);
            m_pURIDriverCom->addItem(UI_PRINTERSEARCH_MANUAL);
            m_isAutoInstallDriver = true;
        }
        m_pURIInstallDriverBtn->setEnabled(true);
    } else {
        m_pURIInstallDriverBtn->setEnabled(false);
    }
}

void PrinterSearchWindow::installDriverSlot()
{
    TDeviceInfo device;
    if (m_pTabListView->currentIndex().row() == 0) {  // 自动
        // 当前选中的row和checkeded的row已经不一致，按照checked的为准
        QStandardItem *pCheckedItem = getCheckedItem(0);
        if (!pCheckedItem) {
            return;
        }
        QVariant temp = pCheckedItem->data(Qt::UserRole + 1);
        device = temp.value<TDeviceInfo>();
        if (m_isAutoInstallDriver) {
            autoInstallPrinter(0, device);
        } else {
            close();
            m_pInstallDriverWindow->show();
            m_pInstallDriverWindow->setDeviceInfo(device);
        }
    } else if (m_pTabListView->currentIndex().row() == 1) { //ip
        QStandardItem *pCheckedItem = getCheckedItem(1);
        if (!pCheckedItem) {
            return;
        }
        QVariant temp = pCheckedItem->data(Qt::UserRole + 1);
        device = temp.value<TDeviceInfo>();
        if (m_isManInstallDriver) {
            autoInstallPrinter(1, device);
        } else {
            close();
            m_pInstallDriverWindow->show();
            m_pInstallDriverWindow->setDeviceInfo(device);
        }
    } else if (m_pTabListView->currentIndex().row() == 2) { //URI 手动构造设备结构体
        device.uriList.append(m_pLineEditURI->text());
        device.iType = InfoFrom_Manual;
        if (m_isURIInstallDriver) {
            autoInstallPrinter(2, device);
        } else {
            close();
            m_pInstallDriverWindow->show();
            m_pInstallDriverWindow->setDeviceInfo(device);
        }
    }
}

void PrinterSearchWindow::driverChangedSlot(int index)
{
    if (m_pTabListView->currentIndex().row() == 0) {
        if (index == m_pAutoDriverCom->count() - 1) {
            // 手动选择驱动
            m_pAutoInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_NEXT);
            m_isAutoInstallDriver = false;
        } else {
            // 自动安装驱动
            m_pAutoInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_AUTO);
            m_isAutoInstallDriver = true;
        }
    } else if (m_pTabListView->currentIndex().row() == 1) {
        if (index == m_pManDriverCom->count() - 1) {
            // 手动选择驱动
            m_pManInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_NEXT);
            m_isManInstallDriver = false;
        } else {
            // 自动安装驱动
            m_pManInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_AUTO);
            m_isManInstallDriver = true;
        }
    } else if (m_pTabListView->currentIndex().row() == 2) {
        if (index == m_pURIDriverCom->count() - 1) {
            // 手动选择驱动
            m_pURIInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_NEXT);
            m_isURIInstallDriver = false;
        } else {
            // 自动安装驱动
            m_pURIInstallDriverBtn->setText(UI_PRINTERSEARCH_INSTALLDRIVER_AUTO);
            m_isURIInstallDriver = true;
        }
    }

}

void PrinterSearchWindow::smbInfomationSlot(int &ret, const QString &host, QString &group, QString &user, QString &password)
{
    PermissionsWindow *pPermissionsWindow = new PermissionsWindow();
    pPermissionsWindow->setHost(host);
//    host = m_pLineEditLocation->text();
    int result = pPermissionsWindow->exec();
    if (result) {
        ++ret;
        user = pPermissionsWindow->getUser();
        group = pPermissionsWindow->getGroup();
        password = pPermissionsWindow->getPassword();
    }
    delete pPermissionsWindow;
}


