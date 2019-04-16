/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/**
 *  author chenyu <yuchen@tsinghua.edu.cn>
 *  teawater <c7code-uc@yahoo.com.cn> add elf load function in 2005.08.30
 */

#ifdef __CYGWIN__
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Anthony Lee 2006-08-22 : for Win32API
#ifdef __MINGW32__
#undef WORD
#undef byte
#include <windows.h>
#endif

#define PROGRAM_NICK_NAME "Lusimus"
#define PROGRAM_REAL_NAME "Lusimus 0.1.2"

/* we place bfd library into our svn, so we always have bfd library available. */
#define HAVE_LIBBFD 1

#include "skyeye_types.h"
#include "skyeye_defs.h"
#include "skyeye_config.h"
#include "skyeye_uart.h"
#include "profile/code_cov.h"

#include <setjmp.h>

/**
 * A global variable , point to the current archtecture
 */
generic_arch_t *arch_instance = NULL;

/**
 *  name of current config file
 */
char *skyeye_config_filename = NULL;

/*
 *  the file description of log
 */
extern FILE *skyeye_logfd;
extern int debugmode;

int big_endian = 0;
int global_argc;
char **global_argv;
int gdb_port = DEFAULT_GDB_PORT;
jmp_buf ctrl_c_cleanup;

int sim_debug(int port);
void sim_resume(int step);

void initialize_all_arch();
int skyeye_option_init(skyeye_config_t * config);
void mem_reset(void * state);
int remote_interrupt();

#if 0
static void
base_termios_exit (void)
{
	//tcsetattr (STDIN_FILENO, TCSANOW, &(state->base_termios));
}
#endif
extern int init_register_type();
/**
 *  Initialize all the gloval variable
 */
static int init() {
	static int done;
	int ret;
	if (!done) {
		done = 1;
		/*some option should init before read config. e.g. uart option. */
		initialize_all_devices();
		initialize_all_arch();
		/* parse skyeye.conf to set skyeye_config */
		skyeye_option_init(&skyeye_config);
		if ((ret = skyeye_read_config()) < 0) return ret;

		/* we should check if some members of skyeye_config is initialized */
		if (!skyeye_config.arch) {
			fprintf(stderr, "arch is not initialization or you have not provide arch option in %s.\n", skyeye_config_filename);
			skyeye_exit(-1);
		}
		if (!skyeye_config.mach) {
			fprintf(stderr, "mach is not initialization or you have not provide mach option in %s.\n", skyeye_config_filename);
			skyeye_exit(-1);
		}

		/* initialize register type for gdb server */
		if ((ret = init_register_type()) < 0) return ret;/* Failed to initialize register type */

		arch_instance = (generic_arch_t *)malloc(sizeof(generic_arch_t));
		if (!arch_instance) {
			printf("malloc error!\n");
			return -1;
		}
		arch_instance->init = skyeye_config.arch->init;
		arch_instance->reset = skyeye_config.arch->reset;
		arch_instance->step_once = skyeye_config.arch->step_once;
		arch_instance->set_pc = skyeye_config.arch->set_pc;
		arch_instance->get_pc = skyeye_config.arch->get_pc;
		arch_instance->ICE_write_byte = skyeye_config.arch->ICE_write_byte;
		arch_instance->ICE_read_byte = skyeye_config.arch->ICE_read_byte;

		arch_instance->init();
		arch_instance->reset();
		arch_instance->big_endian = big_endian;

		skyeye_uart_converter_setup();
		if (skyeye_config.code_cov.prof_on) cov_init(skyeye_config.code_cov.start, skyeye_config.code_cov.end);

		mem_reset(0); /* initialize of memory module */
	}

	return 1;
}

#include "armemu.h"
#include "skyeye2gdb.h"
extern ARMul_State * state;
extern struct SkyEye_ICE skyeye_ice;

/**
 * add by michael.Kang, to load elf file to another address
 */
// ww 2018-0815 this should be int, not long
unsigned int load_base = 0x0;
unsigned int load_mask = 0xffffffff;

//teawater add for load elf 2005.07.31------------------------------------------
static inline void tea_write(uint32_t addr, uint8_t * buffer, int size) {
	int i, fault;
	addr = (addr & load_mask) | load_base;
	for (i = 0; i < size; i++) {
		if (arch_instance->ICE_write_byte)
			fault = arch_instance->ICE_write_byte(addr + i, buffer[i]);
		else
			fault = -1;
		if (fault) {
			printf("tea_write error!!!\n");
			skyeye_exit(-1);
		}
	}
}

