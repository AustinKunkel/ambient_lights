#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "../../cJSON/cJSON.h"
#include "led_functions.h"
#include "main.h"
#include "csv_control.h"
#include "server.h"

static const struct lws_protocols protocols[];

#define WEB_ROOT "./led_control/www"  // Path for static files

#define LED_SETTINGS_FILENAME   "led_control/data/led_settings.csv"
#define LED_SETTINGS_HEADER     "brightness, color, capture screen, sount react, fx num, count, id\n"

#define SC_SETTINGS_FILENAME    "led_control/data/sc_settings.csv"
#define SC_SETTINGS_HEADER      "V offset, H offset, avg color, left count, right count, top count, bottom count, res x, res y, blend depth, blend mode\n"

#define USER_COLORS_FILENAME    "led_control/data/user_colors.csv"
#define USER_COLORS_HEADER      "color\n"
#define MAX_USER_COLORS         20

uint32_t user_colors[MAX_USER_COLORS];
int user_color_count = 0;

#define PORT            80
#define MAX_CLIENTS     10
#define MAX_REQUEST_GAP 100 // milliseconds

struct lws_context *context;

typedef struct per_session_data {
    struct lws *wsi;
} per_session_data_t;

per_session_data_t* clients[MAX_CLIENTS];
int client_count;


int parse_led_settings_data_to_string(char *str) {
    return sprintf(str, "%d,#%06X,%d,%d,%d,%d,%d",
        led_settings.brightness,
        led_settings.color,
        led_settings.capture_screen,
        led_settings.sound_react,
        led_settings.fx_num,
        led_settings.count,
        led_settings.id
    );
}

int parse_screen_settings_data_to_string(char *str) {
    return sprintf(str, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f",
        sc_settings.v_offset,
        sc_settings.h_offset,
        sc_settings.avg_color,
        sc_settings.left_count,
        sc_settings.right_count,
        sc_settings.top_count,
        sc_settings.bottom_count,
        sc_settings.res_x,
        sc_settings.res_y,
        sc_settings.blend_depth,
        sc_settings.blend_mode,
        sc_settings.auto_offset,
        sc_settings.transition_rate
    );
}

int initialize_led_settings() {
    char data_line[512];
    printf("reading led_settings.csv...\n");
    if(read_one_line(LED_SETTINGS_FILENAME, data_line, sizeof(data_line)) == 0)
    {
        char *line_ptr = data_line;
        printf("Setting led settings variables...\n");
        led_settings.brightness = atoi(next_token(&line_ptr));
        printf("Brightness: %d\n", led_settings.brightness);
        char *hex = next_token(&line_ptr);
        //printf("Hex string: %s\n", hex);
        if(hex) {
            char *hex_ptr = (hex[0] == '#') ? hex + 1 : hex;
            //printf("Hex ptr: %s\n", hex_ptr);
            led_settings.color = (int)strtol(hex_ptr, NULL, 16);
        } else {
            led_settings.color = 0xDFC57B;
        }
        //printf("Color: #%06lX\n", led_settings.color);
        led_settings.capture_screen = atoi(next_token(&line_ptr));
        //printf("Capture screen: %d\n", led_settings.capture_screen);
        led_settings.sound_react = atoi(next_token(&line_ptr));
        //printf("sound react: %d\n", led_settings.sound_react);
        led_settings.fx_num = atoi(next_token(&line_ptr));
        //printf("fx num: %d\n", led_settings.fx_num);
        led_settings.count = atoi(next_token(&line_ptr));
        //printf("count: %d\n", led_settings.count);
        led_settings.id = atoi(next_token(&line_ptr));
        //printf("id: %d\n", led_settings.id);
        char buffer[256];
        parse_led_settings_data_to_string(buffer);
        printf("Current LED settings: %s\n", buffer);
        return 0;
    } else {
        perror("Unable to read from led_settings.csv!!\n");
        return 1;
    }
}

