#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"

enum
{
    TYPEDEF_TYPE_STANDARD,
    // A structure typedef is basically "typedef struct ABC { int x; } AAA;"
    TYPEDEF_TYPE_STRUCTURE_TYPEDEF
};

struct typedef_type
{
    int type;
    const char* definiton_name;
    struct vector* value;
    struct typedef_structure
    {
        const char* sname;
    } structure;
};

enum
{
    PREPROCESSOR_FLAG_EVALUATE_NODE = 0b00000001
};

enum
{
    PREPROCESSOR_NUMBER_NODE,
    PREPROCESSOR_IDENTIFIER_NODE,
    PREPROCESSOR_KEYWORD_NODE,
    PREPROCESSOR_UNARY_NODE,
    PREPROCESSOR_EXPRESSION_NODE,
    PREPROCESSOR_PARENTHESES_NODE,
    PREPROCESSOR_JOINED_NODE,
    PREPROCESSOR_TENARY_NODE
};


struct preprocessor_node
{
    int type;
    struct preprocessor_const_val
    {
        union 
        {
            char cval;
            unsigned int inum;
            long lnum;
            long long llnum;
            unsigned long ulnum;
            unsigned long long ullnum;
        };
        
    } const_val;

    union 
    {
        struct preprocessor_exp_node
        {
            struct preprocessor_node* left;
            struct preprocessor_node* right;
            const char* op;
        } exp;

        struct preprocessor_unary_node
        {
            struct preprocessor_node* operand_node;
            const char* op;
            struct preprocessor_unary_indirection
            {
                int depth;
            } indirection;
        } unary_node;

        struct preprocessor_parenthesis
        {
            struct preprocessor_node* exp;
        } parenthesis;


        struct preprocessor_joined_node
        {
            struct preprocessor_node* left;
            struct preprocessor_node* right;
        } joined;

        struct preprocessor_tenary_node
        {
            struct preprocessor_node* true_node;
            struct preprocessor_node* false_node;
        } tenary;
    };

    const char* sval;
    
};

void preprocessor_handle_token(struct compile_process* compiler, struct token* token);
int preprocessor_parse_evaluate(struct compile_process* compiler, struct vector* token_vec);
int preprocessor_evaluate(struct compile_process* compiler, struct preprocessor_node* root_node);

void preprocessor_execute_warning(struct compile_process* compiler, const char* msg)
{
    compiler_warning(compiler, "#warning %s", msg);
}

void preprocessor_execute_error(struct compile_process* compiler, const char* msg)
{
    compiler_error(compiler, "#error %s", msg);
}

struct preprocessor_included_file* preprocessor_add_included_file(struct preprocessor* preprocessor, const char* filename)
{
    struct preprocessor_included_file* included_file = calloc(1, sizeof(struct preprocessor_included_file));
    strncpy(included_file->filename, filename, sizeof(included_file->filename));
    vector_push(preprocessor->includes, &included_file);
    return included_file;
}

void preprocessor_create_static_include(struct preprocessor* preprocessor, const char* filename, PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION creation_handler)
{
    struct preprocessor_included_file* included_file = preprocessor_add_included_file(preprocessor, filename);
    creation_handler(preprocessor, included_file);
}


bool preprocessor_is_keyword(const char* type)
{
    return S_EQ(type, "defined");
}

struct vector* preprocessor_build_value_vector_for_integer(int value)
{
    struct vector* token_vec = vector_create(sizeof(struct token));
    struct token t1 = {};
    t1.type = TOKEN_TYPE_NUMBER;
    t1.llnum = value;
    vector_push(token_vec, &t1);
    return token_vec;
}

void preprocessor_token_vec_push_keyword_and_identifier(struct vector* token_vec, const char* keyword, const char* identifier)
{
    struct token t1 = {};
    t1.type = TOKEN_TYPE_KEYWORD;
    t1.sval = keyword;
    struct token t2 = {};
    t2.type = TOKEN_TYPE_IDENTIFIER;
    t2.sval = identifier;
    vector_push(token_vec, &t1);
    vector_push(token_vec, &t2);
}

