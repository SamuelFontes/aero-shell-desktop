#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyle>
#include <QDebug>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    qInfo() << "Native application style:" << app.style()->objectName()
            << "base:" << app.palette().color(QPalette::Base).name()
            << "icons:" << QIcon::themeName();
    // Loading libkvantum alone also succeeds with its built-in fallback theme.
    // Verify the original light theme's actual application palette as well.
    return app.style()->objectName().contains(QStringLiteral("kvantum"), Qt::CaseInsensitive)
        && app.palette().color(QPalette::Base) == QColor(Qt::white)
        && QIcon::themeName() == QStringLiteral("Windows 7 Aero") ? 0 : 1;
}
