/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_CTF_H
#define	_CTF_H

#if defined(sun)
#pragma ident	"%Z%%M%	%I%	%E% SMI"
#endif

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * CTF - Compact ANSI-C Type Format
 *
 * This file format can be used to compactly represent the information needed
 * by a debugger to interpret the ANSI-C types used by a given program.
 * Traditionally, this kind of information is generated by the compiler when
 * invoked with the -g flag and is stored in "stabs" strings or in the more
 * modern DWARF format.  CTF provides a representation of only the information
 * that is relevant to debugging a complex, optimized C program such as the
 * operating system kernel in a form that is significantly more compact than
 * the equivalent stabs or DWARF representation.  The format is data-model
 * independent, so consumers do not need different code depending on whether
 * they are 32-bit or 64-bit programs.  CTF assumes that a standard ELF symbol
 * table is available for use in the debugger, and uses the structure and data
 * of the symbol table to avoid storing redundant information.  The CTF data
 * may be compressed on disk or in memory, indicated by a bit in the header.
 * CTF may be interpreted in a raw disk file, or it may be stored in an ELF
 * section, typically named .SUNW_ctf.  Data structures are aligned so that
 * a raw CTF file or CTF ELF section may be manipulated using mmap(2).
 *
 * The CTF file or section itself has the following structure:
 *
 * +--------+--------+---------+----------+-------+--------+
 * |  file  |  type  |  data   | function | data  | string |
 * | header | labels | objects |   info   | types | table  |
 * +--------+--------+---------+----------+-------+--------+
 *
 * The file header stores a magic number and version information, encoding
 * flags, and the byte offset of each of the sections relative to the end of the
 * header itself.  If the CTF data has been uniquified against another set of
 * CTF data, a reference to that data also appears in the the header.  This
 * reference is the name of the label corresponding to the types uniquified
 * against.
 *
 * Following the header is a list of labels, used to group the types included in
 * the data types section.  Each label is accompanied by a type ID i.  A given
 * label refers to the group of types whose IDs are in the range [0, i].
 *
 * Data object and function records are stored in the same order as they appear
 * in the corresponding symbol table, except that symbols marked SHN_UNDEF are
 * not stored and symbols that have no type data are padded out with zeroes.
 * For each data object, the type ID (a small integer) is recorded.  For each
 * function, the type ID of the return type and argument types is recorded.
 *
 * The data types section is a list of variable size records that represent each
 * type, in order by their ID.  The types themselves form a directed graph,
 * where each node may contain one or more outgoing edges to other type nodes,
 * denoted by their ID.
 *
 * Strings are recorded as a string table ID (0 or 1) and a byte offset into the
 * string table.  String table 0 is the internal CTF string table.  String table
 * 1 is the external string table, which is the string table associated with the
 * ELF symbol table for this object.  CTF does not record any strings that are
 * already in the symbol table, and the CTF string table does not contain any
 * duplicated strings.
 *
 * If the CTF data has been merged with another parent CTF object, some outgoing
 * edges may refer to type nodes that exist in another CTF object.  The debugger
 * and libctf library are responsible for connecting the appropriate objects
 * together so that the full set of types can be explored and manipulated.
 */

#define	CTF_MAX_TYPE	0xffff	/* max type identifier value */
#define	CTF_MAX_NAME 0x7fffffff	/* max offset into a string table */
#define	CTF_MAX_VLEN	0x3ff	/* max struct, union, enum members or args */
#define	CTF_MAX_INTOFF	0xff	/* max offset of intrinsic value in bits */
#define	CTF_MAX_INTBITS	0xffff	/* max size of an intrinsic in bits */

/* See ctf_type_t */
#define	CTF_MAX_SIZE	0xfffe	/* max size of a type in bytes */
#define	CTF_LSIZE_SENT	0xffff	/* sentinel for ctt_size */
#define	CTF_MAX_LSIZE	UINT64_MAX

typedef struct ctf_preamble {
	ushort_t ctp_magic;	/* magic number (CTF_MAGIC) */
	uchar_t ctp_version;	/* data format version number (CTF_VERSION) */
	uchar_t ctp_flags;	/* flags (see below) */
} ctf_preamble_t;

