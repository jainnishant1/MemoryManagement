#ifndef __MEMMGR_H_
#define __MEMMGR_H_

#include<bits/stdc++.h>
#include <vector>
#include <set>
#include <map>

const int BIT_MAP_SIZE = 2048;
const int INT_SIZE = sizeof(int) * 8;
const int BIT_MAP_ELEMENTS = BIT_MAP_SIZE / INT_SIZE;

//Memory Allocation Pattern
//11111111 11111111 11111111
//11111110 11111111 11111111
//11111100 11111111 11111111
//if all bits for 1st section become 0 proceed to next section

//...
//00000000 11111111 11111111
//00000000 11111110 11111111
//00000000 11111100 11111111
//00000000 11111000 11111111

//The reason for this strategy is that lookup becomes O(1) inside the map 
//for the first available free block

#include <bitset>

class Complex;
typedef struct BitMapEntry
  {
  int      Index;
  int      BlocksAvailable;
  int      BitMap[BIT_MAP_SIZE];
  public:
    BitMapEntry():BlocksAvailable(BIT_MAP_SIZE)
      {
      memset(BitMap,0xff,BIT_MAP_SIZE / sizeof(char)); 
      // initially all blocks are free and bit value 1 in the map denotes 
      // available block
      }
    void SetBit(int position, bool flag);
    void SetMultipleBits(int position, bool flag, int count);
    void SetRangeOfInt(int* element, int msb, int lsb, bool flag);
    Complex* FirstFreeBlock(size_t size);
    Complex* ComplexObjectAddress(int pos);
    void* Head();
  }
BitMapEntry;


typedef struct ArrayInfo
  {
  int   MemPoolListIndex;
  int   StartPosition;
  int   Size;
  }
ArrayMemoryInfo;

class IMemoryManager 
  {
  public:
    virtual void* allocate(size_t) = 0;
    virtual void   free(void*) = 0;
  };

class MemoryManager : public IMemoryManager 
  {
    std::vector<void*> MemoryPoolList;
    std::vector<BitMapEntry> BitMapEntryList;
    //the above two lists will maintain one-to-one correpondence and hence 
    //should be of same  size.
    std::set<BitMapEntry*> FreeMapEntries;
    std::map<void*, ArrayMemoryInfo> ArrayMemoryList;

  private:
    void* AllocateArrayMemory(size_t size);
    void* AllocateChunkAndInitBitMap();
    void SetBlockBit(void* object, bool flag);
    void SetMultipleBlockBits(ArrayMemoryInfo* info, bool flag);
  public: 
     MemoryManager( ) {}
    ~MemoryManager( ) {}
    void* allocate(size_t);
    void   free(void*);
    std::vector<void*>& GetMemoryPoolList(); 
  };

#endif