#ifndef HAVE_LIBBFD
#include <elf32.h>

#ifdef __MINGW32__
#include <io.h>
#undef O_RDONLY
#define O_RDONLY			(_O_RDONLY | _O_BINARY)
#define open(file, flags)		_open(file, flags)
#define close(fd)			_close(fd)
#endif /* __MINGW32__ */

static inline void
tea_set(uint32_t addr, uint8_t value, int size)
{
	int i,fault;

	addr = (addr & load_mask)|load_base;
	for (i = 0; i < size; i++) {
		fault=arch_instance->ICE_write_byte (addr + i, value);
		if(fault) {printf("tea_set error!!!\n");skyeye_exit(-1);}
	}
}

/* These function convert little-endian ELF datatypes
 into host endianess values. */

#ifdef HOST_IS_BIG_ENDIAN
uint16_t
e2h16(uint16_t x)
{
	if (big_endian) return x;
	return ((x & 0xff) << 8) | (x >> 8);
}

uint32_t
e2h32(uint32_t x)
{
	if (big_endian) return x;
	return ((x & 0xff) << 24) |
	(((x >> 8) & 0xff) << 16) |
	(((x >> 16) & 0xff) << 8) |
	(((x >> 24) & 0xff));
}
#else
uint16_t
e2h16(uint16_t x) {
	if (!big_endian) return x;
	return ((x & 0xff) << 8) | (x >> 8);
}

uint32_t
e2h32(uint32_t x) {
	if (!big_endian) return x;
	return ((x & 0xff) << 24) |
	(((x >> 8) & 0xff) << 16) |
	(((x >> 16) & 0xff) << 8) |
	(((x >> 24) & 0xff));
}
#endif /* HOST_IS_BIG_ENDIAN */

static int
elf32_checkFile(struct Elf32_Header *file)
{
	if (file->e_ident[EI_MAG0] != ELFMAG0
			|| file->e_ident[EI_MAG1] != ELFMAG1
			|| file->e_ident[EI_MAG2] != ELFMAG2
			|| file->e_ident[EI_MAG3] != ELFMAG3)
	return -1; /* not an elf file */
	if (file->e_ident[EI_CLASS] != ELFCLASS32)
	return -2; /* not 32-bit file */
	switch (e2h16(file->e_machine)) {
		case EM_ARM:
		case EM_BLACKFIN:
		case EM_COLDFIRE:
		case EM_MIPS:
		case EM_PPC:
		case EM_SPARC:
		break;
		default:
		return -3;
	}
	return 0; /* elf file looks OK */
}

static int
tea_load_exec(const char *file, int only_check_big_endian)
{
	int ret = -1;
	int i;
	int tmp_fd;
	int r;
	struct Elf32_Header *elfFile;
	struct stat stat;
	struct Elf32_Phdr *segments;

	tmp_fd = open(file, O_RDONLY);
	if (tmp_fd == -1) {
		fprintf (stderr, "open %s error: %s\n", file, strerror(errno));
		goto out;
	}

	fstat(tmp_fd, &stat);

	/* malloc */
	elfFile = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, tmp_fd, 0);
	if (elfFile == NULL || elfFile == MAP_FAILED) {
		fprintf (stderr, "mmap error: %s\n", strerror(errno));
		goto out;
	}

	big_endian = (elfFile->e_ident[EI_DATA] == ELFDATA2MSB);
	if (only_check_big_endian) goto out;

	r = elf32_checkFile(elfFile);
	if (r != 0) {
		fprintf (stderr, "elf_checkFile failed: %d\n", r);
		goto out;
	}

	segments = (struct Elf32_Phdr*) (uintptr_t) (((uintptr_t) elfFile) + e2h32(elfFile->e_phoff));

	for(i=0; i < e2h16(elfFile->e_phnum); i++) {
		/* Load that section */
		uint32_t dest;
		char *src;
		size_t filesz = e2h32(segments[i].p_filesz);
		size_t memsz = e2h32(segments[i].p_memsz);
		dest = e2h32(segments[i].p_paddr);
		src = ((char*) elfFile) + e2h32(segments[i].p_offset);
		tea_write(dest, src, filesz);
		dest += filesz;
		tea_set(dest, 0, memsz - filesz);
	}

	if (skyeye_config.start_address == 0) {
		skyeye_config.start_address = e2h32(elfFile->e_entry);
	}

	ret = 0;
	out:
	if (tmp_fd != -1)
	close(tmp_fd);
	if (elfFile)
	munmap(elfFile, stat.st_size);
	return(ret);
}
#else //#ifndef HAVE_LIBBFD

