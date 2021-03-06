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

#include <QDBusMetaType>
#include "metatypedeclarations.h"
#include "notificationmanager.h"
#include "notification.h"
#include "mnotificationproxy.h"
#include "dbusinterfacenotificationsource.h"
#include "dbusinterfacenotificationsink.h"
#include "contextframeworkcontext.h"
#include "genericnotificationparameterfactory.h"
#include "notificationwidgetparameterfactory.h"
#include <QDBusConnection>
#include <QDir>
#include <QDateTime>
#include <mfiledatastore.h>
#include <QFile>

//! Directory in which the persistent data files are located
static const QString PERSISTENT_DATA_PATH = QDir::homePath() + QString("/.config/sysuid/notificationmanager/");

//! The number configuration files to load into the event type store.
static const uint MAX_EVENT_TYPE_CONF_FILES = 100;

//! Name of the file where persistent status data is stored
static const QString STATE_DATA_FILE_NAME = PERSISTENT_DATA_PATH + QString("state.data");

//! Name of the file where persistent notifications are stored
static const QString NOTIFICATIONS_FILE_NAME = PERSISTENT_DATA_PATH + QString("notifications.data");

//! System notifications are identified with 'system' string literal
static const QString SYSTEM_EVENT_ID = "system";

//! Name of the file to determine whether the system was booted or whether it had crashed
static const QString BOOT_FILE = "/sysuid_boot";

NotificationManager::NotificationManager(int relayInterval, uint maxWaitQueueSize) :
    maxWaitQueueSize(maxWaitQueueSize),
    notificationInProgress(false),
    notificationIdInProgress(0),
    relayInterval(relayInterval),
    context(new ContextFrameworkContext),
    lastUsedNotificationUserId(0),
    subsequentStart(false)
{
    dBusSource = new DBusInterfaceNotificationSource(*this);
    dBusSink = new DBusInterfaceNotificationSink(this);

    connect(this, SIGNAL(groupUpdated(uint, const NotificationParameters &)), dBusSink, SLOT(addGroup(uint, const NotificationParameters &)));
    connect(this, SIGNAL(groupRemoved(uint)), dBusSink, SLOT(removeGroup(uint)));
    connect(this, SIGNAL(notificationRemoved(uint)), dBusSink, SLOT(removeNotification(uint)));
    connect(this, SIGNAL(notificationRestored(const Notification &)), dBusSink, SLOT(addNotification(const Notification &)));
    connect(this, SIGNAL(notificationUpdated(const Notification &)), dBusSink, SLOT(addNotification(const Notification &)));
    connect(dBusSink, SIGNAL(notificationRemovalRequested(uint)), this, SLOT(removeNotification(uint)));
    connect(dBusSink, SIGNAL(notificationGroupClearingRequested(uint)), this, SLOT(removeNotificationsInGroup(uint)));
    connect(this, SIGNAL(queuedGroupRemove(uint)), this, SLOT(doRemoveGroup(uint)), Qt::QueuedConnection);
    connect(this, SIGNAL(queuedNotificationRemove(uint)), this, SLOT(removeNotification(uint)), Qt::QueuedConnection);

    waitQueueTimer.setSingleShot(true);
    connect(&waitQueueTimer, SIGNAL(timeout()), this, SLOT(relayNextNotification()));

    initializeEventTypeStore();

    // Connect to D-Bus and register the DBus source as an object
    QDBusConnection::sessionBus().registerService("com.meego.core.MNotificationManager");
    QDBusConnection::sessionBus().registerObject("/notificationmanager", dBusSource);
    QDBusConnection::sessionBus().registerObject("/notificationsinkmanager", dBusSink);
}

void NotificationManager::initializeStore()
{
    // Non-persistent notifications are pruned during reboot by saving notifications after restoring only persistent notifications
    restoreData();
    saveNotifications();
}

NotificationManager::~NotificationManager()
{
    delete dBusSource;
    delete dBusSink;
    delete context;
}

bool NotificationManager::ensurePersistentDataPath()
{
    // Create the data store path if it does not exist yet
    if (!QDir::root().exists(PERSISTENT_DATA_PATH)) {
        return QDir::root().mkpath(PERSISTENT_DATA_PATH);
    }

    return true;
}

void NotificationManager::saveStateData()
{
    if (ensurePersistentDataPath()) {
        QFile file(STATE_DATA_FILE_NAME);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QDataStream stream;
            stream.setDevice(&file);

            stream << lastUsedNotificationUserId;

            foreach(const NotificationGroup & group, groupContainer) {
                stream << group;
            }
            file.close();
        }
    }
}

