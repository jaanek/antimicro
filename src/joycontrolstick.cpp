#include <QDebug>
#include <QHashIterator>
#include <QStringList>
#include <math.h>

#include "joycontrolstick.h"

const double JoyControlStick::PI = acos(-1.0);

JoyControlStick::JoyControlStick(JoyAxis *axis1, JoyAxis *axis2, int index, int originset, QObject *parent) :
    QObject(parent)
{
    this->axisX = axis1;
    this->axisX->setControlStick(this);
    this->axisY = axis2;
    this->axisY->setControlStick(this);

    this->index = index;
    this->originset = originset;
    reset();

    populateButtons();
}

JoyControlStick::~JoyControlStick()
{
    axisX->removeControlStick();
    axisY->removeControlStick();

    deleteButtons();
}

void JoyControlStick::joyEvent(bool ignoresets)
{
    safezone = !inDeadZone();

    if (safezone && !isActive)
    {
        isActive = true;
        emit active(axisX->getCurrentRawValue(), axisY->getCurrentRawValue());
        createDeskEvent(ignoresets);
    }
    else if (!safezone && isActive)
    {
        isActive = false;
        currentDirection = StickCentered;
        emit released(axisX->getCurrentRawValue(), axisY->getCurrentRawValue());

        createDeskEvent(ignoresets);
    }
    else if (isActive)
    {
        createDeskEvent(ignoresets);
    }

    emit moved(axisX->getCurrentRawValue(), axisY->getCurrentRawValue());
}

bool JoyControlStick::inDeadZone()
{
    int axis1Value = axisX->getCurrentRawValue();
    int axis2Value = axisY->getCurrentRawValue();

    unsigned int squareDist = (unsigned int)(axis1Value*axis1Value) + (unsigned int)(axis2Value*axis2Value);

    return squareDist <= (unsigned int)(deadZone*deadZone);
}

void JoyControlStick::populateButtons()
{
    JoyControlStickButton *button = new JoyControlStickButton (this, StickUp, originset, this);
    buttons.insert(StickUp, button);

    button = new JoyControlStickButton (this, StickDown, originset, this);
    buttons.insert(StickDown, button);

    button = new JoyControlStickButton(this, StickLeft, originset, this);
    buttons.insert(StickLeft, button);

    button = new JoyControlStickButton(this, StickRight, originset, this);
    buttons.insert(StickRight, button);

    button = new JoyControlStickButton(this, StickLeftUp, originset, this);
    buttons.insert(StickLeftUp, button);

    button = new JoyControlStickButton(this, StickLeftDown, originset, this);
    buttons.insert(StickLeftDown, button);

    button = new JoyControlStickButton(this, StickRightDown, originset, this);
    buttons.insert(StickRightDown, button);

    button = new JoyControlStickButton(this, StickRightUp, originset, this);
    buttons.insert(StickRightUp, button);
}

int JoyControlStick::getDeadZone()
{
    return deadZone;
}

int JoyControlStick::getDiagonalRange()
{
    return diagonalRange;
}

void JoyControlStick::createDeskEvent(bool ignoresets)
{
    JoyControlStickButton *eventbutton1 = 0;
    JoyControlStickButton *eventbutton2 = 0;
    JoyControlStickButton *eventbutton3 = 0;

    if (safezone)
    {
        double bearing = calculateBearing();

        QList<int> anglesList = getDiagonalZoneAngles();
        int initialLeft = anglesList.value(0);
        int initialRight = anglesList.value(1);
        int upRightInitial = anglesList.value(2);
        int rightInitial = anglesList.value(3);
        int downRightInitial = anglesList.value(4);
        int downInitial = anglesList.value(5);
        int downLeftInitial = anglesList.value(6);
        int leftInitial = anglesList.value(7);
        int upLeftInitial = anglesList.value(8);

        bearing = round(bearing);
        if (bearing <= initialRight || bearing >= initialLeft)
        {
            currentDirection = StickUp;
            eventbutton2 = buttons.value(StickUp);
        }
        else if (bearing >= upRightInitial && bearing < rightInitial)
        {
            currentDirection = StickRightUp;
            if (currentMode == EightWayMode && buttons.contains(StickRightUp))
            {
                eventbutton3 = buttons.value(StickRightUp);
            }
            else
            {
                eventbutton1 = buttons.value(StickRight);
                eventbutton2 = buttons.value(StickUp);
            }
        }
        else if (bearing >= rightInitial && bearing < downRightInitial)
        {
            currentDirection = StickRight;
            eventbutton1 = buttons.value(StickRight);
        }
        else if (bearing >= downRightInitial && bearing < downInitial)
        {
            currentDirection = StickRightDown;
            if (currentMode == EightWayMode && buttons.contains(StickRightDown))
            {
                eventbutton3 = buttons.value(StickRightDown);
            }
            else
            {
                eventbutton1 = buttons.value(StickRight);
                eventbutton2 = buttons.value(StickDown);
            }
        }
        else if (bearing >= downInitial && bearing < downLeftInitial)
        {
            currentDirection = StickDown;
            eventbutton2 = buttons.value(StickDown);
        }
        else if (bearing >= downLeftInitial && bearing < leftInitial)
        {
            currentDirection = StickLeftDown;
            if (currentMode == EightWayMode && buttons.contains(StickLeftDown))
            {
                eventbutton3 = buttons.value(StickLeftDown);
            }
            else
            {
                eventbutton1 = buttons.value(StickLeft);
                eventbutton2 = buttons.value(StickDown);
            }
        }
        else if (bearing >= leftInitial && bearing < upLeftInitial)
        {
            currentDirection = StickLeft;
            eventbutton1 = buttons.value(StickLeft);
        }
        else if (bearing >= upLeftInitial && bearing < initialLeft)
        {
            currentDirection = StickLeftUp;
            if (currentMode == EightWayMode && buttons.contains(StickLeftUp))
            {
                eventbutton3 = buttons.value(StickLeftUp);
            }
            else
            {
                eventbutton1 = buttons.value(StickLeft);
                eventbutton2 = buttons.value(StickUp);
            }
        }
    }

    if (eventbutton2 || activeButton2)
    {
        changeButtonEvent(eventbutton2, activeButton2, ignoresets);
    }

    if (eventbutton1 || activeButton1)
    {
        changeButtonEvent(eventbutton1, activeButton1, ignoresets);
    }

    if (eventbutton3 || activeButton3)
    {
        changeButtonEvent(eventbutton3, activeButton3, ignoresets);
    }
}

