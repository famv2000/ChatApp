#include "uil_chatlist.h"
#include "ui_chatlist.h"
#include "uil_addchat.h"
#include "dal_chatApp.h"
#include "signaltype.h"
#include "uil_chatboxwidget.h"

#include <QMessageBox>


ChatList::ChatList(QWidget* parent, QString localUserName, QString localUserGroupNumber, QHostAddress localIpAddress) :
    QWidget(parent),
    ui(new Ui::ChatList)
{
    ui->setupUi(this);
    this->setWindowTitle("ChatApp");
    this->setWindowIcon(QIcon(":/icon/icons/iconWindow.png"));


    /* Init data on UI */
    ui->lbName->setText(localUserName);
    ui->lbGroupNumber->setText(localUserGroupNumber);
    ui->lbIP->setText(localIpAddress.toString());

    /* Nhận (nghe) tin nhắn */
    this->udpSocket = new QUdpSocket(this);
    this->udpSocket->bind(DAL::getPortChatList(),  // Số cổng ràng buộc
                          QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);  // Địa chỉ dùng chung + ngắt kết nối và kết nối lại
    connect(udpSocket, &QUdpSocket::readyRead,
            this, &ChatList::receiveMessage);


    /* Nhấn vào nút Thêm nhóm trò chuyện sẽ trả về tín hiệu tạo tên trò chuyện nhóm. */
    connect(ui->btnNewChat, &QToolButton::clicked,
            this, [=](){
                AddChat* addChat = new AddChat(nullptr);
                addChat->show();

                connect(addChat, &AddChat::addNewChat,
                        this, [=](const QString &name){

                            if (isChatExist(name))
                            {
                                QMessageBox::warning(this, "Warning", "Chat with the same name already exists");
                                return;
                            }

                            /* Nếu các điều kiện được đáp ứng, hãy tạo một cuộc trò chuyện nhóm và đưa nó vào hồ sơ cục bộ */
                            qint16 port = getRandomPort();
                            QToolButton* btn = getNewBtn(name, port, true);
                            Chat* chat = new Chat(name, port, true);
                            this->vPair_OChat_BtnChat.push_back(QPair<Chat*, QToolButton*>(chat, btn));
                            addBtnChatInLayout(btn);

                            /* Nếu đáp ứng đủ điều kiện, hãy thêm cửa sổ trò chuyện mới */
                            ChatBoxWidget* chatBoxWidget = new ChatBoxWidget(nullptr, name, port);
                            chatBoxWidget->show();

                            /*Hộp thoại đóng trò chuyện sẽ đặt lại mảng xem nó có mở hay không. (Nếu nhận được tín hiệu đóng cửa sổ, XXX)*/
                            connect(chatBoxWidget, &ChatBoxWidget::signalClose,
                                    this, [=](){ setChatState(name, false); });
                        });
            });


    /* Thay đổi trạng thái hộp tìm kiếm */
    connect(ui->leSearch, &QLineEdit::textChanged,
            this, [=](){
                /* Nếu nội dung trong hộp văn bản trống, tất cả các nút trò chuyện sẽ được hiển thị */
                if (ui->leSearch->text().isEmpty())
                {
                    for (auto i : this->vPair_OChat_BtnChat)
                    {
                        i.second->show();
                    }
                    return;
                }

                for (auto i : this->vPair_OChat_BtnChat)
                {
                    QString textOnBtn = i.second->text();
                    if (isNeedHideBtn(textOnBtn)) i.second->hide();
                    else i.second->show();
                }
            });

}

ChatList::~ChatList()
{
    delete ui;
}

/** Cập nhật danh sách trò chuyện trong cửa sổ
 * @brief addBtnChat
 */
void ChatList::addBtnChatInLayout(QToolButton* btn)
{
    if (nullptr == btn) return;

    /* Hiển thị hoặc ẩn các nút dựa trên nội dung trong hộp tìm kiếm */
    if (isNeedHideBtn(btn->text())) btn->hide();

    ui->vLayout->addWidget(btn); // Thêm vào bố cục dọc
}


