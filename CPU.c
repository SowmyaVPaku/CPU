#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ENTER printf("\n_____ %s %d ____\n",__func__,__LINE__);


/* CPU regiseters */
unsigned int R0;
unsigned int R1;
unsigned int R2;
unsigned int R3;
unsigned int R4;
unsigned int R5;
unsigned int R6;
unsigned int R7;
unsigned int R8;
unsigned int R9;
unsigned int R10;
unsigned int R11;
unsigned int R12;
unsigned int R13;
unsigned int R14;
unsigned int R15;

/* Temp registers */
unsigned int R16;
unsigned int R17;
unsigned int R18;
unsigned int R19;
unsigned int R20;
unsigned int R21;
unsigned int R22;
unsigned int R23;
unsigned int R24;
unsigned int R25;
unsigned int R26;
unsigned int R27;
unsigned int R28;
unsigned int R29;
unsigned int R30;
unsigned int R31;

/* Program Counter */
unsigned int pc;

/* Stack Pointer */
unsigned int sp;

/* Base Pointer */
unsigned int bp = 500;

unsigned int ax;

unsigned int bx;

unsigned int cx;

/* Memory block of 1024 bytes */
unsigned char memory[1024];

/* Endian Type*/
int endian_type = 1;

/* Flags Register */
unsigned int flag;

/* Bits position for Flag register */
enum flags_index{
     CF = 0, /* Carry Flag */
     PF = 2, /* Parity Flag */
     AF = 4, /* Adjust Flag */
     ZF = 6, /* Zero Flag */
     SF = 7, /* Sign Flag */
     TF = 8, /* Trap Flag */
     IF = 9, /* Interrupt Enable Flag */
     DF = 10, /* Direction Flag */
     OF = 11, /* Overflow Flag */
     IN   /*  Invalid */
};

char *flag_name[IN+1] = {"CF","","PF","","AF","","ZF","SF","TF","IF","DF","OF",""};


unsigned char show_memory = 1;

/* Opcodes */
enum OPCODE{

	STORE_MEMORY 	= 3,
	LOAD_MEMORY 	= 4,
	LOAD_MEMORY_I 	,
	MOV		,
	ADD		,
	SUB		,
	MUL		,
	DIV		,
	MOD		,
	PUSH	,
	POP		,
	ADDI 	,
	ADDT 	,
	SUBI	,
	MULI	,
	DIVI	,
	MODI	,
	JMP		,
	JNE		,
	JNC		,
	JC		,
	JNO		,
	JO 		,
	JNZ		,
	JZ		,
	CMPI    ,
	AND     ,
	OR      ,
	XOR     ,
	SHR     ,
	SHL     ,
	LEA  	,
	CMP  	,
	JA 		,
	CMPV	,
	JAE 	,
	DEC 	
};


#define STACK_BOTTOM 		896
#define STACK_TOP 			1024
#define STACK_MEMORY_BASE 	896
#define DATA_MEMORY_BASE	384
#define INSTR_MEMORY_BASE 	128
#define BOOT_MEMORY_BASE 	0
#define BIG__ENDIAN 0
#define LITTLE__ENDIAN 1

/* CPU Register index */ 
enum CPURegisterIndex{

	Index_R0 =0,		Index_R1,	Index_R2,	Index_R3,
	Index_R4,		Index_R5,	Index_R6,	Index_R7,
	Index_R8,		Index_R9,	Index_R10,	Index_R11,
	Index_R12,		Index_R13,	Index_R14,	Index_R15,

	Index_R16 = 16,		Index_R17,	Index_R18,	Index_R19,
	Index_R20,		Index_R21,	Index_R22,	Index_R23,
	Index_R24,		Index_R25,	Index_R26,	Index_R27,
	Index_R28,		Index_R29,	Index_R30,	Index_R31,

	Index_BP = 32,		Index_SP,	Index_PC,	Index_AX, 
	Index_BX = 36,		Index_CX,       Index_INVALID
};

#define MAX_LABELS	10

typedef struct label_table{
	char 		name[10];
	unsigned int 	address;
}LABEL_TABLE;

LABEL_TABLE	all_label_table[MAX_LABELS];

LABEL_TABLE	pending_label_table[MAX_LABELS];

static unsigned int filled_all_labels = 0;

unsigned int add_label(char *name, unsigned int address)
{
	
	if(name != NULL)
		strcpy(all_label_table[filled_all_labels].name, name);
	else
		return 0;
	
	all_label_table[filled_all_labels].address = address;
	
	filled_all_labels++;
	//printf("\n_____ %s %d ____ address = %d , label = %s\n",__func__,__LINE__, address, name);

	return 1;
}

unsigned int get_address_from_label(char *label)
{
	unsigned int index;
	unsigned int address = 0;


	if (label == NULL)
		return 0;

	for (index = 0; index < MAX_LABELS && index < filled_all_labels; index++)
	{
		//printf("table label = %s, cmp label = %s, strcasecmp = %d\n", all_label_table[index].name, label, strncasecmp(label, all_label_table[index].name, 2 ));
		if(!strncasecmp(all_label_table[index].name, label, 2))
		{
			address = all_label_table[index].address;
			break;
		}
	}

	return address;
}

static unsigned int filled_pending_labels = 0;

/* This is for Forward Jump instructions labels */
unsigned int add_pending_label(char *name, unsigned int address)
{
	
	if(name != NULL)
	{
		strcpy(pending_label_table[filled_pending_labels].name, name);
		pending_label_table[filled_pending_labels].address = address;
	}
	else
		return 0;
		
	filled_pending_labels++;

	return 1;
}

unsigned int get_from_pending_table(char *name, unsigned int *address)
{
	static unsigned int current_index = 0;
	//printf("inside get_from_pending_table filled_pending_labels = %d , current_index = %d\n", filled_pending_labels, current_index);
	if(current_index < filled_pending_labels)
	{
		//printf("name = %s, adress = %d \n", pending_label_table[current_index].name, pending_label_table[current_index].address);
		strcpy(name, pending_label_table[current_index].name );
		*address = pending_label_table[current_index].address;
		current_index ++;
	}
	else 
		return 0;

	return 1;
}


