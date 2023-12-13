#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data; // the actual data stored in the cache/memory
	bool valid;
	int address;
};

struct Stat
{
	int missL1; 
	int missL2; 
	int missVic;
	int accL1;
	int accL2;
	int accVic;
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	cacheBlock Vic[VICTIM_SIZE]; // 1 block per row, 4 rows
	
	Stat myStat;
	// add more things here
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	Stat getStat();

	bool searchL1(int adr, bool MemR);
	bool searchVic(int adr, bool MemR);
	bool searchL2(int adr, bool MemR);

	void writeL1(int adr, int data);
	void writeVic(int adr, int data);
	void writeL2(int adr, int data);
};


