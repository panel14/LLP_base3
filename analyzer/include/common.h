#ifndef LLP_BASE3_COMMON_H
#define LLP_BASE3_COMMON_H

#include "nanopb/pb.h"

pb_ostream_t pbOstreamFromSocket(int fd);
pb_istream_t pbIstreamFromSocket(int fd);

#endif
