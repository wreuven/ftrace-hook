#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netpoll.h>

MODULE_DESCRIPTION("Example module hooking open() and close() via ftrace");
MODULE_AUTHOR("ilammy <a.lozovsky@gmail.com>");
MODULE_LICENSE("GPL");

#include "udp.h"

static struct netpoll* np = NULL;
static struct netpoll np_t;

//see https://stackoverflow.com/questions/10499865/sending-udp-packets-from-the-linux-kernel
void init_netpoll(void)
{
    np_t.name = "LRNG";
    strlcpy(np_t.dev_name, "eth0", IFNAMSIZ);
    np_t.local_ip.ip = htonl((unsigned long int)0x7f000001); //127.0.0.1
    np_t.local_ip.in.s_addr = htonl((unsigned long int)0x7f000001); //192.168.56.2
    np_t.remote_ip.ip = htonl((unsigned long int)0x7f000001); //127.0.0.1
    np_t.remote_ip.in.s_addr = htonl((unsigned long int)0x7f000001); //192.168.56.1
    np_t.ipv6 = 0;//no IPv6
    np_t.local_port = 6666;
    np_t.remote_port = 5555;
    memset(np_t.remote_mac, 0xff, ETH_ALEN);
    netpoll_print_options(&np_t);
    netpoll_setup(&np_t);
    np = &np_t;
}

void clean_netpoll(void)
{
  //nothing
}

void udp_send(const char* buf)
{
    int len = strlen(buf);
    netpoll_send_udp(np,buf,len);
}

int udp_initmod(void)
{
        printk(KERN_INFO "Hello world :)\n");

        init_netpoll();
        udp_send("[55] testestestestestestestestest");
        udp_send("<55>Mar 17 21:57:57 frodo sshd[701]: blub");


        //0 = module loaded
        return 0;
}

void udp_cleanmod(void)
{
        printk(KERN_INFO "Goodbye world :(\n");
}



