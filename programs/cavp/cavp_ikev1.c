/*
 * Parse IKEv1 CAVP test functions, for libreswan
 *
 * Copyright (C) 2015-2016 Andrew Cagney <cagney@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "lswalloc.h"
#include "ike_alg.h"

#include "ike_alg_sha1.h"
#include "ike_alg_sha2.h"

#include "ikev1_prf.h"
#include "crypt_symkey.h"

#include "cavp.h"
#include "cavp_entry.h"
#include "cavp_print.h"
#include "cavp_ikev1.h"

static void ikev1_skeyid_alphabet(const struct prf_desc *prf,
				  PK11SymKey *g_xy,
				  chunk_t cky_i, chunk_t cky_r,
				  PK11SymKey *skeyid)
{
	PK11SymKey *skeyid_d = ikev1_skeyid_d(prf, skeyid,
					      g_xy, cky_i, cky_r);
	print_symkey("SKEYID_d", skeyid_d, 0);

	PK11SymKey *skeyid_a = ikev1_skeyid_a(prf, skeyid, skeyid_d,
					      g_xy, cky_i, cky_r);
	print_symkey("SKEYID_a", skeyid_a, 0);

	PK11SymKey *skeyid_e = ikev1_skeyid_e(prf, skeyid, skeyid_a,
					      g_xy, cky_i, cky_r);
	print_symkey("SKEYID_e", skeyid_e, 0);

	release_symkey(__func__, "skeyid_d", &skeyid_d);
	release_symkey(__func__, "skeyid_e", &skeyid_e);
	release_symkey(__func__, "skeyid_a", &skeyid_a);
}

void cavp_acvp_ikev1_sig(const struct prf_desc *prf,
			 chunk_t ni, chunk_t nr,
			 chunk_t cky_i, chunk_t cky_r,
			 PK11SymKey *g_xy)
{
	PK11SymKey *skeyid = ikev1_signature_skeyid(prf, ni, nr, g_xy);
	print_symkey("SKEYID", skeyid, 0);
	ikev1_skeyid_alphabet(prf, g_xy, cky_i, cky_r, skeyid);
	release_symkey(__func__, "skeyid", &skeyid);
}

void cavp_acvp_ikev1_psk(const struct prf_desc *prf,
			 chunk_t ni, chunk_t nr,
			 chunk_t cky_i, chunk_t cky_r,
			 chunk_t psk,
			 PK11SymKey *g_xy)
{
	PK11SymKey *skeyid = ikev1_pre_shared_key_skeyid(prf, psk, ni, nr);
	print_symkey("SKEYID", skeyid, 0);
	ikev1_skeyid_alphabet(prf, g_xy, cky_i, cky_r, skeyid);
	release_symkey(__func__, "skeyid", &skeyid);
}


static long int ni_length;
static long int nr_length;
static long int psk_length;
static long int g_xy_length;
static struct cavp_entry *prf_entry;

static struct cavp_entry config_entries[] = {
	{ .key = "SHA-1", .op = op_entry, .entry = &prf_entry, .prf = &ike_alg_prf_sha1, },
	{ .key = "SHA-224", .op = op_entry, .entry = &prf_entry, .prf = NULL, },
	{ .key = "SHA-256", .op = op_entry, .entry = &prf_entry, .prf = &ike_alg_prf_sha2_256, },
	{ .key = "SHA-384", .op = op_entry, .entry = &prf_entry, .prf = &ike_alg_prf_sha2_384, },
	{ .key = "SHA-512", .op = op_entry, .entry = &prf_entry, .prf = &ike_alg_prf_sha2_512, },
	{ .key = "Ni length", .op = op_signed_long, .signed_long = &ni_length, },
	{ .key = "Nr length", .op = op_signed_long, .signed_long = &nr_length, },
	{ .key = "pre-shared-key length", .op = op_signed_long, .signed_long = &psk_length, },
	{ .key = "g^xy length", .op = op_signed_long, .signed_long = &g_xy_length, },
	{ .key = NULL }
};

static long int count;
static chunk_t psk;
static chunk_t ni;
static chunk_t nr;
static chunk_t cky_i;
static chunk_t cky_r;
static PK11SymKey *g_xy;

static struct cavp_entry data_entries[] = {
	{ .key = "COUNT", .op = op_signed_long, .signed_long = &count },
	{ .key = "g^xy", .op = op_symkey, .symkey = &g_xy },
	{ .key = "Ni", .op = op_chunk, .chunk = &ni },
	{ .key = "Nr", .op = op_chunk, .chunk = &nr },
	{ .key = "CKY_I", .op = op_chunk, .chunk = &cky_i },
	{ .key = "CKY_R", .op = op_chunk, .chunk = &cky_r },
	{ .key = "pre-shared-key", .op = op_chunk, .chunk = &psk },
	{ .key = "SKEYID", .op = op_ignore },
	{ .key = "SKEYID_d", .op = op_ignore },
	{ .key = "SKEYID_a", .op = op_ignore },
	{ .key = "SKEYID_e", .op = op_ignore },
	{ .key = "SKEYID_", .op = op_ignore },
	{ .op = NULL }
};

static void ikev1_sig_print_config(void)
{
	config_number("g^xy length", g_xy_length);
	config_key(prf_entry->key);
	config_number("Ni length", ni_length);
	config_number("Nr length", nr_length);
}

static void ikev1_sig_print_test(void)
{
	print_number("COUNT", count);
	print_chunk("CKY_I", cky_i, 0);
	print_chunk("CKY_R", cky_r, 0);
	print_chunk("Ni", ni, 0);
	print_chunk("Nr", nr, 0);
	print_symkey("g^xy", g_xy, 0);
	if (prf_entry->prf == NULL) {
		/* not supported, ignore */
		print_line(prf_entry->key);
		return;
	}
}