void* preprocessor_node_create(struct preprocessor_node* node)
{
    struct preprocessor_node* result = calloc(1, sizeof(struct preprocessor_node));
    memcpy(result, node, sizeof(struct preprocessor_node));
    return result;
}

int preprocessor_definition_argument_exists(struct preprocessor_definition* definition, const char* name)
{
    vector_set_peek_pointer(definition->standard.arguments, 0);
    int i = 0;
    const char* current = vector_peek(definition->standard.arguments);
    while(current)
    {
        if (S_EQ(current, name))
            return i;
        
        i++;
        current = vector_peek(definition->standard.arguments);
    }

    return -1;
}

struct preprocessor_function_argument* preprocessor_function_argument_at(struct preprocessor_function_arguments* arguments, int index)
{
    struct preprocessor_function_argument* argument = vector_at(arguments->arguments, index);
    return argument;
}

void preprocessor_token_push_to_function_arguments(struct preprocessor_function_arguments* arguments, struct token* token)
{
    struct preprocessor_function_argument arg = {};
    arg.tokens = vector_create(sizeof(struct token));
    vector_push(arg.tokens, token);
    vector_push(arguments->arguments, &arg);
}

void preprocessor_function_argument_push_to_vec(struct preprocessor_function_argument* argument, struct vector* vector_out)
{
    vector_set_peek_pointer(argument->tokens, 0);
    struct token* token = vector_peek(argument->tokens);
    while(token)
    {
        vector_push(vector_out, token);
        token = vector_peek(argument->tokens);
    }

}

void preprocessor_token_push_to_dst(struct vector* token_vec, struct token* token)
{
    struct token t = *token;
    vector_push(token_vec, &t);
}

void preprocessor_token_push_dst(struct compile_process* compiler, struct token* token)
{
    preprocessor_token_push_to_dst(compiler->token_vec, token);
}

void preprocessor_token_vec_push_src_to_dst(struct compile_process* compiler, struct vector* src_vec, struct vector* dst_vec)
{
    vector_set_peek_pointer(src_vec, 0);
    struct token* token = vector_peek(src_vec);
    while(token)
    {
        vector_push(dst_vec, token);
        token= vector_peek(src_vec);
    }
}

void preprocessor_token_vec_push_src(struct compile_process* compiler, struct vector* src_vec)
{
    preprocessor_token_vec_push_src_to_dst(compiler, src_vec, compiler->token_vec);
}

void preprocessor_token_vec_push_src_token(struct compile_process* compiler, struct token* token)
{
    vector_push(compiler->token_vec, token);
}

void preprocessor_initialize(struct preprocessor* preprocessor)
{
    memset(preprocessor, 0, sizeof(struct preprocessor));
    preprocessor->definitions = vector_create(sizeof(struct preprocessor_definition*));
    preprocessor->includes = vector_create(sizeof(struct preprocessor_included_file*));
    #warning "Create preprocessor default definitions"
}

struct preprocessor* preprocessor_create(struct compile_process* compiler)
{
    struct preprocessor* preprocessor = calloc(1, sizeof(struct preprocessor));
    preprocessor_initialize(preprocessor);
    preprocessor->compiler = compiler;
    return preprocessor;

}
struct token* preprocessor_previous_token(struct compile_process* compiler)
{
    return vector_peek_at(compiler->token_vec_original, compiler->token_vec_original->pindex-1);
}

struct token* preprocessor_next_token(struct compile_process* compiler)
{
    return vector_peek(compiler->token_vec_original);
}

struct token* preprocessor_next_token_no_increment(struct compile_process* compiler)
{
    return vector_peek_no_increment(compiler->token_vec_original);
}

struct token* preprocessor_peek_next_token_skip_nl(struct compile_process* compiler)
{
    struct token* token = preprocessor_next_token_no_increment(compiler);
    while(token && token->type == TOKEN_TYPE_NEWLINE)
    {
        token = preprocessor_next_token(compiler);
    }
    token = preprocessor_next_token_no_increment(compiler);
    return token;
}

void* preprocessor_handle_number_token(struct expressionable* expressionable)
{
    struct token* token = expressionable_token_next(expressionable);
    return preprocessor_node_create(&(struct preprocessor_node){.type=PREPROCESSOR_NUMBER_NODE,.const_val.llnum=token->llnum});
}

