/* Plutosdr usb gadget driver
 *
 * Copyright (c) 2017 Hoernchen la@tfc-server.de
 * Copyright (c) 2020 Oleg Malyutin <oleg.radioprograms@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#define VERBOSE_DEBUG
#define DEBUG

//#define TEST_JOB
#ifdef TEST_JOB
#define NULL (0)
#include "linux/include/linux/module.h"
#include "linux/include/linux/init.h"
#include "linux/include/linux/fs.h"
#include "linux/include/asm-generic/uaccess.h"
#include "linux/include/linux/moduleparam.h"
#include "linux/include/linux/err.h"
#include "linux/include/linux/export.h"
#include "linux/include/linux/slab.h"
#include "linux/include/linux/mutex.h"
#include "linux/include/linux/of.h"
#include "linux/include/linux/kernel.h"
#include "linux/include/linux/device.h"
#include "linux/include/linux/delay.h"
#include "linux/include/linux/iio/sysfs.h"
#include "linux/include/linux/iio/iio.h"
#include "linux/include/linux/iio/machine.h"
#include "linux/include/linux/iio/driver.h"
#include "linux/include/linux/iio/consumer.h"
#include "linux/include/linux/iio/buffer.h"
#include "linux/include/linux/iio/buffer_impl.h"
#include "linux/include/linux/iio/buffer-dma.h"
#include "linux/include/linux/iio/buffer-dmaengine.h"
#include "linux/include/linux/iio/configfs.h"
#include "linux/include/linux/iio/consumer.h"
#include "linux/include/linux/iio/driver.h"
#include "linux/include/linux/iio/hw_consumer.h"
#include "linux/include/linux/iio/kfifo_buf.h"
#include "linux/include/linux/clk.h"
#include "linux/include/linux/clkdev.h"
#include "linux/include/linux/clk-provider.h"
#include "linux/drivers/iio/adc/ad9361.h"
#include "linux/include/linux/usb/composite.h"
#include "linux/include/linux/err.h"
#include "linux/include/linux/reboot.h"
#include "linux/include/linux/kallsyms.h"
#include "linux/include/linux/sysfs.h"
#else

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>

#include <linux/err.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/of.h>

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/iio/sysfs.h>

#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/iio/consumer.h>
#include <linux/iio/buffer.h>
#include "linux/include/linux/iio/buffer_impl.h"
#include <linux/iio/buffer-dma.h>

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include "linux/drivers/iio/adc/ad9361.h"

#include <linux/usb/composite.h>
#include <linux/err.h>
#include <linux/reboot.h>
#include <linux/kallsyms.h>

#include <linux/sysfs.h>

#endif

//#define DBGTRACE printk("in %s:%d\n", __FUNCTION__, __LINE__);
//#define NUMBLK 16
//#define REQBSIZE ALIGN(0x100000, sizeof(uint16_t) * 512 * 1000)
#define NUMBLK 4
#define REQBSIZE ALIGN(0x400000, sizeof(uint16_t) * 4 * 524288)

/*----------------------------------------------------------------------------------------------------*/
static struct plutousb {
    struct iio_dev *ad9361_iiodev;
    struct iio_dev *ad9361_phydev;
    struct workqueue_struct *wq_subm;
    struct workqueue_struct *wq_enq;
    struct workqueue_struct *wq_cmd;
    struct work_struct work_tx;
    struct work_struct work_enq;
    struct work_struct work_cmd;
    //tx buffers
    spinlock_t tx_lock;
    struct list_head tx_idle;
    wait_queue_head_t tx_wq;
    //control transfer commands
    spinlock_t cmd_lock;
    struct list_head cmd_idle;
    wait_queue_head_t cmd_wq;
    struct usb_function	function;
    struct usb_ep *in_ep;
    volatile unsigned int enabled;
} mydata;
/*----------------------------------------------------------------------------------------------------*/
static struct {
    struct iio_buffer_block block;
    struct usb_request*req;
} blockreqs [NUMBLK];
/*----------------------------------------------------------------------------------------------------*/
static unsigned int sysfs_num_bufs;
/*----------------------------------------------------------------------------------------------------*/
struct iiousb_opts {
    struct usb_function_instance func_inst;
    struct mutex lock;
    int refcnt;
};
/*----------------------------------------------------------------------------------------------------*/
static struct iio_dma_buffer_queue *iio_buffer_to_queue(struct iio_buffer *buf)
{
    return container_of(buf, struct iio_dma_buffer_queue, buffer);
}
/*----------------------------------------------------------------------------------------------------*/
static inline struct plutousb *func_to_iiousb(struct usb_function *f)
{
    return container_of(f, struct plutousb, function);
}
/*----------------------------------------------------------------------------------------------------*/
static struct usb_interface_descriptor iiousb_intf_alt0 = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
    /* .iInterface = DYNAMIC */
};
/*----------------------------------------------------------------------------------------------------*/
/* high speed support: */
static struct usb_endpoint_descriptor hs_source_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN,
    .bmAttributes = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize = cpu_to_le16(512),
};
/*----------------------------------------------------------------------------------------------------*/
static struct usb_descriptor_header *hs_iiousb_descs[] = {
    (struct usb_descriptor_header*) &iiousb_intf_alt0,
    (struct usb_descriptor_header*) &hs_source_desc,
    NULL,
};
/*----------------------------------------------------------------------------------------------------*/
static struct usb_string strings_iiousb[] = {
    [0].s = "delivers raw samples",
    {  }
};
/*----------------------------------------------------------------------------------------------------*/
static struct usb_gadget_strings stringtab_iiousb = {
    .language = 0x0409,	/* en-us */
    .strings = strings_iiousb,
};
/*----------------------------------------------------------------------------------------------------*/
static struct usb_gadget_strings *iiousb_strings[] = {
    &stringtab_iiousb,
    NULL,
};
/*----------------------------------------------------------------------------------------------------*/
static int iiousb_bind(struct usb_configuration *c, struct usb_function *f)
{
    struct usb_composite_dev *cdev = c->cdev;
    int	id;
    int ret;
    // allocate interface ID(s)
    id = usb_interface_id(c, f);

    if (id < 0) return id;

    iiousb_intf_alt0.bInterfaceNumber = id;
    mydata.in_ep = usb_ep_autoconfig(cdev->gadget, &hs_source_desc);
    if (!mydata.in_ep) {
        ERROR(cdev, "%s: can't autoconfigure on %s\n",
        f->name, cdev->gadget->name);

        return -ENODEV;

    }
    // unfuck autoconfig
    hs_source_desc.wMaxPacketSize = cpu_to_le16(512);
    ret = usb_assign_descriptors(f, NULL, hs_iiousb_descs, NULL, NULL);

    if (ret) return ret;

    printk("%s speed %s: IN/%s, maxp/%x\n",
          (gadget_is_superspeed(c->cdev->gadget) ? "super" :
          (gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full")),
          f->name, mydata.in_ep->name, mydata.in_ep->maxpacket);

    return 0;
}
/*----------------------------------------------------------------------------------------------------*/
static void iiousb_free_func(struct usb_function *f)
{
    struct iiousb_opts *opts;
    opts = container_of(f->fi, struct iiousb_opts, func_inst);
    mutex_lock(&opts->lock);
    opts->refcnt--;
    mutex_unlock(&opts->lock);
    usb_free_all_descriptors(f);
    kfree(func_to_iiousb(f));
}
/*----------------------------------------------------------------------------------------------------*/
static void put_req(struct usb_request *req, struct list_head *list, spinlock_t *lock)
{
    unsigned long flags;
    //struct list_head *head = &mydata.tx_idle;
    spin_lock_irqsave(lock, flags);
    list_add_tail(&req->list, list);
    spin_unlock_irqrestore(lock, flags);
}
/*----------------------------------------------------------------------------------------------------*/
static struct usb_request* take_req(struct list_head *list, spinlock_t *lock)
{
    unsigned long flags;
    struct usb_request *req;
    spin_lock_irqsave(lock, flags);
    if (list_empty(list)) {
        req = 0;
    } else {
        req = list_first_entry(list, struct usb_request, list);
        list_del(&req->list);
    }
    spin_unlock_irqrestore(lock, flags);

    return req;
}
/*----------------------------------------------------------------------------------------------------*/
static void txiiobuf_complete(struct usb_ep *ep, struct usb_request *req)
{
    if (!ep->driver_data) return;

    put_req(req, &mydata.tx_idle, &mydata.tx_lock);
    wake_up(&mydata.tx_wq);
}
/*----------------------------------------------------------------------------------------------------*/
static void work_func_enq(struct work_struct *work){
    struct iio_buffer *buffer = mydata.ad9361_iiodev->buffer;
    int ret;
    do {
        struct usb_request* req = 0;
        ret = wait_event_interruptible(mydata.tx_wq,
                                       ((req = take_req(&mydata.tx_idle, &mydata.tx_lock))));
        if (!req) {
            break;
        }
        //        status = req->status;
        //        switch (status) {
        //        case 0:				/* normal completion? */
        //            break;
        //            /* this endpoint is normally active while we're configured */
        //        case -ECONNABORTED:		/* hardware forced ep reset */
        //        case -ECONNRESET:		/* request dequeued */
        //        case -ESHUTDOWN:		/* disconnect from host */
        //        case -EOVERFLOW:		/* buffer overrun on read */
        //        default:
        //            printk("complete --> %d, %d/%d\n", status, req->actual, req->length);
        //        case -EREMOTEIO:		/* short read */
        //            break;
        //        }
        if(mydata.enabled != 1) usb_ep_free_request(mydata.in_ep, req);

        buffer->access->enqueue_block(buffer, req->context);

    } while(1);
}
/*----------------------------------------------------------------------------------------------------*/
static void work_func_tx(struct work_struct *work)
{
    struct iio_buffer *buffer = mydata.ad9361_iiodev->buffer;
    struct usb_ep *ep = mydata.in_ep;
    struct iio_buffer_block localblk;
    int ret;
    int counter = 0;
    do {
        if (!buffer->access->data_available(buffer)) {

            ret = wait_event_interruptible(buffer->pollq,
                                           buffer->access->data_available(buffer) ||
                                           mydata.ad9361_iiodev->info == NULL);

            if (ret || mydata.ad9361_iiodev->info == NULL ) return;

        }
        else{
            printk("nowait\n");
        }
        ret = buffer->access->dequeue_block(buffer, &localblk);
        //if(ret != 0) printk("WAT%d",ret);
        blockreqs[localblk.id].block = localblk;
        //printk("bid %d, %d\n", localblk.id, counter);
        //msleep(100);
        //printk("in %s:%d l:%.8x a:%.8x %d\n", __FUNCTION__, __LINE__,
        //        blockreqs[localblk.id].req->length, blockreqs[localblk.id].req->buf,
        //        blockreqs[localblk.id].req->context);
        ret = usb_ep_queue(ep, blockreqs[localblk.id].req, GFP_ATOMIC);
        if (ret) {
            struct usb_composite_dev *cdev;
            ERROR(cdev, "start %s%s %s --> %d\n", "", "OUT", ep->name, ret);

            return;

        }
        counter++;
    } while(1);
}
/*----------------------------------------------------------------------------------------------------*/
//DECLARE_WORK(workfn, work_func);
/*----------------------------------------------------------------------------------------------------*/
static void disable_iiousb(struct plutousb *ss)
{
    //struct iio_buffer *buffer = mydata.ad9361_iiodev->buffer;
    struct usb_composite_dev	*cdev = mydata.function.config->cdev;
    struct usb_ep *ep = mydata.in_ep;
    int i;
    mydata.enabled = 0;
    usb_ep_disable(mydata.in_ep);
    //wake_up(&mydata.write_wq);
    //wake_up(&buffer->pollq);
    for(i =0; i < NUMBLK; i++) usb_ep_dequeue(ep, blockreqs[i].req);
    VDBG(cdev, "%s disabled\n", mydata.function.name);
}
/*----------------------------------------------------------------------------------------------------*/
static int enable_iiousb(struct usb_composite_dev *cdev,
                         struct plutousb *ss,
                         int alt)
{
    int result = 0, i=0;
    struct usb_ep *ep = mydata.in_ep;
    struct iio_dma_buffer_queue *queue = iio_buffer_to_queue(mydata.ad9361_iiodev->buffer);
    result = config_ep_by_speed(cdev->gadget, &(mydata.function), ep);

    if (result) return result;

    result = usb_ep_enable(ep);

    if (result < 0) return result;

    ep->driver_data = ss;
    for(i =0; i < NUMBLK; i++){
        blockreqs[i].req = usb_ep_alloc_request(ep, GFP_ATOMIC);
        blockreqs[i].req->length = queue->blocks[i]->block.size;
        blockreqs[i].req->buf = queue->blocks[i]->vaddr;
        blockreqs[i].req->context = &blockreqs[i].block;
        blockreqs[i].req->complete = txiiobuf_complete;
    }
    mydata.enabled = 1;
    queue_work(mydata.wq_subm, &mydata.work_tx);
    queue_work(mydata.wq_enq, &mydata.work_enq);
    queue_work(mydata.wq_cmd, &mydata.work_cmd);
    DBG(cdev, "%s enabled, alt intf %d\n", mydata.function.name, alt);

    return result;
}
/*----------------------------------------------------------------------------------------------------*/
static int iiousb_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
    struct plutousb *ss = func_to_iiousb(f);
    struct usb_composite_dev *cdev = f->config->cdev;
    disable_iiousb(ss);
    return enable_iiousb(cdev, ss, alt);
}
/*----------------------------------------------------------------------------------------------------*/
static void iiousb_disable(struct usb_function *f)
{
    struct plutousb *ss = func_to_iiousb(f);
    disable_iiousb(ss);
}
/*----------------------------------------------------------------------------------------------------*/
static asmlinkage long (*sys_reboot) (int magic, int magic2, int flag, void *arg);
/*----------------------------------------------------------------------------------------------------*/
static void work_func_cmd(struct work_struct *work)
{
    int ret;
    do {
        struct usb_request* req = 0;
        int cmd;
        char *reqdata, *beginvalue;
        ret = wait_event_interruptible(mydata.cmd_wq,
                                       ((req = take_req(&mydata.cmd_idle, &mydata.cmd_lock))));

        if (!req) break;

        if(req->status != 0) goto out;

        cmd = (int)req->context;
        reqdata = (char*)req->buf;
        beginvalue = reqdata;
        strsep(&beginvalue, "=");
        //printk("cmd %d: key:%s\t val:%s\n", cmd, reqdata, beginvalue);
        switch (cmd) {
        case 0x00: //enable/disable
        {
            if(reqdata[0] == '1') {
                iio_update_buffers(mydata.ad9361_iiodev, mydata.ad9361_iiodev->buffer, NULL);
            }
            else {
                iio_update_buffers(mydata.ad9361_iiodev, NULL, mydata.ad9361_iiodev->buffer);
            }
        }
            break;
        case 0x01: // ad9361_phy_store
        {
            struct attribute** a = mydata.ad9361_phydev->info->attrs->attrs;
            for(; *a != NULL; a++ ) {
                struct iio_dev_attr *this_attr = to_iio_dev_attr((void*)*a);
                if( strcmp(reqdata, this_attr->dev_attr.attr.name) == 0){
                    if(this_attr->dev_attr.store) {
                        this_attr->dev_attr.store(&mydata.ad9361_phydev->dev,
                                                  &this_attr->dev_attr,
                                                  beginvalue,
                                                  strlen(beginvalue));
                    }
                    break;
                }
            }
        }
            break;
        case 0x02: // chan 1 RX_LO
        {
            const struct iio_chan_spec* d = &mydata.ad9361_phydev->channels[1];
            const struct iio_chan_spec_ext_info* ext_info =  d->ext_info;
            for (; ext_info->name; ext_info++) {
                if( strcmp(reqdata, ext_info->name) == 0) {
                    if(ext_info->write) {
                        ext_info->write(mydata.ad9361_phydev,
                                        ext_info->private,
                                        d, beginvalue,
                                        strlen(beginvalue));
                    }
                    break;
                }
            }
        }
            break;
        case 0x03: //chan 4 RX1 manual gain
        {
            const struct iio_chan_spec* d = &mydata.ad9361_phydev->channels[4];
            int gain, gain_mdb, gain_db;
            kstrtoint(reqdata, 10, &gain);
            gain_db = gain/1000;
            gain_mdb = gain - (gain/1000)*1000;
            mydata.ad9361_phydev->info->write_raw(mydata.ad9361_phydev, d, gain_db,
                                                  gain_mdb,
                                                  IIO_CHAN_INFO_HARDWAREGAIN);
            //printk("hwgain: %d %d\n", gain_db, gain_mdb);
        }
            break;
        case 0x04: //chan 4 RX1 sample freq
        {
            const struct iio_chan_spec* d = &mydata.ad9361_phydev->channels[4];
            int sfreq;
            kstrtoint(reqdata, 10, &sfreq);
            mydata.ad9361_phydev->info->write_raw(mydata.ad9361_phydev, d,
                                                  sfreq, 0,
                                                  IIO_CHAN_INFO_SAMP_FREQ);
        }
            break;
        case 0x05: //chan 4 RX1 gain control mode
        {
            const struct iio_chan_spec* d = &mydata.ad9361_phydev->channels[4];
            const struct iio_chan_spec_ext_info* ext_info =  d->ext_info;
            for (; ext_info->name; ext_info++){
                if( strcmp(reqdata, ext_info->name) == 0) {
                    const struct iio_enum *e = (struct iio_enum*)ext_info->private;
                    e->set(mydata.ad9361_phydev, d, 0);
                    //printk("gainmode: %s %d\n", ext_info->name, 0);
                    break;
                }
            }
        }
            break;
        case 0x06: // W rxtx fir coeff
        {
            struct ad9361_rf_phy *phy = iio_priv(mydata.ad9361_phydev);
            phy->bin.write(0,&mydata.ad9361_phydev->dev.kobj, 0, reqdata, 0, strlen(reqdata));
        }
            break;
        case 0x07: // cf-ad9361-lpc channel buffer enable/disable
        {
            int i;
            int gc = mydata.ad9361_iiodev->groupcounter - 1;
            //printk("cf-ad9361-lpc groupcounter: %d\n", gc);
            for(i = 0; i < gc;  ++i){
               const struct attribute_group *ag = mydata.ad9361_iiodev->groups[i];
                if(ag != NULL){
                    const struct iio_dev_attr *this_ag = to_iio_dev_attr((void*)ag);
                    if(this_ag->dev_attr.attr.name != NULL &&
                       strcmp("scan_elements", this_ag->dev_attr.attr.name) == 0) {
                        //printk("cf-ad9361-lpc group name:%s\n", this_ag->dev_attr.attr.name);
                        struct attribute **a = ag->attrs;
                        for(; *a != NULL; a++){
                            struct iio_dev_attr *this_a = to_iio_dev_attr((void*)*a);
                            if( strcmp(reqdata, this_a->dev_attr.attr.name) == 0){
                                this_a->dev_attr.store(&mydata.ad9361_iiodev->dev,
                                                       &this_a->dev_attr,
                                                       beginvalue,
                                                       strlen(beginvalue));
                                //printk("cf-ad9361-lpc channel:%s %s\n",
                                //          this_a->dev_attr.attr.name,
                                //          beginvalue);
                                break;
                            }
                        }
                    }
                }
            }
        }
            break;

        case 0xfe:
            sys_reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART, NULL);
            break;
        case 0xff: //ignore dummy transfer
            break;
        default:
            printk("unknown control req!\n");
        }

out:
        //those are _copies_ of the preallocated ep0 buffer.
        kfree(req->buf);
        kfree(req);

    } while(1);
}
/*----------------------------------------------------------------------------------------------------*/
static void controlxfer_complete(struct usb_ep *ep, struct usb_request *req)
{
    char* tmpbuf;
    struct usb_request *tmpreq;

    if (!ep->driver_data) return;

    tmpreq = kzalloc(sizeof(struct usb_request), GFP_KERNEL);
    memcpy(tmpreq, req, sizeof(struct usb_request));
    tmpbuf =  kzalloc(req->length, GFP_KERNEL);
    memcpy(tmpbuf, req->buf, req->length);
    tmpreq->buf = tmpbuf;
    put_req(tmpreq, &mydata.cmd_idle, &mydata.cmd_lock);
    wake_up(&mydata.cmd_wq);
}
/*----------------------------------------------------------------------------------------------------*/
static int iiousb_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
    struct usb_configuration *c = f->config;
    struct usb_request *req = c->cdev->req;
    //char* reqdata = (char*)req->buf;
    int value = -EOPNOTSUPP;
    u16 w_value = le16_to_cpu(ctrl->wValue);
    u16	w_length = le16_to_cpu(ctrl->wLength);
    //actual preallocated length
    //req->length = USB_COMP_EP0_BUFSIZ;
    //we did not yet receive the actual data, set up a transfer to receive it
    req->zero = 0;
    req->length = w_length;
    req->complete = controlxfer_complete;
    req->context = w_value;
    value = usb_ep_queue(c->cdev->gadget->ep0, req, GFP_ATOMIC);

    if (value < 0) ERROR(c->cdev, "source/sink response, err %d\n", value);

    /* device either stalls (value < 0) or reports success */
    return value;
}
/*----------------------------------------------------------------------------------------------------*/
static bool iiousb_rqmatch(struct usb_function *a,
                           const struct usb_ctrlrequest *ctrl, bool config0)
{
    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE ||
            (ctrl->bRequestType & USB_TYPE_MASK) != USB_TYPE_VENDOR ||
            (ctrl->bRequest != 0xff )) {

        return false;

    }

    return true;
}
/*----------------------------------------------------------------------------------------------------*/
static struct usb_function *iiousb_alloc_func(struct usb_function_instance *fi)
{
    struct iiousb_opts *this_opts;
    this_opts = container_of(fi, struct iiousb_opts, func_inst);
    mutex_lock(&this_opts->lock);
    this_opts->refcnt++;
    mutex_unlock(&this_opts->lock);
    mydata.function.name = "plutosdr gadget";
    mydata.function.bind = iiousb_bind;
    mydata.function.set_alt = iiousb_set_alt;
    mydata.function.disable = iiousb_disable;
    mydata.function.setup = iiousb_setup;
    mydata.function.req_match = iiousb_rqmatch;
    mydata.function.strings = iiousb_strings;
    mydata.function.free_func = iiousb_free_func;

