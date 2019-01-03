#include <linux/version.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <net/inet_common.h>
#include <net/sock.h>

#define MAX_UDP_SIZE 65507

// Handle UDP connection and socket status.

void construct_header(struct msghdr * msg, struct sockaddr_in * address){
  msg->msg_name    = address;
  msg->msg_namelen = sizeof(struct sockaddr_in);
  msg->msg_control = NULL;
  msg->msg_controllen = 0;
  msg->msg_flags = 0; // this is set after receiving a message
}


u32 create_address(u8 *ip){
  u32 addr = 0;
  int i;

  for(i=0; i<4; i++)
  {
    addr += ip[i];
    if(i==3)
    break;
    addr <<= 8;
  }
  return addr;
}


// returns how many bytes are sent
int _udp_send(struct socket *sock, struct msghdr * header, void *buff, size_t size_buff) {

  struct kvec vec;
  int sent, size_pkt, totbytes = 0;
  long long buffer_size = size_buff;
  char * buf = (char *) buff;

  mm_segment_t oldmm;

  while(buffer_size > 0){
    if(buffer_size < MAX_UDP_SIZE){
      size_pkt = buffer_size;
    }else{
      size_pkt = MAX_UDP_SIZE;
    }

    vec.iov_len = size_pkt;
    vec.iov_base = buf;

    buffer_size -= size_pkt;
    buf += size_pkt;

    oldmm = get_fs(); set_fs(KERNEL_DS);
    sent = kernel_sendmsg(sock, header, &vec, 1, size_pkt);
    set_fs(oldmm);

    totbytes+=sent;
  }

  return totbytes;
}


int udp_init(struct socket ** s, unsigned char * myip, int myport){
  int err;
  struct socket * conn_sock;
  struct sockaddr_in address;
  mm_segment_t fs;
  struct timeval tv = {0,100000};
  int flag = 1;

  #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
    err = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
  #else
    err = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, s);
  #endif

  if(err < 0){
    printk(KERN_ERR "Error %d while creating socket\n", err);
    *s = NULL;
    return err;
  }
  conn_sock = *s;

  printk(KERN_INFO "Socket created\n");

  fs = get_fs();
  set_fs(KERNEL_DS);
  kernel_setsockopt(conn_sock, SOL_SOCKET, SO_RCVTIMEO , (char * )&tv, sizeof(tv));
  kernel_setsockopt(conn_sock, SOL_SOCKET, SO_REUSEADDR , (char * )&flag, sizeof(int));
  kernel_setsockopt(conn_sock, SOL_SOCKET, SO_REUSEPORT , (char * )&flag, sizeof(int));
  set_fs(fs);

  address.sin_addr.s_addr = htonl(create_address(myip));
  address.sin_family = AF_INET;
  address.sin_port = htons(myport);

  err = conn_sock->ops->bind(conn_sock, (struct sockaddr*)&address, sizeof(address));
  if(err < 0) {
    printk(KERN_ERR "Error %d while binding socket to %pI4\n", err, &address.sin_addr);
    sock_release(conn_sock);
    *s = NULL;
    return err;
  }else{
    // get the actual random port and update myport
    int i = (int) sizeof(struct sockaddr_in);
    inet_getname(conn_sock, (struct sockaddr *) &address, &i , 0);
    printk(KERN_INFO "Socket is bind to %pI4:%d\n", &address.sin_addr, ntohs(address.sin_port));
  }
  return 0;
}

void release_socket(struct socket * s){
  if(s){
    sock_release(s);
  }
}

void
fill_sockaddr_in(struct sockaddr_in *buffer, u_long addr, u_short port)
{
	memset(buffer, 0, sizeof *buffer);
	buffer->sin_family = AF_INET;
	buffer->sin_addr.s_addr = addr;
	buffer->sin_port = port;
}

struct socket 	    *s;
struct sockaddr_in  dest_addr;
struct msghdr 	    hdr;

void udp_send(const char* buf) 
{
  if (s) {
    _udp_send(s, &hdr, (void *)buf, strlen(buf));
  } 
}

int udp_initmod(void)
{
  static unsigned char ip[5] = {127,0,0,1,'\0'};
  static unsigned char serverip[5] = {127,0,0,1,'\0'};
  static int destport = 4000;

  int ret = udp_init(&s, ip, 1111);

  dest_addr.sin_addr.s_addr = htonl(create_address(serverip));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(destport);

  construct_header(&hdr, &dest_addr);
  return ret;
}

void udp_cleanmod(void)
{
  release_socket(s);
}