void* preprocessor_handle_identifier_token(struct expressionable* expressionable)
{
    struct token* token = expressionable_token_next(expressionable);
    bool is_preprocessor_keyword = preprocessor_is_keyword(token->sval);
    int type = PREPROCESSOR_IDENTIFIER_NODE;
    if (is_preprocessor_keyword)
    {
        type = PREPROCESSOR_KEYWORD_NODE;
    }
    return preprocessor_node_create(&(struct preprocessor_node){.type=type, .sval=token->sval});
}

void preprocessor_make_unary_node(struct expressionable* expressionable, const char* op, void* right_operand_node_ptr)
{
    struct preprocessor_node* right_operand_node = right_operand_node_ptr;
    void* unary_node = preprocessor_node_create(&(struct preprocessor_node){.type=PREPROCESSOR_UNARY_NODE,.unary_node.op=op,.unary_node.operand_node=right_operand_node});
    expressionable_node_push(expressionable, unary_node);
}

void preprocessor_make_expression_node(struct expressionable* expressionable, void* left_node_ptr, void* right_node_ptr, const char* op)
{
    struct preprocessor_node exp_node;
    exp_node.type = PREPROCESSOR_EXPRESSION_NODE;
    exp_node.exp.left = left_node_ptr;
    exp_node.exp.right = right_node_ptr;
    exp_node.exp.op = op;
    expressionable_node_push(expressionable, preprocessor_node_create(&exp_node));
}

void preprocessor_make_parentheses_node(struct expressionable* expressionable, void* node_ptr)
{
    struct preprocessor_node* node = node_ptr;
    struct preprocessor_node parentheses_node;
    parentheses_node.type = PREPROCESSOR_PARENTHESES_NODE;
    parentheses_node.parenthesis.exp = node_ptr;
    expressionable_node_push(expressionable, preprocessor_node_create(&parentheses_node));
}
void preprocessor_make_tenary_node(struct expressionable* expressionable, void* true_result_node_ptr, void* false_result_node_ptr)
{
    struct preprocessor_node* true_result_node = true_result_node_ptr;
    struct preprocessor_node* false_result_node = false_result_node_ptr;

    expressionable_node_push(expressionable, preprocessor_node_create(&(struct preprocessor_node){.type=PREPROCESSOR_TENARY_NODE,.tenary.true_node=true_result_node,.tenary.false_node=false_result_node}));
}

int preprocessor_get_node_type(struct expressionable* expressionable, void* node)
{
    int generic_type = EXPRESSIONABLE_GENERIC_TYPE_NON_GENERIC;
    struct preprocessor_node* preprocessor_node = node;
    switch(preprocessor_node->type)
    {
        case PREPROCESSOR_NUMBER_NODE:
            generic_type = EXPRESSIONABLE_GENERIC_TYPE_NUMBER;
            break;
        
        case PREPROCESSOR_IDENTIFIER_NODE:
        case PREPROCESSOR_KEYWORD_NODE:
            generic_type = EXPRESSIONABLE_GENERIC_TYPE_IDENTIFIER;
        break;

        case PREPROCESSOR_UNARY_NODE:
            generic_type = EXPRESSIONABLE_GENERIC_TYPE_UNARY;
            break;
        case PREPROCESSOR_EXPRESSION_NODE:
            generic_type = EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION;
            break;  

        case PREPROCESSOR_PARENTHESES_NODE:
            generic_type = EXPRESSIONABLE_GENERIC_TYPE_PARENTHESES;
            break;
    }

    return generic_type;
}

void* preprocessor_get_left_node(struct expressionable* expressionable, void* target_node)
{
    struct preprocessor_node* node = target_node;
    return node->exp.left;
}

void* preprocessor_get_right_node(struct expressionable* expressionable, void* target_node)
{
    struct preprocessor_node* node = target_node;
    return node->exp.right;
}

const char* preprocessor_get_node_operator(struct expressionable* expressionable, void* target_node)
{
    struct preprocessor_node* preprocessor_node = target_node;
    return preprocessor_node->exp.op;
}

void** preprocessor_get_left_node_address(struct expressionable* expressionable, void* target_node)
{
    return (void**)&((struct preprocessor_node*)(target_node))->exp.left;
}

