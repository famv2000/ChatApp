#include "tcpclient.h"
#include "ui_tcpclient.h"
#include "dal_chatApp.h"

#include <QMovie>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>

TcpClient::TcpClient(QWidget *parent, const QString &fileName, qint64 fileSizeBytes, QHostAddress serverIp, qint16 serverPort) :
    QWidget(parent),
    ui(new Ui::TcpClient)
{
    ui->setupUi(this);
    this->setWindowTitle("File receipter");
    this->setAttribute(Qt::WA_DeleteOnClose);
    appendTextBrowser(Qt::blue, "[INFO] TCP client start");


    ///////////////////////////////// ui hình ảnh /////////////////////////////////
    movie = new QMovie(":/gif/icons/eating.gif");
    movie->start();
    ui->labelPic->setMovie(movie);


    ///////////////////////////////// Khởi tạo biến /////////////////////////////////
    appendTextBrowser(Qt::blue, "[INFO] Initializing the TCP client...");

    ui->lbClientIP->setText(DAL::getLocalIpAddress().toString());
    ui->lbClientPort->setText(QString::number(PORT_TCP_FILE));


    this->tcpSocket = new QTcpSocket(this);
    /* Nếu kết nối được thiết lập với máy chủ */
    connect(this->tcpSocket, &QTcpSocket::connected, this, [=](){
        ui->btnSave->setEnabled(false);
        appendTextBrowser(Qt::green, "[INFO] Successfully establish a link with the host");
    });

    /* Nếu ngắt kết nối với máy chủ */
    connect(this->tcpSocket, &QTcpSocket::disconnected, this, [=](){
        ui->btnSave->setEnabled(true);
        appendTextBrowser(Qt::darkYellow, "[WARNING] Disconnected with the host");
    });

    /* Nhận nội dung từ ổ cắm giao tiếp */
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpClient::receiveTcpDataAndSave);

    this->fileName = fileName;
    ui->lbFileName->setText(this->fileName);

    this->fileSizeBytes = fileSizeBytes;
    ui->lbFileSize->setText(QString("%1Kb").arg(QString::number(this->fileSizeBytes / 1024)));

    this->serverIp = serverIp;
    ui->lbServerIP->setText(this->serverIp.toString());

    this->serverPort = serverPort;
    ui->lbServerPort->setText(QString::number(this->serverPort));

    /* Nhấp vào nút <Lưu> */
    connect(ui->btnSave, &QPushButton::clicked, this, &TcpClient::connectTcpServerAndOpenFile);

    /* Nhấp vào nút <Hủy> */
    connect(ui->btnCancel, &QPushButton::clicked, this, [=](){ this->close(); });

    appendTextBrowser(Qt::blue, "[INFO] Initializing the TCP client done");
    appendTextBrowser(Qt::red, "[INFO] Click `Save` to receive the file");


    /* Cập nhật thanh tiến trình bằng bộ đếm thời gian */
    timer_progressBar = new QTimer(this);
    connect(timer_progressBar, &QTimer::timeout, this,
            [=](){
                ui->progressBar->setValue(bytesReceived / fileSizeBytes * 100);  // Cập nhật thanh tiến trình
                ui->progressBar->update();

#if !QT_NO_DEBUG
                appendTextBrowser(Qt::darkCyan, QString("[DEBUG] TCP-Client bytesReceived:%1").arg(bytesReceived));
#endif
            });
}

/** Bấm vào nút <Save> file để khởi tạo dữ liệu và lưu file
 * @brief TcpClient::saveFile
 */
void TcpClient::connectTcpServerAndOpenFile()
{
    /* Nhận đường dẫn lưu */
    ui->lbFileSavePath->setText("[EMPTY] Click `Save` to receive the file");
    QString savePath = QFileDialog::getSaveFileName(this, "Save file to", fileName);
    if (savePath.isEmpty())
    {
        appendTextBrowser(Qt::red, "[ERROR] Save cancel, because the file save path is empty. Click on the `Save` button to receive the file again");
        QMessageBox::critical(this, "ERROR", "File path is empty, because the file save path is empty (You did not specify a save directory).\n"
                                             "You can click on the `Save` button to receive the file again");
        return;
    }

    this->file.setFileName(savePath);
    bool isFileOpen = file.open(QIODevice::WriteOnly);
    if (!isFileOpen)
    {
        appendTextBrowser(Qt::red, QString("[ERROR] Can not write file %1 to %2").arg(fileName, savePath));
        return;
    }

    ui->lbFileSavePath->setText(savePath);
    appendTextBrowser(Qt::blue, QString("[INFO] Open file successfully, the file will save to path: %1").arg(file.fileName()));

    /* Khởi tạo/đặt lại các biến */
    this->isHeaderReceived = false;
    this->bytesReceived = 0;
    this->tcpSocket->connectToHost(QHostAddress(serverIp), serverPort);  // Máy chủ thiết lập kết nối
    appendTextBrowser(Qt::blue, "[INFO] Try to connect to server...");
}


