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
#include "ut_mcompositornotificationsink.h"
#include <QtTest/QtTest>
#include <MApplication>
#include <MSceneManager>
#include "mcompositornotificationsink.h"
#include "mnotificationproxy.h"
#include "testnotificationparameters.h"
#include "genericnotificationparameterfactory.h"
#include <MOnDisplayChangeEvent>
#include <MGConfItem>

static const QString NOTIFICATION_PREVIEW_ENABLED = "/desktop/meego/notifications/previews_enabled";

static QList<MSceneWindow*> gMSceneWindowsAppeared;
static QList<MSceneWindow::DeletionPolicy> gMSceneWindowDeletionPolicies;

MSceneWindowBridge::MSceneWindowBridge(QObject *parent): QObject(parent)
{
}

void MSceneWindowBridge::setSceneWindowState(MSceneWindow::SceneWindowState newState)
{
    QObject *testInterface = children()[0];

    QMetaObject::invokeMethod(testInterface, "setSceneWindowState", Qt::DirectConnection,
            Q_ARG(MSceneWindow::SceneWindowState, newState));
}

bool gMWindowIsOnDisplay = false;
bool MWindow::isOnDisplay() const
{
    return gMWindowIsOnDisplay;
}


// Mock notification manager (used by MCompositorNotificationSink)
MockNotificationManager::MockNotificationManager() :
    nextAvailableNotificationID(0)
{
}

uint MockNotificationManager::addNotification(uint, const NotificationParameters &parameters, uint groupId, int timeout)
{
    uint notificationId = nextAvailableNotificationID++;
    Notification notification;
    if (parameters.value("class") == QString("system")) {
        notification = Notification(notificationId, groupId, 0, parameters, Notification::SystemEvent, timeout);
    } else {
        notification = Notification(notificationId, groupId, 0, parameters, Notification::ApplicationEvent, timeout);
    }
    notificationContainer.append(notification);
    emit notificationUpdated(notification);
    return notificationId;
}

uint MockNotificationManager::addNotification(uint notificationUserId, const NotificationParameters &parameters, uint groupId)
{
    return addNotification(notificationUserId, parameters, groupId, 1000);
}

bool MockNotificationManager::updateNotification(uint, uint notificationId, const NotificationParameters &parameters)
{
    for (int i = 0; i < notificationContainer.count(); ++i) {
        if (notificationContainer.at(i).notificationId() == notificationId) {
            Notification oldNotification = notificationContainer.at(i);
            Notification newNotification = Notification(notificationId, oldNotification.groupId(), oldNotification.userId(), parameters, oldNotification.type(), 1000);
            notificationContainer[i] = newNotification;
            emit notificationUpdated(newNotification);
            return true;
        }
    }
    return false;
}

bool MockNotificationManager::removeNotification(uint, uint notificationId)
{
    bool removed = false;
    for (int i = 0; i < notificationContainer.count(); ++i) {
        if (notificationContainer.at(i).notificationId() == notificationId) {
            notificationContainer.removeAt(i);
            removed = true;
            break;
        }
    }

    if (removed) {
        emit notificationRemoved(notificationId);
    }

    return removed;
}

uint MockNotificationManager::addGroup(uint, const NotificationParameters &)
{
    return 0;
}

bool MockNotificationManager::updateGroup(uint, uint, const NotificationParameters &)
{
    return false;
}

bool MockNotificationManager::removeGroup(uint, uint)
{
    return false;
}

uint MockNotificationManager::notificationUserId()
{
    return 0;
}

QList< uint > MockNotificationManager::notificationIdList(uint /*notificationUserId*/)
{
    QList<uint> tmp;
    return tmp;
}

QList<MNotificationProxy> MockNotificationManager::notificationList(uint)
{
    return QList<MNotificationProxy>();
}

QList<MNotificationGroupProxy> MockNotificationManager::notificationGroupList(uint)
{
    return QList<MNotificationGroupProxy>();
}

QList<MNotificationWithIdentifierProxy> MockNotificationManager::notificationListWithIdentifiers(uint)
{
    return QList<MNotificationWithIdentifierProxy>();
}

