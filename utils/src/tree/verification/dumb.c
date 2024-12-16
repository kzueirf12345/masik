#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "tree/verification/dumb.h"
#include "logger/liblogger.h"
#include "utils/utils.h"


static const char* const HTML_INTRO_ =
    "\n<!DOCTYPE html>\n"
    "<html lang='en'>\n"
        "<head>\n"
            "<meta charset='UTF-8'>\n"
            // "<meta http-equiv='X-UA-Compatible' content='IE=edge'>\n"
            // "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
            "<title>MEGA MEGA MEGA DUMB</title>\n"
        "</head>\n"
        "<body>\n"
            "<pre>\n";


#define CASE_ENUM_TO_STRING_(error) case error: return #error
const char* tree_dumb_strerror(const enum TreeDumbError error)
{
    switch (error)
    {
        CASE_ENUM_TO_STRING_(SYNTAXER_DUMB_ERROR_SUCCESS);
        CASE_ENUM_TO_STRING_(SYNTAXER_DUMB_ERROR_FAILURE);
    default:
        return "UNKNOWN_SYNTAXER_DUMB_ERROR";
    }
    return "UNKNOWN_SYNTAXER_DUMB_ERROR";
}
#undef CASE_ENUM_TO_STRING_


static struct 
{
    char* out_name;

    char* html_name;
    char* dot_name;
    char* svg_name;
    char* graph_count_name;

    FILE* dot_file;
    FILE* html_file;

    size_t graph_count;
} DUMBER_ = {};

static void DUMBER_is_init_lasserts_(void)
{
    lassert(DUMBER_.html_name,          "DUMBER_ is not init");
    lassert(DUMBER_.html_file,          "DUMBER_ is not init");
    lassert(DUMBER_.dot_name,           "DUMBER_ is not init");
    lassert(DUMBER_.dot_file,           "DUMBER_ is not init");
    lassert(DUMBER_.svg_name,           "DUMBER_ is not init");
    lassert(DUMBER_.graph_count_name,   "DUMBER_ is not init");
}


enum TreeDumbError tree_dumb_ctor(void)
{
    lassert(!DUMBER_.html_name         || !DUMBER_.html_file, "");
    lassert(!DUMBER_.dot_name          || !DUMBER_.dot_file,  "");
    lassert(!DUMBER_.svg_name,                                "");
    lassert(!DUMBER_.graph_count_name,                        "");
    lassert(!DUMBER_.out_name,                                "");

    TREE_DUMB_ERROR_HANDLE(tree_dumb_set_out_file("./log/dumb"));

    return SYNTAXER_DUMB_ERROR_SUCCESS;
}

static enum TreeDumbError set_graph_count_          (void);
static enum TreeDumbError write_graph_count_in_file_(void);

enum TreeDumbError tree_dumb_dtor(void)
{
    DUMBER_is_init_lasserts_();

    TREE_DUMB_ERROR_HANDLE(write_graph_count_in_file_());