int check_flag(unsigned int flag_index)
{
	if(flag & (1 << flag_index))
		return 1;
	else
		return 0;
}

void set_flag(unsigned int flag_index)
{
	if (flag_index < IN)
		flag |= 1 << flag_index;
	return;
}

void clear_flag(unsigned int flag_index)
{
	if (flag_index < IN)
		flag &= ~(1 << flag_index);
	return;
}

int print_cpu_state(void)
{
	int i;

	printf("\n*******************************************************\n");
	printf("\t\tGeneral Purpose Registers\n");
	printf("*******************************************************\n");
	printf("R0 = %u\t\tR1 = %u\t\tR2 = %u\t\tR3 = %u\n", R0, R1, R2, R3);
	printf("R4 = %u\t\tR5 = %u\t\tR6 = %u\t\tR7 = %u\n", R4, R5, R6, R7);
	printf("R8 = %u\t\tR9 = %u\t\tR10 = %u\t\tR11 = %u\n", R8, R9, R10, R11);
	printf("R12 = %u\t\tR13 = %u\t\tR14 = %u\t\tR15 = %u\n", R12, R13, R14, R15);

	printf("\n*******************************************************\n");
	printf("\tProgram Counter\n");
	printf("*******************************************************\n");
	printf("\tPC = %u\n", pc+1);


	printf("\n*******************************************************\n");
	printf("\tStack Pointer\n");
	printf("*******************************************************\n");
	printf("\tSP = %u\n", sp);

	printf("\n*******************************************************\n");
	printf("\tBase Pointer and Other\n");
	printf("*******************************************************\n");
	printf("\tBP = %u\tAX = %u\tBX = %u\tCX = %u\n", bp,ax,bx,cx);

	printf("\n*******************************************************\n");
	printf("\tFlags\n");
	printf("*******************************************************\n");
	printf("\tflag = %u\n", flag);

	
	for (i = 0; i < IN ; i++)
	{
		if (i != 1 &&  i != 3 && i != 5 )
		{
			if(check_flag(i))
				printf("\t%s = 1", flag_name[i]);
			else
				printf("\t%s = 0", flag_name[i]);
		}
		if (i == 6)
			printf("\n");
	}
	printf("\n\n");

	if(show_memory)
	{
		printf("\n*******************************************************\n");
		printf("\t\t\t\tReserved Memory\n");
		printf("*******************************************************\n");
		
		printf("Address    \t\tMemory\n");
		
		for (i = 0; i < INSTR_MEMORY_BASE; i = i + 16)
		{
		    printf("%06d   %02X %02X %02X %02X  ", i, memory[i], memory[i+1], memory[i+2], memory[i+3]);
		    printf("%02X %02X %02X %02X  ", memory[i+4], memory[i+5], memory[i+6], memory[i+7]);
		    printf("%02X %02X %02X %02X  ", memory[i+8], memory[i+9], memory[i+10], memory[i+11]);
		    printf("%02X %02X %02X %02X  \n", memory[i+12], memory[i+13], memory[i+14], memory[i+15]);
		}

		printf("\n*******************************************************\n");
		printf("\t\t\t\tInstruction Memory\n");
		printf("*******************************************************\n");
		printf("Address    \t\tMemory\n");
		for (i = INSTR_MEMORY_BASE; i < DATA_MEMORY_BASE; i = i+16)
		{
		    printf("%06d   %02X %02X %02X %02X  ", i, memory[i], memory[i+1], memory[i+2], memory[i+3]);
		    printf("%02X %02X %02X %02X  ", memory[i+4], memory[i+5], memory[i+6], memory[i+7]);
		    printf("%02X %02X %02X %02X  ", memory[i+8], memory[i+9], memory[i+10], memory[i+11]);
		    printf("%02X %02X %02X %02X  \n", memory[i+12], memory[i+13], memory[i+14], memory[i+15]);
		}

		printf("\n*******************************************************\n");
		printf("\t\t\t\tData Memory\n");
		printf("*******************************************************\n");
		printf("Address    \t\tMemory\n");
		for (i = DATA_MEMORY_BASE; i < STACK_MEMORY_BASE; i = i+16)
		{
		    printf("%06d   %02X %02X %02X %02X  ", i, memory[i], memory[i+1], memory[i+2], memory[i+3]);
		    printf("%02X %02X %02X %02X  ", memory[i+4], memory[i+5], memory[i+6], memory[i+7]);
		    printf("%02X %02X %02X %02X  ", memory[i+8], memory[i+9], memory[i+10], memory[i+11]);
		    printf("%02X %02X %02X %02X  \n", memory[i+12], memory[i+13], memory[i+14], memory[i+15]);
		}

		printf("\n*******************************************************\n");
		printf("\t\t\t\tStack Memory\n");
		printf("*******************************************************\n");
		printf("Address    \t\tMemory\n");
		for (i = STACK_MEMORY_BASE; i < 1024; i = i+16)
		{
		    printf("%06d   %02X %02X %02X %02X  ", i, memory[i], memory[i+1], memory[i+2], memory[i+3]);
		    printf("%02X %02X %02X %02X  ", memory[i+4], memory[i+5], memory[i+6], memory[i+7]);
		    printf("%02X %02X %02X %02X  ", memory[i+8], memory[i+9], memory[i+10], memory[i+11]);
		    printf("%02X %02X %02X %02X  \n", memory[i+12], memory[i+13], memory[i+14], memory[i+15]);
		}

		
		    printf("\n\n");
	}
}