int initialize_sc_settings() {
    char data_line[512];
  printf("reading sc_settings.csv...\n");
  if(read_one_line(SC_SETTINGS_FILENAME, data_line, sizeof(data_line)) == 0)
  {
    char *line_ptr = data_line;
    printf("Setting sc settings variables...\n");
    sc_settings.v_offset = atoi(next_token(&line_ptr));
    printf("v_offset: %d\t", sc_settings.v_offset);
    
    sc_settings.h_offset = atoi(next_token(&line_ptr));
    printf("h_offset: %d\t", sc_settings.h_offset);
    
    sc_settings.avg_color = atoi(next_token(&line_ptr));
    printf("avg_color: %d\t", sc_settings.avg_color);
    
    sc_settings.left_count = atoi(next_token(&line_ptr));
    printf("left_count: %d\t", sc_settings.left_count);
    
    sc_settings.right_count = atoi(next_token(&line_ptr));
    printf("right_count: %d\t", sc_settings.right_count);
    
    sc_settings.top_count = atoi(next_token(&line_ptr));
    printf("top_count: %d\t", sc_settings.top_count);
    
    sc_settings.bottom_count = atoi(next_token(&line_ptr));
    printf("bottom_count: %d\t", sc_settings.bottom_count);
    
    sc_settings.res_x = atoi(next_token(&line_ptr));
    printf("res_x: %d\t", sc_settings.res_x);
    
    sc_settings.res_y = atoi(next_token(&line_ptr));
    printf("res_y: %d\t", sc_settings.res_y);
    
    sc_settings.blend_depth = atoi(next_token(&line_ptr));
    printf("blend_depth: %d\t", sc_settings.blend_depth);
    
    sc_settings.blend_mode = atoi(next_token(&line_ptr));
    printf("blend_mode: %d\t", sc_settings.blend_mode);    

    sc_settings.auto_offset = atoi(next_token(&line_ptr));
    printf("Auto offset: %d\t", sc_settings.auto_offset);

    sc_settings.transition_rate = atof(next_token(&line_ptr));
    printf("Transition Rate: %.2f\n", sc_settings.transition_rate);

    return 0;
  } else {
    perror("Unable to read from sc_settings.csv!!\n");
    return 1;
  }
}

void handle_get_led_settings(struct lws *wsi);
void handle_set_led_settings(struct lws *wsi, cJSON *data);
void handle_get_capt_settings(struct lws *wsi);
void handle_set_capt_settings(struct lws *wsi, cJSON *data);
void handle_get_user_colors(struct lws *wsi);
void handle_set_user_colors(struct lws *wsi, cJSON *json);
// void handle_set_color(struct lws *wsi, cJSON *data);

/**
 * Helper function to distribute actions to their correct handler functions
 */
void dispatch_action(struct lws *wsi, const char *action, cJSON *data) {
    if (strcmp(action, "get_led_settings") == 0) {
        handle_get_led_settings(wsi);
    } else if (strcmp(action, "set_led_settings") == 0) {
        handle_set_led_settings(wsi, data);
    } else if(strcmp(action, "get_capt_settings") == 0) {
        handle_get_capt_settings(wsi);
    } else if (strcmp(action, "set_capt_settings") == 0) {
        handle_set_capt_settings(wsi, data);
    } else if (strcmp(action, "get_user_colors") == 0) {
        handle_get_user_colors(wsi);
    } else if (strcmp(action, "set_user_colors") == 0) {
        handle_set_user_colors(wsi, data);
    } else {
        printf("Unknown action: %s\n", action);
    }
}

