#ifndef SIGNALTYPE_H
#define SIGNALTYPE_H


enum SignalType
{
    ChatExist,      // Cửa sổ trò chuyện này tồn tại (để phát sóng)
    ChatDestory,    // Bị hủy khi người dùng cuối cùng trong cuộc trò chuyện thoát ra
    Msg,            // Tin tức chung
    File,           // yêu cầu tập tin
    UserJoin,       // Người dùng vào trò chuyện
    UserLeft,       // Người dùng rời đi

};


static const int    BORDCAST_TIME_STEP      = 1000;             // Khoảng thời gian truyền phát thông tin


#endif // SIGNALTYPE_H
