#ifndef MASIK_UTILS_SRC_TREE_FUNCS_FUNCS_H
#define MASIK_UTILS_SRC_TREE_FUNCS_FUNCS_H

#include "utils/src/tree/structs.h"
#include "utils/src/tree/verification/verification.h"

const char* lexem_type_to_str(const enum LexemType type);

tree_elem_t*   tree_elem_ctor(lexem_t lexem, tree_elem_t* lt, tree_elem_t* rt);
void           tree_elem_dtor          (tree_elem_t** elem);
void           tree_elem_dtor_recursive(tree_elem_t** elem);

enum TreeError tree_ctor(tree_t* tree, const char* const filename);
void           tree_dtor(tree_t* const tree);

enum TreeError tree_print(const tree_t tree, FILE* out);

#endif /* MASIK_UTILS_SRC_TREE_FUNCS_FUNCS_H */