typedef struct ctf_header {
	ctf_preamble_t cth_preamble;
	uint_t cth_parlabel;	/* ref to name of parent lbl uniq'd against */
	uint_t cth_parname;	/* ref to basename of parent */
	uint_t cth_lbloff;	/* offset of label section */
	uint_t cth_objtoff;	/* offset of object section */
	uint_t cth_funcoff;	/* offset of function section */
	uint_t cth_typeoff;	/* offset of type section */
	uint_t cth_stroff;	/* offset of string section */
	uint_t cth_strlen;	/* length of string section in bytes */
} ctf_header_t;

#define	cth_magic   cth_preamble.ctp_magic
#define	cth_version cth_preamble.ctp_version
#define	cth_flags   cth_preamble.ctp_flags

#ifdef CTF_OLD_VERSIONS

typedef struct ctf_header_v1 {
	ctf_preamble_t cth_preamble;
	uint_t cth_objtoff;
	uint_t cth_funcoff;
	uint_t cth_typeoff;
	uint_t cth_stroff;
	uint_t cth_strlen;
} ctf_header_v1_t;

#endif /* CTF_OLD_VERSIONS */

#define	CTF_MAGIC	0xcff1	/* magic number identifying header */

/* data format version number */
#define	CTF_VERSION_1	1
#define	CTF_VERSION_2	2
#define	CTF_VERSION	CTF_VERSION_2	/* current version */

#define	CTF_F_COMPRESS	0x1	/* data buffer is compressed */

typedef struct ctf_lblent {
	uint_t ctl_label;	/* ref to name of label */
	uint_t ctl_typeidx;	/* last type associated with this label */
} ctf_lblent_t;

typedef struct ctf_stype {
	uint_t ctt_name;	/* reference to name in string table */
	ushort_t ctt_info;	/* encoded kind, variant length (see below) */
	union {
		ushort_t _size;	/* size of entire type in bytes */
		ushort_t _type;	/* reference to another type */
	} _u;
} ctf_stype_t;

/*
 * type sizes, measured in bytes, come in two flavors.  99% of them fit within
 * (USHRT_MAX - 1), and thus can be stored in the ctt_size member of a
 * ctf_stype_t.  The maximum value for these sizes is CTF_MAX_SIZE.  The sizes
 * larger than CTF_MAX_SIZE must be stored in the ctt_lsize member of a
 * ctf_type_t.  Use of this member is indicated by the presence of
 * CTF_LSIZE_SENT in ctt_size.
 */
typedef struct ctf_type {
	uint_t ctt_name;	/* reference to name in string table */
	ushort_t ctt_info;	/* encoded kind, variant length (see below) */
	union {
		ushort_t _size;	/* always CTF_LSIZE_SENT */
		ushort_t _type; /* do not use */
	} _u;
	uint_t ctt_lsizehi;	/* high 32 bits of type size in bytes */
	uint_t ctt_lsizelo;	/* low 32 bits of type size in bytes */
} ctf_type_t;

#define	ctt_size _u._size	/* for fundamental types that have a size */
#define	ctt_type _u._type	/* for types that reference another type */

/*
 * The following macros compose and decompose values for ctt_info and
 * ctt_name, as well as other structures that contain name references.
 *
 *             ------------------------
 * ctt_info:   | kind | isroot | vlen |
 *             ------------------------
 *             15   11    10    9     0
 *
 * kind = CTF_INFO_KIND(c.ctt_info);     <-- CTF_K_* value (see below)
 * vlen = CTF_INFO_VLEN(c.ctt_info);     <-- length of variable data list
 *
 * stid = CTF_NAME_STID(c.ctt_name);     <-- string table id number (0 or 1)
 * offset = CTF_NAME_OFFSET(c.ctt_name); <-- string table byte offset
 *
 * c.ctt_info = CTF_TYPE_INFO(kind, vlen);
 * c.ctt_name = CTF_TYPE_NAME(stid, offset);
 */

