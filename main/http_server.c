/*
	 HTTP Server Example

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_http_server.h"
#include "cJSON.h"

#include "parameter.h"

static const char *TAG = "HTTP";

extern QueueHandle_t xQueueParameter;

#define SCRATCH_BUFSIZE (1024)

typedef struct rest_server_context {
	char base_path[ESP_VFS_PATH_MAX + 1]; // Not used in this project
	char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;


/* Handler for root get */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	ESP_LOGD(__FUNCTION__, "root_get_handler req->uri=[%s]", req->uri);

	/* Send empty chunk to signal HTTP response completion */
	httpd_resp_sendstr_chunk(req, NULL);

	return ESP_OK;
}

/* Handler for control */
static esp_err_t control_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	bool parse = true;
	cJSON *root = cJSON_Parse(buf);

	// Search lcd item
	char wk[32];
	bool lcd = false;
	cJSON* state = cJSON_GetObjectItem(root, "lcd");
	if (state) {
		strcpy(wk, cJSON_GetObjectItem(root, "lcd")->valuestring);
		if (strcmp (wk, "on") == 0) {
			lcd = true;
		} else if (strcmp (wk, "off") == 0) {
			lcd = false;
		} else {
			ESP_LOGW(__FUNCTION__, "lcd is not on/off, so off is applied");
			lcd = false;
		}
	} else {
		ESP_LOGE(__FUNCTION__, "lcd item not found");
		parse = false;
	}

    // Search cursor item
    bool cursor = false;
    state = cJSON_GetObjectItem(root, "cursor");
    if (state) {
        strcpy(wk, cJSON_GetObjectItem(root, "cursor")->valuestring);
        if (strcmp (wk, "on") == 0) {
            cursor = true;
        } else if (strcmp (wk, "off") == 0) {
            cursor = false;
        } else {
            ESP_LOGW(__FUNCTION__, "cursor is not on/off, so off is applied");
            cursor = false;
        }
    } else {
        ESP_LOGE(__FUNCTION__, "cursor item not found");
        parse = false;
    }

    // Search blink item
    bool blink = false;
    state = cJSON_GetObjectItem(root, "blink");
    if (state) {
        strcpy(wk, cJSON_GetObjectItem(root, "blink")->valuestring);
        if (strcmp (wk, "on") == 0) {
            blink = true;
        } else if (strcmp (wk, "off") == 0) {
            blink = false;
        } else {
            ESP_LOGW(__FUNCTION__, "blink is not on/off, so off is applied");
            blink = false;
        }
    } else {
        ESP_LOGE(__FUNCTION__, "blink item not found");
        parse = false;
    }

	cJSON_Delete(root);
	if (parse) {
		ESP_LOGI(__FUNCTION__, "lcd=%d cursor=%d blink=%d", lcd, cursor, blink);
		httpd_resp_sendstr(req, "control successfully\n");
        PARAMETER_t paramBuf;
        strcpy(paramBuf.function, "control");
        paramBuf.lcd = lcd;
        paramBuf.cursor = cursor;
        paramBuf.blink = blink;
        if (xQueueSend(xQueueParameter, &paramBuf, 10) != pdPASS) {
            ESP_LOGE(__FUNCTION__, "xQueueSend fail");
        }

	} else {
		ESP_LOGE(__FUNCTION__, "Request parameter not correct [%s]", buf);
		httpd_resp_sendstr(req, "Request parameter not correct");
	}

	return ESP_OK;
}

/* Handler for clear */
static esp_err_t clear_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	httpd_resp_sendstr(req, "clear successfully\n");

	PARAMETER_t paramBuf;
	strcpy(paramBuf.function, "clear");
	if (xQueueSend(xQueueParameter, &paramBuf, 10) != pdPASS) {
		ESP_LOGE(__FUNCTION__, "xQueueSend fail");
	}
	return ESP_OK;
}