/** Sau khi đối tượng tệp được mở và kết nối với <TCP Server>, hãy bắt đầu đọc dữ liệu vào tệp
 * @brief TcpClient::receiveTcpDataAndSave
 */
void TcpClient::receiveTcpDataAndSave()
{
    if (!this->file.isOpen())
    {
        appendTextBrowser(Qt::red, "[ERROR] The file is not open and receiving data is interrupted. Please click the `Save` button to retry");
        tcpSocket->disconnectFromHost(); //Ngắt kết nối
        tcpSocket->close(); //đóng ổ cắm
        return;
    }

    QByteArray byteArray = tcpSocket->readAll();
    /* Nếu đó là tiêu đề tập tin */
    if (!isHeaderReceived)
    {
        appendTextBrowser(Qt::blue, "[INFO] Start parsing the data header");
        this->isHeaderReceived = true;
        this->bytesReceived = 0;
        ui->progressBar->setValue(0);
        timer_progressBar->start(100);

        /** Phân tích cú pháp <header>
        *   headerĐoạn 1: Tên tệp (QString)
        *   headerĐoạn 2: Kích thước tệp (qint64）
        */
        QDataStream dataStream(&byteArray, QIODevice::ReadOnly);
        dataStream >> this->fileName;             // <header>Đoạn 1: Tên tệp (QString)
        dataStream >> this->fileSizeBytes;        // <header>Đoạn 2: Kích thước tệp (qint64)
        appendTextBrowser(Qt::blue, QString("[INFO] The result of the file header parsing is:\n"
                                            "Name: %1\n"
                                            "Size: %2Kb").arg(fileName).arg(fileSizeBytes / 1024));
        appendTextBrowser(Qt::blue, "[INFO] Start to save file, receive data from TCP Server.\nPLEASE WAIT, DO NOT CLOSE THIS WINDOW...");
    }
    /* Tiêu đề file được nhận, theo sau là nội dung dữ liệu của file */
    else
    {
        qint64 lenWrite = file.write(byteArray);
        if (lenWrite > 0)
        {
            this->bytesReceived += lenWrite;
        }

        if (fileSizeBytes == bytesReceived)
        {
            timer_progressBar->stop();
            ui->progressBar->setValue(bytesReceived / fileSizeBytes * 100);  // Cập nhật thanh tiến trình
            ui->progressBar->update();

            ui->textBrowser->setTextColor(Qt::green);
            ui->textBrowser->append(QString("[INFO] File %1 received successfully, already saved to the path: %2").arg(fileName, file.fileName()));

            this->isHeaderReceived = false;
            this->bytesReceived = 0;

            tcpSocket->disconnectFromHost(); //Ngắt kết nối
            tcpSocket->close(); //đóng ổ cắm
            file.close(); //Đóng tập tin
        }
    }
}


void TcpClient::closeEvent(QCloseEvent* event)
{
    QMessageBox::StandardButton btnPush = QMessageBox::warning(this, "Cancel receive",
                                                               "If the file is not received, it will be stopped and disconnected.\n"
                                                               "Are you sure you want to cancel receiving files?",
                                                               QMessageBox::No | QMessageBox::Yes,
                                                               QMessageBox::No);
    if (btnPush == QMessageBox::No)
    {
        event->ignore();
    }
    else
    {
        tcpSocket->disconnectFromHost(); //Ngắt kết nối
        tcpSocket->close(); //đóng ổ cắm
        event->accept();
        QWidget::closeEvent(event);
    }
}


/** Nối nội dung vào TextBrowser của <ui>
 * @brief appendTextBrowser
 * @param color
 * @param text
 */
void TcpClient::appendTextBrowser(Qt::GlobalColor color, QString text)
{
    ui->textBrowser->setTextColor(color);
    ui->textBrowser->append(text);
}


TcpClient::~TcpClient()
{
    delete ui;
}