#define	CTF_INFO_KIND(info)	(((info) & 0xf800) >> 11)
#define	CTF_INFO_ISROOT(info)	(((info) & 0x0400) >> 10)
#define	CTF_INFO_VLEN(info)	(((info) & CTF_MAX_VLEN))

#define	CTF_NAME_STID(name)	((name) >> 31)
#define	CTF_NAME_OFFSET(name)	((name) & 0x7fffffff)

#define	CTF_TYPE_INFO(kind, isroot, vlen) \
	(((kind) << 11) | (((isroot) ? 1 : 0) << 10) | ((vlen) & CTF_MAX_VLEN))

#define	CTF_TYPE_NAME(stid, offset) \
	(((stid) << 31) | ((offset) & 0x7fffffff))

#define	CTF_TYPE_ISPARENT(id)	((id) < 0x8000)
#define	CTF_TYPE_ISCHILD(id)	((id) > 0x7fff)

#define	CTF_TYPE_TO_INDEX(id)		((id) & 0x7fff)
#define	CTF_INDEX_TO_TYPE(id, child)	((child) ? ((id) | 0x8000) : (id))
#define	CTF_PARENT_SHIFT	15

#define	CTF_STRTAB_0	0	/* symbolic define for string table id 0 */
#define	CTF_STRTAB_1	1	/* symbolic define for string table id 1 */

#define	CTF_TYPE_LSIZE(cttp) \
	(((uint64_t)(cttp)->ctt_lsizehi) << 32 | (cttp)->ctt_lsizelo)
#define	CTF_SIZE_TO_LSIZE_HI(size)	((uint32_t)((uint64_t)(size) >> 32))
#define	CTF_SIZE_TO_LSIZE_LO(size)	((uint32_t)(size))

#ifdef CTF_OLD_VERSIONS

#define	CTF_INFO_KIND_V1(info)		(((info) & 0xf000) >> 12)
#define	CTF_INFO_ISROOT_V1(info)	(((info) & 0x0800) >> 11)
#define	CTF_INFO_VLEN_V1(info)		(((info) & 0x07ff))

#define	CTF_TYPE_INFO_V1(kind, isroot, vlen) \
	(((kind) << 12) | (((isroot) ? 1 : 0) << 11) | ((vlen) & 0x07ff))

#endif /* CTF_OLD_VERSIONS */

/*
 * Values for CTF_TYPE_KIND().  If the kind has an associated data list,
 * CTF_INFO_VLEN() will extract the number of elements in the list, and
 * the type of each element is shown in the comments below.
 */
#define	CTF_K_UNKNOWN	0	/* unknown type (used for padding) */
#define	CTF_K_INTEGER	1	/* variant data is CTF_INT_DATA() (see below) */
#define	CTF_K_FLOAT	2	/* variant data is CTF_FP_DATA() (see below) */
#define	CTF_K_POINTER	3	/* ctt_type is referenced type */
#define	CTF_K_ARRAY	4	/* variant data is single ctf_array_t */
#define	CTF_K_FUNCTION	5	/* ctt_type is return type, variant data is */
				/* list of argument types (ushort_t's) */
#define	CTF_K_STRUCT	6	/* variant data is list of ctf_member_t's */
#define	CTF_K_UNION	7	/* variant data is list of ctf_member_t's */
#define	CTF_K_ENUM	8	/* variant data is list of ctf_enum_t's */
#define	CTF_K_FORWARD	9	/* no additional data; ctt_name is tag */
#define	CTF_K_TYPEDEF	10	/* ctt_type is referenced type */
#define	CTF_K_VOLATILE	11	/* ctt_type is base type */
#define	CTF_K_CONST	12	/* ctt_type is base type */
#define	CTF_K_RESTRICT	13	/* ctt_type is base type */

#define	CTF_K_MAX	31	/* Maximum possible CTF_K_* value */

/*
 * Values for ctt_type when kind is CTF_K_INTEGER.  The flags, offset in bits,
 * and size in bits are encoded as a single word using the following macros.
 */
#define	CTF_INT_ENCODING(data)	(((data) & 0xff000000) >> 24)
#define	CTF_INT_OFFSET(data)	(((data) & 0x00ff0000) >> 16)
#define	CTF_INT_BITS(data)	(((data) & 0x0000ffff))