//teawater add for load elf 2005.07.31------------------------------------------
#include <bfd.h>

static int tea_load_exec(const char *file, int only_check_big_endian) {
	int ret = -1;
	bfd *tmp_bfd = NULL;
	asection *s;
	uint8_t *tmp_str = NULL;

	/* open */
	tmp_bfd = bfd_openr(file, NULL);
	if (tmp_bfd == NULL) {
		fprintf(stderr, "open %s error: %s\n", file, bfd_errmsg(bfd_get_error()));
		goto out;
	}
	if (!bfd_check_format(tmp_bfd, bfd_object)) {
		/* FIXME:In freebsd, if bfd_errno is bfd_error_file_ambiguously_recognized,
		 * though bfd can't recognize this format, we should try to load file.*/
		if (bfd_get_error() != bfd_error_file_ambiguously_recognized) {
			fprintf(stderr, "check format of %s error: %s\n", file, bfd_errmsg(bfd_get_error()));
			goto out;
		}
	}

	big_endian = bfd_big_endian(tmp_bfd);
	if (only_check_big_endian) goto out;

	printf("exec file \"%s\"'s format is %s.\n", file, tmp_bfd->xvec->name);

	/* load the corresponding section to memory */
	for (s = tmp_bfd->sections; s; s = s->next) {
		if (bfd_get_section_flags (tmp_bfd, s) & (SEC_LOAD)) {
			if (bfd_section_lma (tmp_bfd, s) != bfd_section_vma(tmp_bfd, s)) {
				printf("load section %s: lma = 0x%08x (vma = 0x%08x)  size = 0x%08x.\n", bfd_section_name(tmp_bfd, s), (unsigned int)bfd_section_lma(tmp_bfd, s), (unsigned int)bfd_section_vma(tmp_bfd, s), (unsigned int)bfd_section_size(tmp_bfd, s));
			}
			else {
				printf("load section %s: addr = 0x%08x  size = 0x%08x.\n", bfd_section_name(tmp_bfd, s), (unsigned int)bfd_section_lma(tmp_bfd, s), (unsigned int)bfd_section_size(tmp_bfd, s));
			}
			if (bfd_section_size (tmp_bfd, s) > 0) {
				tmp_str = malloc(bfd_section_size(tmp_bfd, s));
				if (!tmp_str) {
					fprintf(stderr, "alloc memory to load session %s error.\n", bfd_section_name(tmp_bfd, s));
					goto out;
				}
				if (!bfd_get_section_contents(tmp_bfd, s, tmp_str, 0, bfd_section_size(tmp_bfd, s))) {
					fprintf(stderr, "get session %s content error: %s\n", bfd_section_name(tmp_bfd, s), bfd_errmsg(bfd_get_error()));
					goto out;
				}
				tea_write(bfd_section_vma(tmp_bfd, s), tmp_str, bfd_section_size(tmp_bfd, s));
				free(tmp_str);
				tmp_str = NULL;
			}
		}
		else {
			printf("not load section %s: addr = 0x%08x  size = 0x%08x .\n", bfd_section_name(tmp_bfd, s), (unsigned int)bfd_section_vma(tmp_bfd, s), (unsigned int)bfd_section_size(tmp_bfd, s));
		}
	}

	//set start address
	if (skyeye_config.start_address == 0) {
		skyeye_config.start_address = bfd_get_start_address(tmp_bfd);
	}

	ret = 0;
	out: if (tmp_str) free(tmp_str);
	if (tmp_bfd) bfd_close(tmp_bfd);
	return (ret);
}

#endif //#ifndef HAVE_LIBBFD

static void display_arch_support(const char * arch, machine_config_t machines[]) {
	int i;
	printf("-------- %s architectures ---------\n", arch);

	for (i = 0; machines[i].machine_name; i++)
		printf("%s \n", machines[i].machine_name);
}