    return &mydata.function;
}
/*----------------------------------------------------------------------------------------------------*/
static void iiosub_attr_release(struct config_item *item)
{
    struct iiousb_opts *this_opts;
    this_opts = container_of(to_config_group(item), struct iiousb_opts,
                             func_inst.group);
    usb_put_function_instance(&this_opts->func_inst);
}
/*----------------------------------------------------------------------------------------------------*/
static struct configfs_item_operations iiousb_ops = {
    .release = iiosub_attr_release,
};
/*----------------------------------------------------------------------------------------------------*/
static ssize_t iiousb_buf_num_show(struct config_item *item, char *page)
{
    return 0;
}
/*----------------------------------------------------------------------------------------------------*/
static ssize_t iiousb_buf_num_store(struct config_item *item, const char *page, size_t len)
{
    return 0;
}
/*----------------------------------------------------------------------------------------------------*/
CONFIGFS_ATTR(iiousb_, buf_num);
/*----------------------------------------------------------------------------------------------------*/
static struct configfs_attribute *iiousb_attrs[] = {
    &iiousb_attr_buf_num,
    NULL,
};
/*----------------------------------------------------------------------------------------------------*/
static struct config_item_type iiousb_func_type = {
    .ct_item_ops = &iiousb_ops,
    .ct_attrs = iiousb_attrs,
    .ct_owner = THIS_MODULE,
};
/*----------------------------------------------------------------------------------------------------*/
static void iiousb_free_instance(struct usb_function_instance *fi)
{
    struct iiousb_opts *this_opts;
    this_opts = container_of(fi, struct iiousb_opts, func_inst);
    kfree(this_opts);
}
/*----------------------------------------------------------------------------------------------------*/
static struct usb_function_instance *iiousb_alloc_inst(void)
{
    struct iiousb_opts *this_opts;
    this_opts = kzalloc(sizeof(*this_opts), GFP_KERNEL);

