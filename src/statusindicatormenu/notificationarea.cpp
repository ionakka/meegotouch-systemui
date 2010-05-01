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

#include "sysuid.h"
#include "notificationarea.h"
#include "notificationareasink.h"
#include "mcompositornotificationsink.h"
#include <MInfoBanner>
#include <QTimer>

NotificationArea::NotificationArea(MWidget *parent) :
    MWidgetController(new NotificationAreaModel, parent),
    notificationAreaSink(new NotificationAreaSink)
{
    // Connect notification signals
    NotificationManager *notificationManager = &Sysuid::sysuid()->notificationManager();

    connect(notificationManager, SIGNAL(groupUpdated(uint, const NotificationParameters &)), notificationAreaSink, SLOT(addGroup(uint, const NotificationParameters &)));
    connect(notificationManager, SIGNAL(groupRemoved(uint)), notificationAreaSink, SLOT(removeGroup(uint)));
    connect(notificationManager, SIGNAL(notificationRemoved(uint)), notificationAreaSink, SLOT(removeNotification(uint)));
    connect(notificationManager, SIGNAL(notificationUpdated(const Notification &)), notificationAreaSink, SLOT(addNotification(const Notification &)));
    connect(notificationAreaSink, SIGNAL(addNotification(MInfoBanner &)), this, SLOT(addNotification(MInfoBanner &)));
    connect(notificationAreaSink, SIGNAL(removeNotification(MInfoBanner &)), this, SLOT(removeNotification(MInfoBanner &)));
    connect(notificationAreaSink, SIGNAL(notificationRemovalRequested(uint)), notificationManager, SLOT(removeNotification(uint)));
    connect(notificationAreaSink, SIGNAL(notificationGroupClearingRequested(uint)), notificationManager, SLOT(removeNotificationsInGroup(uint)));
    connect(notificationAreaSink, SIGNAL(notificationAddedToGroup(MInfoBanner &)), this, SLOT(moveNotificationToTop(MInfoBanner &)));
    connect(notificationAreaSink, SIGNAL(bannerClicked()), this, SIGNAL(bannerClicked()));
}

NotificationArea::~NotificationArea()
{
    delete notificationAreaSink;
}

void NotificationArea::moveNotificationToTop(MInfoBanner &notification)
{
    QList<MInfoBanner *> banners(model()->banners());
    if(banners.count() != 0) {
        banners.move(banners.indexOf(&notification), 0);
        model()->setBanners(banners);
    }
}

void NotificationArea::addNotification(MInfoBanner &notification)
{
    // Put the notification into the model of the notification area
    notification.setParentItem(this);
    QList<MInfoBanner *> banners(model()->banners());
    banners.push_front(&notification);
    model()->setBanners(banners);

    emit notificationCountChanged(model()->banners().count());
}

void NotificationArea::removeNotification(MInfoBanner &notification)
{
    // Remove the notification from the model of the notification area
    QList<MInfoBanner *> banners(model()->banners());
    banners.removeOne(&notification);
    model()->setBanners(banners);
    notification.setParentItem(NULL);

    emit notificationCountChanged(model()->banners().count());
}