// --- Main WebSocket callback ---
static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
    per_session_data_t *psd = (per_session_data_t*)user;
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        printf("WebSocket connection established\n");
        psd->wsi = wsi;

        if(client_count < MAX_CLIENTS) {
            handle_get_led_settings(wsi);  // Send LED settings to the client
            handle_get_capt_settings(wsi);
            handle_get_user_colors(wsi);
            clients[client_count++] = psd;
        }
        break;

    case LWS_CALLBACK_RECEIVE: {

        printf("Received message: %.*s\n", (int)len, (char *)in);

        cJSON *json = cJSON_Parse((char *)in);
        if (!json) {
            printf("Invalid JSON\n");
            break;
        }

        cJSON *action = cJSON_GetObjectItem(json, "action");
        cJSON *data = cJSON_GetObjectItem(json, "data");

        if (cJSON_IsString(action)) {
            dispatch_action(wsi, action->valuestring, data);
        }

        cJSON_Delete(json);
        break;
    }
    case LWS_CALLBACK_CLOSED:
        for (int i = 0; i < client_count; i++) {
            if (clients[i] == psd) {
                // Shift remaining clients
                for (int j = i; j < client_count - 1; j++) {
                    clients[j] = clients[j + 1];
                }
                client_count--;
                break;
            }
        }
        printf("WebSocket connection closed\n");
        break;

    default:
        break;
    }
    return 0;
}

void handle_get_led_settings(struct lws *wsi) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "action", "get_led_settings");
    cJSON_AddStringToObject(root, "status", "ok");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "brightness", led_settings.brightness);

    char color_str[8];
    snprintf(color_str, sizeof(color_str), "#%06X", led_settings.color);
    cJSON_AddStringToObject(data, "color", color_str);

    cJSON_AddNumberToObject(data, "capture_screen", led_settings.capture_screen);
    cJSON_AddNumberToObject(data, "sound_react", led_settings.sound_react);
    cJSON_AddNumberToObject(data, "fx_num", led_settings.fx_num);
    cJSON_AddNumberToObject(data, "count", led_settings.count);
    cJSON_AddNumberToObject(data, "id", led_settings.id);

    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    unsigned char buffer[LWS_PRE + 1024];
    size_t json_len = strlen(json_str);
    memcpy(&buffer[LWS_PRE], json_str, json_len);
    lws_write(wsi, &buffer[LWS_PRE], json_len, LWS_WRITE_TEXT);

    free(json_str);
    cJSON_Delete(root);
}

void handle_set_led_settings(struct lws *wsi, cJSON *json) {
    if (!cJSON_IsObject(json)) return;

    if(!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return;
    }
    cJSON *brightness = cJSON_GetObjectItemCaseSensitive(json, "brightness");
    cJSON *color = cJSON_GetObjectItemCaseSensitive(json, "color");
    cJSON *capture_screen = cJSON_GetObjectItemCaseSensitive(json, "capture_screen");
    cJSON *sound_react = cJSON_GetObjectItemCaseSensitive(json, "sound_react");
    cJSON *fx_num = cJSON_GetObjectItemCaseSensitive(json, "fx_num");
    cJSON *count = cJSON_GetObjectItemCaseSensitive(json, "count");
    cJSON *id = cJSON_GetObjectItemCaseSensitive(json, "id");

    LEDSettings temp_settings = led_settings; // used to compare passed values with current led settings

    if (cJSON_IsNumber(brightness)) temp_settings.brightness = brightness->valueint;
    if (cJSON_IsString(color) && color->valuestring) {
        const char *hex = color->valuestring;
        temp_settings.color = (int)strtol(hex[0] == '#' ? hex + 1 : hex, NULL, 16);
    }
    if (cJSON_IsNumber(capture_screen)) temp_settings.capture_screen = capture_screen->valueint;
    if (cJSON_IsNumber(sound_react)) temp_settings.sound_react = sound_react->valueint;
    if (cJSON_IsNumber(fx_num)) temp_settings.fx_num = fx_num->valueint;
    if (cJSON_IsNumber(count)) temp_settings.count = count->valueint;
    if (cJSON_IsNumber(id)) temp_settings.id = id->valueint;

    int just_brightness = 0;

    if(temp_settings.color == led_settings.color &&
        temp_settings.capture_screen == led_settings.capture_screen &&
        temp_settings.sound_react == led_settings.sound_react &&
        temp_settings.fx_num == led_settings.fx_num &&
        temp_settings.count == led_settings.count &&
        temp_settings.id == led_settings.id)
     {
     // if only the brightness could be different, no need to stop any services,
     // just change brightness
         led_settings.brightness = temp_settings.brightness;
         update_led_vars();
         just_brightness = 1;
     } else { // some other variable has changed, we will update led_settings
         led_settings.brightness = temp_settings.brightness;
         led_settings.color = temp_settings.color;
         led_settings.capture_screen = temp_settings.capture_screen;
         led_settings.sound_react = temp_settings.sound_react;
         led_settings.fx_num = temp_settings.fx_num;
         led_settings.count = temp_settings.count;
         led_settings.id = temp_settings.id;
     }

    char led_settings_str[256];
    parse_led_settings_data_to_string(led_settings_str);
    //printf("current led settings: %s\n", led_settings_str);
    if(write_data(LED_SETTINGS_FILENAME, LED_SETTINGS_HEADER, led_settings_str)) { // writes data to csv file
        printf("Failed to write led_settings\n");
    } else { // success
        if(!just_brightness) { // reset the led strip and everything
            update_leds();
        }
    }

    for(int  i = 0; i < client_count; i++) {
        struct lws *wsi = clients[i]->wsi;
        handle_get_led_settings(wsi); // write data to all active clients
    }
}