/* Handler for gotoxy */
static esp_err_t gotoxy_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	bool parse = true;
	cJSON *root = cJSON_Parse(buf);

	// Search col item
	uint8_t col = 0;
	cJSON* state = cJSON_GetObjectItem(root, "col");
	if (state) {
		col = (uint8_t)cJSON_GetObjectItem(root, "col")->valueint;
	} else {
		ESP_LOGE(__FUNCTION__, "col item not found");
		parse = false;
	}

	// Search line item
	uint8_t line = 0;
	state = cJSON_GetObjectItem(root, "line");
	if (state) {
		line = (uint8_t)cJSON_GetObjectItem(root, "line")->valueint;
	} else {
		ESP_LOGE(__FUNCTION__, "line item not found");
		parse = false;
	}

	cJSON_Delete(root);
	if (parse) {
		ESP_LOGI(__FUNCTION__, "col=%d line=%d", col, line);
		httpd_resp_sendstr(req, "gotoxy successfully\n");
		PARAMETER_t paramBuf;
		strcpy(paramBuf.function, "gotoxy");
		paramBuf.col = col;
		paramBuf.line = line;
		if (xQueueSend(xQueueParameter, &paramBuf, 10) != pdPASS) {
			ESP_LOGE(__FUNCTION__, "xQueueSend fail");
		}
	} else {
		ESP_LOGE(__FUNCTION__, "Request parameter not correct [%s]", buf);
		httpd_resp_sendstr(req, "Request parameter not correct");
	}

	return ESP_OK;
}

/* Handler for putc */
static esp_err_t putc_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	bool parse = true;
	cJSON *root = cJSON_Parse(buf);

	// Search char item
	char wk[32];
	char ch = 0;
	cJSON* state = cJSON_GetObjectItem(root, "char");
	if (state) {
		strcpy(wk, cJSON_GetObjectItem(root, "char")->valuestring);
		ESP_LOGI(__FUNCTION__, "wk=[%s]", wk);
		ch = wk[0];
	} else {
		ESP_LOGE(__FUNCTION__, "char item not found");
		parse = false;
	}

	cJSON_Delete(root);
	if (parse) {
		ESP_LOGI(__FUNCTION__, "ch=[%c]", ch);
		httpd_resp_sendstr(req, "putc successfully\n");
		PARAMETER_t paramBuf;
		strcpy(paramBuf.function, "putc");
		paramBuf.ch = ch;
		if (xQueueSend(xQueueParameter, &paramBuf, 10) != pdPASS) {
			ESP_LOGE(__FUNCTION__, "xQueueSend fail");
		}
	} else {
		ESP_LOGE(__FUNCTION__, "Request parameter not correct [%s]", buf);
		httpd_resp_sendstr(req, "Request parameter not correct");
	}

	return ESP_OK;
}

/* Handler for puts */
static esp_err_t puts_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	bool parse = true;
	cJSON *root = cJSON_Parse(buf);

	// Search char item
	char str[64];
	cJSON* state = cJSON_GetObjectItem(root, "str");
	if (state) {
		strcpy(str, cJSON_GetObjectItem(root, "str")->valuestring);
	} else {
		ESP_LOGE(__FUNCTION__, "str item not found");
		parse = false;
	}

	cJSON_Delete(root);
	if (parse) {
		ESP_LOGI(__FUNCTION__, "str=[%s]", str);
		httpd_resp_sendstr(req, "puts successfully\n");
		PARAMETER_t paramBuf;
		strcpy(paramBuf.function, "puts");
		strcpy(paramBuf.str, str);
		if (xQueueSend(xQueueParameter, &paramBuf, 10) != pdPASS) {
			ESP_LOGE(__FUNCTION__, "xQueueSend fail");
		}
	} else {
		ESP_LOGE(__FUNCTION__, "Request parameter not correct [%s]", buf);
		httpd_resp_sendstr(req, "Request parameter not correct");
	}

	return ESP_OK;
}