void NotificationManager::saveNotifications()
{
    if (ensurePersistentDataPath()) {
        if (notificationContainer.size()) {
            QFile file(NOTIFICATIONS_FILE_NAME);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QDataStream stream;
                stream.setDevice(&file);

                foreach(const Notification &notification, notificationContainer) {
                    stream << notification;
                }
                file.close();
            }
        } else {
            QFile::remove(NOTIFICATIONS_FILE_NAME);
        }
    }
}

void NotificationManager::restoreData()
{
    if (ensurePersistentDataPath()) {
        restoreState();
        restoreNotifications();
    }
}

void NotificationManager::restoreState()
{
    QFile stateFile(STATE_DATA_FILE_NAME);

    if (stateFile.open(QIODevice::ReadOnly)) {
        QDataStream stream;
        stream.setDevice(&stateFile);

        stream >> lastUsedNotificationUserId;

        NotificationGroup group;
        while (!stream.atEnd()) {
            // Restore each notification group
            stream >> group;

            // Update the group from the event type parameters to make sure changes in the event type definition are taken into effect
            group.updateParameters(appendEventTypeParameters(group.parameters()));

            // Let the sinks know about the group
            groupContainer.insert(group.groupId(), group);
            emit groupUpdated(group.groupId(), group.parameters());
        }
        stateFile.close();
    }
}

void NotificationManager::restoreNotifications()
{
    // Startup status must be initialized always
    bool restoreAllNotifications = isSubsequentStart();
    QFile notificationFile(NOTIFICATIONS_FILE_NAME);
    if (notificationFile.open(QIODevice::ReadOnly)) {
        QDataStream stream;
        stream.setDevice(&notificationFile);

        Notification notification;
        while (!stream.atEnd()) {
            // Restore each notification
            stream >> notification;

            // When starting on boot add only the persistent notifications
            if (restoreAllNotifications || isPersistent(notification.parameters())) {
                // Update the notification from the event type parameters to make sure changes in the event type definition are taken into effect
                notification.updateParameters(appendEventTypeParameters(notification.parameters()));

                // Let the sinks know about the notification
                notificationContainer.insert(notification.notificationId(), notification);
                emit notificationRestored(notification);
            }
        }
        notificationFile.close();
    }
}

bool NotificationManager::isSubsequentStart()
{
    if (!subsequentStart) {
        subsequentStart = true;
        QFile bootFile(QDir::tempPath() + BOOT_FILE);
        if (!bootFile.exists()) {
            bootFile.open(QIODevice::WriteOnly);
            return false;
        }
    }
    return true;
}

void NotificationManager::initializeEventTypeStore()
{
    if (notificationEventTypeStore) {
        return;
    }
    notificationEventTypeStore = QSharedPointer<EventTypeStore> (new EventTypeStore(NOTIFICATIONS_EVENT_TYPES, MAX_EVENT_TYPE_CONF_FILES));

    connect(notificationEventTypeStore.data(), SIGNAL(eventTypeUninstalled(QString)), this, SLOT(removeNotificationsAndGroupsWithEventType(QString)));
    connect(notificationEventTypeStore.data(), SIGNAL(eventTypeModified(QString)), this, SLOT(updateNotificationsAndGroupsWithEventType(QString)));
}

void NotificationManager::removeNotificationsAndGroupsWithEventType(const QString &eventType)
{
    foreach(const Notification &notification, notificationContainer) {
        if (notification.parameters().value(GenericNotificationParameterFactory::eventTypeKey()).toString() == eventType) {
            removeNotification(notification.notificationId());
        }
    }

    foreach(const NotificationGroup &group, groupContainer) {
        if (group.parameters().value(GenericNotificationParameterFactory::eventTypeKey()).toString() == eventType) {
            doRemoveGroup(group.groupId());
        }
    }
}

void NotificationManager::updateNotificationsAndGroupsWithEventType(const QString &eventType)
{
    foreach(Notification notification, notificationContainer) {
        if(notification.parameters().value(GenericNotificationParameterFactory::eventTypeKey()).toString() == eventType) {
            notification.updateParameters(appendEventTypeParameters(notification.parameters()));
            updateNotification(notification.userId(), notification.notificationId(), notification.parameters());
        }
    }

    foreach(NotificationGroup group, groupContainer) {
        if(group.parameters().value(GenericNotificationParameterFactory::eventTypeKey()).toString() == eventType) {
            group.updateParameters(appendEventTypeParameters(group.parameters()));
            updateGroup(group.userId(), group.groupId(), group.parameters());
        }
    }
}

