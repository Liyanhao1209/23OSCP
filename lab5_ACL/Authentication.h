//
// Created by Administrator on 2023/11/12.
//

#ifndef LAB5_ACL_AUTHENTICATION_H
#define LAB5_ACL_AUTHENTICATION_H

#define pwdLen 9

class User{
    public:
        int uid;
        char password[pwdLen+1];
        bool use;
};

class Authentication{

    bool Add(int uid,char* pwd);

    bool Auth(int uid,char* pwd);

    private:
        int tableSize;
        User *users;

};

#endif //LAB5_ACL_AUTHENTICATION_H