    if (fclose(DUMBER_.html_file))
    {
        perror("Can't close html_file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }
    IF_DEBUG(DUMBER_.html_file = NULL;)

    if (fclose(DUMBER_.dot_file))
    {
        perror("Can't close dot_file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }
    IF_DEBUG(DUMBER_.dot_file = NULL;)

    free(DUMBER_.out_name);         IF_DEBUG(DUMBER_.out_name           = NULL;)
    free(DUMBER_.html_name);        IF_DEBUG(DUMBER_.html_name          = NULL;)
    free(DUMBER_.dot_name);         IF_DEBUG(DUMBER_.dot_name           = NULL;)
    free(DUMBER_.svg_name);         IF_DEBUG(DUMBER_.svg_name           = NULL;)
    free(DUMBER_.graph_count_name); IF_DEBUG(DUMBER_.graph_count_name   = NULL;)

    return SYNTAXER_DUMB_ERROR_SUCCESS;
}

//==========================================================================================

static bool is_set_graph_count_ = false;
static enum TreeDumbError set_graph_count_(void) //NOTE - non assertable
{
    is_set_graph_count_ = true;

    if (access(DUMBER_.graph_count_name, F_OK))
    {
        errno = 0;
        DUMBER_.graph_count = 0;
        return SYNTAXER_DUMB_ERROR_SUCCESS;
    }

    FILE* const graph_count_file = fopen(DUMBER_.graph_count_name, "rb");
    if (!graph_count_file)
    {
        perror("Can't open graph_count_file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    if (fscanf(graph_count_file, "%zu", &DUMBER_.graph_count) <= 0)
    {
        perror("Can't fscanf graph_count");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    if (fclose(graph_count_file))
    {
        perror("Can't close graph_count_file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    return SYNTAXER_DUMB_ERROR_SUCCESS;
}

static enum TreeDumbError write_graph_count_in_file_(void)
{
    DUMBER_is_init_lasserts_();

    FILE* const graph_count_file = fopen(DUMBER_.graph_count_name, "wb");
    if (!graph_count_file)
    {
        perror("Can't open graph_count_file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    if (fprintf(graph_count_file, "%zu", DUMBER_.graph_count) <= 0)
    {
        perror("Can't fprintf graph_count");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    if (fclose(graph_count_file))
    {
        perror("Can't close graph_count_file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    return SYNTAXER_DUMB_ERROR_SUCCESS;
}

//==========================================================================================

static enum TreeDumbError 
tree_dumb_set_out_file_(char* const filename, FILE** const file, char** const old_filename, 
                            const char* const mode, const char* const file_extension);

static enum TreeDumbError 
tree_dumb_set_out_filename_(char*  const filename, const char* const file_extension,
                                char** const old_filename);

enum TreeDumbError tree_dumb_set_out_file(char* const filename)
{
    lassert(filename, "");

    TREE_DUMB_ERROR_HANDLE(
        tree_dumb_set_out_file_(filename, &DUMBER_.html_file, &DUMBER_.html_name, "ab", ".html")
    );
    TREE_DUMB_ERROR_HANDLE(
        tree_dumb_set_out_file_(filename, &DUMBER_.dot_file,  &DUMBER_.dot_name,  "wb", ".dot")
    );

    TREE_DUMB_ERROR_HANDLE(
        tree_dumb_set_out_filename_(filename, "",     &DUMBER_.out_name)
    );
    TREE_DUMB_ERROR_HANDLE(
        tree_dumb_set_out_filename_(filename, ".svg", &DUMBER_.svg_name)
    );
    TREE_DUMB_ERROR_HANDLE(
        tree_dumb_set_out_filename_(filename, "_graph_count.txt", &DUMBER_.graph_count_name)
    );

    TREE_DUMB_ERROR_HANDLE(set_graph_count_());

    return SYNTAXER_DUMB_ERROR_SUCCESS;
}

static enum TreeDumbError
tree_dumb_set_out_file_(char* const filename, FILE** const file, char** const old_filename, 
                            const char* const mode, const char* const file_extension)
{
    lassert(filename, "");
    lassert(file, "");
    lassert(old_filename, "");
    lassert(file_extension, "");

    TREE_DUMB_ERROR_HANDLE(
        tree_dumb_set_out_filename_(filename, file_extension, old_filename)
    );

    if (*file && fclose(*file))
    {  
        perror("Can't close file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }
    
    if (!(*file = fopen(*old_filename, mode))){
        perror("Can't open file");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }
    
    return SYNTAXER_DUMB_ERROR_SUCCESS;
}


static enum TreeDumbError
tree_dumb_set_out_filename_(char*  const filename, const char* const file_extension,
                                char** const old_filename)
{
    lassert(filename, "");
    lassert(file_extension, "");

    free(*old_filename);

    *old_filename = calloc(FILENAME_MAX, sizeof(char));

    if (!*old_filename)
    {
        perror("Can't calloc old_filename");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    if (snprintf(*old_filename, FILENAME_MAX, "%s%s", filename, file_extension) <= 0)
    {
        perror("Can't snprintf old_filename");
        return SYNTAXER_DUMB_ERROR_FAILURE;
    }

    return SYNTAXER_DUMB_ERROR_SUCCESS;
}

//==========================================================================================

static const char* handle_invalid_ptr_(const void* const check_ptr);

static int create_tree_dot_(const tree_t* const syntaxer);
static int create_tree_svg_(void);
static int insert_tree_svg_(void);

#define DUMB_AND_FPRINTF_(format, ...)                                                              \
        do {                                                                                        \
            fprintf(DUMBER_.html_file, format, ##__VA_ARGS__);                                      \
            fprintf(stderr,       format, ##__VA_ARGS__);                                           \
        } while(0)

void tree_dumb (const tree_t* const syntaxer)
{
    if (is_empty_file(DUMBER_.html_file) == 0) fprintf(DUMBER_.html_file, HTML_INTRO_);

    fprintf(DUMBER_.html_file, "</pre><hr /><pre>\n");   

    DUMB_AND_FPRINTF_("\n==TREE DUMB==\nDate: %s\nTime: %s\n\n", __DATE__, __TIME__);

    const char* syntaxer_buf = handle_invalid_ptr_(syntaxer);

    if (syntaxer_buf)
    {
        DUMB_AND_FPRINTF_("syntaxer[%s]\n", syntaxer_buf);
        return;
    }
    DUMB_AND_FPRINTF_("syntaxer[%p]\n\n", syntaxer);

    DUMB_AND_FPRINTF_("syntaxer->size = %zu\n", syntaxer->size);


    if (!is_set_graph_count_ && set_graph_count_())
    {
        fprintf(stderr, "Can't set graph_count_\n");
        return;
    }

    if (create_tree_dot_(syntaxer))
    {
        DUMB_AND_FPRINTF_("Can't create syntaxer dot\n");
        return;
    }

    if (create_tree_svg_())
    {
        DUMB_AND_FPRINTF_("Can't create syntaxer svg\n");
        return;
    }

    if (insert_tree_svg_())
    {
        DUMB_AND_FPRINTF_("Can't insert syntaxer svg\n");
        return;
    }

    ++DUMBER_.graph_count;

    fprintf(DUMBER_.html_file, "</pre><hr /><pre>\n");
}
#undef DUMB_AND_FPRINTF_

static const char* handle_invalid_ptr_(const void* const check_ptr)
{
    switch (is_invalid_ptr(check_ptr))
    {
    case PTR_STATES_VALID:
        return NULL;
    case PTR_STATES_NULL:
        return "NULL";
    case PTR_STATES_INVALID:
        return "INVALID";
    case PTR_STATES_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }

    return "MIPT SHIT";
}

int create_tree_dot_recursive_(const tree_elem_t* const elem, const size_t size, 
                                   size_t* const cur_size);

int create_tree_dot_(const tree_t* const syntaxer)
{
    if (is_invalid_ptr(syntaxer)) return -1;

    fprintf(DUMBER_.dot_file, "digraph {\n"
                              "rankdir=TB;\n"
                              "node[style = filled]\n");

    size_t size = 0;

    if (create_tree_dot_recursive_(syntaxer->Groot, syntaxer->size, &size))
    {
        fprintf(stderr, "Can't create syntaxer dot recursive\n");
        return -1;
    }

    fprintf(DUMBER_.dot_file, "}\n");

    return 0;
}

int dot_print_node_(const tree_elem_t* const elem);

int create_tree_dot_recursive_(const tree_elem_t* const elem, const size_t size, 
                                   size_t* const cur_size)
{
    if (*cur_size >= size)      return  0;
    if (is_invalid_ptr(elem))   return -1;

    ++*cur_size;

    dot_print_node_(elem);

    const tree_elem_t* const parent_ptr = elem;

    if (elem->lt)
    {
        fprintf(DUMBER_.dot_file, "node%p -> node%p [color=red]\n",  parent_ptr, elem->lt);
        if (create_tree_dot_recursive_(elem->lt, size, cur_size))
            return -1;
    }

    if (elem->rt)
    {
        fprintf(DUMBER_.dot_file, "node%p -> node%p [color=green]\n",parent_ptr, elem->rt);
        if (create_tree_dot_recursive_(elem->rt, size, cur_size))
            return -1;
    }

    return 0;
}

int dot_print_node_(const tree_elem_t* const elem)
{
    if (is_invalid_ptr(elem)) return -1;

    switch (elem->lexem.type)
    {
    case LEXEM_TYPE_NUM:
        fprintf(DUMBER_.dot_file, 
                "node%p [shape=Mrecord; label = \"{{%p|%s}|%ld}\"; fillcolor = pink];\n",
                elem, elem, lexem_type_to_str(elem->lexem.type), elem->lexem.data.num);
        break;
    case LEXEM_TYPE_OP:
        fprintf(DUMBER_.dot_file, 
                "node%p [shape=Mrecord; label = \"{{%p|%s}|{%ls|%s}}\"; fillcolor = peachpuff];\n",
                elem, elem, lexem_type_to_str(elem->lexem.type), 
                OPERATIONS[elem->lexem.data.op].keyword, op_type_to_str(elem->lexem.data.op));
        break;
    case LEXEM_TYPE_VAR:
        fprintf(DUMBER_.dot_file, 
                "node%p [shape=Mrecord; label = \"{{%p|%s}|{%ls|%ld}}\"; fillcolor = peachpuff];\n",
                elem, elem, lexem_type_to_str(elem->lexem.type), 
                elem->lexem.data.var.name, elem->lexem.data.var.val);
        break;
    case LEXEM_TYPE_END:
        fprintf(DUMBER_.dot_file, 
                "node%p [shape=Mrecord; label = \"{{%p|%s}|%s}\"; fillcolor = lightyellow];\n",
                elem, elem, lexem_type_to_str(elem->lexem.type), "END");
        break;
    default:
        return -1;
    }

    return 0;
}

int create_tree_svg_(void)
{
    if (DUMBER_.dot_file && fclose(DUMBER_.dot_file))
    {
        perror("Can't fclose dot file");
        return -1;
    }
    DUMBER_.dot_file = NULL;

    static const size_t CREATE_SVG_CMD_SIZE = 256;
    char* create_svg_cmd = calloc(CREATE_SVG_CMD_SIZE, sizeof(char));

    if (!create_svg_cmd)
    {
        fprintf(stderr, "Can't calloc create_svg_cmd\n");
        return -1;
    }

    if (snprintf(create_svg_cmd, CREATE_SVG_CMD_SIZE, 
                 "dot -Tsvg %s -o %s%zu.svg >/dev/null", 
                 DUMBER_.dot_name, DUMBER_.out_name, DUMBER_.graph_count) <= 0)
    {
        fprintf(stderr, "Can't snprintf creare_svg_cmd\n");
        free(create_svg_cmd); create_svg_cmd = NULL;
        return -1;
    }
    
    if (system(create_svg_cmd))
    {
        fprintf(stderr, "Can't call system create svg\n");
        free(create_svg_cmd); create_svg_cmd = NULL;
        return -1;
    }

    free(create_svg_cmd); create_svg_cmd = NULL;

    if (!(DUMBER_.dot_file = fopen(DUMBER_.dot_name, "wb")))
    {
        perror("Can't fopen dot file");
        return -1;
    }

    return 0;
}

int insert_tree_svg_(void)
{
    const char* filename_without_path = DUMBER_.out_name;
    while (strchr(filename_without_path, '/') != NULL)
    {
        filename_without_path = strchr(filename_without_path, '/') + 1;
    }

    fprintf(DUMBER_.html_file, "<img src=%s%zu.svg>\n", 
            filename_without_path, DUMBER_.graph_count);

    return 0;
}