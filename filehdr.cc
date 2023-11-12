// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include <time.h>
#include <string.h>

#define IllegalIndirectSectorNo -1

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accommodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

/**
 * Lab4:filesys extension
 * the guided readme asked me to add a memset code segment
 * in the constructor of FileHeader.
 * This is reasonable since 0x00 is a magic number for the
 * sector number of free bit map file in nachos file sys.
 *  Thus we can judge whether a sector in the file has been
 *  written.
 */
FileHeader::FileHeader(){
    memset(dataSectors,0,sizeof (dataSectors));
    // init the indirect field
    indirect = IllegalIndirectSectorNo;
}

/**
 * This function can only be called once per file.
 * If you want to call this function more than once,
 * you should call Deallocate first before except for the first call
 * Otherwise the blocks represented in bit map would be
 * homeless and untraceable
 */
bool
FileHeader::Allocate(BitMap *freeMap, int fileSize, int secNo)
{ 
    numBytes = fileSize;
    /**
     * Lab4:filesys extension
     * when the file is creating,we should assign its create time
     */
     lastUpdateTime = (int)time(NULL);
    int numSectors  = divRoundUp(fileSize, SectorSize);
    /**
     * Lab5:filesys extension
     * check if there are enough space for the data and the external file headers
     * except for the original one
     */
    int totalSectors = numSectors + divRoundUp(numSectors,NumDirect);
    // ought to be greater than -1!
    totalSectors = (totalSectors-1)>=0?(totalSectors-1):0;
    if (freeMap->NumClear() < totalSectors)
	return FALSE;		// not enough space

    // recursively allocate dataSectors
    int nextSecNo = AllocateEachFHdr(freeMap,0,numSectors,secNo);
    // prevent flushing the disk
    FetchFrom(secNo);
    // in case the indirect has not been written to the disk
    //WriteBack(secNo);
    numBytes = fileSize;
    indirect = nextSecNo;
    //DEBUG('f',"updating the last file header's length\n");
    updateLastFileHeaderLength(secNo);
    return TRUE;
}

/**
 * recursive allocate the dataSectors
 * @return the next file header's sector number,null if -1
 */
 int
