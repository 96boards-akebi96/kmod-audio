#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <fa.h>

#define max(a, b)    ( (a > b) ? (a) : (b) )
#define min(a, b)    ( (a < b) ? (a) : (b) )

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define print_bf(map, reg, ch)    print_bit_field(map, #reg, ch, reg(ch), reg##_bf, ARRAY_SIZE(reg##_bf))


//SW view CHMap   :0x56000000
#define A2CHNMapCtr0(n)             (0x56000000 + 0x40 * (n))
//SW view RBMap   :0x56001000
#define A2RBNMapCtr0(n)             (0x56001000 + 0x40 * (n))
//SW view IPortMap:0x56002000
#define A2IPortNMapCtr0(n)          (0x56002000 + 0x40 * (n))
#define A2IPortNMapCtr1(n)          (0x56002004 + 0x40 * (n))
//SW view IIFMap  :0x56003000
#define A2IIFNMapCtr0(n)            (0x56003000 + 0x40 * (n))
//SW view OPortMap:0x56004000
#define A2OPortNMapCtr0(n)          (0x56004000 + 0x40 * (n))
#define A2OPortNMapCtr1(n)          (0x56004004 + 0x40 * (n))
#define A2OPortNMapCtr2(n)          (0x56004008 + 0x40 * (n))
//SW view OIFMap  :0x56005000
#define A2OIFNMapCtr0(n)            (0x56005000 + 0x40 * (n))


//AIN(PCMINN) AINPORT_SW:0x56022000
#define A2SWIPortMXCtr1(n)          (0x56022000 + 0x400 * (n))
#define A2SWIPortMXCtr2(n)          (0x56022004 + 0x400 * (n))
#define A2SWIPortMXACCtr(n)         (0x5602200c + 0x400 * (n))
#define A2SWIPortMXCntCtr(n)        (0x56022010 + 0x400 * (n))
#define A2SWIPortMXCounter(n)       (0x56022014 + 0x400 * (n))
#define A2SWIPortMXACLKSel0Ex(n)    (0x56022020 + 0x400 * (n))
#define A2SWIPortMXIR(n)            (0x5602202c + 0x400 * (n))
#define A2SWIPortMXIM0(n)           (0x56022030 + 0x400 * (n))
#define A2SWIPortMXID0(n)           (0x56022040 + 0x400 * (n))
#define A2SWIPortMXEXNOE(n)         (0x56022070 + 0x400 * (n))
#define A2SWIPortMXRstCtr(n)        (0x5602207c + 0x400 * (n))

//AIN(PBinMX) PBIN_SW:0x56020200
#define A2SWPBinMXCtr(n)            (0x56020200 + 0x40 * (n))


//AOUT(PCMOUTN) AOUTPORT_SW:0x56042000
#define A2SWOPortMXCtr1(n)               (0x56042000 + 0x400 * (n))
#define A2SWOPortMXCtr2(n)               (0x56042004 + 0x400 * (n))
#define A2SWOPortMXCtr3(n)               (0x56042008 + 0x400 * (n))
#define A2SWOPortMXACLKSel0Ex(n)         (0x56042030 + 0x400 * (n))
#define A2SWOPortMXPath(n)               (0x56042040 + 0x400 * (n))
#define A2SWOPortMXSYNC(n)               (0x56042044 + 0x400 * (n))
#define A2SWOPortMXCntCtr(n)             (0x56042060 + 0x400 * (n))
#define A2SWOPortMXCounter(n)            (0x56042064 + 0x400 * (n))
#define A2SWOPortMXIR(n)                 (0x56042070 + 0x400 * (n))
#define A2SWOPortMXIM0(n)                (0x56042080 + 0x400 * (n))
#define A2SWOPortMXIM1(n)                (0x56042084 + 0x400 * (n))
#define A2SWOPortMXIM2(n)                (0x56042088 + 0x400 * (n))
#define A2SWOPortMXIM3(n)                (0x5604208c + 0x400 * (n))
#define A2SWOPortMXID0(n)                (0x56042090 + 0x400 * (n))
#define A2SWOPortMXID1(n)                (0x56042094 + 0x400 * (n))
#define A2SWOPortMXID2(n)                (0x56042098 + 0x400 * (n))
#define A2SWOPortMXID3(n)                (0x5604209c + 0x400 * (n))
#define A2SWOPortMXEXNOE(n)              (0x560420f0 + 0x400 * (n))
#define A2SWOPortMXT0VolPara1(n)         (0x56042100 + 0x400 * (n))
#define A2SWOPortMXT1VolPara1(n)         (0x56042120 + 0x400 * (n))
#define A2SWOPortMXT2VolPara1(n)         (0x56042140 + 0x400 * (n))
#define A2SWOPortMXT3VolPara1(n)         (0x56042160 + 0x400 * (n))
#define A2SWOPortMXT4VolPara1(n)         (0x56042180 + 0x400 * (n))
#define A2SWOPortMXT0VolPara2(n)         (0x56042104 + 0x400 * (n))
#define A2SWOPortMXT1VolPara2(n)         (0x56042124 + 0x400 * (n))
#define A2SWOPortMXT2VolPara2(n)         (0x56042144 + 0x400 * (n))
#define A2SWOPortMXT3VolPara2(n)         (0x56042164 + 0x400 * (n))
#define A2SWOPortMXT4VolPara2(n)         (0x56042184 + 0x400 * (n))
#define A2SWOPortMXT0VolGainStatus(n)    (0x56042108 + 0x400 * (n))
#define A2SWOPortMXT1VolGainStatus(n)    (0x56042128 + 0x400 * (n))
#define A2SWOPortMXT2VolGainStatus(n)    (0x56042148 + 0x400 * (n))
#define A2SWOPortMXT3VolGainStatus(n)    (0x56042168 + 0x400 * (n))
#define A2SWOPortMXT4VolGainStatus(n)    (0x56042188 + 0x400 * (n))
#define A2SWOPortMXT0VolChPara(n)        (0x5604210c + 0x400 * (n))
#define A2SWOPortMXT1VolChPara(n)        (0x5604212c + 0x400 * (n))
#define A2SWOPortMXT2VolChPara(n)        (0x5604214c + 0x400 * (n))
#define A2SWOPortMXT3VolChPara(n)        (0x5604216c + 0x400 * (n))
#define A2SWOPortMXT4VolChPara(n)        (0x5604218c + 0x400 * (n))
#define A2SWOPortMXT0VolCtr(n)           (0x56042110 + 0x400 * (n))
#define A2SWOPortMXT1VolCtr(n)           (0x56042130 + 0x400 * (n))
#define A2SWOPortMXT2VolCtr(n)           (0x56042150 + 0x400 * (n))
#define A2SWOPortMXT3VolCtr(n)           (0x56042170 + 0x400 * (n))
#define A2SWOPortMXT4VolCtr(n)           (0x56042190 + 0x400 * (n))
#define A2SWOPortMXT0SlotCtr(n)          (0x56042114 + 0x400 * (n))
#define A2SWOPortMXT1SlotCtr(n)          (0x56042134 + 0x400 * (n))
#define A2SWOPortMXT2SlotCtr(n)          (0x56042154 + 0x400 * (n))
#define A2SWOPortMXT3SlotCtr(n)          (0x56042174 + 0x400 * (n))
#define A2SWOPortMXT4SlotCtr(n)          (0x56042194 + 0x400 * (n))
#define A2SWOPortMXT0RstCtr(n)           (0x5604211c + 0x400 * (n))
#define A2SWOPortMXT1RstCtr(n)           (0x5604213c + 0x400 * (n))
#define A2SWOPortMXT2RstCtr(n)           (0x5604215c + 0x400 * (n))
#define A2SWOPortMXT3RstCtr(n)           (0x5604217c + 0x400 * (n))
#define A2SWOPortMXT4RstCtr(n)           (0x5604219c + 0x400 * (n))

//AOUT(PBoutMX) PBOUT_SW:0x56040200
#define A2SWPBoutMXCtr0(n)               (0x56040200 + 0x40 * (n))
#define A2SWPBoutMXCtr1(n)               (0x56040204 + 0x40 * (n))
#define A2SWPBoutMXIntCtr(n)             (0x56040208 + 0x40 * (n))


//A2D(subsystem)
#define CDA2D_STRT0                 (0x56010000)
#define CDA2D_PERFCNFG              (0x56010008)
#define CDA2D_STAT0                 (0x56010020)
#define CDA2D_CHMXCTRL1(n)          (0x56011000 + 0x40 * (n))
#define CDA2D_CHMXCTRL2(n)          (0x56011004 + 0x40 * (n))
#define CDA2D_CHMXSTAT(n)           (0x56011010 + 0x40 * (n))
#define CDA2D_CHMXIR(n)             (0x56011014 + 0x40 * (n))
#define CDA2D_CHMXIE(n)             (0x56011018 + 0x40 * (n))
#define CDA2D_CHMXID(n)             (0x5601101c + 0x40 * (n))
#define CDA2D_CHMXSRCAMODE(n)       (0x56011020 + 0x40 * (n))
#define CDA2D_CHMXDSTAMODE(n)       (0x56011024 + 0x40 * (n))
#define CDA2D_CHMXSRCSTRTADRS(n)    (0x56011028 + 0x40 * (n))
#define CDA2D_CHMXDSTSTRTADRS(n)    (0x5601102c + 0x40 * (n))
#define CDA2D_CHMXSIZE(n)           (0x56011030 + 0x40 * (n))

//A2D(ring buffer)
#define CDA2D_RBFLUSH0              (0x56010040)
#define CDA2D_PARTRESET0            (0x56010050)
#define CDA2D_PARTRESET1            (0x56010054)
#define CDA2D_RBMXBGNADRS(n)        (0x56012000 + 0x40 * (n))
#define CDA2D_RBMXENDADRS(n)        (0x56012004 + 0x40 * (n))
#define CDA2D_RBMXBTH(n)            (0x56012008 + 0x40 * (n))
#define CDA2D_RBMXRTH(n)            (0x5601200c + 0x40 * (n))
#define CDA2D_RBMXSTAT(n)           (0x56012010 + 0x40 * (n))
#define CDA2D_RBMXIR(n)             (0x56012014 + 0x40 * (n))
#define CDA2D_RBMXIE(n)             (0x56012018 + 0x40 * (n))
#define CDA2D_RBMXID(n)             (0x5601201c + 0x40 * (n))
#define CDA2D_RBMXRDPTR(n)          (0x56012020 + 0x40 * (n))
#define CDA2D_RBMXWRPTR(n)          (0x56012028 + 0x40 * (n))
#define CDA2D_RBMXCNFG(n)           (0x56012030 + 0x40 * (n))
#define CDA2D_RBMXPTRCTRL(n)        (0x56012034 + 0x40 * (n))

static const char *get_A2IPortNMapCtrl1_ainnum_name(int v);
static const char *get_A2OPortNMapCtrl1_aoutnum_name(int v);

struct bitfield {
	const char *name;
	int start;
	int end;
	const char *(*to_string)(int v);
};


struct bitfield A2CHNMapCtr0_bf[] = {
	{"en",        31, 31, }, 
	{"mapnum",     0,  4, }, 
};

struct bitfield A2RBNMapCtr0_bf[] = {
	{"en",        31, 31, }, 
	{"mapnum",     0,  4, }, 
};

struct bitfield A2IPortNMapCtr0_bf[] = {
	{"en",        31, 31, }, 
	{"mapnum",     0,  4, }, 
};

struct bitfield A2IPortNMapCtr1_bf[] = {
	{"valid_d",   31, 31, }, 
	{"ainnum_d",  24, 30, get_A2IPortNMapCtrl1_ainnum_name, }, 
	{"valid_c",   23, 23, }, 
	{"ainnum_c",  16, 22, get_A2IPortNMapCtrl1_ainnum_name, }, 
	{"valid_b",   15, 15, }, 
	{"ainnum_b",   8, 14, get_A2IPortNMapCtrl1_ainnum_name, }, 
	{"valid_a",    7,  7, }, 
	{"ainnum_a",   0,  6, get_A2IPortNMapCtrl1_ainnum_name, }, 
};

struct bitfield A2IIFNMapCtr0_bf[] = {
	{"en",        31, 31, }, 
	{"mapnum",     0,  4, }, 
};

struct bitfield A2OPortNMapCtr0_bf[] = {
	{"en",        31, 31, }, 
	{"mapnum",     0,  4, }, 
};

struct bitfield A2OPortNMapCtr1_bf[] = {
	{"valid_d",   31, 31, }, 
	{"aoutnum_d", 24, 30, get_A2OPortNMapCtrl1_aoutnum_name, }, 
	{"valid_c",   23, 23, }, 
	{"aoutnum_c", 16, 22, get_A2OPortNMapCtrl1_aoutnum_name, }, 
	{"valid_b",   15, 15, }, 
	{"aoutnum_b",  8, 14, get_A2OPortNMapCtrl1_aoutnum_name, }, 
	{"valid_a",    7,  7, }, 
	{"aoutnum_a",  0,  6, get_A2OPortNMapCtrl1_aoutnum_name, }, 
};

struct bitfield A2OPortNMapCtr2_bf[] = {
	{"valid_e",    7,  7, }, 
	{"aoutnum_e",  0,  6, }, 
};

struct bitfield A2OIFNMapCtr0_bf[] = {
	{"en",        31, 31, }, 
	{"mapnum",     0,  4, }, 
};




struct bitfield A2SWPBinMXCtr_bf[] = {
	{"reserved", 31, 31, }, 
	{"none",     30, 30, }, 
	{"reserved", 29, 29, }, 
	{"none",     25, 28, }, 
	{"ifstat",   24, 24, }, 
	{"none",     20, 23, }, 
	{"reserved", 16, 19, }, 
	{"nconnect", 15, 15, }, 
	{"inoutsel", 14, 14, }, 
	{"none",     13, 13, }, 
	{"pbinsel",   8, 12, }, 
	{"none",      6,  7, }, 
	{"endian",    4,  5, }, 
	{"memfmt",    0,  3, }, 
};




struct bitfield A2SWOPortMXCtr1_bf[] = {
	{"bit32sel",  31, 31, }, 
	{"i2ssel",    14, 14, }, 
	{"lrsel",     10, 10, }, 
	{"outbitsel",  8,  9, }, 
	{"fssel",      0,  3, }, 
};

struct bitfield A2SWOPortMXCtr2_bf[] = {
	{"aclksel0",    16, 19, }, 
	{"mssel",       15, 15, }, 
	{"extlsifssel", 14, 14, }, 
	{"daccksel",     8,  9, }, 
	{"reqen",        0,  0, }, 
};

struct bitfield A2SWOPortMXCtr3_bf[] = {
	{"ufmod",    22, 23, }, 
	{"fifostat", 21, 21, }, 
	{"spstat",   20, 20, }, 
	{"pmsw",      2,  2, }, 
};

struct bitfield A2SWOPortMXACLKSel0Ex_bf[] = {
	{"aclksel0exa", 0,  1, }, 
};

struct bitfield A2SWOPortMXPath_bf[] = {
	{"nconnect",  7,  7, }, 
	{"inoutsel",  6,  6, }, 
	{"aoutsel",   0,  4, }, 
};

struct bitfield A2SWOPortMXSYNC_bf[] = {
	{"thremode", 31, 31, }, 
	{"sync_c",    7,  7, }, 
	{"sync_ch",   0,  4, }, 
};


struct bitfield A2SWPBoutMXCtr0_bf[] = {
	{"rev_mode", 28, 28, }, 
	{"ifstat",   24, 24, }, 
	{"acc_mode", 20, 20, }, 
	{"flgsel",   16, 19, }, 
	{"endian",    4,  5, }, 
	{"memfmt",    0,  3, }, 
};

struct bitfield A2SWPBoutMXCtr1_bf[] = {
	{"packetsel",  0,  3, }, 
};




static const char *get_A2IPortNMapCtrl1_ainnum_name(int v)
{
	const char *name = "????";

	switch (v) {
	case 0:
		name = "PCM-IN0";
		break;
	case 1:
		name = "PCM-IN1";
		break;
	case 2:
		name = "PCM-IN2";
		break;
	case 3:
		name = "PCM-IN3";
		break;
	case 4:
		name = "PCM-IN4";
		break;
	case 5:
		name = "PCM-IN5";
		break;
	case 6:
		name = "PCM-IN6";
		break;
	case 64:
		name = "IEC-IN0";
		break;
	case 65:
		name = "IEC-IN1";
		break;
	default:
		name = "unknown";
		break;
	}

	return name;
}

static const char *get_A2OPortNMapCtrl1_aoutnum_name(int v)
{
	const char *name = "????";

	switch (v) {
	case 0:
		name = "PCM-OUT0";
		break;
	case 1:
		name = "PCM-OUT1";
		break;
	case 2:
		name = "forbidden2";
		break;
	case 3:
		name = "forbidden3";
		break;
	case 4:
		name = "forbidden4";
		break;
	case 5:
		name = "PCM-OUT5";
		break;
	case 6:
		name = "PCM-OUT6";
		break;
	case 7:
		name = "PCM-OUT7";
		break;
	case 8:
		name = "PCM-OUT8";
		break;
	case 9:
		name = "PCM-OUT9";
		break;
	case 10:
		name = "PCM-OUT10";
		break;
	case 11:
		name = "PCM-OUT11";
		break;
	case 12:
		name = "PCM-OUT12";
		break;
	case 13:
		name = "DUM0";
		break;
	case 14:
		name = "DUM1";
		break;
	case 15:
		name = "DUM2";
		break;
	case 64:
		name = "IEC-OUT0";
		break;
	case 65:
		name = "IEC-OUT1";
		break;
	case 66:
		name = "forbidden66";
		break;
	default:
		name = "unknown";
		break;
	}

	return name;
}




uint32_t get_bits32(uint32_t orig, int st, int ed)
{
	uint32_t maskb;

	if (st > ed) {
		fprintf(stderr, "%s: st:%d > ed:%d, bug?\n", 
			__func__, st, ed);
		return 0;
	}

	maskb = (1 << ed) | ((1 << ed) - 1);

	return (orig & maskb) >> st;
}


void print_bit_field(sni_fa_t *ao, const char *name, int ch, unsigned long paddr, struct bitfield *bf, int n)
{
	uint32_t v, v_bf;
	const char *v_bf_sep, *v_bf_name;
	int i;

	v = fa_readl(ao, paddr);
	printf("%28s(%2d): addr:0x%08lx, val:0x%08x(%d)\n", 
		name, ch, paddr, v, v);

	for (i = 0; i < n; i++) {
		v_bf = get_bits32(v, bf[i].start, bf[i].end);

		if (bf[i].to_string) {
			v_bf_sep = " : ";
			v_bf_name = bf[i].to_string(v_bf);
		} else {
			v_bf_sep = "";
			v_bf_name = "";
		}

		printf("%28s(%2d): %2d:%2d(%2dbit) %-16s: 0x%08x(%d)%s%s\n", 
			name, ch, 
			bf[i].start, bf[i].end, bf[i].end - bf[i].start + 1, 
			bf[i].name, 
			v_bf, v_bf, v_bf_sep, v_bf_name);
	}
}

int main(int argc, char *argv[])
{
	sni_fa_t ao;
	int result;
	int i;

	result = fa_ioremap_nocache(&ao, 0x56000000, 0x01000000);
	if (result != 0) {
		fprintf(stderr, "failed to fa_ioremap_nocache.\n");
		return result;
	}

	for (i = 0; i < 24; i++) {
		printf("CHMap ch:%d ----------\n", i);
		print_bf(&ao, A2CHNMapCtr0, i);
		print_bf(&ao, A2RBNMapCtr0, i);
		print_bf(&ao, A2IPortNMapCtr0, i);
		print_bf(&ao, A2IPortNMapCtr1, i);
		print_bf(&ao, A2IIFNMapCtr0, i);
		print_bf(&ao, A2OPortNMapCtr0, i);
		print_bf(&ao, A2OPortNMapCtr1, i);
		print_bf(&ao, A2OPortNMapCtr2, i);
		print_bf(&ao, A2OIFNMapCtr0, i);
	}

	for (i = 0; i < 16; i++) {
		printf("PBinMX ch:%d ----------\n", i);
		print_bf(&ao, A2SWPBinMXCtr, i);
	}

	for (i = 0; i < 32; i++) {
		printf("PCMOUTN ch:%d ----------\n", i);
		print_bf(&ao, A2SWOPortMXCtr1, i);
		print_bf(&ao, A2SWOPortMXCtr2, i);
		print_bf(&ao, A2SWOPortMXCtr3, i);
		print_bf(&ao, A2SWOPortMXACLKSel0Ex, i);
		print_bf(&ao, A2SWOPortMXPath, i);
		print_bf(&ao, A2SWOPortMXSYNC, i);
		/*print_bf(&ao, A2SWOPortMXCntCtr, i);
		print_bf(&ao, A2SWOPortMXCounter, i);
		print_bf(&ao, A2SWOPortMXIR, i);
		print_bf(&ao, A2SWOPortMXIM0, i);
		print_bf(&ao, A2SWOPortMXIM1, i);
		print_bf(&ao, A2SWOPortMXIM2, i);
		print_bf(&ao, A2SWOPortMXIM3, i);
		print_bf(&ao, A2SWOPortMXID0, i);
		print_bf(&ao, A2SWOPortMXID1, i);
		print_bf(&ao, A2SWOPortMXID2, i);
		print_bf(&ao, A2SWOPortMXID3, i);
		print_bf(&ao, A2SWOPortMXEXNOE, i);
		print_bf(&ao, A2SWOPortMXT0VolPara1, i);
		print_bf(&ao, A2SWOPortMXT1VolPara1, i);
		print_bf(&ao, A2SWOPortMXT2VolPara1, i);
		print_bf(&ao, A2SWOPortMXT3VolPara1, i);
		print_bf(&ao, A2SWOPortMXT4VolPara1, i);
		print_bf(&ao, A2SWOPortMXT0VolPara2, i);
		print_bf(&ao, A2SWOPortMXT1VolPara2, i);
		print_bf(&ao, A2SWOPortMXT2VolPara2, i);
		print_bf(&ao, A2SWOPortMXT3VolPara2, i);
		print_bf(&ao, A2SWOPortMXT4VolPara2, i);
		print_bf(&ao, A2SWOPortMXT0VolGainStatus, i);
		print_bf(&ao, A2SWOPortMXT1VolGainStatus, i);
		print_bf(&ao, A2SWOPortMXT2VolGainStatus, i);
		print_bf(&ao, A2SWOPortMXT3VolGainStatus, i);
		print_bf(&ao, A2SWOPortMXT4VolGainStatus, i);
		print_bf(&ao, A2SWOPortMXT0VolChPara, i);
		print_bf(&ao, A2SWOPortMXT1VolChPara, i);
		print_bf(&ao, A2SWOPortMXT2VolChPara, i);
		print_bf(&ao, A2SWOPortMXT3VolChPara, i);
		print_bf(&ao, A2SWOPortMXT4VolChPara, i);
		print_bf(&ao, A2SWOPortMXT0VolCtr, i);
		print_bf(&ao, A2SWOPortMXT1VolCtr, i);
		print_bf(&ao, A2SWOPortMXT2VolCtr, i);
		print_bf(&ao, A2SWOPortMXT3VolCtr, i);
		print_bf(&ao, A2SWOPortMXT4VolCtr, i);
		print_bf(&ao, A2SWOPortMXT0SlotCtr, i);
		print_bf(&ao, A2SWOPortMXT1SlotCtr, i);
		print_bf(&ao, A2SWOPortMXT2SlotCtr, i);
		print_bf(&ao, A2SWOPortMXT3SlotCtr, i);
		print_bf(&ao, A2SWOPortMXT4SlotCtr, i);
		print_bf(&ao, A2SWOPortMXT0RstCtr, i);
		print_bf(&ao, A2SWOPortMXT1RstCtr, i);
		print_bf(&ao, A2SWOPortMXT2RstCtr, i);
		print_bf(&ao, A2SWOPortMXT3RstCtr, i);
		print_bf(&ao, A2SWOPortMXT4RstCtr, i);*/
	}

	for (i = 0; i < 24; i++) {
		printf("PBoutMX ch:%d ----------\n", i);
		print_bf(&ao, A2SWPBoutMXCtr0, i);
		print_bf(&ao, A2SWPBoutMXCtr1, i);
	}

	fa_iounmap(&ao);

	return 0;
}
