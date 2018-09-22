#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 ****************************************************************************/

/* Hardware & software constants */
#define PAGESIZE		4096	/* page size in bytes */
#define WORDLEN			4		/* word size in bytes */
#define PTEMAGICNO		0x2A


#define ROMPAGESTART	0x20000000	 /* ROM Reserved Page */


/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR	0x10000000
#define TODLOADDR	0x1000001C
#define INTERVALTMR	0x10000020	
#define TIMESCALEADDR	0x10000024

/* new processor state locations */
#define SYSCALLNEWAREA 0x200003D4
#define PBGTRAPNEWAREA 0x200002BC
#define TBLMGMTNEWAREA 0x200001A4
#define INTPNEWAREA    0x2000008C

/* old processor state locations */
#define SYSCALLOLDAREA 0x20000348
#define PGMTRAPOLDAREA 0x20000230
#define TBLMGMTOLDAREA 0x20000118
#define INTPOLDAREA    0x20000000

/* utility constants */
#define	TRUE		1
#define	FALSE		0
#define ON              1
#define OFF             0
#define EOS		'\0'

#define NULL ((void *)0xFFFFFFFF)


/* vectors number and type */
#define VECTSNUM	4

#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2

#define TRAPTYPES	3


/* device interrupts */
#define DISKINT		3
#define TAPEINT 	4
#define NETWINT 	5
#define PRNTINT 	6
#define TERMINT		7

#define DEVREGLEN	4	/* device register field length in bytes & regs per dev */
#define DEVREGSIZE	16 	/* device register size in bytes */

/* device register field number for non-terminal devices */
#define STATUS		0
#define COMMAND		1
#define DATA0		2
#define DATA1		3

/* device register field number for terminal devices */
#define RECVSTATUS      0
#define RECVCOMMAND     1
#define TRANSTATUS      2
#define TRANCOMMAND     3


/* device common STATUS codes */
#define UNINSTALLED	0
#define READY		1
#define BUSY		3

/* device common COMMAND codes */
#define RESET		0
#define ACK		1

/* operations */
#define	MIN(A,B)	((A) < (B) ? A : B)
#define MAX(A,B)	((A) < (B) ? B : A)
#define	ALIGNED(A)	(((unsigned)A & 0x3) == 0)

/* Useful operations */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR))) 

/* Maximum number of ProcBLK allowed to be allocated */
#define MAXPROC 20

/* Quality of Life Constants */
#define HIDDEN      static
#define MAXINT      0xEFFFFFFF

/* Cause Codes */
#define Int     0 /* external device interrupt */
#define Mod     1 /* TLB - Modification Exception */
#define TLBL    2 /* TLB Invalid Exception: on a Load or Fetch */
#define TLBS    3 /* TLB Invalid Exception: on a Store */
#define AdEL    4 /* Address Error Exception: on a Load or Fetch */
#define AdES    5 /* Address Error Exception: on a Load/Store data access */
#define IBE     6 /* Bus Error Exception: on an instruction fetch */
#define DBE     7 /* Bus Error Exception: on a Load/Store data access */
#define Sys     8 /* Syscall Exception */
#define Bp      9 /* Breakpoint Exception */
#define RI      10 /* Reserved Instruction Exception */
#define CpU     11 /* CoProcessor Unusable Exception */
#define OV      12 /* Arithmetic Overflow Exception */
#define BdPT    13 /* Bad Page Table */
#define PTMs    14 /* Page Table Miss */



#endif
