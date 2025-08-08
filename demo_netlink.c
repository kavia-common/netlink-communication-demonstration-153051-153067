#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>

/*
 * This demo program illustrates how to create a Netlink socket in user space,
 * send a message to the kernel, and receive a reply using NETLINK_USERSOCK.
 *
 * NOTE:
 * - The kernel will only reply if a kernel module or kernelspace handler
 *   is registered for the corresponding message, otherwise no response is received.
 * - This code works as a demonstration for sending/receiving via Netlink in user space.
 * - Compile with: gcc -o demo_netlink demo_netlink.c
 * - Run as: ./demo_netlink
 */

#define NETLINK_USER 31 // NETLINK_USERSOCK; linux/netlink.h's 31 is for user-defined usages
#define USER_MSG    "Hello Kernel, this is User Space!"
#define MAX_PAYLOAD 1024  // maximum payload size

int main() {
    int sock_fd;
    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;
    char buffer[MAX_PAYLOAD];

    // 1. Create a netlink socket
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        perror("socket()");
        return -1;
    }

    // 2. Fill in source address (this process's address)
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); // unique PID per process
    src_addr.nl_groups = 0;     // not in multicast group

    // 3. Bind the socket to the source address
    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind()");
        close(sock_fd);
        return -1;
    }

    // 4. Prepare destination address (to kernel)
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;   // Kernel is 0
    dest_addr.nl_groups = 0; // unicast

    // 5. Allocate Netlink message header and fill payload
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    if (!nlh) {
        fprintf(stderr, "malloc() failed\n");
        close(sock_fd);
        return -1;
    }
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(strlen(USER_MSG) + 1);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    strcpy(NLMSG_DATA(nlh), USER_MSG);

    // 6. Set up a generic IOV and msghdr structure
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("User: Sending message to kernel: \"%s\"\n", (char *)NLMSG_DATA(nlh));

    // 7. Send message to kernel
    if (sendmsg(sock_fd, &msg, 0) < 0) {
        perror("sendmsg()");
        free(nlh);
        close(sock_fd);
        return -1;
    }

    // 8. Prepare to receive reply
    memset(buffer, 0, sizeof(buffer));
    nlh = (struct nlmsghdr *)buffer;

    printf("User: Waiting for message from kernel...\n");

    iov.iov_base = (void *)nlh;
    iov.iov_len = MAX_PAYLOAD;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    // 9. Try to receive a message (will block until a response is received)
    ssize_t recv_len = recvmsg(sock_fd, &msg, 0);
    if (recv_len < 0) {
        perror("recvmsg() (Did you load a kernel module to answer this netlink message?)");
        // Not always failure: likely no kernel handler for this NETLINK_USERSOCK msg
        free(nlh);
        close(sock_fd);
        return -1;
    }

    printf("User: Received message payload: \"%s\"\n", (char *)NLMSG_DATA(nlh));

    // 10. Cleanup
    free(nlh);
    close(sock_fd);

    printf("User: Done.\n");
    return 0;
}

/*
 * INSTRUCTIONS TO RUN THE DEMO
 * ----------------------------
 * 1. Compile:   gcc -o demo_netlink demo_netlink.c
 * 2. Run:       ./demo_netlink
 * 
 * NOTE:
 * - By default, unless a custom kernel module or handler is present for NETLINK_USERSOCK (protocol 31),
 *   you will not receive a response from the kernel, and the receive will block or fail.
 * - To see a full round-trip, you need a kernelspace component listening and replying
 *   on NETLINK_USERSOCK messages (such as a test kernel module).
 * - Even when no kernel reply is present, the code demonstrates the full user space pattern for netlink communication.
 * 
 * REFERENCES:
 *  - man 7 netlink
 *  - https://lwn.net/Articles/57369/
 */