uint NotificationManager::addNotification(uint notificationUserId, const NotificationParameters &parameters, uint groupId)
{
    if (groupId == 0 || groupContainer.contains(groupId)) {
        uint notificationId = nextAvailableNotificationID();

        NotificationParameters fullParameters(appendEventTypeParameters(parameters));
        fullParameters.add(GenericNotificationParameterFactory::timestampKey(), timestamp(parameters));
        Notification::NotificationType notificationType = determineType(fullParameters);
        if (notificationType == Notification::SystemEvent) {
            // Consider all system notifications not to be persistent
            fullParameters.add(GenericNotificationParameterFactory::persistentKey(), false);
        }
        Notification notification(notificationId, groupId, notificationUserId, fullParameters, notificationType, relayInterval);

        // Mark the notification used
        notificationContainer.insert(notificationId, notification);

        saveNotifications();

        submitNotification(notification);

        updateGroupTimestampFromNotifications(groupId);

        return notificationId;
    }
    return 0;
}

bool NotificationManager::updateNotification(uint notificationUserId, uint notificationId,
                                             const NotificationParameters &parameters)
{
    Q_UNUSED(notificationUserId);

    QHash<uint, Notification>::iterator ni = notificationContainer.find(notificationId);

    if (ni != notificationContainer.end()) {
        NotificationParameters fullParameters(parameters);
        fullParameters.add(GenericNotificationParameterFactory::timestampKey(), timestamp(parameters));
        (*ni).updateParameters(fullParameters);

        saveNotifications();

        int waitQueueIndex = findNotificationFromWaitQueue(notificationId);
        if (waitQueueIndex >= 0) {
            waitQueue[waitQueueIndex].updateParameters(fullParameters);
        } else {
            // Inform the sinks about the update
            emit notificationUpdated(notificationContainer.value(notificationId));
        }

        updateGroupTimestampFromNotifications((*ni).groupId());

        return true;
    } else {
        return false;
    }
}

bool NotificationManager::removeNotification(uint notificationUserId, uint notificationId)
{
    Q_UNUSED(notificationUserId);

    if (notificationContainer.contains(notificationId)) {
        emit queuedNotificationRemove(notificationId);
        return true;
    }
    return false;
}

bool NotificationManager::removeNotification(uint notificationId)
{
    if (notificationContainer.contains(notificationId)) {
        // Mark the notification unused
        const Notification removedNotification = notificationContainer.take(notificationId);

        saveNotifications();

        int waitQueueIndex = findNotificationFromWaitQueue(notificationId);
        if (waitQueueIndex >= 0) {
            waitQueue.removeAt(waitQueueIndex);
        } else {
            // Inform the sinks about the removal
            emit notificationRemoved(notificationId);

            if (notificationInProgress && notificationId == notificationIdInProgress) {
                // The notification being removed is currently displayed
                // cancel the notification relay timeout and relay the next
                // notification
                waitQueueTimer.stop();
                relayNextNotification();
            }
        }

        updateGroupTimestampFromNotifications(removedNotification.groupId());

        return true;
    } else {
        return false;
    }
}

bool NotificationManager::removeNotificationsInGroup(uint groupId)
{
    QList<uint> notificationIds;

    foreach(const Notification & notification, notificationContainer.values()) {
        if (notification.groupId() == groupId) {
            notificationIds.append(notification.notificationId());
        }
    }

    bool result = !notificationIds.isEmpty();
    foreach(uint notificationId, notificationIds) {
        result &= removeNotification(notificationId);
    }

    return result;
}

uint NotificationManager::addGroup(uint notificationUserId, const NotificationParameters &parameters)
{
    NotificationParameters fullParameters(appendEventTypeParameters(parameters));

    uint groupID = nextAvailableGroupID();
    NotificationGroup group(groupID, notificationUserId, fullParameters);
    groupContainer.insert(groupID, group);

    saveStateData();

    emit groupUpdated(groupID, fullParameters);

    return groupID;
}

bool NotificationManager::updateGroup(uint notificationUserId, uint groupId, const NotificationParameters &parameters)
{
    Q_UNUSED(notificationUserId);

    QHash<uint, NotificationGroup>::iterator gi = groupContainer.find(groupId);

    if (gi != groupContainer.end()) {
        gi->updateParameters(parameters);

        saveStateData();

        emit groupUpdated(groupId, gi->parameters());

        return true;
    } else {
        return false;
    }
}

