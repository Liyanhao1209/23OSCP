// dummy arg,useless
void userFunc(_int dummy){
    // run the user prog
    machine->Run();
    // never returns
    ASSERT(FALSE);
}

void
Interrupt::Exec() {
    printf("Execute system call of Exec()\n");

    int maxFnLen = 50;
    char filename[maxFnLen];
    /**
     * The start address of filename is in the $4 register
     */
     int fnReg = 4;
     int fnAddr = machine->ReadRegister(fnReg);
     DEBUG('u',"file name address is: %d\n",fnAddr);

     int i;
     for(i=0;i<maxFnLen;i++){
         machine->ReadMem(fnAddr+i,1,(int*)&filename[i]);
         if(i==maxFnLen-1){
             // check if the file name oversize
             ASSERT(filename[i]=='\0');
         }
         if(filename[i]=='\0') // end of file name
             break;
     }


     printf("Exec(%s):\n",filename);

     OpenFile* exe = fileSystem->Open(filename);
     if(exe == NULL){
         printf("no such file: %s\n",filename);
         return;
     }

     // allocate the page table for exe
     AddrSpace* as = new AddrSpace(exe);
     // init the vm registers and restore the page table
     as->InitRegisters();
     as->RestoreState();
     // new kernel thread for executing user thread
     Thread* tfu = new Thread(filename);
     // load the as to tfu
     tfu->space = as;
     int dummy = 0;
     tfu->Fork(userFunc,0);
     // free exe
     delete exe;
     // since the nachos Exec() syscall has a ret val
     // we should write the as id back to the ret reg
     machine->WriteRegister(2,as->getAsId());
     // yield currentThread for executing the thread for user
     currentThread->Yield();
}

void
Interrupt::PrintInt() {
    // the arg of syscall is in reg 4
    printf("%d\n",machine->ReadRegister(4));
}

void
Interrupt::Fork(){
    printf("Execute system call of Exec()\n");

    /**
     * The start addr of func is in the r4 reg
     */
     int funcReg = 4;
     int funcAddr = machine->ReadRegister(funcReg);

     /**
      * Let the PC jump to the function entry
      */
    machine->registers[PCReg] = funcAddr;
    machine->registers[PrevPCReg] = (funcAddr>=4)?(funcAddr-4):0;
    machine->registers[NextPCReg] = funcAddr+4;

    /**
     * new thread sharing the same space with the old thread
     */
    Thread* tfu = new Thread(currentThread->getName());
    tfu->space = currentThread->space;
    tfu->Fork(userFunc,0);

    // LET'S FXXKING GO!
    currentThread->Yield();
}

/**
 * Lab6:mup extension
 * simulate the exec() syscall on Unix
 * cover the cur thread's addr space to run
 */
void
Interrupt::UExec() {
    printf("Execute system call of Unix Exec()\n");

    int maxFnLen = 50;
    char filename[maxFnLen];
    /**
     * The start address of filename is in the $4 register
     */
    int fnReg = 4;
    int fnAddr = machine->ReadRegister(fnReg);
    DEBUG('u',"file name address is: %d\n",fnAddr);

    int i;
    for(i=0;i<maxFnLen;i++){
        machine->ReadMem(fnAddr+i,1,(int*)&filename[i]);
        if(i==maxFnLen-1){
            // check if the file name oversize
            ASSERT(filename[i]=='\0');
        }
        if(filename[i]=='\0') // end of file name
            break;
    }


    printf("Unix Exec(%s):\n",filename);

    OpenFile* exe = fileSystem->Open(filename);
    if(exe == NULL){
        printf("no such file: %s\n",filename);
        return;
    }

    // get the cur page table
    TranslationEntry* cpt = currentThread->space->getPT();
    // deallocate the pages on this page table
    unsigned int np = currentThread->space->getNumPages();
    for(int i=0;i<np;i++){
        pageMap->Clear(cpt[i].physicalPage);
    }
    // reallocate the page table for exe
    AddrSpace* as = new AddrSpace(exe);
    // init the vm registers and restore the page table
    as->InitRegisters();
    as->RestoreState();
    // new kernel thread for executing user thread
    Thread* tfu = new Thread(filename);
    // load the as to tfu
    tfu->space = as;
    int dummy = 0;
    tfu->Fork(userFunc,0);
    // free exe
    delete exe;
    // since the nachos Exec() syscall has a ret val
    // we should write the as id back to the ret reg
    machine->WriteRegister(2,as->getAsId());
    // yield currentThread for executing the thread for user
    currentThread->Yield();
}



