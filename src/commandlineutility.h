#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include <QObject>
#include <QStringList>
#include <QRegExp>

class CommandLineUtility : public QObject
{
    Q_OBJECT
public:
    explicit CommandLineUtility(QObject *parent = 0);

    void parseArguments(QStringList& arguments);
    bool isLaunchInTrayEnabled();
    bool isHelpRequested();
    bool isVersionRequested();
    bool isTrayHidden();
    bool hasProfile();
    bool hasControllerNumber();
    QString getProfileLocation();
    unsigned int getControllerNumber();
    bool isHiddenRequested();

    void printHelp();
    void printVersionString();

    bool hasError();

protected:
    bool launchInTray;
    bool helpRequest;
    bool versionRequest;
    bool hideTrayIcon;
    QString profileLocation;
    unsigned int controllerNumber;
    bool encounteredError;
    bool hiddenRequest;

    static QRegExp trayRegexp;
    static QRegExp helpRegexp;
    static QRegExp versionRegexp;
    static QRegExp noTrayRegexp;
    static QRegExp loadProfileRegexp;
    static QRegExp loadProfileForControllerRegexp;
    static QRegExp hiddenRegexp;
    
signals:
    
public slots:
    
};

#endif // COMMANDLINEPARSER_H
