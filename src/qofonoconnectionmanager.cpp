/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: lorn.potter@jollamobile.com
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/

#include "qofonoconnectionmanager.h"
#include "dbus/ofonoconnectionmanager.h"
#include "dbustypes.h"
#include <QThread> ///

class QOfonoConnectionManagerPrivate
{
public:
    QOfonoConnectionManagerPrivate();
    QString modemPath;
    OfonoConnectionManager *connman;
    QVariantMap properties;
    QStringList contexts;
    QHash<QString,QString> contextTypes;
    QString filter;
    bool startup;

    void getContexts();
    void filterContexts();
};

QOfonoConnectionManagerPrivate::QOfonoConnectionManagerPrivate() :
    modemPath(QString())
   , connman(0)
  ,contexts(QStringList())
  ,startup(true)
{
	qDebug() << Q_FUNC_INFO << "xyz startup:" << startup;
}

void QOfonoConnectionManagerPrivate::getContexts()
{
qDebug() << Q_FUNC_INFO << "xyz >>> getContexts";
    contextTypes.clear();
    QDBusReply<ObjectPathPropertiesList> reply2 = connman->GetContexts();
    /*
     * Returns the error code that was returned from the remote function call. If the remote call did not return an error (i.e., if it succeeded),
     * then the QDBusError object that is returned will not be a valid error code
     */
    QDBusError err = reply2.error();
    qDebug() << Q_FUNC_INFO << "xyz error isValid:" <<err.isValid() <<" message:" <<err.message() <<" nanme:" << err.name() <<" type:" <<err.type();

    foreach(ObjectPathProperties context, reply2.value()) {
        qDebug() << Q_FUNC_INFO << "xyz contextTypes.insert path: " << context.path.path() << " type: " << context.properties.value("Type").toString();
        contextTypes.insert(context.path.path(), context.properties.value("Type").toString());
    }
    filterContexts();
    foreach (QString path, contexts) {
	    qDebug() << Q_FUNC_INFO << "xyz context: " << path;
    }
    qDebug() << Q_FUNC_INFO << "xyz <<< getContexts";
}

// FILTER = [!]NAMES
// NAMES = NAME [,NAMES]
// Spaces and tabs are ignored
void QOfonoConnectionManagerPrivate::filterContexts()
{
qDebug() << Q_FUNC_INFO << "xyz filter: \"" << filter << "\"";

    if (contextTypes.isEmpty()) {
	//qDebug() << Q_FUNC_INFO << "xyz contextTypes.isEmpty, contexts.clear()";
        contexts.clear();
    } else {
        //qDebug() << Q_FUNC_INFO << "xyz contextTypes not empty";

        QStringList contextList = contextTypes.keys();
        /*
        foreach (QString pp, contextList) {
    	    qDebug() << Q_FUNC_INFO << "xyz contextList: " << pp;
        }
         */
        if (filter.isEmpty()) {
            //qDebug() << Q_FUNC_INFO << "xyz filter.isEmpty";
            contexts = contextList;
        } else {
            //qDebug() << Q_FUNC_INFO << "xyz filter not empty";

            contexts.clear();
            QString f(filter);
            f.remove(' ').remove('\t');
            if (f[0] == '!') {
                QStringList blackList = f.remove(0,1).split(',', QString::SkipEmptyParts);
                foreach (QString path, contextList) {
                    if (!blackList.contains(contextTypes.value(path)))
                        contexts.append(path);
                }
            } else {
                QStringList whiteList = f.split(',', QString::SkipEmptyParts);
                foreach (QString path, contextList) {
                    if (whiteList.contains(contextTypes.value(path)))
                        contexts.append(path);
                }
            }
        }
    }
}

QOfonoConnectionManager::QOfonoConnectionManager(QObject *parent) :
    QObject(parent),
    d_ptr(new QOfonoConnectionManagerPrivate)
{
	qDebug() << Q_FUNC_INFO << "xyz";
	observe("org.ofono", QDBusConnection::systemBus());
	m_available = QDBusConnection::systemBus().interface()->isServiceRegistered("org.ofono");
	qDebug() << Q_FUNC_INFO << "xyz org.ofono available:" << m_available;
	m_available = QDBusConnection::systemBus().interface()->isServiceRegistered("org.ofono.NetworkRegistration");
	qDebug() << Q_FUNC_INFO << "xyz org.ofono.NetworkRegistration available:" << m_available;
/*
	QStringList ls = QDBusConnection::systemBus().interface()->registeredServiceNames();
        foreach (QString pp, ls) {
    	    qDebug() << Q_FUNC_INFO << "xyz registered service: " << pp;
        }
*/
}