double JoyControlStick::calculateBearing()
{
    double finalAngle = 0.0;

    if (axisX->getCurrentRawValue() == 0 && axisY->getCurrentRawValue() == 0)
    {
        finalAngle = 0.0;
    }
    else
    {
        double temp1 = axisX->getCurrentRawValue() / (double)maxZone;
        double temp2 = axisY->getCurrentRawValue() / (double)maxZone;

        double angle = (atan2(temp1, -temp2) * 180) / PI;

        if (axisX->getCurrentRawValue() >= 0 && axisY->getCurrentRawValue() <= 0)
        {
            // NE Quadrant
            finalAngle = angle;
        }
        else if (axisX->getCurrentRawValue() >= 0 && axisY->getCurrentRawValue() >= 0)
        {
            // SE Quadrant (angle will be positive)
            finalAngle = angle;
        }
        else if (axisX->getCurrentRawValue() <= 0 && axisY->getCurrentRawValue() >= 0)
        {
            // SW Quadrant (angle will be negative)
            finalAngle = 360.0 + angle;
        }
        else if (axisX->getCurrentRawValue() <= 0 && axisY->getCurrentRawValue() <= 0)
        {
            // NW Quadrant (angle will be negative)
            finalAngle = 360.0 + angle;
        }
    }

    return finalAngle;
}

void JoyControlStick::changeButtonEvent(JoyControlStickButton *eventbutton, JoyControlStickButton *&activebutton, bool ignoresets)
{
    if (eventbutton && !activebutton)
    {
        // There is no active button. Call joyEvent and set current
        // button as active button
        eventbutton->joyEvent(true, ignoresets);
        activebutton = eventbutton;
    }
    else if (!eventbutton && activebutton)
    {
        // Currently in deadzone. Disable currently active button.
        activebutton->joyEvent(false, ignoresets);
        activebutton = 0;
    }
    else if (eventbutton && activebutton && eventbutton == activebutton)
    {
        //Button is currently active. Just pass current value
        eventbutton->joyEvent(true, ignoresets);
    }
    else if (eventbutton && activebutton && eventbutton != activebutton)
    {
        // Deadzone skipped. Button for new event is not the currently
        // active button. Disable the active button before enabling
        // the new button
        activebutton->joyEvent(false, ignoresets);
        eventbutton->joyEvent(true, ignoresets);
        activebutton = eventbutton;
    }
}

double JoyControlStick::getDistanceFromDeadZone()
{
    double distance = 0.0;

    int axis1Value = axisX->getCurrentRawValue();
    int axis2Value = axisY->getCurrentRawValue();

    unsigned int square_dist = (unsigned int)(axis1Value*axis1Value) + (unsigned int)(axis2Value*axis2Value);

    distance = (sqrt(square_dist) - deadZone)/(double)(maxZone - deadZone);
    if (distance > 1.0)
    {
        distance = 1.0;
    }
    else if (distance < 0.0)
    {
        distance = 0.0;
    }

    return distance;
}

double JoyControlStick::calculateXDistanceFromDeadZone()
{
    double distance = 0.0;

    int axis1Value = axisX->getCurrentRawValue();

    double relativeAngle = calculateBearing();
    if (relativeAngle > 180)
    {
        relativeAngle = relativeAngle - 180;
    }

    int deadX = (int)round(deadZone * sin(relativeAngle * PI / 180.0));
    distance = (abs(axis1Value) - deadX)/(double)(maxZone - deadX);
    if (distance > 1.0)
    {
        distance = 1.0;
    }
    else if (distance < 0.0)
    {
        distance = 0.0;
    }

    return distance;
}