extern machine_config_t arm_machines[];
extern machine_config_t bfin_machines[];
extern machine_config_t coldfire_machines[];
extern machine_config_t mips_machines[];
extern machine_config_t ppc_machines[];
extern machine_config_t sparc_machines[];
static void usage(void) {

	printf("Usage: %s [options] -e program [program args]\n", PROGRAM_NICK_NAME);
	printf("Default mode is STANDALONE mode\n");
	printf("------------------------------------------------------------------\n");
	printf("Options:\n");
//teawater add for load elf 2005.07.31------------------------------------------
	printf("-e exec-file        the (ELF executable format)kernel file name.\n");
	printf("-l load_address,load_address_mask\n");
	printf("                    Load ELF file to another address, not its entry.\n");
//AJ2D--------------------------------------------------------------------------
	printf( /* 2007-03-29 by Anthony Lee : for specify big endian when non ELF */
	"-b                  specify the data type is big endian when non \"-e\" option.\n");
	printf("-d                  in GDB Server mode (can be connected by GDB).\n");
	printf("-p port number      GDB Server port (default is %d).\n", DEFAULT_GDB_PORT);
	printf("-c config-file      Specify a configure file.\n");
	printf("-h                  List command options, ARCHs and CPUs supported.\n");
	printf("------------------------------------------------------------------\n");
	printf("----------- Architectures and CPUs simulated -------------\n");

	display_arch_support("ARM", arm_machines);
	display_arch_support("BlackFin", bfin_machines);
	display_arch_support("Coldfire", coldfire_machines);
	display_arch_support("MIPS", mips_machines);
	display_arch_support("PowerPC", ppc_machines);
	display_arch_support("Sparc", sparc_machines);
}

void skyeye_exit(int ret) {
	/*
	 * 2007-01-24 removed the term-io functions by Anthony Lee,
	 * moved to "device/uart/skyeye_uart_stdio.c".
	 */
	exit(ret);
}

#ifdef __MINGW32__
	static BOOL init_win32_socket() {
		WSADATA wsdData;
		if (WSAStartup(0x202, &wsdData) != 0 || LOBYTE(wsdData.wVersion) != 2 || HIBYTE(wsdData.wVersion) != 2) return FALSE;
		return TRUE;
	}

	static void cancel_win32_socket() {
		WSACleanup();
	}
#endif

#ifndef __BEOS__
	/* 2007-01-31 disabled by Anthony Lee on BeOS for multi-thread safe. */
	void sigint_handler(int signum) {
		if (skyeye_config.code_cov.prof_on) cov_fini(skyeye_config.code_cov.prof_filename);
// ww 2009-0918 disabled longjmp for mingw32, that causes application error in windows
#ifndef __MINGW32__
		longjmp (ctrl_c_cleanup, 1);
#else
		// TODO: ask for exit
		exit(0);
#endif
	}
#endif

	/**
	 *  The main function of skyeye
	 */
//-eD:\work\tx_sys.full\Main\Debug\Main.elf

