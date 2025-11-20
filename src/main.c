#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <uwb_fira/fira_app.h>
#include <uwb_fira/uci_backend.h>

LOG_MODULE_REGISTER(uwb_tag, CONFIG_LOG_DEFAULT_LEVEL);

#define TAG_ADDRESS 0x01
#define RANGING_INTERVAL_MS 100
#define SLOT_DURATION_RSTU 2400
#define MAX_RETRIES 3
#define RETRY_DELAY_MS 50

struct session_setup {
	uint8_t session_id;
	uint8_t controller_short_address;
	uint8_t channel;
};

static const struct session_setup session_presets[] = {
	{ .session_id = 1, .controller_short_address = 0x10, .channel = 9 },
	{ .session_id = 2, .controller_short_address = 0x11, .channel = 9 },
	{ .session_id = 3, .controller_short_address = 0x12, .channel = 9 },
};

static int configure_session(const struct session_setup *setup)
{
	struct fira_app_config config = {
		.session_id = setup->session_id,
		.role = FIRA_DEVICE_ROLE_CONTROLEE,
		.device_short_address = TAG_ADDRESS,
		.controller_short_address = setup->controller_short_address,
		.channel_number = setup->channel,
		.ranging_interval_ms = RANGING_INTERVAL_MS,
		.slot_duration_rstu = SLOT_DURATION_RSTU,
		.multi_node_mode = FIRA_MULTI_NODE_MODE_UNICAST,
	};

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		int err = fira_app_configure(&config);
		if (!err) {
			return 0;
		}

		LOG_WRN("Session %u configuration failed (attempt %d/%d): %d",
			config.session_id, attempt + 1, MAX_RETRIES, err);
		k_msleep(RETRY_DELAY_MS);
	}

	return -EIO;
}

static int start_session(uint8_t session_id)
{
	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		int err = fira_app_start_session(session_id);
		if (!err) {
			return 0;
		}

		LOG_WRN("Session %u start failed (attempt %d/%d): %d",
			session_id, attempt + 1, MAX_RETRIES, err);
		k_msleep(RETRY_DELAY_MS);
	}

	return -EIO;
}

static int init_stack(void)
{
	int err = uci_backend_init();
	if (err) {
		LOG_ERR("UCI backend init failed: %d", err);
		return err;
	}

	err = fira_app_init();
	if (err) {
		LOG_ERR("FiRa app init failed: %d", err);
		return err;
	}

	return 0;
}

void main(void)
{
	LOG_INF("Starting DWM3001CDK UWB tag (auto sessions)");

	if (init_stack() != 0) {
		LOG_ERR("Stack initialization failed; retrying in background");
		while (init_stack() != 0) {
			k_msleep(500);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(session_presets); ++i) {
		const struct session_setup *setup = &session_presets[i];
		int err = configure_session(setup);
		if (err) {
			LOG_ERR("Could not configure session %u", setup->session_id);
			continue;
		}

		err = start_session(setup->session_id);
		if (err) {
			LOG_ERR("Could not start session %u", setup->session_id);
		}
	}

	while (true) {
		/* Keep the tag alive; metrics or callbacks would be handled elsewhere. */
		k_msleep(1000);
	}
}
