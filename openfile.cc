// openfile.cc 
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"
#include "time.h"

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{ 
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    if(DebugIsEnabled('f')){
        //hdr->printDataSectors(sector);
    }
    /**
     * Lab4:filesys extension
     * if we want to write the file header back to the disk automatically
     * when the file is closed,namely,the ~OpenFile() destructor has been invoked
     * we should know which sector on the disk should the file header be written into
     */
    fileHeaderSectorNo = sector;
    seekPosition = 0;
    latestLength = hdr->FileLength();
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    /**
     * Lab4:filesys extension
     * b:write the file header back to the disk no matter whether the file length changed
     */
    //hdr->WriteBack(fileHeaderSectorNo);
    /**
     * Lab4:filesys extension
     * c:write the file header back to the disk only if the file length has been changed
     * happens when the caller of WriteAt forget to invoke WriteHeaderBack() manually
     */
     if(latestLength!=hdr->FileLength()){
         hdr->FetchFrom(fileHeaderSectorNo);
         // time(NULL) returns the passed seconds since UTC 1970.1.1:00:00:00
         // the type of return value is time_t,which is the same size of type int
         // so the forcing cast is reasonable
         if(this->latestLength!=0){
             hdr->setLastUpdateTime((int)time(NULL));
         }
         hdr->WriteBack(fileHeaderSectorNo);
     }
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes)
{
   int result = ReadAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

int
OpenFile::Write(char *into, int numBytes)
{
   int result = WriteAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
    	return 0; 				// check request
    if ((position + numBytes) > fileLength)		
	numBytes = fileLength - position;
    //DEBUG('f', "Reading %d bytes at %d, from file of length %d.File Sector number: %d\n",numBytes, position, fileLength,fileHeaderSectorNo);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);

    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete [] buf;
    return numBytes;
}

int
OpenFile::WriteAt(char *from, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    int newFileLength = position+numBytes;

//    if ((numBytes <= 0) || (position >= fileLength))  // For original Nachos file system
    /**
     * Lab5:filesys extension
     * the maxFileSize has been changed since we have second level file header index structure
     * We may make a new max file size limit instead
     */
    if ((numBytes <= 0) || (position > fileLength))  // For lab5 ...
	return 0;				// check request

    /**
     * Lab4:filesys extension
     * check if we add some new bytes to this file
     */
    if(newFileLength > fileLength){
        hdr->updateFileLength(newFileLength,fileHeaderSectorNo); // 5 0 2
    }

    /**
     * Lab4:filesys extension
     * Now we are going to make the file length changeable
     * Thus it's necessary for us to break this rule
     */
//    if ((position + numBytes) > fileLength)
//	numBytes = fileLength - position;

    //DEBUG('f', "Writing %d bytes at %d, from file of length %d.File Sector number: %d\n",numBytes, position, fileLength,fileHeaderSectorNo);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    firstAligned = (bool)(position == (firstSector * SectorSize));
    lastAligned = (bool)((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);	
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

// copy in the bytes we want to change 
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

    //DEBUG('f',"Writing file data to disk\n");

// write modified sectors back
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
    delete [] buf;
    return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength(); 
}

/**
 * Lab4:filesys extension
 * since hdr is a private member in class OpenFile
 * we should write the file header back after the file has been changed
 * by manually invoking an exported function of an instance of class OpenFile
 */
void
OpenFile::WriteHeaderBack(int sectorNo) {
    //hdr->setLastUpdateTime((int)time(NULL));
    //this means the caller should pass the correct sector number of the file header on disk
    this->hdr->WriteBack(sectorNo);
}

FileHeader*
OpenFile::getFileHeader() {
    return this->hdr;
}