QOfonoConnectionManager::~QOfonoConnectionManager()
{
qDebug() << Q_FUNC_INFO << "xyz";
    delete d_ptr;
}

void QOfonoConnectionManager::onServiceOwnerChanged(const QString & serviceName, const QString & oldOwner, const QString & newOwner)
{
    qDebug() << Q_FUNC_INFO << "xyz service:" << serviceName << " oldowner:" << oldOwner << "newowner:" << newOwner;
}

void QOfonoConnectionManager::onServiceRegistered(const QString & serviceName)
{
    qDebug() << Q_FUNC_INFO << "xyz service:" << serviceName;
}

void QOfonoConnectionManager::onServiceUnregistered(const QString & serviceName)
{
    qDebug() << Q_FUNC_INFO << "xyz service:" << serviceName;
}

void QOfonoConnectionManager::observe(const QString & service, const QDBusConnection & conn)
{
    qDebug() << Q_FUNC_INFO << "xyz >>> service:" << service;
// todo: monta kutsua, poista vanha instanssi
    QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(service,conn,
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration/*& QDBusServiceWatcher::WatchForUnregistration & QDBusServiceWatcher::WatchForOwnerChange*/,this);
    bool ret;
    ret = connect(serviceWatcher,
            SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            SLOT(onServiceOwnerChanged(QString,QString,QString)));
    qDebug() << "xyz serviceOwnerChanged ret" << ret;
    ret = connect(serviceWatcher,
                SIGNAL(serviceRegistered(QString)),
                SLOT(onServiceRegistered(QString)));
    qDebug() << "xyz serviceRegistered ret" << ret;

    ret = connect(serviceWatcher,
                SIGNAL(serviceUnregistered(QString)),
                SLOT(onServiceUnregistered(QString)));
    qDebug() << "xyz serviceUnregistered ret" << ret;

    qDebug() << Q_FUNC_INFO << "xyz <<<";
}

void QOfonoConnectionManager::setModemPath(const QString &path)
{
qDebug() << Q_FUNC_INFO << "xyz >>> setModemPath, path:" << path;

    if (path == d_ptr->modemPath ||
            path.isEmpty())
        return;

    QStringList removedProperties = d_ptr->properties.keys();

    delete d_ptr->connman;
    d_ptr->connman = new OfonoConnectionManager("org.ofono", path, QDBusConnection::systemBus(),this);

    QDBusError err = d_ptr->connman->lastError();
    qDebug() << Q_FUNC_INFO << "xyz error isValid:" <<err.isValid() <<" message:" <<err.message() <<" nanme:" << err.name() <<" type:" <<err.type();

    qDebug() << Q_FUNC_INFO << "xyz connman->isValid(): " << d_ptr->connman->isValid() <<" connection().isConnected():" << d_ptr->connman->connection().isConnected() <<" interface:" << d_ptr->connman->interface();
    d_ptr->connman->dumpObjectInfo();
    if (d_ptr->connman->isValid()) {

        d_ptr->modemPath = path;
        bool res;///
        res = connect(d_ptr->connman,SIGNAL(PropertyChanged(QString,QDBusVariant)),
                this,SLOT(propertyChanged(QString,QDBusVariant)));
        qDebug() << Q_FUNC_INFO << "xyz connect to PropertyChanged:" << res;///

        res = connect(d_ptr->connman,SIGNAL(ContextAdded(QDBusObjectPath,QVariantMap)),
                this,SLOT(onContextAdd(QDBusObjectPath,QVariantMap)));
        qDebug() << Q_FUNC_INFO << "xyz connect to ContextAdded:" << res;///

        res = connect(d_ptr->connman,SIGNAL(ContextRemoved(QDBusObjectPath)),
                this,SLOT(onContextRemove(QDBusObjectPath)));
        qDebug() << Q_FUNC_INFO << "xyz connect to ContextRemoved:" << res;///

        QVariantMap properties = d_ptr->connman->GetProperties().value();
        for (QVariantMap::ConstIterator it = properties.constBegin();
             it != properties.constEnd(); ++it) {
            updateProperty(it.key(), it.value());
            removedProperties.removeOne(it.key());
        }
        /*
        if (d_ptr->startup) {
        	d_ptr->startup = false;
        	qDebug() << Q_FUNC_INFO << "xyz starting to sleep";
        	QThread::msleep(5000);
                qDebug() << Q_FUNC_INFO << "xyz stopping sleeping";

        }
        */
        d_ptr->getContexts();
        qDebug() << Q_FUNC_INFO << "xyz Q_EMIT modemPathChanged: " << path;
        Q_EMIT modemPathChanged(path);

        qDebug() << Q_FUNC_INFO << "xyz Q_EMIT contextsChanged: " << d_ptr->contexts;
        Q_EMIT contextsChanged(d_ptr->contexts);
    }

    qDebug() << Q_FUNC_INFO << "xyz iterating removed properties";

    foreach (const QString &p, removedProperties)
        updateProperty(p, QVariant());
    qDebug() << Q_FUNC_INFO << "xyz <<< setModemPath, path:" << path;

}