double JoyControlStick::calculateYDistanceFromDeadZone()
{
    double distance = 0.0;

    int axis2Value = axisY->getCurrentRawValue();

    double relativeAngle = calculateBearing();
    if (relativeAngle > 180)
    {
        relativeAngle = relativeAngle - 180;
    }

    int deadY = abs(round(deadZone * cos(relativeAngle * PI / 180.0)));
    distance = (abs(axis2Value) - deadY)/(double)(maxZone - deadY);
    if (distance > 1.0)
    {
        distance = 1.0;
    }
    else if (distance < 0.0)
    {
        distance = 0.0;
    }

    return distance;
}

double JoyControlStick::getAbsoluteDistance()
{
    double distance = 0.0;

    int axis1Value = axisX->getCurrentRawValue();
    int axis2Value = axisY->getCurrentRawValue();

    unsigned int square_dist = (unsigned int)(axis1Value*axis1Value) + (unsigned int)(axis2Value*axis2Value);

    distance = sqrt(square_dist);
    if (distance > JoyAxis::AXISMAX)
    {
        distance = JoyAxis::AXISMAX;
    }
    else if (distance < 0.0)
    {
        distance = 0.0;
    }

    return distance;
}

double JoyControlStick::getNormalizedAbsoluteDistance()
{
    double distance = 0.0;

    int axis1Value = axisX->getCurrentRawValue();
    int axis2Value = axisY->getCurrentRawValue();

    unsigned int square_dist = (unsigned int)(axis1Value*axis1Value) + (unsigned int)(axis2Value*axis2Value);

    distance = sqrt(square_dist)/(double)(maxZone);
    if (distance > 1.0)
    {
        distance = 1.0;
    }
    else if (distance < 0.0)
    {
        distance = 0.0;
    }

    return distance;
}

void JoyControlStick::setIndex(int index)
{
    this->index = index;
}

int JoyControlStick::getIndex()
{
    return index;
}

int JoyControlStick::getRealJoyIndex()
{
    return index+1;
}

QString JoyControlStick::getName()
{
    QString label(tr("Stick"));
    label.append(" ").append(QString::number(getRealJoyIndex()));
    label.append(": ");
    QStringList tempList;
    if (buttons.contains(StickUp))
    {
        JoyControlStickButton *button = buttons.value(StickUp);
        tempList.append(button->getSlotsSummary());
    }

    if (buttons.contains(StickLeft))
    {
        JoyControlStickButton *button = buttons.value(StickLeft);
        tempList.append(button->getSlotsSummary());
    }

    if (buttons.contains(StickDown))
    {
        JoyControlStickButton *button = buttons.value(StickDown);
        tempList.append(button->getSlotsSummary());
    }

    if (buttons.contains(StickRight))
    {
        JoyControlStickButton *button = buttons.value(StickRight);
        tempList.append(button->getSlotsSummary());
    }
    /*QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        tempList.append(button->getSlotsSummary());
    }*/
    label.append(tempList.join(", "));
    return label;
}

int JoyControlStick::getMaxZone()
{
    return maxZone;
}

int JoyControlStick::getCurrentlyAssignedSet()
{
    return originset;
}

void JoyControlStick::reset()
{
    deadZone = 8000;
    maxZone = JoyAxis::AXISMAXZONE;
    diagonalRange = 45;
    isActive = false;

    /*if (activeButton1)
    {
        activeButton1->reset();
    }
    activeButton1 = 0;

    if (activeButton2)
    {
        activeButton2->reset();
    }

    activeButton2 = 0;*/

    activeButton1 = 0;
    activeButton2 = 0;
    activeButton3 = 0;
    safezone = false;
    currentDirection = StickCentered;
    currentMode = StandardMode;
    resetButtons();
}

void JoyControlStick::setDeadZone(int value)
{
    value = abs(value);
    if (value > JoyAxis::AXISMAX)
    {
        value = JoyAxis::AXISMAX;
    }

    if (value != deadZone && value < maxZone)
    {
        deadZone = value;
        emit deadZoneChanged(value);
    }
}

void JoyControlStick::setMaxZone(int value)
{
    value = abs(value);
    if (value >= JoyAxis::AXISMAX)
    {
        value = JoyAxis::AXISMAX;
    }

    if (value != maxZone && value > deadZone)
    {
        maxZone = value;
        emit maxZoneChanged(value);
    }
}

void JoyControlStick::setDiagonalRange(int value)
{
    if (value < 1)
    {
        value = 1;
    }
    else if (value > 89)
    {
        value = 89;
    }

    if (value != diagonalRange)
    {
        diagonalRange = value;
        emit diagonalRangeChanged(value);
    }
}

void JoyControlStick::refreshButtons()
{
    deleteButtons();
    populateButtons();
}

