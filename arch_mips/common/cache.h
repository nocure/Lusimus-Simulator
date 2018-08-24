#ifndef _CACHE_H_
#define _CACHE_H_

#include "types.h"

/* This header describes a physically-indexed, physically-taged MIPS cache.
 * Both direct mapped and n-way set-associative caches are supported. The only
 * restriction is that all geometry parameters must be integer powers of 2.
 * Each line includes 32 bits of uninterpreted cache state.
 */

#define vaddr_width			32 //Shi yang 2006-08-28

/*the parameters of icache */
#define log2_icache_assoc 	  	1
#define log2_icache_size  	    	14
#define log2_icache_line  	  	5

/* the parameters of dcache */
#define log2_dcache_assoc 	 	1
#define log2_dcache_size  	 	14
#define log2_dcache_line  	 	5
#define write_buffers  	  		0
    
#define tlb_size       	 		48
    
#define  kseg_end		TVA(0xFFFF,0xFFFF)

#define Icache_log2_sets  (log2_icache_size - log2_icache_line - log2_icache_assoc)
#define Icache_size   (1 << log2_icache_size)
#define Icache_assoc  (1 << log2_icache_assoc)
#define Icache_line_size  (1 << log2_icache_line)
#define Icache_sets   (1 << log2_icache_size)
#define Icache_index_first (log2_icache_line)
#define Icache_index_last  (Icache_log2_sets - 1 + Icache_index_first)

//for dcache

#define Dcache_log2_sets 			(log2_dcache_size - log2_dcache_line - log2_dcache_assoc)
#define Dcache_size				(1 << log2_dcache_size)
#define Dcache_assoc				(1 << log2_dcache_assoc)
#define Dcache_line_size			(1 << log2_dcache_line)
#define Dcache_sets				(1 << log2_dcache_size)
#define Dcache_index_first			(log2_dcache_line)
#define Dcache_index_last			(Dcache_log2_sets - 1 + Dcache_index_first)

typedef struct MIPSICacheLine_s {
    	UInt32 tag;
    	UInt32 state;
    	UInt32 data[(1 << log2_icache_line) / 8];
}MIPSICacheLine; 

typedef struct MIPSICacheSet_s {
    UInt8 Icache_lru[1 << log2_icache_assoc];
    MIPSICacheLine line[1 << log2_icache_assoc];
}MIPSICacheSet;

//define the type of mips icache
typedef struct MIPSICache_s {
    	MIPSICacheLine line;
    	MIPSICacheSet set[Icache_size];
}ICache;

typedef struct MIPSDCacheLine_s {
    UInt32 tag;
    UInt32 state;
    UInt32 data[(1 << log2_dcache_line) / 8];
}MIPSDCacheLine;

typedef struct MIPSDCacheSet_s {
    UInt8 Dcache_lru[1 << log2_dcache_assoc];
    MIPSDCacheLine line[1 << log2_dcache_assoc];
}MIPSDCacheSet;

typedef struct MIPSDCache_s {
    MIPSDCacheLine line;
    MIPSDCacheSet set[Dcache_size];
}DCache;

void Icache_lru_init(UInt8* Icache_lru);
int Icache_lru_replace(UInt8* Icache_lru);
void Icache_lru_touch(UInt8* Icache_lru, unsigned int n);
VA Icache_index(VA va);
VA Icache_block(VA va);
PA Icache_tag(PA pa);

void Dcache_lru_init(UInt8* Dcache_lru);
int Dcache_lru_replace(UInt8* Dcache_lru);
void Dcache_lru_touch(UInt8* Dcache_lru, int n);
VA Dcache_index(VA va);
VA Dcache_block(VA va);
PA Dcache_tag(PA pa);

#endif //end of _CACHE_H_
