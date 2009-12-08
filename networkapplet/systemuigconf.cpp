#include "systemuigconf.h"

#include <DuiGConfItem>
#include <QDebug>

SystemUIGConf::SystemUIGConf(QObject* parent) :
        QObject(parent)
{
    // init the gconf keys    
    duiGConfItems.insert(SystemUIGConf::NetworkToggle, new DuiGConfItem(mapGConfKey(SystemUIGConf::NetworkToggle)));
    duiGConfItems.insert(SystemUIGConf::NetworkRoaming, new DuiGConfItem(mapGConfKey(SystemUIGConf::NetworkRoaming)));
    duiGConfItems.insert(SystemUIGConf::NetworkRoamingUpdates, new DuiGConfItem(mapGConfKey(SystemUIGConf::NetworkRoamingUpdates)));

    QHash<SystemUIGConf::GConfKey, DuiGConfItem *>::iterator i;
    for (i = duiGConfItems.begin(); i != duiGConfItems.end(); ++i)
        connect( i.value(), SIGNAL(valueChanged()), this, SLOT(keyValueChanged()));

}

SystemUIGConf::~SystemUIGConf()
{
    QHash<SystemUIGConf::GConfKey, DuiGConfItem *>::iterator i;
    for (i = duiGConfItems.begin(); i != duiGConfItems.end(); ++i) {
        delete i.value();
        i.value() = NULL;
    }
}

int SystemUIGConf::keyCount(SystemUIGConf::GConfKeyGroup keyGroup)
{    
    DuiGConfItem duiGConfItem(mapGConfKeyGroup(keyGroup));
    QList<QString> list = duiGConfItem.listEntries();
    return list.size();
}

void SystemUIGConf::setValue(SystemUIGConf::GConfKey key, QVariant value)
{
    duiGConfItems.value(key)->set(value);
}

QVariant SystemUIGConf::value(SystemUIGConf::GConfKey key, QVariant def)
{
    if(def.isNull())
        return duiGConfItems.value(key)->value();
    else
        return duiGConfItems.value(key)->value(def);
}

void SystemUIGConf::keyValueChanged()
{
    DuiGConfItem *item = static_cast<DuiGConfItem *>(this->sender());
    emit valueChanged(duiGConfItems.key(item), item->value());
}

QString SystemUIGConf::mapGConfKeyGroup(SystemUIGConf::GConfKeyGroup keyGroup)
{
    QString keyGroupStr;
    switch(keyGroup) {
        case SystemUIGConf::Network:
            keyGroupStr = "/temp/network";
            break;        
        default:
            break;
    }
    return keyGroupStr;
}

QString SystemUIGConf::mapGConfKey(SystemUIGConf::GConfKey key)
{
    QString keyStr("%1%2");
    switch(key) {        
        case SystemUIGConf::NetworkToggle:
            keyStr = keyStr.arg(mapGConfKeyGroup(SystemUIGConf::Network)).arg("/networkToggle");
            break;
        case SystemUIGConf::NetworkRoaming:
            keyStr = keyStr.arg(mapGConfKeyGroup(SystemUIGConf::Network)).arg("/networkRoaming");
            break;
        case SystemUIGConf::NetworkRoamingUpdates:
            keyStr = keyStr.arg(mapGConfKeyGroup(SystemUIGConf::Network)).arg("/networkRoamingUpdates");
            break;
        default:
            break;
    }
    return keyStr;
}