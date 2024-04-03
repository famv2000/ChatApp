#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QWidget>

#include <QTcpSocket>
#include <QTcpServer>
#include <QFile>
#include <QTime>

namespace Ui {
class TcpServer;
}

class TcpServer : public QWidget
{
    Q_OBJECT

public:
    explicit TcpServer(QWidget *parent, QString filePath, QHostAddress ip, qint16 port);
    ~TcpServer();

    void appendTextBrowser(Qt::GlobalColor color, QString text);
    virtual void closeEvent(QCloseEvent* event);

private:
    Ui::TcpServer *ui;
    QHostAddress ip;  // IP cục bộ (Máy chủ)
    qint16 port;  // Cổng gốc (Máy chủ)

    QTcpSocket* tcpSocket;  // Ổ cắm nghe
    QTcpServer* tcpServer;  // ổ cắm truyền thông

    QFile file;
    QString fileName;
    qint64 fileSize;
    qint64 bytesAlreadySend;  // Bao nhiêu dữ liệu đã được gửi đi?

    QTimer* timer;

    QMovie* movie;  // GIF

    void sendData();
};

#endif // TCPSERVER_H