static void ikev1_sig_run_test(void)
{
	cavp_acvp_ikev1_sig(prf_entry->prf, ni, nr, cky_i, cky_r, g_xy);
}

struct cavp cavp_ikev1_sig = {
	.alias = "v1sig",
	.description = "IKE v1 Digital Signature Authentication",
	.print_config = ikev1_sig_print_config,
	.print_test = ikev1_sig_print_test,
	.run_test = ikev1_sig_run_test,
	.config = config_entries,
	.data = data_entries,
	.match = {
		"IKE v1 Digital Signature Authentication",
		NULL,
	},
};

static void ikev1_psk_print_config(void)
{
	config_number("g^xy length", g_xy_length);
	config_key(prf_entry->key);
	config_number("Ni length", ni_length);
	config_number("Nr length", nr_length);
	config_number("pre-shared-key length", psk_length);
}

static void ikev1_psk_print_test(void)
{
	print_number("COUNT", count);
	print_chunk("CKY_I", cky_i, 0);
	print_chunk("CKY_R", cky_r, 0);
	print_chunk("Ni", ni, 0);
	print_chunk("Nr", nr, 0);
	print_symkey("g^xy", g_xy, 0);
	print_chunk("pre-shared-key", psk, 0);
	if (prf_entry->prf == NULL) {
		/* not supported, ignore */
		print_line(prf_entry->key);
		return;
	}
}

static void ikev1_psk_run_test(void)
{
	cavp_acvp_ikev1_psk(prf_entry->prf, ni, nr, cky_i, cky_r, psk, g_xy);
}

struct cavp cavp_ikev1_psk = {
	.alias = "v1psk",
	.description = "IKE v1 Pre-shared Key Authentication",
	.print_config = ikev1_psk_print_config,
	.print_test = ikev1_psk_print_test,
	.run_test = ikev1_psk_run_test,
	.config = config_entries,
	.data = data_entries,
	.match = {
		"IKE v1 Pre-shared Key Authentication",
		NULL,
	},
};
