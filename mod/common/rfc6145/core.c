/* TODO (warning) read the erratas more (6145 and 6146). */

#include "nat64/mod/common/icmp_wrapper.h"
#include "nat64/mod/common/stats.h"
#include "nat64/mod/common/rfc6145/core.h"
#include "nat64/mod/common/rfc6145/common.h"

static verdict translate_fragment(struct tuple *tuple, struct sk_buff *in, struct sk_buff **out)
{
	struct translation_steps *steps = ttpcomm_get_steps(skb_l3_proto(in), skb_l4_proto(in));
	verdict result;

	*out = NULL;

	result = steps->skb_create_fn(in, out);
	if (result != VER_CONTINUE)
		goto fail;
	result = steps->l3_hdr_fn(tuple, in, *out);
	if (result != VER_CONTINUE)
		goto fail;
	if (skb_has_l4_hdr(in)) {
		result = steps->l3_payload_fn(tuple, in, *out);
		if (result != VER_CONTINUE)
			goto fail;
	} else {
		if (is_error(copy_payload(in, *out)))
			goto drop;
	}

	return VER_CONTINUE;

drop:
	result = VER_DROP;
	/* Fall through. */

fail:
	kfree_skb(*out);
	*out = NULL;
	return result;
}

verdict translating_the_packet(struct tuple *out_tuple, struct sk_buff *in_skb,
		struct sk_buff **out_skb)
{
	struct sk_buff *current_out_skb, *prev_out_skb = NULL;
	unsigned int payload_len;
	verdict result;

	log_debug("Step 4: Translating the Packet");

	/* Translate the first fragment or a complete packet. */
	result = translate_fragment(out_tuple, in_skb, out_skb);
	if (result != VER_CONTINUE)
		return result;

	/* If not a fragment, the next "while" will be omitted. */
	skb_walk_frags(in_skb, in_skb) {
		log_debug("Translating a Fragment Packet");
		result = translate_fragment(out_tuple, in_skb, &current_out_skb);
		if (result != VER_CONTINUE) {
			kfree_skb(*out_skb);
			*out_skb = NULL;
			return result;
		}

		if (!prev_out_skb)
			skb_shinfo(*out_skb)->frag_list = current_out_skb;
		else
			prev_out_skb->next = current_out_skb;
		payload_len = skb_payload_len_frag(current_out_skb);
		(*out_skb)->len += payload_len;
		(*out_skb)->data_len += payload_len;
		(*out_skb)->truesize += current_out_skb->truesize;

		prev_out_skb = current_out_skb;
	}

	log_debug("Done step 4.");
	return VER_CONTINUE;
}