#define __CONFIG_Release__ ""
#define __CONFIG_Debug__ " (DEBUG)"
int main(int argc, char **argv) {
	int c;
	int index;
	int ret = 0;
//teawater add for load elf 2005.07.31------------------------------------------
	char *exec_file = NULL;

	// 2018-0605 ww unify the title.
	printf("%s%s built on %s at %s\n",PROGRAM_REAL_NAME, CONFIG_NAME, __DATE__, __TIME__);

	opterr = 0;

	/*
	 * 2007-01-24 removed the term-io functions by Anthony Lee,
	 * moved to "device/uart/skyeye_uart_stdio.c".
	 */

#ifdef __MINGW32__
	init_win32_socket();
	atexit(cancel_win32_socket);
#endif

#ifndef __BEOS__
	/* 2007-01-31 disabled by Anthony Lee on BeOS for multi-thread safe. */
// ww 2009-0918 disabled longjmp for mingw32, that causes application error in windows
#ifndef __MINGW32__
	if (setjmp (ctrl_c_cleanup) != 0) {
		goto exit_skyeye;
	}
#endif

	signal(SIGINT, sigint_handler);
#endif

	while ((c = getopt(argc, argv, "be:dp:c:l:h")) != -1)
//AJ2D--------------------------------------------------------------------------
		switch (c) {
//teawater add for load elf 2005.07.31------------------------------------------
		case 'e':
			exec_file = optarg;
			break;
//AJ2D--------------------------------------------------------------------------
		case 'd':
			debugmode = 1;
			break;
		case 'p':
			gdb_port = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			usage();
			goto exit_skyeye;
		case 'c':
			skyeye_config_filename = optarg;
			break;
		case 'l': {
			char * tok = ",";
			char * str1 = strtok(optarg, tok);
			char * str2 = (char *)(optarg + strlen(str1) + 1);
			load_base = strtoul(str1, NULL, 16);
			load_mask = strtoul(str2, NULL, 16);
		}
			break;
		case 'b':
			big_endian = 1;
			break;
		case '?':
			if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			ret = 1;
			goto exit_skyeye;
		default:
			fprintf(stderr, "Default option .....\n");
			ret = 1;
			goto exit_skyeye;
		}
	// ww 2018-0821 useless message taken away
#if 0
	if(exec_file == NULL) {
		printf ("\n\
**************************** WARNING **********************************\n\
If you want to run ELF image, you should use -e option to indicate\n\
your elf-format image filename. Or you only want to run binary image,\n\
you need to set the filename of the image and its entry in sys.conf.\n\
***********************************************************************\n\n");
	}
#endif
	if (skyeye_config_filename == NULL) skyeye_config_filename = DEFAULT_CONFIG_FILE;

	for (index = optind; index < argc; index++)
		printf("Non-option argument %s\n", argv[index]);

	//teawater add DBCT_TEST_SPEED 2005.10.04---------------------------------------
#ifdef DBCT_TEST_SPEED
	{
		if (!dbct_test_speed_state) {
			//init timer
			struct itimerval value;
			struct sigaction act;

			dbct_test_speed_state = state;
			state->instr_count = 0;
			act.sa_handler = dbct_test_speed_sig;
			act.sa_flags = SA_RESTART;
			//cygwin don't support ITIMER_VIRTUAL or ITIMER_PROF
#ifndef __CYGWIN__
			if (sigaction(SIGVTALRM, &act, NULL) == -1) {
#else
				if (sigaction(SIGALRM, &act, NULL) == -1) {
#endif  //__CYGWIN__
					fprintf(stderr, "init timer error.\n");
					goto exit_skyeye;
				}
				if (skyeye_config.dbct_test_speed_sec) {
					value.it_value.tv_sec = skyeye_config.dbct_test_speed_sec;
				}
				else {
					value.it_value.tv_sec = DBCT_TEST_SPEED_SEC;
				}
				printf("dbct_test_speed_sec = %ld\n", value.it_value.tv_sec);
				value.it_value.tv_usec = 0;
				value.it_interval.tv_sec = 0;
				value.it_interval.tv_usec = 0;
#ifndef __CYGWIN__
				if (setitimer(ITIMER_VIRTUAL, &value, NULL) == -1) {
#else
					if (setitimer(ITIMER_REAL, &value, NULL) == -1) {
#endif  //__CYGWIN__
						fprintf(stderr, "init timer error.\n");
						goto exit_skyeye;
					}
				}
			}
#endif  //DBCT_TEST_SPEED
//AJ2D--------------------------------------------------------------------------

	/* 2007-03-28 by Anthony Lee : check for big endian at first */
	if (exec_file) {
		tea_load_exec(exec_file, 1);
		printf("The ELF loaded is ");
	}
	else {
		printf("Byte ordering: ");
	}
	if (big_endian)
		printf("Big-Endian.\n");
	else
		printf("Little-Endian.\n");

	/*do some initialization*/
	if ((ret = init()) < 0) goto exit_skyeye;
//teawater add for load elf 2005.07.31------------------------------------------
	if (exec_file) {
		if (tea_load_exec(exec_file, 0)) {
			fprintf(stderr, "load \"%s\" error\n", exec_file);
			goto exit_skyeye;
		}
#if defined(HAVE_LIBBFD) && !defined(__MINGW32__)
		/* get its symbol to debug */
		init_symbol_table(exec_file);
#endif
	}
	//AJ2D--------------------------------------------------------------------------
	if (skyeye_config.start_address != 0) {
		unsigned long addr = (skyeye_config.start_address & load_mask) | load_base;
		arch_instance->set_pc(addr);
		printf("start addr is set to 0x%08x by exec file.\n", (unsigned int)addr);

	}

	fflush(stdout);

	if (debugmode == 0)
		sim_resume(0);
	else {
		sim_debug(gdb_port);
	}
	exit_skyeye:

	/*
	 * 2007-01-24 removed the term-io functions by Anthony Lee,
	 * moved to "device/uart/skyeye_uart_stdio.c".
	 */

	return ret;
}

#ifdef __MINGW32__
int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
#endif // __MINGW32__
