#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>

class ApiClient : public QObject
{
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);

signals:
};

#endif // APICLIENT_H
