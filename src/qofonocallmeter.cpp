/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: lorn.potter@jollamobile.com
**
**
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qofonocallmeter.h"
#include "dbus/ofonocallmeter.h"

class QOfonoCallMeterPrivate
{
public:
    QOfonoCallMeterPrivate();
    QString modemPath;
    OfonoCallMeter *callMeter;
    QVariantMap properties;

};

QOfonoCallMeterPrivate::QOfonoCallMeterPrivate() :
    modemPath(QString())
  , callMeter(0)
{
}

QOfonoCallMeter::QOfonoCallMeter(QObject *parent) :
    QObject(parent)
  , d_ptr(new QOfonoCallMeterPrivate)
{
}

QOfonoCallMeter::~QOfonoCallMeter()
{
    delete d_ptr;
}

void QOfonoCallMeter::setModemPath(const QString &path)
{
    if (!d_ptr->callMeter) {
        d_ptr->modemPath = path;
        d_ptr->callMeter = new OfonoCallMeter("org.ofono", path, QDBusConnection::systemBus(),this);

        if (d_ptr->callMeter) {
            connect(d_ptr->callMeter,SIGNAL(PropertyChanged(QString,QDBusVariant)),
                    this,SLOT(propertyChanged(QString,QDBusVariant)));

            connect(d_ptr->callMeter,SIGNAL(NearMaximumWarning()),this,SIGNAL(nearMaximumWarning()));
            QDBusReply<QVariantMap> reply;
            reply = d_ptr->callMeter->GetProperties();
            d_ptr->properties = reply.value();
        }
    }
}

QString QOfonoCallMeter::modemPath() const
{
    return d_ptr->modemPath;
}


void QOfonoCallMeter::propertyChanged(const QString& property, const QDBusVariant& dbusvalue)
{
    QVariant value = dbusvalue.variant();
    d_ptr->properties.insert(property,value);

    if (property == QLatin1String("AccumulatedCallMeterMaximum")) {
        Q_EMIT accumulatedCallMeterMaximumChanged(value.value<quint32>());
    } else if (property == QLatin1String("PricePerUnit")) {
        Q_EMIT pricePerUnitChanged(value.value<qreal>());
    }
}

quint32 QOfonoCallMeter::callMeter() const
{
    if (d_ptr->callMeter)
        return d_ptr->properties["CallMeter"].value<quint32>();
    else
        return 0;
}

quint32 QOfonoCallMeter::accumulatedCallMeter() const
{
    if (d_ptr->callMeter)
        return d_ptr->properties["AccumulatedCallMeter"].value<quint32>();
    else
        return 0;
}

quint32 QOfonoCallMeter::accumulatedCallMeterMaximum() const
{
    if (d_ptr->callMeter)
        return d_ptr->properties["AccumulatedCallMeterMaximum"].value<quint32>();
    else
        return 0;
}

void QOfonoCallMeter::setAccumulatedCallMeterMaximum(quint32 max, const QString &password)
{
    if (d_ptr->callMeter) {
        QVariantList arguments;
        arguments << QVariant(max);
        d_ptr->callMeter->SetProperty("AccumulatedCallMeterMaximum",QDBusVariant(arguments), password);
    }
}


qreal QOfonoCallMeter::pricePerUnit() const
{
    if (d_ptr->callMeter)
        return d_ptr->properties["PricePerUnit"].value<qreal>();
    else
        return 0;
}

void QOfonoCallMeter::setPricePerUnit(qreal unit, const QString &password)
{
    if (d_ptr->callMeter) {
        QVariantList arguments;
        arguments << QVariant(unit);
        d_ptr->callMeter->SetProperty("PricePerUnit",QDBusVariant(arguments), password);
    }
}

QString QOfonoCallMeter:: currency() const
{
    if (d_ptr->callMeter)
        return d_ptr->properties["Currency"].value<QString>();
    else
        return QString();
}

void QOfonoCallMeter::reset(const QString &password)
{
    if (d_ptr->callMeter)
        d_ptr->callMeter->Reset(password);
}
