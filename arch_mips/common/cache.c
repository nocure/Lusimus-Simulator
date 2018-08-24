#include "instr.h"
#include "emul.h"

int decode_cache(MIPS_State* mstate, Instr instr) //Use in Cache Instruction, it's unuseable in R3000
{
	// CP0 is usable in kernel mode or when the CU bit in SR is set.
	if (!(mstate->mode & kmode) && !bit(mstate->cp0[SR], SR_CU0))
		process_coprocessor_unusable(mstate, 0);

	VA va = sign_extend_UInt32(offset(instr), 16) + mstate->gpr[base(instr)];
	PA pa;
	if (translate_vaddr(mstate, va, cache_op, &pa) != 0) {
		return nothing_special;
	}
	if (pa != bad_pa) {
		if (bit(instr, 16)) {
			// Control data cache.
			control_dcache(mstate, va, pa, bits(instr, 20, 18), bit(instr, 17));
		} else {
			// Control instruction cache.
			control_icache(mstate, va, pa, bits(instr, 20, 18), bit(instr, 17));
		}
	}
	return nothing_special;
}

// Initialize the LRU data.
void Icache_lru_init(UInt8* Icache_lru) {
	UInt8 i;
	for (i = 0; i < (UInt8) Icache_assoc; ++i)
		Icache_lru[i] = i;
}

int Icache_lru_replace(UInt8* Icache_lru) {
	return Icache_lru[Icache_assoc - 1];
}

// Touch a cache line (mark it as most-recently used.)
void Icache_lru_touch(UInt8* Icache_lru, unsigned int n) {
	// Set lru[0] to n and shift the other values accordingly.
	if ((int) Icache_lru[0] != n) {
		unsigned int i;
		for (i = n; i; --i)
			Icache_lru[i] = Icache_lru[i - 1];
		Icache_lru[0] = n;
	}
}

void Icache_lru_init(UInt8* Icache_lru);
int Icache_lru_replace(UInt8* Icache_lru);
void Icache_lru_touch(UInt8* Icache_lru, unsigned int n);
VA Icache_index(VA va);
VA Icache_index(VA va) {
	return bits(va, Icache_index_last, Icache_index_first);
}

VA Icache_block(VA va) {
	return bits(va, Icache_index_last + log2_icache_assoc,
			Icache_index_last + 1);
}

PA Icache_tag(PA pa) {
	return bits(pa, paddr_width - 1, Icache_index_last + 1);
}

// Initialize the LRU data.
void Dcache_lru_init(UInt8* Dcache_lru) {
	UInt8 i;
	for (i = 0; i < Dcache_assoc; ++i)
		Dcache_lru[i] = i;
}

int Dcache_lru_replace(UInt8* Dcache_lru) {
	return Dcache_lru[Dcache_assoc - 1];
}

// Touch a cache line (mark it as most-recently used.)
void Dcache_lru_touch(UInt8* Dcache_lru, int n) {
	// Set lru[0] to n and shift the other values accordingly.
	if (Dcache_lru[0] != n) {
		unsigned i;
		for (i = n; i; --i)
			Dcache_lru[i] = Dcache_lru[i - 1];
		Dcache_lru[0] = n;
	}
}

VA Dcache_index(VA va) {
	return bits(va, Dcache_index_last, Dcache_index_first);
}

VA Dcache_block(VA va) {
	return bits(va, Dcache_index_last + log2_dcache_assoc,
			Dcache_index_last + 1);
}

PA Dcache_tag(PA pa) {
	return bits(pa, paddr_width - 1, Dcache_index_last + 1);
}

/* The caching algorithm is stored as part of the physical address,
 * using the same encoding as that of XKPHYS address space region.
 */
int coherency_algorithm(PA pa) //Shi yang 2006-08-08
{
	return bits(pa, 31, 29);
}

