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
FileHeader::Allocate(BitMap *freeMap, int fileSize)
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
    int totalSectors = numSectors + divRoundUp(numSectors,NumDirect)-1;
    if (freeMap->NumClear() < totalSectors)
	return FALSE;		// not enough space

    // recursively allocate dataSectors
    AllocateEachFHdr(freeMap,0,numSectors,true,0);
    return TRUE;
}

/**
 * recursive allocate the dataSectors
 * @param bitMap
 * @param startNo start sector number to be allocated
 * @param restSectors rest sectors that haven't been allocated on the bitmap
 * @param isOrigin whether is the origin file header
 * @param curSectorNo current file header's sectorNo
 */
void
FileHeader::AllocateEachFHdr(BitMap *bitMap,int startNo,int restSectors,bool isOrigin,int curSectorNo) {
    if(restSectors<=0){
        // nothing left to allocate
        return;
    }

    int numSectors = restSectors>NumDirect?NumDirect:restSectors;
    int nextFHdrSectorNo = bitMap->Find();
    if(isOrigin){
        // the next recursion will be non-origin
        isOrigin = !isOrigin;
        // find a sector for external file header
        indirect = nextFHdrSectorNo;
        // allocate some of the dataSectors
        setDataSectors(bitMap,startNo,numSectors);
        // it's ok to keep the first file header in the RAM
        // we'll write it back by invoking WriteBack function manually
    }else{
        // not the original file header,create one
        FileHeader* fHdr = new FileHeader;
        // in case it has been initialized
        fHdr->FetchFrom(curSectorNo);
        // find a sector for external file header
        fHdr->setIndirect(nextFHdrSectorNo);
        // set the file length
        fHdr->setFileLength(numSectors*SectorSize);
        // allocate some of the dataSectors
        fHdr->setDataSectors(bitMap,startNo,numSectors);
        // write the external fileHeader back to the disk immediately
        fHdr->WriteBack(curSectorNo);
        // important to free the mem space
        delete fHdr;
    }
    // recursively allocate the next file header
    // ought to start at 0!
    AllocateEachFHdr(bitMap,0,restSectors-numSectors,isOrigin,nextFHdrSectorNo);
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    int numSectors = calculateNumSectors()>NumDirect?NumDirect:calculateNumSectors();
    for (int i = 0; i < numSectors; i++) {
        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
    }
    // no more external file headers anymore
    indirect = IllegalIndirectSectorNo;
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
FileHeader::updateFileLength(int newFileLength) {
    //calculate current used sectors
    int curSectors = divRoundUp(newFileLength,SectorSize);
    int oldSectors = calculateNumSectors();
    if(curSectors>oldSectors){

        // find the last file header
        FileHeader* fHdr = new FileHeader;
        fHdr->setIndirect(indirect);
        while(fHdr->getIndirect()!=IllegalIndirectSectorNo){
            fHdr->FetchFrom(fHdr->getIndirect());
            curSectors -= NumDirect;
        }

        // where to start append
        int startNo = fHdr->calculateNumSectors()+1;

        int bitmapSecNo = 0;
        // fetch the bit map from disk
        OpenFile* bitmap = new OpenFile(bitmapSecNo);
        // load the bitmap to mem
        BitMap* bm = new BitMap(NumSectors);
        bm->FetchFrom(bitmap);

        // allocate a block to next file header
        int nextFHdrNo = bm->Find();
        ASSERT(nextFHdrNo!=-1);
        // for the last file header,make changes to the bit map
        AllocateEachFHdr(bitmap,startNo,curSectors,fHdr->getIndirect()==indirect,nextFHdrNo);

        bm->WriteBack(bitmap);
        delete bm;
        delete bitmap;

    }
    this->numBytes = newFileLength;
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
    int numSectors = calculateNumSectors();
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
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
    return divRoundUp(numBytes,SectorSize);
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

void
FileHeader::getDataSectors() {
    return dataSectors;
}

void
FileHeader::setDataSectors(BitMap* bitMap,int startNo,int numSectors) {
    ASSERT(startNo<=numSectors);
    for(int i=startNo;i<numSectors;i++){
        dataSectors[i] = bitMap->Find();
    }
}

void
FileHeader::setFileLength(int fl) {
    numBytes = fl;
}
