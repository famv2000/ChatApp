#include "uil_chatboxwidget.h"
#include "ui_chatboxwidget.h"
#include "dal_chatApp.h"
#include "tcpserver.h"
#include "tcpclient.h"

#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QNetworkAddressEntry>

ChatBoxWidget::ChatBoxWidget(QWidget* parent, QString name, qint16 port)
    : QWidget(parent)
    , ui(new Ui::ChatBoxWidget),
    name(name),
    port(port)
{
    ui->setupUi(this);
    this->setWindowTitle(QString("[Chat] %1 on port %2").arg(name).arg(port));
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowIcon(QIcon(":/icon/icons/user-group.png"));

    /* Phát 8888 đến cùng một địa chỉ của tất cả các cửa sổ (nói với ChatList rằng cửa sổ này tồn tại) */
    this->udpSocketOnPortChatList = new QUdpSocket(this);
    this->udpSocketOnPortChatList->bind(DAL::getPortChatList(),
                                        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);  // Địa chỉ dùng chung + ngắt kết nối và kết nối lại


    /* Phát cổng ngẫu nhiên của riêng bạn tới tất cả các địa chỉ trong cửa sổ này */
    this->udpSocketOnPortChatBox = new QUdpSocket(this);
    this->udpSocketOnPortChatBox->bind(this->port,
                                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);  // Địa chỉ dùng chung + ngắt kết nối và kết nối lại
    connect(udpSocketOnPortChatBox, &QUdpSocket::readyRead,
            this, &ChatBoxWidget::receiveUDPMessage);


    /* Gửi thông tin về cửa sổ hiện tại mở mỗi mili giây BORDCAST_TIME_STEP */
    QTimer* timer = new QTimer(this);
    timer->start(BORDCAST_TIME_STEP);
    connect(timer, &QTimer::timeout,
            this, [=](){ sendUDPSignal(SignalType::ChatExist); });

    // Tạo một cửa sổ mới tương đương với việc người dùng mới nhập
    sendUDPSignal(SignalType::UserJoin);

    /* Bấm vào nút gửi để gửi tin nhắn */
    connect(ui->btnSend, &QPushButton::clicked,
            this, [=](){sendUDPSignal(SignalType::Msg);});


    /* Nhấp vào nút thoát để đóng cửa sổ */
    connect(ui->btnExit, &QPushButton::clicked,
            this, [=](){this->close();});

    //////////////////////////////////////////////// Khả năng tiếp cận ////////////////////////////////////////////////
    /* phông chữ */
    connect(ui->cbxFontType, &QFontComboBox::currentFontChanged,
            this, [=](const QFont& font){
                ui->msgTextEdit->setCurrentFont(font);
                ui->msgTextEdit->setFocus();
            });

    /* Cỡ chữ */
    void(QComboBox::* cbxSingal)(const QString &text) = &QComboBox::currentTextChanged;
    connect(ui->cbxFontSize, cbxSingal,
            this, [=](const QString &text){
                ui->msgTextEdit->setFontPointSize(text.toDouble());
                ui->msgTextEdit->setFocus();
            });

    /* In đậm */
    connect(ui->btnBold, &QToolButton::clicked,
            this, [=](bool isCheck){
                if (isCheck) ui->msgTextEdit->setFontWeight(QFont::Bold);
                else ui->msgTextEdit->setFontWeight(QFont::Normal);
            });

    /* nghiêng */
    connect(ui->btnItalic, &QToolButton::clicked,
            this, [=](bool isCheck){ ui->msgTextEdit->setFontItalic(isCheck);
            });


    /* gạch chân */
    connect(ui->btnUnderLine, &QToolButton::clicked,
            this, [=](bool isCheck){ ui->msgTextEdit->setFontUnderline(isCheck);
            });

    /* thay đổi màu sắc */
    connect(ui->btnColor, &QToolButton::clicked,
            this, [=](){
                QColor color = QColorDialog::getColor(Qt::black);
                ui->msgTextEdit->setTextColor(color);
            });

    /* Xóa cuộc trò chuyện */
    connect(ui->btnClean, &QToolButton::clicked,
            this, [=](){

                if (QMessageBox::Ok ==
                    QMessageBox::question(this,
                                          "Clean all message",
                                          "Are you sure you want to clear all messages?",
                                          QMessageBox::Ok | QMessageBox::Cancel,
                                          QMessageBox::Cancel))
                {
                    ui->msgTextBrowser->clear();
                }});

    /* Lưu lịch sử trò chuyện */
    connect(ui->btnSave, &QToolButton::clicked,
            this, [=](){
                if (ui->msgTextBrowser->document()->isEmpty())
                {
                    QMessageBox::warning(this, "Warning", "Can not save!\nMessage box is empty");
                    return;
                }

                QString path = QFileDialog::getSaveFileName(this, "Save file", "ChatApp-MsgLog", "(*.txt)");
                if (path.isEmpty())
                {
                    QMessageBox::warning(this, "Warning", "Save cancel");
                    return;
                }

                QFile file(path);
                file.open(QIODevice::WriteOnly | QIODevice::Text);  // Hỗ trợ dòng mới
                QTextStream textStream(&file);
                textStream << ui->msgTextBrowser->toPlainText();
                file.close();
            });

    /* Gửi file */
    connect(ui->btnFileSend, &QToolButton::clicked,
            this, [=](){
                QString path = QFileDialog::getOpenFileName(this, "Send file", ".");

                if (path.isEmpty())
                {
                    QMessageBox::warning(this, "Warning", "File sending cancellation");
                    return;
                }

                QFileInfo info(path);
                if (info.size() > FILE_SEND_MAX_BYTES)
                {
                    QMessageBox::critical(this, "Error", "File size cannot exceed 1Gb");
                    return;
                }

                this->lastSendFilePath = path;

                TcpServer* tcpServer = new TcpServer(nullptr, path, DAL::getLocalIpAddress(), PORT_TCP_FILE);
                tcpServer->show();

                sendUDPSignal(SignalType::File);  // Thông tin tập tin phát sóng sẽ được gửi đồng thời đến tất cả các máy khách thông qua UDP
            });

    /* Nhấp vào liên kết trong hộp trò chuyện để mở nó ra bên ngoài */
    connect(ui->msgTextBrowser, &QTextBrowser::anchorClicked,
            this, &ChatBoxWidget::openURL);
}

