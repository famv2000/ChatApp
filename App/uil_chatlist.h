#ifndef UIL_CHATLIST_H
#define UIL_CHATLIST_H

#include "chat.h"

#include <QWidget>
#include <QUdpSocket>
#include <QToolButton>
#include <QRegularExpression>

namespace Ui {
class ChatList;
}

class ChatList : public QWidget
{
    Q_OBJECT

public:
    explicit ChatList(QWidget *parent, QString localUserName, QString localUserGroupNumber, QHostAddress localIpAddress);
    ~ChatList();

#if !QT_NO_DEBUG  // Được sử dụng để mở quyền công khai cho các bài kiểm tra đơn vị
public:
#else
private:
#endif
    void receiveMessage();  // Nhận tin nhắn UDP

    bool isPortExist(const qint16 port);  // Tìm xem số cổng có bị chiếm dụng không
    bool isChatExist_But_BtnNotExist(const QString &name); // UNUSED
    bool isChatExist(const QString &name);  // Tìm xem cuộc trò chuyện nhóm có tên đã tồn tại chưa
    bool isChatOpen(const QString &name);

    void addBtnChatInLayout(QToolButton* btn);  // Thêm đối tượng nút vào Layout

    QToolButton* getNewBtn(QString btn_text, qint16 port, bool isOpen);
    qint16 getRandomPort();  // Nhận số cổng ngẫu nhiên duy nhất

    bool setChatState(const QString &name, bool state);  // Đặt cửa sổ trò chuyện để mở hoặc đóng
    bool updateBtnInvPair(const QString &name, QToolButton* btn); // UNUSED

    bool isNeedHideBtn(QString textOnBtn);  // Xác định xem nút có cần ẩn hay không dựa trên biểu thức chính quy


private:
    Ui::ChatList* ui;

    QUdpSocket* udpSocket;


#if !QT_NO_DEBUG  // Được sử dụng để mở quyền <public> cho các bài kiểm tra đơn vị
public:
#else
private:
#endif
    QVector<QPair<Chat*, QToolButton*>> vPair_OChat_BtnChat;  // Đầu tiên là đối tượng Trò chuyện và thứ hai là đối tượng Nút
};

#endif // UIL_CHATLIST_H
