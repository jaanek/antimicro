#include <QDebug>
#include <QFile>
#include <QApplication>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "joyaxiswidget.h"
#include "joybuttonwidget.h"
#include "joycontrolstickpushbutton.h"
#include "joytabwidget.h"
#include "joydpadbuttonwidget.h"
#include "virtualdpadpushbutton.h"
#include "joycontrolstickbuttonpushbutton.h"
#include "dpadpushbutton.h"
#include "common.h"

MainWindow::MainWindow(QHash<int, Joystick*> *joysticks, CommandLineUtility *cmdutility, bool graphical, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    delete ui->tab_2;
    ui->tab_2 = 0;

    delete ui->tab;
    ui->tab = 0;

    this->graphical = graphical;
    signalDisconnect = false;
    showTrayIcon = !cmdutility->isTrayHidden() && graphical;

    this->joysticks = joysticks;

    if (showTrayIcon)
    {
        trayIconMenu = new QMenu(this);
        trayIcon = new QSystemTrayIcon(this);
        connect(trayIconMenu, SIGNAL(aboutToShow()), this, SLOT(refreshTrayIconMenu()));
        connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClickAction(QSystemTrayIcon::ActivationReason)), Qt::DirectConnection);
    }

    fillButtons(joysticks);
    if (cmdutility->hasProfile())
    {
        if (cmdutility->hasControllerNumber())
        {
            loadConfigFile(cmdutility->getProfileLocation(), cmdutility->getControllerNumber());
        }
        else
        {
            loadConfigFile(cmdutility->getProfileLocation());
        }
    }

    aboutDialog = new AboutDialog(this);

    QApplication *app = static_cast<QApplication*> (QCoreApplication::instance());
    connect(ui->menuOptions, SIGNAL(aboutToShow()), this, SLOT(mainMenuChange()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), app, SLOT(aboutQt()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fillButtons(Joystick *joystick)
{
    int joyindex = joystick->getJoyNumber();
    JoyTabWidget *tabwidget = (JoyTabWidget*)ui->tabWidget->widget(joyindex);
    tabwidget->fillButtons();
}

void MainWindow::fillButtons(QHash<int, Joystick *> *joysticks)
{
    ui->stackedWidget->setCurrentIndex(0);
    removeJoyTabs();

    for (int i=0; i < joysticks->count(); i++)
    {
        Joystick *joystick = joysticks->value(i);

        JoyTabWidget *tabwidget = new JoyTabWidget(joystick, this);
        tabwidget->fillButtons();
        ui->tabWidget->addTab(tabwidget, QString(tr("Joystick %1")).arg(joystick->getRealJoyNumber()));
        //connect(tabwidget, SIGNAL(joystickRefreshRequested(Joystick*)), this, SLOT(joystickRefreshPropogate(Joystick*)));
        if (showTrayIcon)
        {
            connect(tabwidget, SIGNAL(joystickConfigChanged(int)), this, SLOT(populateTrayIcon()));
        }
    }

    if (joysticks->count() > 0)
    {
        loadAppConfig();

        ui->tabWidget->setCurrentIndex(0);
        ui->stackedWidget->setCurrentIndex(1);
    }

    if (showTrayIcon)
    {
        populateTrayIcon();
        trayIcon->show();
    }

    ui->actionUpdate_Joysticks->setEnabled(true);
    ui->actionHide->setEnabled(true);
    ui->actionQuit->setEnabled(true);
}

void MainWindow::joystickRefreshPropogate(Joystick *joystick)
{
    emit joystickRefreshRequested(joystick);
}

// Intermediate slot to be used in Form Designer
void MainWindow::startJoystickRefresh()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->actionUpdate_Joysticks->setEnabled(false);
    ui->actionHide->setEnabled(false);
    ui->actionQuit->setEnabled(false);
    removeJoyTabs();

    emit joystickRefreshRequested();
}

void MainWindow::populateTrayIcon()
{
    trayIconMenu->clear();

    if (joysticks->count() > 0)
    {
        for (int i=0; i < joysticks->count(); i++)
        {
            QMenu *joysticksub = trayIconMenu->addMenu(joysticks->value(i)->getName());
            JoyTabWidget *widget = (JoyTabWidget*)ui->tabWidget->widget(i);
            QHash<int, QString> *configs = widget->recentConfigs();
            QHashIterator<int, QString> iter(*configs);
            while (iter.hasNext())
            {
                iter.next();
                QAction *newaction = new QAction(iter.value(), joysticksub);
                newaction->setCheckable(true);
                newaction->setChecked(false);

                if (iter.key() == widget->getCurrentConfigIndex())
                {
                    newaction->setChecked(true);
                }
                QHash<QString, QVariant> *tempmap = new QHash<QString, QVariant> ();
                tempmap->insert(QString::number(i), QVariant (iter.key()));
                QVariant tempvar (*tempmap);
                newaction->setData(tempvar);
                joysticksub->addAction(newaction);
            }

            QAction *newaction = new QAction(tr("Open File"), joysticksub);
            newaction->setIcon(QIcon::fromTheme("document-open"));
            connect(newaction, SIGNAL(triggered()), widget, SLOT(openConfigFileDialog()));
            joysticksub->addAction(newaction);

            connect(joysticksub, SIGNAL(triggered(QAction*)), this, SLOT(trayMenuChangeJoyConfig(QAction*)));
            connect(joysticksub, SIGNAL(aboutToShow()), this, SLOT(joystickTrayShow()));
        }

        trayIconMenu->addSeparator();
    }

    hideAction = new QAction(tr("&Hide"), trayIconMenu);
    hideAction->setIcon(QIcon::fromTheme("view-restore"));
    connect(hideAction, SIGNAL(triggered()), this, SLOT(hide()));
    //connect(hideAction, SIGNAL(triggered()), this, SLOT(disableFlashActions()));

    restoreAction = new QAction(tr("&Restore"), trayIconMenu);
    restoreAction->setIcon(QIcon::fromTheme("view-fullscreen"));
    //connect(restoreAction, SIGNAL(triggered()), this, SLOT(enableFlashActions()));
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(show()));

    closeAction = new QAction(tr("&Quit"), trayIconMenu);
    closeAction->setIcon(QIcon::fromTheme("application-exit"));
    connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));

    updateJoy = new QAction(tr("&Update Joysticks"), trayIconMenu);
    updateJoy->setIcon(QIcon::fromTheme("view-refresh"));
    connect(updateJoy, SIGNAL(triggered()), this, SLOT(startJoystickRefresh()));

    trayIconMenu->addAction(hideAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(updateJoy);
    trayIconMenu->addAction(closeAction);

    QIcon icon = QIcon(":/images/antimicro_trayicon.png");
    //trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(icon);
    trayIcon->setContextMenu(trayIconMenu);
}

