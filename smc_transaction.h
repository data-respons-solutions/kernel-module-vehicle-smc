#ifndef SMC_TRANSACTION_H_
#define SMC_TRANSACTION_H_

#include <linux/i2c.h>

int smc_send(struct i2c_client *client, u8 *outbuf, int outbuf_size);
int smc_recv(struct i2c_client *client);
int smc_transaction(struct i2c_client *client, u8 *outbuf, int outbuf_size, u8 *result, int result_size);

#endif // SMC_TRANSACTION_H_
