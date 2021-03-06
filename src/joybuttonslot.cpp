#include "joybuttonslot.h"
#include "event.h"

const int JoyButtonSlot::JOYSPEED = 20;
const QString JoyButtonSlot::xmlName = "slot";


JoyButtonSlot::JoyButtonSlot(QObject *parent) :
    QObject(parent)
{
    deviceCode = 0;
    mode = JoyKeyboard;
    distance = 0.0;
    mouseInterval = new QTime();
}

JoyButtonSlot::JoyButtonSlot(int code, JoySlotInputAction mode, QObject *parent) :
    QObject(parent)
{
    deviceCode = 0;

    if (code > 0)
    {
        deviceCode = code;
    }

    this->mode = mode;
    distance = 0.0;
    mouseInterval = new QTime();
}

JoyButtonSlot::~JoyButtonSlot()
{
    delete mouseInterval;
}

void JoyButtonSlot::setSlotCode(int code)
{
    deviceCode = code;
}

int JoyButtonSlot::getSlotCode()
{
    return deviceCode;
}

void JoyButtonSlot::setSlotMode(JoySlotInputAction selectedMode)
{
    mode = selectedMode;
}

JoyButtonSlot::JoySlotInputAction JoyButtonSlot::getSlotMode()
{
    return mode;
}

QString JoyButtonSlot::movementString()
{
    QString newlabel;

    if (mode == JoyMouseMovement)
    {
        newlabel.append(tr("Mouse")).append(" ");
        if (deviceCode == JoyButtonSlot::MouseUp)
        {
            newlabel.append(tr("Up"));
        }
        else if (deviceCode == JoyButtonSlot::MouseDown)
        {
            newlabel.append(tr("Down"));
        }
        else if (deviceCode == JoyButtonSlot::MouseLeft)
        {
            newlabel.append(tr("Left"));
        }
        else if (deviceCode == JoyButtonSlot::MouseRight)
        {
            newlabel.append(tr("Right"));
        }
    }

    return newlabel;
}

void JoyButtonSlot::setDistance(double distance)
{
    this->distance = distance;
}

double JoyButtonSlot::getMouseDistance()
{
    return distance;
}

QTime* JoyButtonSlot::getMouseInterval()
{
    return mouseInterval;
}

void JoyButtonSlot::restartMouseInterval()
{
    mouseInterval->restart();
}

void JoyButtonSlot::readConfig(QXmlStreamReader *xml)
{
    if (xml->isStartElement() && xml->name() == "slot")
    {
        xml->readNextStartElement();
        while (!xml->atEnd() && (!xml->isEndElement() && xml->name() != "slot"))
        {
            if (xml->name() == "code" && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                this->setSlotCode(tempchoice);
            }
            else if (xml->name() == "mode" && xml->isStartElement())
            {
                QString temptext = xml->readElementText();

                if (temptext == "keyboard")
                {
                    this->setSlotMode(JoyKeyboard);
                }
                else if (temptext == "mousebutton")
                {
                    this->setSlotMode(JoyMouseButton);
                }
                else if (temptext == "mousemovement")
                {
                    this->setSlotMode(JoyMouseMovement);
                }
                else if (temptext == "pause")
                {
                    this->setSlotMode(JoyPause);
                }
                else if (temptext == "hold")
                {
                    this->setSlotMode(JoyHold);
                }
                else if (temptext == "cycle")
                {
                    this->setSlotMode(JoyCycle);
                }
                else if (temptext == "distance")
                {
                    this->setSlotMode(JoyDistance);
                }
            }
            else
            {
                xml->skipCurrentElement();
            }

            xml->readNextStartElement();
        }
    }

}

void JoyButtonSlot::writeConfig(QXmlStreamWriter *xml)
{
    xml->writeStartElement(getXmlName());

    xml->writeTextElement("code", QString::number(deviceCode));
    xml->writeStartElement("mode");
    if (mode == JoyKeyboard)
    {
        xml->writeCharacters("keyboard");
    }
    else if (mode == JoyMouseButton)
    {
        xml->writeCharacters("mousebutton");
    }
    else if (mode == JoyMouseMovement)
    {
        xml->writeCharacters("mousemovement");
    }
    else if (mode == JoyPause)
    {
        xml->writeCharacters("pause");
    }
    else if (mode == JoyHold)
    {
        xml->writeCharacters("hold");
    }
    else if (mode == JoyCycle)
    {
        xml->writeCharacters("cycle");
    }
    else if (mode == JoyDistance)
    {
        xml->writeCharacters("distance");
    }

    xml->writeEndElement();

    xml->writeEndElement();
}

QString JoyButtonSlot::getXmlName()
{
    return this->xmlName;
}

QString JoyButtonSlot::getSlotString()
{
    QString newlabel;

    if (deviceCode > 0)
    {
        if (mode == JoyButtonSlot::JoyKeyboard)
        {
            newlabel = newlabel.append(keycodeToKey(deviceCode).toUpper());
        }
        else if (mode == JoyButtonSlot::JoyMouseButton)
        {
            newlabel.append(tr("Mouse")).append(" ");
            switch (deviceCode)
            {
                case 1:
                    newlabel.append(tr("LB"));
                    break;
                case 2:
                    newlabel.append(tr("MB"));
                    break;
                case 3:
                    newlabel.append(tr("RB"));
                    break;
                default:
                    newlabel.append(QString::number(deviceCode));
                    break;
            }
        }
        else if (mode == JoyButtonSlot::JoyMouseMovement)
        {
            newlabel.append(movementString());
        }
        else if (mode == JoyButtonSlot::JoyPause)
        {
            newlabel.append(tr("Pause")).append(" ").append(QString::number(deviceCode / 1000.0, 'g', 3));
        }
        else if (mode == JoyButtonSlot::JoyHold)
        {
            newlabel.append(tr("Hold")).append(" ").append(QString::number(deviceCode / 1000.0, 'g', 3));
        }
        else if (mode == JoyButtonSlot::JoyCycle)
        {
            newlabel.append(tr("Cycle"));
        }
        else if (mode == JoyButtonSlot::JoyDistance)
        {
            QString temp(tr("Distance"));
            temp.append(" ").append(QString::number(deviceCode).append("%"));
            newlabel.append(temp);
        }
        else if (mode == JoyButtonSlot::JoyRelease)
        {
            newlabel.append(tr("Release")).append(" ").append(QString::number(deviceCode / 1000.0, 'g', 3));
        }
    }
    else
    {
        newlabel = newlabel.append(tr("[NO KEY]"));
    }

    return newlabel;
}
