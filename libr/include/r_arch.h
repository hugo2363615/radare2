/* radare2 - LGPL - Copyright 2022 - pancake, condret */

#ifndef R2_ARCH_H
#define R2_ARCH_H

#include <r_util.h>
#include <r_bin.h>
#include <r_anal/op.h>

#ifdef __cplusplus
extern "C" {
#endif

R_LIB_VERSION_HEADER (r_arch);

#include <r_util.h>

struct r_reg_item_t;
struct r_reg_t;

#include <r_reg.h>
#include <r_io.h>
#include <sdb/sdb.h>

enum {
	R_ARCH_SYNTAX_NONE = 0,
	R_ARCH_SYNTAX_INTEL,
	R_ARCH_SYNTAX_ATT,
	R_ARCH_SYNTAX_MASM,
	R_ARCH_SYNTAX_REGNUM, // alias for capstone's NOREGNAME
	R_ARCH_SYNTAX_JZ, // hack to use jz instead of je on x86
};

typedef struct r_arch_config_t {
	char *decoder;
	char *arch;
	char *cpu;
	char *os;
	int bits;
	union {
		int big_endian;
		ut32 endian;
	};
	int syntax;
	int pcalign;
	int dataalign;
	int addrbytes; // move from RIO->addrbytes to RArchConfig->addrbytes
	int segbas;
	int seggrn;
	int invhex;
	int bitshift;
	char *abi;
	R_REF_TYPE;
} RArchConfig;

#define	R_ARCH_CONFIG_IS_BIG_ENDIAN(cfg_) (((cfg_)->endian & R_SYS_ENDIAN_BIG) == R_SYS_ENDIAN_BIG)

#define R_ARCH_INFO_MIN_OP_SIZE	0
#define R_ARCH_INFO_MAX_OP_SIZE	1
#define R_ARCH_INFO_INV_OP_SIZE	2
#define R_ARCH_INFO_ALIGN	4
#define R_ARCH_INFO_DATA_ALIGN	8
#define R_ARCH_INFO_JMPMID	16

typedef enum {
	R_ARCH_OP_MASK_BASIC = 0, // Just fills basic op info , it's fast
	R_ARCH_OP_MASK_ESIL  = 1, // It fills RAnalop->esil info
	R_ARCH_OP_MASK_VAL   = 2, // It fills RAnalop->dst/src info
	R_ARCH_OP_MASK_HINT  = 4, // It calls r_anal_op_hint to override anal options
	R_ARCH_OP_MASK_OPEX  = 8, // It fills RAnalop->opex info
	R_ARCH_OP_MASK_DISASM = 16, // It fills RAnalop->mnemonic // should be RAnalOp->disasm // only from r_core_anal_op()
	R_ARCH_OP_MASK_ALL   = 1 | 2 | 4 | 8 | 16
} RAnalOpMask;

// XXX R2_590 - backward compatible, shouldnt be used
#define R_ANAL_OP_MASK_BASIC = 0, // Just fills basic op info , it's fast
#define R_ANAL_OP_MASK_ESIL  = 1, // It fills RAnalop->esil info
#define R_ANAL_OP_MASK_VAL   = 2, // It fills RAnalop->dst/src info
#define R_ANAL_OP_MASK_HINT  = 4, // It calls r_anal_op_hint to override anal options
#define R_ANAL_OP_MASK_OPEX  = 8, // It fills RAnalop->opex info
#define R_ANAL_OP_MASK_DISASM = 16, // It fills RAnalop->mnemonic // should be RAnalOp->disasm // only from r_core_anal_op()
#define R_ANAL_OP_MASK_ALL   = 1 | 2 | 4 | 8 | 16

typedef struct r_arch_decoder_t {
	struct r_arch_plugin_t *p;
	void *user;
	ut32 refctr;
} RArchDecoder;

typedef struct r_arch_t {
	RList *plugins;	       // all plugins
	RBinBind binb; // required for java, dalvik, wasm and pyc plugin... pending refactor
	RNum *num; // XXX maybe not required
	struct r_arch_session_t *session;
#if 0
	RArchDecoder *current; // currently used decoder
#endif
	RArchConfig *cfg;      // global / default config
} RArch;

typedef struct r_arch_session_t {
	struct r_arch_t *arch;
	struct r_arch_plugin_t *plugin;
	RArchConfig *config; // TODO remove arch->config!
	void *data;
	void *user;
	R_REF_TYPE;
} RArchSession;

typedef ut32 RArchDecodeMask;
typedef ut32 RArchEncodeMask; // syntax ?

typedef int (*RArchPluginInfoCallback)(RArchSession *cfg, ut32 query);
typedef char *(*RArchPluginRegistersCallback)(RArchSession *ai);
typedef char *(*RArchPluginMnemonicsCallback)(RArchSession *s, int id, bool json);
typedef bool (*RArchPluginDecodeCallback)(RArchSession *s, struct r_anal_op_t *op, RArchDecodeMask mask);
typedef bool (*RArchPluginEncodeCallback)(RArchSession *s, struct r_anal_op_t *op, RArchEncodeMask mask);
typedef bool (*RArchPluginPluginCallback)(RArchSession *s, struct r_anal_op_t *op, RArchEncodeMask mask);
typedef bool (*RArchPluginInitCallback)(RArchSession *s);
typedef bool (*RArchPluginFiniCallback)(RArchSession *s);

// TODO: use `const char *const` instead of `char*`
typedef struct r_arch_plugin_t {
	// all const
	char *name;
	char *desc;
	char *license;
	char *arch;
	char *author;
	char *version;
	char *cpus;
	ut32 endian;
	RSysBits bits;
	RSysBits addr_bits;
	RArchPluginInitCallback init;
	RArchPluginInitCallback fini;
	RArchPluginInfoCallback info;
	RArchPluginRegistersCallback regs;
	RArchPluginEncodeCallback encode;
	RArchPluginDecodeCallback decode;
	RArchPluginEncodeCallback patch;
	RArchPluginMnemonicsCallback mnemonics;
//TODO: reenable this later
//	bool (*esil_init)(REsil *esil);
//	void (*esil_fini)(REsil *esil);
} RArchPlugin;

// decoder.c
//dname is name of decoder to use, NULL if current
R_API bool r_arch_load_decoder(RArch *arch, const char *dname);
R_API bool r_arch_use_decoder(RArch *arch, const char *dname);
R_API bool r_arch_unload_decoder(RArch *arch, const char *dname);


// deprecate
R_API int r_arch_info(RArch *arch, int query);
R_API bool r_arch_decode(RArch *a, RAnalOp *op, RArchDecodeMask mask);
R_API bool r_arch_encode(RArch *a, RAnalOp *op, RArchEncodeMask mask);
//R_API bool r_arch_esil_init(RArch *arch, const char *dname, REsil *esil);
//R_API void r_arch_esil_fini(RArch *arch, const char *dname, REsil *esil);

R_API RArchSession *r_arch_session(RArch *arch, RArchConfig *cfg, RArchPlugin *ap);
R_API bool r_arch_session_decode(RArchSession *ai, RAnalOp *op, RArchDecodeMask mask);
R_API bool r_arch_session_encode(RArchSession *ai, RAnalOp *op, RArchEncodeMask mask);
R_API bool r_arch_session_patch(RArchSession *ai, RAnalOp *op, RArchEncodeMask mask);
R_API int r_arch_session_info(RArchSession *ai, int q);

// arch.c
R_API RArch *r_arch_new(void);
R_API bool r_arch_use(RArch *arch, RArchConfig *config, const char *name);

// arch plugins management apis
R_API bool r_arch_add(RArch *arch, RArchPlugin *ap);
R_API bool r_arch_del(RArch *arch, const char *name);
R_API void r_arch_free(RArch *arch);

// deprecate
R_API bool r_arch_set_bits(RArch *arch, ut32 bits);
R_API bool r_arch_set_endian(RArch *arch, ut32 endian);
R_API bool r_arch_set_arch(RArch *arch, char *archname);

// aconfig.c
R_API void r_arch_config_use(RArchConfig *config, R_NULLABLE const char *arch);
R_API void r_arch_config_set_cpu(RArchConfig *config, R_NULLABLE const char *cpu);
R_API bool r_arch_config_set_bits(RArchConfig *c, int bits);
R_API RArchConfig *r_arch_config_new(void);
R_API RArchConfig *r_arch_config_clone(RArchConfig *c);
R_API void r_arch_config_free(RArchConfig *);

typedef enum {
	R_ANAL_VAL_REG,
	R_ANAL_VAL_MEM,
	R_ANAL_VAL_IMM,
} RArchValueType;
#define RAnalValueType RArchValueType

#define USE_REG_NAMES 0

// base + reg + regdelta * mul + delta
typedef struct r_arch_value_t {
	RArchValueType type;
	int access; // rename to `perm` and use R_PERM_R | _W | _X
	int absolute; // if true, unsigned cast is used
	int memref; // is memory reference? which size? 1, 2 ,4, 8
	ut64 base ; // numeric address
	st64 delta; // numeric delta
	st64 imm; // immediate value
	int mul; // multiplier (reg*4+base)
#if USE_REG_NAMES
	const char *seg;
	const char *reg;
	const char *regdelta;
#else
	// XXX can be invalidated if regprofile changes causing an UAF
	RRegItem *seg; // segment selector register
	RRegItem *reg; // register item reference
	RRegItem *regdelta; // register index used
#endif
} RArchValue;
// backward compat
#define RAnalValue RArchValue
R_API RArchValue *r_arch_value_new(void);
#if 0
// switchop
R_API RArchSwitchOp *r_arch_switch_op_new(ut64 addr, ut64 min_val, ut64 max_val, ut64 def_val);
R_API RArchCaseOp *r_arch_case_op_new(ut64 addr, ut64 val, ut64 jump);
R_API void r_arch_switch_op_free(RArchSwitchOp *swop);
R_API RArchCaseOp* r_arch_switch_op_add_case(RArchSwitchOp *swop, ut64 addr, ut64 value, ut64 jump);
// archvalue.c
R_API RArchValue *r_arch_value_copy(RArchValue *ov);
R_API void r_arch_value_free(RArchValue *value);
R_API ut64 r_arch_value_to_ut64(RArchValue *val, struct r_reg_t *reg);
R_API bool r_arch_value_set_ut64(RArchValue *val, struct r_reg_t *reg, RIOBind *iob, ut64 num);
R_API char *r_arch_value_tostring(RArchValue *value);
R_API RAnalOp *r_arch_op_new(void);
R_API void r_arch_op_init(RAnalOp *op);
R_API void r_arch_op_fini(RAnalOp *op);
R_API void r_arch_op_free(void *_op);
#endif

R_API int r_arch_optype_from_string(const char *type);
R_API const char *r_arch_optype_tostring(int t);
R_API const char *r_arch_stackop_tostring(int s);

R_API const char *r_arch_op_family_tostring(int n);
R_API int r_arch_op_family_from_string(const char *f);
R_API const char *r_arch_op_direction_tostring(struct r_anal_op_t *op);

extern RArchPlugin r_arch_plugin_null;
extern RArchPlugin r_arch_plugin_i4004;
extern RArchPlugin r_arch_plugin_amd29k;
extern RArchPlugin r_arch_plugin_jdh8;
extern RArchPlugin r_arch_plugin_pickle;
extern RArchPlugin r_arch_plugin_sh;
extern RArchPlugin r_arch_plugin_v810;
extern RArchPlugin r_arch_plugin_rsp;
extern RArchPlugin r_arch_plugin_riscv;
extern RArchPlugin r_arch_plugin_any_as;
extern RArchPlugin r_arch_plugin_any_vasm;
extern RArchPlugin r_arch_plugin_arm;
extern RArchPlugin r_arch_plugin_x86_nz;
extern RArchPlugin r_arch_plugin_x86_nasm;

#ifdef __cplusplus
}
#endif

#endif