QString QOfonoConnectionManager::modemPath() const
{
    return d_ptr->modemPath;
}

void QOfonoConnectionManager::deactivateAll()
{
    if (!d_ptr->connman)
        return;
    d_ptr->connman->DeactivateAll();
}

void QOfonoConnectionManager::addContext(const QString &type)
{
qDebug() << Q_FUNC_INFO << "xyz type: " << type;

    if (!d_ptr->connman)
        return;

    QStringList allowedTypes;
    allowedTypes << "internet";
    allowedTypes << "mms";
    allowedTypes << "wap";
    allowedTypes << "ims";

    if(!allowedTypes.contains(type)) {
        Q_EMIT reportError("Type not allowed");
        return;
    }
    QDBusPendingReply<QDBusObjectPath> reply = d_ptr->connman->AddContext(type);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(addContextFinished(QDBusPendingCallWatcher*)));
}

void QOfonoConnectionManager::removeContext(const QString &path)
{
qDebug() << Q_FUNC_INFO << "xyz path: " << path;

    if (!d_ptr->connman)
        return;
    QDBusPendingReply<> reply = d_ptr->connman->RemoveContext(QDBusObjectPath(path));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(removeContextFinished(QDBusPendingCallWatcher*)));
}

bool QOfonoConnectionManager::attached() const
{
    if (d_ptr->connman)
        return d_ptr->properties["Attached"].value<bool>();
    else
        return false;
}

QString QOfonoConnectionManager::bearer() const
{
    if (d_ptr->connman)
        return d_ptr->properties["Bearer"].value<QString>();
    else
        return QString();
}

bool QOfonoConnectionManager::suspended() const
{
    if (d_ptr->connman)
        return d_ptr->properties["Suspended"].value<bool>();
    else
        return false;
}


bool QOfonoConnectionManager::roamingAllowed() const
{
    if (d_ptr->connman)
        return d_ptr->properties["RoamingAllowed"].value<bool>();
    else
        return false;
}

void QOfonoConnectionManager::setRoamingAllowed(bool value)
{
    if (roamingAllowed() == value)
        return;

    QString str("RoamingAllowed");
    QDBusVariant var(value);
    setOneProperty(str,var);
}

bool QOfonoConnectionManager::powered() const
{
    if (d_ptr->connman)
        return d_ptr->properties["Powered"].value<bool>();
    else
        return false;
}

void QOfonoConnectionManager::setPowered(bool value)
{
    if (powered() == value)
        return;

    QString str("Powered");
    QDBusVariant var(value);
    setOneProperty(str,var);
}

void QOfonoConnectionManager::propertyChanged(const QString& property, const QDBusVariant& dbusvalue)
{
qDebug() << Q_FUNC_INFO << "xyz property: " << property << " value: " << dbusvalue.variant();

    updateProperty(property, dbusvalue.variant());
}

void QOfonoConnectionManager::updateProperty(const QString &property, const QVariant &value)
{
qDebug() << Q_FUNC_INFO << "xyz property: " << property << " value: " << value;

    if (d_ptr->properties.value(property) == value)
        return;

    if (value.isValid())
        d_ptr->properties.insert(property, value);
    else
        d_ptr->properties.remove(property);

    if (property == QLatin1String("Attached")) {
	//d_ptr->getContexts();
	qDebug() << Q_FUNC_INFO << "xyz Q_EMIT contextsChanged: " << d_ptr->contexts;
	Q_EMIT contextsChanged(d_ptr->contexts);
	qDebug() << Q_FUNC_INFO << "xyz Q_EMIT attachedChanged: " << value.value<bool>();
        Q_EMIT attachedChanged(value.value<bool>());
    } else if (property == QLatin1String("Bearer")) {
	qDebug() << Q_FUNC_INFO << "xyz Q_EMIT bearerChanged: " << value.value<QString>();
        Q_EMIT bearerChanged(value.value<QString>());
    } else if (property == QLatin1String("Suspended")) {
	    qDebug() << Q_FUNC_INFO << "xyz Q_EMIT suspendedChanged: " << value.value<bool>();
        Q_EMIT suspendedChanged(value.value<bool>());
    } else if (property == QLatin1String("RoamingAllowed")) {
	qDebug() << Q_FUNC_INFO << "xyz Q_EMIT roamingAllowedChanged: " << value.value<bool>();
        Q_EMIT roamingAllowedChanged(value.value<bool>());
    } else if (property == QLatin1String("Powered")) {
	qDebug() << Q_FUNC_INFO << "xyz Q_EMIT poweredChanged: " << value.value<bool>();
        Q_EMIT poweredChanged(value.value<bool>());
    }
}

