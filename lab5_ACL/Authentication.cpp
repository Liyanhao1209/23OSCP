//
// Created by Administrator on 2023/11/12.
//

#include "Authentication.h"

bool
Authentication::Add(int uid, char *pwd) {
    for(int i=0;i<tableSize;i++){
        if(!users[i].use){
            users[i].use = true;
            users[i].password = pwd;
            user[i].uid = uid;
            return true;
        }
    }
    return false;
}

bool
Authentication::Auth(int uid, char *pwd) {
    for(int i=0;i<tableSize;i++){
        if(users[i].use&&users[i].uid==uid){
            if(!strcmp(users[i].password,pwd)){
                return false;
            }
            return true;
        }
    }
    return false;
}