#define	CTF_INT_DATA(encoding, offset, bits) \
	(((encoding) << 24) | ((offset) << 16) | (bits))

#define	CTF_INT_SIGNED	0x01	/* integer is signed (otherwise unsigned) */
#define	CTF_INT_CHAR	0x02	/* character display format */
#define	CTF_INT_BOOL	0x04	/* boolean display format */
#define	CTF_INT_VARARGS	0x08	/* varargs display format */

/*
 * Values for ctt_type when kind is CTF_K_FLOAT.  The encoding, offset in bits,
 * and size in bits are encoded as a single word using the following macros.
 */
#define	CTF_FP_ENCODING(data)	(((data) & 0xff000000) >> 24)
#define	CTF_FP_OFFSET(data)	(((data) & 0x00ff0000) >> 16)
#define	CTF_FP_BITS(data)	(((data) & 0x0000ffff))

#define	CTF_FP_DATA(encoding, offset, bits) \
	(((encoding) << 24) | ((offset) << 16) | (bits))

#define	CTF_FP_SINGLE	1	/* IEEE 32-bit float encoding */
#define	CTF_FP_DOUBLE	2	/* IEEE 64-bit float encoding */
#define	CTF_FP_CPLX	3	/* Complex encoding */
#define	CTF_FP_DCPLX	4	/* Double complex encoding */
#define	CTF_FP_LDCPLX	5	/* Long double complex encoding */
#define	CTF_FP_LDOUBLE	6	/* Long double encoding */
#define	CTF_FP_INTRVL	7	/* Interval (2x32-bit) encoding */
#define	CTF_FP_DINTRVL	8	/* Double interval (2x64-bit) encoding */
#define	CTF_FP_LDINTRVL	9	/* Long double interval (2x128-bit) encoding */
#define	CTF_FP_IMAGRY	10	/* Imaginary (32-bit) encoding */
#define	CTF_FP_DIMAGRY	11	/* Long imaginary (64-bit) encoding */
#define	CTF_FP_LDIMAGRY	12	/* Long double imaginary (128-bit) encoding */

#define	CTF_FP_MAX	12	/* Maximum possible CTF_FP_* value */

typedef struct ctf_array {
	ushort_t cta_contents;	/* reference to type of array contents */
	ushort_t cta_index;	/* reference to type of array index */
	uint_t cta_nelems;	/* number of elements */
} ctf_array_t;

/*
 * Most structure members have bit offsets that can be expressed using a
 * short.  Some don't.  ctf_member_t is used for structs which cannot
 * contain any of these large offsets, whereas ctf_lmember_t is used in the
 * latter case.  If ctt_size for a given struct is >= 8192 bytes, all members
 * will be stored as type ctf_lmember_t.
 */

#define	CTF_LSTRUCT_THRESH	8192

typedef struct ctf_member {
	uint_t ctm_name;	/* reference to name in string table */
	ushort_t ctm_type;	/* reference to type of member */
	ushort_t ctm_offset;	/* offset of this member in bits */
} ctf_member_t;

typedef struct ctf_lmember {
	uint_t ctlm_name;	/* reference to name in string table */
	ushort_t ctlm_type;	/* reference to type of member */
	ushort_t ctlm_pad;	/* padding */
	uint_t ctlm_offsethi;	/* high 32 bits of member offset in bits */
	uint_t ctlm_offsetlo;	/* low 32 bits of member offset in bits */
} ctf_lmember_t;

#define	CTF_LMEM_OFFSET(ctlmp) \
	(((uint64_t)(ctlmp)->ctlm_offsethi) << 32 | (ctlmp)->ctlm_offsetlo)
#define	CTF_OFFSET_TO_LMEMHI(offset)	((uint32_t)((uint64_t)(offset) >> 32))
#define	CTF_OFFSET_TO_LMEMLO(offset)	((uint32_t)(offset))

typedef struct ctf_enum {
	uint_t cte_name;	/* reference to name in string table */
	int cte_value;		/* value associated with this name */
} ctf_enum_t;

#ifdef	__cplusplus
}
#endif

#endif	/* _CTF_H */
