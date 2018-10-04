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
#define INTTIME			100000 		/* interval timer period */


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
#define	FALSE		!TRUE
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
#define DEVNOSEM    3   /* we dont know what the first three devices are */

/* device & line number bits on */
#define FIRST			0x1
#define SECOND			0x2
#define THIRD			0x4
#define FOURTH			0x8
#define FIFTH			0x10
#define SIXTH			0x20
#define SEVENTH			0x40
#define EIGHTH			0x80

/* start of interrupt device bitmap and registers */
#define INTBITMAP		0x1000003C
#define INTDEVREG		0x10000050

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
#define QUANTUM     5000 /* 5 milliseconds in microseconds */
#define TOTALSEM    49 /* max number of semaphores possible plus 1 for timer */ /* SYSCALL8 */


#define ZEROSTATUS  0x0F

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

/* Status codes of registers? */
/* It is an AND to turn registers off and an OR to turn registers on. */
#define ALLOFF  0x00000000 /* Turn all of them off */
#define VMON    0x02000000 /* Turns on virtual memory */
#define VMpOFF  0xFDFFFFFF /* Turns off virtual memory *//* stupid name cuz conflict with test */
#define LTON    0x08000000 /* Turns on local time */
#define LTOFF   0xF7FFFFFF /* Turns off local time */
#define IMON    0x0000FF00 /* Turns on interrupt mask */
#define IMOFF   0xFFFF00FF /* Turns off interrupt mask */
#define KUON    0x00000008 /* Puts it into user mode */
#define KUOFF   0xFFFFFFF7 /* Puts it into kernel mode */
#define IEON    0x00000004 /* Turns on global interrupts */
#define IEOFF   0xFFFFFFFB /* Turns off global interrupts */
#define IECON   0x00000001 /* Turns on current interrupts */


/* Syscalls */
#define CREATE_PROCESS                      1
#define TERMINATE_PROCESS                   2
#define VERHOGEN                            3
#define PASSEREN                            4
#define SPECIFY_EXCEPTION_STATE_VECTOR      5
#define GET_CPU_TIME                        6
#define WAIT_FOR_CLOCK                      7    
#define WAIT_FOR_IO_DEVICE                  8

#define TLB                                 0
#define PGMTRAP                             1
#define SYSBP                               2
  
#endif