void** preprocessor_get_right_node_address(struct expressionable* expressionable, void* target_node)
{
    return (void**)&((struct preprocessor_node*)(target_node))->exp.right;
}


void preprocessor_set_expression_node(struct expressionable* expressionable, void* node, void* left_node, void* right_node, const char* op)
{
    struct preprocessor_node* preprocessor_node = node;
    preprocessor_node->exp.left = left_node;
    preprocessor_node->exp.right = right_node;
    preprocessor_node->exp.op = op;
}

bool preprocessor_should_join_nodes(struct expressionable* expressionable, void* previous_node_ptr, void* node_ptr)
{
    return true;
}

void* preprocessor_join_nodes(struct expressionable* expressionable, void* previous_node_ptr, void* node_ptr)
{
    struct preprocessor_node* previous_node = previous_node_ptr;
    struct preprocessor_node* node = node_ptr;
    return preprocessor_node_create(&(struct preprocessor_node){.type=PREPROCESSOR_JOINED_NODE,.joined.left=previous_node,.joined.right=node});
}

bool preprocessor_expecting_additional_node(struct expressionable* expressionable, void* node_ptr)
{
    struct preprocessor_node* node = node_ptr;
    return node->type == PREPROCESSOR_KEYWORD_NODE && S_EQ(node->sval, "defined");
}

bool preprocessor_is_custom_operator(struct expressionable* expressionable, struct token* token)
{
    return false;
}


struct expressionable_config preprocessor_expressionable_config = 
{
    .callbacks.handle_number_callback = preprocessor_handle_number_token,
    .callbacks.handle_identifier_callback = preprocessor_handle_identifier_token,
    .callbacks.make_unary_node = preprocessor_make_unary_node,
    .callbacks.make_expression_node = preprocessor_make_expression_node,
    .callbacks.make_parentheses_node = preprocessor_make_parentheses_node,
    .callbacks.make_tenary_node = preprocessor_make_tenary_node,
    .callbacks.get_node_type = preprocessor_get_node_type,
    .callbacks.get_left_node = preprocessor_get_left_node,
    .callbacks.get_right_node = preprocessor_get_right_node,
    .callbacks.get_node_operator = preprocessor_get_node_operator,
    .callbacks.get_left_node_address = preprocessor_get_left_node_address,
    .callbacks.get_right_node_address = preprocessor_get_right_node_address,
    .callbacks.set_exp_node = preprocessor_set_expression_node,
    .callbacks.should_join_nodes = preprocessor_should_join_nodes,
    .callbacks.join_nodes=preprocessor_join_nodes,
    .callbacks.expecting_additional_node = preprocessor_expecting_additional_node,
    .callbacks.is_custom_operator = preprocessor_is_custom_operator,
};

bool preprocessor_is_preprocessor_keyword(const char* value)
{
    return S_EQ(value, "define") ||
            S_EQ(value, "undef") ||
            S_EQ(value, "warning") ||
            S_EQ(value, "error") ||
            S_EQ(value, "if") ||
            S_EQ(value, "eleif") ||
            S_EQ(value, "ifdef") ||
            S_EQ(value, "ifndef") ||
            S_EQ(value, "endif") ||
            S_EQ(value, "include") ||
            S_EQ(value, "typedef");

}
bool preprocessor_token_is_preprocessor_keyword(struct token* token)
{
    return token->type == TOKEN_TYPE_IDENTIFIER || token->type == TOKEN_TYPE_KEYWORD && preprocessor_is_preprocessor_keyword(token->sval);
}
bool preprocessor_token_is_define(struct token* token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "define"));
}
bool preprocessor_token_is_undef(struct token* token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "undef"));
}

bool preprocessor_token_is_warning(struct token* token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "warning"));
}
bool preprocessor_token_is_error(struct token* token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "error"));
}

bool preprocessor_token_is_if(struct token* token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "if"));
}

bool preprocessor_token_is_ifdef(struct token* token)
{
    if(!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "ifdef"));
}

bool preprocessor_token_is_ifndef(struct token* token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "ifndef"));
}

