#include <QApplication>
#include <QIcon>
#include <QDebug>
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    qDebug() << QIcon::themeName();
    return 0;
}