QList<MNotificationGroupWithIdentifierProxy> MockNotificationManager::notificationGroupListWithIdentifiers(uint)
{
    return QList<MNotificationGroupWithIdentifierProxy>();
}

QList<Notification> MockNotificationManager::notifications() const
{
    QList<Notification> tmp;
    return tmp;
}

QList<NotificationGroup> MockNotificationManager::groups() const
{
    QList<NotificationGroup> tmp;
    return tmp;
}

bool windowEventFilterCalled = false;
bool windowEventFilterBlock = false;
bool WindowEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    MOnDisplayChangeEvent *displayChangeEvent = dynamic_cast<MOnDisplayChangeEvent *>(event);
    if (displayChangeEvent != NULL) {
        windowEventFilterCalled = true;
        if (windowEventFilterBlock) {
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

// MWindow stubs (used by MCompositorNotificationSink)
bool mWindowSetVisibleValue = false;
QWidget* mWindowSetVisibleWidget = NULL;
void MWindow::setVisible(bool visible)
{
    mWindowSetVisibleValue = visible;
    mWindowSetVisibleWidget = this;

    setAttribute(visible ? Qt::WA_WState_Visible : Qt::WA_WState_Hidden);
}

// QWidget stubs (used by MCompositorNotificationSink)
QRegion mWindowMaskRegion;
void QWidget::setMask(const QRegion &region)
{
    MWindow *window = dynamic_cast<MWindow *>(this);
    if (window != NULL) {
        mWindowMaskRegion = region;
    }
}
static bool gQWidgetClearMaskCalled = false;
void QWidget::clearMask()
{
    gQWidgetClearMaskCalled = true;
}

// MGConfItem stub
QVariant gconfValue = QVariant();

QVariant MGConfItem::value() const {

    if (key() == "/meegotouch/target/name")
        return QVariant ("Default");

    return gconfValue;
}

QVariant MGConfItem::value(const QVariant &def) const {
    if (gconfValue.isValid())
        return gconfValue;
    else
        return def;
}

void MGConfItem::set(const QVariant &val) {
    gconfValue = val;
}

void MGConfItem::unset() {
    gconfValue = QVariant();
}

QList<QGraphicsItem *> QGraphicsScene::items () const
{
    QList<QGraphicsItem*> items;
    if (gMSceneWindowsAppeared.size() > 0) {
        items.append(gMSceneWindowsAppeared.at(0));
    }
    return items;
}

void MSceneManager::appearSceneWindow(MSceneWindow *sceneWindow, MSceneWindow::DeletionPolicy policy)
{
    gMSceneWindowsAppeared.append(sceneWindow);
    gMSceneWindowDeletionPolicies.append(policy);
    MSceneWindowBridge bridge;
    bridge.setObjectName("_m_testBridge");
    bridge.setParent(sceneWindow);
    bridge.setSceneWindowState(MSceneWindow::Appeared);
}

MSceneWindow *mSceneManagerDisappearSceneWindowWindow= NULL;
void MSceneManager::disappearSceneWindow(MSceneWindow *sceneWindow)
{
    mSceneManagerDisappearSceneWindowWindow = sceneWindow;
}

M::OrientationAngle gCurrentOrientationAngle;
M::OrientationAngle MSceneManager::orientationAngle() const
{
    return gCurrentOrientationAngle;
}

void MSceneManager::setOrientationAngle(M::OrientationAngle angle, TransitionMode mode)
{
    Q_UNUSED(mode);
    gCurrentOrientationAngle = angle;
    M::Orientation o = (angle == M::Angle0 || angle == M::Angle180) ? M::Landscape : M::Portrait;

    emit orientationAboutToChange(o);
    emit orientationChangeFinished(o);
}

// QTimer stubs (used by MCompositorNotificationSink)
bool qTimerStarted = false;
bool qQTimerEmitTimeoutImmediately = true;
void QTimer::start(int)
{
    qTimerStarted = true;
    if (qQTimerEmitTimeoutImmediately) {
        emit timeout();
    }
}

bool gQTimerStopped = false;
void QTimer::stop()
{
    gQTimerStopped = true;
}

// Tests
void Ut_MCompositorNotificationSink::initTestCase()
{
    static int argc = 1;
    static char *app_name = (char *)"./ut_mcompositornotificationsink";
    app = new MApplication(argc, &app_name);
}

void Ut_MCompositorNotificationSink::cleanupTestCase()
{
    delete app;
}

void Ut_MCompositorNotificationSink::init()
{
    gconfValue = QVariant();
    notificationManager = new MockNotificationManager();
    sink = new MCompositorNotificationSink();
    connect(notificationManager, SIGNAL(notificationRemoved(uint)), sink, SLOT(removeNotification(uint)));
    connect(notificationManager, SIGNAL(notificationUpdated(Notification)), sink, SLOT(addNotification(Notification)));
    gCurrentOrientationAngle = M::Angle0;
    mWindowSetVisibleValue = false;
    mWindowSetVisibleWidget = NULL;
    gQWidgetClearMaskCalled = false;
    gMSceneWindowsAppeared.clear();
    gMSceneWindowDeletionPolicies.clear();

    qQTimerEmitTimeoutImmediately = true;
    qTimerStarted = false;
    gQTimerStopped = false;
    mSceneManagerDisappearSceneWindowWindow = NULL;
    windowEventFilterCalled = false;
    windowEventFilterBlock = false;
    mWindowMaskRegion = QRegion();
    gMWindowIsOnDisplay = false;

    connect(this, SIGNAL(displayEntered()), sink->window, SIGNAL(displayEntered()));
}

void Ut_MCompositorNotificationSink::cleanup()
{
    delete sink;
    delete notificationManager;
}

NotificationParameters Ut_MCompositorNotificationSink::setupSinkDisabledTests(bool isSystemEvent)
{
    connect(this, SIGNAL(statusIndictorMenuVisibilityChanged(bool)), sink, SLOT(setDisabled(bool)));
    emit statusIndictorMenuVisibilityChanged(true);
    // Create notification
    TestNotificationParameters parameters("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    if (isSystemEvent) {
        parameters.add(GenericNotificationParameterFactory::classKey(), "system");
    }
    notificationManager->addNotification(0, parameters);
    return parameters;
}

void Ut_MCompositorNotificationSink::testNotificationWindowProperties()
{
    // Check that the window doesn't request focus
    QCOMPARE(sink->window->testAttribute(Qt::WA_X11DoNotAcceptFocus), true);
}

void Ut_MCompositorNotificationSink::testAddNotificationWhenWindowNotOpen()
{
    QSignalSpy spy(sink, SIGNAL(notificationAdded(const Notification&)));

    // Create a notification
    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    notificationManager->addNotification(0, parameters0, 0);
    emit displayEntered();

    // Check that a MWindow was shown(window->show() called)
    QCOMPARE(mWindowSetVisibleWidget, sink->window);
    QCOMPARE(mWindowSetVisibleValue, true);

    // Check that a MBanner was created with the given parameters
    MBanner* banner = static_cast<MBanner*>(gMSceneWindowsAppeared.at(0));
    QVERIFY(banner);
    QCOMPARE(banner->styleName(), banner->objectName());
    QCOMPARE(banner->iconID(), QString("buttonicon0"));
    QCOMPARE(banner->title(), QString("title0"));
    QCOMPARE(banner->subtitle(), QString("subtitle0"));

    // check that the timeout timer was started
    QCOMPARE(qTimerStarted, true);

    // check that the transfer notification signal was emitted
    QCOMPARE(spy.count(), 1);
}

void Ut_MCompositorNotificationSink::testAddNotificationWhenWindowAlreadyOpen()
{
    // Create a notification: this will create a window
    qQTimerEmitTimeoutImmediately = false;
    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    notificationManager->addNotification(0, parameters0, 0);
    emit displayEntered();
    QVERIFY(mWindowSetVisibleWidget);
    QVERIFY(mWindowSetVisibleValue);
    QVERIFY(qTimerStarted);

    // Reset stubs to see what happens next
    mWindowSetVisibleWidget = NULL;
    mWindowSetVisibleValue = false;
    qTimerStarted = false;

    QSignalSpy spy(sink, SIGNAL(notificationAdded(const Notification&)));

    // Create another notification
    TestNotificationParameters parameters1("title1", "subtitle1", "buttonicon1", "content1 0 0 0");
    notificationManager->addNotification(1, parameters1, 0);

    MBanner* first_banner = static_cast<MBanner*>(gMSceneWindowsAppeared.at(0));
    QVERIFY(first_banner);
    MSceneWindowBridge bridge;
    bridge.setObjectName("_m_testBridge");
    bridge.setParent(first_banner);
    bridge.setSceneWindowState(MSceneWindow::Disappeared);

    // Check that the MWindow was not unnecessarily shown again
    QVERIFY(!mWindowSetVisibleWidget);
    QVERIFY(!mWindowSetVisibleValue);

    // Check that a MBanner was created with the given parameters
    MBanner* banner = static_cast<MBanner*>(gMSceneWindowsAppeared.at(1));
    QVERIFY(banner);
    QCOMPARE(banner->iconID(), QString("buttonicon1"));
    QCOMPARE(banner->title(), QString("title1"));
    QCOMPARE(banner->subtitle(), QString("subtitle1"));

    // check that the timeout timer was started
    QCOMPARE(qTimerStarted, true);

    // check that the transfer notification signal was emitted
    QCOMPARE(spy.count(), 1);
}

void Ut_MCompositorNotificationSink::testWhenWindowOnDisplayThenNotificationAddedWithoutSignal()
{
    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    notificationManager->addNotification(0, parameters0, 0);
    emit displayEntered();

    MBanner* first_banner = static_cast<MBanner*>(gMSceneWindowsAppeared.at(0));
    QVERIFY(first_banner);

    sink->window->QGraphicsView::setVisible(false);

    gMWindowIsOnDisplay = true;
    TestNotificationParameters parameters1("title1", "subtitle1", "buttonicon1", "content1 0 0 0");
    notificationManager->addNotification(1, parameters1, 0);

    QCOMPARE(gMSceneWindowsAppeared.count(), 2);
    QVERIFY(dynamic_cast<MBanner*>(gMSceneWindowsAppeared.at(1)));
}

void Ut_MCompositorNotificationSink::testWhenRemovingCurrentNotificationThenTimerStopsAndBannerDisappears()
{
    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    uint id = notificationManager->addNotification(0, parameters0, 0);
    emit displayEntered();

    QCOMPARE(gMSceneWindowsAppeared.count(), 1);
    MBanner* banner = dynamic_cast<MBanner*>(gMSceneWindowsAppeared.at(0));
    QVERIFY(banner);

    notificationManager->removeNotification(0, id);
    QVERIFY(gQTimerStopped);

    QCOMPARE(mSceneManagerDisappearSceneWindowWindow, banner);
}

void Ut_MCompositorNotificationSink::testUpdateNotification()
{
    // Create two notifications
    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    notificationManager->addNotification(0, parameters0);
    emit displayEntered();
    MBanner* banner1 = static_cast<MBanner*>(gMSceneWindowsAppeared.at(0));

    TestNotificationParameters parameters1( "title1", "subtitle1", "buttonicon1", "content1 1 1 1");
    uint id = notificationManager->addNotification(0, parameters1);
    // Window will not drop to displaychange event slot unless window is explicitly hidden
    mWindowSetVisibleWidget->hide();
    emit displayEntered();

    // Update the second notification
    TestNotificationParameters parametersX("titleX", "subtitleX", "buttoniconX", "contentX X X X");
    notificationManager->updateNotification(0, id, parametersX);

    MSceneWindowBridge bridge;
    bridge.setObjectName("_m_testBridge");
    bridge.setParent(banner1);
    bridge.setSceneWindowState(MSceneWindow::Disappeared);
    emit displayEntered();
    MBanner* banner2 = static_cast<MBanner*>(gMSceneWindowsAppeared.at(1));

    QCOMPARE(banner2->title(), QString("titleX"));
    QCOMPARE(banner2->subtitle(), QString("subtitleX"));
    QCOMPARE(banner2->iconID(), QString("buttoniconX"));

    // Make sure the fist notification banner has still the same content
    QCOMPARE(banner1->title(), QString("title0"));
    QCOMPARE(banner1->subtitle(), QString("subtitle0"));
    QCOMPARE(banner1->iconID(), QString("buttonicon0"));
}

void Ut_MCompositorNotificationSink::testRemoveNotification()
{
    // Create three notifications
    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    parameters0.add(GenericNotificationParameterFactory::classKey(), "system");
    notificationManager->addNotification(0, parameters0, 0);
    emit displayEntered();

    // Make sure that the first one has been shown
    MBanner* banner = static_cast<MBanner*>(gMSceneWindowsAppeared.at(0));
    QVERIFY(banner);

    // Create and immediately remove a notification
    TestNotificationParameters parameters1("title1", "subtitle1", "buttonicon1", "content1 1 1 1");
    uint id = notificationManager->addNotification(0, parameters1);
    notificationManager->removeNotification(0, id);

    // Now make the first banner disappear
    MSceneWindowBridge bridge;
    bridge.setObjectName("_m_testBridge");
    bridge.setParent(banner);
    bridge.setSceneWindowState(MSceneWindow::Disappeared);

    QVERIFY(mWindowSetVisibleWidget);
    QVERIFY(!mWindowSetVisibleValue);
}

void Ut_MCompositorNotificationSink::testTimeout()
{
    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    notificationManager->addNotification(0, parameters0);
    emit displayEntered();
    MBanner* banner = static_cast<MBanner*>(gMSceneWindowsAppeared.at(0));
    MSceneWindowBridge bridge;
    bridge.setObjectName("_m_testBridge");
    bridge.setParent(banner);
    bridge.setSceneWindowState(MSceneWindow::Disappeared);

    // Check that the window disappeared
    QVERIFY(mSceneManagerDisappearSceneWindowWindow);
    QCOMPARE(gMSceneWindowsAppeared.at(0), mSceneManagerDisappearSceneWindowWindow);
}

void Ut_MCompositorNotificationSink::testNotificationWhileApplicationEventsDisabled()
{
    // Create notification
    TestNotificationParameters parameters("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    sink->setApplicationEventsEnabled(false);
    notificationManager->addNotification(0, parameters);
    emit displayEntered();
    QCOMPARE(mWindowSetVisibleValue, false);

    sink->setApplicationEventsEnabled(true);
    notificationManager->addNotification(0, parameters);
    QCOMPARE(mWindowSetVisibleValue, true);
}

void Ut_MCompositorNotificationSink::testWhenSinkDisableTrueNoBannerCreated()
{
    NotificationParameters parameters = setupSinkDisabledTests();
    QCOMPARE(mWindowSetVisibleValue, false);
    emit statusIndictorMenuVisibilityChanged(false);
    notificationManager->addNotification(0, parameters);
    QCOMPARE(mWindowSetVisibleValue, true);
}

void Ut_MCompositorNotificationSink::testWhenSinkIsSetToDisabledSystemNotificationsAreStillGenerated()
{
    setupSinkDisabledTests(true);
    // Check that notification is shown
    QCOMPARE(mWindowSetVisibleValue, true);
}

void Ut_MCompositorNotificationSink::testNotificationPreviewsDisabled_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<bool>("windowshown");

    QTest::newRow("key not set, notification shown") << QVariant() << true;
    QTest::newRow("key has garbage value, notification shown") << QVariant(QString("Garbage")) << true;
    QTest::newRow("key has has previews enabled, window shown") << QVariant(true) << true;
    QTest::newRow("key has has previews disabled, window not shown") << QVariant(false) << false;
}


void Ut_MCompositorNotificationSink::testNotificationPreviewsDisabled()
{
    // Reset previews as not disabled
    sink->allPreviewsDisabled = false;

    // Check that window is following right gconf key
    QVERIFY (sink->notificationPreviewMode->key() == NOTIFICATION_PREVIEW_ENABLED);

    QFETCH(QVariant, value);
    QFETCH(bool, windowshown);

    // Set value from test data to gconf key
    sink->notificationPreviewMode->set(value);
    sink->changeNotificationPreviewMode();

    // Create normal notification
    TestNotificationParameters parameters("title0", "subtitle0", "buttonicon0", "content0 0 0 0");

    // Check that notification is shown/not shown according to test data parameter
    notificationManager->addNotification(0, parameters);
    QCOMPARE(mWindowSetVisibleValue, windowshown);

    // Create system notification
    parameters.add(GenericNotificationParameterFactory::classKey(), "system");
    notificationManager->addNotification(0, parameters);

    // Check that notification is not shown
    QCOMPARE(mWindowSetVisibleValue, windowshown);
}

void Ut_MCompositorNotificationSink::testWindowMasking_data()
{
    QTest::addColumn<M::OrientationAngle>("angle");
    QTest::newRow("0")   << M::Angle0;
    QTest::newRow("90")  << M::Angle90;
    QTest::newRow("180") << M::Angle180;
    QTest::newRow("270") << M::Angle270;
}

const QRegion Ut_MCompositorNotificationSink::calculateTargetMaskRegion(M::OrientationAngle angle, MSceneWindow* window)
{
    QRect maskRect;
    switch(angle) {
    case M::Angle0:   maskRect = QRect(0, 0, window->preferredWidth(), window->preferredHeight()); break;
    case M::Angle90:  maskRect = QRect(sink->window->width() - window->preferredHeight(), 0, window->preferredHeight(), window->preferredWidth()); break;
    case M::Angle180: maskRect = QRect(0, sink->window->height() - window->preferredHeight(), window->preferredWidth(), window->preferredHeight()); break;
    case M::Angle270: maskRect = QRect(0, 0, window->preferredHeight(), window->preferredWidth()); break;
    }

    QRegion region(maskRect, QRegion::Rectangle);
    return region;
}

void Ut_MCompositorNotificationSink::testWindowMasking()
{
    QFETCH(M::OrientationAngle, angle);
    gCurrentOrientationAngle = angle;
    qQTimerEmitTimeoutImmediately = false;

    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    notificationManager->addNotification(0, parameters0, 0);
    emit displayEntered();

    MSceneWindow* window = gMSceneWindowsAppeared.at(0);

    QRegion region = calculateTargetMaskRegion(angle, window);
    QCOMPARE(mWindowMaskRegion, region);
}

void Ut_MCompositorNotificationSink::testWindowMaskingWhenOrientationChangeSignalsEmitted()
{
    qQTimerEmitTimeoutImmediately = false;

    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    notificationManager->addNotification(0, parameters0);
    emit displayEntered();
    int newAngle = sink->window->sceneManager()->orientationAngle() + M::Angle90;
    gCurrentOrientationAngle = (M::OrientationAngle)newAngle;

    sink->window->sceneManager()->setOrientationAngle(gCurrentOrientationAngle, MSceneManager::ImmediateTransition);
    QVERIFY(gQWidgetClearMaskCalled);

    MSceneWindow* window = gMSceneWindowsAppeared.at(0);
    QRegion region = calculateTargetMaskRegion(gCurrentOrientationAngle, window);
    QCOMPARE(mWindowMaskRegion, region);
}

void Ut_MCompositorNotificationSink::testPreviewIconId()
{
    qQTimerEmitTimeoutImmediately = false;

    TestNotificationParameters parameters0("title0", "subtitle0", "buttonicon0", "content0 0 0 0");
    uint id = notificationManager->addNotification(0, parameters0);
    emit displayEntered();

    // Check default icon is used if preview icon id is not defined
    MBanner* banner = static_cast<MBanner*>(gMSceneWindowsAppeared.at(0));
    QCOMPARE(banner->iconID(), QString("buttonicon0"));

    // Check that preview icon id is used if defined
    parameters0.add("previewIconId", "previewicon0");
    notificationManager->updateNotification(0, id, parameters0);
    QCOMPARE(banner->iconID(), QString("previewicon0"));
}

QTEST_APPLESS_MAIN(Ut_MCompositorNotificationSink)