int readRegister(int reg)
{
	switch (reg)
	{ 
		case Index_R0: return R0;
		case Index_R1: return R1;
		case Index_R2: return R2;
		case Index_R3: return R3;
		case Index_R4: return R4;
		case Index_R5: return R5;
		case Index_R6: return R6;
		case Index_R7: return R7;
		case Index_R8: return R8;
		case Index_R9: return R9;
		case Index_R10: return R10;
		case Index_R11: return R11;
		case Index_R12: return R12;
		case Index_R13: return R13;
		case Index_R14: return R14;
		case Index_R15: return R15;
		case Index_R16: return R16;
		case Index_R17: return R17;
		case Index_R18: return R18;
		case Index_R19: return R19;
		case Index_R20: return R20;
		case Index_R21: return R21;
		case Index_R22: return R22;
		case Index_R23: return R23;
		case Index_R24: return R24;
		case Index_R25: return R25;
		case Index_R26: return R26;
		case Index_R27: return R27;
		case Index_R28: return R28;
		case Index_R29: return R29;
		case Index_R30: return R30;
		case Index_R31: return R31;
		case Index_SP: return sp;
		case Index_BP: return bp;
		case Index_PC: return pc;

		default: printf("$$$$$$$ read reg bug \n\n\n");break;
	}

	printf("invalid reg read for %d\n", reg);
}


void writeRegister(int reg, int value)
{
	switch(reg)
	{ 
		case Index_R0:			R0 = value;		break;
		case Index_R1:			R1 = value;		break;
		case Index_R2:			R2 = value;		break;
		case Index_R3:			R3 = value;		break;
		case Index_R4:			R4 = value;		break;
		case Index_R5:			R5 = value;		break;
		case Index_R6:			R6 = value;		break;
		case Index_R7:			R7 = value;		break;
		case Index_R8:			R8 = value;		break;
		case Index_R9:			R9 = value;		break;
		case Index_R10:			R10 = value;		break;
		case Index_R11:			R11 = value;		break;
		case Index_R12:			R12 = value;		break;
		case Index_R13:			R13 = value;		break;
		case Index_R14:			R14 = value;		break;
		case Index_R15:			R15 = value;		break;
		case Index_R16:			R16 = value;		break;
		case Index_R17:			R17 = value;		break;
		case Index_R18:			R18 = value;		break;
		case Index_R19:			R19 = value;		break;
		case Index_R20:			R20 = value;		break;
		case Index_R21:			R21 = value;		break;
		case Index_R22:			R22 = value;		break;
		case Index_R23:			R23 = value;		break;
		case Index_R24:			R24 = value;		break;
		case Index_R25:			R25 = value;		break;
		case Index_R26:			R26 = value;		break;
		case Index_R27:			R27 = value;		break;
		case Index_R28:			R28 = value;		break;
		case Index_R29:			R29 = value;		break;
		case Index_R30:			R30 = value;		break;
		case Index_R31:			R31 = value;		break;
		case Index_BP:			bp = value;		break;
		case Index_SP:			sp = value;		break;
		case Index_PC:			pc = value;		break;
		case Index_AX:			ax = value;		break;
		case Index_BX:			bx = value;		break;
		case Index_CX:			cx = value;		break;

		default: printf("$$$$$$$ read reg bug \n\n\n");break;
		break;

	}
		  
}

int binary(int num) // To get MSB bit of an integer
{
    int i; //Assuming 32 bit integer
    for( i=31; i>=0; i--) 
    {
        if(i == 31) // checking MSB bit is 1 (signed) or 0 (unsigned)
        return ((num >> i) & 1);
    }
    
}

// Check for carry flag
int carry(int x, int y, unsigned int addition)
{
    int c;
    if (addition > 2147483647) // range of int -2147483648 to +2147483647 (32 bit)
    {
		set_flag(CF);
    }
    else
    {   
		clear_flag(CF);
    }
}

/* check for overflow flag mostly comparing MSB bits for a,b and sum:set when add two positive number and get negative answer 
OR set when adding two negative numbers and get positive answer */
int overflow(int x, int y, int z)
{
    if(x == 0 && y == 0) 
    {
        if(z == 1)
        {
	    set_flag(OF);
        }
        else
        {
	    clear_flag(OF);
        }
    }
        
    else if(x == 1 && y == 1)
    {
        if(z == 0)
        {
	    set_flag(OF);
        }
        else
        {
	    clear_flag(OF);
        }
    }
    else
    {
	clear_flag(OF);
    } 
}

int lsb(int num) // To get LSB bit of an integer
{
     int i; //Assuming 32 bit integer
    for( i=31; i>=0; i--) 
    {
        if(i == 0) 
        return ((num >> i) & 1);
    }
}

int add_int(int a, int b)
{
	unsigned int c =0;
	//int x,y;
	//x = binary(a);
	//y = binary(b);
	printf("%s a = %d b = %d\n", __FUNCTION__, a, b);
	while (b != 0)
	{
		c = (a & b) ;

		a = a ^ b; 

		b = c << 1;
		if (c != 0 && c != 2147483648) //set auxillary carry
		{
			set_flag(CF);
		}
	}

	if(a == 0)
		set_flag(ZF);

	if(c == 2147483648){ //set overflow
		set_flag(OF);
		printf("%s carry = %u\n", __FUNCTION__,c);
	}

	return a;
}


int complement(int a) 
{
	return (~a);
}

void alu_store(int src, int dest)
{
	int data = 0;
	unsigned char *ptr = memory, *p;
	if (dest < 0) {
		printf("\n\t##Memory reserved for Bootstrap and Instruction Memory");
		return;
	}	
	
	data = readRegister(src);
	p = (unsigned char *)&data;
	if (BIG__ENDIAN == endian_type) {
		ptr [DATA_MEMORY_BASE + dest + 0] = p[0];
		ptr [DATA_MEMORY_BASE + dest + 1] = p[1];
		ptr [DATA_MEMORY_BASE + dest + 2] = p[2];
		ptr [DATA_MEMORY_BASE + dest + 3] = p[3];
	} else if (LITTLE__ENDIAN == endian_type) {
		ptr [DATA_MEMORY_BASE + dest + 0] = p[3];
		ptr [DATA_MEMORY_BASE + dest + 1] = p[2];
		ptr [DATA_MEMORY_BASE + dest + 2] = p[1];
		ptr [DATA_MEMORY_BASE + dest + 3] = p[0];
	}	
	//printf("\n%s:data = %u\n",__FUNCTION__,data);
	
	//*(int*)(ptr + DATA_MEMORY_BASE + dest) = data;
}

