/* radare - LGPL - Copyright 2009-2022 - nibble, pancake */

#ifndef R2_ASM_H
#define R2_ASM_H

#include <r_types.h>
#include <r_arch.h>
#include <r_bin.h> // only for binding, no hard dep required
#include <r_util.h>
#include <r_parse.h>
#include <r_bind.h>

#ifdef __cplusplus
extern "C" {
#endif

R_LIB_VERSION_HEADER(r_asm);

enum {
	R_ASM_MOD_RAWVALUE = 'r',
	R_ASM_MOD_VALUE = 'v',
	R_ASM_MOD_DSTREG = 'd',
	R_ASM_MOD_SRCREG0 = '0',
	R_ASM_MOD_SRCREG1 = '1',
	R_ASM_MOD_SRCREG2 = '2'
};
// XXX should be using RArchOp !!!
typedef struct r_asm_op_t {
	int size; // instruction size (must be deprecated. just use buf.len
	int bitsize; // instruction size in bits (or 0 if fits in 8bit bytes) // wtf why dupe this field? :D
	int payload; // size of payload (opsize = (size-payload))
	// But this is pretty slow..so maybe we should add some accessors
	RStrBuf buf;
	RStrBuf buf_asm;
	RBuffer *buf_inc; // must die
} RAsmOp;

typedef struct r_asm_code_t {
#if 1
	int len;
	ut8 *bytes;
	char *assembly;
#else
	RAsmOp op; // we have those fields already inside RAsmOp
#endif
	RList *equs; // TODO: must be a hash
	ut64 code_offset;
	ut64 data_offset;
	int code_align;
} RAsmCode;

// TODO: use a hashtable instead of an rlist
typedef struct {
	char *key;
	char *value;
} RAsmEqu;

#define _RAsmPlugin struct r_asm_plugin_t
typedef struct r_asm_t {
	RArch *arch;
	RArchConfig *config;
	ut64 pc;
	void *user;
	RArchSession *ecur; // encode current
	RArchSession *dcur; // decode current
	_RAsmPlugin *cur; // disassemble .. should be RArchPlugin DEPRECATE
	_RAsmPlugin *acur; // assemble DEPRECATE
	// RArchSession *cur;
	// RArchSession *acur;
	RList *plugins;
	RAnalBind analb; // Should be RArchBind instead, but first we need to move all the anal plugins.. well not really we can kill it imho
	RParse *ifilter;
	RParse *ofilter;
	Sdb *pair;
	RSyscall *syscall;
	RNum *num;
	int dataalign;
	HtPP *flags;
	bool pseudo;
} RAsm;

typedef bool (*RAsmModifyCallback)(RAsm *a, ut8 *buf, int field, ut64 val);
typedef int (*RAsmAssembleCallback)(RAsm *a, RAsmOp *op, const char *buf);

typedef struct r_asm_plugin_t {
	const char *name;
	const char *arch;
	const char *author;
	const char *version;
	const char *cpus;
	const char *desc;
	const char *license;
	void *user; // user data pointer
	int bits;
	int endian;

	RAsmAssembleCallback assemble;
	RArchPluginEncodeCallback encode;
} RAsmPlugin;

#ifdef R_API
/* asm.c */
R_API RAsm *r_asm_new(void);
R_API void r_asm_free(RAsm *a);
R_API bool r_asm_modify(RAsm *a, ut8 *buf, int field, ut64 val);
R_API char *r_asm_mnemonics(RAsm *a, int id, bool json);
R_API int r_asm_mnemonics_byname(RAsm *a, const char *name);
R_API void r_asm_set_user_ptr(RAsm *a, void *user);
R_API bool r_asm_add(RAsm *a, RAsmPlugin *foo);
R_API bool r_asm_is_valid(RAsm *a, const char *name);

R_API bool r_asm_use(RAsm *a, const char *name);
R_API bool r_asm_use_assembler(RAsm *a, const char *name);
R_API bool r_asm_set_arch(RAsm *a, const char *name, int bits);
R_API int r_asm_set_bits(RAsm *a, int bits);
R_API void r_asm_set_cpu(RAsm *a, const char *cpu);
R_API bool r_asm_set_big_endian(RAsm *a, bool big_endian);

R_API bool r_asm_set_syntax(RAsm *a, int syntax); // This is in RArchConfig
R_API int r_asm_syntax_from_string(const char *name);
R_API int r_asm_set_pc(RAsm *a, ut64 pc);
R_API int r_asm_disassemble(RAsm *a, RAsmOp *op, const ut8 *buf, int len);
R_API int r_asm_assemble(RAsm *a, RAsmOp *op, const char *buf);
R_API RAsmCode* r_asm_mdisassemble(RAsm *a, const ut8 *buf, int len);
R_API RAsmCode* r_asm_mdisassemble_hexstr(RAsm *a, RParse *p, const char *hexstr);
R_API RAsmCode* r_asm_massemble(RAsm *a, const char *buf);
R_API RAsmCode* r_asm_rasm_assemble(RAsm *a, const char *buf, bool use_spp);
R_API char *r_asm_tostring(RAsm *a, ut64 addr, const ut8 *b, int l);
/* to ease the use of the native bindings (not used in r2) */
R_API ut8 *r_asm_from_string(RAsm *a, ut64 addr, const char *b, int *l);
R_API bool r_asm_sub_names_input(RAsm *a, const char *f);
R_API bool r_asm_sub_names_output(RAsm *a, const char *f);
R_API char *r_asm_describe(RAsm *a, const char* str);
R_API const RList* r_asm_get_plugins(RAsm *a);
R_API void r_asm_list_directives(void);
R_API SdbGperf *r_asm_get_gperf(const char *k);
R_API RList *r_asm_cpus(RAsm *a);

/* code.c */
R_API RAsmCode *r_asm_code_new(void);
R_API void r_asm_code_free(RAsmCode *acode);
R_API void r_asm_equ_item_free(RAsmEqu *equ);
R_API bool r_asm_code_set_equ(RAsmCode *code, const char *key, const char *value);
R_API char *r_asm_code_equ_replace(RAsmCode *code, char *str);
R_API char* r_asm_code_get_hex(RAsmCode *acode);

/* op.c XXX deprecate we have RArchOp which does the same */
R_API RAsmOp *r_asm_op_new(void);
R_API void r_asm_op_init(RAsmOp *op);
R_API void r_asm_op_free(RAsmOp *op);
R_API void r_asm_op_fini(RAsmOp *op);
R_API char *r_asm_op_get_hex(RAsmOp *op);
R_API char *r_asm_op_get_asm(RAsmOp *op);
R_API int r_asm_op_get_size(RAsmOp *op);
R_API void r_asm_op_set_asm(RAsmOp *op, const char *str);
R_API int r_asm_op_set_hex(RAsmOp *op, const char *str);
R_API int r_asm_op_set_hexbuf(RAsmOp *op, const ut8 *buf, int len);
R_API void r_asm_op_set_buf(RAsmOp *op, const ut8 *str, int len);
R_API ut8 *r_asm_op_get_buf(RAsmOp *op);

/* plugin pointers */
extern RAsmPlugin r_asm_plugin_null;
extern RAsmPlugin r_asm_plugin_arm_as;
extern RAsmPlugin r_asm_plugin_ppc_as;
extern RAsmPlugin r_asm_plugin_sparc_gnu;
extern RAsmPlugin r_asm_plugin_x86_as;
extern RAsmPlugin r_asm_plugin_x86_nasm;

#endif

#ifdef __cplusplus
}
#endif

#endif