void MainWindow::quitProgram()
{
    this->close();
    QApplication *app = static_cast<QApplication*> (QCoreApplication::instance());
    app->quit();
}

void MainWindow::refreshTrayIconMenu()
{
    if (this->isHidden())
    {
        hideAction->setEnabled(false);
        restoreAction->setEnabled(true);
    }
    else
    {
        hideAction->setEnabled(true);
        restoreAction->setEnabled(false);
    }
}

void MainWindow::trayIconClickAction(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        if (this->isHidden())
        {
            this->show();
        }
        else
        {
            this->hide();
        }
    }

}

void MainWindow::mainMenuChange()
{
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        ui->actionHide->setEnabled(true);
    }
    else
    {
        ui->actionHide->setEnabled(false);
    }
}

void MainWindow::saveAppConfig()
{
    if (joysticks->count() > 0)
    {
        QSettings settings(PadderCommon::configFilePath, QSettings::IniFormat);
        settings.clear();
        settings.beginGroup("Controllers");

        for (int i=0; i < ui->tabWidget->count(); i++)
        {
            JoyTabWidget *tabwidget = (JoyTabWidget*)ui->tabWidget->widget(i);
            tabwidget->saveSettings(&settings);
        }

        settings.endGroup();
    }
}

void MainWindow::loadAppConfig(bool forceRefresh)
{
    QSettings settings(PadderCommon::configFilePath, QSettings::IniFormat);
    for (int i=0; i < ui->tabWidget->count(); i++)
    {
        JoyTabWidget *tabwidget = (JoyTabWidget*)ui->tabWidget->widget(i);
        tabwidget->loadSettings(&settings, forceRefresh);
    }
}