void alu_load(int src, int dest)
{
	int data = 0;
	unsigned char *ptr = memory, *p, *ptr2;
	if (src < 0) {
		printf("\n\t##Memory reserved for Bootstrap and Instruction memory");
		return;
	}	
	//data = *(int *)(ptr + DATA_MEMORY_BASE + src); //reading memory
	p = (unsigned char *)&data;
	if (BIG__ENDIAN == endian_type) {
		p[0] = ptr[DATA_MEMORY_BASE + src + 0];
		p[1] = ptr[DATA_MEMORY_BASE + src + 1];
		p[2] = ptr[DATA_MEMORY_BASE + src + 2];
		p[3] = ptr[DATA_MEMORY_BASE + src + 3];
	}
	else if (LITTLE__ENDIAN == endian_type) {
		p[0] = ptr[DATA_MEMORY_BASE + src + 3];
		p[1] = ptr[DATA_MEMORY_BASE + src + 2];
		p[2] = ptr[DATA_MEMORY_BASE + src + 1];
		p[3] = ptr[DATA_MEMORY_BASE + src + 0];
	}	
	writeRegister(dest, data);
}

void alu_load_i(int src, int dest)
{
	writeRegister(dest, src);
}

void alu_push(int src)
{
	int data = 0;
	unsigned char *ptr = memory, *p;
	
	if (sp == 893) {
		printf("\n*******************************************************\n");
		printf ("\n\t STACK IS FULL, CANNOT PUSH ELEMENTS");
		printf("\n*******************************************************\n");
		return;
	}
	data = readRegister(src);
	sp -= 4; 
	p = (unsigned char *)&data;
	if (BIG__ENDIAN == endian_type) {
		ptr [sp + 0] = p[0];
		ptr [sp + 1] = p[1];
		ptr [sp + 2] = p[2];
		ptr [sp + 3] = p[3];
	} else if (LITTLE__ENDIAN == endian_type) {
		ptr [sp + 0] = p[3];
		ptr [sp + 1] = p[2];
		ptr [sp + 2] = p[1];
		ptr [sp + 3] = p[0];
	}	
}

void alu_pop(int dest)
{
	int data = 0;
	unsigned char *ptr = memory, *p;
	if (sp == 1021) {
		printf("\n*******************************************************\n");
		printf ("\n\t STACK IS EMPTY, CANNOT POP ELEMENTS");
		printf("\n*******************************************************\n");
		return;
	}
	p = (unsigned char *)&data;
	if (BIG__ENDIAN == endian_type) {
		p[0] = ptr[sp + 0];
		p[1] = ptr[sp + 1];
		p[2] = ptr[sp + 2];
		p[3] = ptr[sp + 3];
	}
	else if (LITTLE__ENDIAN == endian_type) {
		p[0] = ptr[sp + 3];
		p[1] = ptr[sp + 2];
		p[2] = ptr[sp + 1];
		p[3] = ptr[sp + 0];
	}
	writeRegister(dest, data);
	sp += 4;
}

void alu_add(int src, int des)
{
	int sum, x, y;
	int a = readRegister(src); // MSB bit of a
	int b = readRegister(des); // MSB bit of b
	sum = add_int(a,b);
	writeRegister(des,sum);
}

void alu_addt(int src, int des, int target)
{
	int sum;
	int a = readRegister(src); // MSB bit of a
	int b = readRegister(des); // MSB bit of b
	sum = add_int(a, b);
	writeRegister(target, sum);
}

void alu_addi(int src,int des)
{
	int a, b,sum;
	a = src;
	b = readRegister(des);
	sum = add_int(a,b);
	writeRegister(des,sum);
}

int sub(int a, int b)
{
	int answer,difference;
	printf("%s a = %d b = %d\n", __FUNCTION__,a,b);
	answer = add_int(a, complement(b));

	return answer;
}
void alu_sub(int src, int des)
{

	int answer,a,b, diff;
	a = readRegister(src);
	b = readRegister(des);
	diff = sub(a,b);	
	writeRegister(des,diff);
}
    
void alu_subi(int src, int des)
{

	int answer,a,b, diff;
	a = src;
	b = readRegister(des);
	diff = b - a;	
	writeRegister(des,diff);
}
    

int mul(int x, int y)
{
	int a = 0, b = 0, c = 0, mul = 0;
	a=x;
	b=y;
	while(a >= 1)		/*To check if 'a' is even or odd*/
	{
		if (a & 0x1)
		{
			mul = add_int(mul,b);
		}
		a = a>>1;		/*left shift Operation*/
		b<<=1;			/*Right shift Operation*/
	}
	a = binary(a);
	b = binary(b);
	carry(a, b, mul);
	c = binary(mul); 
	overflow(a, b, c);
	return mul;
}

void alu_mul(int src, int des)
{
	int a,b,prod;
	a = readRegister(src);   /*read from register*/

	b = readRegister(des);   /*read from register*/
	prod = mul(a,b);
	writeRegister(des,prod);

}

void alu_muli(int src, int des)
{
int a,b,prod;
	a = src;   /*read from register*/
	b = readRegister(des);   /*read from register*/
	prod = mul(a,b);
	writeRegister(des,prod);
}

int div_int(int a, int b)
{
int nr, dr, zf, sf;
nr = a;
dr = b;
if (nr == 0 || dr == 0)
	{
		zf = 1;
		set_flag(ZF);
		//printf("ZF is.... %d\n", zf);
		//printf("Exception occured.. Divide by zero\n");
	}


	else
	{
		zf = 0;
		//printf("ZF is.... %d\n", zf);
	}
	if (dr < 0 || nr < 0)
	{
		sf = 1;
		set_flag(SF);
		//printf("SF is %d\n", sf);
		//printf("Exception occured.. Divide by a negative number\n");
	}
	else
	{
		sf = 0;
		//printf("sf is %d\n", sf);
	}

	int q = 1, temp1, temp2;
	temp1 = dr;
	if ((nr != 0 && dr != 0) && (dr < nr))

	{
		while (((temp1 << 1) - nr) < 0)
		{
			temp1 = temp1 << 1;
			q = q << 1;
		}
		while ((temp1 + dr) <= nr)
		{
			temp1 = temp1 + dr;
			q = q + 1;
		}
	}

	if (dr)
	{

		b=q;
		//printf("The Quotient is... %d\n", b);
	}
	else
	{
		printf("ALERT...EXCEPTIOn\n");
	}
	


return b;
}