bool NotificationManager::removeGroup(uint notificationUserId, uint groupId)
{
    Q_UNUSED(notificationUserId)

    if (groupContainer.contains(groupId)) {
        emit queuedGroupRemove(groupId);
        return true;
    }
    return false;
}

void NotificationManager::doRemoveGroup(uint groupId)
{
    if (groupContainer.remove(groupId)) {
        foreach(const Notification & notification, notificationContainer) {
            if (notification.groupId() == groupId) {
                removeNotification(notification.notificationId());
            }
        }

        saveStateData();

        emit groupRemoved(groupId);
    }
}

NotificationParameters NotificationManager::appendEventTypeParameters(const NotificationParameters &parameters) const
{
    NotificationParameters fullParameters(parameters);

    QString eventType = parameters.value(GenericNotificationParameterFactory::eventTypeKey()).toString();
    foreach (const QString &key, notificationEventTypeStore->allKeys(eventType)) {
        fullParameters.add(key, notificationEventTypeStore->value(eventType, key));
    }

    return fullParameters;
}

uint NotificationManager::notificationUserId()
{
    lastUsedNotificationUserId++;
    saveStateData();

    return lastUsedNotificationUserId;
}

QList<uint> NotificationManager::notificationIdList(uint notificationUserId)
{
    QList<uint> listOfNotificationIds;

    foreach(const Notification & notification, notificationContainer) {
        if (notification.userId() == notificationUserId) {
            listOfNotificationIds.append(notification.notificationId());
        }
    }

    return listOfNotificationIds;
}

QList<Notification> NotificationManager::notificationList(uint notificationUserId)
{
    QList<Notification> userNotifications;

    foreach (const Notification &notification, notificationContainer) {
        if (notification.userId() == notificationUserId) {
            userNotifications.append(notification);
        }
    }

    return userNotifications;
}

QList<Notification> NotificationManager::notificationListWithIdentifiers(uint notificationUserId)
{
    QList<Notification> userNotificationsWithIdentifiers;

    foreach (const Notification &notification, notificationContainer) {
        if (notification.userId() == notificationUserId) {
            userNotificationsWithIdentifiers.append(notification);
        }
    }

    return userNotificationsWithIdentifiers;
}

QList<NotificationGroup> NotificationManager::notificationGroupList(uint notificationUserId)
{
    QList<NotificationGroup> userGroups;

    foreach (const NotificationGroup &group, groupContainer) {
        if (group.userId() == notificationUserId) {
            userGroups.append(group);
        }
    }

    return userGroups;
}

QList<NotificationGroup> NotificationManager::notificationGroupListWithIdentifiers(uint notificationUserId)
{
    QList<NotificationGroup> userGroups;

    foreach (const NotificationGroup &group, groupContainer) {
        if (group.userId() == notificationUserId) {
            userGroups.append(group);
        }
    }

    return userGroups;
}

void NotificationManager::relayNextNotification()
{
    notificationInProgress = false;
    if (!waitQueue.isEmpty()) {
        submitNotification(waitQueue.takeFirst());
    }
}

Notification::NotificationType NotificationManager::determineType(const NotificationParameters &parameters)
{
    QString classStr;

    QVariant classVariant = parameters.value(GenericNotificationParameterFactory::classKey());
    if (classVariant.isValid()) {
        classStr = classVariant.toString();
    } else {
        QVariant eventTypeVariant = parameters.value(GenericNotificationParameterFactory::eventTypeKey());
        if (eventTypeVariant.isValid()) {
            QString eventType = eventTypeVariant.toString();
            if (notificationEventTypeStore->contains(eventType, GenericNotificationParameterFactory::classKey())) {
                classStr = notificationEventTypeStore->value(eventType, GenericNotificationParameterFactory::classKey());
            }
        }
    }

    return classStr == SYSTEM_EVENT_ID ? Notification::SystemEvent : Notification::ApplicationEvent;
}

int NotificationManager::insertPosition(const Notification &notification)
{
    int pos = 0;
    const NotificationParameters parameters = notification.parameters();
    if(parameters.value("class") != QString("system")) {
        return waitQueue.size();
    } else {
        for(pos = 0; pos < waitQueue.count(); ++pos) {
            if (waitQueue.at(pos).parameters().value("class") == QString("system")) {
                continue;
            } else {
                return pos;
            }
        }
        return pos;
    }
}