ChatBoxWidget::~ChatBoxWidget()
{
    delete ui;
}

/** Phát tin nhắn UDP
 * @brief sendMsg
 * @param type
 */
void ChatBoxWidget::sendUDPSignal(const SignalType type)
{
    /** Tin nhắn được chia thành 5 loại (SignalType) nên dữ liệu cần được phân đoạn.
     * Đoạn 1: Kiểu dữ liệu
     * Đoạn 2: Tên nhóm gửi tín hiệu này
     * Đoạn 3: Số cổng nhóm gửi tín hiệu này
     * Đoạn 4: Tên người dùng hiện tại (cục bộ) localUserName
     * Đoạn 5: Số nhóm người dùng hiện tại (cục bộ) localUserGroupNumber
     * Đoạn 6: Người dùng hiện tại (cục bộ) ip localIpAddress
     * Đoạn 7: Nội dung cụ thể (khi gửi file: tên file)
     * Đoạn 8: (chỉ có khi gửi file) kích thước file qint64-bytes
     */
    SignalType      signalType_1            = type;
    QString         chatName_2              = this->name;
    qint16          chatPort_3              = this->port;
    QString         localUserName_4         = DAL::getLocalUserName();
    QString         localUserGroupNumber_5  = DAL::getLocalUserGroupNumber();
    QHostAddress    localIpAddress_6        = DAL::getLocalIpAddress();


    QByteArray      resByteArray;   // nội dung tin nhắn cuối cùng
    QDataStream     dataStream(&resByteArray, QIODevice::WriteOnly);

    dataStream << signalType_1;             // Đoạn 1: Kiểu dữ liệu
    dataStream << chatName_2;               // Đoạn 2: Tên nhóm gửi tín hiệu này
    dataStream << chatPort_3;               // Phần 3: Số cổng nhóm gửi tín hiệu này
    dataStream << localUserName_4;          // Đoạn 4: Tên người dùng hiện tại (cục bộ) localUserName
    dataStream << localUserGroupNumber_5;   // Đoạn 5: Số nhóm người dùng (cục bộ) hiện tại localUserGroupNumber
    dataStream << localIpAddress_6;         // Đoạn 6: Người dùng hiện tại (cục bộ) ip localIpAddress

    /* Thêm nội dung khối tin nhắn 7 */
    qint64 isSucc;
    switch (type) {

    case SignalType::ChatExist:
        dataStream << QString("SignalType::ChatExist");
        udpSocketOnPortChatList->writeDatagram(resByteArray,
                                               QHostAddress(QHostAddress::Broadcast),
                                               DAL::getPortChatList());
        break;  // END SignalType::ChatExist

    case SignalType::ChatDestory:
        dataStream <<  QString("SignalType::ChatDestory");
        udpSocketOnPortChatList->writeDatagram(resByteArray,
                                               QHostAddress(QHostAddress::Broadcast),
                                               DAL::getPortChatList());
        break;  // END SignalType::ChatDestory

    case SignalType::Msg:
        if (ui->msgTextEdit->toPlainText().isEmpty()) return;

        dataStream << getAndCleanMsg();
        isSucc = udpSocketOnPortChatBox->writeDatagram(resByteArray,
                                                       QHostAddress(QHostAddress::Broadcast),
                                                       port);
        if (-1 == isSucc)
        {
            ui->msgTextBrowser->append("[ERROR] Message delivery failure\n");
        }

        break;  // END SignalType::Msg

    case SignalType::File:
    {
        QFileInfo info(this->lastSendFilePath);
        dataStream << info.fileName();  // Đoạn 7: Nội dung cụ thể (khi gửi file: tên file)
        dataStream << info.size();      // Đoạn 8: (chỉ khả dụng khi gửi tệp) kích thước tệp qint64-byte
    }
        udpSocketOnPortChatBox->writeDatagram(resByteArray,
                                              QHostAddress(QHostAddress::Broadcast),
                                              port);
        break;  // END SignalType::File

    case SignalType::UserJoin:
        dataStream <<  QString("SignalType::UserJoin");
        udpSocketOnPortChatBox->writeDatagram(resByteArray,
                                              QHostAddress(QHostAddress::Broadcast),
                                              port);
        break;  // END SignalType::UserJoin

    case UserLeft:
        dataStream <<  QString("SignalType::UserLeft");
        udpSocketOnPortChatBox->writeDatagram(resByteArray,
                                              QHostAddress(QHostAddress::Broadcast),
                                              port);
        break;  // END SignalType::UserLeft

    default:
#if !QT_NO_DEBUG
        qDebug() << "[ERROR]" << __FILE__ << __LINE__ << "Unkown SignalType";
#endif
        break;
    }
}