/* Nhận và phân tích tin nhắn UDP */
void ChatList::receiveMessage()
{
    /* Nhận gói dữ liệu */
    qint64 size = udpSocket->pendingDatagramSize();
    QByteArray byteArrayGetUDP = QByteArray(size, 0);
    udpSocket->readDatagram(byteArrayGetUDP.data(), size);

    /** Dữ liệu phân tích
     * Đoạn 1: Kiểu dữ liệu
     * Đoạn 2: Tên nhóm gửi tín hiệu này
     * Đoạn 3: Số cổng nhóm gửi tín hiệu này
     * Đoạn 4: Tên người dùng hiện tại (cục bộ) localUserName
     * Đoạn 5: Số nhóm người dùng hiện tại (cục bộ) localUserGroupNumber
     * Đoạn 6: Người dùng hiện tại (cục bộ) ip localIpAddress
     * Đoạn 7: Nội dung cụ thể
     */
    SignalType      signalType_1          ;
    QString         chatName_2            ;
    qint16          chatPort_3            ;
    QString         localUserName_4       ;
    QString         localUserGroupNumber_5;
    QHostAddress    localIpAddress_6      ;
    QString         msg_7                 ;

    QDataStream dataStream(&byteArrayGetUDP, QIODevice::ReadOnly);

    dataStream >> signalType_1          ;
    dataStream >> chatName_2            ;
    dataStream >> chatPort_3            ;
    dataStream >> localUserName_4       ;
    dataStream >> localUserGroupNumber_5;
    dataStream >> localIpAddress_6      ;
    dataStream >> msg_7                 ;

    switch (signalType_1)
    {
    /* Nhận tín hiệu ChatBox tồn tại */
    case SignalType::ChatExist:
    {
        /* Nếu nó không tồn tại trong bản ghi cục bộ, điều đó có nghĩa đó là một cuộc trò chuyện nhóm khác trên mạng LAN
         * và không được có nút nào trong cửa sổ này. */
        if (!isChatExist(chatName_2))
        {
            Chat* chat = new Chat(chatName_2, chatPort_3, false);
            QToolButton* btn = getNewBtn(chatName_2, chatPort_3, false);
            this->vPair_OChat_BtnChat.push_back(QPair<Chat*, QToolButton*>(chat, btn));
            addBtnChatInLayout(btn);
        }
    }
    break;

    case SignalType::ChatDestory:
        /* Xóa các bản ghi và nút cục bộ */
        for (int i = 0; i < this->vPair_OChat_BtnChat.size(); i++)
        {
            if (chatName_2 == this->vPair_OChat_BtnChat.at(i).first->name)
            {
                this->vPair_OChat_BtnChat.at(i).second->close();
                this->vPair_OChat_BtnChat.removeAt(i);
            }
        }
        break;

    default:
        break;
    }
}

/** Tìm xem cuộc trò chuyện nhóm có tên đã tồn tại chưa
 * @brief isChatExist
 * @param name
 * @return Trả về true nếu tồn tại
 */
bool ChatList::isChatExist(const QString &name)
{
    //    for (auto i : this->vPair_OChat_BtnChat)
    //    {
    //        if (name == i.first->name) return true;
    //    }
    //    return false;  Sử dụng thuật toán std::any_of từ thư viện chuẩn thay vì vòng lặp ban đầu

    return std::any_of(
        this->vPair_OChat_BtnChat.begin(),
        this->vPair_OChat_BtnChat.end(),
        [&](auto pair) { return name == pair.first->name; });
}


/** Khi cuộc trò chuyện đã được ghi lại (tồn tại) thì nút này không tồn tại trong vPair_OChat_BtnChat của điều này
 * @brief ChatList::isBtnChatInVector
 * @param name
 * @return
 */
#if ENABLE_UNUSED_FUNCTION
bool ChatList::isChatExist_But_BtnNotExist(const QString &name)
{
    if (!isChatExist(name)) return false;

    //    for (auto i: this->vPair_OChat_BtnChat)
    //    {
    //        if ((name == i.first->name) && (nullptr == i.second)) return true;
    //    }
    //    return false;
    // sử dụng std::any_of Thuật toán tìm phần tử thỏa mãn điều kiện
    return std::any_of(this->vPair_OChat_BtnChat.begin(), this->vPair_OChat_BtnChat.end(),
                       [&](const auto &pair) { return (name == pair.first->name) && (nullptr == pair.second); });
}
#endif

/** Tìm xem số cổng có bị chiếm dụng không
 * @brief isPortExist
 * @param port
 * @return
 */
bool ChatList::isPortExist(const qint16 port)
{
    if (vPair_OChat_BtnChat.isEmpty()) return false;
    //    for (auto i : vPair_OChat_BtnChat)
    //    {
    //        if ((port == i.first->port) || (port == PORT_CHAT_LIST) || (port == PORT_TCP_FILE)) return true;
    //    }
    //    return false;

    // sử dụng std::any_of Thuật toán tìm phần tử thỏa mãn điều kiện
    return std::any_of(vPair_OChat_BtnChat.begin(), vPair_OChat_BtnChat.end(),
                       [&](const auto &pair) { return (port == pair.first->port) || (port == PORT_CHAT_LIST) || (port == PORT_TCP_FILE); });
}

/** Nhận số cổng ngẫu nhiên duy nhất
 * @brief getRandomPort
 * @return
 */
qint16 ChatList::getRandomPort()
{
    while (true)
    {
        qint16 port = QRandomGenerator::global()->bounded(PORT_MIN, PORT_MAX);
        if (!isPortExist(port)) return port;
    }
}


bool ChatList::isChatOpen(const QString &name)
{
    //    for (auto i : vPair_OChat_BtnChat)
    //    {
    //        if (name == i.first->name) return i.first->isOpen;
    //    }
    //    return true;  // FIX!

    // Sử dụng thuật toán std::find_if để tìm xem có phần tử nào đáp ứng điều kiện không
    auto it = std::find_if(vPair_OChat_BtnChat.begin(), vPair_OChat_BtnChat.end(),
                           [&](const auto &pair) { return name == pair.first->name; });

    // Nếu tìm thấy phần tử thỏa mãn điều kiện, trả về thuộc tính isOpen của nó
    // Ngược lại trả về giá trị mặc định true
    return (it != vPair_OChat_BtnChat.end()) ? it->first->isOpen : true;
}

