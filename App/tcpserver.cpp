#include "tcpserver.h"
#include "ui_tcpserver.h"
#include "db_localdata.h"

#include <QMovie>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>

TcpServer::TcpServer(QWidget *parent, QString filePath, QHostAddress ip, qint16 port) :
    QWidget(parent),
    ui(new Ui::TcpServer)
{
    ui->setupUi(this);
    this->setWindowTitle("File sender");
    this->setAttribute(Qt::WA_DeleteOnClose);
    appendTextBrowser(Qt::blue, "[INFO] TCP server start");

    ///////////////////////////////// ui hình ảnh /////////////////////////////////
    movie = new QMovie(":/gif/icons/loading.gif");
    movie->start();
    ui->labelPic->setMovie(movie);

    ///////////////////////////////// Khởi tạo biến /////////////////////////////////
    appendTextBrowser(Qt::blue, "[INFO] Initializing the TCP server...");
    if (filePath.isEmpty())
    {
        appendTextBrowser(Qt::red, "[ERROR] Send Cancel");
        QMessageBox::critical(this, "ERROR", "File path is empty");
        return;
    }

    this->ip = ip;
    this->port = port;

    QFileInfo info(filePath);
    this->fileName = info.fileName();
    this->fileSize = info.size();  // bytes


    /* khởi tạo ui */
    ui->lbServerIP->setText(this->ip.toString());
    ui->lbServerPort->setText(QString::number(this->port));
    ui->lbFilePath->setText(filePath);
    ui->lbFileSize->setText(QString("%1Kb").arg(this->fileSize / 1024));
    appendTextBrowser(Qt::blue, "[INFO] Initializing the TCP server done");

    ///////////////////////////////// Ổ cắm nghe: Nếu có kết nối, bắt đầu gửi tệp /////////////////////////////////
    this->tcpSocket = nullptr;
    this->tcpServer = new QTcpServer(this);
    this->tcpServer->listen(QHostAddress::Any, this->port);
    connect(tcpServer, &QTcpServer::newConnection, this, [=](){

        this->tcpSocket = tcpServer->nextPendingConnection();  // Trả về kết nối đang chờ xử lý tiếp theo dưới dạng đối tượng QTcpSocket được kết nối

        /* Đăng nhập thông tin <client> */
        QString clientIp = tcpSocket->peerAddress().toString();
        qint16 clientPort = tcpSocket->peerPort();
        ui->lbClientIP->setText(clientIp);
        ui->lbClientPort->setText(QString::number(clientPort));
        ui->progressBar->setValue(0);

        /* nếu bị ngắt kết nối */
        connect(tcpSocket, &QTcpSocket::disconnected, this, [=](){
            ui->lbClientIP->setText("[NONE] NO CONNECT");
            ui->lbClientPort->setText("[NONE] NO CONNECT");
            appendTextBrowser(Qt::darkYellow, QString("[WARRING] Disconnect with client %1:%2 ").arg(clientIp).arg(clientPort));
        });

        appendTextBrowser(Qt::green, QString("[INFO] Success connected to %1:%2 ").arg(clientIp).arg(clientPort));

        /* Bắt đầu xử lý (đọc) tệp */
        this->bytesAlreadySend = 0;
        this->file.setFileName(filePath);
        bool isOpenSucc = this->file.open(QIODevice::ReadOnly);
        if (isOpenSucc)
        {
            appendTextBrowser(Qt::blue,
                              QString("[INFO] File opened successfully\n"
                                      "Name: %1\n"
                                      "Path: %2\n"
                                      "Size: %3Kb").arg(this->fileName, filePath, QString::number(fileSize / 1024)));
        }
        else
        {
            appendTextBrowser(Qt::red, "[ERROR] File opening failed");
            return;
        }

        /* Thiết lập kết nối và bắt đầu gửi dữ liệu, gửi tiêu đề trước rồi đến dữ liệu.  */
        appendTextBrowser(Qt::blue, "[INFO] Start sending file header information");
        QByteArray      headerByteArray;   // Gửi đầu tiên header
        QDataStream     dataStream(&headerByteArray, QIODevice::WriteOnly);

        dataStream << this->fileName;             // header Đoạn 1: Tên file (QString)
        dataStream << this->fileSize;             // header Đoạn 2: Kích thước file (qint64)第2段：文件大小（qint64）

        qint64 len = tcpSocket->write(headerByteArray);

        /* Nếu <header> được gửi thành công, hãy gửi lại dữ liệu. Và để ngăn chặn các gói dính TCP, cần có độ trễ hẹn giờ là 20 ms.*/
        if (len > 0)
        {
            appendTextBrowser(Qt::blue, "[INFO] The file header is sent successfully");
            timer->start(TCP_DELAY_MS);  // TCP Khoảng thời gian gửi tệp để ngăn chặn các gói dính
            return;
        }
        else
        {
            appendTextBrowser(Qt::red, "[ERROR] File header sending failed");
            file.close();
            return;
        }
    });

    /* Sau khi gửi tiêu đề tệp, hãy gửi gói dữ liệu vào mỗi thời điểm cố định */
    this->timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=](){
        timer->stop();
        sendData(); });


    /* Bấm vào nút hủy */
    connect(ui->btnCancel, &QPushButton::clicked, this, [=](){ this->close(); });
}


