#include <QGuiApplication>
#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QBuffer>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>

// Reproducible Windows icon, generated only into the build tree from one SVG.
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (argc != 3) return 1;
    QSvgRenderer svg(QString::fromLocal8Bit(argv[1]));
    if (!svg.isValid()) return 2;
    const QList<int> sizes = {16,20,24,30,32,40,48,64,128,256};
    QList<QByteArray> pngs;
    for (int size : sizes) {
        QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        svg.render(&painter);
        painter.end();
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "PNG")) return 3;
        pngs.append(bytes);
    }
    const QString output = QString::fromLocal8Bit(argv[2]);
    if (!QDir().mkpath(QFileInfo(output).absolutePath())) return 4;
    QFile file(output);
    if (!file.open(QIODevice::WriteOnly)) return 5;
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint16(0) << quint16(1) << quint16(sizes.size());
    quint32 offset = 6 + 16*sizes.size();
    for (int index = 0; index < sizes.size(); ++index) {
        const int size = sizes.at(index);
        stream << quint8(size == 256 ? 0 : size) << quint8(size == 256 ? 0 : size)
               << quint8(0) << quint8(0) << quint16(1) << quint16(32)
               << quint32(pngs.at(index).size()) << offset;
        offset += pngs.at(index).size();
    }
    for (const auto &bytes : pngs) stream.writeRawData(bytes.constData(), bytes.size());
    return stream.status() == QDataStream::Ok ? 0 : 6;
}
