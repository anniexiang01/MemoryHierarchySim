#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++){
		L1[i].valid = false; 
	}
	
	for (int i=0; i<L2_CACHE_SETS; i++){
		for (int j=0; j<L2_CACHE_WAYS; j++){
			L2[i][j].valid = false; 
			L2[i][j].lru_position = j;
		}
	}

	for (int i=0; i<VICTIM_SIZE; i++){
		Vic[i].valid = false;
		Vic[i].lru_position = i;
	}

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.missVic =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;
	this->myStat.accVic =0;
	
}
void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	if (MemR){ // search
		if (!searchL1(adr, MemR)){
			if (!searchVic(adr, MemR)){
				if (!searchL2(adr, MemR)){
					writeL1(adr, myMem[adr]);
				}
			}
		}
	}
	else { // write
		myMem[adr] = *data; // write-through (write to memory immediately)

		// write to cache only if in cache already (write no allocate)
		if (searchL1(adr, MemR))
			writeL1(adr, *data);
		else if (searchVic(adr, MemR))
			writeVic(adr, *data);
		else if (searchL2(adr, MemR))
			writeL2(adr, *data);
	}

}

Stat cache::getStat()
{
	return this->myStat;
}

bool cache::searchL1(int adr, bool MemR)
{
	if (MemR)
		this->myStat.accL1++;

	int blockOffsetMask = 0b11;  // 2 bits for block offset
    int indexMask = 0b1111;      // 4 bits for index
    int tagMask = 0b11111111111111111111111111;  // 26 bits for tag

    int blockOffset = adr & blockOffsetMask;
    int index = (adr >> 2) & indexMask; 
    int tag = (adr >> 6) & tagMask;

	if (!L1[index].valid || L1[index].tag != tag){ // L1 miss
		if (MemR)
			this->myStat.missL1++;
		return false;
	}
	else { // L1 hit
		return true;
	}
}

bool cache::searchVic(int adr, bool MemR)
{
	if (MemR)
		this->myStat.accVic++;

	for (int i=0; i<VICTIM_SIZE; i++){
		if (Vic[i].valid && Vic[i].tag == adr){ // victim cache hit

			//update LRU positions
			for (int i2=0; i2<VICTIM_SIZE; i2++){
				if (Vic[i2].lru_position > Vic[i].lru_position){
					Vic[i2].lru_position--;
				}
			}

			writeL1(adr, Vic[i].data); // move data to L1
			Vic[i].lru_position = 3;
			Vic[i].valid = 0;
			return true;
		}
	}

	if (MemR)
		this->myStat.missVic++;
	return false;
}

bool cache::searchL2(int adr, bool MemR)
{
	if (MemR)
		this->myStat.accL2++;

	int blockOffsetMask = 0b11;  // 2 bits for block offset
	int indexMask = 0b1111;      // 4 bits for index
	int tagMask = 0b11111111111111111111111111;  // 26 bits for tag

	int blockOffset = adr & blockOffsetMask;
	int index = (adr >> 2) & indexMask; 
	int tag = (adr >> 6) & tagMask;

	for (int j=0; j<L2_CACHE_WAYS; j++) {
		if (L2[index][j].valid && L2[index][j].tag == tag){ // L2 hit
			
			// update LRU positions
			for (int j2=0; j2<L2_CACHE_WAYS; j2++){
				if (L2[index][j2].lru_position > L2[index][j].lru_position){
					L2[index][j2].lru_position--;
				}
			}

			writeL1(adr, L2[index][j].data); // move data to L1
			L2[index][j].lru_position = 7;
			L2[index][j].valid = 0;
			return true;
		}
	}

	if (MemR)
		this->myStat.missL2++;
	return false;
}

void cache::writeL1(int adr, int data)
{
	int blockOffsetMask = 0b11;  // 2 bits for block offset
	int indexMask = 0b1111;      // 4 bits for index
	int tagMask = 0b11111111111111111111111111;  // 26 bits for tag

	int blockOffset = adr & blockOffsetMask;
	int index = (adr >> 2) & indexMask; 
	int tag = (adr >> 6) & tagMask;

	//store at index (evict page and store in victim cache)
	if (L1[index].valid){
		writeVic(L1[index].address, L1[index].data);
	}

	L1[index].data = data;
	L1[index].tag = tag;
	L1[index].valid = 1;
	L1[index].address = adr;
}

void cache::writeVic(int adr, int data)
{
	//search for LRU block and evict it to L2
	for (int i=0; i<VICTIM_SIZE; i++){
		if (Vic[i].lru_position == 3){
			if (Vic[i].valid){
				writeL2(Vic[i].address, Vic[i].data);
			}
			Vic[i].data = data;
			Vic[i].tag = adr;
			Vic[i].valid = 1;
			Vic[i].address = adr;

			//update LRU positions
			for (int i2=0; i2<VICTIM_SIZE; i2++){
				Vic[i2].lru_position++;
			}
			Vic[i].lru_position = 0;

			break;
		}
	}
}

void cache::writeL2(int adr, int data)
{
	int blockOffsetMask = 0b11;  // 2 bits for block offset
	int indexMask = 0b1111;      // 4 bits for index
	int tagMask = 0b11111111111111111111111111;  // 26 bits for tag

	int blockOffset = adr & blockOffsetMask;
	int index = (adr >> 2) & indexMask; 
	int tag = (adr >> 6) & tagMask;

	//search for LRU block and evict
	for (int j=0; j<L2_CACHE_WAYS; j++) {
		if (L2[index][j].lru_position == 7){
			L2[index][j].data = data;
			L2[index][j].tag = tag;
			L2[index][j].valid = 1;
			L2[index][j].address = adr;

			//update LRU positions
			for (int j2=0; j2<L2_CACHE_WAYS; j2++){
				L2[index][j2].lru_position++;
			}
			L2[index][j].lru_position = 0;

			break;
		}
	}
}