struct buffer* preprocessor_multi_value_string(struct compile_process* compiler)
{
    struct buffer* buffer = buffer_create();
    struct token* value_token = preprocessor_next_token(compiler);
    while(value_token)
    {
        if (value_token->type == TOKEN_TYPE_NEWLINE)
        {
            break;
        }

        if (token_is_symbol(value_token, '\\'))
        {
            // Skip the new line
            preprocessor_next_token(compiler);
            value_token = preprocessor_next_token(compiler);
            continue;
        }

        buffer_printf(buffer, "%s", value_token->sval);
        value_token = preprocessor_next_token(compiler);
    }

    return buffer;
}

void preprocessor_multi_value_insert_to_vector(struct compile_process* compiler, struct vector* value_token_vec)
{
    struct token* value_token = preprocessor_next_token(compiler);
    while(value_token)
    {
        if (value_token->type == TOKEN_TYPE_NEWLINE)
        {
            break;
        }

        if (token_is_symbol(value_token, '\\'))
        {
            // This allows for another line skip the new line
            preprocessor_next_token(compiler);
            value_token = preprocessor_next_token(compiler);
            continue;
        }

        vector_push(value_token_vec, value_token);
        value_token = preprocessor_next_token(compiler);
    }
}

void preprocessor_definition_remove(struct preprocessor* preprocessor, const char* name)
{
    vector_set_peek_pointer(preprocessor->definitions, 0);
    struct preprocessor_definition* current_definition = vector_peek_ptr(preprocessor->definitions);
    while(current_definition)
    {
        if(S_EQ(current_definition->name, name))
        {
            // Remove the definition
            vector_pop_last_peek(preprocessor->definitions);
        }
        current_definition = vector_peek_ptr(preprocessor->definitions);
    }
}
struct preprocessor_definition* preprocessor_definition_create(const char* name, struct vector* value_vec, struct vector* arguments, struct preprocessor* preprocessor)
{
    // Unset the definition if its already created
    preprocessor_definition_remove(preprocessor, name);

    struct preprocessor_definition* definition= calloc(1, sizeof(struct preprocessor_definition));
    definition->type = PREPROCESSOR_DEFINITION_STANDARD;
    definition->name= name;
    definition->standard.value = value_vec;
    definition->standard.arguments = arguments;
    definition->preprocessor = preprocessor;

    if (arguments && vector_count(definition->standard.arguments))
    {
        definition->type  = PREPROCESSOR_DEFINITION_MACRO_FUNCTION;
    }

    vector_push(preprocessor->definitions, &definition);
    return definition;
}

struct preprocessor_definition* preprocessor_get_definition(struct preprocessor* preprocessor, const char* name)
{
    vector_set_peek_pointer(preprocessor->definitions, 0);
    struct preprocessor_definition* definition = vector_peek_ptr(preprocessor->definitions);
    while(definition)
    {
        if(S_EQ(definition->name, name))
        {
            break;
        }

        definition = vector_peek_ptr(preprocessor->definitions);
    }

    return definition;
}


struct vector* preprocessor_definition_value_for_standard(struct preprocessor_definition* definition)
{
    return definition->standard.value;
}

struct vector* preprocessor_definition_value_with_arguments(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments)
{
    if (definition->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK)
    {
        #warning "implement definition value for native.
        return NULL;
    }
    else if(definition->type == PREPROCESSOR_DEFINITION_TYPEDEF)
    {
        #warning "preprocessor definition typedef"
        return NULL;
    }

    return preprocessor_definition_value_for_standard(definition);

}
struct vector* preprocessor_definition_value(struct preprocessor_definition* definition)
{
    return preprocessor_definition_value_with_arguments(definition, NULL);
}

int preprocessor_parse_evaluate_token(struct compile_process* compiler, struct token* token)
{
    struct vector* token_vec = vector_create(sizeof(struct token));
    vector_push(token_vec, token);
    return preprocessor_parse_evaluate(compiler, token_vec);
}

int preprocessor_definition_evaluated_value_for_standard(struct preprocessor_definition* definition)
{
    struct token* token = vector_back(definition->standard.value);
    if (token->type == TOKEN_TYPE_IDENTIFIER)
    {
        return preprocessor_parse_evaluate_token(definition->preprocessor->compiler, token);
    }

    if (token->type != TOKEN_TYPE_NUMBER)
    {
        compiler_error(definition->preprocessor->compiler, "The definition must hold a number value. Unable to use macro IF");
    }
    return token->llnum;
}

