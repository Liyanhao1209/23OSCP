// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

#define NumDirect 	(int)((SectorSize - 3 * sizeof(int)) / sizeof(int))
#define MaxFileSize 	(NumDirect * SectorSize)

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {
  public:
    FileHeader();

    bool Allocate(BitMap *bitMap, int fileSize, int secNo);// Initialize a file header,
						//  including allocating space 
						//  on disk for the file data
    void Deallocate(BitMap *bitMap,int curSectorNogit);  		// De-allocate this file's
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    int FileLength();			// Return the length of the file 
					// in bytes

    void Print();			// Print the contents of the file.

    /**
     * Lab4:filesys extension
     * Reasonable since we support dynamic length file
     */
    void updateFileLength(int newFileLength,int secNo);

    int calculateNumSectors();

    int getLastUpdateTime();

    void setLastUpdateTime(int latestTs);

    int getIndirect();

    void setIndirect(int Indirect);

    int* getDataSectors();

    void setDataSectors(BitMap* bitMap,int startNo,int numSectors);

    void setFileLength(int fl);

    void printDataSectors(int sector);

    /**
     * Lab5:filesys extension
     * copy file header to another file header node to iterate the whole file
     */
     void copyFHdr(FileHeader* dest);

     /**
      * Lab5:filesys extension
      * update the last external file header's length
      * be sure you invoke this function on the original fHdr pointer
      * @return the new length of last file header
      */
     int updateLastFileHeaderLength(int secNo);

  private:
    int numBytes;			// Number of bytes in the file
    /**
     * Lab4:filesys extension
     * record the last update time in the file header
     */
    int lastUpdateTime;
    /**
     * Lab5:filesys extension
     * second level index to extend a single file's max size
     */
    int indirect;
    int dataSectors[NumDirect];		// Disk sector numbers for each data block in the file

    int AllocateEachFHdr(BitMap *bitMap,int startNo,int restSectors,int curSectorNo);

    void copyDataSectors(int *src,int numSectors);
};

#endif // FILEHDR_H