void JoyControlStick::deleteButtons()
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyButton *button = iter.next().value();
        if (button)
        {
            delete button;
            button = 0;
        }
    }

    buttons.clear();
}

void JoyControlStick::readConfig(QXmlStreamReader *xml)
{
    if (xml->isStartElement() && xml->name() == "stick")
    {
        xml->readNextStartElement();

        while (!xml->atEnd() && (!xml->isEndElement() && xml->name() != "stick"))
        {
            if (xml->name() == "deadZone" && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                this->setDeadZone(tempchoice);
            }
            else if (xml->name() == "maxZone" && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                this->setMaxZone(tempchoice);
            }
            else if (xml->name() == "diagonalRange" && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                int tempchoice = temptext.toInt();
                this->setDiagonalRange(tempchoice);
            }
            else if (xml->name() == "mode" && xml->isStartElement())
            {
                QString temptext = xml->readElementText();
                if (temptext == "eight-way")
                {
                    this->setJoyMode(EightWayMode);
                }
            }
            else if (xml->name() == JoyControlStickButton::xmlName && xml->isStartElement())
            {
                int index = xml->attributes().value("index").toString().toInt();
                JoyControlStickButton *button = buttons.value((JoyStickDirections)index);
                if (button)
                {
                    button->readConfig(xml);
                }
                else
                {
                    xml->skipCurrentElement();
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

void JoyControlStick::writeConfig(QXmlStreamWriter *xml)
{
    if (!isDefault())
    {
        xml->writeStartElement("stick");
        xml->writeAttribute("index", QString::number(index+1));
        xml->writeTextElement("deadZone", QString::number(deadZone));
        xml->writeTextElement("maxZone", QString::number(maxZone));
        xml->writeTextElement("diagonalRange", QString::number(diagonalRange));
        if (currentMode == EightWayMode)
        {
            xml->writeTextElement("mode", "eight-way");
        }

        QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
        while (iter.hasNext())
        {
            JoyControlStickButton *button = iter.next().value();
            button->writeConfig(xml);
        }

        xml->writeEndElement();
    }
}

void JoyControlStick::resetButtons()
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyButton *button = iter.next().value();
        if (button)
        {
            button->reset();
        }
    }
}

JoyControlStickButton* JoyControlStick::getDirectionButton(JoyStickDirections direction)
{
    JoyControlStickButton *button = buttons.value(direction);
    return button;
}

double JoyControlStick::calculateNormalizedAxis1Placement()
{
    return axisX->calculateNormalizedAxisPlacement();
}

double JoyControlStick::calculateNormalizedAxis2Placement()
{
    return axisY->calculateNormalizedAxisPlacement();
}

double JoyControlStick::calculateSquareAxisXDistanceFromDeadZone()
{
    double distance = 0.0;

    int axis1Value = axisX->getCurrentRawValue();
    int axis2Value = axisY->getCurrentRawValue();
    int absX = abs(axis1Value);
    int absY = abs(axis2Value);

    unsigned int axis1Square = axis1Value * axis1Value;
    unsigned int axis2Square = axis2Value * axis2Value;
    unsigned int square_dist = (unsigned int)axis1Square + (unsigned int)axis2Square;

    double relativeAngle = calculateBearing();
    if (relativeAngle > 180)
    {
        relativeAngle = relativeAngle - 180;
    }

    int deadX = (int)round(deadZone * sin(relativeAngle * PI / 180.0));
    int maxZoneX = (int)round(maxZone * sin(relativeAngle * PI / 180.0));
    double dirLen = qMin(sqrt(square_dist) * 1.25, JoyAxis::AXISMAX * 1.0);
    int scale = qMax(1, absX > absY ? absX : absY);
    //int squareX = (axis1Value * dirLen)/scale;
    /*int squareX = 0;
    if (abs(axis1Value) < abs(axis2Value))
    {
        squareX = (axis2Value < 0) ? -axis1Value/axis2Value : axis1Value/axis2Value;
    }
    else
    {
        squareX = (axis1Value < 0) ? -square_dist : square_dist;
    }*/
    //int deadY = abs(round(deadZone * cos(relativeAngle * PI / 180.0)));
    //int squareX = sqrt(square_dist/(double)qMax(axis1Square, axis2Square)) * abs(axis1Value);
    //double shitfuck = sqrt(square_dist/(double)qMax(axis1Square, axis2Square));

    //int squareY = sqrt(square_dist/qMax(axis1Square, axis2Square)) * axis2Value;
    double factor = JoyAxis::AXISMAX/(qMin(22000.0, (double)maxZone));
    //distance = ((abs(axis1Value) - deadX)/(double)(maxZone - deadX))*1.5;
    distance = ((abs(axis1Value) - deadX)/(double)(maxZone - deadX));
    if (distance > 1.0)
    {
        distance = 1.0;
    }
    else if (distance < 0.0)
    {
        distance = 0.0;
    }

    //distance *= 0.5;

    return distance;
}

double JoyControlStick::calculateSquareAxisYDistanceFromDeadZone()
{
    double distance = 0.0;

    int axis1Value = axisX->getCurrentRawValue();
    int axis2Value = axisY->getCurrentRawValue();
    int absX = abs(axis1Value);
    int absY = abs(axis2Value);

    unsigned int axis1Square = axis1Value * axis1Value;
    unsigned int axis2Square = axis2Value * axis2Value;
    unsigned int square_dist = (unsigned int)axis1Square + (unsigned int)axis2Square;

    double relativeAngle = calculateBearing();
    if (relativeAngle > 180)
    {
        relativeAngle = relativeAngle - 180;
    }

    int maxZoneX = abs(round(maxZone * sin(relativeAngle * PI / 180.0)));
    //int deadX = (int)round(deadZone * sin(relativeAngle * PI / 180.0));
    int deadY = abs(round(deadZone * cos((relativeAngle * PI) / 180.0)));
    double dudebro = cos((relativeAngle * PI) / 180.0);
    int maxZoneY = abs(round(maxZone * cos((relativeAngle * PI) / 180.0)));
    double dirLen = qMin(sqrt(square_dist) * 1.25, JoyAxis::AXISMAX * 1.0);
    int scale = qMax(1, absX > absY ? absX : absY);
    int squareY = (axis2Value * dirLen)/scale;
    //int squareY = 0;
    /*if (abs(axis1Value) < abs(axis2Value))
    {
        squareY = (axis2Value < 0) ? -square_dist : square_dist;
    }
    else
    {
        squareY = (axis1Value < 0) ? -axis2Value/axis1Value : axis2Value/axis1Value;
    }*/
    //int squareX = sqrt(square_dist/qMax(axis1Square, axis2Square)) * axis1Value;
    //int squareY = sqrt(square_dist/(double)qMax(axis1Square, axis2Square)) * abs(axis2Value);
    double factor = JoyAxis::AXISMAX/(qMin(22000.0, (double)maxZone));
    //distance = ((abs(axis2Value) - deadY)/(double)(maxZone - deadY))*1.5;
    distance = (abs(axis2Value) - deadY)/(double)(maxZone - deadY);
    /*if (abs(axis2Value) > maxZoneY)
    {
        qDebug() << "SQUEALING PIGS";
    }
    */

    if (distance > 1.0)
    {
        distance = 1.0;
    }
    else if (distance < 0.0)
    {
        distance = 0.0;
    }

    //distance *= 0.5;


    return distance;
}

double JoyControlStick::getSquareDistance()
{

}

double JoyControlStick::getNormalizedSquareDistance()
{

}

//double calculateSquareAxis1Distance();
//double calculateSquareAxis2Distance();
//double getSquareDistance();
//double getNormalizedSquareDistance();

double JoyControlStick::calculateDirectionalDistance(JoyControlStickButton *button, JoyButton::JoyMouseMovementMode mouseMode)
{
    double finalDistance = 0.0;

    if (currentDirection == StickUp)
    {
        if (mouseMode == JoyButton::MouseCursor)
        {
            finalDistance = calculateYDistanceFromDeadZone();
        }
        else if (mouseMode == JoyButton::MouseSpring)
        {
            finalDistance = calculateSquareAxisYDistanceFromDeadZone();
        }

    }
    else if (currentDirection == StickRightUp)
    {
        if (activeButton1 && activeButton1 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateXDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisXDistanceFromDeadZone();
            }
        }
        else if (activeButton2 && activeButton2 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateYDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisYDistanceFromDeadZone();
            }
        }
        else if (activeButton3 && activeButton3 == button)
        {
            double radius = getDistanceFromDeadZone();
            double bearing = calculateBearing();
            int relativeBearing = (int)round(bearing) % 90;
            //bearing = round(bearing) % 90;
            int diagonalAngle = relativeBearing;
            if (relativeBearing > 45)
            {
                diagonalAngle = 90 - relativeBearing;
            }

            finalDistance = radius * (diagonalAngle / 45.0);
        }
    }
    else if (currentDirection == StickRight)
    {
        if (mouseMode == JoyButton::MouseCursor)
        {
            finalDistance = calculateXDistanceFromDeadZone();
        }
        else if (mouseMode == JoyButton::MouseSpring)
        {
            finalDistance = calculateSquareAxisXDistanceFromDeadZone();
        }
    }
    else if (currentDirection  == StickRightDown)
    {
        if (activeButton1 && activeButton1 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateXDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisXDistanceFromDeadZone();
            }
        }
        else if (activeButton2 && activeButton2 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateYDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisYDistanceFromDeadZone();
            }
        }
        else if (activeButton3 && activeButton3 == button)
        {
            double radius = getDistanceFromDeadZone();
            double bearing = calculateBearing();
            int relativeBearing = (int)round(bearing) % 90;
            //bearing = round(bearing) % 90;
            int diagonalAngle = relativeBearing;
            if (relativeBearing > 45)
            {
                diagonalAngle = 90 - relativeBearing;
            }

            finalDistance = radius * (diagonalAngle / 45.0);
        }
    }
    else if (currentDirection == StickDown)
    {
        if (mouseMode == JoyButton::MouseCursor)
        {
            finalDistance = calculateYDistanceFromDeadZone();
        }
        else if (mouseMode == JoyButton::MouseSpring)
        {
            finalDistance = calculateSquareAxisYDistanceFromDeadZone();
        }
    }
    else if (currentDirection == StickLeftDown)
    {
        if (activeButton1 && activeButton1 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateXDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisXDistanceFromDeadZone();
            }
        }
        else if (activeButton2 && activeButton2 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateYDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisYDistanceFromDeadZone();
            }
        }
        else if (activeButton3 && activeButton3 == button)
        {
            double radius = getDistanceFromDeadZone();
            double bearing = calculateBearing();
            int relativeBearing = (int)round(bearing) % 90;
            //bearing = round(bearing) % 90;
            int diagonalAngle = relativeBearing;
            if (relativeBearing > 45)
            {
                diagonalAngle = 90 - relativeBearing;
            }

            finalDistance = radius * (diagonalAngle / 45.0);
        }
    }
    else if (currentDirection == StickLeft)
    {
        if (mouseMode == JoyButton::MouseCursor)
        {
            finalDistance = calculateXDistanceFromDeadZone();
        }
        else if (mouseMode == JoyButton::MouseSpring)
        {
            finalDistance = calculateSquareAxisXDistanceFromDeadZone();
        }
    }
    else if (currentDirection == StickLeftUp)
    {
        if (activeButton1 && activeButton1 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateXDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisXDistanceFromDeadZone();
            }
        }
        else if (activeButton2 && activeButton2 == button)
        {
            if (mouseMode == JoyButton::MouseCursor)
            {
                finalDistance = calculateYDistanceFromDeadZone();
            }
            else if (mouseMode == JoyButton::MouseSpring)
            {
                finalDistance = calculateSquareAxisYDistanceFromDeadZone();
            }
        }
        else if (activeButton3 && activeButton3 == button)
        {
            double radius = getDistanceFromDeadZone();
            double bearing = calculateBearing();
            int relativeBearing = (int)round(bearing) % 90;
            //bearing = round(bearing) % 90;
            int diagonalAngle = relativeBearing;
            if (relativeBearing > 45)
            {
                diagonalAngle = 90 - relativeBearing;
            }

            finalDistance = radius * (diagonalAngle / 45.0);
        }
    }

    return finalDistance;
}

