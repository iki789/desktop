//
// Created by veli on 12/7/18.
//

#include <QtCore/QUuid>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtWidgets/QMessageBox>
#include <QtNetwork/QNetworkSession>
#include <QtCore/QJsonObject>
#include "AppUtils.h"

bool AppUtils::applyAdapterName(DeviceConnection *connection)
{
    quint32 ipv4Address = connection->hostAddress.toIPv4Address();

    if (ipv4Address <= 0)
        return false;

    QNetworkConfigurationManager manager;

    const QList<QNetworkConfiguration> &activeConfigurations
            = manager.allConfigurations(QNetworkConfiguration::StateFlag::Active);

    for (const QNetworkConfiguration &config : activeConfigurations) {
        QNetworkSession session(config);
        const QString &interfaceName(session.interface().name());

        for (const QNetworkAddressEntry &address : session.interface().addressEntries()) {
            quint32 netmask = address.netmask().toIPv4Address();

            if (netmask <= 0)
                continue;

            // Declare as found when the IP is in range with netmask and minus 255
            if (netmask - 255 < ipv4Address && ipv4Address <= netmask) {
                connection->adapterName = interfaceName;
                return true;
            }
        }
    }

    connection->adapterName = nullptr;

    return false;
}

void AppUtils::applyDeviceToJSON(QJsonObject &object)
{
    NetworkDevice *device = getLocalDevice();
    QJsonObject deviceInfo;
    QJsonObject appInfo;

    deviceInfo.insert(KEYWORD_DEVICE_INFO_SERIAL, device->deviceId);
    deviceInfo.insert(KEYWORD_DEVICE_INFO_BRAND, device->brand);
    deviceInfo.insert(KEYWORD_DEVICE_INFO_MODEL, device->model);
    deviceInfo.insert(KEYWORD_DEVICE_INFO_USER, device->nickname);

    appInfo.insert(KEYWORD_APP_INFO_VERSION_CODE, device->versionNumber);
    appInfo.insert(KEYWORD_APP_INFO_VERSION_NAME, device->versionName);

    object.insert(KEYWORD_APP_INFO, appInfo);
    object.insert(KEYWORD_DEVICE_INFO, deviceInfo);

    delete device;
}

AccessDatabase *AppUtils::getDatabase()
{
    static AccessDatabase *accessDatabase = nullptr;

    if (accessDatabase == nullptr)
        accessDatabase = newDatabaseInstance(QApplication::instance());

    return accessDatabase;
}

AccessDatabaseSignaller *AppUtils::getDatabaseSignaller()
{
    static AccessDatabaseSignaller *signaller = nullptr;

    if (signaller == nullptr)
        signaller = new AccessDatabaseSignaller(getDatabase());

    return signaller;
}

QSettings &AppUtils::getDefaultSettings()
{
    static QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Genonbeta",
                              QApplication::applicationName());

    return settings;
}

NetworkDevice *AppUtils::getLocalDevice()
{
    NetworkDevice *thisDevice = new NetworkDevice(getDeviceId());

    thisDevice->brand = getDeviceTypeName();
    thisDevice->model = getDeviceNameForOS();
    thisDevice->nickname = getUserNickname();
    thisDevice->versionName = getApplicationVersion();
    thisDevice->versionNumber = getApplicationVersionCode();

    return thisDevice;
}

QString AppUtils::getDeviceId()
{
    QSettings &settings = getDefaultSettings();

    if (!settings.contains("deviceUUID"))
        settings.setValue("deviceUUID", QUuid::createUuid().toString());

    return settings.value("deviceUUID", QString()).toString();
}

AccessDatabase *AppUtils::newDatabaseInstance(QObject *parent)
{
    QSqlDatabase *db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    db->setDatabaseName("local.db");

    if (db->open()) {
        cout << "Database has opened" << endl;

        auto *database = new AccessDatabase(db, parent);
        database->initialize();

        return database;
    }

    return nullptr;
}