void MainWindow::disableFlashActions()
{
    for (int i=0; i < ui->tabWidget->count(); i++)
    {
        QList<JoyButtonWidget*> list = ui->tabWidget->widget(i)->findChildren<JoyButtonWidget*>();
        QListIterator<JoyButtonWidget*> iter(list);
        while (iter.hasNext())
        {
            JoyButtonWidget *buttonWidget = iter.next();
            buttonWidget->disableFlashes();
        }

        QList<JoyAxisWidget*> list2 = ui->tabWidget->widget(i)->findChildren<JoyAxisWidget*>();
        QListIterator<JoyAxisWidget*> iter2(list2);
        while (iter2.hasNext())
        {
            JoyAxisWidget *axisWidget = iter2.next();
            axisWidget->disableFlashes();
        }

        QList<JoyControlStickPushButton*> list3 = ui->tabWidget->widget(i)->findChildren<JoyControlStickPushButton*>();
        QListIterator<JoyControlStickPushButton*> iter3(list3);
        while (iter3.hasNext())
        {
            JoyControlStickPushButton *stickWidget = iter3.next();
            stickWidget->disableFlashes();
        }

        QList<JoyDPadButtonWidget*> list4 = ui->tabWidget->widget(i)->findChildren<JoyDPadButtonWidget*>();
        QListIterator<JoyDPadButtonWidget*> iter4(list4);
        while (iter4.hasNext())
        {
            JoyDPadButtonWidget *dpadWidget = iter4.next();
            dpadWidget->disableFlashes();
        }

        QList<VirtualDPadPushButton*> list5 = ui->tabWidget->widget(i)->findChildren<VirtualDPadPushButton*>();
        QListIterator<VirtualDPadPushButton*> iter5(list5);
        while (iter5.hasNext())
        {
            VirtualDPadPushButton *dpadWidget = iter5.next();
            dpadWidget->disableFlashes();
        }

        QList<JoyControlStickButtonPushButton*> list6 = ui->tabWidget->widget(i)->findChildren<JoyControlStickButtonPushButton*>();
        QListIterator<JoyControlStickButtonPushButton*> iter6(list6);
        while (iter6.hasNext())
        {
            JoyControlStickButtonPushButton *stickButtonWidget = iter6.next();
            stickButtonWidget->disableFlashes();
        }

        QList<DPadPushButton*> list7 = ui->tabWidget->widget(i)->findChildren<DPadPushButton*>();
        QListIterator<DPadPushButton*> iter7(list7);
        while (iter7.hasNext())
        {
            DPadPushButton *dpadWidget = iter7.next();
            dpadWidget->disableFlashes();
        }
    }
}

void MainWindow::enableFlashActions()
{
    for (int i=0; i < ui->tabWidget->count(); i++)
    {
        QList<JoyButtonWidget*> list = ui->tabWidget->widget(i)->findChildren<JoyButtonWidget*>();
        QListIterator<JoyButtonWidget*> iter(list);
        while (iter.hasNext())
        {
            JoyButtonWidget *buttonWidget = iter.next();
            buttonWidget->enableFlashes();
        }

        QList<JoyAxisWidget*> list2 = ui->tabWidget->widget(i)->findChildren<JoyAxisWidget*>();
        QListIterator<JoyAxisWidget*> iter2(list2);
        while (iter2.hasNext())
        {
            JoyAxisWidget *axisWidget = iter2.next();
            axisWidget->enableFlashes();
        }

        QList<JoyControlStickPushButton*> list3 = ui->tabWidget->widget(i)->findChildren<JoyControlStickPushButton*>();
        QListIterator<JoyControlStickPushButton*> iter3(list3);
        while (iter3.hasNext())
        {
            JoyControlStickPushButton *stickWidget = iter3.next();
            stickWidget->enableFlashes();
        }

        QList<JoyDPadButtonWidget*> list4 = ui->tabWidget->widget(i)->findChildren<JoyDPadButtonWidget*>();
        QListIterator<JoyDPadButtonWidget*> iter4(list4);
        while (iter4.hasNext())
        {
            JoyDPadButtonWidget *dpadWidget = iter4.next();
            dpadWidget->enableFlashes();
        }

        QList<VirtualDPadPushButton*> list5 = ui->tabWidget->widget(i)->findChildren<VirtualDPadPushButton*>();
        QListIterator<VirtualDPadPushButton*> iter5(list5);
        while (iter5.hasNext())
        {
            VirtualDPadPushButton *dpadWidget = iter5.next();
            dpadWidget->enableFlashes();
        }

        QList<JoyControlStickButtonPushButton*> list6 = ui->tabWidget->widget(i)->findChildren<JoyControlStickButtonPushButton*>();
        QListIterator<JoyControlStickButtonPushButton*> iter6(list6);
        while (iter6.hasNext())
        {
            JoyControlStickButtonPushButton *stickButtonWidget = iter6.next();
            stickButtonWidget->enableFlashes();
        }

        QList<DPadPushButton*> list7 = ui->tabWidget->widget(i)->findChildren<DPadPushButton*>();
        QListIterator<DPadPushButton*> iter7(list7);
        while (iter7.hasNext())
        {
            DPadPushButton *dpadWidget = iter7.next();
            dpadWidget->enableFlashes();
        }
    }
}

