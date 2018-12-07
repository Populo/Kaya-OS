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
#define SWAPSIZE        10      /* frames in swap */


#define ROMPAGESTART	0x20000000	/* ROM Reserved Page */
#define OSCODEEND       (ROMPAGESTART + (30 * PAGESIZE))
#define TAPEBUFFTOP		(OSCODETOP + (DEVPERINT * PAGESIZE))
#define DISKBUFFTOP		(TAPEBUFFTOP + (DEVPERINT * PAGESIZE))

#define INTTIME			100000 		/* interval timer period */
#define SECOND          1000000     /* 1 second in ms? */


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
#define INTPOLDAREAIDK 0x0000FF00

/* utility constants */
#define	TRUE		1
#define	FALSE		!TRUE
#define ON          1
#define OFF         0
#define EOS		    '\0'
#define DISK0       0

#define NULL ((void *)0xFFFFFFFF)


/* vectors number and type */
#define VECTSNUM	4

#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2

#define TRAPTYPES	3

/* 2D array for states */
#define NEW         0
#define OLD         1


/* device interrupts */
#define DISKINT		3
#define TAPEINT 	4
#define NETWINT 	5
#define PRNTINT 	6
#define TERMINT		7

/* IO Device Commands */
#define DISK_SEEKCYL    2
#define READBLK         3
#define DISK_WRITEBLK   4
#define TAPE_SKIPBLK    2
#define TAPE_BACKBLK    4

/* IO Device Errors */
#define DEVNOTINSTALLED 0
#define TAPE_EOT        0   /* end of tape */
#define TAPE_EOF        1   /* end of file */
#define TAPE_EOB        2   /* end of block */

#define TRANSCHAR       2   /* transmit character */
#define RECVCHAR        2   /* 

/* Misc shifts */
#define SHIFT_SEEK      8
#define SHIFT_SECTOR    8
#define SHIFT_HEAD      16

#define DEVREGLEN	4	/* device register field length in bytes & regs per dev */
#define DEVREGSIZE	16 	/* device register size in bytes */
#define DEVNOSEM    3 	/* we dont know what the first three devices are */
#define DEVPERINT	8	/* devices per interrupt */

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
#define TWENTYQ     100000 /* 100 milliseconds in microseconds */
#define TOTALSEM    49 /* max number of semaphores possible plus 1 for timer */ /* SYSCALL8 */
#define SUCCESS     0 /* successful operation return code */
#define FAILURE     -1


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
#define IECOFF  0xFFFFFFFE /* Turn off current interrupts */

/* interrupt line and device bit patterns */
#define LINEZERO	0x00000100
#define LINEONE		0x00000200
#define LINETWO		0x00000400
#define LINETHREE	0x00000800
#define LINEFOUR	0x00001000
#define LINEFIVE	0x00002000
#define LINESIX		0x00004000
#define LINESEVEN	0x00008000

#define DEVICEZERO	0x00000001
#define DEVICEONE	0x00000002
#define DEVICETWO	0x00000004
#define DEVICETHREE	0x00000008
#define DEVICEFOUR	0x00000010
#define DEVICEFIVE	0x00000020
#define DEVICESIX	0x00000040
#define DEVICESEVEN	0x00000080

/* Syscalls */
#define CREATE_PROCESS                      1
#define TERMINATE_PROCESS                   2
#define VERHOGEN                            3
#define PASSEREN                            4
#define SESV                                5
#define GETTIME                             6
#define WAITCLOCK                           7    
#define WAITIO                              8
#define READTERMINAL	                    9
#define WRITETERMINAL 	                    10
#define VSEMVIRT		                    11
#define PSEMVIRT		                    12
#define DELAY		                    	13
#define DISK_PUT		                    14
#define DISK_GET		                    15
#define WRITEPRINTER	                    16
#define GET_TOD			                    17
#define TERMINATE		                    18

#define SEG0		                        0x00000000
#define SEG1		                        0x40000000
#define SEG2		                        0x80000000
#define SEG3		                        0xC0000000

#define uProcStart                          0x800000B0


#define KSEGSIZE                            64
#define KUSEGSIZE                           32

/* entryHi constants */
#define GET_SEG                             0xD0000000
#define GET_VPN                             0x1FFFF000
#define GET_ASID                            0x00000FC0

#define SHIFT_SEG                           29
#define SHIFT_VPN                           12
#define SHIFT_ASID                          6

#define SET_ASID                            0xFFFFFFFFF ^ GET_ASID

/* entryLo constants */
#define GET_GLOBAL                          0x00000100
#define GET_VALID                           0x00000200
#define GET_DIRTY                           0x00000400
#define GET_NOCACHE                         0x00000800
#define GET_PFN                             0xFFFFF000


#define SHIFT_GLOBAL                        8
#define SHIFT_VALID                         9
#define SHIFT_DIRTY                         10
#define SHIFT_NOCACHE                       11
#define SHIFT_PFN                           12



/* entry bit definitions */
#define GLOBAL		                        (1 << SHIFT_GLOBAL)
#define VALID		                        (1 << SHIFT_VALID)
#define DIRTY		                        (1 << SHIFT_DIRTY)
#define NOCACHE                             (1 << SHIFT_NOCACHE)


#define READTERMINAL                        1
#define PRINTDEV                            24

#define WRITETERM                           0
#define READTERM                            1

#define TERMREADSEM                         32


#endif