int preprocessor_definition_evaluated_value(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments)
{
    if (definition->type == PREPROCESSOR_DEFINITION_STANDARD)
    {
        return preprocessor_definition_evaluated_value_for_standard(definition);
    }
    else if(definition->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK)
    {
        #warning "implement native callbacks.
        return -1;
    }
    
    compiler_error(definition->preprocessor->compiler, "The definition cannot be evaluated into a number");

}
bool preprocessor_is_next_macro_arguments(struct compile_process* compiler)
{
    bool res = false;
    vector_save(compiler->token_vec_original);
    struct token* last_token = preprocessor_previous_token(compiler);
    struct token* current_token = preprocessor_next_token(compiler);

    if (token_is_operator(current_token, "(") && (!last_token || !last_token->whitespace))
    {
        res = true;
    }

    vector_restore(compiler->token_vec_original);
    return res;
}

void preprocessor_parse_macro_argument_declaration(struct compile_process* compiler, struct vector* arguments)
{
    if (token_is_operator(preprocessor_next_token_no_increment(compiler), "("))
    {
        // Skip the (
        preprocessor_next_token(compiler);
        struct token*  next_token = preprocessor_next_token(compiler);
        while(!token_is_symbol(next_token, ')'))
        {
            if (next_token->type != TOKEN_TYPE_IDENTIFIER)
            {
                compiler_error(compiler, "You must provide an identifier in the preprocessor definition!");
            }

            vector_push(arguments, (void*)next_token->sval);
            next_token = preprocessor_next_token(compiler);
            if (!token_is_operator(next_token, ",") && !token_is_symbol(next_token, ')'))
            {
                compiler_error(compiler, "Incomplete sequence for macro arguments");
            }

            if (token_is_symbol(next_token, ')'))
            {
                break;
            }

            // Skip the "," operator
            next_token = preprocessor_next_token(compiler);
        }
    }
}
void preprocessor_handle_definition_token(struct compile_process* compiler)
{
    struct token* name_token = preprocessor_next_token(compiler);
    struct vector* arguments = vector_create(sizeof(const char*));

    if (preprocessor_is_next_macro_arguments(compiler))
    {
        preprocessor_parse_macro_argument_declaration(compiler, arguments);
    }
    // Value can be composed of many tokens
    struct vector* value_token_vec = vector_create(sizeof(struct token));
    preprocessor_multi_value_insert_to_vector(compiler, value_token_vec);

    struct preprocessor* preprocessor= compiler->preprocessor;
    preprocessor_definition_create(name_token->sval, value_token_vec, arguments, preprocessor);
}

void preprocessor_handle_undef_token(struct compile_process* compiler)
{
    struct token* name_token = preprocessor_next_token(compiler);
    preprocessor_definition_remove(compiler->preprocessor, name_token->sval);
}
void preprocessor_handle_warning_token(struct compile_process* compiler)
{
    struct buffer* str_buf = preprocessor_multi_value_string(compiler);
    preprocessor_execute_warning(compiler, buffer_ptr(str_buf));
}

void preprocessor_handle_error_token(struct compile_process* compiler)
{
    struct buffer* str_buf = preprocessor_multi_value_string(compiler);
    preprocessor_execute_error(compiler, buffer_ptr(str_buf));
}

struct token* preprocessor_hashtag_and_identifier(struct compile_process* compiler, const char* str)
{
    if (!preprocessor_next_token_no_increment(compiler))
    {   
        return NULL;    
    }

    if (!token_is_symbol(preprocessor_next_token_no_increment(compiler), '#'))
    {
        return NULL;
    }

    vector_save(compiler->token_vec_original);
    // Skip the hashtag symbol
    preprocessor_next_token(compiler);

    struct token* target_token = preprocessor_next_token_no_increment(compiler);
    if ((token_is_identifier(target_token) && S_EQ(target_token->sval, str)) || token_is_keyword(target_token, str))
    {
        // Pop off the target token
        preprocessor_next_token(compiler);
        // Purge the vector save
        vector_save_purge(compiler->token_vec_original);
        return target_token;
    }

