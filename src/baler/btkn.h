/* -*- c-basic-offset: 8 -*-
 * Copyright (c) 2013-2016 Open Grid Computing, Inc. All rights reserved.
 * Copyright (c) 2013-2016 Sandia Corporation. All rights reserved.
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
 * \file btkn.h
 * \author Narate Taerat (narate@ogc.us)
 *
 * \defgroup btkn Baler Token Management
 * \{
 * Additional to ::bmap module, this module contains extra functions necessary
 * for token management as ::bmap manages only ID\<--\>STR mapping.
 */
#ifndef __BTKN_H
#define __BTKN_H

#include "bcommon.h"
#include "btkn_types.h"
#include "btypes.h"
#include "bmapper.h"
#include "bmvec.h"
#include <sys/mman.h>
#include <linux/limits.h>
#include <sys/fcntl.h>

/**
 * bvec definition for ::btkn_attr.
 */
BVEC_DEF(bvec_tkn_attr, struct btkn_attr);

/**
 * bmvec definition for bvec_tkn_attr.
 */
BMVEC_DEF(bmvec_tkn_attr, struct bvec_tkn_attr);

/**
 * Baler token store.
 * This structure contains various structures for token management.
 */
struct btkn_store {
	char *path; /**< Path of the token store. */
	struct bmvec_tkn_attr *attr; /**< Token attribute. */
	struct bmap *map; /**< Token\<--\>ID map */
};

/**
 * Open btkn_store.
 * \param path Path to the btkn_store.
 * \returns NULL on error.
 * \returns A pointer to the store on success.
 */
struct btkn_store* btkn_store_open(const char *path, int flag);

/**
 * Close and free the given ::btkn_store \c s.
 * \param s The store to be closed and freed.
 */
void btkn_store_close_free(struct btkn_store *s);

/**
 * A function to obtain a C string from \c id.
 * \param store Tht store handle.
 * \param id The token id.
 * \param dest The destination string buffer to copy data to.
 * \param len The maximum length of the \c dest.
 * \retval ENOMEM if the \c dest is too small.
 * \retval ENOENT if \c id does not exist.
 * \retval 0 on success.
 */
int btkn_store_id2str(struct btkn_store *store, uint32_t id, char *dest,
		      int len);

/**
 * Similar to ::btkn_store_id2str, but with '\' escaping.
 *
 * This function will escape the non-printable characters and spaces with
 * '\\x##' hexadecimal character escape sequence.
 *
 * \param store Tht store handle.
 * \param id The token id.
 * \param dest The destination string buffer to copy data to.
 * \param len The maximum length of the \c dest.
 * \retval ENOMEM if the \c dest is too small.
 * \retval ENOENT if \c id does not exist.
 * \retval 0 on success.
 */
int btkn_store_id2str_esc(struct btkn_store *store, uint32_t id,
		      char *dest, int len);

/**
 * Getting token attribute of token ID \c tkn_id.
 * \param store The token store.
 * \param tkn_id The token ID.
 * \return Pointer to the attribute.
 */
static inline
struct btkn_attr btkn_store_get_attr(struct btkn_store *store, uint32_t tkn_id)
{
	struct btkn_attr attr;
	pthread_mutex_lock(&store->attr->mutex);
	attr = store->attr->bvec->data[tkn_id];
	pthread_mutex_unlock(&store->attr->mutex);
	return attr;
}

/**
 * Set token type of the token with ID \c tkn_id to \c type.
 *
 * \param store The store handle.
 * \param tkn_id Token ID.
 * \param type The type of the token.
 */
static inline
void btkn_store_set_attr(struct btkn_store *store, uint32_t tkn_id,
			 struct btkn_attr attr)
{
	bmvec_generic_set((void*)store->attr, tkn_id, &attr, sizeof(attr));
}

/**
 * Insert \c str into the store.
 *
 * If \c str existed, this function does nothing and return the ID of \c str.
 *
 * \return If \c str existed or is successfully inserted, the ID of the token
 *	\c str is returned.
 * \return On error, the function returns ::BMAP_ID_ERR.
 */
static inline
uint32_t btkn_store_insert(struct btkn_store *store, struct bstr *str)
{
	uint32_t id;
	id = bmap_insert(store->map, str);
	if (id == BMAP_ID_ERR)
		/* errno should be set in bmap_insert() */
		goto out;

	/* set attribute to '*' by default for new token */
	struct btkn_attr attr;
	if (bmvec_generic_get_len((void*)store->attr) <= id ) {
		attr.type = BTKN_TYPE_MASK(BTKN_TYPE_TEXT);
		btkn_store_set_attr(store, id, attr);
	}

out:
	return id;
}

static inline
uint32_t btkn_store_insert_with_id(struct btkn_store *store, struct bstr *str, uint32_t id)
{
	uint32_t _id;
	_id = bmap_insert_with_id(store->map, str, id);
	if (_id == BMAP_ID_ERR)
		/* errno should be set in bmap_insert() */
		goto out;

	/* set attribute to '*' by default for new token */
	struct btkn_attr attr;
	if (bmvec_generic_get_len((void*)store->attr) <= id ) {
		attr.type = BTKN_TYPE_MASK(BTKN_TYPE_TEXT);
		btkn_store_set_attr(store, id, attr);
	}

out:
	return _id;
}

static inline
uint32_t btkn_store_get_id(struct btkn_store *store, struct bstr *bstr)
{
	return bmap_get_id(store->map, bstr);
}

static inline
const struct bstr *btkn_store_get_bstr(struct btkn_store *store, uint32_t id)
{
	return bmap_get_bstr(store->map, id);
}

/**
 * Convenient function for inserting C string with token type into the store.
 * \returns Token ID on success.
 * \returns BMAP_ID_ERR on error, and errno will be set accordingly.
 */
uint32_t btkn_store_insert_cstr(struct btkn_store *store, const char *str,
							btkn_type_t type);

/**
 * Treat \c cstr as an array of char and insert each of the character into the
 * \c store as single token. The tokens will be assigned with type \c type.
 */
int btkn_store_char_insert(struct btkn_store *store, const char *cstr,
							btkn_type_t type);

/**
 * Refresh the underlying ::bmap.
 *
 * \retval 0 if OK.
 * \retval errno if error.
 */
int btkn_store_refresh(struct btkn_store *store);

/**
 * Iterate through the \c btkn_store, calling the \c cb function on each token
 * in the store.
 *
 * \note The iteration stopped when the \c cb function return non-zero value.
 *
 * \param btkn_store The store handle.
 * \param cb The callback function.
 */
void btkn_store_iterate(struct btkn_store *btkn_store,
			int (*cb)(uint32_t id, const struct bstr *bstr,
					const struct btkn_attr *attr));

btkn_type_t btkn_type_from_str(const char *str);

/**
 * Get Token Type Mask from \c str.
 *
 * This is a convenient utility that converts input string \c str into a token
 * type mask. The input string can be in the following format:
 *
 * 1. hexadecimal with leading 0x, e.g. "0xABCD",
 * 2. type name, e.g. "HOSTNAME",
 * 3. ored expression of types, e.g. "HOSTNAME|IP4_ADDR|IP6_ADDR".
 *
 * \param str the input string.
 *
 * \retval mask the token type mask.
 * \retval 0 for error. In this case, \c errno will also be set.
 */
uint64_t btkn_type_mask_from_str(const char *str);
#endif /* __BTKN_H */
/**\}*/