void TcpServer::sendData()
{
    appendTextBrowser(Qt::blue, "[INFO] Data is started to be sent......");
    movie->start();

    qint64 lenPackage = 0;  // Ghi lại kích thước gói [hiện tại]
    this->bytesAlreadySend = 0;

    // Cập nhật thanh tiến trình bằng bộ đếm thời gian
    QTimer* timer_progressBar = new QTimer(this);
    connect(timer_progressBar, &QTimer::timeout, this,
            [=](){
                ui->progressBar->setValue(this->bytesAlreadySend / this->fileSize * 100);
                ui->progressBar->update();

#if !QT_NO_DEBUG
                appendTextBrowser(Qt::darkCyan, QString("[DEBUG] TCP-Server this->bytesAlreadySend:%1").arg(this->bytesAlreadySend));
#endif
            });

    timer_progressBar->start(100);
    do {
        char buf[4 * 1024] = {0};  // [Gói dữ liệu] Dữ liệu gửi mỗi lần 4Kb
        lenPackage = file.read(buf, sizeof(buf));
        lenPackage = tcpSocket->write(buf, lenPackage);  // Gửi dữ liệu, đọc bao nhiêu tùy thích, gửi bao nhiêu tùy thích

        this->bytesAlreadySend += lenPackage;  // Dữ liệu được gửi cần được tích lũy


    } while (lenPackage > 0);

    if (this->bytesAlreadySend == this->fileSize)
    {
        timer_progressBar->stop();
        ui->progressBar->setValue(this->bytesAlreadySend / this->fileSize * 100);
        ui->progressBar->update();

        appendTextBrowser(Qt::green, "[INFO] Success to send file\n#############################\n\n");
        this->file.close();
        this->tcpSocket->disconnectFromHost();
        this->tcpSocket->close();
        this->tcpSocket = nullptr;
        //        movie->stop();
    }

    return;
}

/** Nối nội dung vào TextBrowser của ui
 * @brief appendTextBrowser
 * @param color
 * @param text
 */
void TcpServer::appendTextBrowser(Qt::GlobalColor color, QString text)
{
    ui->textBrowser->setTextColor(color);
    ui->textBrowser->append(text);
}


void  TcpServer::closeEvent(QCloseEvent* event)
{
    QMessageBox::StandardButton btnPush = QMessageBox::warning(this, "Cancel send",
                                                               "The current file transfer will be cancelled if it is not completed and all connections will be disconnected.\n"
                                                               "Are you sure you want to cancel sending files?",
                                                               QMessageBox::No | QMessageBox::Yes,
                                                               QMessageBox::No);
    if (btnPush == QMessageBox::No)
    {
        event->ignore();
    }

    /* [Lưu ý] Điều này có thể ngăn chặn sự cố xảy ra khi đóng cửa sổ mà không gửi tệp (tcpSocket chưa được khởi tạo). */
    if (nullptr != this->tcpSocket)
    {
        tcpSocket->disconnectFromHost(); //Ngắt kết nối
        tcpSocket->close(); //đóng ổ cắm
    }
    event->accept();
    QWidget::closeEvent(event);
}


TcpServer::~TcpServer()
{
    delete ui;
}