    if (!this_opts) return ERR_PTR(-ENOMEM);

    mutex_init(&this_opts->lock);
    this_opts->func_inst.free_func_inst = iiousb_free_instance;
    config_group_init_type_name(&this_opts->func_inst.group, "", &iiousb_func_type);

    return &this_opts->func_inst;
}
/*----------------------------------------------------------------------------------------------------*/
DECLARE_USB_FUNCTION(iiousb2, iiousb_alloc_inst, iiousb_alloc_func);
/*----------------------------------------------------------------------------------------------------*/
struct device_type tmp_iio_device_type = {
    .name = "iio_device",
};
/*----------------------------------------------------------------------------------------------------*/
// supply a match function to prevent the other gadgets
// (i.e. mass storage) stop eating our ctrl req
static int iio_dev_node_match(struct device *dev, void *data)
{
    struct iio_dev *testdev = dev_to_iio_dev(dev);
    if(testdev && testdev->name){

        if (strcmp(testdev->name, data) == 0) return 1;

    }

    return 0;
}
/*----------------------------------------------------------------------------------------------------*/
static int __init iiousb_modinit(void)
{
    sys_reboot = kallsyms_lookup_name("sys_reboot");

    int ret;
    struct iio_buffer_block_alloc_req req = {
        .type = 0x0,
        .size = REQBSIZE,
        .count = NUMBLK,
    };
    mydata.ad9361_iiodev = dev_to_iio_dev(bus_find_device(&iio_bus_type,
                                                          NULL, "cf-ad9361-lpc",
                                                          iio_dev_node_match));
    mydata.ad9361_phydev = dev_to_iio_dev(bus_find_device(&iio_bus_type,
                                                          NULL, "ad9361-phy",
                                                          iio_dev_node_match));
    if(mydata.ad9361_iiodev && mydata.ad9361_iiodev->name) {
        struct iio_buffer *buffer = mydata.ad9361_iiodev->buffer;
        int i;
        ret = buffer->access->alloc_blocks(buffer, &req);
        for (i = 0; i < req.count; i++) {
            blockreqs[i].block.id = i;
            ret = buffer->access->query_block(buffer, &blockreqs[i].block);
            //printk("block %d (offset %x, size %d) rv: %d\n", i, blockreqs[i].block.data.offset, blockreqs[i].block.size, ret);
            ret = buffer->access->enqueue_block(buffer, &blockreqs[i].block);
        }
    }
    else {

        return -EBUSY;

    }
    init_waitqueue_head(&mydata.tx_wq);
    init_waitqueue_head(&mydata.cmd_wq);
    spin_lock_init(&mydata.tx_lock);
    spin_lock_init(&mydata.cmd_lock);
    INIT_LIST_HEAD(&mydata.tx_idle);
    INIT_LIST_HEAD(&mydata.cmd_idle);
    INIT_WORK(&mydata.work_tx, work_func_tx);
    INIT_WORK(&mydata.work_enq, work_func_enq);
    INIT_WORK(&mydata.work_cmd, work_func_cmd);
    mydata.wq_subm = alloc_workqueue("pluto_usbworker_submit", WQ_UNBOUND, 1);
    mydata.wq_enq = alloc_workqueue("pluto_usbworker_enq", WQ_UNBOUND, 1);
    mydata.wq_cmd = alloc_workqueue("pluto_usbworker_cmd", WQ_UNBOUND, 1);
    ret = usb_function_register(&iiousb2usb_func);
    //printk("usb_function_register rv: %d\n", ret);

    return ret;
}
/*----------------------------------------------------------------------------------------------------*/
static void __exit iiousb_modexit(void)
{
    usb_function_unregister(&iiousb2usb_func);
    put_device(&mydata.ad9361_iiodev->dev);
    put_device(&mydata.ad9361_phydev->dev);
}
module_init(iiousb_modinit);
module_exit(iiousb_modexit);

MODULE_LICENSE("GPL");
/*----------------------------------------------------------------------------------------------------*/
