#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    if (QIcon::themeName().isEmpty()) {
        QIcon::setThemeName("Papirus");
    }

    MainWindow window;
    window.show();

    return app.exec();
}