void NotificationManager::submitNotification(const Notification &notification)
{
    if (!notificationInProgress) {
        // Inform about the new notification
        emit notificationUpdated(notification);

        if (relayInterval != 0) {
            notificationInProgress = true;
            notificationIdInProgress = notification.notificationId();
            if (relayInterval > 0) {
                waitQueueTimer.start(relayInterval);
            }
        }
    } else {
        // Store new notification in the notification wait queue
        if ((uint)waitQueue.size() < maxWaitQueueSize) {
            waitQueue.insert(insertPosition(notification), notification);
        }
    }
}

int NotificationManager::findNotificationFromWaitQueue(uint notificationId)
{
    for (int i = 0; i < waitQueue.count(); ++i) {
        if (waitQueue[i].notificationId() == notificationId) {
            return i;
        }
    }
    return -1;
}

uint NotificationManager::nextAvailableNotificationID()
{
    unsigned int i = 1;
    // Try to find an unused ID but only do it up to 2^32-1 times
    while (i != 0 && notificationContainer.contains(i)) {
        ++i;
    }
    return i;
}

uint NotificationManager::nextAvailableGroupID()
{
    unsigned int i = 1;
    // Try to find an unused ID but only do it up to 2^32-1 times
    while (i != 0 && groupContainer.contains(i)) {
        ++i;
    }
    return i;
}

QList<Notification> NotificationManager::notifications() const
{
     return notificationContainer.values();
}

QList<NotificationGroup> NotificationManager::groups() const
{
    return groupContainer.values();
}

QObject *NotificationManager::qObject()
{
    return this;
}

uint NotificationManager::notificationCountInGroup(uint notificationUserId, uint groupId)
{
    uint amount = 0;
    foreach(const Notification & notification, notificationContainer.values()) {
        if (notification.groupId() == groupId && notification.userId() == notificationUserId) {
            amount++;
        }
    }
    return amount;
}

bool NotificationManager::isPersistent(const NotificationParameters &parameters)
{
    bool isPersistent = true;

    QVariant persistentVariant = parameters.value(GenericNotificationParameterFactory::persistentKey());
    if (persistentVariant.isValid()) {
        isPersistent = persistentVariant.toBool();
    } else {
        QVariant eventTypeVariant = parameters.value(GenericNotificationParameterFactory::eventTypeKey());
        if (eventTypeVariant.isValid()) {
            QString eventType = eventTypeVariant.toString();
            if (notificationEventTypeStore->contains(eventType, GenericNotificationParameterFactory::persistentKey())) {
                isPersistent = QVariant(notificationEventTypeStore->value(eventType, GenericNotificationParameterFactory::persistentKey())).toBool();
            }
        }
    }

    return isPersistent;
}

uint NotificationManager::timestamp(const NotificationParameters &parameters)
{
    uint timestamp = parameters.value(GenericNotificationParameterFactory::timestampKey()).toUInt();
    return timestamp == 0 ? QDateTime::currentDateTimeUtc().toTime_t() : timestamp;
}

void NotificationManager::updateGroupTimestampFromNotifications(uint groupId)
{
    if (groupId != 0 && groupContainer.contains(groupId)) {
        NotificationGroup group = groupContainer.value(groupId);
        NotificationParameters groupParameters = group.parameters();
        uint oldGroupTimestamp = groupParameters.value(GenericNotificationParameterFactory::timestampKey()).toUInt();

        // Check the latest notification timestamp of the group's notifications
        uint newGroupTimestamp = 0;
        QHash<uint, Notification>::const_iterator notificationIterator;
        for (notificationIterator = notificationContainer.begin(); notificationIterator != notificationContainer.end(); notificationIterator++) {
            if ((*notificationIterator).groupId() == groupId) {
                uint notificationTimestamp = (*notificationIterator).parameters().value(GenericNotificationParameterFactory::timestampKey()).toUInt();
                if (newGroupTimestamp < notificationTimestamp) {
                    newGroupTimestamp = notificationTimestamp;
                }
            }
        }

        if (oldGroupTimestamp != newGroupTimestamp) {
            // Update the group timestamp
            groupParameters.add(GenericNotificationParameterFactory::timestampKey(), newGroupTimestamp);
            group.updateParameters(groupParameters);
            updateGroup(group.userId(), group.groupId(), group.parameters());
        }
    }
}
