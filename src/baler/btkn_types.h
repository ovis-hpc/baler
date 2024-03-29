/* -*- c-basic-offset: 8 -*-
 * Copyright (c) 2015 Open Grid Computing, Inc. All rights reserved.
 * Copyright (c) 2015 Sandia Corporation. All rights reserved.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Export of this program may require a license from the United States
 * Government.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the BSD-type
 * license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *      Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *      Neither the name of Sandia nor the names of any contributors may
 *      be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *      Neither the name of Open Grid Computing nor the names of any
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *      Modified source versions must be plainly marked as such, and
 *      must not be misrepresented as being the original software.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \file btkn_types.h
 * \author Narate Taerat (narate at ogc dot us)
 */
#ifndef __BTKN_TYPES_H
#define __BTKN_TYPES_H

#include "bcommon.h"

enum btkn_type {
	BTKN_TYPE_FIRST = 1,
	/**
	 * This token is a type name
	 */
	BTKN_TYPE_TYPE = 1,
	/**
	 * Input message priority
	 */
	BTKN_TYPE_PRIORITY,
	/**
	 * Input message syntax version
	 */
	BTKN_TYPE_VERSION,
	/**
	 * Timestamp
	 */
	BTKN_TYPE_TIMESTAMP,
	/**
	 * Hostname
	 */
	BTKN_TYPE_HOSTNAME,
	/**
	 * Service Name
	 */
	BTKN_TYPE_SERVICE,
	/**
	 * Process Id
	 */
	BTKN_TYPE_PID,
	/**
	 * IPv4 Address
	 */
	BTKN_TYPE_IP4_ADDR,
	/**
	 * IPv6 Address
	 */
	BTKN_TYPE_IP6_ADDR,
	/**
	 * Ethernet Address
	 */
	BTKN_TYPE_ETH_ADDR,
	/**
	 * Hexadecimal Integer
	 */
	BTKN_TYPE_HEX_INT,
	/**
	 * Decimal Integer
	 */
	BTKN_TYPE_DEC_INT,
	/**
	 * Floating point number
	 */
	BTKN_TYPE_FLOAT,
	/**
	 * Number - used by parsers that want to collapse numeric values
	 * into a single type
	 */
	BTKN_TYPE_NUMBER,
	/**
	 * Unix filesystem path
	 */
	BTKN_TYPE_PATH,
	/**
	 * W3 URL
	 */
	BTKN_TYPE_URL,
	/**
	 * Natural language word
	 */
	BTKN_TYPE_WORD,
	/**
	 * Separator or punctuation
	 */
	BTKN_TYPE_SEPARATOR,
	/**
	 * Whitespace
	 */
	BTKN_TYPE_WHITESPACE,
	/**
	 * Text unrecognized as a particular token type
	 */
	BTKN_TYPE_TEXT,
	BTKN_TYPE_LAST_BUILTIN = BTKN_TYPE_TEXT,
	/**
	 * First user-defined token. Range between BTKN_TYPE_TEXT and
	 * BTKN_TYPE_FIRST_USER reserved for expanding built-in types
	 * without colliding with user-defined types.
	 *
	 * User types are defined by the parser
	 */
	BTKN_TYPE_FIRST_USER = 32,
	/**
	 * Types are stored in a mask that is 64 bits wide
	 */
	BTKN_TYPE_LAST = 64,
};
#define BTKN_TYPE_IDX_MASK	0x3F
#define BTKN_TYPE_ID_MASK	0xFF
#define BTKN_TYPE_WILDCARD	0x80

typedef uint64_t btkn_type_t;

extern
const char *btkn_type_str[];

/**
 * Baler's Token Attributes, containing attributes of a token.
 */
struct btkn_attr {
	btkn_type_t type;
};

static
const char *btkn_attr_type_str(btkn_type_t t)
{
	if (t <= BTKN_TYPE_LAST_BUILTIN)
		return btkn_type_str[t];
	return NULL;
}

static inline
btkn_type_t btkn_type_from_id(uint64_t id)
{
	return (id & BTKN_TYPE_IDX_MASK);
}

static inline
int btkn_is_type(uint64_t id)
{
	return (id & BTKN_TYPE_ID_MASK);
}

static inline
int btkn_id_is_wildcard(uint64_t id)
{
	switch (id) {
	case BTKN_TYPE_WORD:
	case BTKN_TYPE_SEPARATOR:
	case BTKN_TYPE_WHITESPACE:
		return 0;
	default:
		return 1;
	}
	// return (id & BTKN_TYPE_WILDCARD);
}

#endif