JoyControlStick::JoyStickDirections JoyControlStick::getCurrentDirection()
{
    return currentDirection;
}

int JoyControlStick::getXCoordinate()
{
    return axisX->getCurrentRawValue();
}

int JoyControlStick::getYCoordinate()
{
    return axisY->getCurrentRawValue();
}

QList<int> JoyControlStick::getDiagonalZoneAngles()
{
    QList<int> anglesList;

    int diagonalAngle = diagonalRange;

    int cardinalAngle = (360 - (diagonalAngle * 4)) / 4;

    int initialLeft = 360 - (int)((cardinalAngle - 1) / 2);
    int initialRight = (int)((cardinalAngle - 1)/ 2);
    if ((cardinalAngle - 1) % 2 != 0)
    {
        initialLeft = 360 - (cardinalAngle / 2);
        initialRight = (cardinalAngle / 2) - 1;
    }

    int upRightInitial = initialRight + 1;
    int rightInitial = upRightInitial + diagonalAngle ;
    int downRightInitial = rightInitial + cardinalAngle;
    int downInitial = downRightInitial + diagonalAngle;
    int downLeftInitial = downInitial + cardinalAngle;
    int leftInitial = downLeftInitial + diagonalAngle;
    int upLeftInitial = leftInitial + cardinalAngle;

    anglesList.append(initialLeft);
    anglesList.append(initialRight);
    anglesList.append(upRightInitial);
    anglesList.append(rightInitial);
    anglesList.append(downRightInitial);
    anglesList.append(downInitial);
    anglesList.append(downLeftInitial);
    anglesList.append(leftInitial);
    anglesList.append(upLeftInitial);

    return anglesList;
}

