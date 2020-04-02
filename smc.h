#ifndef SMC_H_
#define SMC_H_

#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/rtc.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>

#include "smc_proto.h"

/*
 * Defines
 */

#define xstr(s) str(s)
#define str(s) #s

#define VEC6200_SMC_LED_MAX_NAME_LEN	30
#define MAX_GPIOS 6

struct smc_led {
	char name[VEC6200_SMC_LED_MAX_NAME_LEN + 1]; /* name of the LED in sysfs */
	struct led_classdev led_dev; /* led classdev */
	int led_id;
	int registered;
};

struct smc_data {
	struct i2c_client *client;
	struct mutex transaction_mutex;

	struct smc_led led_status_green; /* led classdev - Status LED Green */
	struct smc_led led_status_red; /* led classdev - Status LED Led */
	int irq;
	bool is_ready;
	struct gpio_chip gpio_ctl;
	struct gpio_desc *gpio_descs[MAX_GPIOS];
	u8 gpio_is_input[MAX_GPIOS];
	//struct device *gpio_dev[MAX_GPIOS];

	wait_queue_head_t readq; /* used by write to wake blk.read */
	MpuVersionHeader_t version;
	MpuVersionHeader_t driver_version;

	struct rtc_device *rtc;
	struct work_struct alert_work;
	struct workqueue_struct *wq;
	u8 *rx_buffer;
	int rx_result;
	u8 *transaction_read_buffer;
	int debug_mode;
	int read_pending;
	SensorMsg_t sensor;
	unsigned long sensor_jiffies;
	PowerSupplyMsg_t powersupplies;
	struct power_supply *psy;
	struct power_supply *psy_gpo1;
	struct power_supply *psy_gpo2;
	struct power_supply *psy_dcout1;
	struct power_supply *psy_dcout2;
	unsigned long psy_jiffies;
	InitEventType_t start_cause;
	u32 alarm_in_seconds;
	int (*set_alarm)(struct smc_data *);
};

#endif // SMC_H_
