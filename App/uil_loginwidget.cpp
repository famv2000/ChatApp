#include "uil_loginwidget.h"
#include "ui_loginwidget.h"
#include "dal_chatApp.h"

#include <QMessageBox>
#include <QPalette>

LoginWidget::LoginWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWidget)
{
    ui->setupUi(this);

    this->setWindowTitle("ChatApp");
    this->setWindowIcon(QIcon(":/icon/icons/iconWindow.png"));
    ui->leUserName->setFocus();
    //    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setAutoFillBackground(true);
    QPalette background = this->palette();
    background.setBrush(QPalette::Window,
                        QBrush(QPixmap(":/bk/background/bg-login.png").scaled(// Thu phóng hình nền.
                            this->size(),
                            Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation)));             // Sử dụng tỷ lệ trơn tru
    this->setPalette(background);

    /* User checked button `login` */
    connect(ui->btnLogin, &QPushButton::clicked,
            this, &LoginWidget::userLogin);

    connect(ui->btnInfo, &QPushButton::clicked,
            this, [=](){
                QMessageBox::about(this, "About App",
                                   "Name: ChatApp<br><br>"
                                   "Source Code:<br>"
                                   "<a href=\"https://github.com/famv2000/ChatApp\">[Github] ChatApp</a>"
                                   "<br><br>"
                                   "License: Apache License 2.0"
                                   "<br><br>"
                                   "Made on Qt 6.6");
            });
}


/** [SLOT] User Login
 * @brief LoginWidget::userLogin
 */
void LoginWidget::userLogin()
{
    /* Check user info in LineEdit, if OK then init data in DB */
    bool isSuccInitLocalUser = DAL::initLocalUser(ui->leUserName->text(), ui->leUserGroupNumber->text());
    if (!isSuccInitLocalUser)
    {
        QMessageBox::warning(this, "Warning", "Name or Group number can not be empty");
        return;
    }
    this->close();

    DAL::initAndShowChatList(nullptr);  // If user login, then show ChatList. After this `ChatList` Widget is main windows
}



LoginWidget::~LoginWidget()
{
    delete ui;

}