QHash<JoyControlStick::JoyStickDirections, JoyControlStickButton*>* JoyControlStick::getButtons()
{
    return &buttons;
}

JoyAxis* JoyControlStick::getAxisX()
{
    return axisX;
}

JoyAxis* JoyControlStick::getAxisY()
{
    return axisY;
}

void JoyControlStick::replaceXAxis(JoyAxis *axis)
{
    axisX->removeControlStick();
    this->axisX = axis;
    this->axisX->setControlStick(this);
}

void JoyControlStick::replaceYAxis(JoyAxis *axis)
{
    axisY->removeControlStick();
    this->axisY = axis;
    this->axisY->setControlStick(this);
}

void JoyControlStick::setJoyMode(JoyMode mode)
{
    currentMode = mode;
}

JoyControlStick::JoyMode JoyControlStick::getJoyMode()
{
    return currentMode;
}

void JoyControlStick::releaseButtonEvents()
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        button->joyEvent(false, true);
    }
}

bool JoyControlStick::isDefault()
{
    bool value = true;
    value = value && (deadZone == 8000);
    value = value && (maxZone == JoyAxis::AXISMAXZONE);
    value = value && (diagonalRange == 45);
    value = value && (currentMode == StandardMode);
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        value = value && (button->isDefault());
    }
    return value;
}

