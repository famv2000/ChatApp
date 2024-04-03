#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QTcpSocket>
#include <QFile>
#include <QTime>

namespace Ui {
class TcpClient;
}

class TcpClient : public QWidget
{
    Q_OBJECT

public:
    explicit TcpClient(QWidget* parent, const QString &fileName, qint64 fileSizeBytes, QHostAddress serverIp, qint16 serverPort);
    ~TcpClient();

    void connectTcpServerAndOpenFile();
    void receiveTcpDataAndSave();

    virtual void closeEvent(QCloseEvent* event);

private:
    void appendTextBrowser(Qt::GlobalColor color, QString text);

private:
    Ui::TcpClient *ui;
    QMovie* movie;  // GIF
    QTimer* timer_progressBar; // Bộ đếm thời gian cập nhật thanh tiến trình

    /* Các tham số được truyền vào */
    QString fileName;
    qint64 fileSizeBytes;
    QHostAddress serverIp;
    qint16 serverPort;

    QTcpSocket* tcpSocket;  // Ổ cắm nghe

    QFile file;  //đối tượng tập tin
    bool isHeaderReceived;  // Xác định xem có nhận được tiêu đề tệp hay không
    qint64 bytesReceived;   // Đã nhận được bao nhiêu dữ liệu
};

#endif // TCPCLIENT_H