void alu_div(int src, int des)
{
	int nr, dr, ZF, SF, Quotient;
	nr = readRegister(src);
	 dr = readRegister(des);
	Quotient = div_int(nr,dr);
	writeRegister(des,Quotient);
	
}

void alu_divi(int src, int des)
{
int nr, dr, ZF, SF, Quotient;
	nr = src;
	 dr = readRegister(des);
	Quotient = div_int(nr,dr);
	writeRegister(des,Quotient);

}

int mod_int(int x, int y)
{
	int nr,dr,remainder,n = 0;
	nr = x;
	dr = y;

	if (nr == 0 || dr == 0)
	{
		set_flag(ZF);
	}

	else
	{
		clear_flag(ZF);
	}
	if (dr < 0 || nr < 0)
	{
		set_flag(SF);
	}
	else
	{
		clear_flag(SF);

	}
	
	
	if (n>0 && dr == 2^n)
	{
		remainder = (nr & (dr - 1));
	}
	else
	{
		
		remainder = (nr - (dr * (nr/dr)));
	}
	
	return remainder;

}

int alu_mov(unsigned int src, unsigned int des) // logical right shift
{
    unsigned int a;
    a = readRegister(src);
    writeRegister(des, a);
}

void alu_mod(int src, int des)
{
	int nr, dr, remainder;
	nr = readRegister(src);
	dr = readRegister(des);
	remainder = mod_int(nr,dr);
	writeRegister(des,remainder);
	
}

void alu_modi(int src, int des)
{
int nr,dr, remainder;
nr = src;
dr = readRegister(des);
remainder= mod_int(nr,dr);
writeRegister(des,remainder);
}


void alu_JMP(unsigned int dsti)
{
	if(128<dsti<384)
	{
		if (BIG__ENDIAN == endian_type)
			pc = dsti;
		else if (LITTLE__ENDIAN == endian_type) 
			pc = dsti-1;
		flag = 0;

	}
	else
		printf("Error");

}

void alu_JNE(unsigned int dst)
{
	//printf(" Jump %d\n", dsti);
	if(!check_flag(ZF))
	{		
		//printf(" ZF is set\n");
		if(128< dst <384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;

			flag = 0;
				//printf(" PC = %d\n",pc);
		}
		else
			printf("Error");
	}
}

void alu_JNC(unsigned int dst)
{
	if(check_flag(CF))
	{
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
			flag = 0;
		}
	}
}

void alu_JC(unsigned int dst)
{
	if(check_flag(CF)==1)
	{
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
			flag = 0;
		}
	}
}

void alu_JNO(unsigned int dst)
{
	if(check_flag(OF)==0)
	{
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
			flag = 0;
		}
	}
}

void alu_JO(unsigned int dst)
{
	if(check_flag(OF)==1)
	{
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
			flag = 0;
		}
	}
}

void alu_JNZ(unsigned int dst)
{
	if(check_flag(ZF)==0)
	{
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
			flag = 0;
		}
	}
}

void alu_JZ(unsigned int dst)
{
	if(check_flag(ZF)==1)
	{
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
			flag = 0;
		}
	}
}

void alu_JAE(unsigned int dst)
{
	if(!(check_flag(SF) ^ check_flag(OF)) )
	{
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
			flag = 0;
		}
	}
}

void alu_JA(unsigned int dst)
{
	//printf("%s JUMP above: CF = %d, OF = %d\n", __FUNCTION__,check_flag(CF), check_flag(OF));

	if((!(check_flag(CF))) & (!(check_flag(OF))) )
	{
		//printf("%s perform JUMP \n", __FUNCTION__ );
		if(128<dst<384)
		{
			if (BIG__ENDIAN == endian_type)
				pc = dst;
			else if (LITTLE__ENDIAN == endian_type) 
				pc = dst-1;
		flag = 0;
		}
	}
}

void alu_cmpi(unsigned int src, unsigned int dst)
{
	if (src == readRegister(dst))
	{
        set_flag(ZF);
    }
    else
    {
        clear_flag(ZF);		
	}
}

void alu_cmp(unsigned int src, unsigned int dst)
{
	int a = readRegister(src);
	int b = readRegister(dst);
	//printf("%s a = %d , b = %d\n", __FUNCTION__ , a, b);
	sub(a,b);
	if (a == b)
	{
        set_flag(ZF);
    }
    else
    {
        clear_flag(ZF);		
	}

	if (a < b)
	{
        set_flag(SF);		
	}
    else
    {
        clear_flag(SF);		
	}

}

void alu_cmpv(unsigned int src, unsigned int dst)
{
	int a = memory[readRegister(src)];
	int b = readRegister(dst);
	sub(a,b);

	if (a == b)
	{
        set_flag(ZF);
    }
    else
    {
        clear_flag(ZF);		
	}

	if (a < b)
	{
        set_flag(SF);		
	}
    else
    {
        clear_flag(SF);		
	}

}

int alu_and(int src, int des)
{
    int a, b, c;
    a = readRegister(src);
    b = readRegister(des);
    c = a & b;
    writeRegister(des, c);
}

int alu_or(int src, int des)
{
    int a, b, c;
    a = readRegister(src);
    b = readRegister(des);
    c = a | b;
    writeRegister(des, c);
}

int alu_shr(unsigned int src, unsigned int des) // logical right shift
{
    int a, c;
    a = readRegister(des);
    a = a >> src;
    if(a == 0)
    {
        set_flag(ZF);
    }
    else
    {
        clear_flag(ZF);
    }
    writeRegister(des, a);
}

int alu_shl(unsigned int des, unsigned int src) // logical left shift
{
    int a, c;
    a = readRegister(des);
    c = binary(a);
    if(c == 0)
    {
        clear_flag(CF);
    }
    else
    {
        set_flag(CF);
    }
    a = a << src;
    writeRegister(des, a);
}

