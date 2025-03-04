#include <r_lib.h>
#include <r_crypto.h>

#define NAME "ror"

enum { MAX_ror_KEY_SIZE = 32768 };

struct ror_state {
	ut8 key[MAX_ror_KEY_SIZE];
	int key_size;
};

static bool ror_init(struct ror_state *const state, const ut8 *key, int keylen) {
	if (!state || !key || keylen < 1 || keylen > MAX_ror_KEY_SIZE) {
		return false;
	}
	int i;
	state->key_size = keylen;
	for (i = 0; i < keylen; i++) {
		state->key[i] = key[i];
	}
	return true;
}

static void ror_crypt(struct ror_state *const state, const ut8 *inbuf, ut8 *outbuf, int buflen) {
	int i;
	for (i = 0; i < buflen; i++) {
		ut8 count = state->key[i % state->key_size] & 7;
		ut8 inByte = inbuf[i];
		outbuf[i] = (inByte >> count) | (inByte << ((8 - count) & 7));
	}
}

static R_TH_LOCAL struct ror_state st;
static R_TH_LOCAL int flag = 0;

static bool ror_set_key(RCryptoJob *cj, const ut8 *key, int keylen, int mode, int direction) {
	flag = direction;
	return ror_init (&st, key, keylen);
}

static int ror_get_key_size(RCryptoJob *cj) {
	return st.key_size;
}

static bool ror_check(const char *algo) {
	return !strcmp (algo, NAME);
}

static bool update(RCryptoJob *cj, const ut8 *buf, int len) {
	if (flag) {
		eprintf ("USE ROL\n");
		return false;
	}
	ut8 *obuf = calloc (1, len);
	if (!obuf) {
		return false;
	}
	ror_crypt (&st, buf, obuf, len);
	r_crypto_job_append (cj, obuf, len);
	free (obuf);
	return true;
}

RCryptoPlugin r_crypto_plugin_ror = {
	.name = NAME,
	.set_key = ror_set_key,
	.get_key_size = ror_get_key_size,
	.check = ror_check,
	.update = update,
	.end = update,
};

#ifndef R2_PLUGIN_INCORE
R_API RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_CRYPTO,
	.data = &r_crypto_plugin_ror,
	.version = R2_VERSION
};
#endif