void color_to_hex(uint32_t color, char *buffer) {
    snprintf(buffer, 8, "#%02X%02X%02X",
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF);
}

void handle_get_capt_settings(struct lws *wsi) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "action", "get_capt_settings");
    cJSON_AddStringToObject(root, "status", "ok");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "v_offset", sc_settings.v_offset);
    cJSON_AddNumberToObject(data, "h_offset", sc_settings.h_offset);
    cJSON_AddNumberToObject(data, "avg_color", sc_settings.avg_color);
    cJSON_AddNumberToObject(data, "left_count", sc_settings.left_count);
    cJSON_AddNumberToObject(data, "right_count", sc_settings.right_count);
    cJSON_AddNumberToObject(data, "top_count", sc_settings.top_count);
    cJSON_AddNumberToObject(data, "bottom_count", sc_settings.bottom_count);
    cJSON_AddNumberToObject(data, "res_x", sc_settings.res_x);
    cJSON_AddNumberToObject(data, "res_y", sc_settings.res_y);
    cJSON_AddNumberToObject(data, "blend_depth", sc_settings.blend_depth);
    cJSON_AddNumberToObject(data, "blend_mode", sc_settings.blend_mode);
    cJSON_AddNumberToObject(data, "auto_offset", sc_settings.auto_offset);
    cJSON_AddNumberToObject(data, "transition_rate", sc_settings.transition_rate);

    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    unsigned char buffer[LWS_PRE + 1024];
    size_t json_len = strlen(json_str);
    memcpy(&buffer[LWS_PRE], json_str, json_len);
    lws_write(wsi, &buffer[LWS_PRE], json_len, LWS_WRITE_TEXT);

    free(json_str);
    cJSON_Delete(root);
}

