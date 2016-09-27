#ifdef __clang__
#define __KERNEL__
#endif

#include <linux/percpu.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/etherdevice.h>
#include <linux/netdevice.h>

#define LPREF "[ioscsi]: "

#undef PDEBUG             /* undef it, just in case */
#ifdef IOSCSI_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG LPREF fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

struct ioscsi_ether_priv {
};

struct ioscsi_priv {
    struct net_device *dev;
    spinlock_t lock;
    int rx_int_enabled;
};

static int ioscsi_timeout = 5;
module_param(ioscsi_timeout, int, 0);

int use_napi;
struct net_device *ioscsi_dev = NULL;

static void ioscsi_rx_ints(struct net_device *dev, int enable)
{
	struct ioscsi_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}

/*
	int	(*create) (struct sk_buff *skb, struct net_device *dev,
			   unsigned short type, const void *daddr,
			   const void *saddr, unsigned int len);
	int	(*parse)(const struct sk_buff *skb, unsigned char *haddr);
	int	(*rebuild)(struct sk_buff *skb);
	int	(*cache)(const struct neighbour *neigh, struct hh_cache *hh, __be16 type);
	void	(*cache_update)(struct hh_cache *hh,
				const struct net_device *dev,
				const unsigned char *haddr);
*/
/*const int ioscsi_create_header = 0;*/
/*const int ioscsi_rebuild_header = 0;*/
static const struct header_ops ioscsi_header_ops = {
    /*.create  = ioscsi_create_header,*/
	/*.rebuild = ioscsi_rebuild_header*/
};

int ioscsi_open(struct net_device *dev)
{
	memcpy(dev->dev_addr, "\0IOS", ETH_ALEN);
	netif_start_queue(dev);
    printk(KERN_INFO LPREF "Device open\n");
	return 0;
}

int ioscsi_release(struct net_device *dev)
{
    netif_stop_queue(dev);
    printk(KERN_INFO LPREF "Device closed\n");
    return 0;
}

/*int ioscsi_tx;*/
/*int ioscsi_ioctl;*/
/*int ioscsi_config;*/
/*int ioscsi_stats;*/
/*int ioscsi_change_mtu;*/
/*int ioscsi_tx_timeout;*/

static const struct net_device_ops ioscsi_netdev_ops = {
	.ndo_open            = ioscsi_open,
	.ndo_stop            = ioscsi_release,
	/*.ndo_start_xmit      = ioscsi_tx,*/
	/*.ndo_do_ioctl        = ioscsi_ioctl,*/
	/*.ndo_set_config      = ioscsi_config,*/
	/*.ndo_get_stats       = ioscsi_stats,*/
	/*.ndo_change_mtu      = ioscsi_change_mtu,*/
	/*.ndo_tx_timeout      = ioscsi_tx_timeout*/
};

void ioscsi_init(struct net_device *dev)
{
    struct ioscsi_priv *priv;

	ether_setup(dev); /* assign some of the fields */
	dev->watchdog_timeo = ioscsi_timeout;

	dev->netdev_ops = &ioscsi_netdev_ops;
	/*dev->header_ops = &ioscsi_header_ops;*/

	/* keep the default flags, just add NOARP */
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_HW_CSUM;

	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
    priv->dev = dev;

	if (use_napi) {
		// netif_napi_add(dev, &priv->napi, snull_poll,2);
	}
	memset(priv, 0, sizeof(struct ioscsi_priv));
	spin_lock_init(&priv->lock);
	ioscsi_rx_ints(dev, 1);		/* enable receive interrupts */
}

static int __init ip_over_scsi_init(void)
{
    int result;

    ioscsi_dev = alloc_netdev(sizeof(struct ioscsi_priv), "ioscsi%d", NET_NAME_ENUM, ioscsi_init);
        
	if (!ioscsi_dev) {
		printk(KERN_ERR LPREF "Could not allocate network device\n");
		return -ENODEV;
	}

    if((result = register_netdev(ioscsi_dev))) {
        free_netdev(ioscsi_dev);
		printk(KERN_ERR LPREF "error %i registering device \"%s\"\n", result, ioscsi_dev->name);
        return -ENODEV;
    }

    PDEBUG("Initialized\n");
    return 0;
}

static void __exit ip_over_scsi_exit(void)
{
    unregister_netdev(ioscsi_dev);
    free_netdev(ioscsi_dev);
    PDEBUG("Exited\n");
}

module_init(ip_over_scsi_init);
module_exit(ip_over_scsi_exit);

MODULE_AUTHOR("Ivan Alechin");
MODULE_DESCRIPTION("IP over SCSI");
MODULE_LICENSE("GPL v3");