FileHeader::AllocateEachFHdr(BitMap *bitMap,int startNo,int restSectors,int curSectorNo) {
    if(restSectors<=0){
        // nothing left to allocate
        // there'll be no more external file headers
        return -1;
    }

    //DEBUG('f',"Allocating the original file header\n");
    int leftSize = NumDirect - startNo;
    // the real number of sectors to be allocated this time
    int allocatedNum = restSectors>leftSize?leftSize:restSectors;
    // if there's no rest sector to be allocated,then there'll be no more external file headers
    int nextFHdrSectorNo = (restSectors-allocatedNum)>0?bitMap->Find():IllegalIndirectSectorNo;
    DEBUG('f',"There'll be %d rest sectors after this allocation\n",restSectors-allocatedNum);
    if(nextFHdrSectorNo!=IllegalIndirectSectorNo){
        DEBUG('f',"Allocate sector %d as an external file header for file header on %d\n",
              nextFHdrSectorNo,curSectorNo);
    }

    FileHeader* fHdr = new FileHeader;
    // fetch the file on the disk
    // in case we change the part that has already been allocated
    fHdr->FetchFrom(curSectorNo);
    fHdr->setLastUpdateTime((int)time(NULL));
    fHdr->setFileLength((startNo+allocatedNum)*SectorSize);
    //allocate the data sectors
    fHdr->setDataSectors(bitMap,startNo,allocatedNum);
    // set indirect field
    fHdr->setIndirect(nextFHdrSectorNo);
    fHdr->WriteBack(curSectorNo);
    DEBUG('f',"Allocate fHdr:Write file header %d back to disk\n",curSectorNo);
    delete fHdr;

    // recursively allocate the next file header
    // ought to start at 0!
    AllocateEachFHdr(bitMap,0,restSectors-allocatedNum,nextFHdrSectorNo);
    return nextFHdrSectorNo;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------
/**
 * Lab5:filesys extension
 * recursively deallocate each file header for current file
 * start with curSectorNo
 */
void 
FileHeader::Deallocate(BitMap *freeMap,int curSectorNo)
{
    if(curSectorNo==IllegalIndirectSectorNo){
        // the end of the external file headers
        return;
    }
    FileHeader* fHdr = new FileHeader;
    // get current file header
    fHdr->FetchFrom(curSectorNo);
    int numSectors = fHdr->calculateNumSectors();
    int* ds = fHdr->getDataSectors();
    for (int i = 0; i < numSectors; i++) {
        ASSERT(freeMap->Test((int) ds[i]));  // ought to be marked!
        freeMap->Clear((int) ds[i]);
    }
    ASSERT(freeMap->Test((int)curSectorNo));
    freeMap->Clear((int)curSectorNo);
    Deallocate(freeMap,fHdr->getIndirect());
}


//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------
/**
 * Lab5:filesys extension
 * @param offset
 * @return
 * there's no need to use recursion to find the dest sector number
 * we can simply use loop instead
 */
int
FileHeader::ByteToSector(int offset)
{
    // the total length of sectors
     int numSectors = offset / SectorSize;
     // special occasion: the offset occurs in the very original file header
     if(numSectors<NumDirect){
         return dataSectors[numSectors];
     }
     // rest portion
     FileHeader* fHdr = new FileHeader;
     fHdr->setIndirect(indirect);
     while(numSectors>=NumDirect){
         numSectors -= NumDirect;
         // next external file header
         fHdr->FetchFrom(fHdr->getIndirect());
     }
     return fHdr->getDataSectors()[numSectors];
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------
/**
 * Lab5:filesys extension
 * The original file header registered in the directory table's length is
 * the length of the complete file
 * Others' are the length of the portion of the file stored in current file header(s)
 */
int
FileHeader::FileLength()
{
    return numBytes;
}

/**
 * Lab4:filesys extension
 * since the length of nachos file is changeable
 * the bit map only allocate the initial size of sectors to
 * the file
 * thus we should change the bit map when we're writing some data into the file
 */
void
FileHeader::updateFileLength(int newFileLength,int secNo) {
    DEBUG('d',"update file length,new file length:%d\n",newFileLength);
    //calculate current used sectors
    int curSectors = divRoundUp(newFileLength,SectorSize);
    int oldSectors = divRoundUp(numBytes,SectorSize);
    if(curSectors>oldSectors){

        int add = curSectors-oldSectors;

        DEBUG('d',"allocate new sectors for current file header\n");

        // find the last file header
        FileHeader* fHdr = new FileHeader;

        int lastFHdrSecNo = secNo;

        fHdr->FetchFrom(secNo);
        DEBUG('f',"Next file header index: %d\n",fHdr->getIndirect());
        while(fHdr->getIndirect()!=IllegalIndirectSectorNo){
            lastFHdrSecNo = fHdr->getIndirect();
            fHdr->FetchFrom(fHdr->getIndirect()); // 5
            DEBUG('f',"Next file header index: %d\n",fHdr->getIndirect());
            curSectors -= NumDirect;
        }

        //DEBUG('f',"last file header's length: %d, needed sectors: %d\n",fHdr->FileLength(),fHdr->calculateNumSectors());

        // where to start append
        int startNo = fHdr->calculateNumSectors();

        int bitmapSecNo = 0;
        // fetch the bit map from disk
        OpenFile* bitmap = new OpenFile(bitmapSecNo); // 0
        // load the bitmap to mem
        BitMap* bm = new BitMap(NumSectors);
        bm->FetchFrom(bitmap); // 2

        // for the last file header,make changes to the bit map
        DEBUG('f',"Ready to allocate sectors for last file header on sector: %d\n",lastFHdrSecNo);
        AllocateEachFHdr(bm,startNo,add,lastFHdrSecNo);
        FetchFrom(secNo);

        bm->WriteBack(bitmap); // 2
        delete bm;
        delete bitmap;
        delete fHdr;
    }
    this->numBytes = newFileLength;
    updateLastFileHeaderLength(secNo);
    // for debugging
//    if(DebugIsEnabled('f')){
//        Print();
//    }

}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    time_t ts = lastUpdateTime;
    char *timeFormat = ctime(&ts);
    timeFormat[strlen(timeFormat) - 1] = '\0';
    printf("FileHeader contents.  File size: %d.  File modification time: %s.  "
           "File blocks:\n", numBytes, timeFormat);
    /**
     * Lab5:filesys extension
     * recursively print all file blocks
     */
     FileHeader* fHdr = new FileHeader;
     copyFHdr(fHdr);
     int numSectors;
     while(true){
         if(DebugIsEnabled('f')){
             printf("Current file header has %d sectors,file length is %d,next file header: %d\n",
                    fHdr->calculateNumSectors(),fHdr->FileLength(),fHdr->getIndirect());
         }
         numSectors = fHdr->calculateNumSectors();
         int* ds = fHdr->getDataSectors();
         for(i = 0;i<numSectors;i++){
             printf("%d ", ds[i]);
         }
         printf("\n");
         if(fHdr->getIndirect()==IllegalIndirectSectorNo){
             break;
         }
         fHdr->FetchFrom(fHdr->getIndirect());
     }
    printf("\nFile contents:\n");
     /**
      * Lab5:filesys extension
      * recursively print all file data
      */
    copyFHdr(fHdr);
    while(true){
        int* ds = fHdr->getDataSectors();
        numSectors = fHdr->calculateNumSectors();
        for (i = k = 0; i < numSectors; i++) {
            synchDisk->ReadSector(ds[i], data);
            for (j = 0; (j < SectorSize) && (k < fHdr->FileLength()); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176'){
                    printf("%c", data[j]);
                }else{
                    printf("\\%x", (unsigned char)data[j]);
                }
            }
            printf("\n");
        }
        if(fHdr->getIndirect()==IllegalIndirectSectorNo){
            break;
        }
        fHdr->FetchFrom(fHdr->getIndirect());
    }
    delete fHdr;
    delete [] data;
}

