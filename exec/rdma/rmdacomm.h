/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _RMDACOMM_H_RPCGEN
#define _RMDACOMM_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


struct data_arg {
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct data_arg data_arg;

struct data_req {
	char c;
	u_int len;
};
typedef struct data_req data_req;



#define OSDPROG 0x30090941
#define OSDVERS 1

#if defined(__STDC__) || defined(__cplusplus)
#define BLOCK_WRITE 1
extern  int * block_write_1(data_arg *, CLIENT *);
extern  int * block_write_1_svc(data_arg *, struct svc_req *);
#define BLOCK_READ 2
extern  data_arg * block_read_1(data_req *, CLIENT *);
extern  data_arg * block_read_1_svc(data_req *, struct svc_req *);
extern int osdprog_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define BLOCK_WRITE 1
extern  int * block_write_1();
extern  int * block_write_1_svc();
#define BLOCK_READ 2
extern  data_arg * block_read_1();
extern  data_arg * block_read_1_svc();
extern int osdprog_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_data_arg (XDR *, data_arg*);
extern  bool_t xdr_data_req (XDR *, data_req*);
extern  bool_t xdr_data_arg (XDR *, data_arg*);
extern  bool_t xdr_data_req (XDR *, data_req*);

#else /* K&R C */
extern bool_t xdr_data_arg ();
extern bool_t xdr_data_req ();
extern bool_t xdr_data_arg ();
extern bool_t xdr_data_req ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_RMDACOMM_H_RPCGEN */