    vector_restore(compiler->token_vec_original);
    return NULL;

}

/**
 * @brief Return true if their is a hastag and any type of preprocessor if statement
 * elif is not included.
 * 
 * @param compiler 
 * @return true 
 * @return false 
 */
bool preprocessor_is_hashtag_and_any_starting_if(struct compile_process* compiler)
{
    return preprocessor_hashtag_and_identifier(compiler, "if") ||
            preprocessor_hashtag_and_identifier(compiler, "ifdef") ||
            preprocessor_hashtag_and_identifier(compiler, "ifndef");
}

void preprocessor_skip_to_endif(struct compile_process* compiler)
{
    while(!preprocessor_hashtag_and_identifier(compiler, "endif"))
    {
        if (preprocessor_is_hashtag_and_any_starting_if(compiler))
        {
            preprocessor_skip_to_endif(compiler);
            continue;
        }
        preprocessor_next_token(compiler);
    }
}

void preprocessor_read_to_end_if(struct compile_process* compiler, bool true_clause)
{
    while(preprocessor_next_token_no_increment(compiler) && !preprocessor_hashtag_and_identifier(compiler, "endif"))
    {
        if (true_clause)
        {
            preprocessor_handle_token(compiler, preprocessor_next_token(compiler));
            continue;
        }

        // Skip the unexpected token
        preprocessor_next_token(compiler);

        if (preprocessor_is_hashtag_and_any_starting_if(compiler))
        {
            preprocessor_skip_to_endif(compiler);
        }
    }
}
int preprocessor_evaluate_number(struct preprocessor_node* node)
{
    return node->const_val.llnum;
}

int preprocessor_evaluate_identifier(struct compile_process* compiler, struct preprocessor_node* node)
{
    struct preprocessor* preprocessor = compiler->preprocessor;
    struct preprocessor_definition* definition = preprocessor_get_definition(preprocessor, node->sval);
    if (!definition)
    {
        return true;
    }

    if (vector_count(preprocessor_definition_value(definition)) > 1)
    {
        struct vector* node_vector = vector_create(sizeof(struct preprocessor_node*));
        struct expressionable* expressionable = expressionable_create(&preprocessor_expressionable_config, preprocessor_definition_value(definition), node_vector, EXPRESSIONABLE_FLAG_IS_PREPROCESSOR_EXPRESSION);
        expressionable_parse(expressionable);
        struct preprocessor_node* node = expressionable_node_pop(expressionable);
        int val = preprocessor_evaluate(compiler, node);
        return val;
    }

    if (vector_count(preprocessor_definition_value(definition)) == 0)
    {
        return false;
    }

    return preprocessor_definition_evaluated_value(definition, NULL);
}

int preprocessor_arithmetic(struct compile_process* compiler, long left_operand, long right_operand, const char* op)
{
    bool success = false;
    long result = arithmetic(compiler, left_operand, right_operand, op, &success);
    if (!success)
    {
        compiler_error(compiler, "We do not support the operator %s for preprocessor arithmetic\n", op);
    }
    return result;
}
int preprocessor_evaluate_exp(struct compile_process* compiler, struct preprocessor_node* node)
{
    // if (preprocessor_exp_is_macro_function_call(node))
    // {
    //     #warning "handle macro function call"
    // }

    long left_operand = preprocessor_evaluate(compiler, node->exp.left);
    if (node->exp.right->type == PREPROCESSOR_TENARY_NODE)
    {
        #warning "handle tenary node"
    }

    long right_operand = preprocessor_evaluate(compiler, node->exp.right);
    return preprocessor_arithmetic(compiler, left_operand, right_operand, node->exp.op);

}
int preprocessor_evaluate(struct compile_process* compiler, struct preprocessor_node* root_node)
{
    struct preprocessor_node* current = root_node;
    int result = 0;
    switch(current->type)
    {
        case PREPROCESSOR_NUMBER_NODE:
            result = preprocessor_evaluate_number(current);
        break;

        case PREPROCESSOR_IDENTIFIER_NODE:
            result = preprocessor_evaluate_identifier(compiler, current);
            break;

        case PREPROCESSOR_EXPRESSION_NODE:
            result = preprocessor_evaluate_exp(compiler, current);
            break;
    }
    return result;
}

