#include "EthFiltering.h"

/**
 * Local copy of the enum because the header in OAN repo is not C comptible
 */
enum EthProtocol {
    ETH_PROTO_OANAUDIO = 0x0681,
    ETH_PROTO_OANDISCO = 0x0682,
    ETH_PROTO_OANCONTROL = 0x0683,
    ETH_PROTO_OANSYNC = 0x0684
} typedef EthProtocol;

static NPF_ETH_TYPE_MATCH(disco_pck, ETH_PROTO_OANDISCO);
static NPF_ETH_TYPE_MATCH(audio_pck, ETH_PROTO_OANAUDIO);
static NPF_ETH_TYPE_MATCH(sync_pck, ETH_PROTO_OANSYNC);
static NPF_ETH_TYPE_MATCH(control_pck, ETH_PROTO_OANCONTROL);

static NPF_RULE(oan_disco, NET_OK, disco_pck);
static NPF_RULE(oan_audio, NET_OK, audio_pck);
static NPF_RULE(oan_sync, NET_OK, sync_pck);
static NPF_RULE(oan_control, NET_OK, control_pck);

void eth_filtering_setup() {
    npf_append_recv_rule(&oan_disco);
    npf_append_recv_rule(&oan_audio);
    npf_append_recv_rule(&oan_sync);
    npf_append_recv_rule(&oan_control);
    npf_append_recv_rule(&npf_default_drop);
}