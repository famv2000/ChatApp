#include "uil_addchat.h"
#include "ui_addchat.h"
// #include "uil_chatboxwidget.h"
// #include "dal_chatApp.h"

#include <QMessageBox>

AddChat::AddChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddChat)
{
    ui->setupUi(this);
    this->setWindowTitle("Add new chat");
    this->setWindowIcon(QIcon(":/icon/icons/register-add-friend.png"));
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowModality(Qt::ApplicationModal);

    connect(ui->btnCancelAddChat, &QPushButton::clicked,
            this, [=](){this->close();});

    connect(ui->btnAddChat, &QPushButton::clicked,
            this, [=](){
                if (ui->leNameNewChat->text().isEmpty())
                {
                    QMessageBox::warning(this, "Warning", "Name can not be empty");
                    return;
                }

                emit this->addNewChat(ui->leNameNewChat->text());

                this->close();
            });
}

AddChat::~AddChat()
{
    delete ui;
}