/**
 * Lab4:filesys extension
 * the member numSectors is a calculated attribute
 * it's unnecessary to allocate 4B space for it on the disk
 * this field will be replaced by timestamp of latestUpdateTime
 */
int
FileHeader::calculateNumSectors() {
    int numSectors = divRoundUp(numBytes,SectorSize);
    return numSectors>NumDirect?NumDirect:numSectors;
}

int
FileHeader::getLastUpdateTime() {
    return lastUpdateTime;
}

void
FileHeader::setLastUpdateTime(int latestTs) {
    lastUpdateTime = latestTs;
}

int
FileHeader::getIndirect() {
    return indirect;
}

void
FileHeader::setIndirect(int Indirect) {
    indirect = Indirect;
}

int*
FileHeader::getDataSectors() {
    return dataSectors;
}

void
FileHeader::setDataSectors(BitMap* bitMap,int startNo,int numSectors) {
    DEBUG('f',"Allocating dataSectors,start number: %d\n",startNo);
    int* ds = getDataSectors();
    for(int i=startNo;i<startNo+numSectors;i++){
        int idle = bitMap->Find();
        DEBUG('f',"find idle sector: %d , data sector to be allocated: %d\n",idle,i);
        ds[i] = idle;
        DEBUG('f',"Allocate idle sector %d to data sector %d\n",idle,i);
    }
    DEBUG('f',"Allocating Done\n");
}

void
FileHeader::setFileLength(int fl) {
    numBytes = fl;
}

void
FileHeader::copyDataSectors(int *src,int numSectors) {
    for(int i=0;i<numSectors;i++){
        dataSectors[i] = src[i];
    }
}

void
FileHeader::copyFHdr(FileHeader *dest) {
    dest->setIndirect(indirect);
    dest->setFileLength(numBytes);
    dest->copyDataSectors(dataSectors,calculateNumSectors());
}

void
FileHeader::printDataSectors(int sector) {
    printf("Sector number for this file header : %d\n",sector);
    for(int i=0;i<NumDirect;i++){
        printf("%d",dataSectors[i]);
    }
    printf("\n");
}

int
FileHeader::updateLastFileHeaderLength(int secNo) {
    FileHeader* fHdr = new FileHeader;
    copyFHdr(fHdr);
    //DEBUG('f',"fHdr.fl= % d,fl = %d\n",fHdr->FileLength(),numBytes);
    int fl = fHdr->FileLength();
    int lastSecNo = secNo;
    // get the last file header
    //DEBUG('f',"fl :%d,max: %d\n",fl,SectorSize*NumDirect);
    while(fl>SectorSize*NumDirect){
        fl -= SectorSize*NumDirect;
        //DEBUG('f',"fl :%d,max: %d\n",fl,SectorSize*NumDirect);
        ASSERT(fHdr->getIndirect()!=IllegalIndirectSectorNo);
        lastSecNo = fHdr->getIndirect();
        fHdr->FetchFrom(fHdr->getIndirect());
    }
    fHdr->setFileLength(fl);
    fHdr->WriteBack(lastSecNo);
    //DEBUG('f',"update last file header on sector %d,file length %d\n",lastSecNo,fl);
    delete fHdr;
    return fl;
}
