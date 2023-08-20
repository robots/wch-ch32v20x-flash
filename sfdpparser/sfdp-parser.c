// SPDX-License-Identifier: GPL-2.0-only
// Author: Petr Malat
#include <byteswap.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

#define NAME_LEN 47

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#define SFDP_SIGNATURE 0x50444653

struct sfdp_param_hdr {
	uint8_t id_lsb;
	uint8_t minor;
	uint8_t major;
	uint8_t len;
	uint8_t ptr[3];
	uint8_t id_msb;
};

struct sfdp_hdr {
	uint32_t signature;
	uint8_t minor;
	uint8_t	major;
	uint8_t	nph;
	uint8_t proto;
};

static void p_num_val(uint32_t val, const char *name, const char *description,
		bool hex)
{
	const char *descriptionfmt = " (%s)";
	int indent, numlen = 10;
	char fmt[32];

	if (!description) {
		descriptionfmt = "%s";
		description = "";
	}

	indent = NAME_LEN - strlen(name);
	if (indent < 1) {
		snprintf(fmt, sizeof fmt, hex ? "%#x" : "%u", val);
		numlen += strlen(fmt) + indent - 2;
		if (numlen < 0)
			numlen = 0;
		indent = 1;
	}

	snprintf(fmt, sizeof fmt, "%%s:%%-%ds%%%s%d%c%s\n", indent, hex ? "#" : "", numlen, hex ? 'x' : 'u', descriptionfmt);
	printf(fmt, name, "", val, description);
}

static void p_dec_val(uint32_t val, const char *name, const char *description)
{
	p_num_val(val, name, description, false);
}

static void p_hex_val(uint32_t val, const char *name, const char *description)
{
	p_num_val(val, name, description, val >= 10);
}

/*
 * Parameter handlers
 */
//#define bits(w, high, low) ((w & ((1ULL << ((high) + 1)) - 1)) >> (low))
#define bits(w, high, low) ((w & (0xffFFffFFU >> (31 - high))) >> (low))

#define parse_bits(w, high, low, name, vals...) do { \
	const char *arr[] = vals; \
	int b = bits(w, high, low); \
        if (b < ARRAY_SIZE(arr) && arr[b]) \
		p_hex_val(b, name, arr[b]); \
        else \
		p_hex_val(b, name, "Invalid value"); \
        } while (0);

#define SUPP_OR_NOT { "not supported", "supported" }

static int sfdp_unknown_parameter(uint32_t *w, size_t len)
{
	unsigned i;

	for (i = 0; i < len; i++) {
		char name[32];

		snprintf(name, sizeof name, "Word %u", i);
		p_hex_val(w[i], name, NULL);
	}

	return 0;
}

