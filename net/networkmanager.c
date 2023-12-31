#include <net/networkmanager.h>

#include <net/skb.h>
#include <kconfig.h>
#include <kutils.h>
#include <memory.h>

static int __default_restart(struct networkmanager*); 
static int __default_start(struct networkmanager*);
static int __default_stop(struct networkmanager*);

struct network_manager_ops default_net_ops = {
    .restart = __default_restart,
    .start = __default_start,
    .stop = __default_stop,
};

struct networkmanager* nm_new()
{    
    struct networkmanager* netd = create(struct networkmanager);
    if(netd == NULL) return NULL;

    netd->state = NETD_UNINITIALIZED;
    netd->packets = 0;
    
    netd->skb_tx_queue = skb_new_queue();
    if(netd->skb_tx_queue == NULL){
        kfree(netd);
        return NULL;
    }

    netd->skb_rx_queue = skb_new_queue();
    if(netd->skb_rx_queue == NULL){
        kfree(netd->skb_tx_queue);
        kfree(netd);
        return NULL;
    }
    
    netd->ops = &default_net_ops;

    kref_init(&netd->ref);

    return netd;
}

void nm_free(struct networkmanager* netd){
    if(netd == NULL) return;

    if(netd->skb_tx_queue != NULL) skb_free_queue(netd->skb_tx_queue);
    if(netd->skb_rx_queue != NULL) skb_free_queue(netd->skb_rx_queue);

    kfree(netd);
}

static int __default_restart(struct networkmanager* netd)
{
    return 0;
}

static int __default_start(struct networkmanager* netd)
{
    ERR_ON_NULL(netd);

    if(netd->instance != NULL){
        return 0;
    }

    pid_t pid = start("netd", 0, NULL);
    if(pid < 0){
        return -1;
    }

    netd->instance = pcb_get_by_pid(pid);
    if(netd->instance == NULL){
        return -1;
    }

    return 0;
}

static int __default_stop(struct networkmanager* netd)
{
    ERR_ON_NULL(netd);

    if(netd->instance == NULL){
        return 0;
    }

    pcb_kill(netd->instance->pid);
    netd->instance = NULL;

    return 0;
}
