/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @brief Model handler for the light switch.
 *
 * Instantiates a Generic OnOff Client model for each button on the devkit, as
 * well as the standard Config and Health Server models. Handles all application
 * behavior related to the models.
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh/proxy.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

/* zsd */
#include <string.h>
#include <bluetooth/mesh/gen_onoff_cli.h>
#include "input_data.h"

extern void mymodel();
extern float Value[4];
extern void model_transition_buf_add(struct net_buf_simple *buf,
			 const struct bt_mesh_model_transition *transition);

extern int bt_mesh_msg_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf);


extern void spi_data_get();
extern uint8_t raw_data[3072];


int debugbuf(uint8_t* buf,int size);

uint8_t classification_result();
uint8_t Fault_Number;
int transition = 0;
#define THE_PT_ID	0X01

static void lpn_send();

int bt_mesh_onoff_cli_set_unack_lpn(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set);

int bt_mesh_onoff_cli_set_unack_test(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set);

int bt_mesh_msg_send_lpn(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf);

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh);

/* to deal with retry */
static struct k_work send_work;
static bool work_queued = false;

static void send_message(struct k_work *work)
{
    //spi_data_get();
	//k_msleep(1); 
	//mymodel();
	//k_msleep(1000);  
	//Fault_Number = classification_result();
	Fault_Number = 1;
    lpn_send();
	//work_queued = false;
	printk("send_message\n");
}

/* Light switch behavior */

/** Context for a single light switch. */
struct button {
	/** Current light status of the corresponding server. */
	bool status;
	/** Generic OnOff client instance for this switch. */
	struct bt_mesh_onoff_cli client;
};

static void status_handler(struct bt_mesh_onoff_cli *cli,
			   struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_onoff_status *status);

static struct button buttons[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3)) && !defined(CONFIG_BT_MESH_LOW_POWER)
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif
};
/* zsd */
static void status_handler(struct bt_mesh_onoff_cli *cli,
			   struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_onoff_status *status)
{
/* 	struct button *button =
		CONTAINER_OF(cli, struct button, client);
	int index = button - &buttons[0];

	button->status = status->present_on_off;
	dk_set_led(index, status->present_on_off);

	printk("Button %d: Received response: %s\n", index + 1,
	       status->present_on_off ? "on" : "off"); */
	//spi_data_get();
	//k_msleep(1); 
	//mymodel();
	//k_msleep(1000);  
	//Fault_Number = classification_result();
	transition++;
	if (transition % 2 != 0) {
        k_work_init(&send_work, send_message);
        k_work_submit(&send_work);
    }
	LOG_DBG("the number of transition is %d\n",transition);
	LOG_DBG("recv_dst is %X\n",ctx->recv_dst);
	LOG_DBG("addr is %X\n",ctx->addr);
	LOG_DBG("TTL is %X\n",ctx->recv_ttl);
	Fault_Number = 1;
	//err = bt_mesh_onoff_cli_set_unack_lpn(&buttons[i].client,NULL, &set);//zsd
	//printk("present status %d and target status %d\n",status->present_on_off,status->target_on_off);
	//lpn_send();
	//printk("the  model_init_flag is :%d\n",model_init_flag);
	//int err;
	//err = bt_mesh_onoff_cli_set_unack_lpn(&buttons[0].client,NULL, NULL);

}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) && (pressed & changed & BIT(3))) {
		bt_mesh_proxy_identity_enable();
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(buttons); ++i) {
		if (!(pressed & changed & BIT(i))) {
			continue;
		}

		struct bt_mesh_onoff_set set = {
			.on_off = !buttons[i].status,
			
		};
		int err;

		/* As we can't know how many nodes are in a group, it doesn't
		 * make sense to send acknowledged messages to group addresses -
		 * we won't be able to make use of the responses anyway. This also
		 * applies in LPN mode, since we can't expect to receive a response
		 * in appropriate time.
		 */
		if (bt_mesh_model_pub_is_unicast(buttons[i].client.model) &&
		    !IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
			err = bt_mesh_onoff_cli_set(&buttons[i].client, NULL, &set, NULL);
		} else {
			err = bt_mesh_onoff_cli_set_unack(&buttons[i].client,
							  NULL, &set);//zsd
			if (!err) {
				/* There'll be no response status for the
				 * unacked message. Set the state immediately.
				 */
				buttons[i].status = set.on_off;
				dk_set_led(i, set.on_off);
			}
		}

		if (err) {
			printk("OnOff %d set failed: %d\n", i + 1, err);
		}
	}
}
/* zsd */
/* send */
int bt_mesh_onoff_cli_set_unack_lpn(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set)
{

	cli->tid++;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_SET_UNACK,
				 BT_MESH_ONOFF_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_SET_UNACK);

	net_buf_simple_add_u8(&msg,THE_PT_ID);
	net_buf_simple_add_u8(&msg,Fault_Number);
	net_buf_simple_add_u8(&msg,cli->tid);
	LOG_DBG("sending time for message %d from PT %d",cli->tid,THE_PT_ID);
	//return bt_mesh_model_send()
	return bt_mesh_msg_send(cli->model, ctx, &msg);

}