int alu_xor(int src, int des)
{
    int a, b, c;
    a = readRegister(src);
    b = readRegister(des);
    c = a ^ b;
    writeRegister(des, c);
}


void alu_lea(int src, int dst)
{
	int result;
	unsigned char *ptr = memory;
	result = src + bp;
	if (dst > 37)
		*(int*)(ptr + DATA_MEMORY_BASE + dst) = result;
	else
		writeRegister(dst, result);
	//printf("\n\n\n\n--- %u  %u %u\n\n\n\n",result,memory[dst], dst);
}


void ALU(char opcode, int src, int dest, int target)
{
	switch(opcode)
	{
	case STORE_MEMORY:
		alu_store(src, dest);
	break;
	case LOAD_MEMORY:
		alu_load(src, dest);
	break;
	case LOAD_MEMORY_I:
		alu_load_i(src, dest);
	break;
	case PUSH:
		alu_push(src);
	break;
	case POP:
		alu_pop(dest);
	break;
	case MUL:
		
		alu_mul(src, dest);
	break;
	case ADD:
		
		alu_add(src, dest);
	break;
	case DIV:
		
		alu_div(src, dest);
	break;
	case MOD:
		alu_mod(src,dest);
	break;
	case MOV:
		alu_mov(src,dest);
	break;
	case SUB:
		alu_sub(src,dest);
	break;
	case ADDI:
		alu_addi(src, dest);
	break;
	case ADDT:
		alu_addt(src, dest, target);
	break;
	case SUBI:
		alu_subi(src,dest);
	break;
	case MULI:
		
		alu_muli(src, dest);
	break;
	case DIVI:
		
		alu_divi(src, dest);
	break;
	case MODI:
		alu_modi(src,dest);
	break;
    case JMP:
		alu_JMP(dest);
	break;
    case JNE:
		alu_JNE(dest);
	break;
	case JNC:
		alu_JNC(dest);
	break;
	case JC:
		alu_JC(dest);
	break;
	case JNZ:
		alu_JNZ(dest);
	break;
	case JZ:
		alu_JZ(dest);
	break;
	case JNO:
		alu_JNO(dest);
	break;
	case JO:
		alu_JO(dest);
	break;
    case JA:
	    alu_JA(dest);
	break;
    case JAE:
	    alu_JAE(dest);
	break;
	case AND:
	    alu_and(src, dest);
	break;
	case OR:
	    alu_or(src, dest);
	break;
	case XOR:
	    alu_xor(src, dest);
	break;
	case SHR:
	    alu_shr(src, dest);
	break;
    case SHL:
	    alu_shl(src, dest);
	break;
    case CMPI:
	    alu_cmpi(src, dest);
	break;
    case CMP:
	    alu_cmp(src, dest);
	break;
    case CMPV:
	    alu_cmpv(src, dest);
	break;
	case LEA: 
		alu_lea(src, dest);
	break;	
	default: 
		break;
	}

}

void usage()
{
	printf("\n*******************************************************\n");
	printf("\n\t##Please Enter the assembly instruction as"
		       				"mentioned below##");
	printf("\n\t##Don't Use First 100 Bytes are reserved for Bootstrap");
	printf("\n\t##  ld 150, R1  ##");
	printf("\n\t##Above Instruction fetches data from 150 memory" 
						"location to Register 1##");
	printf("\n\t##  st R2, 200  ##");
	printf("\n\t##Above Instruction fetches data from R2"
				"Register to 200 Memory Location##");
	printf("\n\t##  lr $50, R5  ##");
	printf("\n\t##Above Instruction load resgister R5 with 50 value");

	printf("\n*******************************************************\n");
	return ;
}

/*This function Returns index of a given register string*/
int returnIndex(char *ptr)
{
	char i = 0;
	i = atoi(++ptr);	
	
	return i;
}

int return_offset(char *ptr)
{
	uint16_t i = 0;
	i = atoi(&ptr[7]);	
	return i;
}

int execute_one_instr()
{
	unsigned char opcode = 0, dst = 0, src = 0, target = 0;
	if (BIG__ENDIAN == endian_type) {
		opcode = memory[pc ];
		src = memory[pc + 1];
		dst = memory[pc + 2];
		target = memory[pc + 3];
	} else if (LITTLE__ENDIAN == endian_type) {
		opcode = memory[pc ];
		src = memory[pc - 1];
		dst = memory[pc - 2];
		target = memory[pc -3];
	}

	if(opcode == 0)
	{	
		//printf("\n Done execution pc = %d\n", pc);
		return 0;
	}
	printf("\nExecuting Instruction number %d : %u %u %u", (pc - 127)/4,  opcode, src, dst);
	ALU(opcode, src, dst, target);
	pc = pc + 4;
	return 1;
}

void initALU()
{
	if (BIG__ENDIAN == endian_type) {
		pc = INSTR_MEMORY_BASE;
	}
	else if (LITTLE__ENDIAN == endian_type) {
		pc = INSTR_MEMORY_BASE + 3;
	}
	sp = STACK_TOP;
	flag = 0;
	//writeRegister(Index_R5, 10);

	memset(all_label_table, 0, sizeof(all_label_table));
	memset(pending_label_table, 0, sizeof(pending_label_table));

}

