#include <QApplication>

#include "ApiClient.h"
#include "StartWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // One shared API client for the whole app
    ApiClient api;
    api.setBaseUrl("http://localhost:3000");  // backend base URL

    StartWindow w(&api);
    w.show();

    return a.exec();
}