void handle_set_capt_settings(struct lws *wsi, cJSON *json) {
    if (!cJSON_IsObject(json)) return;

    if(!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return;
    }

    cJSON *v_offset = cJSON_GetObjectItemCaseSensitive(json, "v_offset");
    cJSON *h_offset = cJSON_GetObjectItemCaseSensitive(json, "h_offset");    
    cJSON *avg_color = cJSON_GetObjectItemCaseSensitive(json, "avg_color");    
    cJSON *left_count = cJSON_GetObjectItemCaseSensitive(json, "left_count");
    cJSON *right_count = cJSON_GetObjectItemCaseSensitive(json, "right_count");
    cJSON *top_count = cJSON_GetObjectItemCaseSensitive(json, "top_count");
    cJSON *bottom_count = cJSON_GetObjectItemCaseSensitive(json, "bottom_count");
    cJSON *res_x = cJSON_GetObjectItemCaseSensitive(json, "res_x");
    cJSON *res_y = cJSON_GetObjectItemCaseSensitive(json, "res_y");
    cJSON *blend_depth = cJSON_GetObjectItemCaseSensitive(json, "blend_depth");
    cJSON *blend_mode = cJSON_GetObjectItemCaseSensitive(json, "blend_mode");
    cJSON *auto_offset = cJSON_GetObjectItemCaseSensitive(json, "auto_offset");
    cJSON *transition_rate = cJSON_GetObjectItemCaseSensitive(json, "transition_rate");

    if (cJSON_IsNumber(v_offset)) sc_settings.v_offset = v_offset->valueint;
    if (cJSON_IsNumber(h_offset)) sc_settings.h_offset = h_offset->valueint;
    if (cJSON_IsNumber(avg_color)) sc_settings.avg_color = avg_color->valueint;
    if (cJSON_IsNumber(left_count)) sc_settings.left_count = left_count->valueint;
    if (cJSON_IsNumber(right_count)) sc_settings.right_count = right_count->valueint;
    if (cJSON_IsNumber(top_count)) sc_settings.top_count = top_count->valueint;
    if (cJSON_IsNumber(bottom_count)) sc_settings.bottom_count = bottom_count->valueint;
    if (cJSON_IsNumber(res_x)) sc_settings.res_x = res_x->valueint;
    if (cJSON_IsNumber(res_y)) sc_settings.res_y = res_y->valueint;
    if (cJSON_IsNumber(blend_depth)) sc_settings.blend_depth = blend_depth->valueint;
    if (cJSON_IsNumber(blend_mode)) sc_settings.blend_mode = blend_mode->valueint;
    if (cJSON_IsNumber(auto_offset)) sc_settings.auto_offset = auto_offset->valueint;
    if (cJSON_IsNumber(transition_rate)) sc_settings.transition_rate = transition_rate->valuedouble;
    
    char capt_settings_str[512];
    parse_screen_settings_data_to_string(capt_settings_str);

    const char *response_text;
    if(write_data(SC_SETTINGS_FILENAME, SC_SETTINGS_HEADER, capt_settings_str)) {
        printf("Failed to write sc_settings\n");
    }
    update_leds();

    for(int  i = 0; i < client_count; i++) {
        struct lws *wsi = clients[i]->wsi;
        handle_get_capt_settings(wsi); // write data to all active clients
    }
}

void load_user_colors_from_file() {
    FILE *fp = fopen(USER_COLORS_FILENAME, "r");
    if (!fp) {
        perror("Unable to read from user_colors.csv!!\n");
        return;
    }
    char line[1024];
    int color_count = 0;
    int first_line = 1;
    while (fgets(line, sizeof(line), fp) && color_count < MAX_USER_COLORS) {
        // Skip header
        if (first_line && strstr(line, "color")) {
            first_line = 0;
            continue;
        }
        first_line = 0;
        char *token = strtok(line, ",\n");
        while (token && color_count < MAX_USER_COLORS) {
            if (token[0] == '#') token++;
            user_colors[color_count++] = (uint32_t)strtol(token, NULL, 16);
            token = strtok(NULL, ",\n");
        }
    }
    user_color_count = color_count;
    fclose(fp);
    printf("Loaded %d user colors\n", user_color_count);
}

