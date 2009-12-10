#include "simcontainer.h"
#include "callandsimtranslation.h"

#include <DuiLayout>
#include <DuiGridLayoutPolicy>
#include <DuiButton>
#include <DuiLabel>

#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>
#include <QDBusInterface>
#include <QDebug>

SimContainer::SimContainer(DuiWidget *parent) :
        DuiContainer(DcpCallAndSim::SimCardText, parent),
        pinRequestLabel(NULL),
        pinRequestButton(NULL),
        changePinButton(NULL),
        lp(NULL),
        pp(NULL),
        changePinButtonWidget(NULL),
        dummyWidget(NULL)
{
    setLayout();

    dbusPinIf = new QDBusInterface("com.nokia.systemui.pin",
                               "/com/nokia/systemui/pin",
                               "com.nokia.systemui.pin.PinCodeQuery",
                               QDBusConnection::systemBus(),
                               this);

    connect(dbusPinIf, SIGNAL(PinQueryStateCompleted(SIMSecurity::PINQuery)),
            this, SLOT(pinQueryState(SIMSecurity::PINQuery)));
    connect(dbusPinIf, SIGNAL(PinQueryEnabled(SIMSecurity::PINQuery)),
            this, SLOT(pinQueryState(SIMSecurity::PINQuery)));

    dbusPinIf->call(QString("PinQueryState"), QVariant(QString("PIN")));
}

SimContainer::~SimContainer()
{
    toggleChangePinButtonWidget(true);
    delete dummyWidget;
}

void SimContainer::setPinRequest(bool enabled)
{
    qDebug() << Q_FUNC_INFO << enabled;

    if (pinRequestButton->isChecked() != enabled) {
        pinRequestButton->setChecked(enabled);
    } else {
        toggleChangePinButtonWidget(enabled);
    }
}

void SimContainer::buttonToggled(bool checked)
{
    toggleChangePinButtonWidget(checked);
    dbusPinIf->call(QString("EnablePinQuery"), QVariant(checked));
}

void SimContainer::changePinClicked()
{
    qDebug() << Q_FUNC_INFO;
    dbusPinIf->call(QDBus::NoBlock, QString("ChangePinCode"));
}

void SimContainer::pinQueryState(SIMSecurity::PINQuery state)
{
    bool enabled = state == SIMSecurity::Enabled;
    pinRequestButton->setChecked(enabled);
    toggleChangePinButtonWidget(enabled);
}

void SimContainer::setLayout()
{
    // layout & policies
    DuiLayout* layout = new DuiLayout();
    lp = new DuiGridLayoutPolicy(layout);
    layout->setLandscapePolicy(lp); // ownership transferred
    pp = new DuiGridLayoutPolicy(layout);
    layout->setPortraitPolicy(pp); // ownership transferred

    // pin request widget
    pinRequestLabel = new DuiLabel(DcpCallAndSim::PinCodeRequestText);
    pinRequestLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    pinRequestButton = new DuiButton();
    pinRequestButton->setObjectName("checkBoxButton");
    pinRequestButton->setCheckable(true);
    pinRequestButton->setChecked(false);

    QGraphicsWidget *pinRequestWidget = new QGraphicsWidget;
    pinRequestWidget->setPreferredWidth(1);
    QGraphicsLinearLayout* pinRequestLayout = new QGraphicsLinearLayout(Qt::Horizontal, pinRequestWidget);
    pinRequestLayout->setContentsMargins(0, 0, 0, 0);
    pinRequestLayout->addItem(pinRequestLabel);
    pinRequestLayout->setAlignment(pinRequestLabel, Qt::AlignLeft | Qt::AlignVCenter);
    pinRequestLayout->addItem(pinRequestButton);
    pinRequestLayout->setAlignment(pinRequestButton, Qt::AlignRight | Qt::AlignVCenter);

    // change pin code button widget
    changePinButtonWidget = new QGraphicsWidget;
    changePinButtonWidget->setPreferredWidth(1);
    QGraphicsLinearLayout* changePinButtonLayout = new QGraphicsLinearLayout(changePinButtonWidget);
    changePinButtonLayout->setContentsMargins(0, 0, 0, 0);
    changePinButton = new DuiButton(DcpCallAndSim::ChangePinText);
    changePinButtonLayout->addItem(changePinButton);

    // dummy placeholder
    dummyWidget = new QGraphicsWidget;
    dummyWidget->setPreferredWidth(1);

    // landscape policy
    lp->setSpacing(5);
    lp->addItemAtPosition(pinRequestWidget, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    lp->addItemAtPosition(dummyWidget, 0, 1, Qt::AlignCenter);

    // portrait policy
    pp->setSpacing(5);
    pp->addItemAtPosition(pinRequestWidget, 0, 0, Qt::AlignLeft);
    pp->addItemAtPosition(dummyWidget, 1, 0, Qt::AlignCenter);

    // connect signals
    connect(pinRequestButton, SIGNAL(toggled(bool)), this, SLOT(buttonToggled(bool)));
    connect(changePinButton, SIGNAL(clicked()), this, SLOT(changePinClicked()));

    // layout
    centralWidget()->setLayout(layout);
}

void SimContainer::toggleChangePinButtonWidget(bool toggle)
{
    if (toggle) {
        lp->removeItem(dummyWidget);
        lp->addItemAtPosition(changePinButtonWidget, 0, 1, Qt::AlignCenter);
        pp->removeItem(dummyWidget);
        pp->addItemAtPosition(changePinButtonWidget, 1, 0, Qt::AlignCenter);
    } else {
        lp->removeItem(changePinButtonWidget);
        lp->addItemAtPosition(dummyWidget, 0, 1, Qt::AlignCenter);
        pp->removeItem(changePinButtonWidget);
        pp->addItemAtPosition(dummyWidget, 1, 0, Qt::AlignCenter);
    }
}