void JoyControlStick::setButtonsMouseMode(JoyButton::JoyMouseMovementMode mode)
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        button->setMouseMode(mode);
    }
}

bool JoyControlStick::hasSameButtonsMouseMode()
{
    bool result = true;

    JoyButton::JoyMouseMovementMode initialMode = JoyButton::MouseCursor;
    QHash<JoyStickDirections, JoyControlStickButton*> temphash;
    temphash.insert(StickUp, buttons.value(StickUp));
    temphash.insert(StickDown, buttons.value(StickDown));
    temphash.insert(StickLeft, buttons.value(StickLeft));
    temphash.insert(StickRight, buttons.value(StickRight));
    if (currentMode == EightWayMode)
    {
        temphash.insert(StickLeftUp, buttons.value(StickLeftUp));
        temphash.insert(StickRightUp, buttons.value(StickRightUp));
        temphash.insert(StickRightDown, buttons.value(StickRightDown));
        temphash.insert(StickLeftDown, buttons.value(StickLeftDown));
    }

    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyControlStickButton *button = iter.next().value();
            initialMode = button->getMouseMode();
        }
        else
        {
            JoyControlStickButton *button = iter.next().value();
            JoyButton::JoyMouseMovementMode temp = button->getMouseMode();
            if (temp != initialMode)
            {
                result = false;
                iter.toBack();
            }
        }
    }

    return result;
}

JoyButton::JoyMouseMovementMode JoyControlStick::getButtonsPresetMouseMode()
{
    JoyButton::JoyMouseMovementMode resultMode = JoyButton::MouseCursor;

    QHash<JoyStickDirections, JoyControlStickButton*> temphash;
    temphash.insert(StickUp, buttons.value(StickUp));
    temphash.insert(StickDown, buttons.value(StickDown));
    temphash.insert(StickLeft, buttons.value(StickLeft));
    temphash.insert(StickRight, buttons.value(StickRight));
    if (currentMode == EightWayMode)
    {
        temphash.insert(StickLeftUp, buttons.value(StickLeftUp));
        temphash.insert(StickRightUp, buttons.value(StickRightUp));
        temphash.insert(StickRightDown, buttons.value(StickRightDown));
        temphash.insert(StickLeftDown, buttons.value(StickLeftDown));
    }

    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyControlStickButton *button = iter.next().value();
            resultMode = button->getMouseMode();
        }
        else
        {
            JoyControlStickButton *button = iter.next().value();
            JoyButton::JoyMouseMovementMode temp = button->getMouseMode();
            if (temp != resultMode)
            {
                resultMode = JoyButton::MouseCursor;
                iter.toBack();
            }
        }
    }

    return resultMode;
}

void JoyControlStick::setButtonsMouseCurve(JoyButton::JoyMouseCurve mouseCurve)
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        button->setMouseCurve(mouseCurve);
    }
}

bool JoyControlStick::hasSameButtonsMouseCurve()
{
    bool result = true;

    JoyButton::JoyMouseCurve initialCurve = JoyButton::LinearCurve;
    QHash<JoyStickDirections, JoyControlStickButton*> temphash;
    temphash.insert(StickUp, buttons.value(StickUp));
    temphash.insert(StickDown, buttons.value(StickDown));
    temphash.insert(StickLeft, buttons.value(StickLeft));
    temphash.insert(StickRight, buttons.value(StickRight));
    if (currentMode == EightWayMode)
    {
        temphash.insert(StickLeftUp, buttons.value(StickLeftUp));
        temphash.insert(StickRightUp, buttons.value(StickRightUp));
        temphash.insert(StickRightDown, buttons.value(StickRightDown));
        temphash.insert(StickLeftDown, buttons.value(StickLeftDown));
    }

    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyControlStickButton *button = iter.next().value();
            initialCurve = button->getMouseCurve();
        }
        else
        {
            JoyControlStickButton *button = iter.next().value();
            JoyButton::JoyMouseCurve temp = button->getMouseCurve();
            if (temp != initialCurve)
            {
                result = false;
                iter.toBack();
            }
        }
    }

    return result;
}