int loader()
{
	unsigned int  i = 0, count = 0, ret = -1;
	size_t len = 0;
	unsigned int opcode = 0, dst = 0, src = 0, target = 0;
	char *instr;
	FILE *file_fd = 0;
	char *p1 = NULL, *p2 = NULL, *p3 = NULL;
	char *label;
	unsigned int address, jmp_address, label_address;

	instr = (char *) malloc(200);
	file_fd = fopen("./instructions.txt","r");

	while( (ret = getline(&instr, &len, file_fd)) != -1)
	{
		/*Here parse the OPCODE, OPERANDS and perform corresponding operations*/
		printf("\n\tInstruction = \t %s", instr);

		p1 = strtok(instr, " ");
		if (!strcasecmp(p1, "ld")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = atoi(p2) - DATA_MEMORY_BASE;
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			opcode = LOAD_MEMORY;
		} else if (!strcasecmp(p1, "ldi")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = atoi(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			opcode = LOAD_MEMORY_I;
		} else if (!strcasecmp(p1, "st")) { 
			p2 = strtok(NULL, ",");		
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");		/* Here loads from Register to location*/	
			dst = atoi(p3) - DATA_MEMORY_BASE;
			opcode =  STORE_MEMORY;
		} else if (!strcasecmp(p1, "mov")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);

			opcode = MOV;
		} else if (!strcasecmp(p1, "mul")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);

			opcode = MUL;
		}
		else if (!strcasecmp(p1, "sub")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			opcode = SUB;
		}
		else if (!strcasecmp(p1, "add")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);		/* Here loads div instruction*/	
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			opcode = ADD;
		}
		else if (!strcasecmp(p1, "addt")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			target = returnIndex(p2);		/* Here loads div instruction*/	
			p3 = strtok(NULL, " ");
			src = returnIndex(p3);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			opcode = ADDT;
		}
		else if (!strcasecmp(p1, "lea")) {
			p2 = strtok(NULL, ",");	     //Load Effective Address(LEA) instruction	
			
			if (!strcasecmp(p2,"ax"))
				dst = Index_AX;
			else if (!strcasecmp(p2,"bx"))
				dst = Index_BX;
			else if (!strcasecmp(p2,"cx"))
				dst = Index_CX;
			else 
				dst = atoi(p2) - DATA_MEMORY_BASE;
			p3 = strtok(NULL, "]");
			src = return_offset(p3);
			opcode = LEA;
		}

		else if (!strcasecmp(p1, "mod")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);

			opcode = MOD;
		}
		else if (!strcasecmp(p1, "div")) {
			p2 = strtok(NULL, ",");	     /* Here loads div instruction*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);

			opcode = DIV;
			
		}
		else if (!strcasecmp(p1, "addi")) { 
			p2 = strtok(NULL, ",");	
			src = atoi(p2);	
			p3 = strtok(NULL, " ");		/* Here loads from Register to location*/	
			dst = returnIndex(p3);
			opcode =  ADDI;
		}
		else if (!strcasecmp(p1, "subi")) { 
			p2 = strtok(NULL, ",");	
			src = atoi(p2);	
			p3 = strtok(NULL, " ");		/* Here loads from Register to location*/	
			dst = returnIndex(p3);
			opcode =  SUBI;
		}
		else if (!strcasecmp(p1, "muli")) { 
			p2 = strtok(NULL, ",");	
			src = atoi(p2);	
			p3 = strtok(NULL, " ");		/* Here loads from Register to location*/	
			dst = returnIndex(p3);
			opcode =  MULI;
		}
		else if (!strcasecmp(p1, "divi")) { 
			p2 = strtok(NULL, ",");	
			src = atoi(p2);	
			p3 = strtok(NULL, " ");		/* Here loads from Register to location*/	
			dst = returnIndex(p3);
			opcode =  DIVI;
		}
		else if (!strcasecmp(p1, "modi")) { 
			p2 = strtok(NULL, ",");	
			src = atoi(p2);	
			p3 = strtok(NULL, " ");		/* Here loads from Register to location*/	
			dst = returnIndex(p3);
			opcode =  MODI;
		}
		else if (!strcasecmp(p1, "push")) { 
			p2 = strtok(NULL, " ");
			if(*p2 == 'R')
				src = returnIndex(p2);
			else if (strstr(p2,"bp"))
				src = Index_BP;
			else if (strstr(p2,"sp"))
				src = Index_SP;
			else if (strstr(p2,"pc"))
				src = Index_PC;

			opcode =  PUSH;

			dst = Index_INVALID;
		} 
		else if (!strcasecmp(p1, "pop")) { 
			p2 = strtok(NULL, " ");
			if(*p2 == 'R')
				dst = returnIndex(p2);
			else if (strstr(p2,"bp"))
				dst = Index_BP;
			else if (strstr(p2,"sp"))
				dst = Index_SP;
			else if (strstr(p2,"pc"))
				dst = Index_PC;

			opcode =  POP;

			src = Index_INVALID;
		}
		else if (!strcasecmp(p1, "jmp")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JMP;
			src = Index_INVALID;

		}
		else if (!strcasecmp(p1, "jne")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JNE;
			src = Index_INVALID;

		}
		else if (!strcasecmp(p1, "jnc")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JNC;
			src = Index_INVALID;

		}
		else if (!strcasecmp(p1, "jc")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JC;
			src = Index_INVALID;

		}
		else if (!strcasecmp(p1, "jnz")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JNZ;
			src = Index_INVALID;

		}
		else if (!strcasecmp(p1, "jz")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JZ;
			src = Index_INVALID;

		}
		else if (!strcasecmp(p1, "jno")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JNO;
			src = Index_INVALID;

		}
		else if (!strcasecmp(p1, "jo")) { 
			p2 = strtok(NULL, " ");

			dst = get_address_from_label(p2);
			
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JO;
			src = Index_INVALID;
		}
		else if (!strcasecmp(p1, "ja")) { 
			p2 = strtok(NULL, " ");
			dst = get_address_from_label(p2);
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JA;
			src = Index_INVALID;
		}
		else if (!strcasecmp(p1, "jae")) { 
			p2 = strtok(NULL, " ");
			dst = get_address_from_label(p2);
			if(dst == 0)
			{
				add_pending_label(p2, INSTR_MEMORY_BASE + i);
				//printf("\n\tadd to pending: label = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			}
			opcode =  JAE;
			src = Index_INVALID;
		}
		else if (!strcasecmp(p1, "and")) {
		    p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			//printf("AND : src = %d, dst = %d\n", src, dst);

			opcode = AND; 
		}
		else if(!strcasecmp(p1, "or")) {
		    p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);

			opcode = OR; 
		}
		else if(!strcasecmp(p1, "xor")) {
		    p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);

			opcode = XOR; 
		}
		else if(!strcasecmp(p1, "shr")) {
		    p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
		    src = atoi(p2);     /* Here loads from memory location to register*/	
			//src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			//printf("SHIFT R: src = %d, dst = %d\n", src, dst);
			opcode = SHR; 
		}
		else if(!strcasecmp(p1, "shl")) {
		    src = atoi(p2);	     /* Here loads from memory location to register*/	
			//src = returnIndex(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);

			opcode = SHL; 
		}
		else if (!strcasecmp(p1, "cmpi")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = atoi(p2);
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			opcode = CMPI;
		} 
		else if (!strcasecmp(p1, "cmp")) {
			p2 = strtok(NULL, ",");	     /* Here loads from memory location to register*/	
			src = returnIndex(p2);;
			p3 = strtok(NULL, " ");
			dst = returnIndex(p3);
			opcode = CMP;
		} 
		else if (!strcasecmp(p1, "cmpv")) {
			p2 = strtok(NULL, "]");
			//printf("cmpv: p2 = %s\n", p2 );
			//*(p2 + 3) = '\0';	     
			src = returnIndex(p2 + 1);
			//printf("src =%d\n", src);
			p2 = strtok(NULL, " ");
			p3 = strtok(NULL, " ");
			//printf("cmpv: p2 = %s\n", p2 );
			dst = returnIndex(p3);
			//printf("cmpv: p3 = %s\n", p3 );
			//printf("dst = %d\n", dst);
			opcode = CMPV;
		} 
		else if (!strcasecmp(p1, "dec")) {
			p2 = strtok(NULL, " ");
			src = returnIndex(p2);
			opcode = DEC;
		} 
		else if (*p1, "L") {  /* This is a label*/ 
			p2 = strtok(p1, ":");
			add_label(p2, INSTR_MEMORY_BASE + i);
			//printf("\n\tlabel = %s, address = %u\n", p2,  INSTR_MEMORY_BASE + i);
			p1 = strtok(NULL, " ");
			continue;

		
		} 
		else {
			usage();
		}

		printf("\tDecoded inst : \t %u %u %u\n", opcode, src, dst);

		/*Store Instruction in BIG or Little Endian Formats*/
		if (endian_type == BIG__ENDIAN)
		{
			memory[INSTR_MEMORY_BASE + i] = opcode;
			i++;
			memory[INSTR_MEMORY_BASE + i] = src;
			i++;
			memory[INSTR_MEMORY_BASE + i] = dst;
			i++;
			memory[INSTR_MEMORY_BASE + i] = target;
			i++;

		}
		else if (endian_type == LITTLE__ENDIAN)
		{
			memory[INSTR_MEMORY_BASE + i] = target;
			i++;
			memory[INSTR_MEMORY_BASE + i] = dst;
			i++;
			memory[INSTR_MEMORY_BASE + i] = src;
			i++;
			memory[INSTR_MEMORY_BASE + i] = opcode;
			i++;
		}
	       	count++;
		p1 = strtok(NULL, " ");
	}

	label = (char *)malloc(10);
	while(get_from_pending_table(label, &jmp_address))
	{
		//printf("label = %s address = %d\n", label, jmp_address);
		label_address = get_address_from_label(label);
		//printf("label_address = %d\n", label_address);
		if (endian_type == BIG__ENDIAN)
		{
			// big endian means dst is at 3rd byte
			memory[jmp_address + 2] = label_address; 
		}
		else if (endian_type == LITTLE__ENDIAN)
		{
			// little endian means dst is at 2nd byte
			memory[jmp_address + 1] = label_address; 
		}		
	}

	return count;
}