static int sfdp_basic(uint32_t *w, size_t len)
{
	unsigned erase_time_mult[] = {1, 16, 128, 1000};
	unsigned program_page_time_mult[] = {8, 64};
	unsigned program_byte_time_mult[] = {1, 8};
	unsigned erase_chip_time_mult[] = {16, 256, 4000, 64000};
	unsigned tmp;

	if (len < 1)
		return 0;
	parse_bits(w[0], 1, 0, "Erase Size", {
		[1] = "4kB supported",
		[3] = "4kB supported",
	});
	parse_bits(w[0], 2, 2, "Write Granularity", {
		"Single byte or less than 64 bytes",
		"64 bytes or more"
	});
	parse_bits(w[0], 3, 3, "Volatile Status Register Block Protect Bits", {
		"non-volatile", "volatile"
	});
	parse_bits(w[0], 4, 4, "Write Enable Instruction for Writing to Volatile Status Register", {
		"50h", "06h"
	});
	p_hex_val(bits(w[0], 15, 8), "4kB Erase Instruction", NULL);
	parse_bits(w[0], 16, 16, "1-1-2 Fast Read", SUPP_OR_NOT);
	parse_bits(w[0], 18, 17, "Address Bytes", {
		"3-byte addressing", "3- or 4-byte addressing", "4-byte addressing"
	});
	parse_bits(w[0], 19, 19, "Double transfer rate (DTR) Clocking", SUPP_OR_NOT);
	parse_bits(w[0], 20, 20, "1-2-2 Fast Read", SUPP_OR_NOT);
	parse_bits(w[0], 21, 21, "1-4-4 Fast Read", SUPP_OR_NOT);
	parse_bits(w[0], 22, 22, "1-1-4 Fast Read", SUPP_OR_NOT);
	

	if (len < 2)
		return 0;
	if (bits(w[1], 31, 31)) {
		p_dec_val(1 << (bits(w[1], 30, 0)-23), "Flash Memory Density", "in megabytes");
	} else {
		p_dec_val(bits(w[1], 30, 0)/8, "Flash Memory Density", "in bytes");
	}
	
	
	if (len < 3)
		return 0;
	p_dec_val(bits(w[2], 4, 0), "1-4-4 Fast Read Number of Wait States Needed", NULL);
	p_dec_val(bits(w[2], 7, 5), "1-4-4 Fast Read Number of Mode Clocks", NULL);
	p_hex_val(bits(w[2], 15, 8), "1-4-4 Fast Read Instructions", NULL);
	p_dec_val(bits(w[2], 20, 16), "1-1-4 Fast Read Number of Wait States Needed", NULL);
	p_dec_val(bits(w[2], 23, 21), "1-1-4 Fast Read Number of Mode Clocks", NULL);
	p_hex_val(bits(w[2], 31, 24), "1-1-4 Fast Read Instructions", NULL);
	
	
	if (len < 4)
		return 0;
	p_dec_val(bits(w[3], 4, 0), "1-1-2 Fast Read Number of Wait States Needed", NULL);
	p_dec_val(bits(w[3], 7, 5), "1-1-2 Fast Read Number of Mode Clocks", NULL);
	p_hex_val(bits(w[3], 15, 8), "1-1-2 Fast Read Instructions", NULL);
	p_dec_val(bits(w[3], 20, 16), "1-2-2 Fast Read Number of Wait States Needed", NULL);
	p_dec_val(bits(w[3], 23, 21), "1-2-2 Fast Read Number of Mode Clocks", NULL);
	p_hex_val(bits(w[3], 31, 24), "1-2-2 Fast Read Instructions", NULL);
	

	if (len < 5)
		return 0;
	parse_bits(w[4], 0, 0, "2-2-2 Fast Read", SUPP_OR_NOT);
	parse_bits(w[4], 4, 4, "4-4-4 Fast Read", SUPP_OR_NOT);


	if (len < 6)
		return 0;
	p_dec_val(bits(w[5], 20, 16), "2-2-2 Fast Read Number of Wait States Needed", NULL);
	p_dec_val(bits(w[5], 23, 21), "2-2-2 Fast Read Number of Mode Clocks", NULL);
	p_hex_val(bits(w[5], 31, 24), "2-2-2 Fast Read Instructions", NULL);


	if (len < 7)
		return 0;
	p_dec_val(bits(w[6], 20, 16), "4-4-4 Fast Read Number of Wait States Needed", NULL);
	p_dec_val(bits(w[6], 23, 21), "4-4-4 Fast Read Number of Mode Clocks", NULL);
	p_hex_val(bits(w[6], 31, 24), "4-4-4 Fast Read Instructions", NULL);


	if (len < 8)
		return 0;
	p_dec_val(1 << bits(w[7], 7, 0), "Erase Type 1 Size", "in bytes");
	p_hex_val(bits(w[7], 15, 8), "Erase Type 1 Instruction", NULL);
	p_dec_val(1 << bits(w[7], 23, 16), "Erase Type 2 Size", bits(w[7], 23, 16) ? "in bytes" : "not supported");
	p_hex_val(bits(w[7], 31, 24), "Erase Type 2 Instruction", NULL);


	if (len < 9)
		return 0;
	p_dec_val(1 << bits(w[8], 7, 0), "Erase Type 3 Size", bits(w[8], 7, 0) ? "in bytes" : "not supported");
	p_hex_val(bits(w[8], 15, 8), "Erase Type 3 Instruction", NULL);
	p_dec_val(1 << bits(w[8], 23, 16), "Erase Type 4 Size", bits(w[8], 23, 16) ? "in bytes" : "not supported");
	p_hex_val(bits(w[8], 31, 24), "Erase Type 4 Instruction", NULL);


	if (len < 10)
		return 0;
	p_dec_val(2 * (1 + bits(w[9], 3, 0)), "Typical Erase Time to Maximum Erase Time Multiplier", NULL);
	tmp = erase_time_mult[bits(w[9], 8, 4)] * (bits(w[9], 10, 9) + 1);
	p_dec_val(tmp, "Erase Type 1 Typical Time", "in milliseconds");
	tmp = erase_time_mult[bits(w[9], 17, 16)] * (bits(w[9], 15, 11) + 1);
	p_dec_val(tmp, "Erase Type 2 Typical Time", "in milliseconds");
	tmp = erase_time_mult[bits(w[9], 24, 23)] * (bits(w[9], 22, 18) + 1);
	p_dec_val(tmp, "Erase Type 4 Typical Time", "in milliseconds");
	tmp = erase_time_mult[bits(w[9], 31, 30)] * (bits(w[9], 29, 25) + 1);
	p_dec_val(tmp, "Erase Type 4 Typical Time", "in milliseconds");


	if (len < 11)
		return 0;
	p_dec_val(2 * (1 + bits(w[10], 3, 0)), "Typical Program Time to Maximum Program Time Multiplier", NULL);
	p_dec_val(1 << bits(w[10], 7, 4), "Page Size", NULL);
	tmp = program_page_time_mult[bits(w[10], 13, 13)] * (bits(w[10], 12, 8) + 1);
	p_dec_val(tmp, "Typical Page Program Time", "in microseconds");
	tmp = program_byte_time_mult[bits(w[10], 18, 18)] * (bits(w[10], 17, 14) + 1);
	p_dec_val(tmp, "Typical First Byte Program Time", "in microseconds");
	tmp = program_byte_time_mult[bits(w[10], 23, 23)] * (bits(w[10], 22, 19) + 1);
	p_dec_val(tmp, "Typical Additional Byte Program Time", "in microseconds");
	tmp = erase_chip_time_mult[bits(w[10], 30, 29)] * (bits(w[10], 28, 24) + 1);
	p_dec_val(tmp, "Typical Chip Erase Time", "in milliseconds");
	

	return 0;
}