JoyButton::JoyMouseCurve JoyControlStick::getButtonsPresetMouseCurve()
{
    JoyButton::JoyMouseCurve resultCurve = JoyButton::LinearCurve;

    QHash<JoyStickDirections, JoyControlStickButton*> temphash;
    temphash.insert(StickUp, buttons.value(StickUp));
    temphash.insert(StickDown, buttons.value(StickDown));
    temphash.insert(StickLeft, buttons.value(StickLeft));
    temphash.insert(StickRight, buttons.value(StickRight));
    if (currentMode == EightWayMode)
    {
        temphash.insert(StickLeftUp, buttons.value(StickLeftUp));
        temphash.insert(StickRightUp, buttons.value(StickRightUp));
        temphash.insert(StickRightDown, buttons.value(StickRightDown));
        temphash.insert(StickLeftDown, buttons.value(StickLeftDown));
    }

    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyControlStickButton *button = iter.next().value();
            resultCurve = button->getMouseCurve();
        }
        else
        {
            JoyControlStickButton *button = iter.next().value();
            JoyButton::JoyMouseCurve temp = button->getMouseCurve();
            if (temp != resultCurve)
            {
                resultCurve = JoyButton::LinearCurve;
                iter.toBack();
            }
        }
    }

    return resultCurve;
}

void JoyControlStick::setButtonsSpringWidth(int value)
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        button->setSpringWidth(value);
    }
}

void JoyControlStick::setButtonsSpringHeight(int value)
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        button->setSpringHeight(value);
    }
}

int JoyControlStick::getButtonsPresetSpringWidth()
{
    int presetSpringWidth = 0;

    QHash<JoyStickDirections, JoyControlStickButton*> temphash;
    temphash.insert(StickUp, buttons.value(StickUp));
    temphash.insert(StickDown, buttons.value(StickDown));
    temphash.insert(StickLeft, buttons.value(StickLeft));
    temphash.insert(StickRight, buttons.value(StickRight));
    if (currentMode == EightWayMode)
    {
        temphash.insert(StickLeftUp, buttons.value(StickLeftUp));
        temphash.insert(StickRightUp, buttons.value(StickRightUp));
        temphash.insert(StickRightDown, buttons.value(StickRightDown));
        temphash.insert(StickLeftDown, buttons.value(StickLeftDown));
    }

    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyControlStickButton *button = iter.next().value();
            presetSpringWidth = button->getSpringWidth();
        }
        else
        {
            JoyControlStickButton *button = iter.next().value();
            int temp = button->getSpringWidth();
            if (temp != presetSpringWidth)
            {
                presetSpringWidth = 0;
                iter.toBack();
            }
        }
    }

    return presetSpringWidth;
}

int JoyControlStick::getButtonsPresetSpringHeight()
{
    int presetSpringHeight = 0;

    QHash<JoyStickDirections, JoyControlStickButton*> temphash;
    temphash.insert(StickUp, buttons.value(StickUp));
    temphash.insert(StickDown, buttons.value(StickDown));
    temphash.insert(StickLeft, buttons.value(StickLeft));
    temphash.insert(StickRight, buttons.value(StickRight));
    if (currentMode == EightWayMode)
    {
        temphash.insert(StickLeftUp, buttons.value(StickLeftUp));
        temphash.insert(StickRightUp, buttons.value(StickRightUp));
        temphash.insert(StickRightDown, buttons.value(StickRightDown));
        temphash.insert(StickLeftDown, buttons.value(StickLeftDown));
    }

    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyControlStickButton *button = iter.next().value();
            presetSpringHeight = button->getSpringHeight();
        }
        else
        {
            JoyControlStickButton *button = iter.next().value();
            int temp = button->getSpringHeight();
            if (temp != presetSpringHeight)
            {
                presetSpringHeight = 0;
                iter.toBack();
            }
        }
    }

    return presetSpringHeight;
}

void JoyControlStick::setButtonsSensitivity(double value)
{
    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyControlStickButton *button = iter.next().value();
        button->setSensitivity(value);
    }
}

double JoyControlStick::getButtonsPresetSensitivity()
{
    double presetSensitivity = 1.0;

    QHash<JoyStickDirections, JoyControlStickButton*> temphash;
    temphash.insert(StickUp, buttons.value(StickUp));
    temphash.insert(StickDown, buttons.value(StickDown));
    temphash.insert(StickLeft, buttons.value(StickLeft));
    temphash.insert(StickRight, buttons.value(StickRight));
    if (currentMode == EightWayMode)
    {
        temphash.insert(StickLeftUp, buttons.value(StickLeftUp));
        temphash.insert(StickRightUp, buttons.value(StickRightUp));
        temphash.insert(StickRightDown, buttons.value(StickRightDown));
        temphash.insert(StickLeftDown, buttons.value(StickLeftDown));
    }

    QHashIterator<JoyStickDirections, JoyControlStickButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyControlStickButton *button = iter.next().value();
            presetSensitivity = button->getSensitivity();
        }
        else
        {
            JoyControlStickButton *button = iter.next().value();
            double temp = button->getSensitivity();
            if (temp != presetSensitivity)
            {
                presetSensitivity = 1.0;
                iter.toBack();
            }
        }
    }

    return presetSensitivity;
}
