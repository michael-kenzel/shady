#ifndef SHADY_EMIT_C
#define SHADY_EMIT_C

#include "shady/ir.h"
#include "growy.h"
#include "arena.h"
#include "printer.h"

typedef enum {
    C,
    GLSL
} Dialect;

typedef struct {
    CompilerConfig* config;
    Dialect dialect;
    bool explicitly_sized_types;
} EmitterConfig;

typedef struct {
    EmitterConfig config;
    IrArena* arena;
    unsigned next_id;
    Printer *type_decls, *fn_decls, *fn_defs;
    struct Dict* emitted;
} Emitter;

#define emit_type c_emit_type
#define emit_value c_emit_value

String emit_type(Emitter* emitter, const Type* type, const char* identifier);
String emit_value(Emitter* emitter, const Node* value);

#endif