QStringList QOfonoConnectionManager::contexts()
{
    return d_ptr->contexts;
}

void QOfonoConnectionManager::onContextAdd(const QDBusObjectPath &path, const QVariantMap &propertyMap)
{
qDebug() << Q_FUNC_INFO << "xyz path: " << path.path();

    Q_UNUSED(propertyMap);
    d_ptr->contextTypes.insert(path.path(), propertyMap.value("Type").toString());
    d_ptr->filterContexts();
    qDebug() << Q_FUNC_INFO << "xyz Q_EMIT contextAdded: " << path.path();
    Q_EMIT contextAdded(path.path());
    qDebug() << Q_FUNC_INFO << "xyz Q_EMIT contextsChanged: " << d_ptr->contexts;
    Q_EMIT contextsChanged(d_ptr->contexts);
}

void QOfonoConnectionManager::onContextRemove(const QDBusObjectPath &path)
{
qDebug() << Q_FUNC_INFO << "xyz path: " << path.path();

    d_ptr->contextTypes.remove(path.path());
    d_ptr->filterContexts();
    qDebug() << Q_FUNC_INFO << "xyz Q_EMIT contextRemoved: " << path.path();
    Q_EMIT contextRemoved(path.path());
    qDebug() << Q_FUNC_INFO << "xyz Q_EMIT contextsChanged: " << d_ptr->contexts;
    Q_EMIT contextsChanged(d_ptr->contexts);
}

QString QOfonoConnectionManager::filter() const
{
    return d_ptr->filter;
}

void QOfonoConnectionManager::setFilter(const QString &filter)
{
    if (d_ptr->filter != filter) {
        d_ptr->filter = filter;
        d_ptr->filterContexts();
        qDebug() << Q_FUNC_INFO << "xyz Q_EMIT filterChanged: " << filter;

        Q_EMIT filterChanged(filter);
        qDebug() << Q_FUNC_INFO << "xyz Q_EMIT contextsChanged: " << d_ptr->contexts;
        Q_EMIT contextsChanged(d_ptr->contexts);
    }
}

bool QOfonoConnectionManager::isValid() const
{
    return d_ptr->connman->isValid();
}

void QOfonoConnectionManager::setOneProperty(const QString &prop, const QDBusVariant &var)
{
    if (d_ptr->connman) {
        QDBusPendingReply <> reply = d_ptr->connman->SetProperty(prop,var);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(setPropertyFinished(QDBusPendingCallWatcher*)));
    }
}

void QOfonoConnectionManager::addContextFinished(QDBusPendingCallWatcher *watch)
{
    watch->deleteLater();
    QDBusPendingReply<QDBusObjectPath> reply = *watch;
    if (reply.isError()) {
        //qDebug() << Q_FUNC_INFO << reply.error();
        qDebug() << Q_FUNC_INFO << "xyz Q_EMIT reportError: " << reply.error();
        Q_EMIT reportError(reply.error().message());
    }
}

void QOfonoConnectionManager::removeContextFinished(QDBusPendingCallWatcher *watch)
{
    watch->deleteLater();
    QDBusPendingReply<> reply = *watch;
    if (reply.isError()) {
        //qDebug() << Q_FUNC_INFO << reply.error();
        qDebug() << Q_FUNC_INFO << "xyz Q_EMIT reportError: " << reply.error();
        Q_EMIT reportError(reply.error().message());
    }
}

void QOfonoConnectionManager::setPropertyFinished(QDBusPendingCallWatcher *watch)
{
    watch->deleteLater();
    QDBusPendingReply<> reply = *watch;
    if (reply.isError()) {
        //qDebug() << Q_FUNC_INFO << reply.error();
        qDebug() << Q_FUNC_INFO << "xyz Q_EMIT reportError: " << reply.error();
        Q_EMIT reportError(reply.error().message());
    }
}

