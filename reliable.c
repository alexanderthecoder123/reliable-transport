#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "rlib.h"
#include "buffer.h"

struct reliable_state {
    rel_t *next;			/* Linked list for traversing all connections */
    rel_t **prev;

    conn_t *c;			
    // ...
    buffer_t* send_buffer;
    // ...
    buffer_t* rec_buffer;
    // ...
    int maximal_pot_window;
    int send_una;
    int send_next;
    int recv_next;
    int time_out;

    int end_of_file_recv;
    int end_of_file_read;
};
rel_t *rel_list;

long obtain_time(){
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec/1000;
}


rel_t *
rel_create (conn_t *c, const struct sockaddr_storage *ss,
const struct config_common *cc)
{
    rel_t *r;

    r = xmalloc (sizeof (*r));
    memset (r, 0, sizeof (*r));

    if (!c) {
        c = conn_create (r, ss);
        if (!c) {
            free (r);
            return NULL;
        }
    }

    r->maximal_pot_window = cc->window;
    r->send_una = 0;
    r->send_next = 1;
    r->recv_next = 1;
    r->time_out = cc->timeout;

    r->end_of_file_recv = 0;
    r->end_of_file_read = 0;

    r->c = c;
    r->next = rel_list;
    r->prev = &rel_list;
    if (rel_list)
    rel_list->prev = &r->next;
    rel_list = r;

    /* Do any other initialization you need here... */
    // ...
    r->send_buffer = xmalloc(sizeof(buffer_t));
    r->send_buffer->head = NULL;
    // ...
    r->rec_buffer = xmalloc(sizeof(buffer_t));
    r->rec_buffer->head = NULL;
    // ...

    return r;
}

void
rel_destroy (rel_t *r)
{
    if (r->next) {
        r->next->prev = r->prev;
    }
    *r->prev = r->next;
    conn_destroy (r->c);

    /* Free any other allocated memory here */
    buffer_clear(r->send_buffer);
    free(r->send_buffer);
    buffer_clear(r->rec_buffer);
    free(r->rec_buffer);
    // ...
   
}



void
rel_recvpkt (rel_t *r, packet_t *pkt, size_t n)
{
    if(ntohs(pkt->len) != n){
        return;
    }
    int check_sum_in_packet = pkt->cksum;
    pkt->cksum = 0;
    int actual_check_sum = cksum(pkt, ntohs(pkt->len));
    if(check_sum_in_packet != actual_check_sum) {
        return;
    }

}    

void
rel_read (rel_t *s)
{
    while(s->send_next - s->send_una <= s->maximal_pot_window){
        packet_t p = {};
        int payload = conn_input(s->c, p.data, 500);
        if(payload <= 0){
            return;
        } else {
            p.len = htons(payload + 12);
            p.ackno = 0;
            p.seqno = htonl(s->send_next);
            p.cksum = cksum(&p, payload + 12);
            buffer_insert(s->send_buffer,&p,obtain_time());
            s->send_next = (s->send_next) + 1;
            conn_sendpkt(s->c,&p, payload + 12);
            

        }
    }
}

void
rel_output (rel_t *r)
{
}

void
rel_timer ()
{
}

