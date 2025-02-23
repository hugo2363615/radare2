/* radare - LGPL - Copyright 2022 - pancake, condret */

#include <r_arch.h>
#include <config.h>

static const RArchPlugin * const arch_static_plugins[] = { R_ARCH_STATIC_PLUGINS };

static void plugin_free(void *p) {
	// XXX
}

R_API RArch *r_arch_new(void) {
	RArch *a = R_NEW0 (RArch);
	if (!a) {
		return NULL;
	}
	a->plugins = r_list_newf ((RListFree)plugin_free);
	if (!a->plugins) {
		free (a);
		return NULL;
	}
	a->num = r_num_new (NULL, NULL, NULL);
	a->cfg = r_arch_config_new ();
	ut32 i = 0;
	while (arch_static_plugins[i]) {
		r_arch_add (a, (RArchPlugin*)arch_static_plugins[i++]);
	}
	return a;
}

static ut32 _rate_compat(RArchPlugin *p, RArchConfig *cfg, const char *name) {
	ut32 bits = cfg->bits;
	ut32 score = 0;
	if (name && !strcmp (p->name, name)) {
		score += 50;
	}
	if (cfg->arch && !strcmp (p->arch, cfg->arch)) {
		score += 50;
	}
	if (score > 0) {
		if (strstr (p->name, ".nz")) {
			score += 50;
		}
		if (R_SYS_BITS_CHECK (p->bits, bits)) {
			score += (!!score) * 30;
		}
		if (p->endian & cfg->endian) {
			score += (!!score) * 20;
		}
	}
	return score;
}

static RArchPlugin *find_bestmatch(RArch *arch, RArchConfig *cfg, const char *name) {
	ut8 best_score = 0;
	RArchPlugin *ap = NULL;
	RListIter *iter;
	RArchPlugin *p;
	r_list_foreach (arch->plugins, iter, p) {
		const ut32 score = _rate_compat (p, cfg, name);
		if (score > 0 && score > best_score) {
			best_score = score;
			ap = p;
		}
	}
	return ap;
}

// use config as new arch config and use matching decoder as current
// must return arch->current, and remove that field. and use refcounting
R_API bool r_arch_use(RArch *arch, RArchConfig *config, const char *name) {
	r_return_val_if_fail (arch, false);
	if (!config) {
		config = arch->cfg;
	}
#if 0
	if (config && arch->cfg == config) {
		eprintf ("retur\n");
		return true;
	}
#endif
	RArchPlugin *ap = find_bestmatch (arch, config, name);
	if (!ap) {
		r_unref (arch->session);
		arch->session = NULL;
		return false;
	}
	arch->session = r_arch_session (arch, config, ap);
#if 0
	RArchConfig *oconfig = arch->cfg;
	r_unref (arch->cfg);
	arch->cfg = config;
	r_ref (arch->cfg);
	r_unref (oconfig);
#endif
	return true;
}

R_API bool r_arch_use_decoder(RArch *arch, const char *dname) {
	RArchConfig *cfg = r_arch_config_clone (arch->cfg);
	bool r = r_arch_use (arch, cfg, dname);
	if (!r) {
		r_unref (cfg);
	}
	return r;
}

R_API bool r_arch_use_encoder(RArch *arch, const char *dname) {
	/// XXX this should be storing the plugin in a separate pointer
	return r_arch_use (arch, arch->cfg, dname);
}

// set bits and update config
// This api conflicts with r_arch_config_set_bits
R_API bool r_arch_set_bits(RArch *arch, ut32 bits) {
	r_return_val_if_fail (arch && bits, false);
	if (!arch->cfg) {
		RArchConfig *cfg = r_arch_config_new ();
		if (!cfg) {
			return false;
		}
		// TODO: check if archplugin supports those bits?
		// r_arch_config_set_bits (arch->cfg, bits);
		cfg->bits = bits;
		if (!r_arch_use (arch, cfg, NULL)) {
			r_unref (cfg);
			arch->cfg = NULL;
			return false;
		}
		return true;
	}
	arch->cfg->bits = bits;
	return true;
}

R_API bool r_arch_set_endian(RArch *arch, ut32 endian) {
	r_return_val_if_fail (arch, false);
	if (!arch->cfg) {
		RArchConfig *cfg = r_arch_config_new ();
		if (!cfg) {
			return false;
		}
		cfg->endian = endian;
		if (!r_arch_use (arch, cfg, NULL)) {
			r_unref (cfg);
			arch->cfg = NULL;
			return false;
		}
		return true;
	}
	arch->cfg->endian = endian;
	return true;
}

R_API bool r_arch_set_arch(RArch *arch, char *archname) {
	// Rename to _use_arch instead ?
	r_return_val_if_fail (arch && archname, false);
	char *_arch = strdup (archname);
	if (!_arch) {
		return false;
	}
	if (!arch->cfg) {
		RArchConfig *cfg = r_arch_config_new ();
		if (!cfg) {
			free (_arch);
			return false;
		}
		free (cfg->arch);
		cfg->arch =_arch;
		if (!r_arch_use (arch, cfg, archname)) {
			r_unref (cfg);
			return false;
		}
		return true;
	}
	free (arch->cfg->arch);
	arch->cfg->arch = _arch;
	return true;
}

R_API bool r_arch_add(RArch *a, RArchPlugin *ap) {
	r_return_val_if_fail (a && ap->name && ap->arch, false);
	return !!r_list_append (a->plugins, ap);
}

R_API bool r_arch_del(RArch *arch, const char *name) {
	r_return_val_if_fail (arch && arch->plugins && name, false);
	RArchPlugin *ap = find_bestmatch (arch, NULL, name);
#if 0
	if (arch->current && !strcmp (arch->current->p->name, name)) {
		arch->current = NULL;
	}
#endif
	r_list_delete_data (arch->plugins, ap);
	return false;
}

R_API void r_arch_free(RArch *arch) {
	if (arch) {
		// ht_pp_free (arch->decoders);
		r_list_free (arch->plugins);
		r_unref (arch->cfg);
		free (arch);
	}
}

R_API int r_arch_info(RArch *a, int query) {
	// XXX should be unused, because its not tied to a session
	RArchSession *session = R_UNWRAP2 (a, session);
	RArchPluginInfoCallback info = R_UNWRAP4 (a, session, plugin, info);
	return info? info (session, query): -1;
}

R_API bool r_arch_encode(RArch *a, RAnalOp *op, RArchEncodeMask mask) {
	// XXX should be unused
	RArchPluginEncodeCallback encode = R_UNWRAP4 (a, session, plugin, encode);
	return encode? encode (a->session, op, mask): false;
}

R_API bool r_arch_decode(RArch *a, RAnalOp *op, RArchDecodeMask mask) {
	// XXX should be unused
	RArchPluginEncodeCallback decode = R_UNWRAP4 (a, session, plugin, decode);
	return decode? decode (a->session, op, mask): false;
}
