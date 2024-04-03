#ifndef CHATBOXWIDGET_H
#define CHATBOXWIDGET_H
#include "signaltype.h"

#include <QWidget>
#include <QTimer>
#include <QUdpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class ChatBoxWidget; }
QT_END_NAMESPACE

class ChatBoxWidget : public QWidget
{
    Q_OBJECT

public:
    ChatBoxWidget(QWidget* parent, QString name, qint16 port);
    ~ChatBoxWidget();

    void sendUDPSignal(const SignalType type);  // Phát tin nhắn UDP
    QString getAndCleanMsg();

    void userJoin(QString name, QString groupNumber, QHostAddress ip);  // Xử lý việc tham gia của người dùng
    void userLeft(QString name, QString time);  // Xử lý việc người dùng rời đi

    virtual void closeEvent(QCloseEvent*);  // [Rewrite] Sự kiện kết thúc kích hoạt

signals:
    void signalClose();  // [Tín hiệu tùy chỉnh] Nếu cửa sổ đóng, hãy gửi tín hiệu đóng

private:
    void receiveUDPMessage();          // Nhận tin nhắn UDP
    void openURL(const QUrl &url);

private:
    Ui::ChatBoxWidget* ui;

    QString lastSendFilePath;

    QUdpSocket* udpSocketOnPortChatList;
    QUdpSocket* udpSocketOnPortChatBox;
    QString name;
    qint16 port;
};
#endif // CHATBOXWIDGET_H
