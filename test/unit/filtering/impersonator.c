#include "nat64/mod/stateful/joold.h"
#include "nat64/unit/unit_test.h"

static struct fake {
	int junk;
} dummy;

struct config_candidate *cfgcandidate_alloc(void)
{
	return (struct config_candidate *)&dummy;
}

void cfgcandidate_get(struct config_candidate *candidate)
{
	/* No code. */
}

void cfgcandidate_put(struct config_candidate *candidate)
{
	/* No code. */
}

void joold_add(struct joold_queue *queue, struct session_entry *entry,
		struct bib *bib, struct net *ns)
{
	/* No code. */
}

void joold_config_copy(struct joold_queue *queue, struct joold_config *config)
{
	broken_unit_call(__func__);
}

struct joold_queue *joold_alloc(struct net *ns)
{
	return (struct joold_queue *)&dummy;
}

void joold_get(struct joold_queue *queue)
{
	/* No code. */
}

void joold_put(struct joold_queue *queue)
{
	/* No code. */
}
