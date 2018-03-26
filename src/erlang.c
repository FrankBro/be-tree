#include <erl_nif.h>

#include "ast.h"
#include "betree.h"

static ERL_NIF_TERM atom_error;
static ERL_NIF_TERM atom_ok;
static ERL_NIF_TERM atom_bad_expr;

static ERL_NIF_TERM
make_atom(ErlNifEnv *env, const char *name)
{
    ERL_NIF_TERM ret;

    if(enif_make_existing_atom(env, name, &ret, ERL_NIF_LATIN1)) {
        return ret;
    }
    return enif_make_atom(env, name);
}

static struct config* config;
static struct cnode* cnode;

void initialize_config()
{
    config = malloc(sizeof(struct config));
    config->lnode_max_cap = 3;
    config->attr_domain_count = 3;
    config->attr_domains = malloc(sizeof(struct config*) * config->attr_domain_count);
    struct attr_domain attr_domain_a = { .name = "a", .minBound = 0, .maxBound = 10 };
    config->attr_domains[0] = attr_domain_a;
    struct attr_domain attr_domain_b = { .name = "b", .minBound = 0, .maxBound = 10 };
    config->attr_domains[1] = attr_domain_b;
    struct attr_domain attr_domain_c = { .name = "c", .minBound = 0, .maxBound = 10 };
    config->attr_domains[2] = attr_domain_c;
}

static int
on_load(ErlNifEnv* env, void** priv, ERL_NIF_TERM info)
{
    (void) priv, (void) info;

    atom_ok = make_atom(env, "ok");
    atom_error = make_atom(env, "error");
    atom_bad_expr = make_atom(env, "bad_expr");

    initialize_config();
    cnode = make_cnode(config, NULL);

    return 0;
}

int parse(const char *text, struct ast_node **node);

/*
static ERL_NIF_TERM
insert_expression(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    (void) argc;

    char* expr_str = NULL;
    size_t len;
    ERL_NIF_TERM retval;

    if (!enif_get_list_length(env, argv[0], &len)) {
        retval = enif_make_badarg(env);
        goto error;
    }

    len++;

    expr_str = malloc(sizeof(char) * len);

    if (enif_get_string(env, argv[0], expr_str, len, ERL_NIF_LATIN1) <= 0) {
        retval = enif_make_badarg(env);
        goto error;
    }

    struct ast_node* node = NULL;
    if(parse(expr_str, &node) != 0) {
        retval = enif_make_tuple2(env, atom_error, atom_bad_expr);
        goto error;
    }

    insert_be_tree(config, , cnode, NULL);
error:
    if(expr_str != NULL) {
        free(expr_str);
    }
    if(node != NULL) {
        free_ast_node(node);
    }

    return retval;
}
*/

static ErlNifFunc nif_functions[] = {
    // {"evaluate_expressions", 3, evaluate_expressions},
    // {"insert_expression", 3, insert_expression},
};

ERL_NIF_INIT(rtb_be_tree_vm, nif_functions, &on_load, NULL, NULL, NULL);