/** Nhận tin nhắn UDP
 * @brief receiveMessage
 */
void ChatBoxWidget::receiveUDPMessage()
{
    /* Nhận gói dữ liệu */
    qint64      msgSize = udpSocketOnPortChatBox->pendingDatagramSize();
    QByteArray  resByteArray = QByteArray(msgSize, 0);
    udpSocketOnPortChatBox->readDatagram(resByteArray.data(), msgSize);

    /** Phân tích dữ liệu - tin nhắn được chia thành 5 loại (SignalType), do đó dữ liệu cần được phân đoạn.
     * Đoạn 1: Kiểu dữ liệu
     * Đoạn 2: Tên nhóm gửi tín hiệu này
     * Đoạn 3: Số cổng nhóm gửi tín hiệu này
     * Đoạn 4: Tên người dùng hiện tại (cục bộ) localUserName
     * Đoạn 5: Số nhóm người dùng hiện tại (cục bộ) localUserGroupNumber
     * Đoạn 6: Người dùng hiện tại (cục bộ) ip localIpAddress
     * Đoạn 7: Nội dung cụ thể QString
     */
    SignalType      signalType_1          ;
    QString         chatName_2            ;
    qint16          chatPort_3            ;
    QString         localUserName_4       ;
    QString         localUserGroupNumber_5;
    QHostAddress    localIpAddress_6      ;
    QString         msg_7                 ;
    qint64          fileSize_8            ; // Đoạn 8: (chỉ khả dụng khi gửi tệp) kích thước tệp qint64-byte

    QDataStream     dataStream(&resByteArray, QIODevice::ReadOnly);
    QString         time = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss");

    dataStream >> signalType_1;             // Phần 1: Kiểu dữ liệu
    dataStream >> chatName_2;               // Đoạn 2: Tên nhóm gửi tín hiệu (this)
    dataStream >> chatPort_3;               // Phần 3: Gửi số cổng nhóm tín hiệu (this)
    dataStream >> localUserName_4;          // Phần 4: Gửi tên người dùng tín hiệu
    dataStream >> localUserGroupNumber_5;   // Phần 5: Gửi tín hiệu số nhóm người dùng
    dataStream >> localIpAddress_6;         // Phần 6: Gửi tín hiệu ip người dùng
    dataStream >> msg_7;                    // Đoạn 7: Nội dung cụ thể QString (khi gửi file: tên file)

#if !QT_NO_DEBUG
    qDebug() << "[UDP-ChatBox] receiveUDPMessage: "
             << SignalType::Msg
             << signalType_1
             << chatName_2
             << chatPort_3
             << localUserName_4
             << localUserGroupNumber_5
             << localIpAddress_6
             << msg_7;
#endif

    switch (signalType_1) {
    case SignalType::Msg:
        //Thêm lịch sử trò chuyện
        ui->msgTextBrowser->setTextColor(Qt::blue);
        ui->msgTextBrowser->append("[" + localUserName_4 + "] " + time);
        ui->msgTextBrowser->append(msg_7);
        break;

    case SignalType::File:
        dataStream >> fileSize_8; // Đoạn 8: (nhận được khi gửi file) là kích thước file qint64-bytes
        ui->msgTextBrowser->setTextColor(Qt::darkCyan);
        ui->msgTextBrowser->append(QString(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n"
                                           "[%1] on [%2] send a file, file information:\n"
                                           "Name: %3\n"
                                           "Size: %4Kb\n"
                                           ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>").arg(localUserName_4, time, msg_7, QString::number(fileSize_8 / 1024)));
#if QT_NO_DEBUG
        /* Xác định xem người gửi có phải là bạn không, nếu là bạn thì không cần nhận nữa.
          * Nếu nó không phải của bạn, hãy hỏi xem bạn có muốn nhận nó không.
          */
        if (localIpAddress_6 == DAL::getLocalIpAddress()) { return; }  // Nếu người nhận và người gửi tệp đều là máy cục bộ thì không cần phải đưa ra yêu cầu nhận tệp.
#endif
        if (QMessageBox::Yes == QMessageBox::information(this, "File reception request", QString(
                                                                                             "[%1] from group [%2] wants to send you a file, do you want to receive it?\n\n"
                                                                                             "---------------------\n"
                                                                                             "File information:\n"
                                                                                             "Name: %3\n"
                                                                                             "Size: %4Kb").arg(localUserName_4, localUserGroupNumber_5, msg_7, QString::number(fileSize_8 / 1024)),
                                                         QMessageBox::No | QMessageBox::Yes,
                                                         QMessageBox::Yes))
        {
            TcpClient* tcpClient = new TcpClient(nullptr, msg_7, fileSize_8, localIpAddress_6, PORT_TCP_FILE);
            tcpClient->show();
        }
        break;

    case SignalType::UserJoin:
        userJoin(localUserName_4, localUserGroupNumber_5, localIpAddress_6);
        break;

    case SignalType::UserLeft:
        userLeft(localUserName_4, time);
        break;

    default:
#if !QT_NO_DEBUG
        qDebug() << "[ERROR]" << __FILE__ << __LINE__ << "Unkown SignalType";
#endif
        break;
    }
}

/* Xử lý việc tham gia của người dùng */
void ChatBoxWidget::userJoin(QString name, QString groupNumber, QHostAddress ip)
{
    if (ui->tbUser->findItems(name, Qt::MatchExactly).isEmpty())
    {
        /* Cập nhật danh sách người dùng */
        ui->tbUser->insertRow(0);
        ui->tbUser->setItem(0, 0, new QTableWidgetItem(name));
        ui->tbUser->setItem(0, 1, new QTableWidgetItem(groupNumber));
        ui->tbUser->setItem(0, 2, new QTableWidgetItem(ip.toString()));

        /* Thêm lịch sử trò chuyện */
        ui->msgTextBrowser->setTextColor(Qt::gray);
        ui->msgTextBrowser->append(QString("%1 online！").arg(name));

        /* Cập nhật người dùng trực tuyến */
        ui->lbNumberOnlineUse->setText(QString::number(ui->tbUser->rowCount()));

        /* Phát sóng thông tin của chính mình */
        sendUDPSignal(SignalType::UserJoin);
    }
}


/* Xử lý việc người dùng rời đi */
void ChatBoxWidget::userLeft(QString name, QString time)
{
    if (!ui->tbUser->findItems(name, Qt::MatchExactly).isEmpty())
    {
        /* Cập nhật danh sách người dùng */
        ui->tbUser->removeRow(ui->tbUser->findItems(name, Qt::MatchExactly).constFirst()->row());

        /* Thêm lịch sử trò chuyện */
        ui->msgTextBrowser->setTextColor(Qt::gray);
        ui->msgTextBrowser->append(QString("%1 left on %2").arg(name, time));

        /* Cập nhật người dùng trực tuyến */
        ui->lbNumberOnlineUse->setText(QString::number(ui->tbUser->rowCount()));
    }
}

QString ChatBoxWidget::getAndCleanMsg()
{
    QString str = ui->msgTextEdit->toHtml();

    // Nhân tiện, hãy xóa hộp thoại
    ui->msgTextEdit->clear();
    ui->msgTextEdit->setFocus();

    return str;
}


/* kích hoạt sự kiện đóng */
void ChatBoxWidget::closeEvent(QCloseEvent* event)
{
    emit this->signalClose();

    if (1 == ui->tbUser->rowCount())
    {
        sendUDPSignal(SignalType::ChatDestory);
    }
    else
    {
        sendUDPSignal(SignalType::UserLeft);
    }

    udpSocketOnPortChatList->close();  // đóng ổ cắm
    emit udpSocketOnPortChatList->destroyed();

    udpSocketOnPortChatBox->close();
    emit udpSocketOnPortChatBox->destroyed();

    QWidget::closeEvent(event);
}

void ChatBoxWidget::openURL(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}