/* Handler for backlight */
static esp_err_t backlight_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	bool parse = true;
	cJSON *root = cJSON_Parse(buf);

	// Search char item
	char wk[32];
	bool backlight = false;
	cJSON* state = cJSON_GetObjectItem(root, "backlight");
	if (state) {
		strcpy(wk, cJSON_GetObjectItem(root, "backlight")->valuestring);
		if (strcmp (wk, "on") == 0) {
			backlight = true;
		} else if (strcmp (wk, "off") == 0) {
			backlight = false;
		} else {
			ESP_LOGW(__FUNCTION__, "backlight is not on/off, so off is applied");
			backlight = false;
		}
	} else {
		ESP_LOGE(__FUNCTION__, "backlight item not found");
		parse = false;
	}

	cJSON_Delete(root);
	if (parse) {
		ESP_LOGI(__FUNCTION__, "backlight=%d", backlight);
		httpd_resp_sendstr(req, "backlight successfully\n");
		PARAMETER_t paramBuf;
		strcpy(paramBuf.function, "backlight");
		paramBuf.backlight = backlight;
		if (xQueueSend(xQueueParameter, &paramBuf, 10) != pdPASS) {
			ESP_LOGE(__FUNCTION__, "xQueueSend fail");
		}
	} else {
		ESP_LOGE(__FUNCTION__, "Request parameter not correct [%s]", buf);
		httpd_resp_sendstr(req, "Request parameter not correct");
	}

	return ESP_OK;
}

/* favicon get handler */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "favicon_get_handler req->uri=[%s]", req->uri);
	return ESP_OK;
}

/* Function to start the file server */
esp_err_t start_server(int port)
{
	rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
	if (rest_context == NULL) {
		ESP_LOGE(__FUNCTION__, "No memory for rest context");
		while(1) { vTaskDelay(1); }
	}

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = port;

	/* Use the URI wildcard matching function in order to
	 * allow the same handler to respond to multiple different
	 * target URIs which match the wildcard scheme */
	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGD(__FUNCTION__, "Starting HTTP Server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "Failed to start file server!");
		return ESP_FAIL;
	}

	/* URI handler for root */
	httpd_uri_t root = {
		.uri		 = "/",	// Match all URIs of type /path/to/file
		.method		 = HTTP_GET,
		.handler	 = root_get_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};
	httpd_register_uri_handler(server, &root);

	/* URI handler for control */
	httpd_uri_t control_uri = {
		.uri		 = "/api/control",
		.method		 = HTTP_POST,
		.handler	 = control_handler, 
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &control_uri);

	/* URI handler for clear */
	httpd_uri_t clear_uri = {
		.uri		 = "/api/clear",
		.method		 = HTTP_POST,
		.handler	 = clear_handler, 
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &clear_uri);

	/* URI handler for gotoxy */
	httpd_uri_t gotoxy_uri = {
		.uri		 = "/api/gotoxy",
		.method		 = HTTP_POST,
		.handler	 = gotoxy_handler, 
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &gotoxy_uri);

	/* URI handler for putc */
	httpd_uri_t putc_uri = {
		.uri		 = "/api/putc",
		.method		 = HTTP_POST,
		.handler	 = putc_handler,
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &putc_uri);

	/* URI handler for puts */
	httpd_uri_t puts_uri = {
		.uri		 = "/api/puts",
		.method		 = HTTP_POST,
		.handler	 = puts_handler,
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &puts_uri);

	/* URI handler for backlight */
	httpd_uri_t backlight_uri = {
		.uri		 = "/api/backlight",
		.method		 = HTTP_POST,
		.handler	 = backlight_handler,
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &backlight_uri);

	/* URI handler for favicon.ico */
	httpd_uri_t _favicon_get_handler = {
		.uri		 = "/favicon.ico",
		.method		 = HTTP_GET,
		.handler	 = favicon_get_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};
	httpd_register_uri_handler(server, &_favicon_get_handler);

	return ESP_OK;
}

#define WEB_PORT 8080

void http_server(void *pvParameters)
{
	char *task_parameter = (char *)pvParameters;
	ESP_LOGI(TAG, "Start task_parameter=%s", task_parameter);
	char url[64];
	sprintf(url, "http://%s:%d", task_parameter, WEB_PORT);
	ESP_LOGI(TAG, "Starting HTTP server on %s", url);
	ESP_ERROR_CHECK(start_server(WEB_PORT));

	while(1) {
		vTaskDelay(1);
	}

	// Never reach here
	ESP_LOGI(TAG, "finish");
	vTaskDelete(NULL);
}
