//
// Created by Administrator on 2023/11/12.
//


bool register(int uid,char* pwd){
    //...
    // suppose we have a correct Authentication instance now,its name is auth
    auth->Add(uid,pwd);
    //...
    //write the auth instance back to the disk
}

/**
 * Read a nachos virtual file
 * @param uid user id
 * @param pwd password
 * @param fName file name
 * @param op operation index
 * @return null if there's no file with fName or Authentication fail
 */
OpenFile* AccessNachosFile(int uid,char* pwd,char* fName,int op){
    // suppose we have a correct Authentication instance now,its name is auth
    if(!auth->Auth(uid,pwd)){
        return NULL; // Authentication fail
    }
    // suppose we have a correct Directory instance now,its name is dir
    if(!dir->VerifyUid(uid,fName,op)){
        return NULL; // Authentication fail
    }
    int secNo = dir->Find(fName);
    if(secNo==-1){
        return NULL; // there's no file with name fName
    }
    OpenFile* ret = new OpenFile(secNo);
    return ret;
}