void handle_get_user_colors(struct lws *wsi) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "action", "get_user_colors");
    cJSON_AddStringToObject(root, "status", "ok");

    cJSON *data_array = cJSON_CreateArray();

    for(int i = 0; i < user_color_count; i++) {
        char color_hex[8];
        color_to_hex(user_colors[i], color_hex);
        cJSON_AddItemToArray(data_array, cJSON_CreateString(color_hex));
    }

    cJSON_AddItemToObject(root, "data", data_array);

    char *json_str = cJSON_PrintUnformatted(root);
    unsigned char buffer[LWS_PRE + 2048];
    size_t json_len = strlen(json_str);
    memcpy(&buffer[LWS_PRE], json_str, json_len);
    lws_write(wsi, &buffer[LWS_PRE], json_len, LWS_WRITE_TEXT);

    free(json_str);
    cJSON_Delete(root);
}

void handle_set_user_colors(struct lws *wsi, cJSON *json) {
    if (!cJSON_IsObject(json)) return;

    if(!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return;
    }

    cJSON *colors = cJSON_GetObjectItemCaseSensitive(json, "colors");

    if (!cJSON_IsArray(colors)) {
        printf("Invalid colors array\n");
        return;
    }

    int color_count = 0;
    cJSON *color_item;
    cJSON_ArrayForEach(color_item, colors) {
        if (cJSON_IsString(color_item) && color_item->valuestring && color_count < MAX_USER_COLORS) {
            const char *hex = color_item->valuestring;
            if(hex[0] == '#') {
                hex++; // skip the '#' character
            }
            user_colors[color_count] = (uint32_t)strtol(hex, NULL, 16);
            color_count++;
        }
    }
    user_color_count = color_count;

    char user_colors_str[1024] = "";
    for(int i = 0; i < user_color_count; i++) {
        char color_hex[8];
        color_to_hex(user_colors[i], color_hex);
        strcat(user_colors_str, color_hex);
        if(i < user_color_count - 1) {
            strcat(user_colors_str, ",");
        }
    }

    if(write_data(USER_COLORS_FILENAME, USER_COLORS_HEADER, user_colors_str)) {
        printf("Failed to write user_colors\n");
    } else {
        printf("User colors updated successfully\n");
    }

    load_user_colors_from_file(); // load updated user colors from file

    for(int  i = 0; i < client_count; i++) {
        struct lws *wsi = clients[i]->wsi;
        handle_get_user_colors(wsi); // write data to all active clients
    }
}

int currently_sending_colors = 0;

void send_led_strip_colors(struct led_position* led_positions) {
    if(currently_sending_colors) {
        return; // prevent overlapping sends
    }
    currently_sending_colors = 1;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "action", "led_pixel_data");
    cJSON *data_array = cJSON_CreateArray();
    
    for(int i = 0; i < led_settings.count; i++) {
        cJSON *item = cJSON_CreateObject();
        char color_hex[8];
        
        // Send black if invalid, otherwise send the actual color
        if(led_positions[i].valid) {
            color_to_hex(led_positions[i].color, color_hex);
        } else {
            strcpy(color_hex, "#000000");  // Black for invalid LEDs
        }
        
        cJSON_AddStringToObject(item, "color", color_hex);
        cJSON_AddItemToArray(data_array, item);
    }
    
    cJSON_AddItemToObject(root, "data", data_array);
    char* json_str = cJSON_PrintUnformatted(root);
    
    for(int i = 0; i < client_count; i++) {
        struct lws *wsi = clients[i]->wsi;
        size_t len = strlen(json_str);
        unsigned char *buf = malloc(LWS_PRE + len);
        if(buf) {
            memcpy(buf + LWS_PRE, json_str, len);
            lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
            free(buf);
        }
    }
    
    cJSON_Delete(root);
    free(json_str);
    currently_sending_colors = 0;
}

// void handle_set_color(struct lws *wsi, cJSON *data) {
//     if (!cJSON_IsObject(data)) return;

//     cJSON *val = cJSON_GetObjectItem(data, "value");
//     if (cJSON_IsString(val)) {
//         unsigned int color;
//         if (sscanf(val->valuestring, "#%06X", &color) == 1) {
//             led_settings.color = color;
//             printf("Color set to #%06X\n", led_settings.color);
//             handle_get_settings(wsi); // echo updated settings
//         }
//     }
// }