/** Tạo và trả về một đối tượng nút
 * @brief ChatList::getNewBtn
 * @param btn_text
 * @param port
 * @param isOpen
 * @return
 */
QToolButton* ChatList::getNewBtn(QString btn_text, qint16 port, bool isOpen)
{
    /* Đặt hình đại diện */
    QToolButton* btn = new QToolButton;
    //    btn->setText(QString("[%1] %2").arg(chatPort).arg(chatName));
    btn->setText(btn_text);
    btn->setIcon(QIcon(":/icon/icons/user-group.png"));
    btn->setAutoRaise(true);  // Nút kiểu trong suốt
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);  // Đặt biểu tượng và văn bản hiển thị
    btn->setAttribute(Qt::WA_DeleteOnClose);
    btn->setFixedSize(220, 50);

    /* Nút để thêm tín hiệu và khe cắm */
    connect(btn, &QToolButton::clicked,this,
            [=](){
                /* Nếu cửa sổ đã mở rồi thì đừng thực hiện lại */
                if (isChatOpen(btn_text))
                {
                    QMessageBox::warning(this, "Warning", QString("Chat %1 already open").arg(btn_text));
                    return;
                }

                /* Nếu đáp ứng đủ điều kiện, hãy thêm cửa sổ trò chuyện mới */
                ChatBoxWidget* chatBoxWidget = new ChatBoxWidget(nullptr, btn_text, port);
                chatBoxWidget->setAttribute(Qt::WA_DeleteOnClose);
                chatBoxWidget->setWindowIcon(QIcon(":/icon/icons/user-group.png"));
                chatBoxWidget->show();

                setChatState(btn_text, true);

                /* Hộp thoại đóng trò chuyện sẽ đặt lại mảng xem nó có mở hay không. (Nếu nhận được tín hiệu đóng cửa sổ, XXX) */
                connect(chatBoxWidget, &ChatBoxWidget::signalClose,
                        this, [=](){ setChatState(btn_text, false); });
            });

    return btn;
}


/** Đặt cửa sổ trò chuyện để mở hoặc đóng
 * @brief setChatState
 * @param name
 * @param state
 * @return Trả về true nếu cài đặt thành công, false nếu không.
 */
bool ChatList::setChatState(const QString &name, bool state)
{
    //    for (auto i : this->vPair_OChat_BtnChat)
    //    {
    //        if (name == i.first->name)
    //        {
    //            i.first->isOpen = state;
    //            return true;
    //        }
    //    }
    //    return false;
    // sử dụng std::find_if Thuật toán tìm phần tử thỏa mãn điều kiện
    auto it = std::find_if(this->vPair_OChat_BtnChat.begin(), this->vPair_OChat_BtnChat.end(),
                           [&](const auto &pair) { return name == pair.first->name; });

    // Nếu tìm thấy phần tử thỏa mãn điều kiện, đặt thuộc tính isOpen của nó và trả về true
    // Ngược lại trả về false
    if (it != this->vPair_OChat_BtnChat.end())
    {
        it->first->isOpen = state;
        return true;
    }
    return false;
}

#if ENABLE_UNUSED_FUNCTION
bool ChatList::updateBtnInvPair(const QString &name, QToolButton* btn)
{
    //    for (auto i : this->vPair_OChat_BtnChat)
    //    {
    //        if (name == i.first->name)
    //        {
    //            i.second = btn;
    //            return true;
    //        }
    //    }
    // Sử dụng thuật toán std::find_if để tìm xem có phần tử nào đáp ứng điều kiện không
    auto it = std::find_if(this->vPair_OChat_BtnChat.begin(), this->vPair_OChat_BtnChat.end(),
                           [&](const auto &pair) { return name == pair.first->name; });

    // Nếu tìm thấy phần tử thỏa mãn điều kiện, hãy cập nhật nút tương ứng của nó và trả về true
    // Ngược lại, xuất thông tin lỗi và trả về false
    if (it != this->vPair_OChat_BtnChat.end())
    {
        it->second = btn;
        return true;
    }

    qDebug() << "[ERROR] Fail to update btn, ChatBox named" << name << "do not exits in local vPair_OChat_BtnChat";
    return false;
}
#endif

/** Dựa vào nội dung trong hộp tìm kiếm và sử dụng biểu thức chính quy, xác định xem nút có cần ẩn hay không
 * @brief isNeedHideBtn
 * @param btnText
 * @return true Đại diện cần phải được ẩn
 */
bool ChatList::isNeedHideBtn(QString textOnBtn)
{
    /* Nếu nội dung trong text box trống thì không cần ẩn đi */
    if (ui->leSearch->text().isEmpty()) return false;

    QString strRegExp("\\S*" + ui->leSearch->text() + "\\S*");
    QRegularExpression regExp;
    regExp.setPattern(strRegExp);

    if (!regExp.match(textOnBtn).hasMatch()) return true;
    else return false;
}