int main(int argc, char **argv)
{
	int count = 0, i;
	char ch, value;
	unsigned int max = -1;
	char *p = NULL;
	int size = 0;
    FILE *myFile;

    if(argc > 1)
    {
    	if(!strcasecmp(argv[1],"-NoMemDisplay"))
    	{
    		show_memory = 0;
    	}
    }
#if 0
	printf("\n*******************************************************\n");
	printf("\tPlease choose MEMORY FORMAT type for BEST CPU\n");
	printf("\t 0 -- BIG ENDIAN\n\t 1 -- LITTLE ENDIAN\n");
	printf("\tEnter Your Choice\n");
	printf("\n*******************************************************\n");
	scanf("%d", &endian_type);
	

	if (endian_type != BIG__ENDIAN && endian_type != LITTLE__ENDIAN) {
		printf("\n\tEntered wrong option, default Little Endian selected\n");
		endian_type = 1;
	}
#endif
	endian_type = 1;
	max = max / 2;
	/*copy max value of signed int, for overflow case*/
	if (BIG__ENDIAN == endian_type) {
		p = (char *) &max;
		memory [384 + 0] = p[0];
		memory [384 + 1] = p[1];
		memory [384 + 2] = p[2];
		memory [384 + 3] = p[3];
	}
	else if (LITTLE__ENDIAN == endian_type) {
		p = (char *) &max;
		memory [384 + 0] = p[3];
		memory [384 + 1] = p[2];
		memory [384 + 2] = p[1];
		memory [384 + 3] = p[0];
	}	
	
	initALU();
	count = loader();
	printf("\t Press any key to start execution :");
	ch = getchar();

    myFile = fopen("numbers.txt", "r");
    fscanf(myFile, "%d", &R4);
    //read file into array
	//while(fscanf(myFile, "%d", &memory[384 + i]) > 0) {
	while(fscanf(myFile, "%d", &memory[i]) > 0) {
        i++;
    }
    fclose(myFile);

    R3 = i;
    R2 = 0;

    printf("\nKey: %d", R4);
    printf("\nTotal elements: %d", R3);
    printf("\nNumbers are: ");
    for (i = 0; i < R3; i++)
    {
        printf("%d ", memory[ i]);
    }

	printf("\n*******************************************************\n");
	printf("\tCPU State before execution\n");
	printf("*******************************************************\n");
	print_cpu_state();

	while (execute_one_instr()) { 
		printf("\n*******************************************************\n");
		printf("\tCPU State After execution of instruction\n");
		printf("*******************************************************\n");
		print_cpu_state();
	}

}