// Intermediate slot used in Design mode
void MainWindow::hideWindow()
{
    hide();
}

void MainWindow::trayMenuChangeJoyConfig(QAction *action)
{
    // Obtaining the selected config
    QHash<QString, QVariant> tempmap = action->data().toHash();
    QHashIterator<QString, QVariant> iter(tempmap);
    while (iter.hasNext())
    {
        iter.next();

        // Fetching indicies and tab associated with the current joypad
        int joyindex = iter.key().toInt();
        int configindex = iter.value().toInt();
        JoyTabWidget *widget = (JoyTabWidget*)ui->tabWidget->widget(joyindex);

        // Checking if the selected config has been disabled by the change (action->isChecked() represents the state of the checkbox AFTER the click)
        if (!action->isChecked())
        {
            // It has - disabling - the 0th config is the new/'null' config
            widget->setCurrentConfig(0);
        }
        else
        {
            // It hasn't - enabling - note that setting this causes the menu to be updated
            widget->setCurrentConfig(configindex);
        }
    }
}

void MainWindow::joystickTrayShow()
{
    QMenu *tempmenu = (QMenu*) sender();
    QList<QAction*> menuactions = tempmenu->actions();
    QListIterator<QAction*> listiter (menuactions);
    while (listiter.hasNext())
    {
        QAction *action = listiter.next();
        action->setChecked(false);

        QHash<QString, QVariant> tempmap = action->data().toHash();
        QHashIterator<QString, QVariant> iter(tempmap);
        while (iter.hasNext())
        {
            iter.next();
            int joyindex = iter.key().toInt();
            int configindex = iter.value().toInt();
            JoyTabWidget *widget = (JoyTabWidget*)ui->tabWidget->widget(joyindex);

            if (configindex == widget->getCurrentConfigIndex())
            {
                action->setChecked(true);
            }
        }
    }
}

void MainWindow::hideEvent(QHideEvent *event)
{
    // Perform if window is minimized via the window manager
    if (event->spontaneous())
    {
        // Check if a system tray exists and hide window if one is available
        if (QSystemTrayIcon::isSystemTrayAvailable() && showTrayIcon)
        {
            hide();
            disableFlashActions();
            signalDisconnect = true;
        }
        // No system tray found. Disconnect processing of flashing buttons
        else
        {
            disableFlashActions();
            signalDisconnect = true;
        }
    }
    else
    {
        // Code is invoked by calling the hide method
        disableFlashActions();
        signalDisconnect = true;
    }

    QMainWindow::hideEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    // Check if hideEvent has been processed
    if (signalDisconnect)
    {
        // Restore flashing buttons
        enableFlashActions();
        signalDisconnect = false;
        // Only needed if hidden with the system tray enabled
        if (QSystemTrayIcon::isSystemTrayAvailable() && showTrayIcon)
        {
            showNormal();
        }
    }

    QMainWindow::showEvent(event);
}

void MainWindow::openAboutDialog()
{
    aboutDialog->show();
}

void MainWindow::loadConfigFile(QString fileLocation, int joystickIndex)
{
    if (joystickIndex > 0 && joysticks->contains(joystickIndex-1))
    {
        JoyTabWidget *widget = static_cast<JoyTabWidget*> (ui->tabWidget->widget(joystickIndex-1));
        if (widget)
        {
            widget->loadConfigFile(fileLocation);
        }
    }
    else if (joystickIndex <= 0)
    {
        for (int i=0; i < ui->tabWidget->count(); i++)
        {
            JoyTabWidget *widget = static_cast<JoyTabWidget*> (ui->tabWidget->widget(i));
            if (widget)
            {
                widget->loadConfigFile(fileLocation);
            }
        }
    }
}

void MainWindow::removeJoyTabs()
{
    int oldtabcount = ui->tabWidget->count();

    for (int i = oldtabcount-1; i >= 0; i--)
    {
        QWidget *tab = ui->tabWidget->widget(i);
        delete tab;
        tab = 0;
    }

    ui->tabWidget->clear();
}
