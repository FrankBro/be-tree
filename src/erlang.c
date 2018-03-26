#include <erl_nif.h>

#include "betree.h"

static ERL_NIF_TERM atom_error;
static ERL_NIF_TERM atom_ok;

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

    initialize_config();
    cnode = make_cnode(config, NULL);

    return 0;
}

static ERL_NIF_TERM
evaluate_expressions(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    (void) argc;

}

static ErlNifFunc nif_functions[] = {
    {"evaluate_expressions", 3, evaluate_expressions},
    // {"insert_expression", 3, insert_expression},
};

ERL_NIF_INIT(rtb_be_tree_vm, nif_functions, &on_load, NULL, NULL, NULL);
