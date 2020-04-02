#include <linux/i2c.h>

#include "smc.h"
#include "smc_transaction.h"
#include "smc_proto.h"

int smc_send(struct i2c_client *client, u8 *outbuf, int outbuf_size)
{
	int ret = 0;
	struct i2c_msg msg_tx[1];
	struct smc_data *data = i2c_get_clientdata(client);

	outbuf[outbuf_size] = mpu_compute_checksum(outbuf, outbuf_size);
	msg_tx[0].addr = client->addr; 		  // 7-bit address
	msg_tx[0].flags = 0;          // Write transaction, beginning with START
	msg_tx[0].len = outbuf_size + 1;  // Send one byte following the address
	msg_tx[0].buf = outbuf;                // Transfer from this address

	data->is_ready = false;

	dev_dbg(&client->dev, "%s: msg %d, sz %d\n", __func__, outbuf[0],
		outbuf_size);
	ret = i2c_transfer(client->adapter, msg_tx, 1);
	if (ret < 1) {
		dev_err(&client->dev, "i2c_transfer: [tx] , ret = %d\n", ret);
		return ret;
	}
	return 0;
}

int smc_recv(struct i2c_client *client)
{
	int ret = 0;
	struct smc_data *data = i2c_get_clientdata(client);
	struct i2c_msg msg_rx[1];
	int rx_len = 0;
	char buf[I2C_SMBUS_BLOCK_MAX + 4];

	/* The data will get returned in this structure */
	msg_rx[0].addr = client->addr;
	msg_rx[0].flags = I2C_M_RD | I2C_M_RECV_LEN;
	msg_rx[0].len = 1;
	msg_rx[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg_rx, 1);
	if (ret <= 0) {
		dev_err(&client->dev, "i2c_transfer: [rx], ret = %d\n", ret);
		return ret;
	}
	rx_len = buf[0];
	dev_dbg(&client->dev, "%s: RX len is %d\n", __func__, rx_len);
	if (rx_len > (I2C_SMBUS_BLOCK_MAX)) {
		dev_warn(&client->dev, "i2c_transfer [rx] encoded len=%d\n",
			rx_len);
		return -EIO;
	}
	if (buf[rx_len] != mpu_compute_checksum(buf + 1, rx_len - 1)) {
		dev_warn(&client->dev,
			"i2c rcv msg[%d] checksum error, len=%d\n", buf[1],
			rx_len - 1);
		return -EINVAL;
	}

	memcpy(data->rx_buffer, buf + 1, rx_len - 1);
	return rx_len - 1;
}

int smc_transaction(struct i2c_client *client, u8 *outbuf, int outbuf_size, u8 *result, int result_size)
{
	int ret;
	int retries = 2;
	MpuMsgHeader_t hdr_in, hdr_out;
	struct smc_data *data = i2c_get_clientdata(client);
	ret = mutex_lock_interruptible(&data->transaction_mutex);
	if (ret < 0)
		return ret;

	if (result_size > (I2C_SMBUS_BLOCK_MAX + 1)) {
		dev_err(&client->dev,
			"Transactation read size of %d to big, max %d\n",
			result_size, I2C_SMBUS_BLOCK_MAX + 1);
		mutex_unlock(&data->transaction_mutex);
		return -EINVAL;
	}
	hdr_in = mpu_message_header(outbuf);
	data->transaction_read_buffer = result;
	do {
		data->read_pending = 1;

		ret = smc_send(client, outbuf, outbuf_size);

		if (ret < 0) {
			data->read_pending = 0;
			continue;
		}

		ret = wait_event_interruptible_timeout(data->readq,
			data->is_ready, msecs_to_jiffies(2000));

		if (ret <= 0) {
			data->read_pending = 0;
			dev_warn(&client->dev,
				"Timeout waiting for command reply\n");
			if (ret == 0)
				ret = -ETIMEDOUT;
			continue;
		}
		if (data->rx_result < 0) {
			dev_warn(&client->dev, "%s: Rx Error %d\n", __func__,
				data->rx_result);
			ret = data->rx_result;
			continue;
		}
		hdr_out = mpu_message_header(result);
		if (hdr_in.type != hdr_out.type) {
			dev_err(&client->dev,
				"Msg in type %d differs from reply %d\n",
				hdr_in.type, hdr_out.type);
			ret = -EINVAL;
		}
	} while (ret && retries--);
	mutex_unlock(&data->transaction_mutex);
	return ret >= 0 ? 0 : ret;
}