// HTTP callback function for serving static files
static int http_callback(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_HTTP: {
            printf("Received http request\n");
            // Get the requested URI
            const char *requested_uri = (const char *)in;

            if (strstr(requested_uri, "..")) {
                lws_return_http_status(wsi, HTTP_STATUS_FORBIDDEN, NULL);
                return -1;
            }

            // If the requested file is not specified, serve index.html
            if (strcmp(requested_uri, "/") == 0) {
            requested_uri = "/index.html";
            }


            // Build the full file path
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, requested_uri);

            // Check if the requested file exists
            struct stat file_stat;
            if (stat(file_path, &file_stat) == -1) {
            // If file doesn't exist, return 404
            lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL);
            return -1;
            }

            printf("File size: %ld\n", file_stat.st_size);


            // Serve the requested file
            const char *content_type;
            const char *ext = strrchr(requested_uri, '.');
            if (!ext) ext = "";
            if (strcmp(ext, ".html") == 0) content_type = "text/html";
            else if (strcmp(ext, ".css") == 0) content_type = "text/css";
            else if (strcmp(ext, ".js") == 0) content_type = "application/javascript";
            else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) content_type = "image/jpeg";
            else if (strcmp(ext, ".png") == 0) content_type = "image/png";
            else if (strcmp(ext, ".gif") == 0) content_type = "image/gif";
            else content_type = "application/octet-stream";

            // lws_return_http_status(wsi, HTTP_STATUS_OK, "Content-Type: text/html\r\n\r\n");
            // const char *response = "<html><body><h1>Hello, world!</h1></body></html>";
            // lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_HTTP);

            //Serve the file with NULL for mime_type and extra_headers

            printf("Serving file: %s\n", file_path);
            if (lws_serve_http_file(wsi, file_path, content_type, NULL, file_stat.st_size) < 0) {
                return -1;
            }
        break;
    }
    default:
        break;
    }

    return 0;
}

// Define the WebSocket protocol
static const struct lws_protocols protocols[] = {
    {
        "http",       // protocol name
        http_callback,  // callback function
        0,             // per session data size
        1024,          // maximum frame size
    }, {
        .name = "websocket",
        .callback = websocket_callback,
        .per_session_data_size = sizeof(per_session_data_t),
    },
    { NULL, NULL, 0, 0 }  // end of protocols list
};

// Create the server context for `libwebsockets`
static struct lws_context *create_server_context()
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = PORT;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    // Create the server context
    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Error creating server context\n");
        return NULL;
    }

    return context;
}


void stop_server(int signo) {
    set_brightness(0);
    show_strip();
    printf("\nStopping server...\n");
    lws_context_destroy(context);
    exit(0);
}

int main(void)
{
    signal(SIGINT, stop_server);
    signal(SIGTERM, stop_server);
    per_session_data_t* clients[MAX_CLIENTS] = {0};
    client_count = 0;

    context = create_server_context();
    if (!context) {
        return -1;
    }

    if(initialize_led_settings()) {
        printf("Error initializing Led settings!\n");
        return 1;
    }

    if(initialize_sc_settings()) {
        printf("Error initialized sc_settings!\n");
        return 1;
    }

    load_user_colors_from_file();

    printf("Server running on localhost:%d/\n", PORT);
    printf("Press CTRL+C to stop the server.\n");

    if(setup_strip(led_settings.count) != 0) {
        printf("Problem setting up strip...\n");
        return 1;
    }

    // set_strip_32int_color(led_settings.color);
    // set_brightness(led_settings.brightness);
    // show_strip();
    update_leds();

    // Main event loop to process connections
    while (1) {
        lws_service(context, 100);
    }

    set_brightness(0);
    show_strip();
    lws_context_destroy(context);
    return 0;
}