/* zsd */
static void lpn_send()
{
	
	/* struct bt_mesh_onoff_set set = {
		.on_off = !buttons[i].status,	
	}; */
	int err;
	/* As we can't know how many nodes are in a group, it doesn't
		 * make sense to send acknowledged messages to group addresses -
		 * we won't be able to make use of the responses anyway. This also
		 * applies in LPN mode, since we can't expect to receive a response
		 * in appropriate time.
	 */
	err = bt_mesh_onoff_cli_set_unack_test(&buttons[0].client,
							NULL, NULL);//zsd
	if (err) {
			printk("OnOff %d set failed: %d\n", 0 + 1, err);
	}
	
}

int bt_mesh_onoff_cli_set_unack_test(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set)
{
	cli->tid++;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_SET_UNACK,
				 BT_MESH_ONOFF_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_SET_UNACK);

	net_buf_simple_add_u8(&msg, THE_PT_ID);
	net_buf_simple_add_u8(&msg, Fault_Number);
	debugbuf(&msg,4);
	/* if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	} */

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_msg_send_lpn(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	if (!ctx && !model->pub) {
		return -ENOTSUP;
	}

	if (ctx) {
		return bt_mesh_model_send(model, ctx, buf, NULL, 0);
	}

}

/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
		BIT(0),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
		BIT(1),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
		BIT(2),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3))
		BIT(3),
#endif
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_elem elements[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_CFG_SRV,
			     BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
			     BT_MESH_MODEL_ONOFF_CLI(&buttons[0].client)),
		     BT_MESH_MODEL_NONE),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
	BT_MESH_ELEM(2,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_ONOFF_CLI(&buttons[1].client)),
		     BT_MESH_MODEL_NONE),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
	BT_MESH_ELEM(3,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_ONOFF_CLI(&buttons[2].client)),
		     BT_MESH_MODEL_NONE),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3)) && !defined(CONFIG_BT_MESH_LOW_POWER)
	BT_MESH_ELEM(4,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_ONOFF_CLI(&buttons[3].client)),
		     BT_MESH_MODEL_NONE),
#endif

};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};

	dk_button_handler_add(&button_handler);
	k_work_init_delayable(&attention_blink_work, attention_blink);

	return &comp;
}

/* zsd */

uint8_t classification_result()
{
    int i;
    float max = Value[0];
    uint8_t max_index = 1;
    
    for ( i = 0; i < 4; i++)
    {
        if (Value[i] > max)
        {
            max = Value[i];
            max_index = i+1;
        }
    }
    if (max_index == 1)
    {
        printk("\nthe classification result is: normal\n");
    }
    else if (max_index == 2)
    {
        printk("\nthe classification result is: OR fault 2\n");
    }
    else if (max_index == 3)
    {
        printk("\nthe classification result is: ball fault 3\n");
    }
    else if (max_index == 4)
    {
        printk("\nthe classification result is: IR fault 4\n");
    }

    return max_index;
}


int debugbuf(uint8_t* buf,int size)
{
    int i = 0;
    for (i=0;i<size;i++)
    {
        printk("%02X",buf[i]&0xff);

    }
    printk("\n");
    return 0;
}