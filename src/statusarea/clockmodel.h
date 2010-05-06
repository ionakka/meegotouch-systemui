/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of systemui.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#ifndef CLOCKMODEL_H_
#define CLOCKMODEL_H_

#include <MWidgetModel>
#include <QTime>

/*!
 * A model for the Clock widget.
 */
class ClockModel : public MWidgetModel
{
    Q_OBJECT
    M_MODEL(ClockModel)

    //! The current time
    M_MODEL_PROPERTY(QTime, time, Time, true, QTime::currentTime())
    //! Whether 24 hour clock mode is used
    M_MODEL_PROPERTY(bool, timeFormat24h, TimeFormat24h, true, true)
    //! Whether an alarm is currently set
    M_MODEL_PROPERTY(bool, alarmSet, AlarmSet, true, false)
};

#endif /* CLOCKMODEL_H_ */