int preprocessor_parse_evaluate(struct compile_process* compiler, struct vector* token_vec)
{
    struct vector* node_vector = vector_create(sizeof(struct preprocessor_node*));
    struct expressionable* expressionable = expressionable_create(&preprocessor_expressionable_config, token_vec, node_vector, 0);
    expressionable_parse(expressionable);
    struct preprocessor_node* root_node = expressionable_node_pop(expressionable);
    return preprocessor_evaluate(compiler, root_node);
}

void preprocessor_handle_if_token(struct compile_process* compiler)
{
    int result = preprocessor_parse_evaluate(compiler, compiler->token_vec_original);
    preprocessor_read_to_end_if(compiler, result > 0);
}

void preprocessor_handle_ifdef_token(struct compile_process* compiler)
{
    struct token* condition_token = preprocessor_next_token(compiler);
    if (!condition_token)
    {
        compiler_error(compiler, "No condition token was provided.\n");
    }

    struct preprocessor_definition* definition = preprocessor_get_definition(compiler->preprocessor, condition_token->sval);
    
    // Read the body of the ifdef 
    preprocessor_read_to_end_if(compiler, definition != NULL);
}


void preprocessor_handle_ifndef_token(struct compile_process* compiler)
{
    struct token* condition_token = preprocessor_next_token(compiler);
    if (!condition_token)
    {
        compiler_error(compiler, "No condition token was provided\n");
    }
    struct preprocessor_definition* definition = preprocessor_get_definition(compiler->preprocessor, condition_token->sval);
    preprocessor_read_to_end_if(compiler, definition == NULL);

}

int preprocessor_handle_hashtag_token(struct compile_process* compiler, struct token* token)
{
    bool is_preprocessed = false;
    struct token* next_token = preprocessor_next_token(compiler);
    if (preprocessor_token_is_define(next_token))
    {
        // handle the definition token.
        preprocessor_handle_definition_token(compiler);
        is_preprocessed = true;
    }
    else if(preprocessor_token_is_undef(next_token))
    {
        preprocessor_handle_undef_token(compiler);
        is_preprocessed = true;
    }
    else if(preprocessor_token_is_warning(next_token))
    {
        preprocessor_handle_warning_token(compiler);
        is_preprocessed = true;
    }
    else if(preprocessor_token_is_error(next_token))
    {
        preprocessor_handle_error_token(compiler);
        is_preprocessed = true;
    }
    else if(preprocessor_token_is_if(next_token))
    {
        preprocessor_handle_if_token(compiler);
        is_preprocessed = true;   
    }
    else if(preprocessor_token_is_ifdef(next_token))
    {
        preprocessor_handle_ifdef_token(compiler);
        is_preprocessed = true;
    }
    else if(preprocessor_token_is_ifndef(next_token))
    {
        preprocessor_handle_ifndef_token(compiler);
        is_preprocessed = true;
    }

    return is_preprocessed;
}
void preprocessor_handle_symbol(struct compile_process* compiler, struct token* token)
{
    int is_preprocessed = false;
    if (token->cval == '#')
    {
        is_preprocessed = preprocessor_handle_hashtag_token(compiler, token);
    }

    if (!is_preprocessed)
    {
        preprocessor_token_push_dst(compiler, token);
    }
}
void preprocessor_handle_token(struct compile_process* compiler, struct token* token)
{
    switch(token->type)
    {
        // Handle all tokens here..


        case TOKEN_TYPE_SYMBOL:
            preprocessor_handle_symbol(compiler, token);
            break;
        case TOKEN_TYPE_NEWLINE:
                // Ignored
            break;

        default:
            preprocessor_token_push_dst(compiler, token);
    };

}
int preprocessor_run(struct compile_process* compiler)
{
    #warning "add our source file as an included file"
    vector_set_peek_pointer(compiler->token_vec_original, 0);
    struct token* token = preprocessor_next_token(compiler);
    while(token)
    {
        preprocessor_handle_token(compiler, token);
        token = preprocessor_next_token(compiler);
    }

    return 0;
}