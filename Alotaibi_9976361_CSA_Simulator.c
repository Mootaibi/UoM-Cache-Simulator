/*
* Filename: Cache_Alotaibi9976361.c
* Author: Mohammed Alotaibi
* Date: 14 / 05 / 2021
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/*
* Setup:
* Place directory/filename
* Choose mode as described in lab script
* Set cache size
*/
#define directory "test_file.trc"
const uint32_t MODE_ID = 1;
const uint32_t CACHE_MEMORY = 512;

uint32_t NUMBER_OF_CACHE_LINES(uint32_t cacheMem, uint32_t mode) {
	return pow(2, (sqrt(cacheMem) - mode));
}

uint32_t CACHE_BLOCK_SIZE(uint32_t mode) {
	return pow(2, mode);
}

struct CACHE {
	uint16_t* cache_data;
	uint32_t* cache_tag;
	uint16_t* cache_counter;
	bool* cache_valid;
	bool* cache_dirty;
	uint32_t cache_Latest;
	uint8_t limit;

	uint32_t NOCL, CBS;
	uint32_t NRA, NWA, NCRH, NCRM, NCWH, NCWM;
};

//Empty function, write data to main memory
void mainM_writeData(int BID) {}
void cache_writeData(uint32_t address) {}

//Fully associative cache search, return 1 if found and 0 if not found
bool cache_search(uint32_t address, struct CACHE* cache) {
	for (int i = 0; i < cache->NOCL; i++) {
		if (cache->cache_valid[i] && cache->cache_tag[i] == (address / cache->CBS)) {
			cache->cache_Latest = i;
			cache->cache_counter[i] = 0;
			return 1;
		}
	}
	return 0;
}

//Used to increment cache after every cycle
void cache_counterIncr(struct CACHE* cache) {
	if (cache->limit < 8) {
		for (int i = 0; i < cache->NOCL; i++) {
			cache->cache_counter[i]++;
		}
		cache->limit++;
	}
	else {
		for (int i = 0; i < cache->NOCL; i++) {
			cache->cache_counter[i] - 4;
		}
		cache->limit = 0;
	}
}

//Least recently used algorithm, returns LRU index
uint32_t cache_LRU(struct CACHE* cache) {
	uint32_t lru = 0;
	for (int i = 0; i < cache->NOCL; i++) {
		if (cache->cache_counter[i] >= cache->cache_counter[lru])
			lru = i;
	}
	return lru;
}

//Moves data from main memory into cache memory
void cache_pullData(uint32_t address, struct CACHE* cache) {
	uint32_t lru = cache_LRU(cache);
	if (cache->cache_dirty[lru])
		mainM_writeData(cache->cache_tag[lru]);
	cache->cache_tag[lru] = address / cache->CBS;
	cache->cache_valid[lru] = true;
	cache_writeData(address);
	cache->cache_Latest = lru;
	cache->cache_counter[lru] = 0;;
}

//Data write function
bool cache_write(uint32_t address, struct CACHE* cache) {
	bool check = cache_search(address, cache);
	if (check) {
		cache_writeData(address);
		cache->cache_dirty[cache->cache_Latest] = 1;
		return 1;
	}
	else {
		cache_pullData(address, cache);
		cache_writeData(address);
		cache->cache_dirty[cache->cache_Latest] = 1;
		return 0;
	}
}
 
//Initialize cache size
void cache_initialize(struct CACHE* cache, uint32_t cacheMem, uint32_t mode) {
	cache->NOCL = NUMBER_OF_CACHE_LINES(cacheMem, mode);
	cache->CBS = CACHE_BLOCK_SIZE(mode);
	cache->cache_data = (uint16_t*)malloc(CACHE_MEMORY * sizeof(uint16_t));
	cache->cache_tag = (uint32_t*)malloc(cache->NOCL * sizeof(uint32_t));
	cache->cache_counter = (uint16_t*)malloc(cache->NOCL * sizeof(uint16_t));
	cache->cache_valid = (bool*)malloc(cache->NOCL * sizeof(bool));
	cache->cache_dirty = (bool*)malloc(cache->NOCL * sizeof(bool));
	cache->NRA = cache->NWA = cache->NCRH = cache->NCRM = cache->NCWH = cache->NCWM = 0;
}

int main(void) {
	//Variable instantiation
	FILE* stream;
	uint32_t addr;
	char access;
	bool search;

	//Cache setup
	struct CACHE *cachePtr, myCache;
	cachePtr = &myCache;
	cache_initialize(cachePtr, CACHE_MEMORY, MODE_ID);

	//Safe file opening protocol
	errno_t err = fopen_s(&stream, directory, "r");
	if (err)
		printf_s("The file was not opened\n");
	else
	{
		fseek(stream, 0L, SEEK_SET);
		do {
			fscanf_s(stream, "%c %x", &access, 1, &addr);
			cache_counterIncr(cachePtr);

			switch (access) {
			case 'R':
				myCache.NRA++;
				search = cache_search(addr, cachePtr);
				if (search)
					myCache.NCRH++;
				else {
					myCache.NCRM++;
					cache_pullData(addr, cachePtr);
				}
				break;

			case 'W':
				myCache.NWA++;
				search = cache_write(addr, cachePtr);
				if (search)
					myCache.NCWH++;
				else
					myCache.NCWM++;
				break;
			}
			//printf("%c %x\n", access, addr);
		} while (fgetc(stream) != EOF);
		fclose(stream);

		printf("%s, %d, %d, %d, %d, %d, %d, %d", directory,
			MODE_ID, myCache.NRA, myCache.NWA,
			myCache.NCRH, myCache.NCRM, myCache.NCWH, myCache.NCWM);
	}
}