struct sfdp_param_handler {
	uint16_t id;
	const char *name;
	int (*dumper)(uint32_t *ptr, size_t len);
};

static const struct sfdp_param_handler sfdp_unknown_param = {
	0, "Unknown parameter", sfdp_unknown_parameter
};

static const struct sfdp_param_handler handlers[] = {
	{ 0xff00, "Basic flash parameter table", sfdp_basic }
};

/*
 * Main program
 */

int main(int argc, char *argv[])
{
	static uint32_t buf[4096];
	struct {
		struct sfdp_hdr hdr;
		struct sfdp_param_hdr param[];
	} *sfdp = (void*)buf;
	int sz, i, j;
	FILE *f;

	if (argc < 2) {
		f = stdin;
	} else if (!strcmp("--help", argv[1])) {
		printf("Parse SFDP to readable format.\n"
		       "Usage: %s SFDP_DUMP\n", argv[0]);
		return 0;
	} else {
		f = fopen(argv[1], "r");
		if (!f)
			err(1, "Can't open '%s'", argv[1]);
	}

	sz = fread(buf, 1, sizeof buf, f);
	if (sz < 0)
		err(1, "Can't read '%s'", argv[1]);
	else if (sz < sizeof *sfdp)
		errx(1, "File '%s' is shorter than SFDP header", argv[1]);
	fclose(f);

	if (sfdp->hdr.signature == bswap_32(SFDP_SIGNATURE)) {
		for (i = 0; i < sz/4; i++)
			buf[i] = bswap_32(buf[i]);
	}

	if (sfdp->hdr.signature != SFDP_SIGNATURE) {
		errx(2, "Invalid signature %#08x, expected %#08x",
				sfdp->hdr.signature, SFDP_SIGNATURE);
	}

	p_hex_val(sfdp->hdr.signature, "Signature", "Must be 0x50444653 ('S', 'F', 'D', 'P')");
	p_dec_val(sfdp->hdr.major, "Major", NULL);
	p_dec_val(sfdp->hdr.minor, "Minor", NULL);
	p_dec_val(sfdp->hdr.nph + 1, "Parameters", NULL);
	p_dec_val(sz, "Total length", NULL);

	for (i = 0; i <= sfdp->hdr.nph; i++) {
		const struct sfdp_param_handler *h = &sfdp_unknown_param;
		struct sfdp_param_hdr *p = &sfdp->param[i];
		unsigned id = p->id_msb << 8 | p->id_lsb;
		unsigned off = p->ptr[2] << 16 | p->ptr[1] << 8 | p->ptr[0];

		for (j = 0; j < ARRAY_SIZE(handlers); j++) {
			if (id == handlers[i].id) {
				h = &handlers[i];
				break;
			}
		}

		printf("\n\nParameter %d (%s)\n", i, h->name);
		p_hex_val(id, "Major", NULL);
		p_dec_val(p->major, "Major", NULL);
		p_dec_val(p->minor, "Minor", NULL);
		p_dec_val(off, "Offset", off % 4 ? "Invalid alignment" : NULL);
		p_dec_val(p->len * 4, "Length", NULL);

		if (off % 4) {
			warnx("Unaligned offset of parameter %u", id);
			continue;
		}

		if (off + sfdp->param[i].len * 4 <= sz)
			h->dumper(&buf[off/4], sfdp->param[i].len);
		else
			warnx("Parameter %u data are behind the end of the file", id);
	}

	return 0;
}
