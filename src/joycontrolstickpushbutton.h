#ifndef JOYCONTROLSTICKPUSHBUTTON_H
#define JOYCONTROLSTICKPUSHBUTTON_H

#include <QPushButton>
#include <QPaintEvent>

#include "joycontrolstick.h"

class JoyControlStickPushButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(bool isflashing READ isButtonFlashing)

public:
    explicit JoyControlStickPushButton(JoyControlStick *stick, QWidget *parent = 0);
    JoyControlStick* getStick();
    bool isButtonFlashing();
    
protected:
    virtual void paintEvent(QPaintEvent *event);
    QString generateLabel();

    JoyControlStick *stick;
    bool isflashing;

signals:
    void flashed(bool flashing);

public slots:
    void refreshLabel();
    void disableFlashes();
    void enableFlashes();

private slots:
    void flash();
    void unflash();
};

#endif // JOYCONTROLSTICKPUSHBUTTON_H
